#ifndef LHDC_V4_MATH_H
#define LHDC_V4_MATH_H
#include "lhdc_v4_numeric_policy.h"
#include <audio_math/math.h>
#include <audio_math/constants.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && LHDC_V4_LOG2_VIA_LN
#include <audio_math/float_log.h>
#endif

/* Compute single-precision log2 under an explicit numeric model, preserving intermediate division rounding. */
static inline float lhdc_v4_log2_compat_f32(float value) {
#if LHDC_V4_LOG2_VIA_LN
#if defined(_KERNEL_MODE) && defined(_MSC_VER)
    /* Round logf's result to float before division; do not divide the extended-precision result directly. */
    float logarithm = (float)audio_math_log_f32_wide_soft(value);
    return logarithm / AUDIO_MATH_LN_TWO_F32;
#else
    return audio_math_logf(value) / AUDIO_MATH_LN_TWO_F32;
#endif
#else
    return audio_math_log2f(value);
#endif
}
#endif
