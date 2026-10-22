#include <audio_math/float_properties.h>
#include <audio_math/constants.h>
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_rounding.h>
#include <audio_math/float_remainder.h>
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <string.h>

/* Reduce angles to a half-turn interval, preserving integer divisors and boundary semantics. */
static float reduce_degrees(float angle) {
    float angle_adjustment;
    float reduced_angle;

    angle_adjustment =
        LHDC_FN(remainder_with_integer_tolerance)(angle, LHDC_CONST(DEGREES_FULL_TURN));
    reduced_angle = -LHDC_CONST(DEGREES_FULL_TURN);
    if (angle_adjustment <= 0.0f) {
        reduced_angle = LHDC_CONST(DEGREES_FULL_TURN);
    }
    reduced_angle = angle_adjustment + reduced_angle;
    if (audio_math_abs_f32(angle_adjustment) <= LHDC_CONST(DEGREES_HALF_TURN)) {
        reduced_angle = angle_adjustment;
    }
    return reduced_angle;
}

/* Evaluate reduced-angle cosine by quadrant without introducing different radian rounding. */
static float cosine_reduced_degrees(float reduced_angle) {
    float absx = audio_math_abs_f32(reduced_angle);
    float angle_adjustment;
    if (absx <= LHDC_CONST(DEGREES_OCTANT)) {
        reduced_angle = audio_math_cosf(reduced_angle * LHDC_CONST(DEGREES_TO_RADIANS));
    } else if (absx <= LHDC_CONST(DEGREES_THREE_OCTANTS)) {
        if (reduced_angle <= 0.0f) {
            absx = LHDC_CONST(DEGREES_QUARTER_TURN);
            angle_adjustment = LHDC_CONST(DEGREES_TO_RADIANS);
        } else {
            absx = -LHDC_CONST(DEGREES_QUARTER_TURN);
            angle_adjustment = -LHDC_CONST(DEGREES_TO_RADIANS);
        }
        reduced_angle = audio_math_sinf((reduced_angle + absx) * angle_adjustment);
    } else {
        angle_adjustment = -LHDC_CONST(DEGREES_HALF_TURN);
        if (reduced_angle <= 0.0f) {
            angle_adjustment = LHDC_CONST(DEGREES_HALF_TURN);
        }
        reduced_angle =
            audio_math_cosf((reduced_angle + angle_adjustment) * LHDC_CONST(DEGREES_TO_RADIANS));
        reduced_angle = -reduced_angle;
    }
    return reduced_angle;
}

/* Evaluate reduced-angle sine by quadrant, preserving propagation of zero and nonfinite inputs. */
static float sine_reduced_degrees(float reduced_angle) {
    float absx = audio_math_abs_f32(reduced_angle);
    float angle_scale;
    if (absx <= LHDC_CONST(DEGREES_OCTANT)) {
        angle_scale = LHDC_CONST(DEGREES_TO_RADIANS);
    } else {
        if (absx <= LHDC_CONST(DEGREES_THREE_OCTANTS)) {
            if (reduced_angle <= 0.0f) {
                reduced_angle = audio_math_cosf((reduced_angle + LHDC_CONST(DEGREES_QUARTER_TURN)) *
                                          LHDC_CONST(DEGREES_TO_RADIANS));
                reduced_angle = -reduced_angle;
            } else {
                reduced_angle = audio_math_cosf((reduced_angle + -LHDC_CONST(DEGREES_QUARTER_TURN)) *
                                          LHDC_CONST(DEGREES_TO_RADIANS));
            }
            return reduced_angle;
        }
        absx = -LHDC_CONST(DEGREES_HALF_TURN);
        if (reduced_angle <= 0.0f) {
            absx = LHDC_CONST(DEGREES_HALF_TURN);
        }
        angle_scale = -LHDC_CONST(DEGREES_TO_RADIANS);
        reduced_angle = reduced_angle + absx;
    }
    reduced_angle = audio_math_sinf(reduced_angle * angle_scale);
    return reduced_angle;
}

/* Reduce degree angles once and evaluate sine/cosine together when their radian arguments match.
 * Middle and outer quadrants use different multiplier signs and retain their respective formulas.
 * Both output pointers must be valid and may alias; write sine first, then cosine. */
void LHDC_FN(sincos_degrees)(float angle, float *sine, float *cosine) {
    float reduced_angle = reduce_degrees(angle);
    float magnitude = audio_math_abs_f32(reduced_angle);
    float sine_value;
    float cosine_value;
    if (magnitude <= LHDC_CONST(DEGREES_OCTANT)) {
        audio_math_sincosf(reduced_angle * LHDC_CONST(DEGREES_TO_RADIANS), &sine_value, &cosine_value);
    } else if (magnitude <= LHDC_CONST(DEGREES_THREE_OCTANTS) && reduced_angle <= 0.0f) {
        audio_math_sincosf((reduced_angle + LHDC_CONST(DEGREES_QUARTER_TURN)) *
                         LHDC_CONST(DEGREES_TO_RADIANS),
                     &cosine_value, &sine_value);
        sine_value = -sine_value;
    } else {
        cosine_value = cosine_reduced_degrees(reduced_angle);
        sine_value = sine_reduced_degrees(reduced_angle);
    }
    *sine = sine_value;
    *cosine = cosine_value;
}

/* Compute floating-point remainder and handle rounding near integer quotients. */
float LHDC_FN(remainder_with_integer_tolerance)(float u0, float u1) {
    /* Test the floating divisor for integrality directly, avoiding pointer-width integers and false zeroing for large values.
     * Nonfinite inputs retain the existing tolerance check and NaN/negative-zero propagation policy. */
    const int integral_divisor = audio_math_is_finite_f32(u1) && audio_math_trunc_f32(u1) == u1;
    if (u1 != 0.0f && !integral_divisor) {
        float quotient = audio_math_abs_f32(u0 / u1);
        float distance = audio_math_abs_f32(quotient - audio_math_floor_f32(quotient + 0.5f));

        if (!(distance > quotient * AUDIO_MATH_EPSILON_F32))
            return u0 * 0.0f;
    }
    return audio_math_fmod_f32(u0, u1);
}
