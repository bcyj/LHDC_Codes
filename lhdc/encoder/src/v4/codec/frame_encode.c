#include "lhdc_memory.h"
#include "../../../../common/include/lhdc_log.h"
#include <audio_math/constants.h>
#include <audio_math/float_scaling.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../../../../common/include/dsp/float_compat.h"
#include "../include/codec/integer_ops.h"
#include "../include/codec/codec_internal.h"
#include <stdlib.h>
#include <string.h>

/* Obtain a residual encoder from the specified host; never dereference failed allocations. */
struct lhdc_Encoder_s *v4f_lhdc_NewEncoder_with_allocator(const LhdcAllocator *allocator) {
    struct lhdc_Encoder_s *encb = lhdc_memory_alloc(allocator, sizeof *encb);
    if (!encb)
        return NULL;
    memset(encb, 0, sizeof *encb);
    return encb;
}

#include <limits.h>
#include <stdint.h>
#include "frame_encode.h"

/* Pack residual low bits MSB-first; each chunk independently pads its final byte. */
static int32_t v4i_block_encode_write_low_bits(const uint32_t *residual, int32_t samples,
                                               uint32_t shift, uint8_t *out) {
    if (!shift || samples <= 0)
        return LHDC_V4_NO_OUTPUT_BYTES;
    uint32_t pending = 0, word = 0;
    int32_t bytes = 0;
    uint32_t mask = UINT32_MAX >> ((0u - shift) & 31);
    for (int32_t i = 0; i < samples; ++i) {
        pending += shift;
        word |= (residual[i] & mask) << ((0u - pending) & 31);
        while (pending >= 8) {
            out[bytes++] = (uint8_t)(word >> 24);
            word <<= 8;
            pending -= 8;
        }
    }
    if (pending)
        out[bytes++] = (uint8_t)(word >> 24);
    return bytes;
}

/* Fold residuals and estimate Rice/LZ parameters; use raw-sample mode when compression is unsuitable. */
static bool evaluate_block_compression(lhdc_Encoder_s *encb, int32_t *num_unencoded, int32_t **rd,
                                       int32_t ch0, int32_t ch1, int32_t do_eval) {
    for (int32_t ch = ch0; ch <= ch1; ++ch) {
        if (!do_eval || encb->compression_type[ch] <= 1)
            continue;

        int32_t ns = encb->block_size - num_unencoded[ch];
        uint64_t total = 0;
        uint32_t max_rd2 = 0;
        for (int32_t i = 0; i < ns; ++i) {
            int32_t value = rd[ch][i + num_unencoded[ch]];
            uint32_t folded = ((uint32_t)value << 1) ^ audio_math_sign_mask_i32(value);
            encb->rd2[ch][i] = folded;
            total += folded;
            if (folded > max_rd2)
                max_rd2 = folded;
        }
        uint32_t max_nb = v4i_block_encode_residual_width(max_rd2);
        uint32_t mean_nb =
            v4i_block_encode_residual_width(ns ? (uint32_t)(total / (uint64_t)ns) : 0);
        int32_t max_len = encb->bytes_per_sample * ns * 90 / 100;
        int32_t len_rice = INT_MAX;
        encb->max_nb[ch] = max_nb;
        encb->sh_rice[ch] = 0;
        for (int32_t rice_shift = (int32_t)mean_nb; rice_shift >= 0; --rice_shift) {
            int32_t len = v4f_eval_rice_encode_len(encb->rd2[ch], ns, (uint32_t)rice_shift);
            if (len > 0) {
                if (len >= len_rice)
                    break;
                len_rice = len;
                encb->sh_rice[ch] = (uint32_t)rice_shift;
            }
        }

        int32_t len_lz;
        if (encb->lz_evl_cnt < 11) {
            ++encb->lz_evl_cnt;
            len_lz = max_len;
        } else {
            encb->lz_evl_cnt = 0;
            encb->sh_lz[ch] = 0;
            len_lz = INT_MAX;
            int32_t best_split_shift = (int32_t)max_nb - 24;
            if (best_split_shift < 0)
                best_split_shift = 0;
            for (int32_t rice_shift = (int32_t)max_nb - 2; rice_shift >= best_split_shift;
                 --rice_shift) {
                int32_t low_bit_bytes = (ns * rice_shift + 7) / 8;
                if (low_bit_bytes < len_rice) {
                    int32_t n = v4f_lz_make_data(encb->rd2[ch], ns, rice_shift, (int32_t)max_nb,
                                                 encb->uc[ch]);
                    uint32_t len = v4f_lzh_enc_eval(&encb->lzh_workspace, encb->uc[ch], (uint32_t)n,
                                                    (uint32_t)(len_rice - low_bit_bytes));
                    if (len && len + low_bit_bytes < (uint32_t)len_lz) {
                        encb->sh_lz[ch] = (uint32_t)rice_shift;
                        len_lz = (int32_t)len + low_bit_bytes;
                    }
                }
            }
        }
        if (max_len <= len_rice && max_len <= len_lz) {
            encb->compression_type[ch] = 1;
            if (encb->use_side == 2 - ch)
                encb->use_side = 0;
            return false;
        }
        encb->use_lz[ch] = len_rice >= len_lz;
        if (encb->use_lz[ch]) {
            encb->lz_evl_cnt = 11;
            ++encb->cnt_lz;
        }
    }

    return true;
}

