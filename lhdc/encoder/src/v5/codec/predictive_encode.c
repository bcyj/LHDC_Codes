#include "../../../../common/include/compat/v5/lhdc_v5_constants.h"
#include "../include/codec/bit_operations.h"
#include "../include/codec/fast_math.h"
#include <audio_math/integer.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/v5_extended.h"
#include "../include/codec/v5_pcm.h"
#include <string.h>

static const uint32_t v5i_bpc_first_plane_frequencies[3] = {54, 12, 1},
                      v5i_bpc_tail_plane_frequencies[3] = {90, 16, 1};

/* Estimate arithmetic coding cost for bit-plane symbols. */
int32_t v5f_bpc_ac_eval_mv(const LhdcEncContext *ecb, int32_t qn, int32_t epn) {
    uint8_t *uc = ecb->lossy_uc;
    int32_t ma0 = ecb->band_cfg->ac_ma0, ma1 = ecb->band_cfg->ac_ma1;
    int32_t first_plane_bits = v5f_FacEncEvalMA(uc, qn, v5i_bpc_first_plane_frequencies, 3, ma0),
            remaining_plane_bits =
                v5f_FacEncEvalMA(uc + qn, epn - qn, v5i_bpc_tail_plane_frequencies, 3, ma1);
    int32_t total_bits = audio_math_add_wrap_i32(first_plane_bits, remaining_plane_bits);
    return audio_math_shift_right_i32(audio_math_multiply_wrap_i32(total_bits, ecb->eval_cp), 16);
}

/* Output predictively compressed bit-plane symbols through FAC. */
int32_t v5f_BitsPredictCompressOutput(LhdcEncContext *ecb, fac_enc *ptr_fenc_s, BitWriter *ptr_bw) {
    bpc_buf_s *bpc = &ecb->ed.enc.lossy.bpc;
    v5f_FacEncInit(ptr_fenc_s, (uint8_t *)bpc->q, LHDC_V5_BPC_OUTPUT_BYTES);
    v5f_FacEncSetMA(ptr_fenc_s, v5i_bpc_first_plane_frequencies, 3, ecb->band_cfg->ac_ma0);
    int32_t err = 0, i = 0;
    for (; i < bpc->nzc; ++i) {
        err = v5f_FacEncodeSymbolMA(ptr_fenc_s, ecb->lossy_uc[i]);
        if (err) {
            ++i;
            break;
        }
    }
    v5f_FacEncSetMA(ptr_fenc_s, v5i_bpc_tail_plane_frequencies, 3, ecb->band_cfg->ac_ma1);
    if (!err)
        for (; i < bpc->qenc_len; ++i) {
            err = v5f_FacEncodeSymbolMA(ptr_fenc_s, ecb->lossy_uc[i]);
            if (err)
                break;
        }
    bpc->sp_enc_bits = v5f_FacEncFlush(ptr_fenc_s, ptr_bw) << 3;
    return err;
}

