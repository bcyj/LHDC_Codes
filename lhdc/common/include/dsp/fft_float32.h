#ifndef LHDC_DSP_FFT_FLOAT32_H
#define LHDC_DSP_FFT_FLOAT32_H
#include "../types/lhdc_dsp_types.h"
#include <audio_math/fft_float32.h>
#include <audio_math/fft_sse.h>
typedef AudioMathFftPlan LhdcKissFftPlan;
typedef AudioMathFft512Work LhdcKissFft512Work;
#define lhdc_kiss_fft_factor audio_math_fft_factor
#define lhdc_kiss_fft_twiddles audio_math_fft_twiddles
#define lhdc_kiss_fft_next_fast_size audio_math_fft_next_fast_size
#define lhdc_kiss_fft_512 audio_math_fft_512
#define lhdc_kiss_fftr_copy_twiddles audio_math_fftr_copy_twiddles
#define lhdc_kiss_fft_power2 audio_math_fft_power2
#define lhdc_kiss_fft_sse_transform audio_math_fft_sse_transform
#define lhdc_kiss_fft_sse_supported audio_math_fft_sse_supported
void lhdc_kiss_fft_work(kiss_fft_cpx *, const kiss_fft_cpx *, size_t, int32_t, const int32_t *,
                        const LhdcKissFftPlan *);
int lhdc_kiss_fft_split(float *, float *, int32_t, int32_t, int, int);
int lhdc_kiss_fft_power2_split(float *, float *, int32_t, int32_t, int, int);
#endif
