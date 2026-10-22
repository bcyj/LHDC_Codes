#include "lhdc_memory.h"
#include "../../../../common/include/lhdc_log.h"
#include <audio_math/float_convert.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../v4/include/encoder_internal.h"
#include "../../v4/include/codec/codec_internal.h"
#include <string.h>

/* Set LHDC sample format, target bitrate, and MTU framing policy, then initialize encoder state. */
int32_t v4f_lhdc_encoder_init(lhdc_para_t *handle, int32_t sampling_freq, uint32_t bitPerSample,
                              uint32_t bitrate_inx, uint32_t dualChannel, uint32_t need_padding,
                              uint32_t mtu, uint32_t interval) {
    handle->sample_rate = sampling_freq;
    handle->bits_per_sample = (int32_t)bitPerSample;
    handle->encode_interval = interval;
    int32_t limit = handle->hasMinBitrateLimit ? 320 : 128;
    int32_t bitrate;
    if (bitrate_inx == 8) {
        v4f_lhdc_util_reset_up_bitrate(ENC_TYPE_LHDC, handle);
        v4f_lhdc_util_reset_down_bitrate(ENC_TYPE_LHDC, handle);
        bitrate = 400;
    } else {
        bitrate = (int32_t)v4f_lhdc_util_get_bitrate(bitrate_inx);
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
                       "%s: bitRateTable[bitrate_inx] = %d, limit = %d", __func__, bitrate, limit);
        if (bitrate < limit)
            bitrate = limit;
    }
    handle->lastBitrate = bitrate;
    handle->qualityStatus = (LHDCBT_QUALITY_T)bitrate_inx;
    if (bitrate_inx != 8 && (uint32_t)handle->limitBitRateStatus < bitrate_inx) {
        bitrate = (int32_t)v4f_lhdc_util_get_bitrate(handle->limitBitRateStatus);
        handle->lastBitrate = bitrate < limit ? limit : bitrate;
        handle->qualityStatus = handle->limitBitRateStatus;
    }
    handle->host_mtu_size = mtu;
    int32_t version_code;
    if (handle->version < 2) {
        handle->block_size = LHDC_V4_FRAME_SAMPLES_LEGACY;
        handle->target_mtu_size = mtu;
        version_code = LHDC_V4_STREAM_VERSION_CODE_200;
    } else {
        handle->block_size = LHDC_V4_FRAME_SAMPLES_V3_V4;
        version_code = handle->version == 3 ? LHDC_V4_STREAM_VERSION_CODE_400
                                            : LHDC_V4_STREAM_VERSION_CODE_300;
        handle->updateFramneInfo = false;
        v4f_cal_frame_size_and_frames_in_packet(handle);
        if (handle->input_cbuf.cbuf)
            lhdc_cirbuf_reset_inline(&handle->input_cbuf);
    }
    v4f_LossyEncoderInit(handle->fft_blk, handle->sample_rate, handle->bits_per_sample, 2,
                         (int32_t)handle->block_size, LHDC_V4_DEFAULT_SINK_BUFFER_BYTES,
                         handle->lastBitrate * 125, 0, (int32_t)dualChannel, (int32_t)need_padding,
                         LHDC_V4_FRAME_SAMPLES_V3_V4, (int32_t)handle->target_mtu_size, 5, 2,
                         version_code);
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
                   "%s: Init Encoder(%p) sampleRate = %d, bitrate = %d, bit per sample = %d, "
                   "version code=%d, Block size=%u, final_mtu=%u",
                   __func__, (void *)handle, handle->sample_rate, handle->lastBitrate,
                   handle->bits_per_sample, version_code, handle->block_size,
                   handle->target_mtu_size);
    return LHDC_V4_STATUS_OK;
}