/* Evaluate channel-block compression, write residual/block information, and return output bytes. */
int32_t v4f_block_encode(lhdc_Encoder_s *encb, int32_t *num_unencoded, int32_t **rd, int32_t ch0,
                         int32_t ch1, uint8_t *out, int32_t out_len, int32_t do_eval) {
    int32_t *dat[2];
    int32_t ird2[2] = {0, 0};
    if (!evaluate_block_compression(encb, num_unencoded, rd, ch0, ch1, do_eval))
        return LHDC_V4_NO_OUTPUT_BYTES;
    int32_t len = 0;
    for (int32_t ch = ch0; ch <= ch1; ++ch) {
        dat[ch] = encb->use_side == 2 - ch ? encb->side : encb->dat[ch];
        if (encb->compression_type[ch] == 0) {
            v4f_byte_write_int32(dat[ch][0], encb->bytes_per_sample, out + len);
            len += encb->bytes_per_sample;
        } else if (encb->compression_type[ch] > 1) {
            uint32_t shift = encb->use_lz[ch] ? encb->sh_lz[ch] : encb->sh_rice[ch];
            uint32_t flags = (encb->use_lz[ch] ? 0x40 : 0) | (num_unencoded[ch] > 1 ? 0x80 : 0);
            out[len++] = (uint8_t)(shift | flags);
            if (encb->use_lz[ch])
                out[len++] = (uint8_t)((encb->max_nb[ch] - encb->sh_lz[ch] + 7) >> 3);
            if (encb->lhdc_ver_code > 299 && encb->compression_type[ch] == 15) {
                out[len++] = (uint8_t)(((encb->lpc_result[ch].quantization_shift & 31) |
                                        (encb->lpc_result[ch].order << 5)) -
                                       32);
                for (uint32_t i = 0; i < encb->lpc_result[ch].order; ++i) {
                    v4f_byte_write_int32(encb->lpc_result[ch].coefficients[i], 2, out + len);
                    len += 2;
                }
            }
        }
    }
    for (int32_t ck = 0; ck < encb->chunk_cnt; ++ck) {
        for (int32_t ch = ch0; ch <= ch1; ++ch) {
            if (encb->compression_type[ch] == 1) {
                for (int32_t i = 0; i < encb->chunk_size; ++i) {
                    v4f_byte_write_int32(dat[ch][i], encb->bytes_per_sample, out + len);
                    len += encb->bytes_per_sample;
                }
            } else if (encb->compression_type[ch] > 1) {
                int32_t cs = encb->chunk_size;
                if (!ck) {
                    int32_t bytes = encb->bytes_per_sample + (encb->use_side == 2 - ch);
                    int32_t initial = num_unencoded[ch] > 0 ? num_unencoded[ch] : 0;
                    for (int32_t i = 0; i < initial; ++i) {
                        v4f_byte_write_int32(dat[ch][i], bytes, out + len);
                        len += bytes;
                    }
                    cs -= initial;
                    if (encb->use_lz[ch]) {
                        int32_t n = v4f_lz_make_data(
                            encb->rd2[ch], encb->block_size - num_unencoded[ch],
                            (int32_t)encb->sh_lz[ch], (int32_t)encb->max_nb[ch], encb->uc[0]);
                        len += (int32_t)v4f_lzh_enc(&encb->lzh_workspace, encb->uc[0], (uint32_t)n,
                                                    out + len, (uint32_t)(out_len - len));
                    }
                }
                uint32_t *residual = encb->rd2[ch] + ird2[ch];
                if (encb->use_lz[ch])
                    len +=
                        v4i_block_encode_write_low_bits(residual, cs, encb->sh_lz[ch], out + len);
                else
                    len += v4f_encode_residual_block(residual, cs, encb->sh_rice[ch], out + len,
                                                     out_len - len);
                ird2[ch] += cs;
            }
            dat[ch] += encb->chunk_size;
        }
    }
    return len;
}

