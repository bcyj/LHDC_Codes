#ifndef AUDIO_MATH_DSP_FLOAT_CONVERT_H
#define AUDIO_MATH_DSP_FLOAT_CONVERT_H
#include <audio_math/float_properties.h>
#include <audio_math/numeric_bits.h>
#include <audio_math/constants.h>
#include <limits.h>

/* Extract the integer magnitude of a finite binary32 value.
 * Callers guarantee uint32 representability; magnitudes below one yield zero, with shifts in 0..23. */
static inline uint32_t audio_math_truncate_magnitude_f32_bits(uint32_t bits) {
    unsigned exponent = (bits >> 23) & 0xffu;
    if (exponent < 127)
        return 0;
    unsigned integer_bits = exponent - 127;
    uint32_t significand = (bits & UINT32_C(0x007fffff)) | UINT32_C(0x00800000);
    return integer_bits <= 23 ? significand >> (23 - integer_bits)
                              : significand << (integer_bits - 23);
}

/* Extract integer magnitude from two words of finite binary64.
 * Callers guarantee uint32 representability; split-word shifts avoid Win32 kernel CRT references.
 * Maximum valid exponent is 31; nonzero paths shift left 1..11 or the low word right 21..31. */
static inline uint32_t audio_math_truncate_magnitude_f64_words(uint32_t high, uint32_t low) {
    unsigned exponent = (high >> 20) & 0x7ffu;
    if (exponent < 1023)
        return 0;
    unsigned integer_bits = exponent - 1023;
    uint32_t significand_high = (high & 0xfffffu) | 0x100000u;
    return integer_bits <= 20
               ? significand_high >> (20 - integer_bits)
               : (significand_high << (integer_bits - 20)) | (low >> (52 - integer_bits));
}

/* Truncate binary32 fields to int32, saturating overflow and zeroing NaN.
 * The integer part uses at most 31 bits; only bounded uint32 shifts are needed. */
static inline int32_t audio_math_truncate_sat_i32_f32_soft(float value) {
    uint32_t bits = audio_math_float32_bits(value);
    uint32_t magnitude = bits & AUDIO_MATH_FLOAT32_MAGNITUDE_BITS;
    if (magnitude >= AUDIO_MATH_FLOAT32_POW2_31_BITS) {
        if (magnitude > AUDIO_MATH_FLOAT32_INFINITY_BITS)
            return 0;
        return bits & AUDIO_MATH_FLOAT32_SIGN_BITS ? INT32_MIN : INT32_MAX;
    }
    uint32_t integer = audio_math_truncate_magnitude_f32_bits(bits);
    return audio_math_int32_from_bits(bits & AUDIO_MATH_FLOAT32_SIGN_BITS ? 0u - integer : integer);
}

/* Truncate binary32 fields to uint32; negatives/NaN yield zero and positive overflow saturates. */
static inline uint32_t audio_math_truncate_sat_u32_f32_soft(float value) {
    uint32_t bits = audio_math_float32_bits(value);
    if ((bits & AUDIO_MATH_FLOAT32_SIGN_BITS) || bits > AUDIO_MATH_FLOAT32_INFINITY_BITS)
        return 0;
    if (bits >= AUDIO_MATH_FLOAT32_POW2_32_BITS)
        return UINT32_MAX;
    return audio_math_truncate_magnitude_f32_bits(bits);
}

/* Truncate binary64 fields to int32, saturating overflow and zeroing NaN.
 * Split into two 32-bit words to avoid x86 CRT helpers for variable uint64 shifts. */
static inline int32_t audio_math_truncate_sat_i32_f64_soft(double value) {
    uint64_t bits = audio_math_float64_bits(value);
    uint32_t high = (uint32_t)(bits >> 32);
    uint32_t low = (uint32_t)bits;
    unsigned exponent = (high >> 20) & 0x7ffu;
    if (exponent >= 1023 + 31) {
        if (exponent == 0x7ffu && ((high & 0xfffffu) || low))
            return 0;
        return high & AUDIO_MATH_FLOAT32_SIGN_BITS ? INT32_MIN : INT32_MAX;
    }
    uint32_t integer = audio_math_truncate_magnitude_f64_words(high, low);
    return audio_math_int32_from_bits(high & AUDIO_MATH_FLOAT32_SIGN_BITS ? 0u - integer : integer);
}

/* Truncate finite binary32 whose integer part fits int32 toward zero. */
static inline int32_t audio_math_truncate_i32_f32(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    uint32_t bits = audio_math_float32_bits(value);
    uint32_t integer = audio_math_truncate_magnitude_f32_bits(bits);
    return audio_math_int32_from_bits(bits & AUDIO_MATH_FLOAT32_SIGN_BITS ? 0u - integer : integer);
#else
    return (int32_t)value;
#endif
}

