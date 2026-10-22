#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_rounding.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include <audio_math/bits.h>
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/codec/v4_pcm.h"

/* Quantize LLAC spectral residuals and write remaining precision bits. */
void v4f_llac_encode_residual_bits(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gain, LLAC_INT L_spec,
                                   LLAC_INT targetBits, LLAC_INT nBits, LLAC_UINT8 *resBits,
                                   LLAC_INT *numResBits, LLAC_INT hrmode) {
    v4_llac_encode_residual_bits(x, xq, gain, L_spec, targetBits, nBits, resBits, numResBits,
                                 hrmode);
}
#include "../include/llac/llac_const.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "../include/llac/llac_code_types.h"

/* Emit a stable high byte, deferring 0xff runs and possible carries. */
static void v4i_llac_arithmetic_encoder_shift_range_byte(RangeEncoder *st, uint32_t value) {
    if (st->carry == 1 || value < 0xff0000u) {
        if (st->cache >= 0)
            st->ptr[st->bp++] = (LLAC_UINT8)(st->cache + st->carry);
        if (st->carry_count > 0) {
            memset(st->ptr + st->bp, st->carry - 1, (size_t)st->carry_count);
            st->bp += st->carry_count;
            st->carry_count = 0;
        }
        st->carry = 0;
        st->cache = (LLAC_INT)(value >> 16);
    } else
        ++st->carry_count;
}

/* Narrow a 24-bit arithmetic interval using Q10 cumulative probabilities, then normalize bytewise. */
static void v4i_llac_arithmetic_encoder_encode_symbol(RangeEncoder *st, const LLAC_INT *cumfreq,
                                                      LLAC_INT symbol) {
    uint32_t unit = st->range >> 10;
    st->low += (uint32_t)cumfreq[symbol] * unit;
    st->range = (uint32_t)(cumfreq[symbol + 1] - cumfreq[symbol]) * unit;
    if (st->low >> 24 == 1)
        st->carry = 1;
    st->low &= 0xffffffu;
    while (st->range < 0x10000u) {
        v4i_llac_arithmetic_encoder_shift_range_byte(st, st->low);
        st->range <<= 8;
        st->low = (st->low & 0xffffu) << 8;
    }
}
#include "bitstream_encode.h"

/* Select a terminal value, resolve carries/deferred bytes, and write only valid high bits of the final byte. */
static void v4i_llac_arithmetic_encoder_finish_range_encoder(RangeEncoder *st, LLAC_INT log_range) {
    LLAC_INT bits = 24 - log_range;
    uint32_t mask = 0xffffffu >> (bits & 31);
    uint32_t value = (st->low + mask) & (0xffffffu - mask);
    if (((st->low + mask) ^ (st->low + st->range)) >> 24 == 0) {
        if (((st->low + st->range) & 0xffffffu) <= value + mask) {
            ++bits;
            mask >>= 1;
            value = (st->low + mask) & (0xffffffu - mask);
        }
        if (value < st->low)
            st->carry = 1;
    }
    LLAC_INT remaining = bits;
    if (bits < 9) {
        if (st->carry != 1 && value > 0xfeffffu)
            ++st->carry_count;
        else
            v4i_llac_arithmetic_encoder_shift_range_byte(st, value);
    } else {
        LLAC_INT old;
        do {
            v4i_llac_arithmetic_encoder_shift_range_byte(st, value);
            value = (value & 0xffffu) << 8;
            old = remaining;
            remaining -= 8;
        } while (remaining != 0 && old > 7);
    }
    if (remaining < 0)
        remaining += 8;
    uint32_t final_byte = (uint32_t)st->cache;
    if (st->carry_count > 0) {
        st->ptr[st->bp++] = (LLAC_UINT8)st->cache;
        if (st->carry_count > 1) {
            memset(st->ptr + st->bp, 255, (size_t)(st->carry_count - 1));
            st->bp += st->carry_count - 1;
        }
        final_byte = 255u << ((remaining - 8) & 31);
    }
    for (LLAC_INT i = 0, bit = 128; i < remaining; ++i, bit >>= 1) {
        if (final_byte & (uint32_t)bit)
            st->ptr[st->bp] |= (LLAC_UINT8)bit;
        else
            st->ptr[st->bp] &= (LLAC_UINT8)~bit;
    }
}

