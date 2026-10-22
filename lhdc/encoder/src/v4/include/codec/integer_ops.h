/* Explicit V4 scalar-shift model; host ABI determines only the policy header default. */
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_INTEGER_OPS_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_INTEGER_OPS_H
#include <audio_math/bits.h>
#include "../../../../../common/include/compat/v4/lhdc_v4_numeric_policy.h"

/* Left-shift a 32-bit integer using the configured five/eight-bit count rule. */
static inline uint32_t v4_shift_left_compat32(uint32_t value, uint32_t count) {
#if LHDC_V4_SHIFT_COUNT_BITS == 8
    return audio_math_shift_left_low8_zero32(value, count);
#else
    return audio_math_shift_left_mod32(value, count);
#endif
}

/* Logical right-shift using the configured five/eight-bit count rule. */
static inline uint32_t v4_shift_right_compat32(uint32_t value, uint32_t count) {
#if LHDC_V4_SHIFT_COUNT_BITS == 8
    return audio_math_shift_right_low8_zero32(value, count);
#else
    return audio_math_shift_right_mod32(value, count);
#endif
}
#endif
