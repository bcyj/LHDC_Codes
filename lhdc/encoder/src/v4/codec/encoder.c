#include "lhdc_memory.h"
#include <audio_math/math.h>
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "../../../../common/include/lhdc_log.h"
#include <audio_math/constants.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                 \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_sqrt.h>
#include "../include/encoder_internal.h"
#include "../include/codec/integer_ops.h"
#include "../include/codec/codec_internal.h"
#include "encoder_tables.h"

/* Extension state is supported only by the V4 frame format. */
bool v4f_LhdcGetExtFuncState(FFT_BLOCK *fb, lhdc_ext_func_t ext_func) {
    return fb->lhdc_ver_code > 399 && v4f_lhdc_GetExtFuncState(fb->encb, ext_func);
}

/* AR extension switch for the V4 frame format. */
void v4f_LhdcExtFuncArEnable(FFT_BLOCK *fb, int32_t enable_ar) {
    if (fb->lhdc_ver_code > 399)
        v4f_lhdc_ExtFuncArEnable(fb->encb, enable_ar);
}

/* JAS extension switch for the V4 frame format. */
void v4f_LhdcExtFuncJasEnable(FFT_BLOCK *fb, int32_t enable_Jas) {
    if (fb->lhdc_ver_code > 399)
        v4f_lhdc_ExtFuncJasEnable(fb->encb, enable_Jas);
}

/* Metadata extension switch for the V4 frame format. */
void v4f_LhdcExtFuncMetaEnable(FFT_BLOCK *fb, int32_t enable_meta, uint8_t *pmeta_data,
                               int32_t meta_data_len, int32_t conti_frame_cnt) {
    if (fb->lhdc_ver_code > 399)
        v4f_lhdc_ExtFuncMetaEnable(fb->encb, enable_meta, pmeta_data, meta_data_len,
                                   conti_frame_cnt);
}
#include "../include/codec/scalar.h"
#include "../include/codec/v4_pcm.h"

/* Convert bitrate parameters to a quantization divisor. */
int32_t v4f_BTRtoDIV(int32_t btr, int32_t btr_ext) {
    return v4_BTRtoDIV(btr, btr_ext);
}

/* Convert a quantization divisor back to base/extension bitrate parameters. */
void v4f_DIVtoBTR(int32_t div, int32_t *btr, int32_t *btr_ext) {
    v4_DIVtoBTR(div, btr, btr_ext);
}

/* Split stereo 16-bit PCM into integer and floating input buffers. */
int32_t v4f_Get2ChWavData16(FFT_BLOCK *fb, uint8_t *wav, int32_t ns) {
    return v4_Get2ChWavData16(fb, wav, ns);
}

/* Split packed stereo 24-bit PCM and populate input buffers. */
int32_t v4f_Get2ChWavData24(FFT_BLOCK *fb, uint8_t *wav, int32_t ns) {
    return v4_Get2ChWavData24(fb, wav, ns);
}

#include <string.h>

/* Encode complete buffered blocks continuously; final is handled by the caller and unused here. */
int32_t v4f_EncodeBlocks(FFT_BLOCK *fb, int32_t ns, int32_t final, int32_t *btr, int32_t *btr_ext,
                         uint8_t *enc_buf, int32_t out_len, int32_t do_eval) {
    (void) final;
    int32_t *pcm[2] = {fb->pcm[0], fb->pcm[1]};
    int32_t bytes = 0;
    while (ns > fb->enc_block_size) {
        int32_t n = v4f_lhdc_Encode(fb->encb, pcm, btr, btr_ext, fb->down_step, enc_buf + bytes,
                                    out_len - bytes, do_eval, fb->min_block_len);
        if (n < 0)
            return LHDC_V4_STATUS_ERROR;
        bytes += n;
        ns -= fb->enc_block_size;
        pcm[0] += fb->enc_block_size;
        pcm[1] += fb->enc_block_size;
    }
    int32_t n = v4f_lhdc_Encode(fb->encb, pcm, btr, btr_ext, fb->down_step, enc_buf + bytes,
                                out_len - bytes, do_eval, fb->min_block_len);
    return n < 0 ? LHDC_V4_STATUS_ERROR : bytes + n;
}

