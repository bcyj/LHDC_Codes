#include <audio_math/numeric_bits.h>
/* Fixed-point complex FFT uses iterative butterflies and per-stage scaling; fixed decomposition avoids recursion and large scratch. */
#include "../../include/dsp/fft_fixed32.h"
#include <stddef.h>
#include <string.h>

/* Interpret modulo-2^32 bits without out-of-range unsigned-to-signed conversion. */
#include "fft_fixed32.h"

/* Precomputed leaf permutations for lengths 60, 120, 240, and 480 eliminate per-frame recursive
 * division and remainder operations. */
static const uint8_t leaf_positions_60[12] = {0, 4, 8, 1, 5, 9, 2, 6, 10, 3, 7, 11};

static const uint8_t leaf_positions_120[24] = {0, 8,  16, 4, 12, 20, 1, 9,  17, 5, 13, 21,
                                               2, 10, 18, 6, 14, 22, 3, 11, 19, 7, 15, 23};

static const uint8_t leaf_positions_240[48] = {
    0, 16, 32, 4, 20, 36, 8,  24, 40, 12, 28, 44, 1, 17, 33, 5, 21, 37, 9,  25, 41, 13, 29, 45,
    2, 18, 34, 6, 22, 38, 10, 26, 42, 14, 30, 46, 3, 19, 35, 7, 23, 39, 11, 27, 43, 15, 31, 47};

static const uint8_t leaf_positions_480[96] = {
    0, 32, 64, 16, 48, 80, 4, 36, 68, 20, 52, 84, 8,  40, 72, 24, 56, 88, 12, 44, 76, 28, 60, 92,
    1, 33, 65, 17, 49, 81, 5, 37, 69, 21, 53, 85, 9,  41, 73, 25, 57, 89, 13, 45, 77, 29, 61, 93,
    2, 34, 66, 18, 50, 82, 6, 38, 70, 22, 54, 86, 10, 42, 74, 26, 58, 90, 14, 46, 78, 30, 62, 94,
    3, 35, 67, 19, 51, 83, 7, 39, 71, 23, 55, 87, 11, 43, 75, 27, 59, 91, 15, 47, 79, 31, 63, 95};

/* Merge two subtransforms, shifting right once before rotation and addition/subtraction. */
static inline void butterfly2(LhdcFftFixed32Complex *out, int m, int stride,
                              const LhdcFftFixed32Complex *twiddles) {
    LhdcFftFixed32Complex *f0 = out;
    LhdcFftFixed32Complex *f1 = out + m;
    /* k = 0 */
    {
        LhdcFftFixed32Complex a = shifted(f0[0], 1);
        LhdcFftFixed32Complex b = rotate(shifted(f1[0], 1), twiddles[0]);
        f0[0] = plus(a, b);
        f1[0] = minus(a, b);
    }
    const LhdcFftFixed32Complex *tw1 = twiddles + stride;
    for (int k = 1; k < m; ++k, tw1 += stride) {
        LhdcFftFixed32Complex a = shifted(f0[k], 1);
        LhdcFftFixed32Complex b = rotate(shifted(f1[k], 1), *tw1);
        f0[k] = plus(a, b);
        f1[k] = minus(a, b);
    }
}

/* Merge four subtransforms; the direction flag selects the imaginary-unit rotation. */
static inline void butterfly4(LhdcFftFixed32Complex *out, int m, int stride,
                              const LhdcFftFixed32Plan *plan) {
    LhdcFftFixed32Complex *f0 = out;
    LhdcFftFixed32Complex *f1 = out + m;
    LhdcFftFixed32Complex *f2 = out + 2 * m;
    LhdcFftFixed32Complex *f3 = out + 3 * m;
    const LhdcFftFixed32Complex *tw = plan->twiddles;
    const int inv = plan->inverse;

    /* k = 0 */
    {
        LhdcFftFixed32Complex a = shifted(f0[0], 2);
        LhdcFftFixed32Complex b = rotate(shifted(f1[0], 2), tw[0]);
        LhdcFftFixed32Complex c = rotate(shifted(f2[0], 2), tw[0]);
        LhdcFftFixed32Complex d = rotate(shifted(f3[0], 2), tw[0]);
        LhdcFftFixed32Complex even = plus(a, c), odd = plus(b, d);
        LhdcFftFixed32Complex even_difference = minus(a, c), odd_difference = minus(b, d);
        f0[0] = plus(even, odd);
        f2[0] = minus(even, odd);
        if (inv) {
            f1[0] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                            add(even_difference.i, odd_difference.r)};
            f3[0] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                            sub(even_difference.i, odd_difference.r)};
        } else {
            f1[0] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                            sub(even_difference.i, odd_difference.r)};
            f3[0] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                            add(even_difference.i, odd_difference.r)};
        }
    }
    const LhdcFftFixed32Complex *tw1 = tw + stride;
    const LhdcFftFixed32Complex *tw2 = tw + 2 * stride;
    const LhdcFftFixed32Complex *tw3 = tw + 3 * stride;
    for (int k = 1; k < m; ++k, tw1 += stride, tw2 += 2 * stride, tw3 += 3 * stride) {
        LhdcFftFixed32Complex a = shifted(f0[k], 2);
        LhdcFftFixed32Complex b = rotate(shifted(f1[k], 2), *tw1);
        LhdcFftFixed32Complex c = rotate(shifted(f2[k], 2), *tw2);
        LhdcFftFixed32Complex d = rotate(shifted(f3[k], 2), *tw3);
        LhdcFftFixed32Complex even = plus(a, c), odd = plus(b, d);
        LhdcFftFixed32Complex even_difference = minus(a, c), odd_difference = minus(b, d);
        f0[k] = plus(even, odd);
        f2[k] = minus(even, odd);
        if (inv) {
            f1[k] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                            add(even_difference.i, odd_difference.r)};
            f3[k] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                            sub(even_difference.i, odd_difference.r)};
        } else {
            f1[k] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                            sub(even_difference.i, odd_difference.r)};
            f3[k] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                            add(even_difference.i, odd_difference.r)};
        }
    }
}

