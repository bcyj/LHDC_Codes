#include "../../../include/lhdc/decoder/status.h"
#include "../../common/lhdc_decoder_log.h"
#include "include/decoder_state.h"
#include "../../v4/include/lhdc_dec_core.h"
#include "../../v4/include/llac_init.h"
#include <stdio.h>
#include <string.h>
#include "../../../../common/include/lhdc_alignment.h"

/* Save the internal print_log_fp callback; NULL disables logging. */
void lhdc_register_log_cb(void *handle, LhdcV4LogCallback cb) {
    LhdcV4Decoder *dec_inst = handle;
    if (dec_inst)
        dec_inst->log_callback = cb;
}

/* Clear branch core addresses while retaining external memory and other fields. */
void lhdcDestroy(uint32_t *ptr) {
    LhdcV4Decoder *dec_inst = (void *)ptr;
    if (dec_inst && dec_inst->dec.llac.priv)
        dec_inst->dec.llac.priv = 0;
}

/* Return LHDC block length or LLAC output samples; log unknown branches and return zero. */
uint32_t lhdcGetSampleSize(uint32_t *ptr) {
    LhdcV4Decoder *dec_inst = (void *)ptr;
    if (dec_inst->type == 0)
        return dec_inst->dec.lhdc.block_size;
    if (dec_inst->type == 1)
        return dec_inst->dec.llac.output_samples;
    if (!(dec_inst->log_callback)) {
        return LHDC_DEC_NO_OUTPUT;
    }
    LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message,
                     "[LHDC.Decoder.V4] %s: Unsupported decoder type", __func__);
    return LHDC_DEC_NO_OUTPUT;
}

/* Parse headers; invalid LLAC split-channel headers still write frame length but preserve both channel-output fields. */
bool lhdcFetchFrameInfo(uint32_t *ptr, uint8_t *frameData, LhdcV4FrameInfo *frameInfo) {
    LhdcV4Decoder *dec_inst = (void *)ptr;
    uint32_t hdr;
    if (dec_inst->type == 0) {
        memcpy(&hdr, frameData, 4);
        frameInfo->frame_len = (hdr & 0xfffff) >> 8;
        frameInfo->isSplit = (hdr & 0x600000) == 0x600000;
        frameInfo->isLeft = (hdr & 15) == 0;
        return (hdr & 0xff000000) == 0x4c000000;
    }
    if (dec_inst->type == 1) {
        memcpy(&hdr, frameData, 4);
        uint32_t n =
            (hdr & 1023) * dec_inst->dec.llac.output_samples * 1000u / (dec_inst->sample_rate << 3);
        frameInfo->frame_len = n + 2;
        if (!(hdr & 0xc000)) {
            frameInfo->isSplit = 0;
            frameInfo->isLeft = 0;
            return (hdr & 1023) < 901 && (hdr & 0x3c0) != 0;
        }
        frameInfo->frame_len = n + 4;
        if (dec_inst->log_callback) {
            LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message,
                             "[LHDC.Decoder.V4] %s: LARC frames are not supported", __func__);
        }
    }
    return false;
}

/* Synchronize LLAC core and save the short channel enum; the wrapper does not reject modes above two. */
uint32_t lhdcChannelSelsect(uint32_t *ptr, uint8_t channel_type) {
    LhdcV4Decoder *dec_inst = (void *)ptr;
    if (dec_inst->type == 1)
        llac_dec_set_channel_mode(channel_type, dec_inst->dec.llac.priv);
    if (!(dec_inst->log_callback)) {
        dec_inst->channel_type = channel_type;
        return LHDC_DEC_OK;
    }
    LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message,
                     "[LHDC.Decoder.V4] %s: Channel config changed(%d -> %d)", __func__,
                     dec_inst->channel_type, channel_type);
    dec_inst->channel_type = channel_type;
    return LHDC_DEC_OK;
}

/* Query native-ABI wrapper bytes; unsupported versions still write the default requirement. */
uint32_t lhdc_util_dec_get_mem_req(uint16_t version, uint32_t *mem_req_bytes) {
    if (!mem_req_bytes)
        return (uint32_t)LHDC_DEC_INVALID_ARGUMENT;

    uint32_t ret = LHDC_DEC_OK,
             n = (uint32_t)lhdc_align_size(sizeof(LhdcV4Decoder), _Alignof(void *)) +
                 512 * sizeof(int32_t);
    int core = 0;
    if (version == 500 || version == 200 || version == 300 || version == 400) {
        core = version == 500 ? llac_dec_mem_get_size() : (int)lhdc_dec_mem_get_size();
        n = (uint32_t)lhdc_align_size(lhdc_align_size(sizeof(LhdcV4Decoder), _Alignof(void *)) +
                                          core,
                                      _Alignof(int32_t)) +
            512 * sizeof(int32_t);

    } else {
        ret = (uint32_t)LHDC_DEC_INVALID_ARGUMENT;
    }

    *mem_req_bytes = n;
    return ret;
}
