#ifndef LHDC_AR_NUMERIC_POLICY_H
#define LHDC_AR_NUMERIC_POLICY_H
#include <stdint.h>

/* Floating-point rounding policy, independent of CPU execution capability.
 * Defaults preserve existing PCM for each ABI; explicit selection uses the same policy on any host.
 * 32 preserves negative zero and input-power NaN payloads; 64 preserves an alternate convolution accumulation order.
 */
#ifndef LHDC_AR_ROUNDING_PROFILE
#if UINTPTR_MAX == UINT32_MAX
#define LHDC_AR_ROUNDING_PROFILE 32
#else
#define LHDC_AR_ROUNDING_PROFILE 64
#endif
#endif
#if LHDC_AR_ROUNDING_PROFILE != 32 && LHDC_AR_ROUNDING_PROFILE != 64
#error LHDC_AR_ROUNDING_PROFILE_must_be_32_or_64
#endif
#endif