/* Merge three subtransforms, preserving reciprocal scaling, complex rotation, and shifted sum/difference order. */
static inline void butterfly3(LhdcFftFixed32Complex *out, int m, int stride,
                              const LhdcFftFixed32Complex *twiddles) {
    int32_t sine = twiddles[stride * m].i;
    LhdcFftFixed32Complex *f0 = out;
    LhdcFftFixed32Complex *f1 = out + m;
    LhdcFftFixed32Complex *f2 = out + 2 * m;
    /* k = 0 */
    {
        LhdcFftFixed32Complex a = divided(f0[0], 3);
        LhdcFftFixed32Complex b = rotate(divided(f1[0], 3), twiddles[0]);
        LhdcFftFixed32Complex c = rotate(divided(f2[0], 3), twiddles[0]);
        LhdcFftFixed32Complex sum = plus(b, c), difference = minus(b, c);
        LhdcFftFixed32Complex base = minus(a, shifted(sum, 1));
        difference =
            (LhdcFftFixed32Complex){multiply(difference.r, sine), multiply(difference.i, sine)};
        f0[0] = plus(a, sum);
        f1[0] = (LhdcFftFixed32Complex){sub(base.r, difference.i), add(base.i, difference.r)};
        f2[0] = (LhdcFftFixed32Complex){add(base.r, difference.i), sub(base.i, difference.r)};
    }
    const LhdcFftFixed32Complex *tw1 = twiddles + stride;
    const LhdcFftFixed32Complex *tw2 = twiddles + 2 * stride;
    for (int k = 1; k < m; ++k, tw1 += stride, tw2 += 2 * stride) {
        LhdcFftFixed32Complex a = divided(f0[k], 3);
        LhdcFftFixed32Complex b = rotate(divided(f1[k], 3), *tw1);
        LhdcFftFixed32Complex c = rotate(divided(f2[k], 3), *tw2);
        LhdcFftFixed32Complex sum = plus(b, c), difference = minus(b, c);
        LhdcFftFixed32Complex base = minus(a, shifted(sum, 1));
        difference =
            (LhdcFftFixed32Complex){multiply(difference.r, sine), multiply(difference.i, sine)};
        f0[k] = plus(a, sum);
        f1[k] = (LhdcFftFixed32Complex){sub(base.r, difference.i), add(base.i, difference.r)};
        f2[k] = (LhdcFftFixed32Complex){add(base.r, difference.i), sub(base.i, difference.r)};
    }
}

/* First-stage radix-five leaf butterfly. Gather directly from input and compute in place in output,
 * avoiding an intermediate copy. m=1 and k=0 is the only iteration. */