/* Write LHDC headers and metadata, reversing metadata bytes within each four-byte group. */
int32_t v4f_out_frame_hdr(struct lhdc_Encoder_s *encb, int32_t down_step, int32_t ch,
                          uint8_t *out) {
    out[0] = 'L';
    out[1] = down_step == 1 ? 0x80 : 0;
    out[2] = 0;
    int32_t header_bytes;
    if (encb->one_frame_per_channle) {
        out[1] |= 0x60;
        out[3] = (uint8_t)((uint32_t)ch | (uint32_t)encb->compression_type[ch] << 4);
        out[4] = (uint8_t)((encb->divisor_ext[ch] & 7) | (uint32_t)encb->divisor[ch] << 3);
        header_bytes = 5;
    } else {
        out[1] |= (uint8_t)((uint32_t)encb->use_side << 5);
        out[3] =
            (uint8_t)((encb->compression_type[1] & 15) | (uint32_t)encb->compression_type[0] << 4);
        for (int32_t i = 0; i < 2; ++i)
            out[4 + i] = (uint8_t)((encb->divisor_ext[i] & 7) | (uint32_t)encb->divisor[i] << 3);
        header_bytes = 6;
    }
    if (encb->lhdc_ver_code <= 399)
        return header_bytes;
    if (!encb->meta_frame_cnt) {
        encb->ext_offset = 0;
        encb->ext_func &= 0xfb;
        if (!encb->ext_func)
            encb->extention_flag = 0;
    }
    if (!encb->extention_flag)
        return header_bytes;
    if (encb->ext_offset && (encb->ext_func & 4)) {
        for (uint32_t i = 0; i < encb->metadata_len; i += 4) {
            uint32_t count = encb->metadata_len - i;
            if (count > 4)
                count = 4;
            for (uint32_t j = 0; j < count; ++j)
                out[header_bytes + 1 + i + count - 1 - j] = encb->metadata[i + j];
        }
    }
    out[1] |= 0x10;
    out[header_bytes] = (uint8_t)(encb->ext_func | (uint32_t)encb->ext_offset << 4);
    return header_bytes + 1 + encb->ext_offset;
}

#include <math.h>
#include "../../../../platform/include/lhdc_print.h"

/* Release underlying LHDC encoder state. */
void v4f_lhdc_FreeEncoder(struct lhdc_Encoder_s *encb) {
    lhdc_memory_free(encb);
}

/* Add/subtract one half by sign and truncate to int32; saturate overflow and zero NaN independently of long width. */
int32_t v4f_lround(float x) {
    return audio_math_truncate_sat_i32_f64((double)x + audio_math_copy_sign_f64(0.5, (double)x));
}

/* Decrement metadata validity frames, leaving zero unchanged. */
void v4f_lhdc_DecMetaFrameCnt(struct lhdc_Encoder_s *encb) {
    if (encb->meta_frame_cnt)
        --encb->meta_frame_cnt;
}

