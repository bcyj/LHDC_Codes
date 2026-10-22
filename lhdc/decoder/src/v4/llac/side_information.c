#include "../../../../common/include/types/llac_scalar_types.h"
#include <audio_math/bits.h>
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "../include/llac_semantic_records.h"
#include "../include/llac_side_info.h"
#include <math.h>
#include <stdbool.h>
static const int DAT_00010268[4] = {

    1, 1, 2, 2};
/* Read one LSB-first side-information bit from the frame tail and update byte position/mask. */
static unsigned read_side_bit(const LLAC_UINT8 *bytes, LLAC_INT *bp_side, LLAC_INT *mask_side) {
    unsigned bit = (bytes[*bp_side] & *mask_side) != 0;
    if (*mask_side != 0x80) {
        *mask_side <<= 1;
        return bit;
    }
    *mask_side = 1;
    --*bp_side;

    return bit;
}

/* Read side-information fields in encoder order, least-significant bit first. */
static unsigned read_side_bits(const LLAC_UINT8 *bytes, LLAC_INT *bp_side, LLAC_INT *mask_side,
                               unsigned count) {
    unsigned bits = 0;
    for (unsigned bit_index = 0; bit_index < count; ++bit_index)
        bits |= read_side_bit(bytes, bp_side, mask_side) << (bit_index & 0xff);
    return bits;
}

/* Read bandwidth, last spectral position, TNS, SNS, LTPF, and noise-factor fields backward from the frame tail.
 * Errors retain written fields and bit pointers; success does not proactively clear caller bfi.
 */
void llac_read_side_information(LLAC_UINT8 *bytes, LLAC_INT numbytes, LLAC_INT *mask_side,
                                LLAC_INT *bp_side, LLAC_INT N, LLAC_INT fs_idx,
                                LLAC_INT bw_cutoff_bits, LLAC_INT *bfi, LLAC_INT *gg_idx,
                                LLACSnsIndices *sns, LLAC_INT *fac_ns_idx, LLACTnsData *tns,
                                LLACLtpfIndices *ltpf, LLAC_INT *bw_cutoff_idx, LLAC_INT *lastnz,
                                LLAC_INT *lsbMode, LLAC_INT frame_dms) {
    const LLAC_INT *gainMSBbits = DAT_00010268;
    *bp_side = numbytes - 1;
    *mask_side = 1;

    *bw_cutoff_idx = bw_cutoff_bits < 1
                         ? fs_idx
                         : (LLAC_INT)read_side_bits(bytes, bp_side, mask_side, bw_cutoff_bits);
    if (bw_cutoff_bits > 0 && *bw_cutoff_idx > fs_idx) {
        *bfi = 1;
        return;
    }
    tns->filter_count = *bw_cutoff_idx < 3 || frame_dms == 25 ? 1 : 2;
    int lastnz_bits = (int)audio_math_ceil_log2_u32((uint32_t)(N / 2));

    *lastnz =
        ((LLAC_INT)read_side_bits(bytes, bp_side, mask_side, lastnz_bits < 1 ? 1 : lastnz_bits) +
         1) *
        2;
    if (*lastnz > N) {
        *bfi = 1;
        return;
    }
    *lsbMode = read_side_bit(bytes, bp_side, mask_side);
    *gg_idx = read_side_bits(bytes, bp_side, mask_side, 8);
    for (int i = 0; i < tns->filter_count; ++i)
        tns->order[i] = read_side_bit(bytes, bp_side, mask_side);
    unsigned ltpf_present = read_side_bit(bytes, bp_side, mask_side);
    sns->low_codebook = read_side_bits(bytes, bp_side, mask_side, 5);
    sns->high_codebook = read_side_bits(bytes, bp_side, mask_side, 5);
    unsigned submodeMSB = read_side_bit(bytes, bp_side, mask_side);
    sns->submode = submodeMSB * 2;
    sns->gain = read_side_bits(bytes, bp_side, mask_side, gainMSBbits[submodeMSB * 2]);
    sns->leading_sign = read_side_bit(bytes, bp_side, mask_side);

    unsigned combined_index = read_side_bits(bytes, bp_side, mask_side, submodeMSB ? 24 : 25);
    int submodeLSB;
    if (submodeMSB == 0) {
        if (combined_index > 0x01fe8f57) {
            *bfi = 1;
            return;
        }
        /* Compute the upper gain index by integer division within the valid index range. */
        int quotient = (int)(combined_index / LHDC_V4_LLAC_SNS_MPVQ_RADIX);
        sns->main_mpvq = combined_index - (unsigned)quotient * LHDC_V4_LLAC_SNS_MPVQ_RADIX;
        if (quotient < 2) {
            sns->secondary_mpvq = -2;
            submodeLSB = 1;
            sns->gain = quotient + sns->gain * 2;
        } else {
            sns->secondary_mpvq = quotient - 2;
            submodeLSB = 0;
        }
    } else {
        if (combined_index > 0x00fef1ff) {
            *bfi = 1;
            return;
        }
        if (combined_index < 0x00e74c00) {
            sns->main_mpvq = combined_index;
            sns->secondary_mpvq = -1;
            submodeLSB = 0;
        } else {
            sns->main_mpvq = (int)((combined_index - 0x00e74c00u) / 2u);
            sns->secondary_mpvq = -2;
            submodeLSB = 1;
            sns->gain = (combined_index & 1) + sns->gain * 2;
        }
    }
    sns->submode += submodeLSB;

    int ltpf_active = ltpf_present ? (int)read_side_bit(bytes, bp_side, mask_side) : 0;
    int ltpf_pitch = ltpf_present ? (int)read_side_bits(bytes, bp_side, mask_side, 9) : 0;
    ltpf->present = ltpf_present;
    ltpf->active = ltpf_active;
    ltpf->pitch = ltpf_pitch;
    *fac_ns_idx = read_side_bits(bytes, bp_side, mask_side, 3);
}