#include <math.h>
#include "../../../../platform/include/lhdc_print.h"
#include <stdlib.h>

/* Create analysis state, FFT configuration, and residual state in host storage; fully unwind on failure. */
FFT_BLOCK *v4f_LossyEncoderNew_with_allocator(int32_t version, const LhdcAllocator *allocator) {
    FFT_BLOCK *fb = lhdc_memory_alloc(allocator, sizeof *fb);
    if (!fb)
        return NULL;
    v4f_kiss_fftr_alloc(LHDC_V4_ANALYSIS_FFT_MAX_POINTS, 0, NULL, &fb->cfg_len);
    fb->cfg = lhdc_memory_alloc(allocator, fb->cfg_len);
    if (fb->cfg)
        v4f_kiss_fftr_alloc(LHDC_V4_ANALYSIS_FFT_MAX_POINTS, 0, fb->cfg, &fb->cfg_len);
    if (!fb->cfg) {
        lhdc_memory_free(fb);
        return NULL;
    }
    fb->encb = v4f_lhdc_NewEncoder_with_allocator(allocator);
    if (!fb->encb) {
        lhdc_memory_free(fb->cfg);
        lhdc_memory_free(fb);
        return NULL;
    }
    fb->lhdc_ver_code = version == 3   ? LHDC_V4_STREAM_VERSION_CODE_400
                        : version == 2 ? LHDC_V4_STREAM_VERSION_CODE_300
                                       : LHDC_V4_STREAM_VERSION_CODE_200;
    fb->pcm[0] = fb->g_pcm[0];
    fb->pcm[1] = fb->g_pcm[1];
    v4f_RestoreDefQualityPreset(fb);
    return fb;
}

/* Release FFT configuration, lossless encoder, and spectral workspace; null handles return -1. */
int32_t v4f_LossyEncoderDelete(FFT_BLOCK *fb) {
    if (!fb)
        return LHDC_V4_STATUS_ERROR;
    lhdc_memory_free(fb->cfg);
    if (fb->encb)
        v4f_lhdc_FreeEncoder(fb->encb);
    lhdc_memory_free(fb);
    return LHDC_V4_STATUS_OK;
}

