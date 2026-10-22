/* V5 encoding FFT configuration; default retains the existing floating-point path. */
#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_LHDC_V5_FFT_OPTIONS_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_LHDC_V5_FFT_OPTIONS_H
#ifndef LHDC_V5_ENCODER_FIXED_FFT
#define LHDC_V5_ENCODER_FIXED_FFT 0
#endif
#if LHDC_V5_ENCODER_FIXED_FFT != 0 && LHDC_V5_ENCODER_FIXED_FFT != 1
#error LHDC_V5_ENCODER_FIXED_FFT must be 0 or 1
#endif
#if LHDC_V5_ENCODER_FIXED_FFT
#include "../../../../../common/include/dsp/fft_fixed32.h"
#define LHDC_V5_ENCODER_FIXED_FFT_MAX_LENGTH 480
/* Instances own integer twiddles and two scratch areas; no per-frame allocation or large stack arrays. */
typedef struct LhdcV5EncoderFixedFftWork {
    LhdcFftFixed32Complex twiddles[LHDC_V5_ENCODER_FIXED_FFT_MAX_LENGTH];
    LhdcFftFixed32Complex input[LHDC_V5_ENCODER_FIXED_FFT_MAX_LENGTH];
    LhdcFftFixed32Complex output[LHDC_V5_ENCODER_FIXED_FFT_MAX_LENGTH];
} LhdcV5EncoderFixedFftWork;
#define LHDC_V5_ENCODER_FIXED_FFT_EXTRA_BYTES sizeof(LhdcV5EncoderFixedFftWork)
#else
#define LHDC_V5_ENCODER_FIXED_FFT_EXTRA_BYTES 0u
#endif
#endif
