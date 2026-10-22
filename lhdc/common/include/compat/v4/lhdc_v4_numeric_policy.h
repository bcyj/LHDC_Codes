#include "../../../../platform/include/lhdc_numeric_policy.h"
#ifndef LHDC_V4_NUMERIC_POLICY_H
#define LHDC_V4_NUMERIC_POLICY_H
#include <stdint.h>

/* 5: use the low five count bits; 8: use the low eight bits and zero/sign-extend beyond the word width.
 * Defaults preserve validated ABI outputs; either numeric model can be selected explicitly on one host.
 * Algorithms no longer dispatch shifts by pointer width. This is not a bug-fix switch.
 */
#ifndef LHDC_V4_SHIFT_COUNT_BITS
#if defined(LHDC_V4_ARM32_SHIFT_SEMANTICS) || UINTPTR_MAX == UINT32_MAX
#define LHDC_V4_SHIFT_COUNT_BITS 8
#else
#define LHDC_V4_SHIFT_COUNT_BITS 5
#endif
#endif
#if LHDC_V4_SHIFT_COUNT_BITS != 5 && LHDC_V4_SHIFT_COUNT_BITS != 8
#error LHDC_V4_SHIFT_COUNT_BITS_must_be_5_or_8
#endif

/* Continuous-logarithm rounding model, independent of the shift model. Defaults preserve validated ABI bitstreams.
 * 1 uses logf(x)/ln(2); 0 uses the single-precision log2 backend. Not an ARM acceleration switch.
 */
#ifndef LHDC_V4_LOG2_VIA_LN
#if UINTPTR_MAX == UINT32_MAX
#define LHDC_V4_LOG2_VIA_LN 1
#else
#define LHDC_V4_LOG2_VIA_LN 0
#endif
#endif
#if LHDC_V4_LOG2_VIA_LN != 0 && LHDC_V4_LOG2_VIA_LN != 1
#error LHDC_V4_LOG2_VIA_LN_must_be_0_or_1
#endif

/* Return policy for LLAC integer gain steps; not a bug-fix or acceleration switch.
 * 0 retains host powf; 1 uses the verified common table; 2 preserves ten UCRT x64 differences.
 * Unverified hosts use the system entry point; pointer width does not determine math-library behavior. */
#define LHDC_V4_LLAC_GAIN_HOST 0
#define LHDC_V4_LLAC_GAIN_COMMON 1
#define LHDC_V4_LLAC_GAIN_UCRT64 2
#ifndef LHDC_V4_LLAC_GAIN_MODEL
#if LHDC_PLATFORM_LLAC_GAIN_UCRT64
#define LHDC_V4_LLAC_GAIN_MODEL LHDC_V4_LLAC_GAIN_UCRT64
#elif LHDC_PLATFORM_LLAC_GAIN_COMMON
#define LHDC_V4_LLAC_GAIN_MODEL LHDC_V4_LLAC_GAIN_COMMON
#else
#define LHDC_V4_LLAC_GAIN_MODEL LHDC_V4_LLAC_GAIN_HOST
#endif
#endif
#if LHDC_V4_LLAC_GAIN_MODEL < 0 || LHDC_V4_LLAC_GAIN_MODEL > 2
#error LHDC_V4_LLAC_GAIN_MODEL_must_be_0_1_or_2
#endif
#endif
