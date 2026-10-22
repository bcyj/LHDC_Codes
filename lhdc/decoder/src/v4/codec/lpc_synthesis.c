#include <audio_math/bits.h>
#include "../../../../../math/include/audio_math/integer.h"
#include "../include/lhdc_dec_core.h"
#include <string.h>

/* Save the last order outputs before prev_data, preserving sequential write order. */
static void lpc_history(int *data, unsigned count, unsigned order, int *prev) {
    for (unsigned j = 0; j < order; ++j)
        prev[(int)j - (int)order] = data[(int)count - (int)order + (int)j];
}

/* Reconstruct residual output from LPC coefficients and history using 64-bit multiply-add and double-word shifts.
 * Supports prediction through order eight; higher orders leave output unchanged. */
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void lpcV_Ressw(int *residual, unsigned count, int *coef, unsigned order, int shift, int *data,
                int *prev) {
    if (order > 8)
        return;
    unsigned used = order ? order : 1, shift_bits = (unsigned)shift;
    for (unsigned i = 0; i < count && (int)count > 0; ++i) {
        uint64_t sum = 0;
        for (unsigned j = 0; j < used; ++j)
            sum += (uint64_t)((int64_t)coef[j] * prev[(int)i - 1 - (int)j]);
        uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
        int32_t value = audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
        data[i] = value;
        prev[i] = value;
    }
    if (order)
        lpc_history(data, count, order, prev);
}
