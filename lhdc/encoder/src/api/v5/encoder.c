#include "../../../../common/include/lhdc_log.h"
#include "../../../../common/include/lhdc_alignment.h"
#include <audio_math/byte_order.h>
#include <audio_math/integer.h>
#include <audio_math/float_convert.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../v5/include/encoder_internal.h"
#include "../../v5/include/codec/codec_internal.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read-only initial values for per-instance bitrate tables. 48/96/192 kHz share one table and
 * 44.1 kHz retains a separate table. Each initialized instance owns a writable copy; runtime
 * adjustments never modify these values. */
static const int32_t v5_default_bitrate_kbps[10] = {64,  128, 192, 256,  320,
                                                    400, 500, 900, 1000, 2000};
static const int32_t v5_44100_bitrate_kbps[10] = {64,  128, 192, 240,  320,
                                                  400, 480, 900, 1000, 2000};

/* LHDC V5 encoder: construct the object and subworkspaces in caller memory. */
int32_t v5f_lhdcv5_encoder_new(uint32_t version, lhdcv5_para_t *handle, uint32_t mem_size) {
    uint32_t header_bytes = sizeof(*handle) + 16;
    if (!handle || version != 1 || mem_size < header_bytes)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    lhdcv5_para_t *lhdcv5 = (void *)lhdc_align16(handle);
    memset(lhdcv5, 0, sizeof(*lhdcv5));
    memcpy(lhdcv5->bitrate_table, v5_default_bitrate_kbps, sizeof lhdcv5->bitrate_table);
    memcpy(lhdcv5->bitrate_tables[0], v5_44100_bitrate_kbps, sizeof lhdcv5->bitrate_tables[0]);
    memcpy(lhdcv5->bitrate_tables[1], v5_default_bitrate_kbps, sizeof lhdcv5->bitrate_tables[1]);
    memcpy(lhdcv5->bitrate_tables[2], v5_default_bitrate_kbps, sizeof lhdcv5->bitrate_tables[2]);
    memcpy(lhdcv5->bitrate_tables[3], v5_default_bitrate_kbps, sizeof lhdcv5->bitrate_tables[3]);
    lhdcv5->version = 1;
    lhdcv5->sample_rate = LHDC_V5_SAMPLE_RATE_48000;
    lhdcv5->bits_per_sample = 24;
    lhdcv5->frame_duration = LHDC_V5_FRAME_DURATION_5MS;
    lhdcv5->host_mtu_size = 660;
    lhdcv5->lastBitrate = 400;
    lhdcv5->qualityStatus = 5;
    lhdcv5->max_bitrate_inx = 8;
    lhdcv5->actual_bitrate = 400000;
    lhdcv5->encode_interval = 20;
    int32_t lhdcv5_size = 0;
    if (v5f_lhdc_v5_enc_workspace_get_size(2, &lhdcv5_size) || lhdcv5_size <= 0 ||
        (mem_size - header_bytes) < (uint32_t)lhdcv5_size + 16) {
        lhdcv5->lh5_enc = 0;
        lhdcv5->cirbuff = 0;
        lhdcv5->enc_in_buf = 0;
        return LHDCV5_FRET_ERROR;
    }
    lhdcv5->lh5_enc = lhdc_align16((uint8_t *)lhdcv5 + sizeof(*lhdcv5));
    memset(lhdcv5->lh5_enc, 0, lhdcv5_size);
    if (v5f_lhdc_v5_enc_workspace_init(2, lhdcv5->lh5_enc) ||
        v5f_lhdc_v5_enc_set_version(1, lhdcv5->lh5_enc)) {
        lhdcv5->lh5_enc = 0;
        lhdcv5->cirbuff = 0;
        lhdcv5->enc_in_buf = 0;
        return LHDCV5_FRET_ERROR;
    }
    uint32_t remaining_bytes = (mem_size - header_bytes) - lhdcv5_size - 16;
    if (remaining_bytes < 0x1e010) {
        lhdcv5->lh5_enc = 0;
        lhdcv5->cirbuff = 0;
        lhdcv5->enc_in_buf = 0;
        return LHDCV5_FRET_ERROR;
    }
    lhdcv5->cirbuff = lhdc_align16((uint8_t *)lhdcv5->lh5_enc + lhdcv5_size);
    memset(lhdcv5->cirbuff, 0, 0x1e000);
    lhdc_cirbuf_init_inline(&lhdcv5->input_cbuf, lhdcv5->cirbuff, 0x1e000);
    lhdcv5->enc_in_buf_bytes = 0x1e00;
    if (remaining_bytes - 0x1e010 < 0x1e10) {
        lhdcv5->lh5_enc = 0;
        lhdcv5->cirbuff = 0;
        lhdcv5->enc_in_buf = 0;
        return LHDCV5_FRET_ERROR;
    }
    lhdcv5->enc_in_buf = lhdcv5->cirbuff + 0x1e000;
    return LHDCV5_FRET_SUCCESS;
}

