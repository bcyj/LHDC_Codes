#include "../../../include/lhdc/decoder/status.h"
#include "../../../../common/include/lhdc_alignment.h"
#include <audio_math/byte_order.h>
#include "../../common/lhdc_decoder_log.h"
#include "include/decoder_state.h"
#include "../../v4/include/lhdc_dec_core.h"
#include "../../v4/include/llac_decode.h"
#include "../../v4/include/llac_memory.h"
#include <stdio.h>
#include <string.h>
#define V4_LOG(...) LHDC_DECODER_LOG(dec_inst->log_callback, dec_inst->log_message, __VA_ARGS__)

/* Initialize the integrated instance, ring input, and naturally aligned LHDC/LLAC workspaces. */
void lhdcInit(uint32_t *ptr, uint32_t bs, uint32_t sr, uint32_t bitrate, uint16_t ver) {
    if (!ptr) {
        return;
    }
    LhdcV4Decoder *dec_inst = (LhdcV4Decoder *)ptr;
    memset(dec_inst, 0, sizeof(*dec_inst));
    V4_LOG("[LHDC.Decoder.V4] %s: Decoder instance bytes=%zu", __func__, sizeof(*dec_inst));
    dec_inst->sample_rate = sr;
    dec_inst->bits_per_samples = bs;
    dec_inst->target_bitrate = bitrate;
    uint8_t *mem_ptr = lhdc_align_pointer((uint8_t *)ptr + sizeof(*dec_inst), _Alignof(void *));
    memset(dec_inst->cbuf_store, 0, sizeof(dec_inst->cbuf_store));
    lhdc_cirbuf_init_inline(&dec_inst->cbuf, dec_inst->cbuf_store, sizeof(dec_inst->cbuf_store));
    memset(dec_inst->dec_buf, 0, sizeof(dec_inst->dec_buf));

    if (ver == 500) {
        dec_inst->type = 1;
        int llac_mem_size = llac_dec_mem_get_size();
        V4_LOG("[LHDC.Decoder.V4] %s: llac dec memory size (%d)", __func__, llac_mem_size);
        dec_inst->dec.llac.priv = (void *)mem_ptr;
        llac_dec_mem_init(mem_ptr);
        if (llac_dec_initial(dec_inst->sample_rate, 2, (short)dec_inst->bits_per_samples, 5.0,
                             dec_inst->target_bitrate, (int *)&dec_inst->dec.llac.input_bytes,
                             (unsigned *)&dec_inst->dec.llac.output_samples,
                             dec_inst->dec.llac.priv) != LHDC_DEC_OK) {
            dec_inst->dec.llac.priv = NULL;
            dec_inst->type = 2;
            return;
        }
        V4_LOG(
            "[LHDC.Decoder.V4] %s: Init Type: LLAC, sample rate:%d, bits per sample:%d, block size:%d",
            __func__, (int)dec_inst->sample_rate, (int)dec_inst->bits_per_samples,
            (int)dec_inst->dec.llac.output_samples);
        V4_LOG("[LHDC.Decoder.V4] %s: Init Type: LLAC, input data len:%d", __func__,
               (int)dec_inst->dec.llac.input_bytes);
        dec_inst->dec_buf_out =
            (int32_t *)lhdc_align_pointer(mem_ptr + llac_mem_size, _Alignof(int32_t));
        return;
    }
    if (ver != 200 && ver != 300 && ver != 400) {
        V4_LOG("[LHDC.Decoder.V4] %s: Unsupported version (%d)", __func__, (int)ver);
        return;
    }
    uint32_t lhdc_mem_size = lhdc_dec_mem_get_size();
    dec_inst->dec.lhdc.priv = (void *)mem_ptr;
    dec_inst->dec.lhdc.ver_code = ver;
    dec_inst->dec.lhdc.block_size = ver == 200 ? 512 : 256;
    lhdc_dec_init(dec_inst->dec.lhdc.priv, (uint8_t)dec_inst->bits_per_samples,
                  dec_inst->dec.lhdc.block_size, &dec_inst->cbuf, (uint8_t *)dec_inst->dec_buf,
                  ver);
    V4_LOG(
        "[LHDC.Decoder.V4] %s: Init Type: LHDC, sample rate:%d, bits per sample:%d, block size:%d, version:%d",
        __func__, (int)dec_inst->sample_rate, (int)dec_inst->bits_per_samples,
        (int)dec_inst->dec.lhdc.block_size, (int)ver);
    dec_inst->dec_buf_out =
        (int32_t *)lhdc_align_pointer(mem_ptr + lhdc_mem_size, _Alignof(int32_t));
}

/* Write little-endian interleaved stereo: low halfwords for 16-bit, right-aligned 32-bit slots otherwise. */
static uint32_t write_v4_stereo(uint8_t *output, const int *left, const int *right,
                                uint32_t samples, uint32_t bits_per_samples) {
    uint32_t output_bytes = samples * (bits_per_samples == 16 ? 4u : 8u);
    if (samples > INT32_MAX)
        return output_bytes;
    for (uint32_t i = 0; i < samples; ++i) {
        if (bits_per_samples == 16) {
            audio_math_write_le16(output + (size_t)i * 4u, (uint16_t)left[i]);
            audio_math_write_le16(output + (size_t)i * 4u + 2u, (uint16_t)right[i]);
        } else {
            audio_math_write_le32(output + (size_t)i * 8u, (uint32_t)left[i]);
            audio_math_write_le32(output + (size_t)i * 8u + 4u, (uint32_t)right[i]);
        }
    }
    return output_bytes;
}

/* Validate frame length, decode the LHDC/LLAC branch, and output interleaved PCM.
 */