/* Analyze quantized coefficients and estimate bit-plane compression cost. */
int32_t v5f_BitsPredictCompressEval(LhdcEncContext *ecb) {
    bpc_buf_s *bpc = &ecb->ed.enc.lossy.bpc;
    int32_t coefficient_count = bpc->nzc;
    for (int32_t i = 0; i < coefficient_count; ++i)
        bpc->bna[i + 1] =
            v5_calc_bits_inline((uint32_t)bpc->rd[ecb->frm_size - coefficient_count + i]) << 7;
    int32_t init =
        ecb->init_bn_bits ? (bpc->bna[1] < ecb->max_init_bn ? bpc->bna[1] : ecb->max_init_bn) : 0;
    bpc->q[0] = (uint32_t)init;
    bpc->rb[0] = init;
    bpc->bna[0] = init;
    int32_t d3 = 0, d7 = 0, short_predictor = init, long_predictor = init;
    for (int32_t i = 1; i < coefficient_count; ++i) {
        long_predictor = audio_math_shift_right_i32(
            audio_math_add_wrap_i32(audio_math_multiply_wrap_i32(long_predictor, 7), bpc->bna[i]),
            3);
        bpc->q[i] = (uint32_t)long_predictor;
        if (ecb->enc_frm_len < 200) {
            short_predictor = audio_math_shift_right_i32(
                audio_math_add_wrap_i32(audio_math_multiply_wrap_i32(short_predictor, 3),
                                        audio_math_multiply_wrap_i32(bpc->bna[i], 5)),
                3);
            bpc->rb[i] = short_predictor;
            int32_t short_prediction_error =
                audio_math_subtract_wrap_i32(short_predictor, bpc->bna[i + 1]);
            int32_t long_prediction_error =
                audio_math_subtract_wrap_i32(long_predictor, bpc->bna[i + 1]);
            d3 = audio_math_add_wrap_i32(
                d3, audio_math_int32_from_bits((uint32_t)short_prediction_error ^
                                               audio_math_sign_mask_i32(short_prediction_error)));
            d7 = audio_math_add_wrap_i32(
                d7, audio_math_int32_from_bits((uint32_t)long_prediction_error ^
                                               audio_math_sign_mask_i32(long_prediction_error)));
        }
    }
    bpc->alpha = ecb->enc_frm_len < 200 && coefficient_count > 1 && d3 < d7 ? 0 : 1;
    int32_t sum = 0;
    for (int32_t i = 0; i < coefficient_count; ++i) {
        int32_t smooth = bpc->alpha ? audio_math_int32_from_bits(bpc->q[i]) : bpc->rb[i];
        int32_t bits = audio_math_shift_right_i32(audio_math_add_wrap_i32(smooth, 64), 7);
        bpc->q[i] = (uint32_t)bpc->rd[ecb->frm_size - coefficient_count + i] >> (bits & 31);
        bpc->rb[i] = bits;
        sum = audio_math_add_wrap_i32(sum, bits);
    }
    int32_t epn = v5f_RiceQuotientEncode(bpc->q, coefficient_count, ecb->band_cfg->rice_min_p0,
                                         ecb->lossy_uc);
    bpc->qenc_len = epn;
    if (epn < 0)
        return ecb->ava_bits + 1;
    bpc->sp_eval_bits = v5f_bpc_ac_eval_mv(ecb, coefficient_count, epn);
    return audio_math_add_wrap_i32(audio_math_add_wrap_i32(bpc->sp_eval_bits, sum), bpc->sbn);
}

/* Generate spectral quantization-gain lookup tables. */
void v5f_GenerateGainTable(LhdcEncContext *state, int32_t enc_len, int32_t frame_duration) {
    v5_GenerateGainTable(enc_len, state->bits_per_sample, state->sample_rate, frame_duration,
                         state->gain_tab, &state->gain_tab_base, &state->gain_tab_step);
}

/* Look up or extrapolate quantization gain by index. */
float v5f_EncGainByTable(int32_t gi, float *gtab, int32_t base, float gstep) {
    return v5_EncGainByTable(gi, (void *)gtab, base, gstep);
}

/* Encode Rice quotients as bit-plane symbols. */
int32_t v5f_RiceQuotientEncode(uint32_t *q, int32_t qn, int32_t min_p0, uint8_t *qexp) {
    return v5_RiceQuotientEncode((void *)q, qn, min_p0, (void *)qexp);
}

/* Encode Rice quotients as bit-plane symbols. */
int32_t v5_RiceQuotientEncode(uint32_t *q, int32_t qn, int32_t min_p0, uint8_t *qexp) {
    int32_t max_pi = qn >= min_p0 ? qn - min_p0 : 0, pi = 0;
    if (qn <= 0)
        return LHDC_V5_NO_OUTPUT_BYTES;
    for (int32_t i = 0; i < qn; ++i) {
        if (pi <= max_pi && q[i] > 1) {
            qexp[i] = 2;
            pi = i;
        } else
            qexp[i] = (uint8_t)(q[i] & 1);
    }
    if (pi < max_pi)
        pi = qn;
    int32_t epn = qn;
    for (int32_t i = 0; i < qn; ++i) {
        uint32_t qd;
        if (i > pi)
            qd = q[i] >> 1;
        else {
            if (q[i] <= 1)
                continue;
            qd = q[i] - 2;
        }
        if (qd) {
            int32_t escape = qd > 3;
            uint32_t tail = qd;
            while (tail > 3) {
                if (epn > LHDC_V5_BPC_LAST_TRIPLE_START)
                    return LHDC_V5_STATUS_ERROR;
                qexp[epn++] = 2;
                qexp[epn++] = (uint8_t)((tail >> 1) & 1);
                qexp[epn++] = (uint8_t)(tail & 1);
                tail >>= 2;
            }
            tail -= (uint32_t)escape;
            if ((uint32_t)epn + tail > LHDC_V5_BPC_LAST_OUTPUT_INDEX)
                return LHDC_V5_STATUS_ERROR;
            for (uint32_t j = 0; j < tail; ++j)
                qexp[epn++] = 1;
            if (tail == 3) {
                if (epn - 3 > LHDC_V5_BPC_LAST_TRIPLE_START)
                    return LHDC_V5_STATUS_ERROR;
                continue;
            }
        }
        if (epn > LHDC_V5_BPC_LAST_OUTPUT_INDEX)
            return LHDC_V5_STATUS_ERROR;
        qexp[epn++] = 0;
    }
    return epn;
}

