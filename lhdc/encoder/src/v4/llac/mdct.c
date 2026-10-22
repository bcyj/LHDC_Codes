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

/* MDCT encoding entry: transform one PCM frame into N spectral coefficients and advance overlap history. */
void v4f_llac_mdct_analyze(const LLAC_FLOAT *pcm, LLAC_FLOAT *coefficients, Mdct *mdct) {
    v4f_llac_mdct_analyze_overlap(pcm, coefficients, mdct);
}

/* Compute N MDCT coefficients from a 2N analysis block and retain unwindowed history for the next frame. */
void v4f_llac_mdct_analyze_overlap(const LLAC_FLOAT *pcm, LLAC_FLOAT *coefficients, Mdct *mdct) {
    const LLAC_INT frame_samples = mdct->length;
    const LLAC_INT half_frame = frame_samples / 2;
    HANDLE_IIS_FFT fft = mdct->dct.fft.handle;
    /* FFT radix scratch holds 1024 complex values, enough to reuse initially as a 2N analysis block.
     * Folding ends its lifetime, allowing the DCT FFT stage to overwrite it safely. */
    LLAC_FLOAT *analysis_frame = fft->scratch->analysis;
    memset(analysis_frame, 0, LHDC_V4_LLAC_MDCT_TEMP_SAMPLES * sizeof(LLAC_FLOAT));

    /* 1. Assemble [previous history | current N PCM samples | trailing zeros].
     * Normally mem_length + leading_zeros = N; the low-delay window determines history length.
     */
    memmove(analysis_frame, mdct->mem, (size_t)mdct->mem_length * sizeof(LLAC_FLOAT));
    memmove(analysis_frame + mdct->mem_length, pcm, (size_t)frame_samples * sizeof(LLAC_FLOAT));
    memset(analysis_frame + 2 * frame_samples - mdct->leading_zeros, 0,
           (size_t)mdct->leading_zeros * sizeof(LLAC_FLOAT));

    /* 2. Save raw history, then window the current 2N analysis block. */
    memmove(mdct->mem, analysis_frame + frame_samples,
            (size_t)mdct->mem_length * sizeof(LLAC_FLOAT));
    for (LLAC_INT n = 0; n < 2 * frame_samples; ++n)
        analysis_frame[n] *= mdct->window[n];

    /* 3. Time-alias folding: map 2N windowed samples to N DCT-IV inputs.
     * For H=N/2, use negative sums of the last two sections, then reversed differences of the first two.
     */
    for (LLAC_INT n = 0; n < half_frame; ++n) {
        coefficients[n] =
            -analysis_frame[3 * half_frame - 1 - n] - analysis_frame[3 * half_frame + n];
        coefficients[half_frame + n] = analysis_frame[n] - analysis_frame[2 * half_frame - 1 - n];
    }

    /* 4. The common core fully packs folded coefficients first, allowing in-place output without copying the analysis block. */
    v4f_llac_dct4_transform(&mdct->dct, coefficients, coefficients);
}

/* Compute N-point orthonormal DCT-IV using an N/2-point complex FFT.
 * Input is windowed/folded real data; output rearranges evens forward and odds backward.
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