/* Arithmetic-code TNS/spectral symbols, write escapes/signs/residuals backward, then close the 24-bit interval. */
void v4f_llac_encode_spectral_symbols(LLAC_UINT8 *bytes, LLAC_INT bp_side, LLAC_INT mask_side,
                                      LLAC_INT *x, const LLACTnsData *tns, LLAC_INT lastnz,
                                      const LlacSpectralPairCode *pair_codes, LLAC_UINT8 *res_bits,
                                      LLAC_INT resBitsLen, LLAC_INT lsbMode, LLAC_INT nbbits,
                                      LLAC_INT enable_lpc_weighting) {
    RangeEncoder st = {0, 0xffffffu, 0, -1, 0, 0, bytes};
#if defined(__arm__) || defined(__aarch64__) ||                                                    \
    (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                          \
     (defined(_M_IX86) || defined(_M_X64)))
    /* One bit per entry; 960-bit capacity occupies 120 bytes. */
    LLAC_UINT8 lsbs[(LHDC_V4_LLAC_MAX_FRAME_SAMPLES + 7) / 8] = {0};
#else
    /* Deferred low/sign bits are 0/1, requiring one byte per entry. */
    LLAC_UINT8 lsbs[LHDC_V4_LLAC_MAX_FRAME_SAMPLES] = {0};
#endif
    LLAC_INT lsbsLen = 0;
    for (LLAC_INT f = 0; f < tns->filter_count; ++f) {
        LLAC_INT order = tns->order[f];
        if (order <= 0)
            continue;
        v4i_llac_arithmetic_encoder_encode_symbol(&st, alg_code_t6[enable_lpc_weighting],
                                                  order - 1);
        for (LLAC_INT i = 0; i < order; ++i)
            v4i_llac_arithmetic_encoder_encode_symbol(&st, alg_code_t5[i],
                                                      tns->reflection_indices[8 * f + i]);
    }
    for (LLAC_INT k = 0; k < lastnz; k += 2, ++pair_codes) {
        LLAC_INT a = x[k], b = x[k + 1], level = pair_codes->escape_count;
        uint32_t first_magnitude = a < 0 ? 0u - (uint32_t)a : (uint32_t)a;
        uint32_t second_magnitude = b < 0 ? 0u - (uint32_t)b : (uint32_t)b;
        LLAC_INT lsb1 = 0, lsb2 = 0;
        for (LLAC_INT lev = 0; lev < level; ++lev) {
            LLAC_INT context = pair_codes->context + (lev < 3 ? lev : 3) * 1024;
            v4i_llac_arithmetic_encoder_encode_symbol(&st, alg_code_t4[alg_code_t3[context]], 16);
            LLAC_INT bit1 = (first_magnitude >> (lev & 31)) & 1u,
                     bit2 = (second_magnitude >> (lev & 31)) & 1u;
            if (lsbMode == 1 && lev == 0) {
                lsb1 = bit1;
                lsb2 = bit2;
            } else {
                v4i_llac_write_side_bit(bytes, &bp_side, &mask_side, bit1);
                v4i_llac_write_side_bit(bytes, &bp_side, &mask_side, bit2);
            }
        }
        LLAC_INT lev1 = level > 0 ? level : 0;
        if (lev1 > 3)
            lev1 = 3;
        v4i_llac_arithmetic_encoder_encode_symbol(
            &st, alg_code_t4[alg_code_t3[pair_codes->context + lev1 * 1024]], pair_codes->symbol);
        if (lsbMode == 1 && level > 0) {
            first_magnitude >>= 1;
            second_magnitude >>= 1;
#if defined(__arm__) || defined(__aarch64__) ||                                                    \
    (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                          \
     (defined(_M_IX86) || defined(_M_X64)))
            unsigned group = (unsigned)lsb1;
            unsigned group_bits = 1;
            if (a != 0 && first_magnitude == 0) {
                group |= (unsigned)(a < 0) << group_bits;
                ++group_bits;
            }
            group |= (unsigned)lsb2 << group_bits;
            ++group_bits;
            if (b != 0 && second_magnitude == 0) {
                group |= (unsigned)(b < 0) << group_bits;
                ++group_bits;
            }
            v4i_llac_append_deferred_group(lsbs, &lsbsLen, group, group_bits);
#else
            lsbs[lsbsLen++] = lsb1;
            if (a != 0 && first_magnitude == 0)
                lsbs[lsbsLen++] = a < 1;
            lsbs[lsbsLen++] = lsb2;
            if (b != 0 && second_magnitude == 0)
                lsbs[lsbsLen++] = b < 1;
#endif
        }
        if (first_magnitude)
            v4i_llac_write_side_bit(bytes, &bp_side, &mask_side, a < 0);
        if (second_magnitude)
            v4i_llac_write_side_bit(bytes, &bp_side, &mask_side, b < 0);
    }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86) && LHDC_V4_LOG2_VIA_LN
    /* Normalized range is 65536..16777215. Thresholds retain ln-path single rounding,
     * so some powers of two carry early; this is not mathematical floor(log2). */
    static const uint32_t rounded_log2_thresholds[] = {131072,  262144,  524288,  1048576,
                                                       2097151, 4194303, 8388601, 16777201};
    LLAC_INT log_range = 16;
    for (unsigned i = 0; i < sizeof rounded_log2_thresholds / sizeof rounded_log2_thresholds[0] &&
                         st.range >= rounded_log2_thresholds[i];
         ++i)
        ++log_range;
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Other log policies retain dynamic evaluation; only finite integer extraction changes. */
    LLAC_INT log_range = audio_math_truncate_i32_f32(
        audio_math_floor_f32(lhdc_v4_log2_compat_f32((LLAC_FLOAT)st.range)));
