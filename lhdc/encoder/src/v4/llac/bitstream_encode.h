#ifndef LHDC_ENCODER_SRC_V4_LLAC_BITSTREAM_ENCODE_H
#define LHDC_ENCODER_SRC_V4_LLAC_BITSTREAM_ENCODE_H
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../../../../common/include/compat/v4/lhdc_v4_math.h"
/* Write one side-information bit from low to high, preserving other unoccupied bits. */
static inline void v4i_llac_write_side_bit(LLAC_UINT8 *bytes, LLAC_INT *bp_side,
                                           LLAC_INT *mask_side, LLAC_INT bit) {
    if (bit)
        bytes[*bp_side] |= (LLAC_UINT8)*mask_side;
    else
        bytes[*bp_side] &= (LLAC_UINT8) ~*mask_side;
    if (*mask_side == 128) {
        *mask_side = 1;
        --*bp_side;
    } else
        *mask_side <<= 1;
}

#if defined(__arm__) || defined(__aarch64__) || (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && (defined(_M_IX86) || defined(_M_X64)))
/* Append a two-to-four-bit low-bit/optional-sign group to a cleared bit queue. */
static inline void v4i_llac_append_deferred_group(LLAC_UINT8 *bits, LLAC_INT *count,
                                               unsigned group, unsigned group_bits) {
    unsigned byte = (unsigned)*count >> 3;
    unsigned offset = (unsigned)*count & 7u;
    bits[byte] |= (LLAC_UINT8)(group << offset);
    if (offset + group_bits > 8u)
        bits[byte + 1] = (LLAC_UINT8)(group >> (8u - offset));
    *count += (LLAC_INT)group_bits;
}
#endif

#endif
