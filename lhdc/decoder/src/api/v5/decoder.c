#include "../../../include/lhdc/decoder/status.h"
#include "../../../../common/include/lhdc_alignment.h"
#include "../../../../../math/include/audio_math/byte_order.h"
#include "../../common/lhdc_decoder_log.h"
#include "include/decoder_state.h"
#include "../../v5/include/lhdc_workspace_layout.h"
#include "../../v5/include/lhdc_decode.h"
#include "../../v5/include/lhdc_dec_control.h"
#include <stdio.h>
#include <string.h>
#define V5_LOG(...) LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message, __VA_ARGS__)

/* Configure V5 audio format, align workspace, initialize the core, and accumulate errors.
 * Initialization continues after some failures and returns accumulated error status.
 */
int32_t lhdcv5_util_init_decoder(void *ptr, uint32_t bs, uint32_t sr, uint32_t audio_fmt,
                                 uint32_t frm_duration, uint32_t lossless_enable, uint16_t ver) {
    (void)lossless_enable;
    if (!ptr) {
        return LHDC_DEC_INVALID_ARGUMENT;
    }
    LhdcV5Decoder *dec_inst = ptr;
    memset(dec_inst, 0, sizeof(*dec_inst));
    V5_LOG("[LHDC.Decoder.V5] %s: Decoder instance bytes=%zu", __func__, sizeof(*dec_inst));
    dec_inst->sample_rate = sr;
    dec_inst->bits_per_samples = bs;
    dec_inst->target_bitrate = 400;
    dec_inst->audio_format = audio_fmt;
    int32_t ret = LHDC_DEC_OK;
    switch (audio_fmt) {
    case 2:
        dec_inst->channel_num = 1;
        break;
    case 3:
        dec_inst->channel_num = 2;
        break;
    case 0:
        dec_inst->channel_num = 2;
        dec_inst->interleaved = true;
        break;
    default:
        V5_LOG("[LHDC.Decoder.V5] %s: Unsupported audio format (%d)", __func__, (int)audio_fmt);
        ret = LHDC_DEC_INVALID_ARGUMENT;
        break;
    }
    if (ver != 550) {
        V5_LOG("[LHDC.Decoder.V5] %s: Unsupported version (%d)", __func__, (int)ver);
        return LHDC_DEC_INVALID_ARGUMENT;
    }
    dec_inst->type = 1;
    uint8_t *mem_ptr =
        lhdc_align_pointer((uint8_t *)ptr + sizeof(*dec_inst), _Alignof(LHDCV5_WorkspacePrefix));
    int workspace_bytes = 0;
    int ret_lhdcv5 = lhdc_v5_dec_workspace_get_size(dec_inst->channel_num, dec_inst->sample_rate,
                                                    50, 0, &workspace_bytes);
    if (ret_lhdcv5 != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 workspace get size error - (%d)", __func__,
               ret_lhdcv5);
        ret = LHDC_DEC_CODEC_ERROR;
    }
    if (frm_duration != 50) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 invalid frame duration - (%d)", __func__,
               (int)frm_duration);
        return LHDC_DEC_INVALID_ARGUMENT;
    }
    dec_inst->dec.priv = mem_ptr;
    ret_lhdcv5 =
        lhdc_v5_dec_workspace_init(dec_inst->channel_num, dec_inst->sample_rate, 50, 0, mem_ptr);
    if (ret_lhdcv5 != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 workspace init error - (%d)", __func__, ret_lhdcv5);
        ret = LHDC_DEC_CODEC_ERROR;
    }
    V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 workspace size: %d", __func__, workspace_bytes);
    mem_ptr = lhdc_align_pointer(mem_ptr + workspace_bytes, _Alignof(int32_t));
    int resolution = dec_inst->bits_per_samples == 32 ? 24 : dec_inst->bits_per_samples;
    ret_lhdcv5 = lhdc_v5_dec_init(dec_inst->channel_num, resolution, dec_inst->sample_rate,
                                  frm_duration, dec_inst->dec.priv);
    if (ret_lhdcv5 != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 dec init error - (%d)", __func__, ret_lhdcv5);
        ret = LHDC_DEC_CODEC_ERROR;
    }
    ret_lhdcv5 = lhdc_v5_dec_set_channel_mode(1, dec_inst->dec.priv);
    if (ret_lhdcv5 != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 set channel error - (%d)", __func__, ret_lhdcv5);
        ret = LHDC_DEC_CODEC_ERROR;
    }
    /* Preserve the previous query value if a failed query writes nothing, retaining partial initialization updates. */
    int frame_samples = workspace_bytes;
    ret_lhdcv5 = lhdc_v5_dec_get_samples_per_frame(&frame_samples, dec_inst->dec.priv);
    if (ret_lhdcv5 != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 get samples error - (%d)", __func__, ret_lhdcv5);
        ret = LHDC_DEC_CODEC_ERROR;
    }
    dec_inst->dec.output_samples = frame_samples;
    dec_inst->dec_buf = (int32_t *)mem_ptr;

    V5_LOG(
        "[LHDC.Decoder.V5] %s: Init Type: LHDC V5, sample rate:%d, bits per sample:%d, block size:%d",
        __func__, (int)dec_inst->sample_rate, (int)dec_inst->bits_per_samples, frame_samples);
    V5_LOG("[LHDC.Decoder.V5] %s: Init Type: LHDC V5, input data len:%d", __func__,
           (int)dec_inst->dec.input_bytes);
    return ret;
}

