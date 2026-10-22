#ifndef AUDIO_MATH_DSP_FMA_SOFT_H
#define AUDIO_MATH_DSP_FMA_SOFT_H
#include <audio_math/float_properties.h>
#include <audio_math/constants.h>
#include <audio_math/numeric_bits.h>
#include <math.h>
#include <audio_math/compiler.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
#include <emmintrin.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Compile the larger exact binary32 integer fallback once instead of duplicating it at each call site. */
float audio_math_fma_exact_f32(float a, float b, float c);
/* Use two 64-bit integer parts for nearest-even, single-rounding binary64. */
double audio_math_fma_soft_f64(double a, double b, double c);

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Share register bit copying with platform math instead of retaining FMA-specific copies. */
#define audio_math_fma_f64_bits audio_math_float64_bits_sse2
#define audio_math_fma_f64_from_bits audio_math_float64_from_bits_sse2
#else
/* Ordinary builds call common software entries directly, avoiding wrapper effects on objects/unwind data. */
#define audio_math_fma_f64_bits audio_math_float64_bits
#define audio_math_fma_f64_from_bits audio_math_float64_from_bits
#endif

/* Compute binary32 FMA through a provably safe double fast path.
 * The 48-bit product of finite binary32 operands is exactly representable in binary64.
 * A double sum at a float midpoint may lose decisive low bits and must fall back;
 * subnormals, zero, near-overflow, and nonfinite inputs also use exact integer single rounding.
 * This does not unconditionally replace FMA with (float)((double)a*b+c).
 */
static inline float audio_math_fma_soft_f32(float a, float b, float c) {
    double sum = (double)a * (double)b + (double)c;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* Positive finite magnitude bits follow numeric order; nonfinite values also exceed the bound and fall back. */
    uint64_t bits = audio_math_fma_f64_bits(sum);
    uint64_t magnitude = bits & AUDIO_MATH_FLOAT64_MAGNITUDE_BITS;
    if (magnitude < AUDIO_MATH_FLOAT64_FLOAT32_MIN_NORMAL_BITS ||
        magnitude > AUDIO_MATH_FLOAT64_FLOAT32_MAX_FINITE_BITS)
        return audio_math_fma_exact_f32(a, b, c);
#else
    double magnitude = audio_math_abs_f64(sum);
    if (!(magnitude >= (double)AUDIO_MATH_MIN_NORMAL_F32 &&
          magnitude <= (double)AUDIO_MATH_MAX_FINITE_F32))
        return audio_math_fma_exact_f32(a, b, c);
    uint64_t bits = audio_math_fma_f64_bits(sum);
#endif
    if ((bits & UINT64_C(0x1fffffff)) == UINT64_C(0x10000000))
        return audio_math_fma_exact_f32(a, b, c);
    /* Win32 returns through x87; explicitly round to binary32 without excess return precision. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* x64 returns are already binary32-rounded, requiring no volatile spill/reload. */
    return (float)sum;
#else
    volatile float rounded = (float)sum;
    return rounded;
#endif
}

#ifdef __cplusplus
}
#endif
#endif
