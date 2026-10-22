#ifndef LHDC_X86_SMOOTHING_H
#define LHDC_X86_SMOOTHING_H

#include <string.h>

#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define LHDC_X86_SMOOTH_121 1

/* Smooth in place with three-point [1, 2, 1]/4 weights and repeated endpoints.
 * scratch holds at least ten floats and must not overlap values.
 * Small snapshots let MSVC process independent elements with SSE2, preserving double
 * multiply-add order and final float rounding. No FMA/AVX or cross-call state. */
static inline void lhdc_x86_smooth_121_f32(float *values, int length, float *scratch) {
    if (length <= 0)
        return;
    float previous = values[0];
    for (int offset = 0; offset < length;) {
        int count = length - offset;
        if (count > 8)
            count = 8;
        scratch[0] = previous;
        memcpy(scratch + 1, values + offset, (size_t)count * sizeof *scratch);
        scratch[count + 1] = offset + count < length ? values[offset + count] : scratch[count];
        previous = scratch[count];
        for (int i = 0; i < count; ++i)
            values[offset + i] = (float)((double)scratch[i + 1] * 0.5 + (double)scratch[i] * 0.25 +
                                         (double)scratch[i + 2] * 0.25);
        offset += count;
    }
}
#endif

#endif