/* Queue PCM in the ring, encode once enough packet frames exist, and copy the host-MTU-sized data. */
int32_t v4f_lhdc_encoder_encodeV3(lhdc_para_t *handle, void *p_pcm, uint8_t *out_put,
                                  uint32_t *written, uint32_t *out_fraems) {
    if (!handle) {
        lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4", "%s: Handle error!(%p)", __func__,
                       (void *)0);
        return LHDC_V4_NO_OUTPUT_BYTES;
    }
    if (!p_pcm || !out_put) {
        lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4",
                       "%s: Buffer error! source(%p), output(%p)", __func__, p_pcm,
                       (void *)out_put);
        return LHDC_V4_NO_OUTPUT_BYTES;
    }
    *written = 0;
    *out_fraems = 0;
    int32_t bytes_per_frame =
        (int32_t)(((uint32_t)handle->bits_per_sample << 6) & UINT32_C(0xfffffe00));
    if (bytes_per_frame <= 0 || (uint32_t)bytes_per_frame > sizeof handle->pcm_frame)
        return LHDC_V4_NO_OUTPUT_BYTES;
    uint8_t *in_tmp = handle->pcm_frame;
    if (handle->updateFramneInfo) {
        v4f_cal_frame_size_and_frames_in_packet(handle);
        v4f_LossyEncoderUpdateFrameSize(handle->fft_blk, (int32_t)handle->target_mtu_size,
                                        handle->lastBitrate * 125);
        handle->updateFramneInfo = false;
    }
    lhdc_cirbuf_put(&handle->input_cbuf, p_pcm, bytes_per_frame);
    uint32_t required = handle->frame_per_packet > 16 ? 16 : handle->frame_per_packet;
    if ((uint32_t)lhdc_cirbuf_len_inline(&handle->input_cbuf) <
        required * (uint32_t)bytes_per_frame)
        return LHDC_V4_NO_OUTPUT_BYTES;
    uint8_t *enc_tmp = handle->packet_scratch;
    memset(enc_tmp, 0, sizeof handle->packet_scratch);
    uint32_t encodedSize = 0, frames = 0;
    while (frames < handle->frame_per_packet) {
        lhdc_cirbuf_get(&handle->input_cbuf, in_tmp, bytes_per_frame);
        uint32_t n = (uint32_t)v4f_LossyEncoderProcessWav(handle->fft_blk, in_tmp, 256, 0,
                                                          enc_tmp + encodedSize, bytes_per_frame);
        if (!n)
            return LHDC_V4_NO_OUTPUT_BYTES;
        if (n > handle->target_mtu_size)
            lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4",
                           "Encoded size not accurate!(target:%u, real:%u)",
                           handle->target_mtu_size, n);
        ++frames;
        encodedSize += n;
    }
    *written = encodedSize;
    *out_fraems = frames;

    memcpy(out_put, enc_tmp, handle->host_mtu_size);
    return (int32_t)encodedSize;
}

#include <stdlib.h>

/* Create the LLAC API handle, default format/quality, and contiguous core workspace. */
llac_para_t *v4f_llac_encoder_new(void) {
    return v4f_llac_encoder_new_with_allocator(NULL);
}

/* Obtain LLAC handle, core, and FIFO from the host; reuse all storage during processing. */
llac_para_t *v4f_llac_encoder_new_with_allocator(const LhdcAllocator *allocator) {
    llac_para_t *llac = lhdc_memory_alloc(allocator, sizeof *llac);
    if (!llac)
        return NULL;
    memset(llac, 0, sizeof *llac);
    llac->sample_rate = LHDC_V4_SAMPLE_RATE_96000;
    llac->bits_per_sample = 24;
    llac->qualityStatus = LHDCBT_QUALITY_MID;
    llac->limitBitRateStatus = LHDCBT_QUALITY_HIGH;
    llac->lastBitrate = 560;
    llac->encode_interval = 20;
    llac->lh4_enc = v4f_llac_enc_mem_alloc_with_allocator(allocator);
    uint8_t *fifo = lhdc_memory_alloc(allocator, LHDC_V4_INPUT_FIFO_BYTES);
    if (!llac->lh4_enc || !fifo) {
        lhdc_memory_free(fifo);
        v4f_llac_encoder_free(llac);
        return NULL;
    }
    lhdc_cirbuf_init_inline(&llac->input_cbuf, fifo, LHDC_V4_INPUT_FIFO_BYTES);
    return llac;
}