/* Bluetooth encoder wrapper: construct encoder and AR handles in caller memory. */
int32_t v5f_lhdcv5_util_get_handle(uint32_t version, lhdcv5_cb_t *handle, uint32_t mem_size) {
    if (!handle || version != 1 || mem_size < sizeof(*handle))
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    handle->enc_type = 1;
    handle->err = 0;
    handle->enc.lhdcv5 = 0;
    handle->ar_filter = 0;
    handle->reference_abr_index = 0;
    handle->owned_workspace = NULL;
    uint32_t mem_req_lhdcv5_encoder = 0, mem_req_ar = 0;
    if (v5f_lhdcv5_encoder_get_mem_req(1, &mem_req_lhdcv5_encoder) || !mem_req_lhdcv5_encoder ||
        mem_size - sizeof(*handle) < mem_req_lhdcv5_encoder + 16) {
        if (handle->enc.lhdcv5) {
            v5f_lhdcv5_encoder_free(handle->enc.lhdcv5);
            handle->enc.lhdcv5 = 0;
        }
        if (!handle->ar_filter) {
            return LHDCV5_FRET_ERROR;
        }
        v5f_ar_process_free(handle->ar_filter);
        handle->ar_filter = 0;
        return LHDCV5_FRET_ERROR;
    }
    handle->enc.lhdcv5 = (void *)lhdc_align16((uint8_t *)handle + sizeof(*handle));
    if (v5f_lhdcv5_encoder_new(1, handle->enc.lhdcv5, mem_req_lhdcv5_encoder)) {
        if (handle->enc.lhdcv5) {
            v5f_lhdcv5_encoder_free(handle->enc.lhdcv5);
            handle->enc.lhdcv5 = 0;
        }
        if (!handle->ar_filter) {
            return LHDCV5_FRET_ERROR;
        }
        v5f_ar_process_free(handle->ar_filter);
        handle->ar_filter = 0;
        return LHDCV5_FRET_ERROR;
    }
#if LHDC_ENCODER_WITH_AR
    if (v5f_ar_process_get_mem_req(&mem_req_ar) || !mem_req_ar ||
        (mem_size - sizeof(*handle) - (mem_req_lhdcv5_encoder + 16)) < mem_req_ar + 16) {
        if (handle->enc.lhdcv5) {
            v5f_lhdcv5_encoder_free(handle->enc.lhdcv5);
            handle->enc.lhdcv5 = 0;
        }
        if (!handle->ar_filter) {
            return LHDCV5_FRET_ERROR;
        }
        v5f_ar_process_free(handle->ar_filter);
        handle->ar_filter = 0;
        return LHDCV5_FRET_ERROR;
    }
    handle->ar_filter =
        (void *)lhdc_align16((uint8_t *)handle->enc.lhdcv5 + mem_req_lhdcv5_encoder);
    if (v5f_ar_process_new(handle->ar_filter, mem_req_ar)) {
        if (handle->enc.lhdcv5) {
            v5f_lhdcv5_encoder_free(handle->enc.lhdcv5);
            handle->enc.lhdcv5 = 0;
        }
        if (!handle->ar_filter) {
            return LHDCV5_FRET_ERROR;
        }
        v5f_ar_process_free(handle->ar_filter);
        handle->ar_filter = 0;
        return LHDCV5_FRET_ERROR;
    }
#endif
    return LHDCV5_FRET_SUCCESS;
}

/* Attempt AR on input PCM; log query failure or unavailable effects and continue encoding. */
static void process_optional_ar(lhdcv5_cb_t *handle, void *pcm, uint32_t pcm_bytes) {

    uint32_t enc_type = handle->enc_type;
    if (enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       enc_type);
        return;
    }
    if (!handle->enc.lhdcv5) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        return;
    }

    int32_t ar_enabled = 0;
    LHDCV5_Workspace *workspace = handle->enc.lhdcv5->lh5_enc;
    if (!workspace) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        return;
    }
    int32_t flag_result = v5f_lhdc_v5_enc_get_ar_flag(&ar_enabled, workspace);
    if (flag_result != 0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Failed to query AR flag (%d)",
                       __func__, flag_result);
        return;
    }
    if (!ar_enabled)
        return;
    lhdcv5_ar_para_t *ar_handle = handle->ar_filter;
    if (!ar_handle)
        return;
    uint32_t sample_rate = ar_handle->sample_rate;
    if (sample_rate != LHDC_V5_SAMPLE_RATE_48000) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: AR process doesn't support current sample rate (%u)!", __func__,
                       sample_rate);
        return;
    }
    if (v5f_ar_process_run(ar_handle, pcm, pcm_bytes) != 0)
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to do AR process!", __func__);
}