/* Query extension bits for AR, JAS, and META. */
bool v4f_lhdc_GetExtFuncState(struct lhdc_Encoder_s *encb, lhdc_ext_func_t ext_func) {
    uint32_t mask = ext_func == EXT_FUNC_AR     ? 1u
                    : ext_func == EXT_FUNC_JAS  ? 2u
                    : ext_func == EXT_FUNC_META ? 4u
                                                : 0u;
    return (encb->ext_func & mask) != 0;
}

/* Set AR; clear the extension-header flag when disabling the last extension without metadata. */
void v4f_lhdc_ExtFuncArEnable(struct lhdc_Encoder_s *encb, int32_t enable_ar) {
    if (enable_ar) {
        encb->ext_func |= 1;
        encb->extention_flag = 1;
    } else {
        encb->ext_func &= 0xfe;
        if (!encb->ext_func && !encb->ext_offset)
            encb->extention_flag = 0;
    }
}

/* Set JAS while retaining other extensions and existing metadata offsets. */
void v4f_lhdc_ExtFuncJasEnable(struct lhdc_Encoder_s *encb, int32_t enable_Jas) {
    if (enable_Jas) {
        encb->ext_func |= 2;
        encb->extention_flag = 1;
    } else {
        encb->ext_func &= 0xfd;
        if (!encb->ext_func && !encb->ext_offset)
            encb->extention_flag = 0;
    }
}

/* Set up to eight metadata bytes; disabling clears length but retains stored contents. */
void v4f_lhdc_ExtFuncMetaEnable(struct lhdc_Encoder_s *encb, int32_t enable_meta,
                                uint8_t *pmeta_data, int32_t meta_data_len,
                                int32_t conti_frame_cnt) {
    if (!enable_meta || meta_data_len < 1) {
        encb->ext_offset = 0;
        encb->metadata_len = 0;
        encb->meta_frame_cnt = 0;
        encb->ext_func &= 0xfb;
        if (!encb->ext_func)
            encb->extention_flag = 0;
        return;
    }
    if (meta_data_len > 8)
        meta_data_len = 8;
    encb->ext_func |= 4;
    encb->extention_flag = 1;
    encb->metadata_len = (uint8_t)meta_data_len;
    for (int32_t i = 0; i < meta_data_len; ++i)
        encb->metadata[i] = pmeta_data[i];
    encb->ext_offset = (uint8_t)meta_data_len;
    encb->meta_frame_cnt = (uint32_t)conti_frame_cnt;
}
#include "../include/codec/scalar.h"
#include "../include/codec/v4_pcm.h"

/* Estimate V4 Rice residual bit count. */
int32_t v4f_eval_rice_encode_len(uint32_t *rd2, int32_t ns, uint32_t nb) {
    return v4_eval_rice_encode_len(rd2, ns, nb);
}

/* Write a big-endian integer using the specified byte count. */
void v4f_byte_write_int32(int32_t dat, int32_t nb, uint8_t *out) {
    v4_byte_write_int32(dat, nb, out);
}

/* Quantize samples by divisor and sample stride. */
void v4f_applyQuantization(struct lhdc_Encoder_s *encb, int32_t *src, int32_t *dst, int32_t div,
                           int32_t down_step) {
    v4_applyQuantization(encb, src, dst, div, down_step);
}

/* Write residual bytes at the specified effective width for LZ compression. */
int32_t v4f_lz_make_data(uint32_t *rd2, int32_t ns, int32_t sh, int32_t nb, uint8_t *uc) {
    return v4_lz_make_data(rd2, ns, sh, nb, uc);
}

/* Encode Rice residual blocks with V4 escape rules. */
int32_t v4f_encode_residual_block(uint32_t *rd2, int32_t ns, uint32_t nb, uint8_t *out,
                                  int32_t out_len) {
    return v4_encode_residual_block(rd2, ns, nb, out, out_len);
}

