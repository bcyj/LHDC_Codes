#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../src/v4/llac/gain.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static float (*volatile system_power)(float, float) = powf;
static uint32_t state = UINT32_C(0x7415032d);
static uint64_t checks;
static float inputs[1024];
static volatile uint64_t consumed;

/* Compute gain with integer-step division and system power for numeric verification/timing. */
static float baseline_gain(float step) {
    return powf(10.0f, step / 28.0f);
}
static float (*volatile functions[2])(float) = {baseline_gain, llac_gain_from_step};

/* Generate deterministic integer/floating bit-pattern samples. */
static uint32_t random_bits(void) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

/* Compare current default policy bitwise with the host entry. */
static int check_step(float step) {
    uint32_t expected = audio_math_float32_bits(system_power(10.0f, step / 28.0f));
    uint32_t compiled_reference = audio_math_float32_bits(functions[0](step));
    uint32_t actual = audio_math_float32_bits(functions[1](step));
    ++checks;
    if (expected != actual || expected != compiled_reference) {
        printf("FAIL step=%08" PRIx32 " expected=%08" PRIx32 " compiled=%08" PRIx32
               " actual=%08" PRIx32 "\n",
               audio_math_float32_bits(step), expected, compiled_reference, actual);
        return 0;
    }
    return 1;
}

/* Read monotonic time for tests only. */
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

/* Time common steps, the complete finite range, saturation, and extremes, alternating order and verifying checksums. */
static int benchmark(void) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    for (unsigned group = 0; group < 3; ++group) {
        for (unsigned i = 0; i < 1024; ++i) {
            int step = group == 0   ? (int)(random_bits() % 396) - 250
                       : group == 1 ? (int)(random_bits() % 2346) - 1265
                                    : (int)(random_bits() % 200001) - 100000;
            inputs[i] = (float)step;
        }
        for (unsigned round = 0; round < 9; ++round) {
            double times[2];
            uint64_t sums[2];
            for (unsigned visit = 0; visit < 2; ++visit) {
                unsigned side = (round + visit) & 1;
                uint64_t sum = 0;
                double start = now_seconds();
                for (unsigned i = 0; i < 262144; ++i)
                    sum += audio_math_float32_bits(functions[side](inputs[i & 1023]));
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

/* Verify both table policies using independently collected host results without duplicating patch conditions in tests. */
static int check_model_file(const char *path, unsigned model) {
    FILE *file = fopen(path, "rb");
    if (!file)
        return 2;
    for (int step = -1266; step <= 1081; ++step) {
        uint32_t expected;
        if (fread(&expected, sizeof expected, 1, file) != 1 ||
            expected != audio_math_float32_bits(llac_gain_from_step_model((float)step, model))) {
            fclose(file);
            printf("FAIL model=%u step=%d\n", model, step);
            return 1;
        }
    }
    int extra = fgetc(file);
    int invalid = ferror(file) || extra != EOF;
    fclose(file);
    return invalid;
}

/* Exhaust finite results, scan saturation boundaries, and cover a million int32 steps plus special patterns. */
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--bench") == 0)
        return benchmark();
    if (argc == 4 && strcmp(argv[1], "--models") == 0) {
        if (check_model_file(argv[2], LHDC_V4_LLAC_GAIN_COMMON) ||
            check_model_file(argv[3], LHDC_V4_LLAC_GAIN_UCRT64))
            return 1;
        puts("PASS gain_models=2 checks=4696");
        return 0;
    }
    for (int step = -100000; step <= 100000; ++step)
        if (!check_step((float)step))
            return 1;
    for (unsigned i = 0; i < 1000000; ++i)
        if (!check_step((float)audio_math_int32_from_bits(random_bits())))
            return 1;
    const uint32_t specials[] = {0,           0x80000000u, 0x7f800000u, 0xff800000u, 0x7f7fffffu,
                                 0xff7fffffu, 0x7fc12345u, 0xffc12345u, 0x7f812345u, 0xff812345u};
    for (unsigned i = 0; i < sizeof specials / sizeof *specials; ++i)
        if (!check_step(audio_math_float32_from_bits(specials[i])))
            return 1;
    printf("PASS gain_checks=%" PRIu64 " model=%u\n", checks, (unsigned)LHDC_V4_LLAC_GAIN_MODEL);
    return 0;
}