/* Validate input, attempt AR, then submit PCM to the encoder and return frames/bytes. */
int32_t v5f_lhdcv5_util_enc_process(lhdcv5_cb_t *handle, void *p_pcm, uint32_t pcm_bytes,
                                    uint8_t *out_put, uint32_t out_buf_bytes, uint32_t *written,
                                    uint32_t *out_frames) {
    if (!handle) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_CB;
    }
    if (!p_pcm || !out_put || !written || !out_frames) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: input parameter is NULL!", __func__);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!pcm_bytes) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: input buffer size is invalid (%d)!",
                       __func__, 0);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!out_buf_bytes) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: output buffer size is invalid (%d)!",
                       __func__, 0);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }

    process_optional_ar(handle, p_pcm, pcm_bytes);
    /* Reread codec type/handle after AR to use processor-updated instance state. */
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        return LHDCV5_FRET_INVALID_CODEC;
    }
    if (!handle->enc.lhdcv5) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    int32_t encode_result = v5f_lhdcv5_encoder_encode(handle->enc.lhdcv5, p_pcm, pcm_bytes, out_put,
                                                      out_buf_bytes, written, out_frames);
    if (encode_result == 0) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to encode (%d)!", __func__,
                   encode_result);
    return LHDCV5_FRET_ERROR;
}

/* Log an encoding API numeric-parameter error and return the caller-required status. */
static int32_t encoder_config_error(const char *function, const char *format, uint32_t value,
                                    int32_t error) {
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", format, function, value);
    return error;
}
/* Convert interleaved PCM to stereo integer planes; 32-bit containers retain the high 24 bits. */
static void unpack_encoder_pcm(const uint8_t *in, uint32_t bits_per_sample,
                               uint32_t samples_per_frame, int32_t *in_tmp) {
    int32_t *ch0 = in_tmp;
    int32_t *ch1 = in_tmp + samples_per_frame;

    if (bits_per_sample == 24) {
        for (uint32_t i = 0; i < samples_per_frame; ++i) {
            const uint8_t *src = in + i * 6;
            uint32_t l_bits = (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16);
            uint32_t r_bits = (uint32_t)src[3] | ((uint32_t)src[4] << 8) | ((uint32_t)src[5] << 16);
            ch0[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(l_bits << 8), 8);
            ch1[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(r_bits << 8), 8);
        }
    } else if (bits_per_sample == 16) {
        for (uint32_t i = 0; i < samples_per_frame; ++i) {
            uint32_t pcm_s16_l = audio_math_read_le16(in + i * 4);
            uint32_t pcm_s16_r = audio_math_read_le16(in + i * 4 + 2);
            ch0[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(pcm_s16_l << 16), 16);
            ch1[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(pcm_s16_r << 16), 16);
        }
    } else if (bits_per_sample == 32) {
        for (uint32_t i = 0; i < samples_per_frame; ++i) {
            uint32_t pcm_s32_l = audio_math_read_le32(in + i * 8);
            uint32_t pcm_s32_r = audio_math_read_le32(in + i * 8 + 4);
            ch0[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(pcm_s32_l), 8);
            ch1[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(pcm_s32_r), 8);
        }
    }
}
/* Read output frame length and compute actual bitrate with a 44.1 kHz proportional correction. */
static void update_encoded_frame_size(lhdcv5_para_t *lhdcv5, uint32_t encoded_frame_size) {
    uint32_t actual_bitrate =
        lhdcv5->frame_duration ? encoded_frame_size * 80000 / lhdcv5->frame_duration : 0;
    lhdcv5->encoded_frame_size = encoded_frame_size;
    lhdcv5->actual_bitrate = actual_bitrate;
    if (lhdcv5->sample_rate == LHDC_V5_SAMPLE_RATE_44100) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Double exactly holds the product of two binary32 operands, then truncation occurs at extended precision. */
        lhdcv5->actual_bitrate =
            audio_math_truncate_i32_f64((double)(float)actual_bitrate * (double)0.91875f);
#else
        lhdcv5->actual_bitrate = (int32_t)((float)actual_bitrate * 0.91875f);
#endif
    }
}
/* Validate sample-rate, duration, and frame-sample combinations and return explicit configuration errors. */
static int32_t validate_v5_frame_configuration(const lhdcv5_para_t *lhdcv5, const char *function) {
    if (lhdcv5->frame_duration != LHDC_V5_FRAME_DURATION_5MS)
        return encoder_config_error(function, "%s: Invalid frame duration (%d)!",
                                    lhdcv5->frame_duration, -8);
    uint32_t samples_per_frame = lhdcv5->samples_per_frame;
    uint32_t expected_samples;
    switch (lhdcv5->sample_rate) {
    case LHDC_V5_SAMPLE_RATE_44100:
    case LHDC_V5_SAMPLE_RATE_48000:
        expected_samples = 240;
        break;
    case LHDC_V5_SAMPLE_RATE_96000:
        expected_samples = 480;
        break;
    case LHDC_V5_SAMPLE_RATE_192000:
        expected_samples = 960;
        break;
    default:
        return encoder_config_error(function, "%s: Invalid sample rate (%d)!", lhdcv5->sample_rate,
                                    -8);
    }
    if (samples_per_frame != expected_samples)
        return encoder_config_error(function, "%s: Invalid samples per frame (%d)!",
                                    samples_per_frame, -8);
    return LHDCV5_FRET_SUCCESS;
}