/* Evaluate stereo difference prediction; disable it for independently framed channels. */
static void evaluate_side_channel(lhdc_Encoder_s *encb, int32_t **samples, const int32_t div[2],
                                  const unsigned long long min_total[2], int32_t down_step) {
    if (encb->one_frame_per_channle) {
        encb->use_side = 0;
        return;
    }
    int32_t replace_side_channel = div[0] < div[1];
    int32_t preserved_channel = 1 - replace_side_channel;
    int32_t side_min = 0, side_max = 0;
    v4f_applyQuantization(encb, samples[replace_side_channel], encb->side, div[preserved_channel],
                          down_step);
    for (int32_t n = 0; n < encb->block_size; ++n) {
        int32_t difference =
            replace_side_channel
                ? audio_math_subtract_wrap_i32(encb->side[n], encb->dat[preserved_channel][n])
                : audio_math_subtract_wrap_i32(encb->dat[preserved_channel][n], encb->side[n]);
        encb->side[n] = difference;
        if (encb->lhdc_ver_code > 299) {
            if (difference < side_min)
                side_min = difference;
            if (difference > side_max)
                side_max = difference;
        }
    }
    if (encb->lhdc_ver_code > 299 && (div[0] > 1 || div[1] > 1) && side_max < 5 && side_min > -5 &&
        encb->block_size > 0)
        memset(encb->side, 0, (size_t)encb->block_size * sizeof *encb->side);
    unsigned long long side_min_total;
    int32_t side_num_unencoded;
    int32_t side_ctype = v4f_CalcResidual(encb, encb->side, encb->side_residual, &side_min_total,
                                          &side_num_unencoded, &encb->side_lpc_result);
    if (min_total[preserved_channel] + side_min_total * 100 / 100 >= min_total[0] + min_total[1]) {
        encb->use_side = 0;
        return;
    }
    encb->use_side = replace_side_channel ? 1 : 2;
    encb->compression_type[replace_side_channel] = side_ctype;
    encb->num_unencoded[replace_side_channel] = side_num_unencoded;
    encb->rd[replace_side_channel] =
        side_ctype > 2 ? encb->side_residual[side_ctype - 2] : encb->side;
    encb->divisor[replace_side_channel] = encb->divisor[preserved_channel];
    encb->divisor_ext[replace_side_channel] = encb->divisor_ext[preserved_channel];
    v4f_applyQuantization(encb, samples[replace_side_channel], encb->dat[replace_side_channel],
                          div[preserved_channel], down_step);
    if (encb->lhdc_ver_code > 299) {
        encb->lpc_result[replace_side_channel].order = encb->side_lpc_result.order;
        encb->lpc_result[replace_side_channel].quantization_shift =
            encb->side_lpc_result.quantization_shift;
        encb->lpc_result[replace_side_channel].bits_needed = encb->side_lpc_result.bits_needed;
        for (uint32_t n = 0; n < encb->side_lpc_result.order; ++n)
            encb->lpc_result[replace_side_channel].coefficients[n] =
                encb->side_lpc_result.coefficients[n];
    }
}

