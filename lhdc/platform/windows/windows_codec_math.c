#include "include/lhdc_windows_exp_log.h"
#include <audio_math/float_rounding.h>
#include <audio_math/float_convert.h>
/* Shared Windows floating-point compatibility functions.
 * Copyright (c) 2017-2025, Arm Limited. SPDX-License-Identifier: MIT
 * See math/platform/windows/NOTICE.math.txt for provenance and the complete license. */
#include <audio_math/fma_native.h>
#include "include/lhdc_windows_float_math_constants.h"
#include <audio_math/constants.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "include/lhdc_windows_math_tables.h"

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
#include <intrin.h>
/* Select the default host single-precision log rounding model without CRT calls or mutable caching.
 * Choose fused only with AVX/FMA, OSXSAVE, and XMM/YMM enabled in XCR0.
 * This selects a math model only; common software FMA still computes it without AVX/FMA instructions. */
unsigned lhdc_windows_log10_policy(void) {
    int registers[4];
    __cpuid(registers, 1);
    const unsigned required = (1u << 12) | (1u << 27) | (1u << 28);
    if (((unsigned)registers[2] & required) != required)
        return 0;
    return (_xgetbv(0) & 6u) == 6u;
}
/* Compute a single-rounding float multiply-add within log10's reduced finite domain.
 * Nonzero call-site results are normal/finite and zero means exact cancellation; not a general FMA.
 * Double results exactly at float midpoints fall back to the integer core to avoid double rounding.
 * Native MSVC x64 only: return is already rounded to binary32, requiring no x87 spill. */
static inline float log10_fma_bounded(float a, float b, float c) {
    double sum = (double)a * (double)b + (double)c;
    uint64_t bits = audio_math_float64_bits_sse2(sum);
    if ((bits & UINT64_C(0x1fffffff)) == UINT64_C(0x10000000))
        return audio_math_fma_exact_f32(a, b, c);
    return (float)sum;
}
/* Reproduce two single-precision return models with 128-segment float reduction/log tables.
 * fused zero retains separate operation rounding; nonzero uses common single-rounding software FMA.
 * Only default-rounding return bits are specified, not errno, exceptions, or CRT manual switches. */
float lhdc_windows_log10f_soft(float input, unsigned fused) {
    uint32_t bits = audio_math_float32_bits_sse2(input), magnitude = bits & 0x7fffffffu;
    if (magnitude > 0x7f800000u)
        return audio_math_float32_from_bits(bits | 0x00400000u);
    if (!magnitude)
        return audio_math_float32_from_bits(0xff800000u);
    if (bits & 0x80000000u)
        return audio_math_float32_from_bits(0xffc00000u);
    if (bits == 0x7f800000u)
        return input;
    float difference = input - 1.0f;
    if ((audio_math_float32_bits_sse2(difference) & 0x7fffffffu) < 0x3d800000u) {
        float u = difference / (2.0f + difference);
        float correction = difference * u;
        u = u + u;
        float square = u * u, cube = u * square, tail;
        if (fused) {
            tail = log10_fma_bounded(LHDC_WIN_LOG10_SERIES_C5_F32, square, LHDC_WIN_LOG10_SERIES_C3_F32);
            tail = log10_fma_bounded(cube, tail, -correction);
        } else {
            tail = square * LHDC_WIN_LOG10_SERIES_C5_F32;
            tail = tail + LHDC_WIN_LOG10_SERIES_C3_F32;
            tail = tail * cube;
            tail = tail - correction;
        }
        float high = audio_math_float32_from_bits(audio_math_float32_bits_sse2(difference) & 0xffff0000u);
        tail = tail + (difference - high);
        float result = high * LHDC_WIN_LOG10_E_LOW_F32;
        if (fused)
            result = log10_fma_bounded(tail, LHDC_WIN_LOG10_E_LOW_F32, result);
        else
            result = result + tail * LHDC_WIN_LOG10_E_LOW_F32;
        result = result + tail * LHDC_WIN_LOG10_E_HIGH_F32;
        return result + high * LHDC_WIN_LOG10_E_HIGH_F32;
    }
    int exponent = (int)(bits >> 23) - 127;
    if (exponent == -127) {
        float normalized = audio_math_float32_from_bits((bits & 0x007fffffu) | 0x3f800000u) - 1.0f;
        bits = audio_math_float32_bits_sse2(normalized);
        exponent = (int)(bits >> 23) - 253;
    }
    unsigned rounded = (bits & 0x007f0000u) + ((bits & 0x00008000u) << 1);
    unsigned index = rounded >> 16;
    float center = audio_math_float32_from_bits(rounded | 0x3f000000u);
    float mantissa = audio_math_float32_from_bits((bits & 0x007fffffu) | 0x3f000000u);
    float reduced = (center - mantissa) * lhdc_win_log10f_table[index][0];
    float square = reduced * reduced, polynomial;
    if (fused) {
        polynomial = log10_fma_bounded(reduced, LHDC_WIN_LOG10_CUBIC_F32, 0.5f);
        polynomial = log10_fma_bounded(polynomial, square, reduced);
    } else {
        polynomial = reduced * LHDC_WIN_LOG10_CUBIC_F32;
        polynomial = polynomial + 0.5f;
        polynomial = polynomial * square;
        polynomial = polynomial + reduced;
    }
    polynomial = polynomial * LHDC_WIN_LOG10_E_F32;
    float e = (float)exponent, low, high;
    if (fused) {
        low = log10_fma_bounded(e, LHDC_WIN_LOG10_TWO_LOW_F32, -polynomial);
    } else {
        low = e * LHDC_WIN_LOG10_TWO_LOW_F32;
        low = low - polynomial;
    }
    /* e is an integer in [-149,127] and the high part is 77/256; multiplication is exact without FMA.
     * Share only high-part synthesis; low parts and preceding polynomials retain their rounding models. */
    high = e * LHDC_WIN_LOG10_TWO_HIGH_F32;
    /* Restore omitted zero bits in the high-part pattern without approximation. */
    high = high + audio_math_float32_from_bits((uint32_t)lhdc_win_log10f_high_bits[index] <<
                                          LHDC_WIN_LOG10_HIGH_ZERO_BITS);
    low = low + lhdc_win_log10f_table[index][1];
    return high + low;
}
#endif

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Scale a positive normal double exactly by a power of two, then round to float.
 * Results remain normal double in this call domain; exponent addition needs no host math library. */