/* Release this LLAC instance's workspace, input ring, and API handle. */
void v4f_llac_encoder_free(llac_para_t *llac) {
    if (!llac)
        return;
    if (llac->lh4_enc)
        v4f_llac_enc_mem_free(llac->lh4_enc);
    lhdc_memory_free(llac->input_cbuf.cbuf);
    lhdc_memory_free(llac);
}

/* Create the LHDC handle/spectral encoder; versions above one allocate a 32 KiB input ring. */
lhdc_para_t *v4f_lhdc_encoder_new(int32_t version) {
    return v4f_lhdc_encoder_new_with_allocator(version, NULL);
}

/* Construct LHDC objects through the specified host, releasing earlier objects on any partition failure. */
lhdc_para_t *v4f_lhdc_encoder_new_with_allocator(int32_t version, const LhdcAllocator *allocator) {
    if (version >= 5) {
        lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4",
                       "%s: Unsupported codec version (%d); maximum is 4", __func__, version);
        return NULL;
    }
    lhdc_para_t *handle = lhdc_memory_alloc(allocator, sizeof *handle);
    if (!handle)
        return NULL;
    memset(handle, 0, sizeof *handle);
    handle->fft_blk = v4f_LossyEncoderNew_with_allocator(version, allocator);
    if (!handle->fft_blk) {
        lhdc_memory_free(handle);
        return NULL;
    }
    handle->sample_rate = LHDC_V4_SAMPLE_RATE_96000;
    handle->bits_per_sample = 24;
    handle->qualityStatus = LHDCBT_QUALITY_MID;
    handle->limitBitRateStatus = LHDCBT_QUALITY_HIGH;
    handle->encode_interval = 20;
    handle->lastBitrate = 560;
    handle->version = version;
    if (version >= 2) {
        uint8_t *fifo = lhdc_memory_alloc(allocator, LHDC_V4_INPUT_FIFO_BYTES);
        if (!fifo) {
            v4f_lhdc_encoder_free(handle);
            return NULL;
        }
        lhdc_cirbuf_init_inline(&handle->input_cbuf, fifo, LHDC_V4_INPUT_FIFO_BYTES);
    }
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4", "%s: get LHDC instance(%p), version(%d)",
                   __func__, (void *)handle, version);
    return handle;
}

/* Release this LHDC instance's spectral workspace and input buffer. */
void v4f_lhdc_encoder_free(lhdc_para_t *handle) {
    if (!handle)
        return;
    if (handle->fft_blk) {
        v4f_LossyEncoderDelete(handle->fft_blk);
        handle->fft_blk = NULL;
    }
    lhdc_memory_free(handle->input_cbuf.cbuf);
    lhdc_memory_free(handle);
}

