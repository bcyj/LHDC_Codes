#ifndef LHDC_ENCODER_V4_LLAC_GAIN_H
#define LHDC_ENCODER_V4_LLAC_GAIN_H
#include <audio_math/math.h>
#include "../../../../common/include/compat/v4/lhdc_v4_numeric_policy.h"
#include <audio_math/numeric_bits.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include <math.h>

extern const uint32_t lhdc_llac_gain_bits[2346];

/* Convert integer gain steps to 10^(step/28), preserving selected table-policy return bits.
 * model is COMMON or UCRT64 only; the default entry below selects system evaluation separately.
 * Finite step input must be integral; both quantizer calls obtain it from integer search or ceiling.
 * Single-precision integer arithmetic remains integral; do not cast to int first and overflow on large finite values.
 * Quiet NaNs while preserving payloads; infinities/out-of-table integers return positive zero or infinity as appropriate.
 * Only return values are specified, not host errno, exception flags, or nondefault rounding environments. */
static inline float llac_gain_from_step_model(float step, unsigned model) {
    uint32_t bits = audio_math_float32_bits(step);
    if ((bits & AUDIO_MATH_FLOAT32_MAGNITUDE_BITS) > AUDIO_MATH_FLOAT32_INFINITY_BITS)
        return audio_math_float32_from_bits(bits | UINT32_C(0x00400000));
    if (step < -1265.0f)
        return 0.0f;
    if (step > 1080.0f)
        return audio_math_float32_from_bits(AUDIO_MATH_FLOAT32_INFINITY_BITS);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* NaN and out-of-table values have exited; finite indices are bounded to -1265..1080. */
    int index = audio_math_truncate_i32_f32(step);
#else
    int index = (int)step;
#endif
    if (model == LHDC_V4_LLAC_GAIN_UCRT64) {
        /* UCRT x64 returns zero for nine tiny values and differs by one mantissa bit at step 114. */
        if (index >= -1264 && index <= -1256)
            return 0.0f;
        if (index == 114)
            return audio_math_float32_from_bits(UINT32_C(1178087108));
    }
    return audio_math_float32_from_bits(lhdc_llac_gain_bits[index + 1265]);
}

/* Compute LLAC integer-step gain under current numeric policy; unknown hosts use host math. */
static inline float llac_gain_from_step(float step) {
#if LHDC_V4_LLAC_GAIN_MODEL == LHDC_V4_LLAC_GAIN_HOST
    return audio_math_powf(10.0f, step / 28.0f);
#else
    return llac_gain_from_step_model(step, LHDC_V4_LLAC_GAIN_MODEL);
#endif
}
#endif