/* Validate PCM/output capacity, queue input, encode complete packets, and return output bytes/frames. */
int32_t v5f_lhdcv5_encoder_encode(lhdcv5_para_t *lhdcv5, uint8_t *in, uint32_t in_buf_bytes,
                                  uint8_t *out, uint32_t out_buf_bytes, uint32_t *written_bytes,
                                  uint32_t *out_frames) {
    if (!lhdcv5 || !lhdcv5->lh5_enc || !lhdcv5->cirbuff || !in || !out || !written_bytes ||
        !out_frames) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        if (!lhdcv5)
            return LHDCV5_FRET_INVALID_HANDLE_PARA;
        if (!lhdcv5->lh5_enc)
            return LHDCV5_FRET_INVALID_HANDLE_ENC;
        if (!lhdcv5->cirbuff)
            return LHDCV5_FRET_INVALID_HANDLE_CBUF;
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    *out_frames = 0;
    *written_bytes = 0;
    if (!out_buf_bytes)
        return encoder_config_error("v5f_lhdcv5_encoder_encode",
                                    "%s: output buffer size is invalid (%d)!", 0, -1);
    if (lhdcv5->version != 1)
        return encoder_config_error("v5f_lhdcv5_encoder_encode", "%s: Invalid version (%d)!",
                                    lhdcv5->version, -7);
    if (!lhdcv5->enc_in_buf) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_CODEC_NOT_READY;
    }
    int32_t configuration_result =
        validate_v5_frame_configuration(lhdcv5, "v5f_lhdcv5_encoder_encode");
    if (configuration_result != LHDCV5_FRET_SUCCESS)
        return configuration_result;
    uint32_t samples_per_frame = lhdcv5->samples_per_frame;
    savi_circ_buf *cbuf = &lhdcv5->input_cbuf;
    uint32_t encoded_frame_size = 0;
    if (lhdcv5->updateFramneInfo && lhdc_cirbuf_len_inline(cbuf) == 0) {
        lhdcv5->updateFramneInfo = false;
        int32_t func_ret = v5f_lhdc_v5_enc_set_bitrate(lhdcv5->lastBitrate * 1000, lhdcv5->lh5_enc);
        if (func_ret)
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to set bit rate (%d)!",
                           __func__, func_ret);
        func_ret =
            v5f_lhdc_v5_enc_get_encoded_frame_size((int32_t *)&encoded_frame_size, lhdcv5->lh5_enc);
        if (!func_ret)
            update_encoded_frame_size(lhdcv5, encoded_frame_size);
        else
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get frame size. (%d)!",
                           __func__, func_ret);
        v5f_cal_frame_size_and_frames_in_packet(lhdcv5);
    }
    uint32_t frame_per_packet = lhdcv5->frame_per_packet;
    if (frame_per_packet - 1 >= 16)
        return encoder_config_error("v5f_lhdcv5_encoder_encode",
                                    "%s: Invalid frames per packet (%d)!", frame_per_packet, -8);
    encoded_frame_size = lhdcv5->encoded_frame_size;
    if (encoded_frame_size - 1 >= 1023)
        return encoder_config_error("v5f_lhdcv5_encoder_encode",
                                    "%s: Invalid encoded frames bytes (%d)!", encoded_frame_size,
                                    -8);
    uint32_t bytes_per_frame = samples_per_frame * 8;
    uint32_t bytes_per_frame_in = (samples_per_frame * lhdcv5->bits_per_sample >> 2) & 0x1fffffff;
    if (in_buf_bytes < bytes_per_frame_in || lhdcv5->enc_in_buf_bytes < bytes_per_frame) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: Input buffer is not enough (%u) (%u) (%u) (%u)!", __func__,
                       in_buf_bytes, bytes_per_frame_in, lhdcv5->enc_in_buf_bytes, bytes_per_frame);
        return LHDCV5_FRET_BUF_NOT_ENOUGH;
    }
    uint32_t input_bits = lhdcv5->bits_per_sample_ui;
    if (input_bits != 16 && input_bits != 24 && input_bits != 32)
        return encoder_config_error("v5f_lhdcv5_encoder_encode", "%s: Invald bits per sample (%d)!",
                                    input_bits, -8);
    int32_t *in_tmp = (int32_t *)lhdcv5->enc_in_buf;
    unpack_encoder_pcm(in, input_bits, samples_per_frame, in_tmp);
    if ((uint32_t)lhdc_cirbuf_len_inline(cbuf) + bytes_per_frame >= 0x1e001) {
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                       "%s: Queue buffer is not enough (%u, %d)!", __func__,
                       (uint32_t)lhdc_cirbuf_len_inline(cbuf) + bytes_per_frame, 0x1e000);
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_cirbuf_put(cbuf, (uint8_t *)in_tmp, bytes_per_frame);
    if ((uint32_t)lhdc_cirbuf_len_inline(cbuf) < frame_per_packet * bytes_per_frame) {
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                       "%s: samples queued is not enough (%d, %u)!", __func__,
                       lhdc_cirbuf_len_inline(cbuf), frame_per_packet * bytes_per_frame);
        return LHDCV5_FRET_SUCCESS;
    }
    if (out_buf_bytes < encoded_frame_size * frame_per_packet)
        return encoder_config_error("v5f_lhdcv5_encoder_encode",
                                    "%s: Output buffer is not enough (%d)!", out_buf_bytes, -11);
    memset(out, 0, out_buf_bytes);
    uint32_t enc_bytes_cnt = 0, out_frames_cnt = 0;
    while (out_frames_cnt < frame_per_packet &&
           encoded_frame_size + enc_bytes_cnt <= out_buf_bytes) {
        int32_t encoded_bytes = 0;
        lhdc_cirbuf_get(cbuf, (uint8_t *)in_tmp, bytes_per_frame);
        int32_t func_ret =
            v5f_lhdc_v5_enc_encode(in_tmp, out + enc_bytes_cnt, &encoded_bytes, lhdcv5->lh5_enc);
        if (func_ret || (uint32_t)encoded_bytes != lhdcv5->encoded_frame_size) {
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                           "%s: Encode bytes is not conformant (%d) (%d, %u)!", __func__, func_ret,
                           encoded_bytes, lhdcv5->encoded_frame_size);
            *written_bytes = enc_bytes_cnt;
            *out_frames = out_frames_cnt;
            return LHDCV5_FRET_SUCCESS;
        }
        enc_bytes_cnt += encoded_bytes;
        ++out_frames_cnt;
    }
    if (out_frames_cnt == frame_per_packet) {
        *written_bytes = enc_bytes_cnt;
        *out_frames = frame_per_packet;
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Output buffer is not enough (%u + %u, %u)!", __func__, enc_bytes_cnt,
                   encoded_frame_size, out_buf_bytes);
    return LHDCV5_FRET_BUF_NOT_ENOUGH;
}