/* Configure frame length, FFT, quality control, and filter history; fast_mode and lhdc_ver are unused. */
void v4f_LossyEncoderInit(FFT_BLOCK *fb, int32_t sample_rate, int32_t bits_per_sample,
                          int32_t channels, int32_t block_size, int32_t sink_buf_len,
                          int32_t target_byte_rate, int32_t fast_mode,
                          int32_t one_frame_per_channel, int32_t need_min_byte_rate,
                          int32_t process_size, int32_t target_mtu_byte,
                          int32_t no_btr_limit_frame_cnt, int32_t min_bits_reserved,
                          int32_t lhdc_ver) {
    (void)fast_mode;
    (void)lhdc_ver;
    fb->sample_rate = sample_rate;
    fb->bits_per_sample = bits_per_sample;
    fb->ch = channels;
    if (block_size > LHDC_V4_MAX_BLOCK_SAMPLES || process_size > LHDC_V4_ANALYSIS_MAX_SAMPLES)
        return;
    size_t len = fb->cfg_len;
    fb->input_block_size = fb->enc_block_size = block_size;
    fb->fft_len = process_size;
    v4f_kiss_fftr_alloc(process_size, 0, fb->cfg, &len);
    fb->bin_low = 1;
    v4f_SetQuality(fb, 15);
    fb->brc_quality_up_cnt = 0;
    fb->brc_sink_buf_len = sink_buf_len;
    v4f_LossyEncoderSetTargetByteRate(fb, target_byte_rate);
    fb->brc_target_block_mtu_limit =
        fb->lhdc_ver_code < LHDC_V4_STREAM_VERSION_CODE_300 ? 2000 : target_mtu_byte;
    fb->min_bits_reserved = min_bits_reserved;
    fb->min_block_len = 0;
    if (need_min_byte_rate) {
        int32_t bytes =
            sample_rate ? (int32_t)((uint32_t)target_byte_rate * (uint32_t)block_size) / sample_rate
                        : 0;
        if (target_mtu_byte < bytes)
            bytes = target_mtu_byte;
        fb->min_block_len = bytes - 8;
    }
    for (int32_t ch = 0; ch < 2; ++ch) {
        fb->avgMS[ch] = -1.0f;
        fb->dat[ch] = fb->dat_b[ch] + 16;
        if (process_size >= -15)
            memset(fb->dat_b[ch], 0, (size_t)(process_size + 16) * sizeof(float));
        fb->last_div[ch] = 1;
    }
    if (!v4_copy_analysis_window(process_size, fb->HannWindow)) {
        for (int32_t i = 0; i < process_size; ++i) {
            float phase = ((float)i * AUDIO_MATH_TWO_PI_F32) / (float)(process_size - 1);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                 \
    (defined(_M_IX86) || defined(_M_X64))
            fb->HannWindow[i] = audio_math_hann_bounded_f64_to_f32_soft((double)phase);
#else
            fb->HannWindow[i] = (float)((1.0 - audio_math_cos((double)phase)) * .5);
#endif
        }
    }
    v4f_win_sinc_init(&fb->resample, bits_per_sample);
    v4f_ResetEncodeLPF(&fb->resample);
    fb->weighting_filter = (float *)(sample_rate == LHDC_V4_SAMPLE_RATE_44100   ? ITUR468_44100
                                     : sample_rate == LHDC_V4_SAMPLE_RATE_48000 ? ITUR468_48000
                                                                                : ITUR468_96000);
    memset(fb->quality_count, 0, sizeof fb->quality_count);
    memset(fb->use_avg_num, 0, sizeof fb->use_avg_num);
    memset(fb->limit_by_RMS, 0, sizeof fb->limit_by_RMS);
    fb->block_num = 0;
    fb->total_div = 0.0f;
    v4f_lhdc_InitEncoder(fb->encb, bits_per_sample, block_size, one_frame_per_channel,
                         fb->lhdc_ver_code);
    fb->no_btr_limit_frame_cnt =
        fb->lhdc_ver_code < LHDC_V4_STREAM_VERSION_CODE_300 || no_btr_limit_frame_cnt < 0
            ? 0
            : no_btr_limit_frame_cnt;
}

