#ifndef LHDC_ENCODER_V4_RESAMPLE_STATE_H
#define LHDC_ENCODER_V4_RESAMPLE_STATE_H
#include <stdint.h>

/* Each encoder owns filter history, FIR coefficients, and the current PCM saturation range. */
typedef struct LhdcV4ResampleState {
    float history[2][8][3];
    float sinc_48k[17];
    float sinc_24k[17];
    int32_t pcm_max, pcm_min;
} LhdcV4ResampleState;
#endif
