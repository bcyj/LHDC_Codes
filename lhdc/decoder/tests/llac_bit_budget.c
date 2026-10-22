#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../src/v4/include/llac_bit_budget.h"
#include "../src/v4/llac/spectral_tables.h"
#include <audio_math/float_rounding.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static float (*volatile system_log2)(float) = log2f;
static uint32_t random_state = UINT32_C(0x17592ec1);
static volatile uint64_t consumed;
static LLACArithmeticReader inputs[1024];
static LLACSideReader sides[1024];

/* Compute residual bits using the floating budget formula for integer-budget verification and timing. */
static int baseline_budget(const LLACArithmeticReader *reader, const LLACSideReader *side,
                           int frame_bits) {
    float mask_log2 = log2f((float)side->mask);
    float range_log2 = log2f((float)reader->range);
    float side_position = (float)(side->byte_position * 8 + 16);
    float arithmetic_position = (float)(reader->byte_position * 8 + 9);
    range_log2 = audio_math_floor_f32(range_log2);
    return frame_bits - ((int)((float)frame_bits - (side_position - mask_log2)) +
                         (int)(arithmetic_position - range_log2));
}
static int (*volatile functions[2])(const LLACArithmeticReader *, const LLACSideReader *,
                                    int) = {baseline_budget, llac_spectral_residual_budget};

/* Generate deterministic cursor, mask, and range test inputs. */
static uint32_t random_bits(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

/* Verify positive probability frequencies summing to 1024 per row, keeping arithmetic ranges within 24 bits. */
static int check_frequencies(const uint32_t *frequency, unsigned count) {
    unsigned total = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (!frequency[i] || frequency[i] > 1024)
            return 0;
        total += frequency[i];
    }
    return total == 1024;
}

/* Read the host monotonic clock for tests only. */
static double now_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER ticks, frequency;
    QueryPerformanceCounter(&ticks);
    QueryPerformanceFrequency(&frequency);
    return (double)ticks.QuadPart / frequency.QuadPart;
#else
    struct timespec ticks;
    clock_gettime(CLOCK_MONOTONIC, &ticks);
    return ticks.tv_sec + ticks.tv_nsec * 1e-9;
#endif
}

/* Time complete local budget calculations over actual ranges, power-of-two neighborhoods, and low ranges. */
static int benchmark(void) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    for (unsigned group = 0; group < 3; ++group) {
        for (unsigned i = 0; i < 1024; ++i) {
            uint32_t bits = random_bits();
            uint32_t range = group == 0   ? (bits % UINT32_C(0xff0000)) + UINT32_C(0x10000)
                             : group == 1 ? (UINT32_C(1) << (16 + i % 9)) - (i % 16 + 1)
                                          : bits % UINT32_C(0xffff) + 1;
            inputs[i].range = range;
            inputs[i].byte_position = (int)(i % 300 + 3);
            sides[i].byte_position = (int)(511 - i % 400);
            sides[i].mask = UINT32_C(1) << (i & 7);
        }
        for (unsigned round = 0; round < 9; ++round) {
            double times[2];
            uint64_t sums[2];
            for (unsigned visit = 0; visit < 2; ++visit) {
                unsigned side = (visit + round) & 1;
                uint64_t sum = 0;
                double start = now_seconds();
                for (unsigned i = 0; i < 262144; ++i)
                    sum += (uint32_t)functions[side](&inputs[i & 1023], &sides[i & 1023], 4096);
                times[side] = now_seconds() - start;
                sums[side] = sum;
                consumed = sum;
            }
            if (sums[0] != sums[1])
                return 1;
            printf("{\"range\":%u,\"round\":%u,\"operations\":262144,\"before\":%.12g,"
                   "\"after\":%.12g,\"checksum\":\"%016" PRIx64 "\"}\n",
                   group, round, times[0], times[1], sums[0]);
        }
    }
    return 0;
}

/* Exhaust logarithm truncation for all positive 24-bit integers, then check masks, cursors, and complete residual budgets. */
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--bench") == 0)
        return benchmark();
    unsigned models = 0;
    for (unsigned i = 0; i < 2; ++i) {
        if (!check_frequencies(lhdc_decoder_v4_llac_spectral_tns_order_models[i].frequency, 8))
            return 1;
        ++models;
    }
    for (unsigned i = 0; i < 8; ++i) {
        if (!check_frequencies(lhdc_decoder_v4_llac_spectral_tns_reflection_models[i].frequency,
                               17))
            return 1;
        ++models;
    }
    for (unsigned i = 0; i < 64; ++i) {
        if (!check_frequencies(lhdc_decoder_v4_llac_spectral_spectral_pair_models[i].frequency, 17))
            return 1;
        ++models;
    }
    unsigned rounded_boundaries = 0;
    for (uint32_t range = 1; range < UINT32_C(0x1000000); ++range) {
        unsigned expected = (unsigned)system_log2((float)range);
        unsigned actual = llac_spectral_range_bits(range);
        if (expected != actual) {
            printf("FAIL range=%u expected=%u actual=%u\n", range, expected, actual);
            return 1;
        }
        rounded_boundaries += actual != 31u - audio_math_count_leading_zeros32(range);
    }
    for (unsigned bit = 0; bit < 8; ++bit)
        if (system_log2((float)(UINT32_C(1) << bit)) != (float)bit)
            return 1;
    for (unsigned i = 0; i < 1000000; ++i) {
        LLACArithmeticReader reader = {0};
        LLACSideReader side = {0};
        reader.range = random_bits() % UINT32_C(0xffffff) + 1;
        reader.byte_position = (int)(random_bits() % UINT32_C(0x200000)) - 512;
        side.mask = UINT32_C(1) << (random_bits() & 7);
        side.byte_position = (int)(random_bits() % UINT32_C(0x200000)) - 512;
        int frame_bits = (int)(random_bits() % UINT32_C(0x1000000));
        int expected = functions[0](&reader, &side, frame_bits);
        int actual = functions[1](&reader, &side, frame_bits);
        if (expected != actual) {
            printf("FAIL budget=%d expected=%d\n", actual, expected);
            return 1;
        }
    }
    if (rounded_boundaries != 19 || models != 74)
        return 1;
    printf(
        "PASS range_checks=16777215 budget_checks=1000000 masks=8 rounded_boundaries=19 models=74\n");
    return 0;
}