/* Bluetooth encoder wrapper: initialize sample format, bitrate, frame length, and scheduling. */
int32_t v5f_lhdcv5_util_init_encoder(lhdcv5_cb_t *handle, uint32_t sampling_freq,
                                     uint32_t bits_per_sample, uint32_t bitrate_inx,
                                     uint32_t frame_duration, uint32_t mtu, uint32_t interval) {
    if (!handle) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_CB;
    }
    if (sampling_freq != LHDC_V5_SAMPLE_RATE_44100 && sampling_freq != LHDC_V5_SAMPLE_RATE_48000 &&
        sampling_freq != LHDC_V5_SAMPLE_RATE_96000 && sampling_freq != LHDC_V5_SAMPLE_RATE_192000)
        return encoder_config_error("v5f_lhdcv5_util_init_encoder",
                                    "%s: Invalid sampling frequency (%d)!", sampling_freq,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    if (!(bits_per_sample < 0x21 &&
          (UINT64_C(1) << ((lhdc_ulong_t)bits_per_sample & 0x3f) & 0x101010000U) != 0))
        return encoder_config_error("v5f_lhdcv5_util_init_encoder",
                                    "%s: Invalid bits per sample (%d)!", bits_per_sample,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    if (bitrate_inx >= 10)
        return encoder_config_error("v5f_lhdcv5_util_init_encoder",
                                    "%s: Invalid bit rate (index) (%d)!", bitrate_inx,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    if (frame_duration != 0x32)
        return encoder_config_error("v5f_lhdcv5_util_init_encoder",
                                    "%s: Invalid frame duration (%d)!", frame_duration,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    /* Retain unsigned-subtraction range checking to reject MTUs both below and above bounds. */
    if (mtu - 300 >= 0x2d4)
        return encoder_config_error("v5f_lhdcv5_util_init_encoder", "%s: Invalid MTU (%d)!", mtu,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    if (interval != 10 && interval != 0x14)
        return encoder_config_error("v5f_lhdcv5_util_init_encoder",
                                    "%s: Invalid encode interval (%d)!", interval,
                                    LHDCV5_FRET_INVALID_INPUT_PARAM);
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        return LHDCV5_FRET_INVALID_CODEC;
    }
    if (!handle->enc.lhdcv5) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }

    int32_t func_ret = v5f_lhdcv5_encoder_init(handle->enc.lhdcv5, sampling_freq, bits_per_sample,
                                               bitrate_inx, 0x32, mtu, interval);
    if (func_ret != 0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to init. CODEC (%d)!", __func__,
                       func_ret);
        return LHDCV5_FRET_ERROR;
    }
    handle->enc.lhdcv5->frame_duration = 0x32;
    uint32_t samples_per_frame = 0;
    func_ret = v5f_lhdcv5_encoder_get_frame_len(handle->enc.lhdcv5, &samples_per_frame);
    if (func_ret != 0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get frame length (%d)!",
                       __func__, func_ret);
        return LHDCV5_FRET_ERROR;
    }
    if (!LHDC_ENCODER_WITH_AR || sampling_freq != LHDC_V5_SAMPLE_RATE_48000)
        return LHDCV5_FRET_SUCCESS;
    if (!samples_per_frame || !handle->ar_filter) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: AR instance is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_AR;
    }
    if (v5f_ar_process_init(handle->ar_filter, LHDC_V5_SAMPLE_RATE_48000, bits_per_sample, 2,
                            samples_per_frame) == 0) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to init. AR!", __func__);
    return LHDCV5_FRET_INVALID_HANDLE_AR;
}

