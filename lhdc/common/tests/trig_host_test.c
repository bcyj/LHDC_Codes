#include "trig_host_reference.h"
#include <audio_math/numeric_bits.h>
#include <audio_math/constants.h>
#include <audio_math/float_rounding.h>
#include "../include/dsp/float_compat.h"
#include <math.h>
#include <audio_math/math.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/* Observe the double-precision bits returned by host sine. */
static uint64_t current_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float64_bits(audio_math_host_sin(x));
}

/* Observe the double-precision bits returned by host cosine. */
static uint64_t current_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float64_bits(audio_math_host_cos(x));
}

/* Observe double-sine conversion to single-precision coefficient bits. */
static uint64_t current_sin_f32(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float32_bits((float)audio_math_host_sin(x));
}

/* Observe double-cosine conversion to single-precision coefficient bits. */
static uint64_t current_cos_f32(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float32_bits((float)audio_math_host_cos(x));
}

/* Check a window formed from two sine evaluations and intermediate double multiplication. */
static uint64_t current_window(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double inner = audio_math_host_sin(x);
    double phase = (inner * (double)AUDIO_MATH_HALF_PI_F32) * inner;
    return audio_math_float32_bits((float)audio_math_host_sin(phase));
}

/* Check saturated Q31 conversion of single-precision sine coefficients. */
static uint64_t current_q31_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    float value = (float)audio_math_host_sin(x);
    return (uint32_t)audio_math_truncate_sat_i32_f64((double)(value * AUDIO_MATH_POW2_31_F32));
}

/* Check saturated Q31 conversion of single-precision cosine coefficients. */
static uint64_t current_q31_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    float value = (float)audio_math_host_cos(x);
    return (uint32_t)audio_math_truncate_sat_i32_f64((double)(value * AUDIO_MATH_POW2_31_F32));
}

/* Check double scaling, floor, and saturation of FFT sine coefficients. */
static uint64_t current_fft_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double value = audio_math_host_sin(x) * AUDIO_MATH_INT32_MAX_F64 + 0.5;
    return (uint32_t)audio_math_truncate_sat_i32_f64(audio_math_floor_f64(value));
}

/* Check double scaling, floor, and saturation of FFT cosine coefficients. */
static uint64_t current_fft_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double value = audio_math_host_cos(x) * AUDIO_MATH_INT32_MAX_F64 + 0.5;
    return (uint32_t)audio_math_truncate_sat_i32_f64(audio_math_floor_f64(value));
}

static LhdcTrigHostFunction volatile current_functions[LHDC_TEST_TRIG_HOST_OPERATIONS] = {
    current_sin,     current_cos,     current_sin_f32, current_cos_f32, current_window,
    current_q31_sin, current_q31_cos, current_fft_sin, current_fft_cos};

static uint32_t state = UINT32_C(0x17430692);
static uint64_t cases, checks;
static uint64_t input_x[1024], input_y[1024];
static volatile uint64_t consumed;

/* Generate deterministic full-width floating-point bit patterns. */
static uint32_t random_bits(void) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

/* Compare trig results, coefficients, and window expressions bitwise, including NaN payloads and negative zero. */
static int check_all(uint64_t x, uint64_t y) {
    ++cases;
    for (unsigned op = 0; op < LHDC_TEST_TRIG_HOST_OPERATIONS; ++op) {
        uint64_t before = lhdc_test_trig_host_reference[op](x, y);
        uint64_t after = current_functions[op](x, y);
        ++checks;
        if (before != after) {
            printf("FAIL op=%u x=%016" PRIx64 " y=%016" PRIx64 " before=%016" PRIx64
                   " after=%016" PRIx64 "\n",
                   op, x, y, before, after);
            return 0;
        }
    }
    return 1;
}

/* Read the host monotonic clock for testing only. */
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

/* Time finite and wide-bit-pattern domains per operation, alternating order and checking checksums. */
static int benchmark(void) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    for (unsigned op = 0; op < LHDC_TEST_TRIG_HOST_OPERATIONS; ++op) {
        for (unsigned group = 0; group < 2; ++group) {
            for (unsigned i = 0; i < 1024; ++i) {
                if (group) {
                    input_x[i] = ((uint64_t)random_bits() << 32) | random_bits();
                    input_y[i] = ((uint64_t)random_bits() << 32) | random_bits();
                } else {
                    double x = ((double)i / 1024.0 - 0.5) * AUDIO_MATH_TWO_PI_F64;
                    input_x[i] = audio_math_float64_bits(x);
                    input_y[i] = 0;
                }
            }
            for (unsigned round = 0; round < 7; ++round) {
                double times[2];
                uint64_t sums[2];
                for (unsigned visit = 0; visit < 2; ++visit) {
                    unsigned side = (visit + round) & 1;
                    LhdcTrigHostFunction fn =
                        side ? current_functions[op] : lhdc_test_trig_host_reference[op];
                    uint64_t sum = 0;
                    double start = now_seconds();
                    for (unsigned i = 0; i < 65536; ++i)
                        sum += fn(input_x[i & 1023], input_y[i & 1023]);
                    times[side] = now_seconds() - start;
                    sums[side] = sum;
                    consumed = sum;
                }
                if (sums[0] != sums[1])
                    return 1;
                printf("{\"op\":%u,\"range\":%u,\"round\":%u,\"operations\":65536,"
                       "\"before\":%.12g,\"after\":%.12g,\"checksum\":\"%016" PRIx64 "\"}\n",
                       op, group, round, times[0], times[1], sums[0]);
            }
        }
    }
    return 0;
}

