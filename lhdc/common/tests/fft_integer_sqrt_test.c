#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/bits.h>
#include "../include/dsp/fft_float32.h"
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static double (*volatile reference_sqrt)(double) = sqrt;
static uint64_t checks;
static uint32_t random_state = UINT32_C(0x517cc1b7);

/* Generate reproducible full-width unsigned input. */
static uint32_t random_bits(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

/* Check integer results against independent squared inequalities and converted double square roots. */
static int check_value(uint32_t value) {
    uint32_t actual = audio_math_sqrt_floor_u32(value);
    uint32_t expected = (uint32_t)reference_sqrt((double)value);
    uint64_t next = (uint64_t)actual + 1;
    ++checks;
    if (actual > 65535 || (uint64_t)actual * actual > value || next * next <= value ||
        actual != expected) {
        printf("FAIL value=%" PRIu32 " expected=%" PRIu32 " actual=%" PRIu32 "\n", value, expected,
               actual);
        return 0;
    }
    return 1;
}

/* Exhaust common and extended positive FFT lengths, comparing complete factor arrays and every stage remainder. */
static int check_factors(uint64_t *checksum) {
    for (int32_t length = 1; length <= 65536; ++length) {
        int32_t previous[64], current[64];
        memset(previous, 0xa5, sizeof previous);
        memset(current, 0xa5, sizeof current);
        int32_t old_threshold = (int32_t)reference_sqrt((double)length);
        int32_t new_threshold = (int32_t)audio_math_sqrt_floor_u32((uint32_t)length);
        lhdc_kiss_fft_factor(length, old_threshold, previous);
        lhdc_kiss_fft_factor(length, new_threshold, current);
        if (memcmp(previous, current, sizeof current)) {
            printf("FAIL factors length=%" PRId32 "\n", length);
            return 0;
        }
        int32_t remainder = length;
        int completed = 0;
        for (unsigned pair = 0; pair < 32; ++pair) {
            int32_t radix = current[2 * pair];
            int32_t quotient = current[2 * pair + 1];
            if (radix < 1 || remainder % radix || remainder / radix != quotient)
                return 0;
            remainder = quotient;
            *checksum = (*checksum ^ (uint32_t)radix) * UINT64_C(1099511628211);
            *checksum = (*checksum ^ (uint32_t)quotient) * UINT64_C(1099511628211);
            if (quotient == 1) {
                completed = 1;
                break;
            }
        }
        if (!completed)
            return 0;
    }
    return 1;
}

/* Retain the pre-change sqrt(double)-then-truncate entry for comparison. */
static uint32_t baseline_threshold(uint32_t value) {
    return (uint32_t)(int32_t)sqrt((double)(int32_t)value);
}
/* Benchmark the full uint32_t domain only; production thresholds use validated positive int32_t lengths. */
static uint32_t baseline_full_width(uint32_t value) {
    return (uint32_t)(int32_t)sqrt((double)value);
}
static uint32_t (*volatile implementations[2])(uint32_t) = {baseline_threshold,
                                                            audio_math_sqrt_floor_u32};
static uint32_t samples[1024];
static volatile uint64_t consumed;

/* Read the host monotonic timer without participating in product implementation. */
static double now_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart / frequency.QuadPart;
#else
    struct timespec counter;
    clock_gettime(CLOCK_MONOTONIC, &counter);
    return counter.tv_sec + counter.tv_nsec * 1e-9;
#endif
}

/* Time actual FFT lengths, 16-bit input, and full-width input with matching call boundaries and checksums. */
static int benchmark(void) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    const uint32_t lengths[] = {60, 110, 120, 128, 220, 240, 256, 440, 480, 512, 960, 1024, 1920};
    for (unsigned range = 0; range < 3; ++range) {
        implementations[0] = range == 2 ? baseline_full_width : baseline_threshold;
        for (unsigned k = 0; k < 1024; ++k)
            samples[k] = range == 0   ? lengths[k % (sizeof lengths / sizeof *lengths)]
                         : range == 1 ? random_bits() & 65535u
                                      : random_bits();
        for (unsigned round = 0; round < 9; ++round) {
            double times[2];
            uint64_t sums[2];
            for (unsigned visit = 0; visit < 2; ++visit) {
                unsigned side = visit ^ (round & 1);
                uint64_t sum = 0;
                double start = now_seconds();
                for (unsigned k = 0; k < 262144; ++k)
                    sum += implementations[side](samples[k & 1023]);
                times[side] = now_seconds() - start;
                sums[side] = sum;
                consumed = sum;
            }
            if (sums[0] != sums[1])
                return 1;
            printf("{\"range\":%u,\"round\":%u,\"operations\":262144,\"before\":%.12g,"
                   "\"after\":%.12g,\"checksum\":\"%016" PRIx64 "\"}\n",
                   range, round, times[0], times[1], sums[0]);
        }
    }
    return 0;
}

/* Cover all square boundaries, low-value domains, a million full-width random values, and complete FFT factor arrays. */
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--bench") == 0)
        return benchmark();
    for (uint32_t value = 0; value <= 65535; ++value) {
        if (!check_value(value))
            return 1;
    }
    for (uint32_t root = 1; root <= 65535; ++root) {
        uint32_t square = root * root;
        if (!check_value(square - 1) || !check_value(square) || !check_value(square + 1))
            return 1;
    }
    for (unsigned k = 0; k < 1000000; ++k) {
        if (!check_value(random_bits()))
            return 1;
    }
    const uint32_t limits[] = {0, 1, 2, UINT32_MAX, UINT32_MAX - 1, INT32_MAX};
    for (unsigned k = 0; k < sizeof limits / sizeof *limits; ++k) {
        if (!check_value(limits[k]))
            return 1;
    }
    uint64_t checksum = UINT64_C(14695981039346656037);
    if (!check_factors(&checksum))
        return 1;
    printf("PASS sqrt_checks=%" PRIu64 " factor_lengths=65536 checksum=%016" PRIx64 "\n", checks,
           checksum);
    return 0;
}
