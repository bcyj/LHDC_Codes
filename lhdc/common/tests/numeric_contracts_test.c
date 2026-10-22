#include "../../../math/include/audio_math/integer.h"
#include "../../../math/include/audio_math/byte_order.h"
#include "../include/dsp/float_compat.h"
#include <audio_math/bits.h>
#include <stdio.h>
#include <math.h>

/* Compute expected shifts with low-byte counts, including out-of-range counts. */
static int64_t half_floor(int64_t value) {
    return value / 2 - (value < 0 && value % 2 != 0);
}

/* Independently check leading-zero count with bitwise loops, covering zero and every highest-bit position. */
static int check_leading_zeros(uint64_t value) {
    unsigned expected = 64;
    for (uint64_t copy = value; copy; copy >>= 1)
        --expected;
    return audio_math_count_leading_zeros64(value) == expected;
}

/* Retain the previous fused fast-power expression to independently check rounding order and bit conversion. */
static float reference_exp2(float x) {
    if (x < LHDC_FAST_EXP2_MIN_EXPONENT_F32)
        return audio_math_float32_from_bits(LHDC_FAST_FUSED_EXP2_BELOW_MIN_BITS);
    float offset = x < 0.0f ? 1.0f : 0.0f;
    float z = (x - (float)audio_math_truncate_sat_i32_f32(x)) + offset;
    float approximation =
        audio_math_fma_f32(-z, audio_math_float32_from_bits(LHDC_FAST_EXP2_CORRECTION_BITS_F32),
                     audio_math_float32_from_bits(LHDC_FAST_EXP2_NUMERATOR_BITS_F32) /
                             (audio_math_float32_from_bits(LHDC_FAST_EXP2_DENOMINATOR_BITS_F32) - z) +
                         (x + audio_math_float32_from_bits(LHDC_FAST_EXP2_BIAS_BITS_F32)));
    return audio_math_float32_from_return_bits(
        audio_math_truncate_sat_u32_f32(approximation * AUDIO_MATH_POW2_23_F32));
}

/* Verify classification, boundaries, and shifts in generic numeric helpers. */
static int check_exp2(float x) {
    uint32_t expected = audio_math_float32_bits(reference_exp2(x));
    if (audio_math_float32_bits(lhdc_approx_exp2_fused_f32(x)) != expected)
        return 0;
    return !(x < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32) ||
           audio_math_float32_bits(lhdc_approx_exp2_fused_bounded_f32(x)) == expected;
}

void lhdc_test_float_properties(void);