/* Select quantization divisor from FFT spectrum, smoothed mean square, and weighted noise floor; encode and update statistics. */
int32_t v4f_LossyEncoderProcess(FFT_BLOCK *fb, int32_t ns, int32_t final, uint8_t *out,
                                int32_t out_len) {
    if (out_len < fb->bits_per_sample / 8 * fb->input_block_size * fb->ch)
        return LHDC_V4_STATUS_ERROR;
    if (!ns)
        return v4f_DoEncode(fb, 0, final, out, out_len);
    if (ns < fb->input_block_size) {
        for (int32_t i = ns; i < fb->input_block_size; ++i)
            fb->dat[0][i] = fb->dat[1][i] = 0.0f;
        ns = fb->input_block_size;
    }
    for (int32_t ch = 0; ch < fb->ch; ++ch) {
        float ms = 0.0f;
        for (int32_t i = 0; i < fb->input_block_size; ++i)
            ms = ms + fb->dat[ch][i] * fb->dat[ch][i];
        ms /= (float)fb->input_block_size;
        if (fb->avgMS[ch] >= 0.0f)
            ms = (float)((double)ms * .75 + (double)fb->avgMS[ch] * .25);
        fb->avgMS[ch] = ms;

        int32_t fft_len = fb->fft_len;
        int32_t bins = fft_len / 2 + 1;
        memset(fb->fft_result + 1, 0, (size_t)(bins - 1) * sizeof *fb->fft_result);
        for (int32_t i = 0; i < fft_len; ++i)
            fb->fft_in[i] = fb->dat[ch][i];
        for (int32_t block = fft_len; block < fb->input_block_size; block += fft_len)
            for (int32_t i = 0; i < fft_len; ++i)
                fb->fft_in[i] = fb->dat[ch][block + i] + fb->fft_in[i];
        for (int32_t i = 0; i < fft_len; ++i)
            fb->fft_in[i] = fb->HannWindow[i] * fb->fft_in[i];
        if (v4f_kiss_fftr(fb->cfg, fb->fft_in, fb->fft_out) != LHDC_V4_STATUS_OK)
            return LHDC_V4_STATUS_ERROR;
        float total_spectrum = 0.0f;
        for (int32_t i = 1; i < bins; ++i) {
            float real = fb->fft_out[i].r, imag = fb->fft_out[i].i;
            float spectrum = fb->fft_result[i] + real * real + imag * imag;
            fb->fft_result[i] = spectrum;
            total_spectrum += spectrum;
        }
        int32_t bin_hi = fb->bin_hi;
        while (bin_hi > fb->bin_11025 &&
               !((total_spectrum * fb->min_noise_floor) / (float)bins < fb->fft_result[bin_hi]))
            --bin_hi;
        int32_t lowest_bin = bin_hi < fb->bin_11025 ? bin_hi : fb->bin_11025;
        for (int32_t i = bin_hi; i > fb->bin_11025; --i) {
            lowest_bin = i;
            if (!(fb->fft_result[i] <= fb->fft_result[i - 1]))
                break;
            lowest_bin = fb->bin_11025;
        }
        float lo_spectrum = AUDIO_MATH_POW2_53_F32;
        for (int32_t i = 2; i <= lowest_bin; ++i) {
            float spectrum = fb->fft_result[i] / fb->weighting_filter[i];
            /* Use ordered less-than; NaN does not update the minimum. Do not rewrite as !(a <= b). */
            if (spectrum < lo_spectrum)
                lo_spectrum = spectrum;
        }
        int32_t fft_step = fft_len ? fb->input_block_size / fft_len : 0;
        double scale = (2.0 / (double)(fft_len * fft_len)) / (double)fft_step;
        total_spectrum = (float)(scale * (double)total_spectrum);
        lo_spectrum = (float)(scale * (double)lo_spectrum);
        float floor = fb->min_noise_floor * total_spectrum;
        if (!(lo_spectrum > floor))
            lo_spectrum = floor;
        float avg = fb->avg_power_shift * (total_spectrum / (float)fb->upper_freq);
        float noise_floor = fb->noise_floor_shift * lo_spectrum;
        if (avg < noise_floor) {
            ++fb->use_avg_num[fb->quality];
            noise_floor = avg;
        }
        float rms = (float)(audio_math_sqrt_f64((double)ms) / (double)fb->min_bits_rms);
        float limit = rms * rms / 12.0f;
        if (limit < fb->min_rms)
            limit = fb->min_rms;
        int32_t limit_by_rms = v4f_CalcBTR(noise_floor, limit, &fb->btr[ch], &fb->btr_ext[ch],
                                           fb->min_btr_down_sample);
        if (limit_by_rms > 0)
            ++fb->limit_by_RMS[fb->quality];
    }
    if (fb->sample_rate == LHDC_V4_SAMPLE_RATE_96000) {
        for (int32_t ch = 0; ch < 2; ++ch) {
            if (fb->down_step == 1)
                v4f_win_sinc_delay(fb->dat[ch], fb->pcm[ch], fb->input_block_size);
            else
                v4f_EncodeLPF(&fb->resample, ch, fb->dat[ch], fb->pcm[ch], fb->input_block_size,
                              fb->bits_per_sample);
        }
    }
    ++fb->quality_count[fb->quality];
    int32_t bytes = v4f_DoEncode(fb, ns, final, out, out_len);
    ++fb->block_num;
    int32_t div0 = v4f_BTRtoDIV(fb->btr[0], fb->btr_ext[0]);
    int32_t div1 = v4f_BTRtoDIV(fb->btr[1], fb->btr_ext[1]);
    fb->total_div = fb->total_div + (float)div0 + (float)div1;
    return bytes;
}

/* Read interleaved stereo little-endian 16/24-bit PCM, then analyze and encode. */
int32_t v4f_LossyEncoderProcessWav(FFT_BLOCK *fb, uint8_t *wav, int32_t ns, int32_t final,
                                   uint8_t *out, int32_t out_len) {
    if (fb->bits_per_sample == LHDC_V4_PCM_BITS_S16)
        v4f_Get2ChWavData16(fb, wav, ns);
    else if (fb->bits_per_sample == LHDC_V4_PCM_BITS_S24)
        v4f_Get2ChWavData24(fb, wav, ns);
    return v4f_LossyEncoderProcess(fb, ns, final, out, out_len);
}