/* Generate spectral quantization-gain lookup tables. */
void v5_GenerateGainTable(int32_t enc_len, int32_t bits_per_sample, int32_t sample_rate,
                          int32_t frm_duration, float *gtab, int32_t *base, float *gstep) {
    *base = 1;
    memset(gtab, 0, LHDC_V5_GAIN_TABLE_SIZE * sizeof(float));
    float max = LHDC_V5_GAIN_MAX_EXPONENT;
    if (frm_duration == LHDC_V5_FRAME_DURATION_10MS || frm_duration == LHDC_V5_FRAME_DURATION_5MS) {
        if (bits_per_sample == 24) {
            if (sample_rate == LHDC_V5_SAMPLE_RATE_44100 ||
                sample_rate == LHDC_V5_SAMPLE_RATE_48000)
                max = (float)(enc_len - 50) * -LHDC_V5_GAIN_SLOPE_24BIT_48K + 26.2f;
            else if (sample_rate == LHDC_V5_SAMPLE_RATE_96000)
                max = (float)(enc_len - 50) * -LHDC_V5_GAIN_SLOPE_24BIT_96K + 25.7f;
            else if (sample_rate == LHDC_V5_SAMPLE_RATE_192000)
                max = (float)(enc_len - 50) * -LHDC_V5_GAIN_SLOPE_24BIT_192K + 25.7f;
                /* Win32 kernel builds retain extended precision until truncation; SDK builds retain float expressions. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            *base = audio_math_truncate_i32_f64((double)max * 0.25);
#else
            *base = audio_math_truncate_i32_f32(max * 0.25f);
#endif
        } else if (bits_per_sample == 16) {
            if (sample_rate == LHDC_V5_SAMPLE_RATE_48000)
                max = (float)(enc_len - 50) * -LHDC_V5_GAIN_SLOPE_16BIT_48K + 26.18f;
            else if (sample_rate == LHDC_V5_SAMPLE_RATE_44100)
                max = (float)(enc_len - 50) * -LHDC_V5_GAIN_SLOPE_16BIT_44K + 26.15f;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            *base = audio_math_truncate_i32_f64((double)max / 2.5);
#else
            *base = audio_math_truncate_i32_f32(max / 2.5f);
#endif
        }
    }
    *gstep = (max - (float)*base) / (float)(LHDC_V5_GAIN_LINEAR_LAST_INDEX - *base);
}

/* Look up or extrapolate quantization gain by index. */
float v5_EncGainByTable(int32_t gi, float *gtab, int32_t base, float gstep) {
    float gain = gtab[gi];
    if (gain != 0.0f) {
        return gain;
    }
    if (gi <= base)
        gain = (float)(int32_t)(1u << ((uint32_t)gi & 31));
    else {
        float p;
        if (gi < LHDC_V5_GAIN_EXTRAPOLATION_INDEX)
            p = (float)(gi - base) * gstep + (float)base;
        else {
            p = (float)(LHDC_V5_GAIN_EXTRAPOLATION_INDEX - base) * gstep + (float)base +
                (float)(gi - LHDC_V5_GAIN_EXTRAPOLATION_INDEX);
            if (p > LHDC_V5_GAIN_MAX_EXPONENT)
                p = LHDC_V5_GAIN_MAX_EXPONENT;
        }
        /* Ordered comparisons also exclude NaN; validate the normal quantization-gain domain once here. */
        gain = p < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32 ? v5_fast_pow2_bounded_inline(p)
                                                    : v5_fast_pow2_inline(p);
    }
    gain = 1.0f / gain;
    gtab[gi] = gain;
    return gain;
}
