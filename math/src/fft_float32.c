#include <audio_math/math.h>
#include <audio_math/float_sqrt.h>
#include <audio_math/bits.h>
/* Shared floating KISS FFT: version adapters pass read-only plans without copying state. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/fft_float32.h>
#include <audio_math/constants.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Fixed-length forward/inverse twiddles are generated at build time without runtime initialization or mutable globals. */
#include <audio_math/fft_float32_tables.h>
#include "fft_float32.h"

/* Merge two subtransforms with a radix-two butterfly. */
static inline void audio_math_fft_bfly2(AudioMathComplex32 *Fout, size_t fstride, int32_t m,
                                  const AudioMathFftPlan *st) {
    const AudioMathComplex32 *tw1 = st->twiddles;
    for (int32_t k = 0; k < m; ++k, ++Fout, tw1 += fstride) {
        AudioMathComplex32 t = audio_math_fft_twiddle_product(Fout[m], *tw1);
        float imag = Fout->i;
        Fout[m].r = Fout->r - t.r;
        Fout[m].i = imag - t.i;
        Fout->r = t.r + Fout->r;
        Fout->i = t.i + Fout->i;
    }
}

/* Merge three subtransforms using cube roots of unity in a radix-three butterfly. */
static inline void audio_math_fft_bfly3(AudioMathComplex32 *Fout, size_t fstride, int32_t m,
                                  const AudioMathFftPlan *st) {
    const AudioMathComplex32 *tw1 = st->twiddles, *tw2 = st->twiddles;
    float epi3 = st->twiddles[m * fstride].i;
    for (int32_t k = 0; k < m; ++k, ++Fout, tw1 += fstride, tw2 += 2 * fstride) {
        AudioMathComplex32 scratch1 = audio_math_fft_twiddle_product(Fout[m], *tw1);
        AudioMathComplex32 scratch2 = audio_math_fft_twiddle_product(Fout[2 * m], *tw2);
        float imag = Fout->i;
        AudioMathComplex32 sum = {scratch1.r + scratch2.r, scratch1.i + scratch2.i};
        Fout[m].r = Fout->r - sum.r * 0.5f;
        Fout[m].i = imag - sum.i * 0.5f;
        imag = Fout->i;
        float difference_imag = epi3 * (scratch1.i - scratch2.i);
        float difference_real = epi3 * (scratch1.r - scratch2.r);
        Fout->r = sum.r + Fout->r;
        Fout->i = sum.i + imag;
        imag = Fout[m].i;
        Fout[2 * m].r = difference_imag + Fout[m].r;
        Fout[2 * m].i = imag - difference_real;
        imag = Fout[m].i;
        Fout[m].r = Fout[m].r - difference_imag;
        Fout[m].i = difference_real + imag;
    }
}

/* Merge four subtransforms with a direction-aware radix-four butterfly. */
static inline void audio_math_fft_bfly4(AudioMathComplex32 *Fout, size_t fstride, int32_t m,
                                  const AudioMathFftPlan *st) {
    const AudioMathComplex32 *tw1 = st->twiddles, *tw2 = st->twiddles, *tw3 = st->twiddles;
    for (int32_t k = 0; k < m;
         ++k, ++Fout, tw1 += fstride, tw2 += 2 * fstride, tw3 += 3 * fstride) {
        AudioMathComplex32 scratch0 = audio_math_fft_twiddle_product(Fout[m], *tw1);
        AudioMathComplex32 scratch1 = audio_math_fft_twiddle_product(Fout[2 * m], *tw2);
        AudioMathComplex32 scratch2 = audio_math_fft_twiddle_product(Fout[3 * m], *tw3);
        AudioMathComplex32 even_sum = {scratch1.r + Fout->r, scratch1.i + Fout->i};
        AudioMathComplex32 odd_sum = {scratch0.r + scratch2.r, scratch0.i + scratch2.i};
        AudioMathComplex32 even_difference = {Fout->r - scratch1.r, Fout->i - scratch1.i};
        Fout->i = even_sum.i;
        Fout->r = even_sum.r;
        Fout[2 * m].i = even_sum.i - odd_sum.i;
        Fout[2 * m].r = even_sum.r - odd_sum.r;
        float real = Fout->r;
        AudioMathComplex32 odd_difference = {scratch0.r - scratch2.r, scratch0.i - scratch2.i};
        Fout->i = odd_sum.i + Fout->i;
        Fout->r = odd_sum.r + real;
        if (st->inverse == 0) {
            Fout[m].r = even_difference.r + odd_difference.i;
            Fout[m].i = even_difference.i - odd_difference.r;
            Fout[3 * m].r = even_difference.r - odd_difference.i;
            Fout[3 * m].i = even_difference.i + odd_difference.r;
        } else {
            Fout[m].r = even_difference.r - odd_difference.i;
            Fout[m].i = even_difference.i + odd_difference.r;
            Fout[3 * m].r = even_difference.r + odd_difference.i;
            Fout[3 * m].i = even_difference.i - odd_difference.r;
        }
    }
}