/* Divide integers, returning zero for a zero divisor. */
static int32_t v4i_quality_divide_or_zero(int32_t numerator, int32_t denominator) {
    return denominator ? numerator / denominator : 0;
}

/* Restore sixteen default quality presets for 16-bit and 24-bit encoding. */
void v4f_RestoreDefQualityPreset(FFT_BLOCK *fb) {
    memcpy(fb->quality_presets.pcm16, def_q_set16, sizeof fb->quality_presets.pcm16);
    memcpy(fb->quality_presets.pcm24, def_q_set24, sizeof fb->quality_presets.pcm24);
}

/* Update instance presets from a complete host table without files, allocation, or large stack scratch. */
int32_t v4f_LossyEncoderSetQualityPresets(FFT_BLOCK *fb, const LhdcV4QualityPresets *presets) {
    if (!fb || !presets)
        return LHDC_V4_STATUS_ERROR;
    memmove(&fb->quality_presets, presets, sizeof *presets);
    return LHDC_V4_STATUS_OK;
}

/* Copy instance parameters to the host without exposing writable internal pointers. */
int32_t v4f_LossyEncoderGetQualityPresets(const FFT_BLOCK *fb, LhdcV4QualityPresets *presets) {
    if (!fb || !presets)
        return LHDC_V4_STATUS_ERROR;
    memmove(presets, &fb->quality_presets, sizeof *presets);
    return LHDC_V4_STATUS_OK;
}

/* Select quality presets, adjust rate-specific noise floors, and calculate bin weights. */
void v4f_SetQuality(FFT_BLOCK *fb, int32_t quality) {
    if (quality >= 16)
        return;
    const QUALITY_PRESET *q_set = fb->bits_per_sample == LHDC_V4_PCM_BITS_S16
                                      ? fb->quality_presets.pcm16
                                      : fb->quality_presets.pcm24;
    const QUALITY_PRESET *preset = &q_set[quality];
    fb->quality = quality;
    fb->upper_freq = preset->upper_freq;
    fb->avg_power_shift = preset->avg_power_shift;
    fb->noise_floor_shift =
        (float)(((double)preset->noise_floor_shift * 96000.0) / (double)fb->sample_rate);
    v4_quality_power_factors(preset, &fb->min_bits_rms, &fb->min_noise_floor);
    float full_scale = (float)(int32_t)(1u << (((uint32_t)fb->bits_per_sample - 1) & 31));
    fb->min_rms = ((full_scale * full_scale) * fb->min_noise_floor) / 12.0f;
    fb->bin_hi = v4i_quality_divide_or_zero(
        (int32_t)((uint32_t)fb->fft_len * (uint32_t)fb->upper_freq), fb->sample_rate);
    fb->bin_11025 =
        v4i_quality_divide_or_zero((int32_t)((uint32_t)fb->fft_len * 11025u), fb->sample_rate);
    for (int32_t n = fb->bin_low; n <= fb->bin_hi; ++n)
        fb->fft_freq_factor[n] = (float)((double)((float)n / (float)fb->bin_hi) * (2.0 / 3.0));
}

/* Initialize bitrate-control windows and downsampling conditions from target bytes per second. */
void v4f_LossyEncoderSetTargetByteRate(FFT_BLOCK *fb, int32_t target_byte_rate) {
    int32_t block_len = v4i_quality_divide_or_zero(
        (int32_t)((uint32_t)fb->input_block_size * (uint32_t)target_byte_rate), fb->sample_rate);
    fb->down_step =
        fb->sample_rate == LHDC_V4_SAMPLE_RATE_96000 && target_byte_rate < 75001 ? 2 : 1;
    fb->min_btr_down_sample = 0;
    fb->brc_target_block_len = block_len;
    fb->brc_target_block_len_up_limit =
        target_byte_rate < 112500
            ? v4i_quality_divide_or_zero((int32_t)((uint32_t)block_len * 11), 10)
            : block_len;
    fb->brc_quality_up_threshold = (int32_t)((uint32_t)block_len * 2);
    fb->brc_target_block_len_low_limit =
        v4i_quality_divide_or_zero((int32_t)((uint32_t)block_len * 6), 10);
    int32_t block_num = v4i_quality_divide_or_zero(fb->brc_sink_buf_len, block_len);
    if (block_num > 15)
        block_num = 16;
    fb->brc_block_oldest = 0;
    fb->brc_block_num = block_num;
    fb->brc_total_len = (int32_t)((uint32_t)block_num * (uint32_t)block_len);
    fb->brc_target_len = fb->brc_total_len;
    for (int32_t i = 0; i < block_num; ++i)
        fb->brc_block_len[i] = block_len;
}