/* Set LLAC format, bitrate, packet capacity, and instance extension mode, then rebuild input buffering. */
int32_t v4f_llac_encoder_init(llac_para_t *handle, uint32_t sample_rate, uint32_t bits_per_sample,
                              uint32_t bitrate_inx, uint32_t mtu_size, uint32_t encode_interval) {
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4", "%s:%p", __func__, (void *)handle);
    if (!handle)
        return LHDC_V4_STATUS_ERROR;
    int32_t bitrate;
    if (bitrate_inx == 8) {
        bitrate = 400;
        handle->upBitrateSum = handle->upBitrateCnt = 0;
        handle->dnBitrateSum = handle->dnBitrateCnt = 0;
    } else
        bitrate = (int32_t)v4f_lhdc_util_get_bitrate(bitrate_inx);
    handle->lastBitrate = bitrate;
    handle->qualityStatus = (LHDCBT_QUALITY_T)bitrate_inx;
    if (bitrate_inx != 8 && (uint32_t)handle->limitBitRateStatus < bitrate_inx) {
        handle->qualityStatus = handle->limitBitRateStatus;
        handle->lastBitrate = (int32_t)v4f_lhdc_util_get_bitrate(handle->limitBitRateStatus);
    }
    if (!handle->mode.init_calls) {
        if (handle->lastBitrate == 900 || handle->lastBitrate == 600) {
            lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4", "%s: Extra function mode setting! ",
                           __func__);
            if (bits_per_sample == LHDC_V4_PCM_BITS_S16 && handle->mode.extra_func_mode)
                --handle->mode.extra_func_mode;
            else if (bits_per_sample == LHDC_V4_PCM_BITS_S24 && handle->mode.extra_func_mode < 7)
                ++handle->mode.extra_func_mode;
            handle->mode.ar_enabled = handle->mode.extra_func_mode & 1;
            handle->mode.larc_enabled = (handle->mode.extra_func_mode >> 1) & 1;
            handle->mode.exth_enabled = (handle->mode.extra_func_mode >> 2) & 1;
        } else if (handle->lastBitrate == 64) {
            lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
                           "%s: Automatic bitrate mode setting! ", __func__);
            if (bits_per_sample == LHDC_V4_PCM_BITS_S16)
                handle->mode.autobitrate_enabled = 0;
            else if (bits_per_sample == LHDC_V4_PCM_BITS_S24)
                handle->mode.autobitrate_enabled = 1;
        }
        ++handle->mode.init_calls;
    } else
        handle->mode.init_calls = 0;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
                   "%s: automatic bitrate = %d, mode = %d, exth = %d, larc = %d, ar = %d ",
                   __func__, handle->mode.autobitrate_enabled, handle->mode.extra_func_mode,
                   handle->mode.exth_enabled, handle->mode.larc_enabled, handle->mode.ar_enabled);
    if (v4f_llac_enc_initial(sample_rate, 2, (int16_t)bits_per_sample, (int16_t)bits_per_sample,
                             5.0f, handle->lastBitrate * 1000, &handle->out_nbytes,
                             &handle->samples_per_frame, &handle->real_bitrate,
                             handle->lh4_enc) != LLAC_OK)
        return LHDC_V4_STATUS_ERROR;
    lhdc_cirbuf_reset_inline(&handle->input_cbuf);
    int32_t bytes_per_sample = (int32_t)bits_per_sample / 8 * 2;
    uint32_t bytes_per_tick = ((uint32_t)bytes_per_sample * sample_rate * encode_interval) / 1000;
    uint32_t packets_per_tick = mtu_size ? (uint32_t)handle->out_nbytes / mtu_size : 0;
    if ((uint32_t)handle->out_nbytes < mtu_size)
        packets_per_tick = 1;
    uint32_t frames_per_packet = 0;
    if (packets_per_tick) {
        float ratio =
            (float)bytes_per_tick / (float)(handle->samples_per_frame * (uint32_t)bytes_per_sample);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* ratio has rounded to binary32; add one half in double, then truncate toward zero. */
        frames_per_packet =
            (uint32_t)audio_math_truncate_i32_f64((double)ratio + 0.5) / packets_per_tick;
#else
        frames_per_packet = (uint32_t)(int32_t)((double)ratio + 0.5) / packets_per_tick;
#endif
    }
    handle->encode_interval = encode_interval;
    handle->sample_rate = sample_rate;
    handle->bits_per_sample = bits_per_sample;
    handle->host_mtu_size = mtu_size;
    handle->frame_per_packet = frames_per_packet;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4", "%s: max_mtu_limit(%u)", __func__, mtu_size);

    while (mtu_size <= frames_per_packet * (uint32_t)handle->out_nbytes)
        frames_per_packet = (frames_per_packet & 1) && frames_per_packet > 1
                                ? frames_per_packet - 1
                                : frames_per_packet >> 1;
    handle->frame_per_packet = frames_per_packet;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
                   "%s: sample rate:%u, bits per sample:%u, target bitrate:%d, mtu size:%u",
                   __func__, sample_rate, bits_per_sample, handle->lastBitrate * 1000, mtu_size);
    lhdc_log_print(
        LHDC_LOG_DEBUG, "LHDC.Encoder.V4",
        "%s: output bytes:%d, samples per frame:%u, real bitrate:%d, frames per packet:%u",
        __func__, handle->out_nbytes, handle->samples_per_frame, handle->real_bitrate,
        handle->frame_per_packet);
    return LHDC_V4_STATUS_OK;
}