static inline float positive_power_scale(double value, int scale) {
    uint64_t bits =
        audio_math_float64_bits_sse2(value) + (uint64_t)(int64_t)scale * UINT64_C(0x10000000000000);
    return (float)audio_math_float64_from_bits_sse2(bits);
}
/* Compute SNS base-ten powers or TNS [0.85f,1] powers 2..8 with host return rounding.
 * Arbitrary negative bases and errno are unsupported; uncertain fused results use common software FMA. */
float lhdc_windows_positive_powf_soft(float base, float exponent, unsigned fused) {
    uint32_t y = audio_math_float32_bits_sse2(exponent), magnitude = y & 0x7fffffffu;
    if (!magnitude)
        return 1.0f;
    if (magnitude > 0x7f800000u)
        return audio_math_float32_from_bits(y | 0x00400000u);
    if (magnitude == 0x7f800000u)
        return y >> 31 ? 0.0f : audio_math_float32_from_bits(0x7f800000u);
    if (exponent == 1.0f)
        return base;
    double x = (double)base, difference = x - 1.0, logarithm;
    unsigned exp_fused = fused;
    if (base == 10.0f) {
        /* Reduction coefficient is RN(RN(3*ln2)+ln1.25), rounded independently of mathematical ln10. */
        logarithm = LHDC_WIN_POW_LN10_POLICY_F64;
    } else if (difference > -LHDC_WIN_POW_NEAR_ONE_F64 && difference < LHDC_WIN_POW_NEAR_ONE_F64) {
        double u = difference / (difference + 2.0);
        double correction = u * difference;
        u = u + u;
        double square = u * u, cube = u * square;
        double low = square * LHDC_WIN_POW_LOG_C5_F64 + LHDC_WIN_POW_LOG_C3_F64;
        double high = square * LHDC_WIN_POW_LOG_C9_F64 + LHDC_WIN_POW_LOG_C7_F64;
        double fifth = square * square;
        fifth = fifth * cube;
        low = cube * low;
        high = fifth * high;
        logarithm = difference + ((low + high) - correction);
        /* The near-one fused entry rejoins the non-fused exponential path. */
        exp_fused = 0;
    } else {
        uint64_t bits = audio_math_float64_bits_sse2(x);
        unsigned index = (unsigned)((bits & UINT64_C(0xfffffffffffff)) >> 44);
        index += (unsigned)((bits >> 43) & 1u);
        double center =
            audio_math_float64_from_bits_sse2(((uint64_t)index << 44) | UINT64_C(0x3fe0000000000000));
        double mantissa = audio_math_float64_from_bits_sse2((bits & UINT64_C(0xfffffffffffff)) |
                                                 UINT64_C(0x3fe0000000000000));
        double reduced = (center - mantissa) * lhdc_win_pow_log_table[index - LHDC_WIN_POW_LOG_FIRST_INDEX][0];
        double polynomial;
        if (fused) {
            polynomial = audio_math_fma_f64(LHDC_WIN_POW_THIRD_F64, reduced, 0.5);
            polynomial = audio_math_fma_f64(polynomial, reduced, 1.0);
            polynomial = reduced * polynomial;
        } else {
            double quadratic = LHDC_WIN_POW_THIRD_F64 * reduced;
            quadratic = quadratic + 0.5;
            polynomial = reduced * reduced;
            polynomial = polynomial * quadratic;
            polynomial = polynomial + reduced;
        }
        int e = (int)(bits >> 52) - 1023;
        logarithm = (double)e * LHDC_WIN_POW_LN2_F64;
        logarithm = logarithm + lhdc_win_pow_log_table[index - LHDC_WIN_POW_LOG_FIRST_INDEX][1];
        logarithm = logarithm - polynomial;
    }
    double power = (double)exponent * logarithm;
    if (power > LHDC_WIN_POW_OVERFLOW_F64)
        return audio_math_float32_from_bits(0x7f800000u);
    if (power <= -LHDC_WIN_POW_UNDERFLOW_MAG_F64)
        return 0.0f;
    double scaled = power * LHDC_WIN_POW_REDUCTION_F64;
    /* Bounded segment indices use default MXCSR nearest-even mode without post-truncation branch correction. */
    int n = _mm_cvtsd_si32(_mm_set_sd(scaled));
    /* Evaluate separately rounded exponential polynomials first; fused models fall back only for uncertain float rounding. */
    double reduced = power - (double)n * LHDC_WIN_POW_LN2_OVER_64_F64;
    double quadratic = LHDC_WIN_POW_SIXTH_F64 * reduced;
    quadratic = quadratic + 0.5;
    double result = reduced * reduced;
    result = result * quadratic;
    result = result + reduced;
    result = result * audio_math_win_expf_sse2_table[(unsigned)n & 63u];
    result = result + audio_math_win_expf_sse2_table[(unsigned)n & 63u];
    /* Groups contain 64 segments; explicit arithmetic shift floors negative exponents rather than truncating toward zero. */
    int scale = audio_math_int32_from_bits(audio_math_shift_arithmetic_low8_sign32((uint32_t)n, 6));
    if (exp_fused) {
        /* A 2^-42 relative interval bounds both evaluation orders in this domain.
         * Accept the ordinary result only if both endpoints round identically after exact power-of-two scaling. */
        double guard = result * LHDC_WIN_POW_ROUND_GUARD_F64;
        float lower = positive_power_scale(result - guard, scale);
        float upper = positive_power_scale(result + guard, scale);
        if (audio_math_float32_bits_sse2(lower) == audio_math_float32_bits_sse2(upper))
            return lower;
        reduced = audio_math_fma_f64(-(double)n, LHDC_WIN_POW_LN2_OVER_64_F64, power);
        result = audio_math_fma_f64(LHDC_WIN_POW_SIXTH_F64, reduced, 0.5);
        result = audio_math_fma_f64(result, reduced, 1.0);
        result = reduced * result;
        result = audio_math_fma_f64(result, audio_math_win_expf_sse2_table[(unsigned)n & 63u],
                              audio_math_win_expf_sse2_table[(unsigned)n & 63u]);
    }
    return positive_power_scale(result, scale);
}
/* Round precomputed sequential double TNS powers, falling back when the interval cannot determine float rounding.
 * Bases [0.85f,1], powers 2..8 are exhaustively checked; near boundaries use existing software power without changing policy.
 * Not for arbitrary bases/real exponents; no CPU query or allocation. */
float lhdc_windows_tns_round_power_soft(double value, float base, unsigned exponent) {
    double guard = value * LHDC_WIN_TNS_POWER_ROUND_GUARD_F64;
    float lower = (float)(value - guard);
    float upper = (float)(value + guard);
    if (audio_math_float32_bits_sse2(lower) == audio_math_float32_bits_sse2(upper))
        return lower;
    /* Both host policies agree in this domain; do not query CPU again in the hot path. */
    return lhdc_windows_positive_powf_soft(base, (float)exponent, 0);
}
#endif