/* Write little-endian planar/interleaved PCM in channel-major order; 32-bit format left-aligns 24-bit samples. */
static uint32_t write_v5_pcm(uint8_t *output, const LhdcV5Decoder *state) {
    const int32_t *samples = state->dec_buf;
    uint32_t samples_per_frame = state->dec.output_samples;
    uint32_t channel_num = state->channel_num, bits_per_sample = state->bits_per_samples;
    bool interleaved = state->interleaved;
    uint32_t sample_bytes = bits_per_sample == 16 ? 2u : 4u;
    uint32_t output_bytes = samples_per_frame * channel_num * sample_bytes;
    if (samples_per_frame > INT32_MAX)
        return output_bytes;
    bool stereo_interleaved = interleaved && channel_num == 2;
    size_t output_stride = sample_bytes * (stereo_interleaved ? 2u : 1u);
    for (uint32_t channel = 0; channel < channel_num; ++channel) {
        const int32_t *input = samples + channel * samples_per_frame;
        size_t first_sample = stereo_interleaved ? channel : (size_t)channel * samples_per_frame;
        uint8_t *channel_output = output + first_sample * sample_bytes;
        for (uint32_t i = 0; i < samples_per_frame; ++i) {
            uint8_t *sample_output = channel_output + (size_t)i * output_stride;
            uint32_t sample = (uint32_t)input[i];
            if (sample_bytes == 2)
                audio_math_write_le16(sample_output, (uint16_t)sample);
            else {
                if (bits_per_sample == 32)
                    sample <<= 8;
                audio_math_write_le32(sample_output, sample);
            }
        }
    }
    return output_bytes;
}

/* Validate and decode frames, then convert planar/interleaved PCM by channel and depth.
 * Parse failure preserves OutLen; core failure still converts the internally cleared PCM.
 */
int32_t lhdcv5_util_dec_process(void *ptr, uint8_t *pOutBuf, uint8_t *pInput, uint32_t InLen,
                                uint32_t *OutLen) {
    if (!ptr) {
        return LHDC_DEC_OK;
    }
    LhdcV5Decoder *dec_inst = ptr;
    int encoded_frame_size = 0;
    if (lhdc_v5_dec_get_encoded_frame_size(pInput, &encoded_frame_size, dec_inst->dec.priv) != 0) {
        V5_LOG("[LHDC.Decoder.V5] %s: Parse input data error!", __func__);
        return LHDC_DEC_CODEC_ERROR;
    }
    if (InLen != (uint32_t)encoded_frame_size) {
        V5_LOG("[LHDC.Decoder.V5] %s: Input data error! frame_len(%d) != len(%d)", __func__,
               encoded_frame_size, (int)InLen);
        return LHDC_DEC_INVALID_ARGUMENT;
    }
    if (dec_inst->type != 1) {
        *OutLen = 0;
        return LHDC_DEC_OK;
    }
    memset(dec_inst->dec_buf, 0, dec_inst->channel_num * 960 * sizeof *dec_inst->dec_buf);
    int meta_flag = 0;
    uint8_t meta_data[8] = {0};
    encoded_frame_size = 0;
    lhdc_v5_dec_get_meta_flag_and_data(pInput, &meta_flag, meta_data);
    int ret_lhdcv5 = lhdc_v5_dec_decode(pInput, (int *)dec_inst->dec_buf, &encoded_frame_size,
                                        dec_inst->dec.priv);
    int32_t ret = ret_lhdcv5 == LHDC_DEC_OK ? LHDC_DEC_OK : LHDC_DEC_CODEC_ERROR;
    if (ret_lhdcv5 != 0)
        V5_LOG("[LHDC.Decoder.V5] %s: V5 decode failed - (%d)", __func__, ret_lhdcv5);
    /* Skip metadata and encoded-length logging on failure if no logger is registered. */
    if (ret_lhdcv5 == 0 || dec_inst->log_callback) {
        if (meta_flag == 1)
            V5_LOG("[LHDC.Decoder.V5] %s: meta_data = %d,%d,%d,%d,%d,%d,%d,%d", __func__,
                   meta_data[0], meta_data[1], meta_data[2], meta_data[3], meta_data[4],
                   meta_data[5], meta_data[6], meta_data[7]);
        V5_LOG("[LHDC.Decoder.V5] %s: lhdcv5 encoded_frame_size = %d", __func__,
               encoded_frame_size);
    }
    uint32_t out_len = write_v5_pcm(pOutBuf, dec_inst);
    V5_LOG("[LHDC.Decoder.V5] %s: Type LHDC V5 : Decode end (%d)", __func__, (int)out_len);
    *OutLen = out_len;
    return ret;
}
