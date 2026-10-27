#include "../../../../common/include/dsp/float_compat.h"
#include <audio_math/fma_native.h>
#include "../include/lhdc_dec.h"
#include <limits.h>
#include <math.h>
#include <string.h>

/* Compute level offset, capping the result at 30 for indices >= 504. */
float lhdc_v5_dec_gain_offset(int offset_idx, int start, float offset_jump) {
    if (offset_idx >= 504) {
        float offset = (float)audio_math_subtract_wrap_i32(offset_idx, 504) +
                       audio_math_fma_f32((float)audio_math_subtract_wrap_i32(504, start),
                                          offset_jump, (float)start);
        return offset > 30.0f ? 30.0f : offset;
    }
    return audio_math_fma_f32((float)audio_math_subtract_wrap_i32(offset_idx, start), offset_jump,
                              (float)start);
}

/* Clear 512 levels and compute table origin/step from frame format; final multiply-subtract rounds once. */
void lhdc_v5_dec_init_gain_table(int size, int resolution, int hz, int ms, float *offset_tbl,
                                 int *start, float *offset_jump) {
    *start = 1;
    memset(offset_tbl, 0, 512 * sizeof(float));
    if ((UINT32_C(0xc28f5c29) * (uint32_t)ms + UINT32_C(0x051eb851) > UINT32_C(0x0a3d70a2)) ||
        ms > 100 || (resolution != 16 && resolution != 24)) {
        *offset_jump = (30.0f - (float)*start) / (float)audio_math_subtract_wrap_i32(503, *start);
        return;
    }
    int low = hz == 8000 || hz == 16000 || hz == 24000 || hz == 32000 || hz == 44100 || hz == 48000;
    uint32_t slope, maximum;
    if (resolution == 16) {
        if (!low) {
            *start = 12;
            *offset_jump = audio_math_float32_from_bits(UINT32_C(0x3d1628ab));
            return;
        }
        slope = hz == 48000 ? UINT32_C(0x3c8c799a) : UINT32_C(0x3c8a9f75);
        maximum = hz == 48000 ? UINT32_C(0x41d170a4) : UINT32_C(0x41d13333);
    } else {
        if (!low && hz != 96000 && hz != 192000) {
            *start = 7;
            *offset_jump = audio_math_float32_from_bits(UINT32_C(0x3d3def7c));
            return;
        }
        slope = hz == 96000    ? UINT32_C(0x3c0d0b7e)
                : hz == 192000 ? UINT32_C(0x3b11e390)
                               : UINT32_C(0x3c8b9ec5);
        maximum = hz == 96000 || hz == 192000 ? UINT32_C(0x41cd999a) : UINT32_C(0x41d1999a);
    }
    float offset_max = audio_math_fma_f32(-(float)audio_math_subtract_wrap_i32(size, 50),
                                          audio_math_float32_from_bits(slope),
                                          audio_math_float32_from_bits(maximum));
    int first =
        audio_math_truncate_sat_i32_f32(resolution == 16 ? offset_max / 2.5f : offset_max * 0.25f);
    *start = first;
    *offset_jump = (offset_max - (float)first) / (float)audio_math_subtract_wrap_i32(503, first);
}