/* Quantize, select fixed/LPC prediction and stereo differences, and write a complete aligned LHDC frame. */
int32_t v4f_lhdc_Encode(lhdc_Encoder_s *encb, int32_t **samples, int32_t *btr, int32_t *btr_ext,
                        int32_t down_step, uint8_t *out, int32_t out_len, int32_t do_eval,
                        int32_t min_len) {
    encb->block_size = down_step ? encb->block_size / down_step : 0;
    encb->chunk_size = down_step ? encb->chunk_size / down_step : 0;
    if (do_eval) {
        int32_t div[2];
        unsigned long long min_total[2];
        for (int32_t ch = 0; ch < encb->channel; ++ch) {
            encb->divisor[ch] = btr[ch];
            encb->divisor_ext[ch] = btr_ext[ch];
            div[ch] = v4f_BTRtoDIV(btr[ch], btr_ext[ch]);
            v4f_applyQuantization(encb, samples[ch], encb->dat[ch], div[ch], down_step);
            int32_t compression_type =
                v4f_CalcResidual(encb, encb->dat[ch], encb->residual[ch], &min_total[ch],
                                 &encb->num_unencoded[ch], &encb->lpc_result[ch]);
            encb->compression_type[ch] = compression_type;
            encb->rd[ch] =
                compression_type > 2 ? encb->residual[ch][compression_type - 2] : encb->dat[ch];
        }
        evaluate_side_channel(encb, samples, div, min_total, down_step);
        ++encb->cnt_side[encb->use_side];
    }

    if (encb->one_frame_per_channle)
        min_len /= 2;
    int32_t payload_bytes = 0;
    int32_t frames = encb->one_frame_per_channle ? 2 : 1;
    for (int32_t ch = 0; ch < frames; ++ch) {
        int32_t header, payload;
        do {
            header = v4f_out_frame_hdr(encb, down_step, ch, out + payload_bytes);
            int32_t first = encb->one_frame_per_channle ? ch : 0;
            int32_t last = encb->one_frame_per_channle ? ch : 1;
            payload = v4f_block_encode(encb, encb->num_unencoded, encb->rd, first, last,
                                       out + payload_bytes + header,
                                       out_len - payload_bytes - header, do_eval);
            if (payload < 0) {
                encb->block_size *= down_step;
                encb->chunk_size *= down_step;
                return LHDC_V4_STATUS_ERROR;
            }
        } while (!payload);
        int32_t length = header + payload;
        int32_t padded_frame_bytes = length > min_len ? length : min_len;
        padded_frame_bytes = (padded_frame_bytes + 3) & ~3;
        if (!do_eval && length < padded_frame_bytes)
            memset(out + payload_bytes + length, 0, (size_t)(padded_frame_bytes - length));
        out[payload_bytes + 2] = (uint8_t)padded_frame_bytes;
        out[payload_bytes + 1] |= (uint8_t)((uint32_t)(padded_frame_bytes >> 2) >> 6);
        payload_bytes += padded_frame_bytes;
    }
    ++encb->cnt_encode_type[0][encb->compression_type[0]];
    ++encb->cnt_encode_type[1][encb->compression_type[1]];
    encb->block_size *= down_step;
    encb->chunk_size *= down_step;
    if (do_eval)
        ++encb->frames;
    return payload_bytes;
}

/* Initialize LHDC frame parameters; V3 onward also generates a half-ratio Tukey window for LPC. */
int32_t v4f_lhdc_InitEncoder(struct lhdc_Encoder_s *encb, int32_t bits_per_sample,
                             int32_t block_size, int32_t one_frame_per_channle,
                             int32_t lhdc_ver_code) {
    uint32_t min = UINT32_MAX << (((uint32_t)bits_per_sample - 1u) & 31);
    encb->bits_per_sample = bits_per_sample;
    encb->bytes_per_sample = bits_per_sample / 8;
    encb->channel = 2;
    encb->block_size = block_size;
    encb->chunk_size = 64;
    encb->chunk_cnt = block_size / 64;
    encb->one_frame_per_channle = one_frame_per_channle;
    encb->min = (int32_t)min;
    encb->max = (int32_t)~min;
    encb->lhdc_ver_code = lhdc_ver_code;
    if (lhdc_ver_code >= 300)
        v4f_lpcV_Wintu(encb->pfLpcVWindow, block_size, 0.5f);
    return LHDC_V4_STATUS_OK;
}

