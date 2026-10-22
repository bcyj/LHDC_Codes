#ifndef LHDC_AR_SRC_EFFECTS_UPMIX_UPMIX_LIBRARY_H
#define LHDC_AR_SRC_EFFECTS_UPMIX_UPMIX_LIBRARY_H
#include "lhdc_version.h"
#include <stdint.h>
/* Evaluate weighted products in order and update smoothed power. */
static inline void LHDC_PRIVATE(smooth_normalized_power)(const float *input,
                                                                       float alpha, float max,
                                                                       int32_t sample_count,
                                                                       float *output_power,
                                                                       float *previous_power) {
    for (int32_t i = 0; i < sample_count; ++i) {
        float v = max == 0.f ? 0.f : input[i] / max;
        *previous_power = (*previous_power * alpha + (1.f - alpha) * (v * v));
        output_power[i] = *previous_power;
    }
}
#endif