/* V1/V2 update frame length by byte rate; other versions update available frame bytes by MTU. */
void v4f_LossyEncoderUpdateFrameSize(FFT_BLOCK *fb, int32_t target_mtu_byte,
                                     int32_t target_byte_rate) {
    if (fb->lhdc_ver_code < LHDC_V4_STREAM_VERSION_CODE_300) {
        v4f_LossyEncoderSetTargetByteRate(fb, target_byte_rate);
        target_mtu_byte = v4i_quality_divide_or_zero(
            (int32_t)((uint32_t)fb->enc_block_size * (uint32_t)target_byte_rate), fb->sample_rate);
    } else
        fb->brc_target_block_mtu_limit = target_mtu_byte;
    fb->min_block_len = (int32_t)((uint32_t)target_mtu_byte - 8);
}

/* Select quantization shift/extension level from noise floor and mean-square energy; report RMS limiting. */
int32_t v4f_CalcBTR(float noise_floor, float ms, int32_t *btr, int32_t *btr_ext, int32_t min_btr) {
    int32_t bits = min_btr, limit_by_rms = 0;
    float bit_err = ms_bits_err[bits];
    /* Use ordered bit_err < noise_floor; NaN does not enter this branch. */
    if (bit_err < noise_floor) {
        while (1) {
            if (ms < bit_err) {
                limit_by_rms = 1;
                break;
            }
            bit_err = ms_bits_err[++bits];
            if (!(bit_err < noise_floor))
                break;
        }
    }
    *btr = bits;
    if (bits < 3) {
        static const float threshold[7] = {.083333336f, .33333334f, .75f,      1.3333334f,
                                           2.0833333f,  3.0f,       4.0833335f};
        *btr = 0;
        for (int32_t ext = 1; ext < 7; ++ext) {
            bit_err = threshold[ext - 1];
            /* For fewer than three bits, unordered comparison also enters this branch. */
            if (!(noise_floor > bit_err)) {
                *btr_ext = ext;
                return limit_by_rms;
            }
            if (ms < bit_err) {
                *btr_ext = ext;
                return LHDC_V4_BTR_LIMITED_BY_RMS;
            }
        }
        if (!(noise_floor > threshold[6])) {
            *btr_ext = 7;
            return limit_by_rms;
        }
        if (!(ms < threshold[6])) {
            *btr_ext = 8;
            return limit_by_rms;
        }
    } else {
        int32_t base = (int32_t)(1u << ((uint32_t)bits & 31));
        float bit_err_step = (float)base * .125f;
        float value = (float)(base - 1) + bit_err_step;
        for (int32_t ext = 0; ext < 7; ++ext) {
            bit_err = (value * value) / 12.0f;
            if (noise_floor <= bit_err) {
                *btr_ext = ext;
                return limit_by_rms;
            }
            if (ms <= bit_err) {
                *btr_ext = ext;
                return LHDC_V4_BTR_LIMITED_BY_RMS;
            }
            value = value + bit_err_step;
        }
        bit_err = (value * value) / 12.0f;
        if (noise_floor <= bit_err) {
            *btr_ext = 7;
            return limit_by_rms;
        }

        if (!(ms <= bit_err)) {
            *btr_ext = 8;
            return limit_by_rms;
        }
    }
    *btr_ext = 7;
    return LHDC_V4_BTR_LIMITED_BY_RMS;
}

/* Exact V4 constants; hexadecimal floating notation avoids decimal rounding. */

