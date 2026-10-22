#ifndef LHDC_DSP_FAST_MATH_CONSTANTS_H
#define LHDC_DSP_FAST_MATH_CONSTANTS_H
#include <stdint.h>

/* Fitted parameters for bit-pattern approximations, not general mathematical constants.
 * Declare floating values and integer bit patterns separately for expressions and construction; preserve values and literal types.
 */
#define LHDC_FAST_LOG2_BIT_BIAS_F32 126.94269561767578125f
#define LHDC_FAST_EXP2_BIAS_F32 121.274055f
#define LHDC_FAST_EXP2_MIN_EXPONENT_F32 (-126.0f)
#define LHDC_FAST_EXP2_BOUNDED_LIMIT_F32 128.0f
#define LHDC_FAST_EXP2_NUMERATOR_F32 27.728024f
#define LHDC_FAST_EXP2_DENOMINATOR_F32 4.8425255f
#define LHDC_FAST_EXP2_CORRECTION_F32 1.4901291f
#define LHDC_FAST_EXP2_BIAS_BITS_F32 UINT32_C(0x42f28c51)
#define LHDC_FAST_EXP2_NUMERATOR_BITS_F32 UINT32_C(0x41ddd2fe)
#define LHDC_FAST_EXP2_DENOMINATOR_BITS_F32 UINT32_C(0x409af5f8)
#define LHDC_FAST_EXP2_CORRECTION_BITS_F32 UINT32_C(0x3fbebc8d)
#define LHDC_FAST_FUSED_EXP2_BELOW_MIN_BITS UINT32_C(0x008000a7)

/* Bit-pattern inverse-square-root seed and atan2 polynomial parameters. MAGNITUDE receives its sign at the use site. */
#define LHDC_FAST_INVERSE_SQRT_SEED_BITS UINT32_C(0x5f3759df)
#define LHDC_FAST_ATAN_C7_MAGNITUDE_F32 0.046496473f
#define LHDC_FAST_ATAN_C5_F32 0.15931422f
#define LHDC_FAST_ATAN_C3_MAGNITUDE_F32 0.32762277f

#endif
