#include <audio_math/float_properties.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../common/gain_smoothing.h"
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include <audio_math/float_sqrt.h>
#include "lhdc_version.h"
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <string.h>

/* Calculate reverb algorithm workspace size. */
uint32_t LHDC_FN(reverb_workspace_bytes)(void) {
    uint32_t size;
    LHDC_FN(upconv_alloc)(0, &size);
    return size;
}

/* Partition reverb buffers and initialize state. */
void LHDC_FN(init_reverb_memory)(uint8_t *ptr_reverb_mem, uint32_t size) {
    LHDC_FN(upconv_alloc)(ptr_reverb_mem, &size);
    LHDC_FN(upconv_init)((LHDCConvolutionDirectory *)ptr_reverb_mem);
}

/* Compute reverb output from distance, angle, and history. */
void LHDC_FN(reverb_effect_reduce)(PST_LHDC_REVERB state, int32_t samples) {
    const float *data_in_fl = state->pfPcm[0], *data_in_fr = state->pfPcm[1],
                *data_in_c = state->pfPcm[2];
    float frame_len = (float)samples, fs = (float)state->nSampleRate;
    float dreal = state->dreal, dref = state->dref, attack_t = state->attack_t;
    float gain_reverb_i = state->gain_reverb_i, gain_mem_r = state->gain_mem_r;
    float ele = state->theta_cur, azi = state->phy_cur, wscale = state->wscale;
    float *dist_mem_r_fl = state->dist_mem_r_fl, *dist_mem_r_fr = state->dist_mem_r_fr,
          *dist_mem_r_c = state->dist_mem_r_c;
    float *data_reverb_l = state->pfPcmOutput[0], *data_reverb_r = state->pfPcmOutput[1];
    uint8_t *ptr_reverb_mem = state->pucUpolsMem;
    float cos_elevation, cos_azimuth, sin_elevation, sin_azimuth;
    LHDC_FN(sincos_degrees)(ele, &sin_elevation, &cos_elevation);
    LHDC_FN(sincos_degrees)(azi, &sin_azimuth, &cos_azimuth);
    float direction_gain_x = cos_azimuth * cos_elevation + audio_math_abs_f32(sin_elevation),
          direction_gain_y = sin_azimuth * cos_elevation;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    const int32_t sample_count = audio_math_truncate_i32_f32(frame_len);
    if (sample_count > 0) {
#else
    if ((int32_t)frame_len > 0) {
#endif
        float decay = lhdc_ar_smoothing_decay(fs * attack_t);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        for (int32_t i = 0; i < sample_count; ++i) {
#else
        for (int32_t i = 0; i < (int32_t)frame_len; ++i) {
#endif
            gain_reverb_i += (1.f - decay) * (audio_math_sqrt_f32(dref / dreal) - gain_reverb_i);
            dist_mem_r_fl[i] = gain_reverb_i * data_in_fl[i];
            dist_mem_r_fr[i] = gain_reverb_i * data_in_fr[i];
            dist_mem_r_c[i] = gain_reverb_i * data_in_c[i];
            gain_mem_r = gain_reverb_i;
        }
    }
    if (!ptr_reverb_mem) {
        state->gain_reverb_i = gain_mem_r;
        return;
    }

    /* The input FFT stage does not yet use spectral sum storage; clear and accumulate only after saving all three inputs.
     * Borrow its first three segments without increasing instance size or allocating during processing.
     * This private sum area must not simultaneously serve as external PCM or distance storage. */
    _Static_assert(3 * LHDC_CONST(AR_BLOCK_SAMPLES) <= LHDC_CONST(AR_SPECTRUM_FLOATS),
                   "reverb input blocks must fit in the convolution sum buffer");
    LHDCConvolutionDirectory *workspace = (void *)ptr_reverb_mem;
    float *omnidirectional = workspace->sum[0];
    float *directional_x = omnidirectional + LHDC_CONST(AR_BLOCK_SAMPLES);
    float *directional_y = directional_x + LHDC_CONST(AR_BLOCK_SAMPLES);
    for (int32_t i = 0; i < LHDC_CONST(AR_BLOCK_SAMPLES); ++i) {
        float front_left = dist_mem_r_fl[i], front_right = dist_mem_r_fr[i],
              center_sample = dist_mem_r_c[i];
        omnidirectional[i] = (front_left * wscale + front_right * wscale) + center_sample * wscale;
        directional_x[i] = (front_left * direction_gain_x + front_right * direction_gain_x) +
                           center_sample * direction_gain_x;
        directional_y[i] = (front_left * direction_gain_y + front_right * direction_gain_y) +
                           center_sample * direction_gain_y;
    }
    LHDC_FN(upconv_process)
    (omnidirectional, directional_x, directional_y, data_reverb_l, data_reverb_r, workspace);
    state->gain_reverb_i = gain_mem_r;
}
