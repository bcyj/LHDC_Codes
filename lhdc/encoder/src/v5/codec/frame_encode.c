#include "../include/codec/bit_writer.h"
#include "../include/codec/bit_operations.h"
#include "../include/codec/fast_math.h"
#include <audio_math/float_convert.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include <math.h>
#include <string.h>

static const uint8_t v5i_frame_byte_order[2][8] = {{4, 0, 1, 5, 7, 3, 2, 6},
                                                   {6, 3, 7, 0, 2, 1, 4, 5}};

/* Quantize spectrum backward within cutoff, updating nonzero count and even coded length. */
static void quantize_spectrum(LhdcEncContext *ecb, float gain) {
    bpc_buf_s *bpc = &ecb->ed.enc.lossy.bpc;
    int32_t band_stop = ecb->band_stop;
    uint32_t frame_size = ecb->frm_size;
    uint32_t first_nonzero = frame_size - band_stop;
    bpc->sbn = 0;
    if (band_stop > 0) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        const float *spectrum_end = ecb->ed.sdf + frame_size - 1;
#else
        const float *psdf = ecb->ed.sdf + band_stop - 1;
#endif
        int32_t i = (int32_t)(ecb->frm_size - band_stop);
        do {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            /* Address backward from an invariant spectral-tail base so the zero branch can reuse the address register. */
            float sdf = spectrum_end[-i];
#else
            float sdf = *psdf;
#endif
            if (0.5f / gain <= sdf) {
                /* Win32 kernel builds retain extended product precision until truncation; other targets evaluate in float. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                bpc->rd[i] = audio_math_truncate_i32_f64((double)gain * sdf + 0.5);
#else
                bpc->rd[i] = audio_math_truncate_i32_f32(gain * sdf + 0.5f);
#endif
                ++bpc->sbn;
            } else if (sdf <= -(0.5f / gain)) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                bpc->rd[i] = audio_math_truncate_i32_f64(0.5 - (double)gain * sdf);
#else
                bpc->rd[i] = audio_math_truncate_i32_f32(0.5f - gain * sdf);
#endif
                ++bpc->sbn;
            } else {
                bpc->rd[i] = 0;
            }
            frame_size = ecb->frm_size;
            ++i;
#if !(defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86))
            --psdf;
#endif
        } while (i < (int32_t)frame_size);
        int32_t start = frame_size - band_stop;
        first_nonzero = frame_size;
        if ((int32_t)frame_size <= start)
            first_nonzero = start + 1;
        i = start;
        do {
            if (bpc->rd[i] != 0) {
                first_nonzero = (uint32_t)i;
                break;
            }
            ++i;
        } while (i < (int32_t)frame_size);
    }
    int32_t nzc = 0;
    if (frame_size - first_nonzero != 0)
        nzc = (frame_size - first_nonzero) + (first_nonzero & 1);
    bpc->nzc = nzc < 5 ? 4 : nzc;
}
/* Search preset bounds for the minimum gain index needed by 24-bit spectrum. */
static int32_t minimum_spectrum_gain(LhdcEncContext *ecb) {
    float fmin = -LHDC_V5_QUANTIZED_SPECTRUM_LIMIT, fmax = LHDC_V5_QUANTIZED_SPECTRUM_LIMIT;
    for (int32_t i = 0; i < ecb->frm_size; ++i) {
        float sdf = ecb->ed.sdf[i];
        /* Extrema use ordered comparisons; NaN updates neither extreme. */
        if (sdf > fmax)
            fmax = sdf;
        else if (sdf < fmin)
            fmin = sdf;
    }
    float magnitude = -fmin;
    if (-fmin <= fmax)
        magnitude = fmax;
    int32_t gi = 0;
    if (magnitude > 0.0f) {
        gi = -1;
        float gain;
        do {
            ++gi;
            gain = v5f_EncGainByTable(gi, ecb->gain_tab, ecb->gain_tab_base, ecb->gain_tab_step);
        } while (!(magnitude * gain <= LHDC_V5_QUANTIZED_SPECTRUM_LIMIT));
        ecb->ed.gi_highest = gi;
    }
    return gi;
}
/* Iteratively select gain within the bitrate budget and retain inter-frame gain-change slope. */
static void select_channel_gain(LhdcEncContext *ecb, ChannelData *chd) {
    int32_t gi = 0;
    ecb->ed.gi_highest = 0;
    if (ecb->bits_per_sample == 24)
        gi = minimum_spectrum_gain(ecb);
    if (gi <= chd->gi)
        gi = chd->gi;
    int32_t gi_hi = LHDC_V5_GAIN_INDEX_MAX, gi_low = -1, enc_len_last = 0, gi_last = gi;
    float step = chd->gain_step;
    for (;;) {
        float gain = v5f_EncGainByTable(gi, ecb->gain_tab, ecb->gain_tab_base, ecb->gain_tab_step);
        quantize_spectrum(ecb, gain);
        int32_t enc_len = v5f_BitsPredictCompressEval(ecb);
        if (enc_len > ecb->ava_bits) {
            gi_low = gi;
            if (gi == gi_hi - 1) {
                gain = v5f_EncGainByTable(gi_hi, ecb->gain_tab, ecb->gain_tab_base,
                                          ecb->gain_tab_step);
                quantize_spectrum(ecb, gain);
                v5f_BitsPredictCompressEval(ecb);
                gi = gi_hi;
                break;
            }
        } else {
            if (enc_len >= ecb->ava_bits_low || gi <= ecb->ed.gi_highest)
                break;
            gi_hi = gi;
            if (ecb->gain_tab_base + 1 >= gi) {
                enc_len_last = enc_len;
                --gi;
                continue;
            }
            if (gi == gi_low + 1)
                break;
        }
        if (gi != gi_last) {
            int32_t bit_delta = enc_len_last - enc_len, gain_delta = gi - gi_last;
            if (gi <= gi_last) {
                bit_delta = enc_len - enc_len_last;
                gain_delta = gi_last - gi;
            }
            int32_t slope = gain_delta ? bit_delta / gain_delta : 0;
            step = step * 0.8f + (float)slope * 0.2f;
        }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Round numerator to float first; Win32 kernel quotient retains double precision before truncation. */
        float bit_difference = (float)(enc_len - ecb->ava_bits);
        int32_t gain_delta = audio_math_truncate_i32_f64((double)bit_difference / step);
#else
        int32_t gain_delta = audio_math_truncate_i32_f32((float)(enc_len - ecb->ava_bits) / step);
#endif
        int32_t next_gain = gi + gain_delta;
        if (gain_delta == 0)
            next_gain += enc_len < ecb->ava_bits ? -1 : 1;
        if (next_gain <= ecb->gain_tab_base)
            next_gain = ecb->gain_tab_base + 1;
        int32_t above_lower = next_gain <= gi_low ? gi_low + 1 : next_gain;
        enc_len_last = enc_len;
        gi_last = gi;
        gi = next_gain <= gi_hi ? above_lower : gi_hi - 1;
    }
    chd->gi = gi;
    chd->gain_step = step;
}
/* Remove trailing coefficient pairs when over budget and reestimate bit-plane compression cost. */
static void reduce_channel_coefficients(LhdcEncContext *ecb) {
    bpc_buf_s *bpc = &ecb->ed.enc.lossy.bpc;
    int32_t nzc = bpc->nzc, removed_cost = 0;
    int32_t remaining_cost = bpc->sp_enc_bits - bpc->sp_eval_bits;
    int32_t first = (int32_t)ecb->frm_size - nzc;
    int32_t pair = 0;
    do {
        nzc -= 2;
        int32_t cost0 = 4, cost1 = 4;
        if (bpc->rd[first + pair] != 0) {
            --remaining_cost;
            --bpc->sbn;
            cost0 = 20;
        }
        if (bpc->rd[first + pair + 1] != 0) {
            --remaining_cost;
            --bpc->sbn;
            cost1 = 20;
        }
        removed_cost += cost0 + cost1;
        bpc->nzc = nzc;
        remaining_cost = (remaining_cost - bpc->rb[pair]) - bpc->rb[pair + 1];
        pair += 2;
    } while ((removed_cost >> 4) < remaining_cost);
    v5f_BitsPredictCompressEval(ecb);
}
/* Write mono side information, FAC bit planes, and sign remainders; trim coefficients and retry if necessary. */
static int32_t write_channel_frame(LhdcEncContext *ecb, LHDCV5_Workspace *workspace, int32_t ch,
                                   uint32_t *output) {
    bpc_buf_s *bpc = &ecb->ed.enc.lossy.bpc;
    for (;;) {
        BitWriter *ptr_bw = v5f_lhdc_v5_get_mem_content_addr(workspace, BW_INDEX);
        v5_bw_init_inline(ptr_bw, output, ecb->enc_frm_len);
        fac_enc *ptr_fenc_s = v5f_lhdc_v5_get_mem_content_addr(workspace, FENC_S_INDEX);
        uint32_t header14 = (((uint32_t)ecb->chd[ch].gi & 0x1FFu) << 4) |
                            ((uint32_t)(ecb->ed.adsq_index0 - 8) & 0x0Fu);
        uint32_t rtn = v5_bw_write_uint_inline(ptr_bw, header14, 14);
        rtn |= v5f_BitWriteBitsArrayU8(ptr_bw, ecb->ed.adsq_code, 1, ecb->band_cfg->n - 1, 0);
        uint32_t nzc_val = (uint32_t)((bpc->nzc >> 1) - 1) & (UINT32_MAX >> (32 - ecb->nzc_bits));
        uint32_t bpc_header = (nzc_val << (1 + ecb->init_bn_bits)) |
                              (((uint32_t)bpc->alpha & 1u) << ecb->init_bn_bits) |
                              ((uint32_t)bpc->rb[0] & (UINT32_MAX >> (32 - ecb->init_bn_bits)));
        rtn |= v5_bw_write_uint_inline(ptr_bw, bpc_header, ecb->nzc_bits + 1 + ecb->init_bn_bits);
        rtn |= v5f_BitsPredictCompressOutput(ecb, ptr_fenc_s, ptr_bw);
        int32_t nzc = bpc->nzc, side_bits = bpc->sp_enc_bits, bits = 0;
        uint32_t u32 = 0, empty = 32;
        for (int32_t i = 0; i < nzc; ++i) {
            uint32_t nb = bpc->rb[i];
            if (empty < nb + 1) {
                rtn |= v5_bw_write_uint_inline(ptr_bw, u32, 32 - empty);
                nb = bpc->rb[i];
                empty = 32;
            }
            uint32_t rdt = bpc->rd[(int32_t)ecb->frm_size - nzc + i];
            if (nb != 0) {
                empty -= nb;
                u32 = (rdt & (UINT32_MAX >> (-nb & 31))) | (u32 << (nb & 31));
                bits += nb;
            }
            if (rdt != 0) {
                --empty;
                u32 <<= 1;
                if (ecb->ed.sdf[nzc - i - 1] < 0.0f)
                    u32 |= 1;
            }
        }
        rtn |= v5_bw_write_uint_inline(ptr_bw, u32, 32 - empty);
        bits += bpc->sbn;
        if (bits < 24) {
            rtn |= v5_bw_write_uint_inline(ptr_bw, 0, 24 - bits);
            bits = 24;
        }
        bits += side_bits;
        if (bits > ecb->ava_bits) {
            reduce_channel_coefficients(ecb);
            continue;
        }
        if (bits < ecb->ava_bits && bpc->nzc > 0) {
            uint32_t frame_size = ecb->frm_size;
            int32_t i = (int32_t)(int32_t)frame_size;
            int32_t sdf_index = -(int32_t)frame_size;
            float gain = ecb->gain_tab[ecb->chd[ch].gi];
            do {
                if (bpc->rd[i - 1] != 0) {
                    float sdf = ecb->ed.sdf[(int32_t)frame_size + sdf_index];
                    float signed_gain = -gain;
                    if (0.0f <= sdf)
                        signed_gain = gain;
                    int32_t truncated_spectrum;
                    /* Win32 kernel products retain extended precision until truncation without additional float rounding. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                    truncated_spectrum = audio_math_truncate_i32_f64((double)sdf * signed_gain);
#else
                    truncated_spectrum = audio_math_truncate_i32_f32(sdf * signed_gain);
#endif
                    rtn |= v5_bw_write_uint_inline(ptr_bw, bpc->rd[i - 1] - truncated_spectrum, 1);
                    ++bits;
                }
                if (ecb->ava_bits <= bits)
                    break;
                frame_size = ecb->frm_size;
                --i;
                ++sdf_index;
            } while (((int32_t)frame_size - bpc->nzc) < i);
        }
        for (int32_t total_bits = bits + ecb->side_info_bits + 8; total_bits < ecb->enc_frm_bits;
             total_bits += 8)
            rtn |= v5_bw_write_uint_inline(ptr_bw, 0, 8);
        rtn |= v5_bw_zero_pad_inline(ptr_bw);
        if (rtn == 0)
            rtn = ecb->enc_frm_bits;
        if ((int32_t)rtn > 0) {
            ptr_bw = v5f_lhdc_v5_get_mem_content_addr(workspace, BW_INDEX);
            return v5_bw_flush_inline(ptr_bw);
        }
        reduce_channel_coefficients(ecb);
    }
}
/* Per channel, run PCM to MDCT, SNS/ADSQ, gain search, and bitstream output; update frame count. */
void v5f_LhdcEncode(int32_t *pcm, uint32_t *out0, int32_t *out_len0, uint32_t *out1,
                    int32_t *out_len1, LHDCV5_Workspace *workspace) {
    LhdcEncContext *ecb = v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_INDEX);
    for (int32_t ch = 0; ch < ecb->channels; ++ch) {
        memcpy(ecb->pcm, pcm + ecb->frm_size * (int32_t)ch, (size_t)ecb->frm_size * sizeof *pcm);
        mdct_ch *channels = v5f_lhdc_v5_get_mem_content_addr(workspace, MDCT_CH_INDEX);
        mdct_s *md = ecb->mdct_ws;
        md->ch = channels + ch;
        float *in = ecb->ed.enc.lossy.mdct.pcm_in;
        for (int32_t i = 0; i < ecb->frm_size; ++i)
            in[i] = (float)ecb->pcm[i];
        v5f_do_mdct(md, (int32_t)ch, in, ecb->ed.sdf);
        ecb->ed.be_mean = v5f_sns_encode(ecb);
        if (ecb->ed.adsq_index0 != 23)
            v5f_sns_apply(ecb->band_cfg, ecb->ed.band_scf, ecb->ed.sdf, ecb->frm_size);
        select_channel_gain(ecb, &ecb->chd[ch]);
        int32_t *encoded_length = ch == 0 ? out_len0 : out_len1;
        *encoded_length = write_channel_frame(ecb, workspace, ch, ch == 0 ? out0 : out1);
    }
    ++ecb->fc;
}

/* Encoding workspace: buffer PCM and output complete frames. */
int32_t v5f_lhdc_v5_enc_encode(int32_t *pcm, uint8_t *encoded_frame, int32_t *encoded_frame_size,
                               LHDCV5_Workspace *workspace) {
    if (!pcm || !encoded_frame || !encoded_frame_size || !workspace)
        return LHDC_V5_ENC_ERROR;
    LhdcEncContext *ecb = workspace->encoder;
    hdr_s *hdr_state = workspace->header;
    uint8_t *out[2] = {(void *)workspace->encoded_ch0, (void *)workspace->encoded_ch1};
    int32_t enc_frm_len = 0, channel_bytes[2] = {0, 1};
    int32_t header_bytes =
        v5f_enc_process_header(hdr_state, ecb->channels, &enc_frm_len, encoded_frame);
    if (hdr_state->enc_frm_len_need_udpate) {
        v5f_LhdcEncSetEncFrameLen(enc_frm_len, workspace);
        hdr_state->enc_frm_len_need_udpate = 0;
    }
    v5f_LhdcEncode(pcm, (void *)out[0], channel_bytes, (void *)out[1], channel_bytes + 1,
                   workspace);
    for (int32_t channel_index = 0; channel_index < 2; ++channel_index) {
        uint64_t scrambled_prefix;
        uint8_t bytes[8];
        scrambled_prefix = audio_math_read_le64(out[channel_index]);
        scrambled_prefix ^= UINT64_C(0x73cde5dab37ae7ff);
        audio_math_write_le64(bytes, scrambled_prefix);
        uint32_t sel = out[channel_index][8] & 1;
        for (int32_t i = 0; i < 8; ++i)
            out[channel_index][i] = bytes[v5i_frame_byte_order[sel][i]];
    }
    memcpy(encoded_frame + header_bytes, out[0], ecb->enc_frm_len);
    memcpy(encoded_frame + header_bytes + ecb->enc_frm_len, out[1], ecb->enc_frm_len);
    *encoded_frame_size = header_bytes + ecb->enc_frm_len * ecb->channels;
    return LHDC_V5_ENC_OK;
}

/* Set encoded frame length and update the effective bit budget. */
int32_t v5f_LhdcEncSetEncFrameLen(int32_t enc_frm_len, LHDCV5_Workspace *workspace) {
    LhdcEncContext *ecb = v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_INDEX);
    int32_t frame_duration = ecb->frm_duration;
    if (frame_duration != LHDC_V5_FRAME_DURATION_5MS &&
        frame_duration != LHDC_V5_FRAME_DURATION_10MS)
        return LHDC_V5_ENC_ERROR;
    if (enc_frm_len >= 2001 || enc_frm_len < frame_duration * 30 / 100)
        return LHDC_V5_ENC_ERROR;
    int32_t normalized_frame_length = frame_duration ? enc_frm_len * 100 / frame_duration : 0,
            i = 0;
    /* Above the table's maximum length, retain its last bandwidth entry instead of reading adjacent constants. */
    const int32_t band_stop_count = (int32_t)(sizeof band_stop / sizeof band_stop[0]);
    while (i + 1 < band_stop_count && band_stop[i].frm_len < normalized_frame_length)
        ++i;
    int32_t band_stop_limit = band_stop[i].stop * frame_duration / 100;
    ecb->band_stop = band_stop_limit <= ecb->frm_size ? band_stop_limit : ecb->frm_size;
    v5f_GenerateGainTable(ecb, enc_frm_len, frame_duration);
    int32_t bitrate = frame_duration ? enc_frm_len * 80000 / frame_duration : 0,
            percent = frame_duration < LHDC_V5_FRAME_DURATION_10MS ? 90
                      : bitrate < 120000                           ? 95
                                                                   : 96;
    if (frame_duration != LHDC_V5_FRAME_DURATION_5MS &&
        frame_duration != LHDC_V5_FRAME_DURATION_10MS)
        return LHDC_V5_ENC_ERROR;
    if (ecb->sample_rate == LHDC_V5_SAMPLE_RATE_44100 ||
        ecb->sample_rate == LHDC_V5_SAMPLE_RATE_48000)
        ecb->band_cfg = bitrate >= 120000 ? &band_480_HR
                        : bitrate > 95999 ? &band_480
                                          : &band_480_LB;
    else if (ecb->sample_rate == LHDC_V5_SAMPLE_RATE_96000)
        ecb->band_cfg = &band_960;
    else if (ecb->sample_rate == LHDC_V5_SAMPLE_RATE_192000)
        ecb->band_cfg = &band_1920;
    else
        return LHDC_V5_ENC_ERROR;
    ecb->side_info_bits = ecb->nzc_bits + (enc_frm_len > 159 ? 4 : 0) + ecb->band_cfg->n + 14;
    for (i = 0; i < 2; ++i) {
        int32_t gi = enc_frm_len ? ecb->enc_frm_len * ecb->chd[i].gi / enc_frm_len : 0;
        ecb->chd[i].gi = gi > 510 ? LHDC_V5_GAIN_INDEX_MAX : gi;
    }
    ecb->enc_frm_len = enc_frm_len;
    ecb->enc_frm_bits = enc_frm_len * 8;
    ecb->ava_bits = ecb->enc_frm_bits - ecb->side_info_bits;
    int32_t low_bit_budget = ecb->ava_bits * percent / 100;
    ecb->ava_bits_low = ecb->ava_bits - low_bit_budget > 23 ? low_bit_budget : ecb->ava_bits - 24;
    ecb->init_bn_bits = enc_frm_len < 160 ? 0 : 4;
    ecb->max_init_bn = enc_frm_len < 160 ? 0 : 1920;
    if (ecb->bits_per_sample == 16) {
        ecb->eval_cp = 0x10290;
        if (enc_frm_len < 126) {
            int32_t high =
                (int32_t)(((int64_t)(enc_frm_len * 0xe14 - 0x2bfc8) * -0x1b4e81b5) >> 32);
            ecb->eval_cp = (high >> 3) - (high >> 31) + 0x10f5c;
        }

        return LHDC_V5_ENC_OK;
    }
    if (enc_frm_len < 121) {
        int32_t correction_numerator = enc_frm_len * 0xccc - 0x27fb5;
        int32_t high =
            (int32_t)(((int64_t)correction_numerator * 0x15f15f15) >> 32) - correction_numerator;
        ecb->eval_cp = (high >> 6) - (high >> 31) + 0x10ccc;
    } else
        ecb->eval_cp = 0x10000;
    return LHDC_V5_ENC_OK;
}

/* Initialize encoder context, bands, MDCT, and gain tables. */
int32_t v5f_LhdcEncInit(LHDCV5_Workspace *workspace, int32_t wsize, int32_t ch,
                        int32_t bits_per_sample, int32_t sample_rate, int32_t frm_duration,
                        int32_t enc_frm_len) {
    int32_t total = sizeof(LhdcEncContext) + v5f_mdct_get_size();
    if (!workspace)
        return total;
    if (wsize < total)
        return LHDC_V5_ENC_ERROR;
    LhdcEncContext *ecb = v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_INDEX);
    /* Clear complete 32-bit words, preserving any trailing partial word. */
    if (total > 3)
        memset(ecb, 0, ((uint32_t)total >> 2) * 4u);
    ecb->lossy_uc = (uint8_t *)ecb->ed.enc.lossy.bpc.bna;
    if ((frm_duration != LHDC_V5_FRAME_DURATION_2P5MS &&
         frm_duration != LHDC_V5_FRAME_DURATION_5MS &&
         frm_duration != LHDC_V5_FRAME_DURATION_10MS) ||
        (bits_per_sample != 16 && bits_per_sample != 24) ||
        (sample_rate != LHDC_V5_SAMPLE_RATE_44100 && sample_rate != LHDC_V5_SAMPLE_RATE_48000 &&
         sample_rate != LHDC_V5_SAMPLE_RATE_96000 && sample_rate != LHDC_V5_SAMPLE_RATE_192000))
        return LHDC_V5_ENC_ERROR;
    int32_t effective =
        sample_rate == LHDC_V5_SAMPLE_RATE_44100 ? LHDC_V5_SAMPLE_RATE_48000 : sample_rate;
    ecb->sample_rate = sample_rate;
    ecb->bits_per_sample = bits_per_sample;
    ecb->channels = ch;
    ecb->frm_duration = frm_duration;
    ecb->frm_size = effective * frm_duration / LHDC_V5_FRAME_DURATION_1S;
    ecb->nzc_bits = v5_calc_bits_inline(ecb->frm_size) - 1;
    if (v5f_LhdcEncSetEncFrameLen(enc_frm_len, workspace))
        return LHDC_V5_ENC_ERROR;
    ecb->chd[0].gain_step = ecb->chd[1].gain_step = 17.f;
    ecb->overlap = effective / (frm_duration < LHDC_V5_FRAME_DURATION_10MS ? 500 : 400);
    ecb->pcm = ecb->ed.enc.lossy.mdct.dat + ecb->overlap;
    ecb->mdct_ws = v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_MDCT_WS_INDEX);
    int32_t ret = v5f_mdct_init(ecb->mdct_ws, ecb->frm_size * 2, ecb->overlap, 31 - bits_per_sample,
                                &ecb->ed.enc.lossy.mdct,
                                v5f_lhdc_v5_get_mem_content_addr(workspace, MDCT_TWID1_INDEX),
                                v5f_lhdc_v5_get_mem_content_addr(workspace, MDCT_TWID2_INDEX),
                                v5f_lhdc_v5_get_mem_content_addr(workspace, MDCT_WIN_INDEX),
                                v5f_lhdc_v5_get_mem_content_addr(workspace, FFT_TWID_INDEX));
    return ret ? ret : total;
}

/* Encoding workspace: initialize configuration and runtime state. */
int32_t v5f_lhdc_v5_enc_init(int32_t ch, int32_t bits_per_sample, int32_t sample_rate,
                             int32_t frm_duration, int32_t bitrate, LHDCV5_Workspace *workspace) {
    if ((uint32_t)(ch - 1) >= 8 || (bits_per_sample != 16 && bits_per_sample != 24) ||
        (sample_rate != LHDC_V5_SAMPLE_RATE_44100 && sample_rate != LHDC_V5_SAMPLE_RATE_48000 &&
         sample_rate != LHDC_V5_SAMPLE_RATE_96000 && sample_rate != LHDC_V5_SAMPLE_RATE_192000) ||
        (uint32_t)bitrate > LHDC_V5_MAX_BITRATE_BPS ||
        (frm_duration != LHDC_V5_FRAME_DURATION_5MS &&
         frm_duration != LHDC_V5_FRAME_DURATION_10MS) ||
        !workspace)
        return LHDC_V5_ENC_ERROR;
    hdr_s *hdr_state = v5f_lhdc_v5_get_mem_content_addr(workspace, HDR_S_INDEX);
    int32_t denom = (sample_rate == LHDC_V5_SAMPLE_RATE_44100 ? 73500 : 80000) * ch;
    int32_t len = denom ? bitrate * frm_duration / denom : 0;
    hdr_state->enc_frm_len_provided = len;
    return v5f_LhdcEncInit(workspace, 99999999, ch, bits_per_sample, sample_rate, frm_duration,
                           len) < 0
               ? LHDC_V5_ENC_ERROR
               : LHDC_V5_ENC_OK;
}

/* Encoding workspace: set bitrate and update output frame length. */
int32_t v5f_lhdc_v5_enc_set_bitrate(int32_t bitrate, LHDCV5_Workspace *workspace) {
    if ((uint32_t)bitrate > LHDC_V5_MAX_BITRATE_BPS || !workspace)
        return LHDC_V5_ENC_ERROR;
    LhdcEncContext *ecb = v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_INDEX);
    hdr_s *hdr_state = v5f_lhdc_v5_get_mem_content_addr(workspace, HDR_S_INDEX);
    int32_t bitrate_denominator =
                (ecb->sample_rate == LHDC_V5_SAMPLE_RATE_44100 ? 73500 : 80000) * ecb->channels,
            enc_frm_len =
                bitrate_denominator ? ecb->frm_duration * bitrate / bitrate_denominator : 0;
    hdr_state->enc_frm_len_provided = enc_frm_len;
    hdr_state->enc_frm_len_need_udpate = 1;
    return v5f_LhdcEncSetEncFrameLen(enc_frm_len, workspace) < 0 ? LHDC_V5_ENC_ERROR
                                                                 : LHDC_V5_ENC_OK;
}

/* Read PCM samples per frame. */
int32_t v5f_LhdcEncFrameSize(LHDCV5_Workspace *workspace) {
    return ((LhdcEncContext *)v5f_lhdc_v5_get_mem_content_addr(workspace, ECB_INDEX))->frm_size;
}

/* Encoding workspace: read samples per frame. */
int32_t v5f_lhdc_v5_enc_get_samples_per_frame(int32_t *sample_per_frame,
                                              LHDCV5_Workspace *workspace) {
    if (!sample_per_frame || !workspace)
        return LHDC_V5_ENC_ERROR;
    *sample_per_frame = v5f_LhdcEncFrameSize(workspace);
    return LHDC_V5_ENC_OK;
}
