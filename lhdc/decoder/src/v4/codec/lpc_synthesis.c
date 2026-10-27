#include <audio_math/bits.h>
#include "../../../../../math/include/audio_math/integer.h"
#include "../include/lhdc_dec_core.h"
#include <string.h>

/* Save the last order outputs before prev_data, preserving sequential write order. */
static void lpc_history(int *data, unsigned count, unsigned order, int *prev) {
    for (unsigned j = 0; j < order; ++j)
        prev[(int)j - (int)order] = data[(int)count - (int)order + (int)j];
}

/* Reconstruct output from residuals, LPC coefficients, and history using 64-bit multiply-adds
 * and two-word shifts. Predictor orders through eight are supported; higher orders leave output unchanged. */
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void lpcV_Ressw(int *residual, unsigned count, int *coef, unsigned order, int shift, int *data,
                int *prev) {
    if (order > 8)
        return;
    if ((int)count <= 0)
        return;
    unsigned used = order ? order : 1, shift_bits = (unsigned)shift;
#if defined(_MSC_VER) && defined(_M_IX86)
    /* MSVC x86 emits substantially smaller and faster code for the compact fallthrough form. */
    for (unsigned i = 0; i < count; ++i) {
        uint64_t sum = 0;
        switch (used) {
        case 8:
            sum += (uint64_t)((int64_t)coef[7] * prev[(int)i - 8]); /* fallthrough */
        case 7:
            sum += (uint64_t)((int64_t)coef[6] * prev[(int)i - 7]); /* fallthrough */
        case 6:
            sum += (uint64_t)((int64_t)coef[5] * prev[(int)i - 6]); /* fallthrough */
        case 5:
            sum += (uint64_t)((int64_t)coef[4] * prev[(int)i - 5]); /* fallthrough */
        case 4:
            sum += (uint64_t)((int64_t)coef[3] * prev[(int)i - 4]); /* fallthrough */
        case 3:
            sum += (uint64_t)((int64_t)coef[2] * prev[(int)i - 3]); /* fallthrough */
        case 2:
            sum += (uint64_t)((int64_t)coef[1] * prev[(int)i - 2]); /* fallthrough */
        case 1:
            sum += (uint64_t)((int64_t)coef[0] * prev[(int)i - 1]);
            break;
        default:
            break;
        }
        uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
        int32_t value =
            audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
        data[i] = value;
        prev[i] = value;
    }
#else
    switch (used) {
    case 8: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2], c3 = coef[3];
        int64_t c4 = coef[4], c5 = coef[5], c6 = coef[6], c7 = coef[7];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c7 * prev[(int)i - 8]) +
                           (uint64_t)(c6 * prev[(int)i - 7]) +
                           (uint64_t)(c5 * prev[(int)i - 6]) +
                           (uint64_t)(c4 * prev[(int)i - 5]) +
                           (uint64_t)(c3 * prev[(int)i - 4]) +
                           (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 7: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2], c3 = coef[3];
        int64_t c4 = coef[4], c5 = coef[5], c6 = coef[6];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c6 * prev[(int)i - 7]) +
                           (uint64_t)(c5 * prev[(int)i - 6]) +
                           (uint64_t)(c4 * prev[(int)i - 5]) +
                           (uint64_t)(c3 * prev[(int)i - 4]) +
                           (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 6: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2], c3 = coef[3];
        int64_t c4 = coef[4], c5 = coef[5];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c5 * prev[(int)i - 6]) +
                           (uint64_t)(c4 * prev[(int)i - 5]) +
                           (uint64_t)(c3 * prev[(int)i - 4]) +
                           (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 5: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2], c3 = coef[3], c4 = coef[4];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c4 * prev[(int)i - 5]) +
                           (uint64_t)(c3 * prev[(int)i - 4]) +
                           (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 4: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2], c3 = coef[3];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c3 * prev[(int)i - 4]) +
                           (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 3: {
        int64_t c0 = coef[0], c1 = coef[1], c2 = coef[2];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c2 * prev[(int)i - 3]) +
                           (uint64_t)(c1 * prev[(int)i - 2]) +
                           (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 2: {
        int64_t c0 = coef[0], c1 = coef[1];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum =
                (uint64_t)(c1 * prev[(int)i - 2]) + (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    case 1: {
        int64_t c0 = coef[0];
        for (unsigned i = 0; i < count; ++i) {
            uint64_t sum = (uint64_t)(c0 * prev[(int)i - 1]);
            uint32_t shifted = audio_math_shift_pair_right_low8_u32(sum, shift_bits);
            int32_t value =
                audio_math_add_wrap_i32(audio_math_int32_from_bits(shifted), residual[i]);
            data[i] = value;
            prev[i] = value;
        }
        break;
    }
    default:
        break;
    }
#endif
    if (order)
        lpc_history(data, count, order, prev);
}