/* Select the rate-specific bitrate table, validate limits, and initialize automatic/fixed bitrate state. */
static int32_t initialize_v5_bitrate_limits(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                            uint32_t bitrate_inx, const char *function) {
    uint32_t min_bitrate_inx = lhdcv5->min_bitrate_inx, max_bitrate_inx = lhdcv5->max_bitrate_inx;
    if (min_bitrate_inx >= 9)
        return encoder_config_error(function, "%s: Error, min bit rate (index) (%d)",
                                    min_bitrate_inx, -8);
    if (max_bitrate_inx >= 9)
        return encoder_config_error(function, "%s: Error, max bit rate (index) (%d)",
                                    max_bitrate_inx, -8);
    if (max_bitrate_inx < min_bitrate_inx) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: Error, min and max bit rate (index) (%u) (%u)", function,
                       min_bitrate_inx, max_bitrate_inx);
        return LHDCV5_FRET_CODEC_NOT_READY;
    }
    int32_t *bitrate_tbl_ptr =
        sampling_freq == LHDC_V5_SAMPLE_RATE_192000  ? lhdcv5->bitrate_tables[3]
        : sampling_freq == LHDC_V5_SAMPLE_RATE_96000 ? lhdcv5->bitrate_tables[2]
        : sampling_freq == LHDC_V5_SAMPLE_RATE_44100 ? lhdcv5->bitrate_tables[0]
                                                     : lhdcv5->bitrate_tables[1];
    memcpy(lhdcv5->bitrate_table, bitrate_tbl_ptr, sizeof lhdcv5->bitrate_table);
    lhdcv5->qualityStatus = bitrate_inx;
    if (bitrate_inx == 9) {
        lhdcv5->lastBitrate = lhdcv5->bitrate_table[5];
        lhdcv5->upBitrateCnt = lhdcv5->upBitrateSum = lhdcv5->dnBitrateCnt = lhdcv5->dnBitrateSum =
            0;
    } else {
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: bit rate = %d, limit = [%d, %d]",
                       function, lhdcv5->bitrate_table[bitrate_inx],
                       lhdcv5->bitrate_table[min_bitrate_inx],
                       lhdcv5->bitrate_table[max_bitrate_inx]);
        uint32_t selected_index = bitrate_inx < min_bitrate_inx ? min_bitrate_inx : bitrate_inx;
        if (selected_index > max_bitrate_inx)
            selected_index = max_bitrate_inx;
        lhdcv5->qualityStatus = selected_index;
        lhdcv5->lastBitrate = lhdcv5->bitrate_table[selected_index];
    }
    return LHDCV5_FRET_SUCCESS;
}