#else
    LLAC_INT log_range =
        (LLAC_INT)audio_math_floor_f32(lhdc_v4_log2_compat_f32((LLAC_FLOAT)st.range));
    LLAC_FLOAT log_mask = lhdc_v4_log2_compat_f32((LLAC_FLOAT)mask_side);
#endif
    LLAC_INT nbits_ari = st.bp * 8 + 33 - log_range + (st.cache >= 0 ? 8 : 0);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Backward-write masks cycle through 1,2,4...128; binary exponent determines bit position.
     * Valid packet bit counts are exactly representable; compute remaining side information directly with integers. */
    LLAC_INT nbits_side =
        (LLAC_INT)audio_math_ceil_log2_u32((uint32_t)mask_side) - (bp_side * 8 + 16) + nbbits;
#else
    LLAC_INT nbits_side =
        (LLAC_INT)((log_mask - (LLAC_FLOAT)(bp_side * 8 + 16)) + (LLAC_FLOAT)nbbits);
#endif
    LLAC_INT residual = nbbits - st.carry_count * 8 - nbits_side - nbits_ari;
    LLAC_INT available = lsbMode == 0 ? resBitsLen : lsbsLen;
    if (residual > available)
        residual = available;
    for (LLAC_INT i = 0; i < residual; ++i) {
#if defined(__arm__) || defined(__aarch64__) ||                                                    \
    (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                          \
     (defined(_M_IX86) || defined(_M_X64)))
        LLAC_INT bit = ((lsbMode == 0 ? res_bits : lsbs)[i >> 3] >> (i & 7)) & 1;
#else
        LLAC_INT bit = lsbMode == 0 ? (res_bits[i >> 3] >> (i & 7)) & 1 : lsbs[i];
#endif
        v4i_llac_write_side_bit(bytes, &bp_side, &mask_side, bit);
    }
    v4i_llac_arithmetic_encoder_finish_range_encoder(&st, log_range);
}

/* Write integer low bits using signed division-by-two rules; callers guarantee output capacity. */
static void v4i_llac_side_information_write_side_bits(LLAC_UINT8 *bytes, LLAC_INT *bp_side,
                                                      LLAC_INT *mask_side, LLAC_INT value,
                                                      LLAC_INT count) {
    for (LLAC_INT k = 0; k < count; ++k) {
        v4i_llac_write_side_bit(bytes, bp_side, mask_side, (uint32_t)value & 1u);
        value /= 2;
    }
}

