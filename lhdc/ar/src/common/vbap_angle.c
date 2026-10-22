#include "../../../platform/include/lhdc_numeric_policy.h"
#include "vbap_angle.h"
#include <audio_math/float_properties.h>
#include <audio_math/float_convert.h>
#include <audio_math/math.h>
#include <audio_math/constants.h>
#include "../core/include/ar_math_constants.h"

#if LHDC_PLATFORM_CALIBRATED_FLOAT_MATH
#define LHDC_AR_VBAP_CALIBRATED_HOST 1
#endif

#ifdef LHDC_AR_VBAP_CALIBRATED_HOST
/* Each entry is the minimum input at which float asin, degree conversion, and half-away-from-zero rounding enter the next bin.
 * High-precision sin(n + 0.5) alone is insufficient: include both float roundings and the final addition.
 * The 15.5-degree boundary differs by one binary32 ULP between UCRT and Bionic. */
static const float angle_thresholds[30] = {
    0.00872653536f,
    0.0261769481f,
    0.0436193869f,
    0.0610485412f,
    0.0784590989f,
    0.0958457515f,
    0.113203213f,
    0.1305262f,
    0.147809401f,
    0.165047601f,
    0.182235509f,
    0.199367926f,
    0.21643962f,
    0.233445361f,
    0.250380009f,
    LHDC_PLATFORM_VBAP_15_5_THRESHOLD_F32,
    0.284015357f,
    0.300705791f,
    0.317304641f,
    0.333806843f,
    0.350207388f,
    0.366501212f,
    0.382683426f,
    0.398749083f,
    0.414693236f,
    0.430511087f,
    0.446197808f,
    0.4617486f,
    0.477158755f,
    0.492423564f,
};
#endif

/* Find an integer angle over the finite half-amplitude sine interval, preserving validated host rounding boundaries. */
int lhdc_ar_vbap_angle(float sine) {
#ifdef LHDC_AR_VBAP_CALIBRATED_HOST
    float magnitude = audio_math_abs_f32(sine);
    /* Over the half-amplitude sine interval, rounded 60*x equals the target bin or the next bin.
     * Estimate first and correct once against an exact boundary, avoiding repeated binary-search branches.
     * Finite |sine| <= 0.5 bounds the converted value to [0, 30], preventing out-of-range casts. */
    int angle = audio_math_truncate_i32_f32(magnitude * LHDC_AR_VBAP_LINEAR_DEGREE_SCALE_F32 + 0.5f);
    if (angle && magnitude < angle_thresholds[angle - 1])
        --angle;
    return sine < 0.0f ? -angle : angle;
#else
    /* Uncalibrated hosts retain the continuous-angle algorithm rather than using another runtime's boundaries. */
    float angle = audio_math_asinf(sine) * AUDIO_MATH_DEGREES_PER_RADIAN_F32;
    return (int)(angle + (angle >= 0.0f ? 0.5f : -0.5f));
#endif
}