/* Initialize the V5 core and input buffering from format, bitrate, MTU, and scheduling interval. */
int32_t v5f_lhdcv5_encoder_init(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                uint32_t bits_per_sample, uint32_t bitrate_inx,
                                uint32_t frame_duration, uint32_t mtu, uint32_t interval) {
    if (!lhdcv5 || !lhdcv5->lh5_enc || !lhdcv5->cirbuff) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        return !lhdcv5            ? LHDCV5_FRET_INVALID_HANDLE_PARA
               : !lhdcv5->lh5_enc ? LHDCV5_FRET_INVALID_HANDLE_ENC
                                  : LHDCV5_FRET_INVALID_HANDLE_CBUF;
    }
    if (lhdcv5->version != 1)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid version (%d)!",
                                    lhdcv5->version, -7);
    if (sampling_freq != LHDC_V5_SAMPLE_RATE_44100 && sampling_freq != LHDC_V5_SAMPLE_RATE_48000 &&
        sampling_freq != LHDC_V5_SAMPLE_RATE_96000 && sampling_freq != LHDC_V5_SAMPLE_RATE_192000)
        return encoder_config_error("v5f_lhdcv5_encoder_init",
                                    "%s: Invalid sampling frequency (%d)!", sampling_freq, -1);
    if (bits_per_sample != 16 && bits_per_sample != 24 && bits_per_sample != 32)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid bits per sample (%d)!",
                                    bits_per_sample, -1);
    if (bitrate_inx >= 10)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid bit rate (index) (%d)!",
                                    bitrate_inx, -1);
    if (frame_duration != LHDC_V5_FRAME_DURATION_5MS)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid frame duration (%d)!",
                                    frame_duration, -1);
    if (mtu - 300 >= 724)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid MTU (%d)!", mtu, -1);
    if (interval != 10 && interval != 20)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Invalid encode interval (%d)!",
                                    interval, -1);
    int32_t bitrate_result =
        initialize_v5_bitrate_limits(lhdcv5, sampling_freq, bitrate_inx, "v5f_lhdcv5_encoder_init");
    if (bitrate_result != LHDCV5_FRET_SUCCESS)
        return bitrate_result;
    lhdcv5->host_mtu_size = mtu;
    lhdcv5->encode_interval = interval;
    lhdcv5->frame_duration = LHDC_V5_FRAME_DURATION_5MS;
    lhdcv5->sample_rate = sampling_freq;
    lhdcv5->bits_per_sample = bits_per_sample == 32 ? 24 : bits_per_sample;
    lhdcv5->bits_per_sample_ui = bits_per_sample;
    lhdc_cirbuf_reset_inline(&lhdcv5->input_cbuf);
    int32_t func_ret =
        v5f_lhdc_v5_enc_init(2, lhdcv5->bits_per_sample, sampling_freq, LHDC_V5_FRAME_DURATION_5MS,
                             lhdcv5->lastBitrate * 1000, lhdcv5->lh5_enc);
    if (func_ret)
        return encoder_config_error("v5f_lhdcv5_encoder_init", "%s: Fail to init. (%d)!", func_ret,
                                    -10);
    int32_t samples_per_frame = 0, encoded_frame_size = 0;
    func_ret = v5f_lhdc_v5_enc_get_samples_per_frame(&samples_per_frame, lhdcv5->lh5_enc);
    uint32_t expected_samples = sampling_freq == LHDC_V5_SAMPLE_RATE_192000  ? 960
                                : sampling_freq == LHDC_V5_SAMPLE_RATE_96000 ? 480
                                                                             : 240;
    if (func_ret || samples_per_frame < 1)
        return encoder_config_error("v5f_lhdcv5_encoder_init",
                                    "%s: Invalid samples per frame (%d)!", samples_per_frame, -8);
    lhdcv5->samples_per_frame = samples_per_frame;
    if ((uint32_t)samples_per_frame != expected_samples)
        return encoder_config_error("v5f_lhdcv5_encoder_init",
                                    "%s: Invalid samples per frame (%d)!", samples_per_frame, -8);
    func_ret = v5f_lhdc_v5_enc_get_encoded_frame_size(&encoded_frame_size, lhdcv5->lh5_enc);
    if (!func_ret)
        update_encoded_frame_size(lhdcv5, encoded_frame_size);
    else
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get frame size. (%d)!",
                       __func__, func_ret);
    lhdcv5->updateFramneInfo = false;
    v5f_cal_frame_size_and_frames_in_packet(lhdcv5);
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                   "%s: Init Encoder(%p) sampleRate = %u, bitrate = %u, bit per sample = %u, "
                   "Block size=%u, final_mtu=%u",
                   __func__, (void *)lhdcv5, lhdcv5->sample_rate, lhdcv5->lastBitrate,
                   lhdcv5->bits_per_sample, lhdcv5->samples_per_frame, lhdcv5->target_mtu_size);
    return LHDCV5_FRET_SUCCESS;
}

/* Bluetooth encoder wrapper: release child resources and clear the handle. */
int32_t v5f_lhdcv5_util_free_handle(lhdcv5_cb_t *handle) {
    int32_t func_ret;
    lhdcv5_para_t *lhdcv5;
    lhdcv5_ar_para_t *lhdcv5_ar_para;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    lhdcv5 = (handle->enc).lhdcv5;
    if (lhdcv5 != (lhdcv5_para_t *)0x0) {
        if (lhdcv5->lh5_enc != (void *)0x0) {
            lhdcv5->lh5_enc = (void *)0x0;
        }
        if (lhdcv5->cirbuff != (uint8_t *)0x0) {
            lhdcv5->cirbuff = (uint8_t *)0x0;
        }
        if (lhdcv5->enc_in_buf != (uint8_t *)0x0) {
            lhdcv5->enc_in_buf = (uint8_t *)0x0;
        }
        (handle->enc).lhdcv5 = (lhdcv5_para_t *)0x0;
    }
    lhdcv5_ar_para = handle->ar_filter;
    func_ret = LHDCV5_FRET_SUCCESS;
    if (lhdcv5_ar_para == (lhdcv5_ar_para_t *)0x0) {
        return func_ret;
    }
    if (lhdcv5_ar_para->pucARParams != (uint8_t *)0x0) {
        lhdcv5_ar_para->pucARParams = (uint8_t *)0x0;
    }
    if (lhdcv5_ar_para->pucLhdcArMem != (uint8_t *)0x0) {
        lhdcv5_ar_para->pucLhdcArMem = (uint8_t *)0x0;
    }
    func_ret = LHDCV5_FRET_SUCCESS;
    handle->ar_filter = (lhdcv5_ar_para_t *)0x0;
    return func_ret;
}

