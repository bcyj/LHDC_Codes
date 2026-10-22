#include "../../common/gain_smoothing.h"
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <string.h>

/* Smooth distance gain per sample, process four channels, and retain gain state for the next frame.
 * Output copying always covers 256 floats; samples controls only the number processed for gain. */
void LHDC_FN(dist_effect_reduce)(PST_LHDC_DIST_EFFECT state, int32_t samples) {
    const float *data_in_fl = state->pfPcm[0], *data_in_fr = state->pfPcm[1],
                *data_in_c = state->pfPcm[2], *data_in_lfe = state->pfPcm[3];
    float frame_len = (float)samples, fs = (float)state->nSampleRate;
    float dreal = state->dreal, dref = state->dref, attack_t = state->attack_t;
    float gain_dist_i = state->gain_dist_i, gain_mem = state->gain_mem;
    float *dist_mem_fl = state->dist_mem_fl, *dist_mem_fr = state->dist_mem_fr,
          *dist_mem_c = state->dist_mem_c, *dist_mem_lfe = state->dist_mem_lfe;
    float *data_out_fl = state->pfPcmOutput[0], *data_out_fr = state->pfPcmOutput[1],
          *data_out_c = state->pfPcmOutput[2], *data_out_lfe = state->pfPcmOutput[3];
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    int32_t sample_count = audio_math_truncate_i32_f32(frame_len);
#else
    int32_t sample_count = (int32_t)frame_len;
#endif
    if (sample_count > 0) {
        float smoothing_decay = lhdc_ar_smoothing_decay(fs * attack_t);
        for (int32_t i = 0; i != sample_count; ++i) {

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            /* Blocks contain at most 256 samples; float round trips match integer indices throughout this range. */
            uint32_t sample_index = (uint32_t)i;
#else
            uint32_t sample_index = (int32_t)((float)i + 1.0f) - 1;
#endif
            gain_dist_i = gain_dist_i + (1.0f - smoothing_decay) * (dref / dreal - gain_dist_i);
            dist_mem_fl[sample_index] = gain_dist_i * data_in_fl[sample_index];
            dist_mem_fr[sample_index] = gain_dist_i * data_in_fr[sample_index];
            dist_mem_c[sample_index] = gain_dist_i * data_in_c[sample_index];
            dist_mem_lfe[sample_index] = gain_dist_i * data_in_lfe[sample_index];
            gain_mem = gain_dist_i;
        }
    }
    memcpy(data_out_fl, dist_mem_fl, LHDC_CONST(AR_BLOCK_SAMPLES) * sizeof(float));
    memcpy(data_out_fr, dist_mem_fr, LHDC_CONST(AR_BLOCK_SAMPLES) * sizeof(float));
    memcpy(data_out_c, dist_mem_c, LHDC_CONST(AR_BLOCK_SAMPLES) * sizeof(float));
    memcpy(data_out_lfe, dist_mem_lfe, LHDC_CONST(AR_BLOCK_SAMPLES) * sizeof(float));
    state->gain_dist_i = gain_mem;
}