#include <stdint.h>

/* Query AR by codec type, process PCM in place, then perform buffered LHDC/LLAC encoding. */
int32_t v4f_lhdc_util_encv4_process(lhdc_cb_t *handle, void *p_pcm, uint8_t *out_put,
                                    uint32_t *written, uint32_t *out_fraems) {
    if (!handle) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V4", "%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    int32_t result = LHDC_V4_STATUS_OK;
    bool ar_enabled = false;
    if (handle->enc_type == ENC_TYPE_LHDC)
        ar_enabled = v4f_LhdcGetExtFuncState(handle->enc.lhdc->fft_blk, EXT_FUNC_AR);
    else if (handle->enc_type == ENC_TYPE_LLAC)
        ar_enabled = v4f_llac_enc_get_extra_func(0, handle->enc.llac->lh4_enc);
    if (ar_enabled) {
        lhdc_ar_para_t *ar = handle->ar_filter;
        const char *error = NULL;
        if (!ar)
            error = "%s: AR instance is NULL";
        else if (!p_pcm)
            error = "%s: AR PCM buffer is NULL";
        else if (!ar->ulNumBytes)
            error = "%s: AR PCM byte count is zero";
        if (error) {
            lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4", error, __func__);
            return LHDC_V4_STATUS_ERROR;
        }
        result =
            v4f_ProcessLhdcAR(p_pcm, p_pcm, ar->ulNumBytes, ar->pucLhdcArMem, ar->uiLhdcArMemBytes);
        if (result < 0) {
            lhdc_log_print(LHDC_LOG_ERROR, "LHDC.Encoder.V4", "%s: AR process error(%d)", __func__,
                           result);
            return result;
        }
    }
    if (handle->enc_type == ENC_TYPE_LHDC)
        return v4f_lhdc_encoder_encodeV3(handle->enc.lhdc, p_pcm, out_put, written, out_fraems);
    if (handle->enc_type != ENC_TYPE_LLAC)
        return result;
    llac_para_t *llacBT = handle->enc.llac;
    *out_fraems = 0;
    *written = 0;
    if (!llacBT->lh4_enc)
        return LHDC_V4_ENCODER_NOT_READY;
    uint32_t bytes_per_frame = llacBT->samples_per_frame * 8u;
    if (bytes_per_frame > sizeof llacBT->input_scratch)
        return LHDC_V4_STATUS_ERROR;
    int32_t *in_tmp = llacBT->input_scratch;
    uint32_t samples = llacBT->samples_per_frame;
    if (llacBT->bits_per_sample == LHDC_V4_PCM_BITS_S24) {
        const uint8_t *pcm = p_pcm;
        for (int32_t i = 0; i < (int32_t)samples; ++i) {
            uint32_t left =
                ((uint32_t)pcm[0] << 8) | ((uint32_t)pcm[1] << 16) | ((uint32_t)pcm[2] << 24);
            uint32_t right =
                ((uint32_t)pcm[3] << 8) | ((uint32_t)pcm[4] << 16) | ((uint32_t)pcm[5] << 24);
            in_tmp[i] = (int32_t)left >> 8;
            in_tmp[samples + (uint32_t)i] = (int32_t)right >> 8;
            pcm += 6;
        }
    } else {
        const int16_t *pcm = p_pcm;
        for (int32_t i = 0; i < (int32_t)samples; ++i) {
            in_tmp[i] = pcm[2 * i];
            in_tmp[samples + (uint32_t)i] = pcm[2 * i + 1];
        }
    }
    lhdc_cirbuf_put(&llacBT->input_cbuf, (void *)in_tmp, (int32_t)bytes_per_frame);
    uint32_t frame_per_packet = llacBT->frame_per_packet > 16 ? 16 : llacBT->frame_per_packet;
    if (frame_per_packet * bytes_per_frame <=
        (uint32_t)lhdc_cirbuf_len_inline(&llacBT->input_cbuf)) {
        if (llacBT->frame_per_packet > 16 || llacBT->out_nbytes < 0 ||
            (size_t)llacBT->frame_per_packet * (uint32_t)llacBT->out_nbytes >
                sizeof llacBT->packet_scratch)
            return LHDC_V4_STATUS_ERROR;
        size_t allocation = sizeof llacBT->packet_scratch;
        uint8_t *out_tmp = llacBT->packet_scratch;
        memset(out_tmp, 0, allocation);
        uint32_t out_frames_cnt = 0, enc_bytes_count = 0;
        while (out_frames_cnt < llacBT->frame_per_packet) {
            lhdc_cirbuf_get(&llacBT->input_cbuf, (void *)in_tmp, (int32_t)bytes_per_frame);
            int32_t processedBytes =
                v4f_llac_enc_process(in_tmp, out_tmp + enc_bytes_count, llacBT->lh4_enc);
            if (processedBytes < 0)
                return LHDC_V4_STATUS_ERROR;
            if (processedBytes != llacBT->out_nbytes)
                break;
            ++out_frames_cnt;
            enc_bytes_count += (uint32_t)processedBytes;
        }
        *written = enc_bytes_count;
        *out_fraems = out_frames_cnt;
        memcpy(out_put, out_tmp, enc_bytes_count);
    }

    return LHDC_V4_STATUS_OK;
}