uint32_t lhdcDecodeProcess(uint32_t *ptr, uint8_t *pOutBuf, uint8_t *pInput, uint32_t len) {
    if (!ptr || !pOutBuf || !pInput || len < 4)
        return LHDC_DEC_NO_OUTPUT;
    LhdcV4Decoder *dec_inst = (LhdcV4Decoder *)ptr;
    if (!dec_inst->dec_buf_out)
        return LHDC_DEC_NO_OUTPUT;
    uint32_t header = audio_math_read_le32(pInput);
    if (dec_inst->type == 0) {
        if ((header & 0xff000000) != 0x4c000000) {
            V4_LOG("[LHDC.Decoder.V4] %s: Type LHDC : frame error!", __func__);
            return LHDC_DEC_NO_OUTPUT;
        }
        uint32_t frame_len = (header & 0xfffff) >> 8;
        if (len != frame_len) {
            V4_LOG("[LHDC.Decoder.V4] %s: Type LHDC : different len: frame_len(%d), len(%d)",
                   __func__, (int)frame_len, (int)len);
            return LHDC_DEC_NO_OUTPUT;
        }
        int *dec_dat[2];
        dec_dat[0] = memset(dec_inst->dec_buf_out, 0, 0x200);
        dec_dat[1] = dec_dat[0] + 0x100;
        cirbuf_s *pcb = &dec_inst->cbuf;
        lhdc_cirbuf_put(pcb, pInput, len);
        uint32_t out_len = 0;
        for (;;) {
            int ns = lhdc_dec_decode(dec_inst->dec.lhdc.priv, dec_dat);
            if (ns <= 0) {
                if (ns < 0) {
                    lhdc_cirbuf_reset_inline(pcb);
                    lhdc_dec_restart(dec_inst->dec.lhdc.priv, (uint8_t)dec_inst->bits_per_samples,
                                     dec_inst->dec.lhdc.block_size, pcb,
                                     (uint8_t *)dec_inst->dec_buf, dec_inst->dec.lhdc.ver_code);
                    V4_LOG("[LHDC.Decoder.V4] %s: Type LHDC : Decode error!(%d)", __func__, ns);
                }
                V4_LOG("[LHDC.Decoder.V4] %s: Type LHDC : Decode end (%d)", __func__, (int)out_len);
                return out_len;
            }
            uint8_t ucExtFlag, ucExtOffset, ucExtFunc, pucExtData[16];
            lhdc_chk_ext_header(dec_inst->dec.lhdc.priv, &ucExtFlag, &ucExtOffset, &ucExtFunc,
                                pucExtData, sizeof(pucExtData));
            V4_LOG("[LHDC.Decoder.V4] %s: JAS = %d, Meta = %d, AR = %d, ", __func__,
                   (ucExtFunc & 3) >> 1, (ucExtFunc & 7) >> 2, ucExtFunc & 1);
            if (dec_inst->channel_type == 1)
                memset(dec_dat[1], 0, ns * sizeof(int));
            else if (dec_inst->channel_type == 2)
                memset(dec_dat[0], 0, ns * sizeof(int));
            out_len += write_v4_stereo(pOutBuf + out_len, dec_dat[0], dec_dat[1], ns,
                                       dec_inst->bits_per_samples);
        }
    }
    if (dec_inst->type != 1)
        return LHDC_DEC_NO_OUTPUT;
    if (header & 0xc000) {
        V4_LOG("[LHDC.Decoder.V4] %s: LARC frames are not supported", __func__);
        V4_LOG("[LHDC.Decoder.V4] %s: Type LLAC : frame error!", __func__);
        return LHDC_DEC_NO_OUTPUT;
    }
    uint32_t bitrate = header & 0x3ff;
    if (bitrate > 900 || (bitrate & 0x3c0) == 0) {
        V4_LOG("[LHDC.Decoder.V4] %s: Type LLAC : frame error!", __func__);
        return LHDC_DEC_NO_OUTPUT;
    }
    uint32_t frame_len =
        dec_inst->dec.llac.output_samples * bitrate * 1000 / (dec_inst->sample_rate << 3) + 2;
    if (len != frame_len) {
        V4_LOG("[LHDC.Decoder.V4] %s: Type LLAC : different len: frame_len(%d), len(%d)", __func__,
               (int)frame_len, (int)len);
        return LHDC_DEC_NO_OUTPUT;
    }
    int *output_tmp = (int *)dec_inst->dec_buf_out;
    memset(output_tmp, 0, dec_inst->dec.llac.output_samples * 8);
    int bytes = llac_dec_process(pInput, output_tmp, dec_inst->dec.llac.priv);
    if (bytes < 1) {
        V4_LOG("[LHDC.Decoder.V4] %s: Type LLAC : decode error (%d)", __func__, bytes);
        return LHDC_DEC_NO_OUTPUT;
    }

    V4_LOG("[LHDC.Decoder.V4] %s: exth = %d, larc = %d, ar = %d", __func__,
           llac_dec_get_extra_func(2, dec_inst->dec.llac.priv),
           llac_dec_get_extra_func(1, dec_inst->dec.llac.priv),
           llac_dec_get_extra_func(0, dec_inst->dec.llac.priv));
    uint32_t out_len =
        write_v4_stereo(pOutBuf, output_tmp, output_tmp + dec_inst->dec.llac.output_samples,
                        dec_inst->dec.llac.output_samples, dec_inst->bits_per_samples);
    V4_LOG("[LHDC.Decoder.V4] %s: Type LLAC : Decode end (%d)", __func__, (int)out_len);
    return out_len;
}
