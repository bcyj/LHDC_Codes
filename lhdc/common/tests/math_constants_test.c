#include <string.h>
#include <stdio.h>
#include <audio_math/constants.h>
#include "../include/compat/v4/lhdc_v4_constants.h"
#include "../include/compat/v5/lhdc_v5_constants.h"
#include "../../ar/src/core/include/ar_math_constants.h"
#include "../../platform/windows/include/lhdc_windows_float_math_constants.h"
/* Verify mathematical constant precision, bit patterns, and literal types. */
int main(void) {
    {
        float before = .00024414063f, after = AUDIO_MATH_POW2_NEG_12_F32;
        _Static_assert(sizeof(.00024414063f) == sizeof(AUDIO_MATH_POW2_NEG_12_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 1;
    }
    {
        float before = .0009765625f, after = AUDIO_MATH_POW2_NEG_10_F32;
        _Static_assert(sizeof(.0009765625f) == sizeof(AUDIO_MATH_POW2_NEG_10_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 2;
    }
    {
        float before = .001953125f, after = AUDIO_MATH_POW2_NEG_9_F32;
        _Static_assert(sizeof(.001953125f) == sizeof(AUDIO_MATH_POW2_NEG_9_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 3;
    }
    {
        float before = .00390625f, after = AUDIO_MATH_POW2_NEG_8_F32;
        _Static_assert(sizeof(.00390625f) == sizeof(AUDIO_MATH_POW2_NEG_8_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 4;
    }
    {
        float before = .0078125f, after = AUDIO_MATH_POW2_NEG_7_F32;
        _Static_assert(sizeof(.0078125f) == sizeof(AUDIO_MATH_POW2_NEG_7_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 5;
    }
    {
        double before = 0.00048828125, after = AUDIO_MATH_POW2_NEG_11_F64;
        _Static_assert(sizeof(0.00048828125) == sizeof(AUDIO_MATH_POW2_NEG_11_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 6;
    }
    {
        float before = 0.0022260882f, after = LHDC_V5_GAIN_SLOPE_24BIT_192K;
        _Static_assert(sizeof(0.0022260882f) == sizeof(LHDC_V5_GAIN_SLOPE_24BIT_192K),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 7;
    }
    {
        float before = 0.008608697f, after = LHDC_V5_GAIN_SLOPE_24BIT_96K;
        _Static_assert(sizeof(0.008608697f) == sizeof(LHDC_V5_GAIN_SLOPE_24BIT_96K),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 8;
    }
    {
        float before = 0.016921738f, after = LHDC_V5_GAIN_SLOPE_16BIT_44K;
        _Static_assert(sizeof(0.016921738f) == sizeof(LHDC_V5_GAIN_SLOPE_16BIT_44K),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 9;
    }
    {
        float before = 0.01704348f, after = LHDC_V5_GAIN_SLOPE_24BIT_48K;
        _Static_assert(sizeof(0.01704348f) == sizeof(LHDC_V5_GAIN_SLOPE_24BIT_48K),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 10;
    }
    {
        float before = 0.017147828f, after = LHDC_V5_GAIN_SLOPE_16BIT_48K;
        _Static_assert(sizeof(0.017147828f) == sizeof(LHDC_V5_GAIN_SLOPE_16BIT_48K),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 11;
    }
    {
        float before = 0.35355338f, after = AUDIO_MATH_INV_SQRT_EIGHT_F32;
        _Static_assert(sizeof(0.35355338f) == sizeof(AUDIO_MATH_INV_SQRT_EIGHT_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 12;
    }
    {
        double before = 0x1.8p+47, after = AUDIO_MATH_WIN_INTEGER_ROUND_BIAS_F64;
        _Static_assert(sizeof(0x1.8p+47) == sizeof(AUDIO_MATH_WIN_INTEGER_ROUND_BIAS_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 13;
    }
    {
        double before = 0x1.f5c28f5c28f5bp+0, after = LHDC_V4_LLAC_QUANT_BIT_BUDGET_SCALE;
        _Static_assert(sizeof(0x1.f5c28f5c28f5bp+0) == sizeof(LHDC_V4_LLAC_QUANT_BIT_BUDGET_SCALE),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 14;
    }
    {
        double before = 0x1.fffffep127, after = (double)AUDIO_MATH_MAX_FINITE_F32;
        _Static_assert(sizeof(0x1.fffffep127) == sizeof((double)AUDIO_MATH_MAX_FINITE_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 15;
    }
    {
        double before = 0x1.fffffffc00000p+30, after = AUDIO_MATH_INT32_MAX_F64;
        _Static_assert(sizeof(0x1.fffffffc00000p+30) == sizeof(AUDIO_MATH_INT32_MAX_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 16;
    }
    {
        float before = 0x1p+23f, after = AUDIO_MATH_POW2_23_F32;
        _Static_assert(sizeof(0x1p+23f) == sizeof(AUDIO_MATH_POW2_23_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 17;
    }
    {
        double before = 0x1p-11, after = AUDIO_MATH_POW2_NEG_11_F64;
        _Static_assert(sizeof(0x1p-11) == sizeof(AUDIO_MATH_POW2_NEG_11_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 18;
    }
    {
        double before = 0x1p-126, after = (double)AUDIO_MATH_MIN_NORMAL_F32;
        _Static_assert(sizeof(0x1p-126) == sizeof((double)AUDIO_MATH_MIN_NORMAL_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 19;
    }
    {
        float before = 0x1p-16f, after = AUDIO_MATH_POW2_NEG_16_F32;
        _Static_assert(sizeof(0x1p-16f) == sizeof(AUDIO_MATH_POW2_NEG_16_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 20;
    }
    {
        float before = 0x1p-20f, after = AUDIO_MATH_POW2_NEG_20_F32;
        _Static_assert(sizeof(0x1p-20f) == sizeof(AUDIO_MATH_POW2_NEG_20_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 21;
    }
    {
        float before = 0x1p-23f, after = AUDIO_MATH_EPSILON_F32;
        _Static_assert(sizeof(0x1p-23f) == sizeof(AUDIO_MATH_EPSILON_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 22;
    }
    {
        float before = 0x1p-24f, after = AUDIO_MATH_POW2_NEG_24_F32;
        _Static_assert(sizeof(0x1p-24f) == sizeof(AUDIO_MATH_POW2_NEG_24_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 23;
    }
    {
        float before = 0x1p-30f, after = AUDIO_MATH_POW2_NEG_30_F32;
        _Static_assert(sizeof(0x1p-30f) == sizeof(AUDIO_MATH_POW2_NEG_30_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 24;
    }
    {
        double before = 0x1p-31, after = AUDIO_MATH_POW2_NEG_31_F64;
        _Static_assert(sizeof(0x1p-31) == sizeof(AUDIO_MATH_POW2_NEG_31_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 25;
    }
    {
        float before = 0x1p-32f, after = AUDIO_MATH_POW2_NEG_32_F32;
        _Static_assert(sizeof(0x1p-32f) == sizeof(AUDIO_MATH_POW2_NEG_32_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 26;
    }
    {
        float before = 0x1p-52f, after = (float)AUDIO_MATH_EPSILON_F64;
        _Static_assert(sizeof(0x1p-52f) == sizeof((float)AUDIO_MATH_EPSILON_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 27;
    }
    {
        float before = 0x1p-8f, after = AUDIO_MATH_POW2_NEG_8_F32;
        _Static_assert(sizeof(0x1p-8f) == sizeof(AUDIO_MATH_POW2_NEG_8_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 28;
    }
    {
        float before = 0x1p100f, after = LHDC_V4_LLAC_SNS_INITIAL_DISTANCE;
        _Static_assert(sizeof(0x1p100f) == sizeof(LHDC_V4_LLAC_SNS_INITIAL_DISTANCE),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 29;
    }
    {
        float before = 0x1p20f, after = AUDIO_MATH_POW2_20_F32;
        _Static_assert(sizeof(0x1p20f) == sizeof(AUDIO_MATH_POW2_20_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 30;
    }
    {
        float before = 0x1p23f, after = AUDIO_MATH_POW2_23_F32;
        _Static_assert(sizeof(0x1p23f) == sizeof(AUDIO_MATH_POW2_23_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 31;
    }
    {
        float before = 0x1p31f, after = AUDIO_MATH_POW2_31_F32;
        _Static_assert(sizeof(0x1p31f) == sizeof(AUDIO_MATH_POW2_31_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 32;
    }
    {
        float before = 0x1p32f, after = AUDIO_MATH_POW2_32_F32;
        _Static_assert(sizeof(0x1p32f) == sizeof(AUDIO_MATH_POW2_32_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 33;
    }
    {
        float before = 0x1p53f, after = AUDIO_MATH_POW2_53_F32;
        _Static_assert(sizeof(0x1p53f) == sizeof(AUDIO_MATH_POW2_53_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 34;
    }
    {
        float before = 1.0e-5f, after = LHDC_V4_LLAC_ATTACK_ENERGY_FLOOR;
        _Static_assert(sizeof(1.0e-5f) == sizeof(LHDC_V4_LLAC_ATTACK_ENERGY_FLOOR),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 35;
    }
    {
        float before = 1.1614486f, after = LHDC_V4_RESAMPLE_OUTPUT_GAIN;
        _Static_assert(sizeof(1.1614486f) == sizeof(LHDC_V4_RESAMPLE_OUTPUT_GAIN),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 36;
    }
    {
        float before = 1.1920928955078125e-7f, after = AUDIO_MATH_EPSILON_F32;
        _Static_assert(sizeof(1.1920928955078125e-7f) == sizeof(AUDIO_MATH_EPSILON_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 37;
    }
    {
        float before = 1.1920929e-07f, after = AUDIO_MATH_EPSILON_F32;
        _Static_assert(sizeof(1.1920929e-07f) == sizeof(AUDIO_MATH_EPSILON_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 38;
    }
    {
        float before = 1e-05f, after = LHDC_AR_GAIN_ZERO_THRESHOLD_F32;
        _Static_assert(sizeof(1e-05f) == sizeof(LHDC_AR_GAIN_ZERO_THRESHOLD_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 39;
    }
    {
        float before = 1e-5f, after = LHDC_AR_GAIN_ZERO_THRESHOLD_F32;
        _Static_assert(sizeof(1e-5f) == sizeof(LHDC_AR_GAIN_ZERO_THRESHOLD_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 40;
    }
    {
        float before = 1e-5f, after = LHDC_V4_LLAC_QUANT_ENERGY_FLOOR;
        _Static_assert(sizeof(1e-5f) == sizeof(LHDC_V4_LLAC_QUANT_ENERGY_FLOOR),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 41;
    }
    {
        double before = 1e-6, after = LHDC_V4_STATISTICS_COUNT_BIAS;
        _Static_assert(sizeof(1e-6) == sizeof(LHDC_V4_STATISTICS_COUNT_BIAS),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 42;
    }
    {
        double before = 2147483647.0, after = AUDIO_MATH_INT32_MAX_F64;
        _Static_assert(sizeof(2147483647.0) == sizeof(AUDIO_MATH_INT32_MAX_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 43;
    }
    {
        double before = 2147483648.0, after = AUDIO_MATH_POW2_31_F64;
        _Static_assert(sizeof(2147483648.0) == sizeof(AUDIO_MATH_POW2_31_F64),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 44;
    }
    {
        float before = 32767.625f, after = LHDC_V4_LLAC_QUANT_LIMIT_NORMAL;
        _Static_assert(sizeof(32767.625f) == sizeof(LHDC_V4_LLAC_QUANT_LIMIT_NORMAL),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 45;
    }
    {
        float before = 4194303.0f, after = LHDC_V4_UPSAMPLE_MAX_F32;
        _Static_assert(sizeof(4194303.0f) == sizeof(LHDC_V4_UPSAMPLE_MAX_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 46;
    }
    {
        float before = 4194304.0f, after = LHDC_V4_UPSAMPLE_MAGNITUDE_F32;
        _Static_assert(sizeof(4194304.0f) == sizeof(LHDC_V4_UPSAMPLE_MAGNITUDE_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 47;
    }
    {
        float before = 4294967296.0f, after = AUDIO_MATH_POW2_32_F32;
        _Static_assert(sizeof(4294967296.0f) == sizeof(AUDIO_MATH_POW2_32_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 48;
    }
    {
        float before = 8388606.0f, after = LHDC_V4_LLAC_QUANT_LIMIT_HR;
        _Static_assert(sizeof(8388606.0f) == sizeof(LHDC_V4_LLAC_QUANT_LIMIT_HR),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 49;
    }
    {
        float before = 8388607.0f, after = AUDIO_MATH_INT24_MAX_F32;
        _Static_assert(sizeof(8388607.0f) == sizeof(AUDIO_MATH_INT24_MAX_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 50;
    }
    {
        float before = 8388608.0f, after = AUDIO_MATH_POW2_23_F32;
        _Static_assert(sizeof(8388608.0f) == sizeof(AUDIO_MATH_POW2_23_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 51;
    }
    {
        float before = 0x1.99999ap-4f, after = AUDIO_MATH_ONE_TENTH_F32;
        _Static_assert(sizeof(0x1.99999ap-4f) == sizeof(AUDIO_MATH_ONE_TENTH_F32),
                       "constant expression type");
        if (memcmp(&before, &after, sizeof before))
            return 52;
    }
    puts("PASS 52 exact scalar constant values and expression types");
    return 0;
}