static inline void butterfly5_leaf(LhdcFftFixed32Complex *out, LhdcFftFixed32Complex in0,
                                   LhdcFftFixed32Complex in1, LhdcFftFixed32Complex in2,
                                   LhdcFftFixed32Complex in3, LhdcFftFixed32Complex in4,
                                   LhdcFftFixed32Complex ya, LhdcFftFixed32Complex yb,
                                   LhdcFftFixed32Complex tw0) {
    LhdcFftFixed32Complex a = divided(in0, 5);
    LhdcFftFixed32Complex b = rotate(divided(in1, 5), tw0);
    LhdcFftFixed32Complex c = rotate(divided(in2, 5), tw0);
    LhdcFftFixed32Complex d = rotate(divided(in3, 5), tw0);
    LhdcFftFixed32Complex e = rotate(divided(in4, 5), tw0);
    LhdcFftFixed32Complex outer_sum = plus(b, e), inner_sum = plus(c, d),
                          outer_difference = minus(b, e), inner_difference = minus(c, d);
    LhdcFftFixed32Complex first = {
        add(add(a.r, multiply(outer_sum.r, ya.r)), multiply(inner_sum.r, yb.r)),
        add(add(a.i, multiply(outer_sum.i, ya.r)), multiply(inner_sum.i, yb.r))};
    LhdcFftFixed32Complex first_rotation = {
        add(multiply(outer_difference.i, ya.i), multiply(inner_difference.i, yb.i)),
        sub(sub(0, multiply(outer_difference.r, ya.i)), multiply(inner_difference.r, yb.i))};
    LhdcFftFixed32Complex second = {
        add(add(a.r, multiply(outer_sum.r, yb.r)), multiply(inner_sum.r, ya.r)),
        add(add(a.i, multiply(outer_sum.i, yb.r)), multiply(inner_sum.i, ya.r))};
    LhdcFftFixed32Complex second_rotation = {
        sub(multiply(inner_difference.i, ya.i), multiply(outer_difference.i, yb.i)),
        sub(multiply(outer_difference.r, yb.i), multiply(inner_difference.r, ya.i))};
    out[0] = plus(plus(a, outer_sum), inner_sum);
    out[1] = minus(first, first_rotation);
    out[4] = plus(first, first_rotation);
    out[2] = plus(second, second_rotation);
    out[3] = minus(second, second_rotation);
}

/* Check contiguous complex ranges for overlap without constructing pointers beyond object bounds. */
static int overlaps(const void *a, const void *b, size_t bytes) {
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < bytes : x - y < bytes;
}

/* Apply leaf permutation and staged merges using fixed mixed-radix decomposition; no recursion or mutable global state. */
int lhdc_fft_fixed32_execute(const LhdcFftFixed32Plan *plan, const LhdcFftFixed32Complex *input,
                             LhdcFftFixed32Complex *output) {
    if (!plan)
        return LHDC_FFT_FIXED32_INVALID_ARGUMENT;
    const int *radices;
    const uint8_t *leaf_positions;
    int stages;
    static const int radix60[] = {4, 3, 5}, radix120[] = {4, 2, 3, 5}, radix240[] = {4, 4, 3, 5},
                     radix480[] = {4, 4, 2, 3, 5};
    switch (plan->length) {
    case 60:
        radices = radix60;
        leaf_positions = leaf_positions_60;
        stages = 3;
        break;
    case 120:
        radices = radix120;
        leaf_positions = leaf_positions_120;
        stages = 4;
        break;
    case 240:
        radices = radix240;
        leaf_positions = leaf_positions_240;
        stages = 4;
        break;
    case 480:
        radices = radix480;
        leaf_positions = leaf_positions_480;
        stages = 5;
        break;
    default:
        return LHDC_FFT_FIXED32_UNSUPPORTED_LENGTH;
    }
    size_t bytes = (size_t)plan->length * sizeof(LhdcFftFixed32Complex);
    if (!input || !output || !plan->twiddles || overlaps(input, output, bytes) ||
        overlaps(plan->twiddles, output, bytes))
        return LHDC_FFT_FIXED32_INVALID_ARGUMENT;

    const int leaves = plan->length / 5;
    const LhdcFftFixed32Complex ya = plan->twiddles[leaves];
    const LhdcFftFixed32Complex yb = plan->twiddles[2 * leaves];
    const LhdcFftFixed32Complex tw0 = plan->twiddles[0];

    for (int leaf = 0; leaf < leaves; ++leaf) {
        int pos = leaf_positions[leaf];
        butterfly5_leaf(output + 5 * leaf, input[pos], input[pos + leaves], input[pos + 2 * leaves],
                        input[pos + 3 * leaves], input[pos + 4 * leaves], ya, yb, tw0);
    }
    int m = 5;
    for (int stage = stages - 2; stage >= 0; --stage) {
        int radix = radices[stage], block = radix * m, stride = plan->length / block;
        for (int base = 0; base < plan->length; base += block) {
            if (radix == 2)
                butterfly2(output + base, m, stride, plan->twiddles);
            else if (radix == 3)
                butterfly3(output + base, m, stride, plan->twiddles);
            else
                butterfly4(output + base, m, stride, plan);
        }
        m = block;
    }
    return LHDC_FFT_FIXED32_OK;
}
