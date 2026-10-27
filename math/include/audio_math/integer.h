#ifndef AUDIO_MATH_INTEGER_H
#define AUDIO_MATH_INTEGER_H
#include <audio_math/numeric_bits.h>
#include <audio_math/bits.h>

/* Return an all-one mask for negatives, otherwise zero, without negative right shifts. */
static inline uint32_t audio_math_sign_mask_i32(int32_t value) {
    return 0u - ((uint32_t)value >> 31);
}

/* Add modulo 2^32 and return the signed bit interpretation. */
static inline int32_t audio_math_add_wrap_i32(int32_t a, int32_t b) {
    return audio_math_int32_from_bits((uint32_t)a + (uint32_t)b);
}
/* Subtract modulo 2^32 without signed overflow or out-of-range conversion. */
static inline int32_t audio_math_subtract_wrap_i32(int32_t a, int32_t b) {
    return audio_math_int32_from_bits((uint32_t)a - (uint32_t)b);
}
/* Unfold Rice values to 0,-1,1,-2,... across the complete int32 bit range. */
static inline int32_t audio_math_rice_unfold_u32(uint32_t value) {
    return audio_math_int32_from_bits((value >> 1) ^ (0u - (value & 1u)));
}
/* Multiply modulo 2^32 without relying on signed overflow. */
static inline int32_t audio_math_multiply_wrap_i32(int32_t a, int32_t b) {
    return audio_math_int32_from_bits((uint32_t)a * (uint32_t)b);
}
/* Return the low 64 product bits of two uint64 values without requiring the full product to fit.
 * Win32 kernel builds use 32-bit partial products to avoid CRT wide multiplication; others use native multiplication.
 * Cross terms retain only low 32 bits; a_high*b_high contributes nothing modulo 2^64.
 */
static AUDIO_MATH_FORCE_INLINE uint64_t audio_math_multiply_wrap_u64(uint64_t a, uint64_t b) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    uint32_t a_low = (uint32_t)a;
    uint32_t b_low = (uint32_t)b;
    uint64_t low = __emulu(a_low, b_low);
    uint32_t high =
        (uint32_t)(low >> 32) + (uint32_t)(a >> 32) * b_low + a_low * (uint32_t)(b >> 32);
    return ((uint64_t)high << 32) | (uint32_t)low;
#else
    return a * b;
#endif
}
/* Implement arithmetic right shift with unsigned shifts only; oversized counts return sign bits. */
static inline int32_t audio_math_shift_right_portable_i32(int32_t value, unsigned count) {
    if (count >= 32)
        return value < 0 ? -1 : 0;
    if (!count)
        return value;
    uint32_t bits = (uint32_t)value;
    return audio_math_int32_from_bits((bits >> count) | (audio_math_sign_mask_i32(value) << (32 - count)));
}

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
/* These compilers sign-extend negative right shifts; assert target behavior to prevent silent semantic changes. */
#if defined(__cplusplus)
static_assert((INT32_MIN >> 1) == -1073741824 && (-1 >> 31) == -1 &&
                  (INT64_MIN >> 1) == INT64_MIN / 2 && (INT64_C(-1) >> 63) == -1,
              "Native signed right shift must extend the sign bit");
#else
_Static_assert((INT32_MIN >> 1) == -1073741824 && (-1 >> 31) == -1 &&
                   (INT64_MIN >> 1) == INT64_MIN / 2 && (INT64_C(-1) >> 63) == -1,
               "Native signed right shift must extend the sign bit");
#endif
#endif

/* Arithmetic right shift for proven counts 0..31, after caller checks outside loops. */
static inline int32_t audio_math_shift_right_bounded_i32(int32_t value, unsigned count) {
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
    return value >> count;
#else
    return audio_math_shift_right_portable_i32(value, count);
#endif
}

/* Arithmetic 64-bit right shift for guaranteed counts 0..63.
 * Supported compilers retain checked native semantics; Win32 kernel/other compilers use explicit two's-complement fallback.
 */
static AUDIO_MATH_FORCE_INLINE int64_t audio_math_shift_right_bounded_i64(int64_t value, unsigned count) {
#if (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)) &&                              \
    !(defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86))
    return value >> count;
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    if (!count)
        return value;
    uint64_t value_bits = (uint64_t)value;
    int32_t high = audio_math_int32_from_bits((uint32_t)(value_bits >> 32));
    if (count >= 32)
        return (int64_t)audio_math_shift_right_bounded_i32(high, count - 32);
    uint32_t low = ((uint32_t)value_bits >> count) | ((uint32_t)high << (32 - count));
    uint32_t shifted_high = (uint32_t)audio_math_shift_right_bounded_i32(high, count);
    uint64_t result_bits = ((uint64_t)shifted_high << 32) | low;
    int64_t result;
    memcpy(&result, &result_bits, sizeof result);
    return result;
#else
    uint64_t bits = audio_math_shift_arithmetic_sign64((uint64_t)value, count);
    int64_t result;
    memcpy(&result, &bits, sizeof result);
    return result;
#endif
}

/* Arithmetic right shift for arbitrary counts; oversized counts act as shift 31, retaining only sign.
 * Supported compilers use assertion-checked native operations; others use unsigned fallback.
 */
static inline int32_t audio_math_shift_right_i32(int32_t value, unsigned count) {
    return audio_math_shift_right_bounded_i32(value, count < 32 ? count : 31);
}
/* Extract the shifted low word of a modulo-2^64 product; count is modulo 64. */
static inline int32_t audio_math_multiply_shift_wrap_i32(int64_t a, int32_t b, unsigned count) {
    uint64_t product = audio_math_multiply_wrap_u64((uint64_t)a, (uint64_t)(int64_t)b);
    return audio_math_int32_from_bits((uint32_t)audio_math_shift_arithmetic_sign64(product, count & 63u));
}
/* Extract Q31 middle bits of a modulo-2^64 multiply-add, preserving truncation/modulo-2^32 behavior. */
static inline int32_t audio_math_q31_truncate_wrap_i64(uint64_t product_bits) {
    return audio_math_int32_from_bits((uint32_t)(product_bits >> 31));
}
/* Truncate a two-word arithmetic shift to low 32 bits, retaining per-word low-eight-bit count conventions.
 * Counts 0..63 match 64-bit arithmetic shifts; other counts also have defined results.
 */
static inline uint32_t audio_math_shift_pair_right_low8_u32(uint64_t bits, uint32_t shift) {
    uint32_t lo = (uint32_t)bits, hi = (uint32_t)(bits >> 32);
    uint32_t value =
        audio_math_shift_right_low8_zero32(lo, shift) | audio_math_shift_left_low8_zero32(hi, 32u - shift);
    if (audio_math_int32_from_bits(shift - 32u) >= 0)
        value |= audio_math_shift_arithmetic_low8_sign32(hi, shift - 32u);
    return value;
}
#endif
