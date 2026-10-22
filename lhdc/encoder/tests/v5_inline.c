#include "../src/v5/include/codec/bit_writer.h"
#include "../src/v5/include/codec/bit_operations.h"
#include "../src/v5/include/codec/fast_math.h"
#include <audio_math/bits.h>
#include <stdio.h>
#include <string.h>

extern void v5_bw_init(v5_bit_writer *, uint32_t *, uint32_t);
extern int32_t v5_bw_write_uint(v5_bit_writer *, uint32_t, uint32_t);
extern int32_t v5_bw_flush(v5_bit_writer *);
extern int32_t v5_BitWriteZeroPadToByteBoundary(v5_bit_writer *);
extern float v5_EncGainByTable(int32_t, float *, int32_t, float);
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "inline test line %d\n", __LINE__);                                    \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

/* Verify word crossings, masks, zero padding, and inline/exported entries against independently bit-generated streams. */
int main(void) {
    /* Independently compute expected values in the normal gain domain to check integer conversion/rounding placement. */
    for (unsigned i = 0; i <= 259072; ++i) {
        float p = -126.0f + (float)i / 1024.0f;
        float offset = p < 0.0f ? 1.0f : 0.0f;
        float z = offset + (p - (float)(int32_t)p);
        float a = p + LHDC_FAST_EXP2_BIAS_F32;
        float b = LHDC_FAST_EXP2_NUMERATOR_F32 / (LHDC_FAST_EXP2_DENOMINATOR_F32 - z);
        float y = (a + b) + z * -LHDC_FAST_EXP2_CORRECTION_F32;
        uint32_t expected_bits = (uint32_t)(y * AUDIO_MATH_POW2_23_F32);
        CHECK(audio_math_float32_bits(v5_fast_pow2_inline(p)) == expected_bits);
        CHECK(audio_math_float32_bits(v5_fast_pow2_bounded_inline(p)) == expected_bits);
    }
    CHECK(audio_math_float32_bits(v5_fast_pow2_inline(NAN)) == 0);
    /* +Inf produces NaN in the rational approximation; common integer conversion maps that intermediate to zero. */
    CHECK(audio_math_float32_bits(v5_fast_pow2_inline(INFINITY)) == 0);
    CHECK(audio_math_float32_bits(v5_fast_pow2_inline(-INFINITY)) ==
          audio_math_float32_bits(v5_fast_pow2_inline(-126.0f)));
    /* Raw floating bits cover integer neighborhoods, lower clamping, subnormals, and the predecessor of 128. */
    uint32_t pow2_seed = 0x12349876u;
    for (unsigned i = 0; i < 1000000; ++i) {
        pow2_seed = pow2_seed * 1664525u + 1013904223u;
        float p = audio_math_float32_from_bits(pow2_seed);
        if (p < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32)
            CHECK(audio_math_float32_bits(v5_fast_pow2_bounded_inline(p)) ==
                  audio_math_float32_bits(v5_fast_pow2_inline(p)));
    }
    const float pow2_edges[] = {-INFINITY,   -0x1.fffffep127f,
                                -126.00001f, -126.0f,
                                -1.0000001f, -1.0f,
                                -0x1p-149f,  -0.0f,
                                0.0f,        0x1p-149f,
                                0.99999994f, 1.0f,
                                127.0f,      0x1.fffffep6f};
    for (unsigned i = 0; i < sizeof pow2_edges / sizeof *pow2_edges; ++i)
        CHECK(audio_math_float32_bits(v5_fast_pow2_bounded_inline(pow2_edges[i])) ==
              audio_math_float32_bits(v5_fast_pow2_inline(pow2_edges[i])));
    const float gain_steps[] = {NAN, INFINITY, -INFINITY, 127.0f, 126.99999f, 0.0f, 1.0f};
    for (unsigned i = 0; i < sizeof gain_steps / sizeof *gain_steps; ++i) {
        float table[64] = {0};
        float p = gain_steps[i] + 1.0f;
        volatile float expected_gain = 1.0f / v5_fast_pow2_inline(p);
        float gain = v5_EncGainByTable(2, table, 1, gain_steps[i]);
        CHECK(audio_math_float32_bits(gain) == audio_math_float32_bits(expected_gain));
        CHECK(audio_math_float32_bits(table[2]) == audio_math_float32_bits(gain));
    }
    uint32_t seed = 0x19872349u;
    for (unsigned test = 0; test < 1000; ++test) {
        uint32_t actual[80], legacy[80], expected[80];
        memset(actual, 0xa5, sizeof actual);
        memset(legacy, 0xa5, sizeof legacy);
        memset(expected, 0xa5, sizeof expected);
        BitWriter writer, old_writer;
        v5_bw_init_inline(&writer, actual + 4, 256);
        v5_bw_init(&old_writer, legacy + 4, 256);
        unsigned bit_position = 0;
        uint8_t *reference = (uint8_t *)(expected + 4);
        for (unsigned item = 0; item < 32; ++item) {
            seed = seed * 1664525u + 1013904223u;
            unsigned width = seed % 33;
            seed = seed * 1664525u + 1013904223u;
            uint32_t value = seed;
            CHECK(!v5_bw_write_uint_inline(&writer, value, width));
            CHECK(!v5_bw_write_uint(&old_writer, value, width));
            for (unsigned bit = 0; bit < width; ++bit) {
                if (!(bit_position & 7))
                    reference[bit_position / 8] = 0;
                reference[bit_position / 8] |= ((value >> (width - 1 - bit)) & 1u)
                                               << (7 - (bit_position & 7));
                ++bit_position;
            }
        }
        CHECK(!v5_bw_zero_pad_inline(&writer) && !v5_BitWriteZeroPadToByteBoundary(&old_writer));
        int count = v5_bw_flush_inline(&writer);
        CHECK(count == (int)((bit_position + 7) / 8) && v5_bw_flush(&old_writer) == count);
        CHECK(!memcmp(actual, expected, sizeof actual) && !memcmp(legacy, expected, sizeof legacy));
        BitWriter before = writer;
        CHECK(v5_bw_write_uint_inline(&writer, 0, 33) == LHDC_V5_BIT_WRITER_INVALID_WIDTH);
        CHECK(!memcmp(&writer, &before, sizeof writer));
    }
    for (unsigned i = 0; i < 100000; ++i) {
        seed = seed * 1664525u + 1013904223u;
        uint32_t value = seed;
        int count = 0;
        while (value) {
            ++count;
            value >>= 1;
        }
        CHECK(v5_calc_bits_inline(seed) == count);
    }
    CHECK(v5_calc_bits_inline(0) == 0);
    for (unsigned bit = 0; bit < 32; ++bit)
        CHECK(v5_calc_bits_inline(UINT32_C(1) << bit) == (int)bit + 1);
    static const uint32_t shifts[][4] = {{0, 0x80000001, 0x80000001, 0x80000001},
                                         {1, 2, 0x40000000, 0xc0000000},
                                         {31, 0x80000000, 1, 0xffffffff},
                                         {32, 0, 0, 0xffffffff},
                                         {255, 0, 0, 0xffffffff},
                                         {256, 0x80000001, 0x80000001, 0x80000001},
                                         {257, 2, 0x40000000, 0xc0000000}};
    for (unsigned i = 0; i < sizeof shifts / sizeof shifts[0]; ++i) {
        CHECK(audio_math_shift_left_low8_zero32(0x80000001, shifts[i][0]) == shifts[i][1]);
        CHECK(audio_math_shift_right_low8_zero32(0x80000001, shifts[i][0]) == shifts[i][2]);
        CHECK(audio_math_shift_arithmetic_low8_sign32(0x80000001, shifts[i][0]) == shifts[i][3]);
    }
    puts(
        "inline bit writer: 1000 independent streams; bit width: 100000 + boundaries; register shifts passed");
    return 0;
}