const float ms_bits_err[24] = {

    0.083333336f,   0.75f,         4.0833335f,     18.75f,         80.083336f,     330.75f,
    1344.0834f,     5418.75f,      21760.084f,     87210.75f,      349184.1f,      1397418.8f,
    5.59104e+06f,   2.236689e+07f, 8.947302e+07f,  3.57903e+08f,   1.4316339e+09f, 5.726579e+09f,
    2.2906405e+10f, 9.162579e+10f, 3.6650353e+11f, 1.4660148e+12f, 5.8640606e+12f, 2.3456245e+13f};

#include <stdint.h>

/* Trial-encode and increase the divisor to meet bitrate/MTU limits; update quality/bitrate history after final output. */
int32_t v4f_DoEncode(FFT_BLOCK *fb, int32_t ns, int32_t final, uint8_t *out, int32_t out_len) {
    int32_t quality = fb->quality, bytes = 0;
    uint8_t *enc_buf = (uint8_t *)fb->out_buf;
    if (ns) {
        bytes = v4f_EncodeBlocks(fb, ns, final, fb->btr, fb->btr_ext, enc_buf, out_len, 1);
        if (bytes < 0)
            return LHDC_V4_STATUS_ERROR;
        int32_t btr[2] = {fb->btr[0], fb->btr[1]};
        int32_t btr_ext[2] = {fb->btr_ext[0], fb->btr_ext[1]};
        int32_t div[2] = {v4f_BTRtoDIV(btr[0], btr_ext[0]), v4f_BTRtoDIV(btr[1], btr_ext[1])};
        if (bytes > fb->brc_target_block_len_up_limit || bytes > fb->brc_target_block_mtu_limit) {
            for (int32_t ch = 0; ch < 2; ++ch) {
                int32_t lower = fb->last_div[ch] / 4;
                if (div[ch] < lower) {
                    div[ch] = lower;
                    v4f_DIVtoBTR(lower, &btr[ch], &btr_ext[ch]);
                }
            }
            uint32_t retries = 0;
            while (bytes > fb->brc_target_block_len_up_limit ||
                   bytes > fb->brc_target_block_mtu_limit) {
                int32_t max_btr = fb->bits_per_sample - fb->min_bits_reserved;
                if ((div[0] >= fb->last_div[0] * 8 && div[1] >= fb->last_div[1] * 8 &&
                     fb->no_btr_limit_frame_cnt < 1 && bytes <= fb->brc_target_block_mtu_limit) ||
                    (btr[0] >= max_btr && btr[1] >= max_btr))
                    break;
                for (int32_t ch = 0; ch < 2; ++ch) {
                    if (btr[ch] >= max_btr)
                        continue;
                    int32_t doubled = v4f_BTRtoDIV(btr[ch], btr_ext[ch]) * 2;
                    int32_t upper = fb->last_div[ch] * 8;
                    div[ch] = doubled;
                    if (doubled > upper && fb->no_btr_limit_frame_cnt <= 0 &&
                        bytes <= fb->brc_target_block_mtu_limit)
                        div[ch] = upper;
                    v4f_DIVtoBTR(div[ch], &btr[ch], &btr_ext[ch]);
                }
                bytes = v4f_EncodeBlocks(fb, ns, final, btr, btr_ext, enc_buf, out_len, 1);
                if (bytes < 0)
                    return LHDC_V4_STATUS_ERROR;
                ++retries;
                for (int32_t ch = 0; ch < 2; ++ch) {
                    fb->btr[ch] = btr[ch];
                    fb->btr_ext[ch] = btr_ext[ch];
                }
            }
            if (retries > 1)
                quality = quality > 0 ? quality - 1 : 0;
        }
        bytes = v4f_EncodeBlocks(fb, ns, final, btr, btr_ext, enc_buf, out_len, 0);
        if (fb->no_btr_limit_frame_cnt > 0)
            --fb->no_btr_limit_frame_cnt;
        fb->last_div[0] = div[0];
        fb->last_div[1] = div[1];
        for (int32_t i = 0; i < bytes; i += 4) {
            uint32_t word;
            memcpy(&word, enc_buf + i, sizeof word);
            word = audio_math_byte_swap32(word);
            memcpy(enc_buf + i, &word, sizeof word);
        }
    }
    memcpy(out, enc_buf, (size_t)bytes);
    fb->brc_total_len -= fb->brc_block_len[fb->brc_block_oldest];
    fb->brc_block_len[fb->brc_block_oldest] = bytes;
    fb->brc_total_len += bytes;
    if (++fb->brc_block_oldest >= fb->brc_block_num)
        fb->brc_block_oldest = 0;
    if (quality < fb->quality) {
        v4f_SetQuality(fb, quality);
        fb->brc_quality_up_cnt = 0;
    } else {
        if (bytes > fb->brc_target_block_len && fb->brc_total_len > fb->brc_target_len) {
            quality = quality > 0 ? quality - 1 : 0;
            fb->brc_quality_up_cnt = 0;
        } else if (bytes >= fb->brc_target_block_len ||
                   fb->brc_target_len - fb->brc_total_len <= fb->brc_quality_up_threshold) {
            fb->brc_quality_up_cnt = 0;
        } else {
            int32_t previous = fb->brc_quality_up_cnt++;
            if (previous > 8) {
                quality = quality < 15 ? quality + 1 : 15;
                fb->brc_quality_up_cnt = 0;
            }
        }
        if (quality != fb->quality)
            v4f_SetQuality(fb, quality);
    }
    v4f_lhdc_DecMetaFrameCnt(fb->encb);
    return bytes;
}