/* LHDC V5 encoder: clear object resource references. */
int32_t v5f_lhdcv5_encoder_free(lhdcv5_para_t *lhdcv5) {
    int32_t func_ret;
    if (lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    if (lhdcv5->lh5_enc != (void *)0x0) {
        lhdcv5->lh5_enc = (void *)0x0;
    }
    if (lhdcv5->cirbuff != (uint8_t *)0x0) {
        lhdcv5->cirbuff = (uint8_t *)0x0;
    }
    func_ret = LHDCV5_FRET_SUCCESS;
    if (lhdcv5->enc_in_buf == (uint8_t *)0x0) {
        return func_ret;
    }
    func_ret = LHDCV5_FRET_SUCCESS;
    lhdcv5->enc_in_buf = (uint8_t *)0x0;
    return func_ret;
}

/* LHDC V5 encoder: calculate object and partition memory requirements. */
int32_t v5f_lhdcv5_encoder_get_mem_req(uint32_t version, uint32_t *mem_req_bytes) {
    int32_t workspace_result;
    int32_t func_ret;
    int32_t lhdcv5_size;

    lhdcv5_size = 0;
    if (mem_req_bytes == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (version != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid version (%u)!", __func__,
                       version);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;
        return func_ret;
    }
    *mem_req_bytes = sizeof(lhdcv5_para_t) + 16;
    workspace_result = v5f_lhdc_v5_enc_workspace_get_size(2, &lhdcv5_size);
    if ((workspace_result == 0) && (0 < lhdcv5_size)) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *mem_req_bytes = lhdcv5_size + *mem_req_bytes + 0x1fe30;

        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Workspace size query failed (%d); bytes=%d", __func__, workspace_result,
                   lhdcv5_size);
    *mem_req_bytes = 0;
    func_ret = LHDCV5_FRET_ERROR;

    return func_ret;
}

/* LHDC V5 encoder: read frame length and sample count. */
int32_t v5f_lhdcv5_encoder_get_frame_len(lhdcv5_para_t *lhdcv5, uint32_t *samples_per_frame) {
    const char *log_format;
    uint32_t sample_rate;

    if (lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    if (lhdcv5->lh5_enc == (void *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_ENC;
    }
    if (samples_per_frame == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (lhdcv5->frame_duration != 0x32) {
        log_format = "%s: Invalid frame duration (%d)!";
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", log_format, __func__);
        return LHDCV5_FRET_CODEC_NOT_READY;
    }
    sample_rate = lhdcv5->sample_rate;
    if ((int32_t)sample_rate < LHDC_V5_SAMPLE_RATE_96000) {
        if ((sample_rate != 0xac44) && (sample_rate != LHDC_V5_SAMPLE_RATE_48000)) {
            log_format = "%s: Invalid sample rate (%d)!";
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", log_format, __func__);
            return LHDCV5_FRET_CODEC_NOT_READY;
        }
        if (lhdcv5->samples_per_frame == 0xf0) {
            sample_rate = 0xf0;
            *samples_per_frame = sample_rate;
            return LHDCV5_FRET_SUCCESS;
        }
    } else if (sample_rate == LHDC_V5_SAMPLE_RATE_96000) {
        if (lhdcv5->samples_per_frame == 0x1e0) {
            sample_rate = 0x1e0;
            *samples_per_frame = sample_rate;
            return LHDCV5_FRET_SUCCESS;
        }
    } else {
        if (sample_rate != 0x2ee00) {
            log_format = "%s: Invalid sample rate (%d)!";
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", log_format, __func__);
            return LHDCV5_FRET_CODEC_NOT_READY;
        }
        if (lhdcv5->samples_per_frame == 0x3c0) {
            sample_rate = 0x3c0;
            *samples_per_frame = sample_rate;
            return LHDCV5_FRET_SUCCESS;
        }
    }
    log_format = "%s: Invalid samples per frame (%d)!";
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", log_format, __func__);
    return LHDCV5_FRET_CODEC_NOT_READY;
}

/* Bluetooth encoder wrapper: read PCM samples per frame. */
int32_t v5f_lhdcv5_util_get_block_Size(lhdcv5_cb_t *handle, uint32_t *block_size) {
    int32_t operation_result;
    int32_t func_ret;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_CB;
    }
    if (block_size == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    if ((handle->enc).lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    operation_result = v5f_lhdcv5_encoder_get_frame_len((handle->enc).lhdcv5, block_size);
    func_ret = LHDCV5_FRET_SUCCESS;
    if (operation_result == 0) {
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get frame length (%d)!", __func__,
                   operation_result);
    return LHDCV5_FRET_ERROR;
}

/* Bluetooth encoder wrapper: calculate object and partition memory requirements. */
int32_t v5f_lhdcv5_util_get_mem_req(uint32_t version, uint32_t *mem_req_bytes) {
    if (!mem_req_bytes || version != 1)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    uint32_t mem_req_lhdcv5_encoder, mem_req_ar;
    v5f_lhdcv5_encoder_get_mem_req(1, &mem_req_lhdcv5_encoder);
#if LHDC_ENCODER_WITH_AR
    v5f_ar_process_get_mem_req(&mem_req_ar);
#else
    mem_req_ar = 0;
#endif
    *mem_req_bytes = mem_req_lhdcv5_encoder + mem_req_ar + sizeof(_lhdcv5_control_block_t) + 32;
    return LHDCV5_FRET_SUCCESS;
}