/* Check integer bit operations, floating-point properties, and unaligned-access boundaries. */
int main(void) {
    lhdc_test_float_properties();
    if (!check_leading_zeros(0))
        return 16;
    for (unsigned bit = 0; bit < 64; ++bit)
        if (!check_leading_zeros(UINT64_C(1) << bit) ||
            !check_leading_zeros((UINT64_C(1) << bit) - 1))
            return 16;
    for (int step = -129 * 1024; step <= 128 * 1024; ++step)
        if (!check_exp2((float)step / 1024.0f))
            return 17;
    for (int integer = -129; integer <= 128; ++integer) {
        float x = (float)integer;
        if (!check_exp2(nextafterf(x, -INFINITY)) || !check_exp2(x) ||
            !check_exp2(nextafterf(x, INFINITY)))
            return 17;
    }
    static const unsigned counts[] = {
        0, 1, 7, 31, 32, 33, 63, 64, 255, 256, 257, UINT32_MAX, UINT32_C(0x80000020)};
    uint32_t random = 12345;
    static const uint32_t edge_values[] = {0, 1, UINT32_MAX, UINT32_C(0x80000000)};
    unsigned checked = 0;
    for (unsigned i = 0; i < 4096; ++i) {
        random = random * 1664525u + 1013904223u;
        uint32_t sample = i < 4 ? edge_values[i] : random;
        int32_t rice_expected =
            (sample & 1u) ? (int32_t)(-(int64_t)(sample / 2u) - 1) : (int32_t)(sample / 2u);
        if (audio_math_rice_unfold_u32(sample) != rice_expected)
            return 19;
        int32_t signed_sample = audio_math_int32_from_bits(sample);
        if (audio_math_sign_mask_i32(signed_sample) != (signed_sample < 0 ? UINT32_MAX : 0u))
            return 14;
        for (unsigned j = 0; j < sizeof counts / sizeof counts[0]; ++j) {
            unsigned count = counts[j], n = count & 255u;
            uint32_t left = sample, right = sample;
            int64_t arithmetic = audio_math_int32_from_bits(sample);
            for (unsigned k = 0; k < n; ++k) {
                left *= 2u;
                right /= 2u;
                arithmetic = half_floor(arithmetic);
            }
            if (audio_math_shift_left_low8_zero32(sample, count) != left ||
                audio_math_shift_right_low8_zero32(sample, count) != right ||
                audio_math_shift_arithmetic_low8_sign32(sample, count) != (uint32_t)arithmetic)
                return 1;
            int32_t shift_expected =
                count >= 32 ? (signed_sample < 0 ? -1 : 0) : (int32_t)arithmetic;
            if (audio_math_shift_right_i32(signed_sample, count) != shift_expected ||
                audio_math_shift_right_portable_i32(signed_sample, count) != shift_expected)
                return 15;
            left = sample;
            right = sample;
            for (unsigned k = 0; k < (count & 31u); ++k) {
                left *= 2u;
                right /= 2u;
            }
            if (audio_math_shift_left_mod32(sample, count) != left ||
                audio_math_shift_right_mod32(sample, count) != right)
                return 2;
            ++checked;
        }
        uint64_t bits = (uint64_t)sample << 32 | (random ^ UINT32_C(0x98765432));
        int64_t signed_value;
        memcpy(&signed_value, &bits, sizeof signed_value);
        for (unsigned count = 0; count <= 64; ++count) {
            int64_t reference = signed_value;
            for (unsigned k = 0; k < count; ++k)
                reference = half_floor(reference);
            uint64_t reference_bits;
            memcpy(&reference_bits, &reference, sizeof reference_bits);
            if (audio_math_shift_arithmetic_sign64(bits, count) != reference_bits)
                return 3;
            if (count < 64 &&
                audio_math_shift_pair_right_low8_u32(bits, count) != (uint32_t)reference_bits)
                return 4;
        }
    }
    if (audio_math_shift_pair_right_low8_u32(UINT64_C(0x8000000112345678), 64) != UINT32_MAX ||
        audio_math_shift_pair_right_low8_u32(UINT64_C(0x8000000112345678), 256) != UINT32_MAX ||
        audio_math_shift_pair_right_low8_u32(UINT64_C(0x8000000112345678), UINT32_MAX) != 0)
        return 5;
    if (audio_math_add_wrap_i32(INT32_MAX, 1) != INT32_MIN ||
        audio_math_subtract_wrap_i32(INT32_MIN, 1) != INT32_MAX ||
        audio_math_q31_truncate_wrap_i64(UINT64_C(0x4000000000000000)) != INT32_MIN)
        return 6;
    for (uint32_t n = 0; n < 8192; ++n) {
        unsigned expected = 0;
        uint64_t power = 1;
        while (power < n) {
            power *= 2;
            ++expected;
        }
        if (audio_math_ceil_log2_u32(n) != expected)
            return 7;
    }
    const double values[] = {NAN,           INFINITY,       -INFINITY, 2147483648.0, -2147483648.0,
                             2147483647.75, -2147483648.75, -1.99,     1.99,         -0.0};
    static const int32_t expected[] = {0,         INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN,
                                       INT32_MAX, INT32_MIN, -1,        1,         0};
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; ++i)
        if (audio_math_truncate_sat_i32_f64(values[i]) != expected[i])
            return 8;
    /* Random raw bits cover subnormals, negative zero, infinity, NaNs, and both conversion boundaries. */
    for (unsigned i = 0; i < 1000000; ++i) {
        random = random * 1664525u + 1013904223u;
        /* Check little-endian unaligned writes, narrowing truncation, and surrounding sentinels. */
        uint8_t little_endian[6] = {0x5a, 0, 0, 0, 0, 0xa5};
        audio_math_write_le32(little_endian + 1, random);
        uint64_t byte_reference = random;
        for (unsigned byte = 0; byte < 4; ++byte) {
            if (little_endian[byte + 1] != byte_reference % 256u)
                return 21;
            byte_reference /= 256u;
        }
        uint16_t narrow = (uint16_t)(random ^ 0x5a3cu);
        audio_math_write_le16(little_endian + 1, narrow);
        if (little_endian[0] != 0x5a || little_endian[5] != 0xa5 ||
            little_endian[1] != narrow % 256u || little_endian[2] != narrow / 256u ||
            little_endian[3] != random / 65536u % 256u || little_endian[4] != random / 16777216u)
            return 22;
        /* Check every valid count of each random integer against native and fallback paths and independent iterative bisection. */
        int32_t shift_sample = audio_math_int32_from_bits(random);
        int64_t shift_expected = shift_sample;
        for (unsigned count = 0; count <= 33; ++count) {
            if (audio_math_shift_right_i32(shift_sample, count) != shift_expected ||
                audio_math_shift_right_portable_i32(shift_sample, count) != shift_expected ||
                (count < 32 && audio_math_shift_right_bounded_i32(shift_sample, count) != shift_expected))
                return 20;
            shift_expected = half_floor(shift_expected);
        }
        float value = audio_math_float32_from_bits(random);
        if (!check_exp2(value) ||
            !check_leading_zeros((uint64_t)random << 32 | (random ^ UINT32_C(0x92837465))))
            return 18;
        int32_t signed_expected = audio_math_truncate_sat_i32_f64((double)value);
        uint32_t unsigned_expected = !(value > 0.0f)                 ? 0u
                                     : (double)value >= 4294967296.0 ? UINT32_MAX
                                                                     : (uint32_t)(double)value;
        if (audio_math_truncate_sat_i32_f32(value) != signed_expected ||
            audio_math_truncate_sat_u32_f32(value) != unsigned_expected ||
            audio_math_truncate_sat_i32_f32_soft(value) != signed_expected ||
            audio_math_truncate_sat_u32_f32_soft(value) != unsigned_expected ||
            audio_math_truncate_sat_i32_f64_soft((double)value) != signed_expected)
            return 16;
    }
    const uint32_t conversion_edges[] = {
        0,           1,           0x80000000u, 0x80000001u, 0x4effffffu, 0x4f000000u,
        0x4f000001u, 0xceffffffu, 0xcf000000u, 0xcf000001u, 0x4f7fffffu, 0x4f800000u,
        0x4f800001u, 0x7f800000u, 0xff800000u, 0x7fc00001u, 0xffc00001u};
    for (unsigned i = 0; i < sizeof conversion_edges / sizeof *conversion_edges; ++i) {
        float value = audio_math_float32_from_bits(conversion_edges[i]);
        uint32_t unsigned_expected = !(value > 0.0f)                 ? 0u
                                     : (double)value >= 4294967296.0 ? UINT32_MAX
                                                                     : (uint32_t)(double)value;
        if (audio_math_truncate_sat_i32_f32(value) != audio_math_truncate_sat_i32_f64((double)value) ||
            audio_math_truncate_sat_u32_f32(value) != unsigned_expected)
            return 17;
    }
    uint8_t bytes[6] = {9, 0x81, 0x23, 0x45, 0x67, 8};
    if (audio_math_read_be32(bytes + 1) != UINT32_C(0x81234567))
        return 9;
    static const unsigned widths[] = {0, 1, 11, 20, 31, 32, 63, 64, UINT32_MAX};
    for (unsigned i = 0; i < 4096; ++i) {
        random = random * 1664525u + 1013904223u;
        uint64_t a_bits = (uint64_t)random << 32 | (random ^ UINT32_C(0xa5a5a5a5));
        int64_t a;
        memcpy(&a, &a_bits, sizeof a);
        int32_t b = audio_math_int32_from_bits(random ^ UINT32_C(0x91827364));
        uint64_t multiplier = (uint64_t)(int64_t)b, reference_product = 0;
        for (unsigned bit = 0; bit < 64; ++bit)
            if (multiplier & (UINT64_C(1) << bit))
                reference_product += a_bits << bit;
        for (unsigned j = 0; j < sizeof widths / sizeof *widths; ++j) {
            uint64_t reference = reference_product;
            for (unsigned k = 0; k < (widths[j] & 63u); ++k)
                reference = (reference >> 1) | (reference & UINT64_C(0x8000000000000000));
            if ((uint32_t)audio_math_multiply_shift_wrap_i32(a, b, widths[j]) != (uint32_t)reference)
                return 10;
        }
    }
    if (audio_math_multiply_wrap_i32(INT32_MIN, -1) != INT32_MIN || audio_math_shift_right_i32(-3, 1) != -2 ||
        audio_math_shift_right_i32(-3, 32) != -1 || audio_math_shift_right_i32(3, 32) != 0)
        return 11;
    for (unsigned offset = 1; offset < 8; ++offset) {
        uint8_t storage[24];
        memset(storage, 0x5a, sizeof storage);
        audio_math_write_le64(storage + offset, UINT64_C(0xfedcba9876543210));
        if (audio_math_read_le64(storage + offset) != UINT64_C(0xfedcba9876543210) ||
            audio_math_read_le32(storage + offset) != UINT32_C(0x76543210) ||
            audio_math_read_le24(storage + offset) != UINT32_C(0x543210) ||
            audio_math_read_le16(storage + offset) != UINT16_C(0x3210) || storage[offset - 1] != 0x5a ||
            storage[offset + 8] != 0x5a)
            return 12;
        audio_math_write_le_bytes(storage + offset, UINT32_C(0x89abcdef), 3);
        if (storage[offset] != 0xef || storage[offset + 1] != 0xcd || storage[offset + 2] != 0xab ||
            storage[offset + 3] != 0x76)
            return 13;
    }
    printf(
        "PASS numeric contracts: %u shift tuples; 266240 signed64 shifts; ceil log2 and saturation\n",
        checked);
    return 0;
}