#include "../include/codec/scalar.h"



/* Map target base/extension bitrates to a quantization divisor. */

int32_t v4_BTRtoDIV(int32_t btr, int32_t btr_ext) {
    if (btr < 3)
        return btr_ext;
    return (int32_t)(v4_shift_left_compat32(1u, (uint32_t)btr) +
                     v4_shift_left_compat32((uint32_t)btr_ext, (uint32_t)btr - 3u));
}

/* Convert quantization divisor to base/extension bitrate. */

void v4_DIVtoBTR(int32_t div, int32_t *btr, int32_t *btr_ext) {
    if (div < 8) {
        *btr = 0;
        *btr_ext = div;
        return;
    }
    int32_t b = 3;
    while ((int32_t)v4_shift_left_compat32(1u, (uint32_t)b + 1u) <= div)
        ++b;
    int32_t unit = (int32_t)v4_shift_left_compat32(1u, (uint32_t)b - 3u);
    int32_t difference = (int32_t)((uint32_t)div - v4_shift_left_compat32(1u, (uint32_t)b));
    *btr_ext = unit ? difference / unit : 0;
    *btr = b;
}

#include "../include/codec/v4_pcm.h"
#include "encoder_tables.h"

/* Read interleaved PCM into integer/floating input buffers and zero-pad. */
static int32_t v4i_pcm_input_get_pcm(struct FFT_block_s *fb, uint8_t *wav, int32_t ns,
                                     int32_t bytes) {
    int32_t *pcm[2];
    float *dat[2];
    for (int32_t ch = 0; ch < 2; ++ch) {
        pcm[ch] = fb->pcm[ch];
        dat[ch] = fb->dat[ch];
    }
    if (ns < 0)
        ns = 0;
    for (int32_t i = 0; i < ns; ++i)
        for (int32_t ch = 0; ch < 2; ++ch) {
            uint32_t u = 0;
            for (int32_t k = 0; k < bytes; ++k)
                u |= (uint32_t)*wav++ << (8 * k);
            int32_t value = bytes == 2 ? (int32_t)(int16_t)u : (int32_t)(u << 8) >> 8;
            pcm[ch][i] = value;
            dat[ch][i] = (float)value;
        }
    int32_t block = fb->input_block_size;
    for (; ns < block; ++ns)
        for (int32_t ch = 0; ch < 2; ++ch) {
            pcm[ch][ns] = 0;
            dat[ch][ns] = 0.0f;
        }
    return ns;
}

/* Read interleaved stereo 16-bit PCM and update analysis input. */
int32_t v4_Get2ChWavData16(struct FFT_block_s *fb, uint8_t *wav, int32_t ns) {
    return v4i_pcm_input_get_pcm(fb, wav, ns, 2);
}

/* Read interleaved stereo 24-bit PCM and update analysis input. */
int32_t v4_Get2ChWavData24(struct FFT_block_s *fb, uint8_t *wav, int32_t ns) {
    return v4i_pcm_input_get_pcm(fb, wav, ns, 3);
}
