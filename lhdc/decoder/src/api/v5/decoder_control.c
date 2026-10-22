#include "../../../include/lhdc/decoder/status.h"
#include "../../common/lhdc_decoder_log.h"
#include "include/decoder_state.h"
#include "../../v5/include/lhdc_workspace_layout.h"
#include "../../../../common/include/lhdc_alignment.h"
#include "../../v5/include/lhdc_dec_control.h"
#include <stdio.h>

/* Format a log message and invoke the current callback. */
static void util_message(LhdcV5Decoder *dec_inst, const char *fn, const char *format) {
    if (dec_inst && dec_inst->log_callback) {
        LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message, format, fn);
    }
}

/* Save the log callback; NULL disables logging. */
void lhdcv5_util_dec_register_log_cb(void *handle, LhdcV5LogCallback cb) {
    LhdcV5Decoder *dec_inst = handle;
    if (dec_inst)
        dec_inst->log_callback = cb;
}

/* Clear the core address only for V5 instances. */
int32_t lhdcv5_util_dec_destroy(void *ptr) {
    LhdcV5Decoder *dec_inst = ptr;
    if (dec_inst && dec_inst->type == 1 && dec_inst->dec.priv)
        dec_inst->dec.priv = 0;
    return LHDC_DEC_OK;
}

/* Get instance sample count, preserving the two error codes. */
int32_t lhdcv5_util_dec_get_sample_size(void *ptr) {
    LhdcV5Decoder *dec_inst = ptr;
    if (!dec_inst) {
        util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Codec not initialized!");
        return LHDC_DEC_NOT_INITIALIZED;
    }
    if (dec_inst->type == 1) {
        return (int32_t)dec_inst->dec.output_samples;
    }
    util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Unsupported decoder type");
    return LHDC_DEC_UNSUPPORTED_CODEC;
}

/* Parse frame length; unknown types succeed without output changes, while parse failures still write all three fields. */
int32_t lhdcv5_util_dec_fetch_frame_info(void *ptr, uint8_t *frameData, uint32_t frameDataLen,
                                         LhdcV5FrameInfo *frameInfo) {
    LhdcV5Decoder *dec_inst = ptr;
    if (!dec_inst) {
        util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Codec not initialized!");
        return LHDC_DEC_NOT_INITIALIZED;
    }
    if (frameDataLen < 6) {
        util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Frame data is too small!");
        return LHDC_DEC_INVALID_ARGUMENT;
    }
    if (dec_inst->type != 1)
        return LHDC_DEC_OK;
    int n = 0, ret = lhdc_v5_dec_get_encoded_frame_size(frameData, &n, dec_inst->dec.priv);
    frameInfo->frame_len = (uint32_t)n;
    frameInfo->isSplit = 0;
    frameInfo->isLeft = 0;
    return ret ? LHDC_DEC_CODEC_ERROR : LHDC_DEC_OK;
}

/* Set channel selection, writing the wrapper enum even on underlying failure; reject stereo requests for mono early. */
int32_t lhdcv5_util_dec_channel_selsect(void *ptr, uint8_t channel_type) {
    LhdcV5Decoder *dec_inst = ptr;

    if (!dec_inst) {
        util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Codec not initialized!");
        return LHDC_DEC_NOT_INITIALIZED;
    }
    if (dec_inst->type != 1) {
        util_message(dec_inst, __func__, "[LHDC.Decoder.V5] %s: Unsupported decoder type");
        return LHDC_DEC_UNSUPPORTED_CODEC;
    }
    if (dec_inst->channel_num < 2 && channel_type == 0) {
        util_message(dec_inst, __func__,
                     "[LHDC.Decoder.V5] %s: Stereo selection requires two channels");
        return LHDC_DEC_CODEC_ERROR;
    }
    int ret = lhdc_v5_dec_set_channel_mode(channel_type, dec_inst->dec.priv);
    if (ret && dec_inst->log_callback) {
        LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message,
                         "[LHDC.Decoder.V5] %s: lhdcv5 set channel error - (%d)", __func__, ret);
    }
    if (!(dec_inst->log_callback)) {
        dec_inst->channel_type = channel_type;
        return ret ? LHDC_DEC_CODEC_ERROR : LHDC_DEC_OK;
    }
    LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message,
                     "[LHDC.Decoder.V5] %s: Channel config changed(%d -> %d)", __func__,
                     dec_inst->channel_type, channel_type);
    dec_inst->channel_type = channel_type;
    return ret ? LHDC_DEC_CODEC_ERROR : LHDC_DEC_OK;
}

/* Query wrapper size for the target ABI; lossless_enable is ignored and the core is queried with zero. */
int32_t lhdcv5_util_dec_get_mem_req(uint16_t version, uint32_t sampleRate, uint32_t audioFormat,
                                    uint32_t lossless_enable, uint32_t *mem_req_bytes) {
    (void)lossless_enable;

    if (!mem_req_bytes)
        return LHDC_DEC_INVALID_ARGUMENT;
    uint32_t n = (uint32_t)lhdc_align_size(sizeof(LhdcV5Decoder), _Alignof(LHDCV5_WorkspacePrefix));
    int channels = 0, ret = LHDC_DEC_OK;
    if (audioFormat == 2) {
        channels = 1;
        n = (uint32_t)lhdc_align_size(sizeof(LhdcV5Decoder), _Alignof(LHDCV5_WorkspacePrefix)) +
            960 * sizeof(int32_t);
    } else if (audioFormat == 3 || audioFormat == 0) {
        channels = 2;
        n = (uint32_t)lhdc_align_size(sizeof(LhdcV5Decoder), _Alignof(LHDCV5_WorkspacePrefix)) +
            2 * 960 * sizeof(int32_t);
    } else {
        ret = LHDC_DEC_INVALID_ARGUMENT;
    }
    if (version == 550) {
        int size = 0,
            core = lhdc_v5_dec_workspace_get_size(channels, (int)sampleRate, 50, 0, &size);
        if (core) {
            ret = LHDC_DEC_CODEC_ERROR;
        }
        n = (uint32_t)lhdc_align_size(n + size, _Alignof(int32_t));
    } else {
        ret = LHDC_DEC_INVALID_ARGUMENT;
    }

    *mem_req_bytes = n;
    return ret;
}
