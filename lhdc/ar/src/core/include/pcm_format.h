#ifndef LHDC_AR_SRC_CORE_INCLUDE_PCM_FORMAT_H
#define LHDC_AR_SRC_CORE_INCLUDE_PCM_FORMAT_H
#include "ar_internal.h"
#include "ar_math_constants.h"
#define LHDC_AR_WORKSPACE_TAG UINT32_C(0x25109908)
/* Check whether the AR input or output bit depth is supported. */
static inline int32_t LHDC_PRIVATE(pcm_bit_depth_supported)(int32_t bits_per_sample) {
    return bits_per_sample == LHDC_CONST(PCM_BITS_S16) ||
           bits_per_sample == LHDC_CONST(PCM_BITS_S24) ||
           bits_per_sample == LHDC_CONST(PCM_BITS_S32);
}

/* Check whether the AR input sample rate is supported. */
static inline int32_t LHDC_PRIVATE(pcm_sample_rate_supported)(int32_t sample_rate) {
    return sample_rate == LHDC_CONST(SAMPLE_RATE_44100) ||
           sample_rate == LHDC_CONST(SAMPLE_RATE_48000) ||
           sample_rate == LHDC_CONST(SAMPLE_RATE_88200) ||
           sample_rate == LHDC_CONST(SAMPLE_RATE_96000);
}

/* Clamp each channel's floating-point PCM to the target output range. */
static inline void LHDC_PRIVATE(clip_pcm_channels)(float **output_channels, int32_t channel_count,
                                                   int32_t sample_count) {
    for (int32_t j = 0; j < channel_count; ++j)
        for (int32_t i = 0; i < sample_count; ++i) {
            float x = output_channels[j][i];
            if (x > 1.f)
                output_channels[j][i] = 1.f;
            else if (x < -1.f)
                output_channels[j][i] = -1.f;
        }
}
#endif
