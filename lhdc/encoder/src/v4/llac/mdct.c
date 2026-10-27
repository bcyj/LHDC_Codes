#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_sqrt.h>
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/llac/llac_const.h"
#include "../../../../common/include/dsp/dct4_float32.h"
#include <math.h>
#include <string.h>

#if defined(__SSE2__) ||                                                                          \
    (defined(_MSC_VER) && (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86))))
#include <emmintrin.h>
#define LHDC_V4_MDCT_USE_SSE2 1
#else
#define LHDC_V4_MDCT_USE_SSE2 0
#endif

/* MDCT encoding entry: transform one PCM frame into N spectral coefficients and advance overlap history. */
void v4f_llac_mdct_analyze(const LLAC_FLOAT *pcm, LLAC_FLOAT *coefficients, Mdct *mdct) {
    v4f_llac_mdct_analyze_overlap(pcm, coefficients, mdct);
}

/* Compute N MDCT coefficients from a 2N analysis block and retain unwindowed history for the next frame. */
void v4f_llac_mdct_analyze_overlap(const LLAC_FLOAT *pcm, LLAC_FLOAT *coefficients, Mdct *mdct) {
    const LLAC_INT frame_samples = mdct->length;
    const LLAC_INT half_frame = frame_samples / 2;
    HANDLE_IIS_FFT fft = mdct->dct.fft.handle;
    /* The FFT radix workspace has capacity for 1024 complex values and can first hold the 2N-point
     * analysis block. The block is dead after folding, so the DCT FFT stage may safely overwrite it. */
    LLAC_FLOAT *analysis_frame = fft->scratch->analysis;
    memset(analysis_frame, 0, LHDC_V4_LLAC_MDCT_TEMP_SAMPLES * sizeof(LLAC_FLOAT));

    /* 1. Assemble [previous-frame history | current N PCM samples | trailing zeros]. Normally
     * mem_length + leading_zeros = N; the low-delay window configuration selects history length.
     */
    memmove(analysis_frame, mdct->mem, (size_t)mdct->mem_length * sizeof(LLAC_FLOAT));
    memmove(analysis_frame + mdct->mem_length, pcm, (size_t)frame_samples * sizeof(LLAC_FLOAT));
    memset(analysis_frame + 2 * frame_samples - mdct->leading_zeros, 0,
           (size_t)mdct->leading_zeros * sizeof(LLAC_FLOAT));

    /* 2. Save raw history, then window the current 2N analysis block. */
    memmove(mdct->mem, analysis_frame + frame_samples,
            (size_t)mdct->mem_length * sizeof(LLAC_FLOAT));
#if LHDC_V4_MDCT_USE_SSE2
    LLAC_INT n = 0;
    const LLAC_INT total_samples = 2 * frame_samples;
    for (; n + 4 <= total_samples; n += 4) {
        __m128 a = _mm_loadu_ps(&analysis_frame[n]);
        __m128 w = _mm_loadu_ps(&mdct->window[n]);
        _mm_storeu_ps(&analysis_frame[n], _mm_mul_ps(a, w));
    }
    for (; n < total_samples; ++n)
        analysis_frame[n] *= mdct->window[n];
#else
    for (LLAC_INT n = 0; n < 2 * frame_samples; ++n)
        analysis_frame[n] *= mdct->window[n];
#endif

    /* 3. Time-domain alias folding maps 2N windowed samples to N DCT-IV inputs. With H=N/2,
     * the first half is the negative sum of the last two sections and the second half is the
     * reversed difference of the first two sections.
     */
#if LHDC_V4_MDCT_USE_SSE2
    n = 0;
    const __m128 sign_mask = _mm_set1_ps(-0.0f);
    for (; n + 4 <= half_frame; n += 4) {
        __m128 rev1_raw = _mm_loadu_ps(&analysis_frame[3 * half_frame - 4 - n]);
        __m128 rev1 = _mm_shuffle_ps(rev1_raw, rev1_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 fwd1 = _mm_loadu_ps(&analysis_frame[3 * half_frame + n]);
        __m128 c_first = _mm_sub_ps(_mm_xor_ps(rev1, sign_mask), fwd1);
        _mm_storeu_ps(&coefficients[n], c_first);

        __m128 fwd2 = _mm_loadu_ps(&analysis_frame[n]);
        __m128 rev2_raw = _mm_loadu_ps(&analysis_frame[2 * half_frame - 4 - n]);
        __m128 rev2 = _mm_shuffle_ps(rev2_raw, rev2_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 c_second = _mm_sub_ps(fwd2, rev2);
        _mm_storeu_ps(&coefficients[half_frame + n], c_second);
    }
    for (; n < half_frame; ++n) {
        coefficients[n] =
            -analysis_frame[3 * half_frame - 1 - n] - analysis_frame[3 * half_frame + n];
        coefficients[half_frame + n] = analysis_frame[n] - analysis_frame[2 * half_frame - 1 - n];
    }
#else
    for (LLAC_INT n = 0; n < half_frame; ++n) {
        coefficients[n] =
            -analysis_frame[3 * half_frame - 1 - n] - analysis_frame[3 * half_frame + n];
        coefficients[half_frame + n] = analysis_frame[n] - analysis_frame[2 * half_frame - 1 - n];
    }
#endif

    /* 4. The common core fully packs folded coefficients first, allowing in-place output without copying the analysis block. */
    v4f_llac_dct4_transform(&mdct->dct, coefficients, coefficients);
}

/* Compute an orthonormal N-point DCT-IV with an N/2-point complex FFT. Input is a windowed,
 * folded real sequence; output is reordered as forward evens and reversed odds.
 */
void v4f_llac_dct4_transform(const Dct4 *dct, const LLAC_FLOAT *folded, LLAC_FLOAT *coefficients) {
    HANDLE_IIS_FFT fft = dct->fft.handle;
    const LhdcDct4F32Kernel kernel = {dct->length,
                                      fft->kiss_factors,
                                      dct->twid1,
                                      dct->twid2,
                                      fft->kiss_twiddles,
                                      1,
                                      LHDC_DCT4_SCALE_AFTER_ROTATION,
                                      1.0f / audio_math_sqrt_f32((float)(dct->length / 2))};
    const size_t capacity = 2 * LHDC_V4_LLAC_FFT_WORKSPACE_POINTS;
    LhdcDct4F32Work work = {fft->kiss_input,     capacity, fft->kiss_output, capacity,
                            fft->scratch->radix, capacity};
    (void)lhdc_dct4_f32_execute(&kernel, folded, coefficients, &work);
}