/* Read the bitrate-index table; callers validate index bounds. */
uint32_t v4f_lhdc_util_get_bitrate(uint32_t index) {
    static const int32_t bitRateTable[9] = {

        64, 128, 192, 256, 320, 400, 500, 900, 1000};
    return (uint32_t)bitRateTable[index];
}

/* Return the specified instance's input buffer by codec type. */
uint8_t *v4f_lhdc_util_get_buffer(lhdc_enc_type_t type, void *handle) {
    if (!handle)
        return NULL;
    if (type == ENC_TYPE_LHDC)
        return ((lhdc_para_t *)handle)->input_cbuf.cbuf;
    if (type == ENC_TYPE_LLAC)
        return ((llac_para_t *)handle)->input_cbuf.cbuf;
    return NULL;
}

/* Dispatch LHDC/LLAC extension-state queries through the enc_t double handle. */
bool v4f_lhdc_util_get_ext_func_state(lhdc_enc_type_t type, void *h, lhdc_ext_func_t ext_type) {
    enc_t *enc = h;
    if (type == ENC_TYPE_LLAC)
        return v4f_llac_enc_get_extra_func((uint8_t)ext_type, enc->llac->lh4_enc);
    if (type == ENC_TYPE_LHDC)
        return v4f_LhdcGetExtFuncState(enc->lhdc->fft_blk, ext_type);
    return false;
}

#include <math.h>

/* Clear LHDC/LLAC bitrate-increase statistics while preserving structure padding. */
void v4f_lhdc_util_reset_up_bitrate(lhdc_enc_type_t type, void *h) {
    if (type == ENC_TYPE_LHDC) {
        lhdc_para_t *p = h;
        p->upBitrateCnt = 0;
        p->upBitrateSum = 0;
    } else if (type == ENC_TYPE_LLAC) {
        llac_para_t *p = h;
        p->upBitrateCnt = 0;
        p->upBitrateSum = 0;
    }
}