/* Merge five subtransforms using fifth roots of unity in a radix-five butterfly. */
static inline void audio_math_fft_bfly5(AudioMathComplex32 *Fout, size_t fstride, int32_t m,
                                  const AudioMathFftPlan *st) {
    AudioMathComplex32 ya = st->twiddles[m * fstride], yb = st->twiddles[2 * m * fstride];
    for (int32_t u = 0; u < m; ++u, ++Fout) {
        AudioMathComplex32 scratch0 = *Fout;
        AudioMathComplex32 scratch1 = audio_math_fft_twiddle_product(Fout[m], st->twiddles[u * fstride]);
        AudioMathComplex32 scratch2 =
            audio_math_fft_twiddle_product(Fout[2 * m], st->twiddles[2 * u * fstride]);
        AudioMathComplex32 scratch3 =
            audio_math_fft_twiddle_product(Fout[3 * m], st->twiddles[3 * u * fstride]);
        AudioMathComplex32 scratch4 =
            audio_math_fft_twiddle_product(Fout[4 * m], st->twiddles[4 * u * fstride]);
        AudioMathComplex32 scratch7 = {scratch1.r + scratch4.r, scratch1.i + scratch4.i};
        AudioMathComplex32 scratch8 = {scratch2.r + scratch3.r, scratch2.i + scratch3.i};
        AudioMathComplex32 scratch9 = {scratch2.r - scratch3.r, scratch2.i - scratch3.i};
        AudioMathComplex32 scratch10 = {scratch1.r - scratch4.r, scratch1.i - scratch4.i};
        float cross_imag = -(scratch10.r * ya.i) - yb.i * scratch9.r;
        float sum_imag = scratch0.i + ya.r * scratch7.i + yb.r * scratch8.i;
        float sum_real = scratch0.r + ya.r * scratch7.r + yb.r * scratch8.r;
        float cross_real = ya.i * scratch10.i + yb.i * scratch9.i;
        Fout[m].i = sum_imag - cross_imag;
        float second_cross_imag = yb.i * scratch10.r - ya.i * scratch9.r;
        float second_cross_real = ya.i * scratch9.i - yb.i * scratch10.i;
        float second_sum_imag = scratch0.i + yb.r * scratch7.i + ya.r * scratch8.i;
        Fout[m].r = sum_real - cross_real;
        Fout[4 * m].r = cross_real + sum_real;
        Fout[2 * m].i = second_cross_imag + second_sum_imag;
        Fout[3 * m].i = second_sum_imag - second_cross_imag;
        float second_sum_real = scratch0.r + yb.r * scratch7.r + ya.r * scratch8.r;
        Fout->i = scratch0.i + (scratch8.i + scratch7.i);
        Fout->r = scratch0.r + (scratch8.r + scratch7.r);
        Fout[4 * m].i = cross_imag + sum_imag;
        Fout[2 * m].r = second_cross_real + second_sum_real;
        Fout[3 * m].r = second_sum_real - second_cross_real;
    }
}