/* Write bandwidth, gain, TNS, SNS, LTPF, and noise side information; return remaining tail position. */
void v4f_llac_write_side_information(LLAC_UINT8 *bytes, LLAC_INT *bp_side, LLAC_INT *mask_side,
                                     LLAC_INT numbytes, LLAC_INT bw_cutoff_bits,
                                     LLAC_INT bw_cutoff_idx, LLAC_INT lastnz, LLAC_INT N,
                                     LLAC_INT lsbMode, LLAC_INT gg_idx, const LLACTnsData *tns,
                                     const LLACLtpfIndices *ltpf, const LLACSnsIndices *sns,
                                     LLAC_INT fac_ns_idx) {
    const LLAC_INT gainMSBbits[4] = {1, 1, 2, 2}, gainLSBbits[4] = {0, 1, 0, 1};
    *bp_side = numbytes - 1;
    *mask_side = 1;
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, bw_cutoff_idx,
                                              bw_cutoff_bits);
    LLAC_INT nz_bits = (LLAC_INT)audio_math_ceil_log2_u32((uint32_t)(N / 2));
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, lastnz / 2 - 1, nz_bits);
    v4i_llac_write_side_bit(bytes, bp_side, mask_side, lsbMode != 0);
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, gg_idx, 8);
    for (LLAC_INT i = 0; i < tns->filter_count; ++i)
        v4i_llac_write_side_bit(bytes, bp_side, mask_side, tns->order[i] != 0);
    v4i_llac_write_side_bit(bytes, bp_side, mask_side, ltpf->present != 0);
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, sns->low_codebook, 5);
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, sns->high_codebook, 5);
    LLAC_INT submode = sns->submode, gain = sns->gain;
    v4i_llac_write_side_bit(bytes, bp_side, mask_side, (uint32_t)submode + 1u > 2u);
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side,
                                              gain >> gainLSBbits[submode], gainMSBbits[submode]);
    v4i_llac_write_side_bit(bytes, bp_side, mask_side, sns->leading_sign != 0);
    uint32_t gainLSB = (uint32_t)gain & 1u, combined;
    if ((uint32_t)submode + 1u < 3u) {
        if (!(submode & 1))
            gainLSB = (uint32_t)sns->secondary_mpvq + 2u;
        combined = (uint32_t)sns->main_mpvq + gainLSB * LHDC_V4_LLAC_SNS_MPVQ_RADIX;
        v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, (LLAC_INT)combined,
                                                  25);
    } else {
        combined = (uint32_t)sns->main_mpvq;
        if (submode & 1)
            combined = (gainLSB | 0xe74c00u) + combined * 2u;
        v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, (LLAC_INT)combined,
                                                  24);
    }
    if (ltpf->present == 1) {
        v4i_llac_write_side_bit(bytes, bp_side, mask_side, (uint32_t)ltpf->active & 1u);
        v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, ltpf->pitch, 9);
    }
    v4i_llac_side_information_write_side_bits(bytes, bp_side, mask_side, fac_ns_idx, 3);
}
#include "../include/codec/scalar.h"

/* Encode quantized spectral residual bits within the remaining budget. */

void v4_llac_encode_residual_bits(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gain, LLAC_INT L_spec,
                                  LLAC_INT targetBits, LLAC_INT nBits, LLAC_UINT8 *resBits,
                                  LLAC_INT *numResBits, LLAC_INT hrmode) {
    memset(resBits, 0, 625 * sizeof *resBits);
    LLAC_INT budget = (LLAC_INT)((uint32_t)targetBits - (uint32_t)nBits + (hrmode ? 14u : 4u));
    uint32_t k = 0;
    LLAC_FLOAT offset = 0.25f;
    uint32_t iter_max = hrmode ? 20u : 1u;
    if (budget > 0) {
        for (uint32_t iter = 0; iter < iter_max; ++iter) {
            for (LLAC_INT i = 0; i < L_spec && k < (uint32_t)budget; ++i) {
                if (xq[i]) {
                    LLAC_UINT8 mask = (LLAC_UINT8)(1u << (k & 7u));
                    if ((LLAC_FLOAT)xq[i] * gain <= x[i]) {
                        resBits[k >> 3] |= mask;
                        x[i] -= offset * gain;
                    } else {
                        resBits[k >> 3] &= (LLAC_UINT8)~mask;
                        x[i] += offset * gain;
                    }
                    ++k;
                }
            }
            if (k >= (uint32_t)budget)
                break;
            offset *= 0.5f;
        }
    }
    *numResBits = (LLAC_INT)k;
}
