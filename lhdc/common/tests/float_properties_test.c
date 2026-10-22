/* Check floating-point classification and special-value bits standalone or within numeric contract tests. */
#include <audio_math/float_properties.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define TEST_NOINLINE __declspec(noinline)
#else
#include <time.h>
#define TEST_NOINLINE __attribute__((noinline))
#endif

typedef struct FloatProperties {
    uint64_t magnitude;
    int nan, infinite, finite;
} FloatProperties;
typedef FloatProperties (*FloatOperation)(uint64_t, int);

/* Compute expected floating-point classifications and special values using standard expressions. */
static TEST_NOINLINE FloatProperties before_properties(uint64_t bits, int wide) {
    FloatProperties result;
    if (wide) {
        double value = audio_math_float64_from_bits(bits);
        result.magnitude = audio_math_float64_bits(fabs(value));
        result.nan = !!isnan(value);
        result.infinite = !!isinf(value);
        result.finite = !!isfinite(value);
    } else {
        float value = audio_math_float32_from_bits((uint32_t)bits);
        result.magnitude = audio_math_float32_bits(fabsf(value));
        result.nan = !!isnan(value);
        result.infinite = !!isinf(value);
        result.finite = !!isfinite(value);
    }
    return result;
}

/* Call common property helpers across function boundaries to inspect actual returned bits. */
static TEST_NOINLINE FloatProperties after_properties(uint64_t bits, int wide) {
    FloatProperties result;
    if (wide) {
        double value = audio_math_float64_from_bits(bits);
        result.magnitude = audio_math_float64_bits(audio_math_abs_f64(value));
        result.nan = audio_math_is_nan_f64(value);
        result.infinite = audio_math_is_infinite_f64(value);
        result.finite = audio_math_is_finite_f64(value);
    } else {
        float value = audio_math_float32_from_bits((uint32_t)bits);
        result.magnitude = audio_math_float32_bits(audio_math_abs_f32(value));
        result.nan = audio_math_is_nan_f32(value);
        result.infinite = audio_math_is_infinite_f32(value);
        result.finite = audio_math_is_finite_f32(value);
    }
    return result;
}
static FloatOperation volatile operations[2] = {before_properties, after_properties};
static uint64_t checks;

/* Check independent integer classification and host returns by fields, excluding structure padding. */
static void check(uint64_t bits, int wide) {
    FloatProperties before = operations[0](bits, wide), after = operations[1](bits, wide);
    uint64_t sign = wide ? UINT64_C(0x8000000000000000) : UINT64_C(0x80000000);
    uint64_t infinity = wide ? UINT64_C(0x7ff0000000000000) : UINT64_C(0x7f800000);
    uint64_t magnitude = bits & (sign - 1);
    if (after.nan != (magnitude > infinity) || after.infinite != (magnitude == infinity) ||
        after.finite != (magnitude < infinity) || before.nan != after.nan ||
        before.infinite != after.infinite || before.finite != after.finite ||
        before.magnitude != after.magnitude) {
        fprintf(stderr,
                "properties failure wide=%d bits=%016" PRIx64 " abs=%016" PRIx64 "/%016" PRIx64
                "\n",
                wide, bits, before.magnitude, after.magnitude);
        exit(1);
    }
    if (magnitude <= infinity && after.magnitude != magnitude)
        exit(2);
    ++checks;
}

/* Generate deterministic 64-bit samples without signed overflow. */
static uint64_t next_bits(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

/* Cover every exponent, sign, field boundary, and a million random patterns including signaling NaNs. */
void lhdc_test_float_properties(void) {
    static const uint64_t fractions[] = {0,
                                         1,
                                         2,
                                         3,
                                         15,
                                         16,
                                         17,
                                         UINT64_MAX,
                                         UINT64_MAX - 1,
                                         UINT64_MAX - 2,
                                         UINT64_MAX >> 1,
                                         (UINT64_MAX >> 1) - 1,
                                         (UINT64_MAX >> 1) + 1};
    checks = 0;
    for (int wide = 0; wide < 2; ++wide) {
        unsigned fraction_bits = wide ? 52 : 23, exponent_limit = wide ? 2048 : 256;
        uint64_t fraction_mask = (UINT64_C(1) << fraction_bits) - 1;
        for (unsigned exponent = 0; exponent < exponent_limit; ++exponent)
            for (unsigned sign = 0; sign < 2; ++sign)
                for (unsigned i = 0; i < sizeof fractions / sizeof fractions[0]; ++i) {
                    uint64_t fraction = fractions[i] & fraction_mask;
                    check(((uint64_t)sign << (wide ? 63 : 31)) |
                              ((uint64_t)exponent << fraction_bits) | fraction,
                          wide);
                    check(((uint64_t)sign << (wide ? 63 : 31)) |
                              ((uint64_t)exponent << fraction_bits) |
                              (fraction ^ (UINT64_C(1) << (fraction_bits - 1))),
                          wide);
                }
        uint64_t state = UINT64_C(0x56181795aa358f61);
        for (unsigned i = 0; i < 1048576; ++i) {
            uint64_t bits = next_bits(&state);
            check(wide ? bits : (uint32_t)bits, wide);
        }
    }
    printf("{\"property_cases\":%" PRIu64 ",\"oracle\":\"host-and-bits\"}\n", checks);
}

#ifdef LHDC_FLOAT_PROPERTIES_STANDALONE
static volatile uint64_t consumed;

/* Read a monotonic clock with the same timing protocol on the host and Android. */
static double seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER count, frequency;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&frequency);
    return (double)count.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec * 1e-9;
#endif
}

/* Alternate common and standard-expression timing over finite and exceptional inputs, checking return checksums. */
static void benchmark(int wide) {
    uint64_t inputs[256], state = UINT64_C(0x927519ab47e82d19);
    for (unsigned i = 0; i < 256; ++i) {
        uint64_t bits = next_bits(&state);
        inputs[i] = wide ? bits : (uint32_t)bits;
    }
    for (unsigned round = 0; round < 7; ++round) {
        double elapsed[2];
        uint64_t sums[2] = {0};
        for (unsigned iteration = 0; iteration < 2; ++iteration) {
            unsigned side = iteration ^ (round & 1);
            double begin = seconds();
            for (unsigned i = 0; i < 262144; ++i) {
                FloatProperties result = operations[side](inputs[i & 255], wide);
                sums[side] += result.magnitude + result.nan + result.infinite + result.finite;
            }
            elapsed[side] = seconds() - begin;
            consumed = sums[side];
        }
        if (sums[0] != sums[1])
            exit(3);
        printf("{\"round\":%u,\"operations\":262144,\"before\":%.12g,\"after\":%.12g,"
               "\"checksum\":\"%016" PRIx64 "\"}\n",
               round, elapsed[0], elapsed[1], sums[0]);
    }
}

/* Verify classification, special-value bits, and rounding/conversion boundaries. */
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--bench-32"))
        benchmark(0);
    else if (argc == 2 && !strcmp(argv[1], "--bench-64"))
        benchmark(1);
    else
        lhdc_test_float_properties();
    return 0;
}
#endif