/* Apply generic complex merging for radices without dedicated butterflies. */
static void audio_math_fft_bfly_generic(AudioMathComplex32 *Fout, size_t fstride, int32_t m, int32_t p,
                                  const AudioMathFftPlan *st) {
    AudioMathComplex32 *scratch = st->generic_scratch;
    if (!scratch)
        return;
    for (int32_t u = 0; u < m; ++u) {
        for (int32_t q = 0; q < p; ++q)
            scratch[q] = Fout[u + q * m];
        for (int32_t q1 = 0; q1 < p; ++q1) {
            int32_t k = u + q1 * m, twidx = 0;
            Fout[k] = scratch[0];
            for (int32_t q = 1; q < p; ++q) {
                twidx += (int32_t)(k * fstride);
                if (twidx >= st->length)
                    twidx -= st->length;
                AudioMathComplex32 tw = st->twiddles[twidx];
                Fout[k].r = Fout[k].r + (scratch[q].r * tw.r - scratch[q].i * tw.i);
                Fout[k].i = Fout[k].i + (tw.r * scratch[q].i + scratch[q].r * tw.i);
            }
        }
    }
}

#if AUDIO_MATH_FFT_ITERATIVE_STATIC
/* Gather leaf-ordered input and merge stagewise, preserving butterfly operation order. */
void audio_math_fft_work(AudioMathComplex32 *output, const AudioMathComplex32 *input, size_t twiddle_stride,
                        int32_t input_stride, const int32_t *factors, const AudioMathFftPlan *plan) {
    const int32_t length = factors[0] * factors[1];
    int stages = 0;
    do {
        ++stages;
    } while (factors[2 * stages - 1] != 1);
    /* Gather leaf inputs in mixed-radix digit-reversed order. */
    const uint16_t *order = audio_math_fft_permutation(length, factors);
    if (order) {
        const ptrdiff_t step = (ptrdiff_t)twiddle_stride * input_stride;
        for (int32_t out = 0; out < length; ++out)
            output[out] = input[(ptrdiff_t)order[out] * step];
    } else {
        for (int32_t out = 0; out < length; ++out) {
            int32_t remaining = out;
            size_t source = 0, stride = twiddle_stride;
            for (int stage = 0; stage < stages; ++stage) {
                int32_t sublength = factors[2 * stage + 1];
                source += (size_t)(remaining / sublength) * stride;
                remaining %= sublength;
                stride *= factors[2 * stage];
            }
            output[out] = input[(ptrdiff_t)source * input_stride];
        }
    }
    /* Blocks within one level are independent; finish that level before its parent without recursion frames. */
    for (int stage = stages - 1; stage >= 0; --stage) {
        int32_t radix = factors[2 * stage], sublength = factors[2 * stage + 1];
        int32_t block = radix * sublength;
        size_t stride = twiddle_stride * (size_t)(length / block);
        for (int32_t base = 0; base < length; base += block) {
            AudioMathComplex32 *out = output + base;
            switch (radix) {
            case 2:
                audio_math_fft_bfly2(out, stride, sublength, plan);
                break;
            case 3:
                audio_math_fft_bfly3(out, stride, sublength, plan);
                break;
            case 4:
                audio_math_fft_bfly4(out, stride, sublength, plan);
                break;
            case 5:
                if (sublength > 0)
                    audio_math_fft_bfly5(out, stride, sublength, plan);
                break;
            default:
                if (plan->allow_generic_radix)
                    audio_math_fft_bfly_generic(out, stride, sublength, radix, plan);
                break;
            }
        }
    }
}

#else
/* Recursively compute subtransforms and merge by radix into natural bin order. */
void audio_math_fft_work(AudioMathComplex32 *output, const AudioMathComplex32 *input, size_t twiddle_stride,
                        int32_t input_stride, const int32_t *factors, const AudioMathFftPlan *plan) {
    const int32_t radix = factors[0], subtransform_length = factors[1];
    if (subtransform_length == 1) {
        for (int32_t sample = 0; sample < radix; ++sample) {
            output[sample] = *input;
            input += twiddle_stride * input_stride;
        }
    } else {
        for (int32_t branch = 0; branch < radix; ++branch) {
            audio_math_fft_work(output + branch * subtransform_length, input, twiddle_stride * radix,
                               input_stride, factors + 2, plan);
            input += twiddle_stride * input_stride;
        }
    }
    switch (radix) {
    case 2:
        audio_math_fft_bfly2(output, twiddle_stride, subtransform_length, plan);
        break;
    case 3:
        audio_math_fft_bfly3(output, twiddle_stride, subtransform_length, plan);
        break;
    case 4:
        audio_math_fft_bfly4(output, twiddle_stride, subtransform_length, plan);
        break;
    case 5:
        if (subtransform_length > 0)
            audio_math_fft_bfly5(output, twiddle_stride, subtransform_length, plan);
        break;
    default:
        /* This adapter omits merges for other radices; its caller selects the appropriate path. */
        if (plan->allow_generic_radix)
            audio_math_fft_bfly_generic(output, twiddle_stride, subtransform_length, radix, plan);
        break;
    }
}

