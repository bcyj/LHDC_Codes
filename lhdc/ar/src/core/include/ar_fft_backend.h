#ifndef LHDC_AR_SRC_CORE_INCLUDE_AR_FFT_BACKEND_H
#define LHDC_AR_SRC_CORE_INCLUDE_AR_FFT_BACKEND_H

#include "ar_workspace_views.h"
#include "audio_math/fft_sse.h"

#ifndef LHDC_AR_HIGH_PERFORMANCE_SSE
#define LHDC_AR_HIGH_PERFORMANCE_SSE 0
#endif

/* Executes the AR 512-point complex FFT. The default path preserves the
 * reference operation order. The optional split-radix SSE path favors speed
 * and may change the least-significant output bits. */
static inline void lhdc_ar_fft_512(float *interleaved, int inverse,
                                   AudioMathFft512Work *work) {
#if LHDC_AR_HIGH_PERFORMANCE_SSE && AUDIO_MATH_HAS_FFT_SSE
    AudioMathComplex32 *data = (AudioMathComplex32 *)interleaved;
    if (audio_math_fft_sse_transform(data, work->input, 512, inverse)) {
        if (inverse) {
            const float scale = 1.0f / 512.0f;
            for (int32_t i = 0; i < 1024; ++i)
                interleaved[i] *= scale;
        }
        return;
    }
#endif
    audio_math_fft_512(interleaved, inverse, work);
}

#endif