/* Compare fixed-order difference prediction with V3/V4 variable-quantization LPC to select residual coding. */
int32_t v4f_CalcResidual(lhdc_Encoder_s *encb, int32_t *dat,
                         int32_t (*rd)[LHDC_V4_MAX_BLOCK_SAMPLES], unsigned long long *min_total,
                         int32_t *num_unencoded, LhdcV4LpcResult *lpc) {
    /* Record first-sample costs while forming differences, then reuse storage for clipped costs per order. */
    uint64_t total[14] = {0}, stage_totals[14] = {0};
    int32_t ns = encb->block_size;
    for (int32_t r = 0; r < 14; ++r)
        rd[r][0] = 0;
    for (int32_t i = 1; i < ns; ++i) {
        rd[1][i] = audio_math_subtract_wrap_i32(dat[i], dat[i - 1]);
        total[0] += v4i_prediction_folded_magnitude(dat[i]);
        total[1] += v4i_prediction_folded_magnitude(rd[1][i]);
    }
    uint32_t max_lpc = encb->lhdc_ver_code < 300 ? 14 : 13;
    for (uint32_t r = 2; r < max_lpc; ++r) {
        for (int32_t i = 1; i < ns; ++i) {
            rd[r][i] = audio_math_subtract_wrap_i32(rd[r - 1][i], rd[r - 1][i - 1]);
            uint32_t magnitude = v4i_prediction_folded_magnitude(rd[r][i]);
            total[r] += magnitude;
            if ((uint32_t)i < r)
                stage_totals[r] += magnitude;
        }
        if (total[r - 1] - stage_totals[r - 1] < total[r] - stage_totals[r]) {
            max_lpc = r;
            break;
        }
    }
    *num_unencoded = 0;
    if (!total[1]) {
        int32_t all_zero = 1;
        for (int32_t i = 0; i < ns; ++i)
            if (rd[1][i]) {
                all_zero = 0;
                break;
            }
        if (all_zero) {
            *min_total = 0;
            return LHDC_V4_RESIDUAL_CONSTANT_SIGNAL;
        }
    }
    uint32_t min = 0, min_unencode_first = 0;
    /* First-sample costs are no longer needed; every active entry is assigned before minimum comparison. */
    stage_totals[0] = total[0];
    for (uint32_t r = 1; r < max_lpc; ++r) {
        if (total[r] < total[min])
            min = r;
        stage_totals[r] = total[r];
        for (uint32_t i = 1; i < r; ++i)
            stage_totals[r] -= v4i_prediction_folded_magnitude(rd[r][i]);
        if (stage_totals[r] < stage_totals[min_unencode_first])
            min_unencode_first = r;
    }
    if (min_unencode_first && total[min_unencode_first] - stage_totals[min_unencode_first] >
                                  stage_totals[min_unencode_first]) {
        min = min_unencode_first;
        *num_unencoded = (int32_t)min;
        *min_total = stage_totals[min];
    } else {
        *num_unencoded = min > 0;
        *min_total = total[min];
    }
    if (encb->lhdc_ver_code >= 300) {
        unsigned long long lpcV_min_total;
        int32_t lpcV_num_unencoded;
        lpc->bits_needed =
            v4f_lpcV_Cares(encb, dat, rd[13], &lpcV_min_total, &lpcV_num_unencoded, lpc);
        if (lpc->bits_needed != LHDC_V4_LPC_EVALUATION_FAILED_BITS &&
            lpcV_min_total <= UINT64_MAX - 600 && lpcV_min_total + 600 < *min_total) {
            *min_total = lpcV_min_total;
            min = 13;
            *num_unencoded = lpcV_num_unencoded;
        }
    }
    return (int32_t)min + 2;
}

#include "../include/codec/scalar.h"

/* Estimate V4 Rice bytes, preserving escape quotients and integer wraparound. */

int32_t v4_eval_rice_encode_len(uint32_t *rd2, int32_t ns, uint32_t nb) {
    uint32_t len = 0;
    for (int32_t i = 0; i < ns; ++i) {
        uint32_t q = v4_shift_right_compat32(rd2[i], nb);
        int32_t escaped = 0;
        while (q > 9u) {
            len += 16u;
            if (q - 10u < 63u) {
                escaped = 1;
                break;
            }
            q -= 73u;
        }
        if (!escaped)
            len += q + 1u;

        len += nb;
    }
    if (ns < 1)
        return LHDC_V4_NO_OUTPUT_BYTES;
    /* Round up to bytes using signed-integer rules; addition may wrap at 32 bits. */
    return (int32_t)(len + 7u) / 8;
}

/* Write low integer bytes big-endian; nb selects one to four bytes. */

