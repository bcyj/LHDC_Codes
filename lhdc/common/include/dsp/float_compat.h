#ifndef LHDC_DSP_FLOAT_COMPAT_H
#define LHDC_DSP_FLOAT_COMPAT_H
#include <audio_math/float_convert.h>
#include <audio_math/compiler.h>
#include <audio_math/constants.h>
#include <audio_math/numeric_bits.h>
#include "../../../../math/include/audio_math/integer.h"
#include "fast_math_constants.h"
#include <audio_math/fma_native.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Compute the fast-power value to convert to bits, preserving fused multiply-subtract order. */
static AUDIO_MATH_FORCE_INLINE float lhdc_approx_exp2_fused_scaled_bits_f32(float x, float z) {
    float biased = x + audio_math_float32_from_bits(LHDC_FAST_EXP2_BIAS_BITS_F32);
    /* Assign the denominator to float first to preserve single rounding and prevent x87 subtraction precision from entering division. */
    float denominator = audio_math_float32_from_bits(LHDC_FAST_EXP2_DENOMINATOR_BITS_F32) - z;
    float quotient = audio_math_float32_from_bits(LHDC_FAST_EXP2_NUMERATOR_BITS_F32) / denominator;
    float approximation = audio_math_fma_f32(
        -z, audio_math_float32_from_bits(LHDC_FAST_EXP2_CORRECTION_BITS_F32), quotient + biased);
    return approximation * AUDIO_MATH_POW2_23_F32;
}
/* Use bounded integer conversion on the ordered x<128 path, preserving the fixed lower-bound result. */
static inline float lhdc_approx_exp2_fused_bounded_f32(float x) {
    assert(x < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32);
    if (x < LHDC_FAST_EXP2_MIN_EXPONENT_F32)
        return audio_math_float32_from_bits(LHDC_FAST_FUSED_EXP2_BELOW_MIN_BITS);
    /* Here x is finite with -126<=x<128, z is in [0,1], and the scaled result fits uint32. */
    float offset = x < 0.0f ? 1.0f : 0.0f;
    float z = (x - (float)audio_math_truncate_i32_f32(x)) + offset;
    return audio_math_float32_from_return_bits(
        audio_math_truncate_u32_f32(lhdc_approx_exp2_fused_scaled_bits_f32(x, z)));
}
/* Generic fused fast-power entry, preserving the fixed result below -126 and exceptional-input conversion rules. */
static inline float lhdc_approx_exp2_fused_f32(float x) {
    if (x < LHDC_FAST_EXP2_MIN_EXPONENT_F32)
        return audio_math_float32_from_bits(LHDC_FAST_FUSED_EXP2_BELOW_MIN_BITS);
    float offset = x < 0.0f ? 1.0f : 0.0f;
    float z = (x - (float)audio_math_truncate_sat_i32_f32(x)) + offset;
    return audio_math_float32_from_return_bits(
        audio_math_truncate_sat_u32_f32(lhdc_approx_exp2_fused_scaled_bits_f32(x, z)));
}
/* Perform one inverse-square-root Newton step, then reciprocate, using a bit-pattern seed and fused multiply-subtract. */
static inline float lhdc_approx_sqrt_fused_f32(float x) {
    uint32_t bits;
    memcpy(&bits, &x, sizeof bits);
    uint32_t arithmetic_half = (bits >> 1) | (bits & UINT32_C(0x80000000));
    float y = audio_math_float32_from_bits(LHDC_FAST_INVERSE_SQRT_SEED_BITS - arithmetic_half);
    float product = (x * 0.5f) * y;
    float correction = audio_math_fma_f32(-y, product, 1.5f);
#if defined(_MSC_VER) && defined(_M_IX86)
    /* Win32 returns float through x87; explicitly round to binary32 before division so multiplication and division round separately. */
    volatile float corrected_inverse = y * correction;
    return 1.0f / corrected_inverse;
#else
    return 1.0f / (y * correction);
#endif
}
#ifdef __cplusplus
}
#endif
#endif