#endif

/* Record stage radices/remainders in order 4,2,3,then successive odd numbers. */
void audio_math_fft_factor(int32_t length, int32_t threshold, int32_t *factors) {
    int32_t radix = 4;
    do {
        while (length % radix) {
            radix = radix == 4 ? 2 : radix == 2 ? 3 : radix + 2;
            if (radix > threshold)
                radix = length;
        }
        length /= radix;
        *factors++ = radix;
        *factors++ = length;
    } while (length > 1);
}

/* Extract power-of-two real-FFT half-spectrum reconstruction coefficients from read-only complex twiddles. */
int audio_math_fftr_copy_twiddles(int32_t length, int inverse, AudioMathComplex32 *twiddles) {
    if (length < 4 || length > 512 || (length & (length - 1)) || !twiddles)
        return 0;
    size_t start = (size_t)length / 4 + 1;
    size_t bytes = (size_t)(length / 4) * sizeof *twiddles;
    if (length == 512) {
        memcpy(twiddles, audio_math_fft_512_twiddles[inverse != 0] + start, bytes);
        return 1;
    }
    const uint32_t *bits = audio_math_fft_static_twiddles(length, inverse != 0);
    if (!bits)
        return 0;
    memcpy(twiddles, bits + 2 * start, bytes);
    return 1;
}

/* Copy covered-length forward/inverse twiddles; generate other lengths from phase formulas.
 * Coefficient caching is independent of recursive/iterative scheduling; retain near-zero terms and signed zeros. */
void audio_math_fft_twiddles(int32_t length, int32_t inverse, AudioMathComplex32 *twiddles) {
    if (length == 512) {
        memcpy(twiddles, audio_math_fft_512_twiddles[inverse != 0], sizeof audio_math_fft_512_twiddles[0]);
        return;
    }
    const uint32_t *bits = audio_math_fft_static_twiddles(length, inverse != 0);
    if (bits) {
        memcpy(twiddles, bits, (size_t)length * sizeof *twiddles);
        return;
    }
    for (int32_t sample = 0; sample < length; ++sample) {
        double phase = (-AUDIO_MATH_TWO_PI_F64 * (double)sample) / (double)length;
        if (inverse)
            phase = -phase;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
        audio_math_sincos_bounded_f64_to_f32_soft(phase, &twiddles[sample].i, &twiddles[sample].r);
#else
        double sine, cosine;
        audio_math_sincos(phase, &sine, &cosine);
        twiddles[sample].r = (float)cosine;
        twiddles[sample].i = (float)sine;
#endif
    }
}

/* Find the next FFT length with prime factors only 2/3/5. */
int32_t audio_math_fft_next_fast_size(int32_t n) {
    for (;; ++n) {
        int32_t m = n;
        while (!(m & 1))
            m /= 2;
        while (m % 3 == 0)
            m /= 3;
        while (m % 5 == 0)
            m /= 5;
        if (m <= 1)
            return n;
    }
}

/* Execute AR's 512-point complex FFT in instance buffers, dividing inverse output by 512 without allocation/table generation. */
void audio_math_fft_512(float *interleaved, int inverse, AudioMathFft512Work *work) {
    const AudioMathFftPlan plan = {512, inverse != 0, audio_math_fft_512_twiddles[inverse != 0], 0, NULL};
    for (int sample = 0; sample < 512; ++sample)
        work->input[sample] = (AudioMathComplex32){interleaved[2 * sample], interleaved[2 * sample + 1]};
    audio_math_fft_work(work->output, work->input, 1, 1, audio_math_fft_512_factors, &plan);
    for (int sample = 0; sample < 512; ++sample) {
        interleaved[2 * sample] =
            inverse ? work->output[sample].r / 512.0f : work->output[sample].r;
        interleaved[2 * sample + 1] =
            inverse ? work->output[sample].i / 512.0f : work->output[sample].i;
    }
}
