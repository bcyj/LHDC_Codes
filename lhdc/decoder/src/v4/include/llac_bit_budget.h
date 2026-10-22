#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_BIT_BUDGET_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_BIT_BUDGET_H
#include "llac_semantic_records.h"
#include <audio_math/bits.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif

/* Return the integer part of a positive 24-bit arithmetic range's single-precision log2.
 * Preconditions: 1 <= range < 2^24; normalized valid arithmetic state also has range >= 2^16.
 * This is not floor of exact real log2: just below 2^21, 2^22, 2^23, and 2^24,
 * respectively 1, 2, 5, and 11 integers first round their binary32 logarithm up to an integer exponent.
 * Preserve these 19 boundaries to avoid changing subsequent residual-bit budgets. */
static inline unsigned llac_spectral_range_bits(uint32_t range) {
    unsigned exponent = 31u - audio_math_count_leading_zeros32(range);
    switch (exponent) {
    case 20:
        return exponent + (range >= (UINT32_C(1) << 21) - 1u);
    case 21:
        return exponent + (range >= (UINT32_C(1) << 22) - 2u);
    case 22:
        return exponent + (range >= (UINT32_C(1) << 23) - 5u);
    case 23:
        return exponent + (range >= (UINT32_C(1) << 24) - 11u);
    default:
        return exponent;
    }
}

/* Compute bits remaining between forward arithmetic coding and backward side information.
 * side->mask is a power of two in 1..128; position calculation rounds to float before integer conversion. */
static inline int llac_spectral_residual_budget(const LLACArithmeticReader *reader,
                                                const LLACSideReader *side, int frame_bits) {
    float mask_log2 = (float)(31u - audio_math_count_leading_zeros32(side->mask));
    float range_log2 = (float)llac_spectral_range_bits(reader->range);
    float side_position = (float)(side->byte_position * 8 + 16);
    float arithmetic_position = (float)(reader->byte_position * 8 + 9);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* These Win32 kernel-path subtractions do not write back to float; all operands are
     * binary32 integers and binary64 represents their differences exactly. Convert to integer last;
     * preceding integer-to-float conversions round separately, so frame_bits or cursor rounding cannot be removed. */
    double side_bits = (double)side_position - (double)mask_log2;
    return frame_bits - (audio_math_truncate_i32_f64((double)(float)frame_bits - side_bits) +
                         audio_math_truncate_i32_f64((double)arithmetic_position - (double)range_log2));
#else
    return frame_bits - ((int)((float)frame_bits - (side_position - mask_log2)) +
                         (int)(arithmetic_position - range_log2));
#endif
}
#endif
