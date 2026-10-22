#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <audio_math/numeric_bits.h>
#include <audio_math/constants.h>
#include "../src/common/vbap_angle.h"

float v4f_remainder_with_integer_tolerance(float numerator, float divisor);
float v5f_remainder_with_integer_tolerance(float numerator, float divisor);

/* Verify integer conversion boundaries, angle periodicity, and remainder behavior for exceptional floats. */
int main(void) {
    float (*functions[])(float, float) = {v4f_remainder_with_integer_tolerance,
                                          v5f_remainder_with_integer_tolerance};
    unsigned checked = 0;
    /* Check every VBAP integer-degree boundary and adjacent floating-point inputs. */
    for (int half_degree = 1; half_degree < 60; half_degree += 2) {
        float center = sinf((float)half_degree * 0.5f /
                            AUDIO_MATH_DEGREES_PER_RADIAN_F32);
        uint32_t bits = audio_math_float32_bits(center);
        for (int offset = -32; offset <= 32; ++offset)
            for (int sign = -1; sign <= 1; sign += 2) {
                float value = sign * audio_math_float32_from_bits(bits + offset);
                float angle = asinf(value) * AUDIO_MATH_DEGREES_PER_RADIAN_F32;
                int expected = (int)(angle + (angle >= 0.0f ? 0.5f : -0.5f));
                if (lhdc_ar_vbap_angle(value) != expected)
                    return 5;
                ++checked;
            }
    }
    for (unsigned version = 0; version < 2; ++version) {
        for (int index = -8192; index <= 8192; ++index) {
            float angle = (float)index * 0.125f;
            float actual = functions[version](angle, 360.0f);
            float expected = fmodf(angle, 360.0f);
            if (audio_math_float32_bits(actual) != audio_math_float32_bits(expected))
                return 1;
            ++checked;
        }
        if (!isnan(functions[version](1.0f, 0.0f)) ||
            !isnan(functions[version](INFINITY, 360.0f)) ||
            audio_math_float32_bits(functions[version](-0.0f, 360.0f)) != UINT32_C(0x80000000))
            return 2;
        ++checked;
        /* Noninteger divisors retain tolerance-based zeroing near integer quotients. */
        if (functions[version](0.875f, 0.4375f) != 0.0f)
            return 3;
        ++checked;
        const float divisors[] = {4000000000.0f,  8000000000.0f,  1.0e12f,
                                  -4000000000.0f, -8000000000.0f, -1.0e12f};
        for (unsigned i = 0; i < sizeof divisors / sizeof divisors[0]; ++i) {
            float numerator = nextafterf(2.0f * divisors[i], INFINITY);
            float actual = functions[version](numerator, divisors[i]);
            float expected = fmodf(numerator, divisors[i]);
            if (audio_math_float32_bits(actual) != audio_math_float32_bits(expected))
                return 4;
            ++checked;
        }
    }
    printf("angle contracts passed: %u cases\n", checked);
    return 0;
}