/* Clear LHDC/LLAC bitrate-decrease statistics. */
void v4f_lhdc_util_reset_down_bitrate(lhdc_enc_type_t type, void *h) {
    if (type == ENC_TYPE_LHDC) {
        lhdc_para_t *p = h;
        p->dnBitrateCnt = 0;
        p->dnBitrateSum = 0;
    } else if (type == ENC_TYPE_LLAC) {
        llac_para_t *p = h;
        p->dnBitrateCnt = 0;
        p->dnBitrateSum = 0;
    }
}

/* Return LLAC samples per frame, distinguishing null handles from absent workspace. */
int32_t v4f_llac_encoder_get_frame_len(llac_para_t *handle) {
    return !handle            ? LHDC_V4_STATUS_ERROR
           : !handle->lh4_enc ? LHDC_V4_FRAME_QUERY_ENCODER_NOT_READY
                              : (int32_t)handle->samples_per_frame;
}

/* Return actual LLAC bitrate; null workspace returns -3. */
int32_t v4f_llac_encoder_get_target_bitrate(llac_para_t *handle) {
    return !handle            ? LHDC_V4_STATUS_ERROR
           : !handle->lh4_enc ? LHDC_V4_ENCODER_NOT_READY
                              : handle->real_bitrate;
}

/* Convert LHDC's saved kbps target to bps. */
int32_t v4f_lhdc_encoder_get_target_bitrate(lhdc_para_t *handle) {
    return handle ? (int32_t)((uint32_t)handle->lastBitrate * 1000u) : LHDC_V4_STATUS_ERROR;
}

/* Query block sample count: 256 for versions above one, otherwise 512. */
int32_t v4f_lhdc_encoder_get_frame_len(lhdc_para_t *handle) {
    return handle
               ? (handle->version > 1 ? LHDC_V4_FRAME_SAMPLES_V3_V4 : LHDC_V4_FRAME_SAMPLES_LEGACY)
               : LHDC_V4_NO_SAMPLES;
}

/* Compute packet frame count and per-frame budget from PCM frame length, bitrate, interval, and host MTU. */
void v4f_cal_frame_size_and_frames_in_packet(lhdc_para_t *handle) {
    uint32_t bytes_per_sample = (uint32_t)(handle->bits_per_sample / 8) * 2;
    uint32_t bytes_per_frame = handle->block_size * bytes_per_sample;
    uint32_t target_bytes_per_second = (uint32_t)handle->lastBitrate * 125;
    uint32_t bytes_per_second = (uint32_t)handle->sample_rate * bytes_per_sample;
    uint32_t bytes_per_tick = (bytes_per_second * handle->encode_interval) / 1000;
    float max_output_bytes_per_frame =
        ((float)target_bytes_per_second / (float)bytes_per_second) * (float)bytes_per_frame;
    uint32_t frames_per_tick = (uint32_t)((float)bytes_per_tick / (float)bytes_per_frame + .5f);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Scheduled frame count need not be a power of two; multiply the two binary32 operands in extended precision. */
    uint32_t max_output_bytes_per_tick = (uint32_t)audio_math_truncate_i32_f64(
        (double)max_output_bytes_per_frame * (double)(float)frames_per_tick);
#else
    uint32_t max_output_bytes_per_tick =
        (uint32_t)(int32_t)(max_output_bytes_per_frame * (float)frames_per_tick);
#endif
    uint32_t max_mtu_limit = handle->host_mtu_size;
    uint32_t packet_per_tick = max_mtu_limit ? max_output_bytes_per_tick / max_mtu_limit : 0;
    if (max_output_bytes_per_tick < max_mtu_limit)
        packet_per_tick = 1;
    uint32_t frames = packet_per_tick ? frames_per_tick / packet_per_tick : 0;
    while ((float)max_mtu_limit <= max_output_bytes_per_frame * (float)frames) {
        frames = (frames & 1) && frames > 1 ? frames - 1 : frames >> 1;
    }
    handle->frame_per_packet = frames;
    handle->target_mtu_size = (float)max_mtu_limit <= max_output_bytes_per_frame * (float)frames
                                  ? (frames ? max_mtu_limit / frames : 0)
                                  : (uint32_t)max_output_bytes_per_frame;
}
