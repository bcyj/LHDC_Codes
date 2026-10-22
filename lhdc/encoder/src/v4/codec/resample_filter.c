#include "../../../../platform/windows/include/lhdc_windows_sine.h"
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include <audio_math/float_convert.h>
#include <audio_math/float_sine.h>
#include <audio_math/numeric_bits.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <string.h>

/* Clear both channels' eight-stage encoding low-pass history. */
void v4f_ResetEncodeLPF(LhdcV4ResampleState *state) {
    memset(state->history, 0, sizeof state->history);
}

/* Apply seven cascaded IIR stages and final output shaping; saturate to 24 bits before restoring input depth. */
void v4f_EncodeLPF(LhdcV4ResampleState *state, int32_t nCh, float *X, int32_t *pcm, int32_t N,
                   int32_t bits_per_sample) {
    static const float feedback[7][2] = {
        {.58305f, -.80582f}, {1.15013f, -.35153f}, {.87121f, -.56523f}, {.47391f, -.96556f},
        {.7082f, -.69577f},  {1.03872f, -.43612f}, {.50504f, -.8931f}};
    static const float feedforward[7][2] = {{.11254f, .025052529f}, {.2239f, .43113288f},
                                            {.70629f, .6367769f},   {.34747f, .0027519623f},
                                            {.12888f, .06258026f},  {.62836f, .9142323f},
                                            {.30596f, .022693053f}};
    uint32_t shift = (24u - (uint32_t)bits_per_sample) & 31;
    for (int32_t i = 0; i < N; ++i) {
        float value = X[i] * (float)(int32_t)(1u << shift);
        for (int32_t j = 0; j < 7; ++j) {
            float *pfU = state->history[nCh][j];
            float old1 = pfU[1], old2 = pfU[2];
            float state = (value + old1 * feedback[j][0]) + old2 * feedback[j][1];
            pfU[0] = state;
            pfU[2] = old1;
            pfU[1] = state;
            value =
                old2 * feedforward[j][0] + (old1 * feedforward[j][1] + state * feedforward[j][0]);
        }
        int32_t output = value >= AUDIO_MATH_INT24_MAX_F32  ? 8388607
                         : value <= -AUDIO_MATH_POW2_23_F32 ? -8388608
                                                           : audio_math_truncate_i32_f32(value);
        pcm[i] = output >> shift;
    }
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <string.h>

/* Output integer PCM delayed by seven samples and update sixteen history entries sequentially.
 * Input/history must be finite representable PCM; conversion does not repair exceptional filter state. */
void v4f_win_sinc_delay(float *X, int32_t *pcm, int32_t N) {
    for (int32_t i = 0; i < N; ++i)
        pcm[i] = audio_math_truncate_i32_f32(X[i - 7]);
    for (int32_t j = -16; j < 0; ++j)
        X[j] = X[N + j];
}

/* Copy windowed sinc for two standard cutoffs; generate other inputs from the windowed-sinc formula.
 * Kernel builds share verified fixed-window cosines; cutoff still determines sinc dynamically.
 * Preserve platform long double precision and float accumulation/normalization order. */
void v4f_init_window(float *H, float FC) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
    /* Double results for 17 fixed angles; retain asymmetric last bits and near-zero cosines rather than merging theoretical values. */
    static const double hamming_cosines[17] = {
        1.0,
        0.9238795325112867,
        0.7071067811865476,
        0.38268343236508984,
        6.123233995736766e-17,
        -0.3826834323650897,
        -0.7071067811865475,
        -0.9238795325112867,
        -1.0,
        -0.9238795325112868,
        -0.7071067811865477,
        -0.38268343236509034,
        -1.8369701987210297e-16,
        0.38268343236509,
        0.7071067811865474,
        0.9238795325112865,
        1.0,
    };
#endif
    static const float sinc_48k_coefficients[17] = {
        -0.00153581344f, 0.00223026262f,  -0.00420002127f, 0.00717110606f, -0.0107029947f,
        0.0142577933f,   -0.0172862522f,  0.0193164758f,   0.981498837f,   0.0193164758f,
        -0.0172862522f,  0.0142577933f,   -0.0107029947f,  0.00717110606f, -0.00420002127f,
        0.00223026262f,  -0.00153581344f,
    };
    static const float sinc_24k_coefficients[17] = {
        -0.000793569023f, -0.0051167924f,   0.00213991292f,  0.0229423195f, -0.00539916521f,
        -0.075824216f,    0.00866845902f,   0.307774782f,    0.491216511f,  0.307774782f,
        0.00866845902f,   -0.075824216f,    -0.00539916521f, 0.0229423195f, 0.00213991292f,
        -0.0051167924f,   -0.000793569023f,
    };
    const uint32_t cutoff_bits = audio_math_float32_bits(FC);
    if (cutoff_bits == audio_math_float32_bits(LHDC_V4_RESAMPLE_CUTOFF_48K)) {
        memcpy(H, sinc_48k_coefficients, sizeof sinc_48k_coefficients);
        return;
    }
    if (cutoff_bits == audio_math_float32_bits(LHDC_V4_RESAMPLE_CUTOFF_24K)) {
        memcpy(H, sinc_24k_coefficients, sizeof sinc_24k_coefficients);
        return;
    }
    const long double two_pi = AUDIO_MATH_TWO_PI_LONG_DOUBLE;
    long double cutoff_angle = (long double)FC * two_pi;
    for (int32_t i = -8; i <= 8; ++i) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
        float sinc =
            i ? (float)(lhdc_windows_sine_v4_phase_f64((double)(cutoff_angle * (long double)i)) /
                        (double)i)
#else
        float sinc = i ? (float)(audio_math_sin((double)(cutoff_angle * (long double)i)) / (double)i)
#endif
              : (float)cutoff_angle;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
        double window = hamming_cosines[i + 8] * -.46 + .54;
#else
        double window = audio_math_cos((double)(((long double)(i + 8) * two_pi) * .0625L)) * -.46 + .54;
#endif
        H[i + 8] = (float)(window * (double)sinc);
    }
    float sum = H[0] + 0.0f;
    for (int32_t i = 1; i < 17; ++i)
        sum = sum + H[i];
    for (int32_t i = 0; i < 17; ++i)
        H[i] = H[i] / sum;
}

/* Update this instance's PCM saturation range and two windowed-sinc coefficient sets. */
void v4f_win_sinc_init(LhdcV4ResampleState *state, int32_t bits_per_sample) {
    state->pcm_min = (int32_t)(UINT32_MAX << (((uint32_t)bits_per_sample - 1) & 31));
    state->pcm_max = ~state->pcm_min;
    v4f_init_window(state->sinc_48k, LHDC_V4_RESAMPLE_CUTOFF_48K);
    v4f_init_window(state->sinc_24k, LHDC_V4_RESAMPLE_CUTOFF_24K);
}
