#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_FAST_MATH_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_FAST_MATH_H
#include <audio_math/numeric_bits.h>
#include "../../../../../common/include/dsp/fast_math_constants.h"
#include <audio_math/constants.h>
#include "../../../../../common/include/dsp/float_compat.h"
#include <assert.h>
/* Approximate negative binary logarithm from IEEE floating-point bits. */
static inline float v5_fast_nlog2_inline(float x) {
    return (float)audio_math_float32_bits(x) * -AUDIO_MATH_EPSILON_F32 + LHDC_FAST_LOG2_BIT_BIAS_F32;
}

/* Evaluate the numeric IEEE bit pattern using the established single-precision operation order. */
static inline float v5_fast_pow2_scaled_bits_inline(float p, float z) {
    float a = p + LHDC_FAST_EXP2_BIAS_F32;
    /* Round denominator, sum, and correction product separately to float, preserving common evaluation boundaries across hosts. */
    float denominator = LHDC_FAST_EXP2_DENOMINATOR_F32 - z;
    float b = LHDC_FAST_EXP2_NUMERATOR_F32 / denominator;
    float sum = a + b;
    float correction = z * -LHDC_FAST_EXP2_CORRECTION_F32;
    float y = sum + correction;
    return y * AUDIO_MATH_POW2_23_F32;
}

/* Compute fast power on the internal non-NaN p < 128 path.
 * Clamp the lower bound to -126 so -126 <= p < 128 and 0 <= z <= 1 before conversion.
 * Rational term minus correction is about 5.64..5.73; final bit-pattern value lies in (0, 2^32).
 * Both integer conversions fit, without out-of-range casts or CPU-specific saturation instructions.
 */
static inline float v5_fast_pow2_bounded_inline(float p) {
    assert(p < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32);
    float offset = p < 0.0f ? 1.0f : 0.0f;
    if (p < LHDC_FAST_EXP2_MIN_EXPONENT_F32)
        p = LHDC_FAST_EXP2_MIN_EXPONENT_F32;
    float z = offset + (p - (float)audio_math_truncate_i32_f32(p));
    return audio_math_float32_from_return_bits(
        audio_math_truncate_u32_f32(v5_fast_pow2_scaled_bits_inline(p, z)));
}

/* Generic fast-power entry retaining common conversion rules for exceptional exponents/intermediates. */
static inline float v5_fast_pow2_inline(float p) {
    float offset = p < 0.0f ? 1.0f : 0.0f;
    if (p < LHDC_FAST_EXP2_MIN_EXPONENT_F32)
        p = LHDC_FAST_EXP2_MIN_EXPONENT_F32;
    float z = offset + (p - (float)audio_math_truncate_sat_i32_f32(p));
    return audio_math_float32_from_return_bits(
        audio_math_truncate_sat_u32_f32(v5_fast_pow2_scaled_bits_inline(p, z)));
}

/* Approximate square root using fast inverse-square-root iteration. */
static inline float v5_fast_sqrt_inline(float x) {
    uint32_t bits = audio_math_float32_bits(x);
    uint32_t arithmetic_half = (bits >> 1) | (bits & UINT32_C(0x80000000));
    float y = audio_math_float32_from_bits(LHDC_FAST_INVERSE_SQRT_SEED_BITS - arithmetic_half);
    float t = x * -0.5f;
    t *= y;
    t *= y;
    t += 1.5f;
    t *= y;
    return 1.0f / t;
}

#endif