/* Cover exceptional values, all exponents, actual initialization phases, and 250,000 random input pairs. */
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--bench") == 0)
        return benchmark();
    const uint32_t f32[] = {0,           0x80000000u, 0x3f800000u, 0xbf800000u, 0x3f800001u,
                            0x3f7fffffu, 0xbf800001u, 0xbf7fffffu, 0x00800000u, 0x007fffffu,
                            1,           0x7f7fffffu, 0xff7fffffu, 0x7f800000u, 0xff800000u,
                            0x7fc12345u, 0xffc12345u, 0x7f812345u, 0xff812345u, 0x40000000u};
    const uint64_t f64[] = {0,
                            UINT64_C(0x8000000000000000),
                            UINT64_C(0x3ff0000000000000),
                            UINT64_C(0xbff0000000000000),
                            UINT64_C(0x3ff0000000000001),
                            UINT64_C(0x3fefffffffffffff),
                            UINT64_C(0xbff0000000000001),
                            UINT64_C(0xbfefffffffffffff),
                            UINT64_C(0x0010000000000000),
                            UINT64_C(0x000fffffffffffff),
                            1,
                            UINT64_C(0x7fefffffffffffff),
                            UINT64_C(0xffefffffffffffff),
                            UINT64_C(0x7ff0000000000000),
                            UINT64_C(0xfff0000000000000),
                            UINT64_C(0x7ff8123456789abc),
                            UINT64_C(0xfff8123456789abc),
                            UINT64_C(0x7ff0123456789abc),
                            UINT64_C(0xfff0123456789abc),
                            UINT64_C(0x4000000000000000)};
    for (unsigned i = 0; i < 20; ++i)
        for (unsigned j = 0; j < 20; ++j)
            if (!check_all(audio_math_float64_bits((double)audio_math_float32_from_bits(f32[i])), f32[j]) ||
                !check_all(f64[i], f64[j]))
                return 1;
    const uint32_t fraction32[] = {0, 1, 0x003fffffu, 0x007fffffu};
    for (unsigned exponent = 0; exponent < 256; ++exponent)
        for (unsigned variant = 0; variant < 8; ++variant) {
            uint32_t x =
                (exponent << 23) | fraction32[variant & 3] | (variant & 4 ? 0x80000000u : 0);
            if (!check_all(audio_math_float64_bits((double)audio_math_float32_from_bits(x)), 0))
                return 1;
        }
    const uint64_t fraction64[] = {0, 1, UINT64_C(0x0007ffffffffffff),
                                   UINT64_C(0x000fffffffffffff)};
    for (unsigned exponent = 0; exponent < 2048; ++exponent)
        for (unsigned variant = 0; variant < 8; ++variant) {
            uint64_t x = ((uint64_t)exponent << 52) | fraction64[variant & 3] |
                         (variant & 4 ? UINT64_C(0x8000000000000000) : 0);
            if (!check_all(x, variant & 1 ? UINT64_C(0x3fe0000000000000)
                                          : UINT64_C(0xbff0000000000000)))
                return 1;
        }
    for (unsigned i = 0; i < 2048; ++i) {
        double x = (double)i / 16.0 - 64.0, y = (double)i / 64.0 - 16.0;
        if (!check_all(audio_math_float64_bits((double)(float)x), audio_math_float32_bits((float)y)) ||
            !check_all(audio_math_float64_bits(x), audio_math_float64_bits(y)))
            return 1;
    }
    const int lengths[] = {2,   6,   10,  14,  22,  26,  32,   44,   48,   60,   64,   88,   96,
                           120, 128, 160, 176, 192, 220, 240,  256,  320,  352,  384,  440,  480,
                           512, 640, 704, 768, 880, 960, 1024, 1280, 1408, 1536, 1760, 1920, 2048};
    for (unsigned n = 0; n < sizeof lengths / sizeof lengths[0]; ++n) {
        int length = lengths[n];
        for (int i = 0; i < length / 2; ++i) {
            float pre = (((float)i + 0.25f) * -AUDIO_MATH_PI_F32) / (float)length;
            float post = ((float)i * -AUDIO_MATH_PI_F32) / (float)length;
            double fft_phase = (double)i * -(double)AUDIO_MATH_TWO_PI_F32 / (double)length;
            if (!check_all(audio_math_float64_bits((double)pre), 0) ||
                !check_all(audio_math_float64_bits((double)post), 0) ||
                !check_all(audio_math_float64_bits(fft_phase), 0))
                return 1;
        }
        for (int i = 0; i < length; ++i) {
            float phase = ((float)i + 0.5f) * AUDIO_MATH_HALF_PI_F32 / (float)length;
            if (!check_all(audio_math_float64_bits((double)phase), 0))
                return 1;
        }
    }
    for (unsigned i = 0; i < 250000; ++i) {
        uint64_t x = ((uint64_t)random_bits() << 32) | random_bits();
        uint64_t y = ((uint64_t)random_bits() << 32) | random_bits();
        if (!check_all(x, y))
            return 1;
    }
    printf("PASS trig_host_cases=%" PRIu64 " checks=%" PRIu64 " operations=9\n", cases, checks);
    return 0;
}
