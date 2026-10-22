#include "../../../../platform/windows/include/lhdc_windows_exp_log.h"
#include <audio_math/constants.h>
#include <audio_math/math.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/llac/llac_const.h"
#include "../../../../common/include/dsp/dct4_float32.h"
#include <math.h>
#include <string.h>

/* Compute mean-square MDCT energy per band; output is an average, not total energy. */
void v4f_llac_band_mean_square(LLAC_INT bands_number, const LLAC_INT *acc_coeff_per_band,
                               LLAC_FLOAT *band_energy, LLAC_FLOAT *coefficients) {
    v4_llac_band_mean_square(bands_number, acc_coeff_per_band, band_energy, coefficients);
}

/* Average squared coefficients, not total band energy.
 * Retains the native precondition that band boundaries are valid; no guard
 * against empty bands, invalid pointers, or out-of-range coefficient offsets.
 * Compile with -ffp-contract=off to preserve multiply/add rounding separately.
 */
void v4_llac_band_mean_square(LLAC_INT bands_number, const LLAC_INT *acc_coeff_per_band,
                              LLAC_FLOAT *band_energy, LLAC_FLOAT *coefficients) {
    for (LLAC_INT band = 0; band < bands_number; ++band) {
        LLAC_INT start = acc_coeff_per_band[band], stop = acc_coeff_per_band[band + 1];
        LLAC_FLOAT squared_sum = 0.0f;
        for (LLAC_INT bin = start; bin < stop; ++bin)
            squared_sum += coefficients[bin] * coefficients[bin];
        band_energy[band] = squared_sum / (LLAC_FLOAT)(stop - start);
    }
}

/* Estimate bandwidth from high-band means and confirm by energy drops; non-5-ms configurations may select null tables, which callers must prevent. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
void v4f_llac_estimate_bandwidth_with_policy(LLAC_FLOAT *d2, LLAC_INT fs_idx, LLAC_INT frame_dms,
                                             LLAC_INT *bw_idx, unsigned math_policy) {
#else
/* Estimate coding bandwidth index from rate, duration, and grouped energy. */
void v4f_llac_estimate_bandwidth(LLAC_FLOAT *d2, LLAC_INT fs_idx, LLAC_INT frame_dms,
                                 LLAC_INT *bw_idx) {
#endif
    const LLAC_INT *const *starts = frame_dms == LHDC_V4_FRAME_DURATION_5MS     ? alg_mdct_t68
                                    : frame_dms == LHDC_V4_FRAME_DURATION_2P5MS ? alg_mdct_t58
                                                                                : alg_mdct_t48;
    const LLAC_INT *const *stops = frame_dms == LHDC_V4_FRAME_DURATION_5MS     ? alg_mdct_t69
                                   : frame_dms == LHDC_V4_FRAME_DURATION_2P5MS ? alg_mdct_t59
                                                                               : alg_mdct_t49;
    const LLAC_INT *warp_idx_start = starts[fs_idx - 1];
    const LLAC_INT *warp_idx_stop = stops[fs_idx - 1];
    LLAC_INT counter = fs_idx;
    while (counter > 0) {
        LLAC_INT band = counter - 1;
        LLAC_FLOAT d2_sum = 0.0f;
        for (LLAC_INT i = warp_idx_start[band]; i <= warp_idx_stop[band]; ++i)
            d2_sum += d2[i];
        LLAC_FLOAT d2_mean = d2_sum / (LLAC_FLOAT)(warp_idx_stop[band] - warp_idx_start[band] + 1);
        if (!(d2_mean < (LLAC_FLOAT)alg_mdct_t1[band]))
            break;
        --counter;
    }
    *bw_idx = counter;
    if (counter >= fs_idx)
        return;
    LLAC_INT dist = alg_mdct_t3[counter];
    LLAC_INT stop = warp_idx_start[counter];
    LLAC_FLOAT thr = (LLAC_FLOAT)alg_mdct_t2[counter];
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)

#endif
    for (LLAC_INT i = stop + 1; i > stop - dist; --i) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
        LLAC_FLOAT low =
            lhdc_windows_log10f_soft(d2[i - dist] + AUDIO_MATH_EPSILON_F32, math_policy);
        LLAC_FLOAT high = lhdc_windows_log10f_soft(d2[i] + AUDIO_MATH_EPSILON_F32, math_policy);
#else
        LLAC_FLOAT low = audio_math_log10f(d2[i - dist] + AUDIO_MATH_EPSILON_F32);
        LLAC_FLOAT high = audio_math_log10f(d2[i] + AUDIO_MATH_EPSILON_F32);
#endif
        LLAC_FLOAT e_diff = (LLAC_FLOAT)((double)low * 10.0 - (double)high * 10.0);
        if (thr < e_diff)
            return;
    }
    *bw_idx = fs_idx;
}

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Stateless algorithm entry selecting host policy automatically; frame encoding uses the instance-policy entry. */
void v4f_llac_estimate_bandwidth(LLAC_FLOAT *d2, LLAC_INT fs_idx, LLAC_INT frame_dms,
                                 LLAC_INT *bw_idx) {
    v4f_llac_estimate_bandwidth_with_policy(d2, fs_idx, frame_dms, bw_idx,
                                            lhdc_windows_log10_policy());
}
#endif
