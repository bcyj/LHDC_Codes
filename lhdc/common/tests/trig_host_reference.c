#include "trig_host_reference.h"
#include <audio_math/numeric_bits.h>
#include <audio_math/constants.h>
#include <audio_math/float_rounding.h>
#include "../include/dsp/float_compat.h"
#include <math.h>

/* Independently observe the double-precision bits returned by host sine. */
static uint64_t reference_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float64_bits(sin(x));
}

/* Independently observe the double-precision bits returned by host cosine. */
static uint64_t reference_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float64_bits(cos(x));
}

/* Independently observe single-precision sine-coefficient rounding. */
static uint64_t reference_sin_f32(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float32_bits((float)sin(x));
}

/* Independently observe single-precision cosine-coefficient rounding. */
static uint64_t reference_cos_f32(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    return audio_math_float32_bits((float)cos(x));
}

/* Independently observe a window retaining double-precision intermediate multiplication. */
static uint64_t reference_window(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double inner = sin(x);
    double phase = (inner * (double)AUDIO_MATH_HALF_PI_F32) * inner;
    return audio_math_float32_bits((float)sin(phase));
}

/* Independently verify saturated Q31 sine-coefficient conversion. */
static uint64_t reference_q31_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    float value = (float)sin(x);
    return (uint32_t)audio_math_truncate_sat_i32_f64((double)(value * AUDIO_MATH_POW2_31_F32));
}

/* Independently verify saturated Q31 cosine-coefficient conversion. */
static uint64_t reference_q31_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    float value = (float)cos(x);
    return (uint32_t)audio_math_truncate_sat_i32_f64((double)(value * AUDIO_MATH_POW2_31_F32));
}

/* Independently verify FFT sine-coefficient scaling and rounding. */
static uint64_t reference_fft_sin(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double value = sin(x) * AUDIO_MATH_INT32_MAX_F64 + 0.5;
    return (uint32_t)audio_math_truncate_sat_i32_f64(audio_math_floor_f64(value));
}

/* Independently verify FFT cosine-coefficient scaling and rounding. */
static uint64_t reference_fft_cos(uint64_t a, uint64_t b) {
    double x = audio_math_float64_from_bits(a);
    (void)b;
    double value = cos(x) * AUDIO_MATH_INT32_MAX_F64 + 0.5;
    return (uint32_t)audio_math_truncate_sat_i32_f64(audio_math_floor_f64(value));
}

LhdcTrigHostFunction const lhdc_test_trig_host_reference[LHDC_TEST_TRIG_HOST_OPERATIONS] = {
    reference_sin,     reference_cos,     reference_sin_f32, reference_cos_f32, reference_window,
    reference_q31_sin, reference_q31_cos, reference_fft_sin, reference_fft_cos};
