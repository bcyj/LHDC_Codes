/* Android NDK ILP32/LP64 types and ARM32 native floating-point calling convention. */
#ifndef LHDC_ANDROID_ABI_H
#define LHDC_ANDROID_ABI_H
#include <stdint.h>
typedef long lhdc_long_t;
typedef unsigned long lhdc_ulong_t;
typedef long long __int64;
#if defined(__arm__)
#define V4_LOCAL_FLOAT_CC __attribute__((pcs("aapcs-vfp")))
#else
#define V4_LOCAL_FLOAT_CC
#endif

/* Android PCM retains native floating representation without Windows NaN adaptation. */
static inline void lhdc_normalize_pcm_float(float *sample) {
    (void)sample;
}
#endif
