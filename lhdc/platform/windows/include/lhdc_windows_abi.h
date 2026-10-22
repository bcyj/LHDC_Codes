#include <audio_math/float_properties.h>
#include <audio_math/constants.h>
/* Windows LLP64/ILP32 ABI and Android numeric-behavior compatibility. */
#ifndef LHDC_WINDOWS_ABI_H
#define LHDC_WINDOWS_ABI_H
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "lhdc_msvc_intrinsics.h"
#include <audio_math/math.h>
typedef intptr_t lhdc_long_t;
typedef uintptr_t lhdc_ulong_t;
#define V4_LOCAL_FLOAT_CC

/* Preserve ARM32 PCM division's positive default NaN on Win32. */
static inline void lhdc_normalize_pcm_float(float *sample) {
#ifndef _WIN64
    if (audio_math_is_nan_f32(*sample)) {
        uint32_t bits = 0x7fc00000u;
        memcpy(sample, &bits, sizeof bits);
    }
#else
    (void)sample;
#endif
}
#endif
