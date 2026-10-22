/* Generic C pointer-width scalars and PCM float interface without a processor-specific calling convention. */
#ifndef LHDC_GENERIC_ABI_H
#define LHDC_GENERIC_ABI_H
#include <stdint.h>
typedef intptr_t lhdc_long_t;
typedef uintptr_t lhdc_ulong_t;
typedef long long __int64;
#define V4_LOCAL_FLOAT_CC

/* Retain generic C float representation without platform return-convention adaptation. */
static inline void lhdc_normalize_pcm_float(float *sample) {
    (void)sample;
}
#endif
