#include "../include/lhdc_cirbuf.h"
#include <audio_math/numeric_bits.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
/* Report buffer or numeric-check failures. */
static void require(int ok) {
    if (!ok) {
        fputs("common buffer/numeric test failed\n", stderr);
    }
}
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            require(0);                                                                            \
            return __LINE__;                                                                       \
        }                                                                                          \
    } while (0)
/* Verify ring-buffer wraparound, capacity, lengths, and numeric boundaries. */
int main(void) {
    uint8_t memory[40], output[40], input[16];
    for (unsigned i = 0; i < sizeof input; ++i)
        input[i] = (uint8_t)i;
    memset(memory, 0xa5, sizeof memory);
    LhdcCirbuf buffer;
    lhdc_cirbuf_init(&buffer, memory + 8, 8);
    CHECK(lhdc_cirbuf_put(&buffer, input, 4) == 4);
    CHECK(lhdc_cirbuf_get(&buffer, output, 3) == 3 && !memcmp(output, input, 3));
    CHECK(lhdc_cirbuf_put(&buffer, input + 4, 6) == 6);
    CHECK(lhdc_cirbuf_get(&buffer, output, 7) == 7);
    const uint8_t expected[] = {3, 4, 5, 6, 7, 8, 9};
    CHECK(!memcmp(output, expected, sizeof expected));
    CHECK(!lhdc_cirbuf_len(&buffer) && lhdc_cirbuf_empty_len(&buffer) == 8);
    lhdc_cirbuf_reset(&buffer);
    CHECK(lhdc_cirbuf_put(&buffer, input, 8) == 8);
    uint8_t *span = output;
    CHECK(!lhdc_cirbuf_get_no_copy(&buffer, &span, 9) && span == output);
    CHECK(lhdc_cirbuf_get_no_copy(&buffer, &span, 4) == 4 && span == memory + 8);
    CHECK(lhdc_cirbuf_put_no_copy(&buffer, &span, 9) == 4 && span == memory + 8);
    memcpy(span, input + 8, 4);
    span = output;
    CHECK(!lhdc_cirbuf_get_no_copy(&buffer, &span, 8) && span == memory + 12);
    CHECK(buffer.s_len == 8 && buffer.odx == 4);
    CHECK(lhdc_cirbuf_get_no_copy(&buffer, &span, 4) == 4 && !memcmp(span, input + 4, 4));
    CHECK(lhdc_cirbuf_get(&buffer, output, 4) == 4 && !memcmp(output, input + 8, 4));
    CHECK(lhdc_cirbuf_put(&buffer, input, 4) == 4);
    const int lengths[] = {0, -1, INT_MIN};
    for (unsigned i = 0; i < sizeof lengths / sizeof lengths[0]; ++i) {
        LhdcCirbuf saved = buffer;
        uint8_t saved_memory[40];
        memcpy(saved_memory, memory, sizeof memory);
        span = output;
        CHECK(!lhdc_cirbuf_get(&buffer, NULL, lengths[i]));
        CHECK(!lhdc_cirbuf_put(&buffer, NULL, lengths[i]));
        CHECK(!lhdc_cirbuf_get_no_copy(&buffer, &span, lengths[i]) && span == output);
        CHECK(!lhdc_cirbuf_put_no_copy(&buffer, &span, lengths[i]) && span == output);
        CHECK(buffer.idx == saved.idx && buffer.odx == saved.odx && buffer.s_len == saved.s_len &&
              buffer.r_len == saved.r_len);
        CHECK(!memcmp(saved_memory, memory, sizeof memory));
    }
    for (unsigned i = 0; i < 8; ++i)
        CHECK(memory[i] == 0xa5 && memory[16 + i] == 0xa5);
    lhdc_cirbuf_init(&buffer, NULL, -1);
    CHECK(buffer.max_len == 0 && !lhdc_cirbuf_put(&buffer, input, 8));
    const uint32_t patterns[] = {0,           0x80000000u, 0x3f800000u, 0x7f800000u,
                                 0xff800000u, 0x7fc12345u, 0xffc12345u};
    for (unsigned i = 0; i < sizeof patterns / sizeof patterns[0]; ++i)
        CHECK(audio_math_float32_bits(audio_math_float32_from_bits(patterns[i])) == patterns[i]);
    const uint64_t pattern64 = UINT64_C(0xfff8123456789abc);
    double number = audio_math_float64_from_bits(pattern64);
    uint64_t observed;
    memcpy(&observed, &number, sizeof observed);
    CHECK(observed == pattern64);
    CHECK(audio_math_int32_from_bits(UINT32_MAX) == -1 &&
          audio_math_int32_from_bits(UINT32_C(0x80000000)) == INT32_MIN);
    puts("common cirbuf boundaries and numeric bit patterns passed");
    return 0;
}