/* Truncate finite binary64 whose integer part fits int32 toward zero. */
static inline int32_t audio_math_truncate_i32_f64(double value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    uint64_t bits = audio_math_float64_bits(value);
    uint32_t high = (uint32_t)(bits >> 32);
    uint32_t integer = audio_math_truncate_magnitude_f64_words(high, (uint32_t)bits);
    return audio_math_int32_from_bits(high & AUDIO_MATH_FLOAT32_SIGN_BITS ? 0u - integer : integer);
#else
    return (int32_t)value;
#endif
}

/* Truncate finite nonnegative binary32 whose integer part fits uint32 toward zero. */
static inline uint32_t audio_math_truncate_u32_f32(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    return audio_math_truncate_magnitude_f32_bits(audio_math_float32_bits(value));
#else
    return (uint32_t)value;
#endif
}

/* Truncate finite binary32 whose integer part fits int64 toward zero.
 * Win32 kernel builds construct magnitude/two's complement with two 32-bit words and shifts of at most 31,
 * avoiding runtime helpers for float-to-integer conversion and variable 64-bit shifts. */
static inline int64_t audio_math_truncate_i64_f32(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    uint32_t bits = audio_math_float32_bits(value);
    unsigned exponent = (bits >> 23) & 0xffu;
    if (exponent < 127)
        return 0;
    unsigned integer_bits = exponent - 127;
    uint32_t significand = (bits & UINT32_C(0x007fffff)) | UINT32_C(0x00800000);
    uint32_t low, high = 0;
    if (integer_bits <= 23)
        low = significand >> (23 - integer_bits);
    else if (integer_bits < 32)
        low = significand << (integer_bits - 23);
    else if (integer_bits < 55) {
        low = significand << (integer_bits - 23);
        high = significand >> (55 - integer_bits);
    } else {
        low = 0;
        high = significand << (integer_bits - 55);
    }
    if (bits & AUDIO_MATH_FLOAT32_SIGN_BITS) {
        low = 0u - low;
        high = 0u - high - (low != 0);
    }
    uint64_t result_bits = ((uint64_t)high << 32) | low;
    int64_t result;
    memcpy(&result, &result_bits, sizeof result);
    return result;
#else
    return (int64_t)value;
#endif
}

/* Truncate double toward zero with int32 saturation; NaN yields zero. */
static inline int32_t audio_math_truncate_sat_i32_f64(double value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* Handle directly convertible values first; bit comparisons retain NaN-zeroing and endpoint saturation. */
    uint64_t bits = audio_math_float64_bits_sse2(value);
    uint64_t magnitude = bits & AUDIO_MATH_FLOAT64_MAGNITUDE_BITS;
    if (magnitude < AUDIO_MATH_FLOAT64_POW2_31_BITS)
        return audio_math_truncate_i32_f64(value);
    if (magnitude > AUDIO_MATH_FLOAT64_INFINITY_BITS)
        return 0;
    return bits & AUDIO_MATH_FLOAT64_SIGN_BITS ? INT32_MIN : INT32_MAX;
#else
    if (audio_math_is_nan_f64(value))
        return 0;
    if (value >= AUDIO_MATH_POW2_31_F64)
        return INT32_MAX;
    if (value <= -AUDIO_MATH_POW2_31_F64)
        return INT32_MIN;
    return audio_math_truncate_i32_f64(value);
#endif
}

/* Truncate float toward zero with int32 saturation; NaN yields zero. */
static inline int32_t audio_math_truncate_sat_i32_f32(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* Read binary32 bits from XMM and convert with boundary saturation. */
    uint32_t bits = audio_math_float32_bits_sse2(value);
#else
    uint32_t bits = audio_math_float32_bits(value);
#endif
    uint32_t magnitude = bits & AUDIO_MATH_FLOAT32_MAGNITUDE_BITS;
    if (magnitude < AUDIO_MATH_FLOAT32_POW2_31_BITS)
        return audio_math_truncate_i32_f32(value);
    if (magnitude > AUDIO_MATH_FLOAT32_INFINITY_BITS)
        return 0;
    return bits & AUDIO_MATH_FLOAT32_SIGN_BITS ? INT32_MIN : INT32_MAX;
}

/* Truncate float toward zero with uint32 saturation; negatives/NaN yield zero. */
static inline uint32_t audio_math_truncate_sat_u32_f32(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* Read binary32 bits from XMM and convert with boundary saturation. */
    uint32_t bits = audio_math_float32_bits_sse2(value);
#else
    uint32_t bits = audio_math_float32_bits(value);
#endif
    if (bits < AUDIO_MATH_FLOAT32_POW2_32_BITS)
        return audio_math_truncate_u32_f32(value);
    if ((bits & AUDIO_MATH_FLOAT32_SIGN_BITS) || bits > AUDIO_MATH_FLOAT32_INFINITY_BITS)
        return 0;
    return UINT32_MAX;
}
#endif
