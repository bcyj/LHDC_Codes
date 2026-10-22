#include <audio_math/numeric_bits.h>
/* Fixed-point complex FFT uses iterative butterflies and per-stage scaling; fixed decomposition avoids recursion and large scratch. */
#include "../../include/dsp/fft_fixed32.h"
#include <stddef.h>
#include <string.h>

/* Interpret modulo-2^32 bits without out-of-range unsigned-to-signed conversion. */
#include "fft_fixed32.h"

/* Merge two subtransforms, shifting right once before rotation and addition/subtraction. */
static void butterfly2(LhdcFftFixed32Complex *out, int m, int stride,
                       const LhdcFftFixed32Complex *twiddles) {
    for (int k = 0; k < m; ++k) {
        LhdcFftFixed32Complex a = shifted(out[k], 1),
                              b = rotate(shifted(out[m + k], 1), twiddles[k * stride]);
        out[k] = plus(a, b);
        out[m + k] = minus(a, b);
    }
}
/* Merge four subtransforms; the direction flag selects the imaginary-unit rotation. */
static void butterfly4(LhdcFftFixed32Complex *out, int m, int stride,
                       const LhdcFftFixed32Plan *plan) {
    for (int k = 0; k < m; ++k) {
        LhdcFftFixed32Complex a = shifted(out[k], 2);
        LhdcFftFixed32Complex b = rotate(shifted(out[m + k], 2), plan->twiddles[k * stride]);
        LhdcFftFixed32Complex c =
            rotate(shifted(out[2 * m + k], 2), plan->twiddles[2 * k * stride]);
        LhdcFftFixed32Complex d =
            rotate(shifted(out[3 * m + k], 2), plan->twiddles[3 * k * stride]);
        LhdcFftFixed32Complex even = plus(a, c), odd = plus(b, d), even_difference = minus(a, c),
                              odd_difference = minus(b, d);
        out[k] = plus(even, odd);
        out[2 * m + k] = minus(even, odd);
        if (plan->inverse) {
            out[m + k] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                                 add(even_difference.i, odd_difference.r)};
            out[3 * m + k] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                                     sub(even_difference.i, odd_difference.r)};
        } else {
            out[m + k] = (LhdcFftFixed32Complex){add(even_difference.r, odd_difference.i),
                                                 sub(even_difference.i, odd_difference.r)};
            out[3 * m + k] = (LhdcFftFixed32Complex){sub(even_difference.r, odd_difference.i),
                                                     add(even_difference.i, odd_difference.r)};
        }
    }
}
/* Merge three subtransforms, preserving reciprocal scaling, complex rotation, and shifted sum/difference order. */
static void butterfly3(LhdcFftFixed32Complex *out, int m, int stride,
                       const LhdcFftFixed32Complex *twiddles) {
    int32_t sine = twiddles[stride * m].i;
    for (int k = 0; k < m; ++k) {
        LhdcFftFixed32Complex a = divided(out[k], 3);
        LhdcFftFixed32Complex b = rotate(divided(out[m + k], 3), twiddles[k * stride]);
        LhdcFftFixed32Complex c = rotate(divided(out[2 * m + k], 3), twiddles[2 * k * stride]);
        LhdcFftFixed32Complex sum = plus(b, c), difference = minus(b, c),
                              base = minus(a, shifted(sum, 1));
        difference =
            (LhdcFftFixed32Complex){multiply(difference.r, sine), multiply(difference.i, sine)};
        out[k] = plus(a, sum);
        out[m + k] = (LhdcFftFixed32Complex){sub(base.r, difference.i), add(base.i, difference.r)};
        out[2 * m + k] =
            (LhdcFftFixed32Complex){add(base.r, difference.i), sub(base.i, difference.r)};
    }
}
/* Merge five subtransforms after separately rounding scalar rotation terms. */
static void butterfly5(LhdcFftFixed32Complex *out, int m, int stride,
                       const LhdcFftFixed32Complex *twiddles) {
    LhdcFftFixed32Complex ya = twiddles[stride * m], yb = twiddles[2 * stride * m];
    for (int k = 0; k < m; ++k) {
        LhdcFftFixed32Complex a = divided(out[k], 5);
        LhdcFftFixed32Complex b = rotate(divided(out[m + k], 5), twiddles[k * stride]);
        LhdcFftFixed32Complex c = rotate(divided(out[2 * m + k], 5), twiddles[2 * k * stride]);
        LhdcFftFixed32Complex d = rotate(divided(out[3 * m + k], 5), twiddles[3 * k * stride]);
        LhdcFftFixed32Complex e = rotate(divided(out[4 * m + k], 5), twiddles[4 * k * stride]);
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
        out[k] = plus(plus(a, outer_sum), inner_sum);
        out[m + k] = minus(first, first_rotation);
        out[4 * m + k] = plus(first, first_rotation);
        out[2 * m + k] = plus(second, second_rotation);
        out[3 * m + k] = minus(second, second_rotation);
    }
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
    int stages;
    static const int radix60[] = {4, 3, 5}, radix120[] = {4, 2, 3, 5}, radix240[] = {4, 4, 3, 5},
                     radix480[] = {4, 4, 2, 3, 5};
    switch (plan->length) {
    case 60:
        radices = radix60;
        stages = 3;
        break;
    case 120:
        radices = radix120;
        stages = 4;
        break;
    case 240:
        radices = radix240;
        stages = 4;
        break;
    case 480:
        radices = radix480;
        stages = 5;
        break;
    default:
        return LHDC_FFT_FIXED32_UNSUPPORTED_LENGTH;
    }
    size_t bytes = (size_t)plan->length * sizeof(LhdcFftFixed32Complex);
    if (!input || !output || !plan->twiddles || overlaps(input, output, bytes) ||
        overlaps(plan->twiddles, output, bytes))
        return LHDC_FFT_FIXED32_INVALID_ARGUMENT;
    int leaves = plan->length / 5;
    for (int leaf = 0; leaf < leaves; ++leaf) {
        int index = leaf, position = 0, stride = leaves;
        for (int stage = stages - 2; stage >= 0; --stage) {
            stride /= radices[stage];
            position += (index % radices[stage]) * stride;
            index /= radices[stage];
        }
        for (int j = 0; j < 5; ++j)
            output[5 * leaf + j] = input[position + j * leaves];
        butterfly5(output + 5 * leaf, 1, leaves, plan->twiddles);
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