void v4_byte_write_int32(int32_t dat, int32_t nb, uint8_t *out) {
    if (nb >= 4)
        *out++ = (uint8_t)((uint32_t)dat >> 24);
    if (nb >= 3)
        *out++ = (uint8_t)((uint32_t)dat >> 16);
    if (nb >= 2)
        *out++ = (uint8_t)((uint32_t)dat >> 8);
    *out = (uint8_t)dat;
}

/* Flush complete bytes from the bit buffer and update remaining bits/content. */

uint8_t *v4_MoveOutBytes(V4BitWrite *bw, uint8_t *out) {
    while (bw->rb > 7u) {
        *out++ = (uint8_t)(bw->db.u32 >> 24);
        bw->db.u32 <<= 8;
        bw->rb -= 8u;
    }
    return out;
}
#include "../include/codec/v4_pcm.h"

/* Reduce PCM dynamic range by quantization divisor and downsampling stride. */
void v4_applyQuantization(struct lhdc_Encoder_s *encb, int32_t *src, int32_t *dst, int32_t div,
                          int32_t down_step) {
    int32_t n = encb->block_size;
    /* Callers must ensure div > 0 and src contains n*down_step samples. */
    for (int32_t i = 0; i < n; ++i)
        dst[i] = src[(size_t)i * (uint32_t)down_step] / div;
}
#include "../include/codec/v4_pcm.h"
#include "../include/codec/scalar.h"

/* Pack LZ input using the specified shift and byte width. */
int32_t v4_lz_make_data(uint32_t *rd2, int32_t ns, int32_t sh, int32_t nb, uint8_t *uc) {
    int32_t width = (nb - sh + 7) / 8;
    if (width < 1)
        width = 1;
    if (width > 4)
        width = 4;
    for (int32_t i = 0; i < ns; ++i)
        v4_byte_write_int32((int32_t)(rd2[i] >> (uint32_t)sh), width,
                            uc + (size_t)i * (uint32_t)width);
    return ns > 0 ? ns * width : LHDC_V4_NO_OUTPUT_BYTES;
}

/* Write a bit field and propagate error status. */
static void v4i_residual_output_append(V4BitWrite *bw, uint32_t value, uint32_t n) {
    bw->db.u32 |= value << (32 - bw->rb - n);
    bw->rb += n;
}

/* Write V4 Rice residual blocks, preserving partial writes before capacity checks. */
int32_t v4_encode_residual_block(uint32_t *rd2, int32_t ns, uint32_t nb, uint8_t *out,
                                 int32_t out_len) {
    /* Native failure behavior includes partial writes before capacity checks.
     * Test allocation is larger than logical out_len to retain and inspect them.
     * Tested nb is 0..24 so appends fit the native 32-bit accumulator. */
    uint8_t *cursor = out, *end = out + out_len;
    V4BitWrite bw = {0, {0}};
    uint32_t mask = nb ? UINT32_MAX >> (32 - nb) : 0;
    for (int32_t i = 0; i < ns; ++i) {
        uint32_t q = rd2[i] >> nb;
        int32_t escaped = 0;
        while (q > 9) {
            if (q - 10 < 63) {
                v4i_residual_output_append(&bw, q - 10, 16);
                cursor = v4_MoveOutBytes(&bw, cursor);
                escaped = 1;
                break;
            }
            v4i_residual_output_append(&bw, 63, 16);
            cursor = v4_MoveOutBytes(&bw, cursor);
            q -= 73;
            if (end <= cursor)
                return LHDC_V4_STATUS_ERROR;
        }
        if (!escaped) {
            v4i_residual_output_append(&bw, 1, q + 1);
            cursor = v4_MoveOutBytes(&bw, cursor);
        }

        if (end < cursor + (bw.rb + nb + 7) / 8)
            return LHDC_V4_STATUS_ERROR;
        if (nb) {
            v4i_residual_output_append(&bw, rd2[i] & mask, nb);
            cursor = v4_MoveOutBytes(&bw, cursor);
            if (end <= cursor)
                return LHDC_V4_STATUS_ERROR;
        }
    }
    if (bw.rb)
        *cursor++ = (uint8_t)(bw.db.u32 >> 24);
    return (int32_t)(cursor - out);
}
