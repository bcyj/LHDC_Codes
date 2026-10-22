#include <audio_math/float_properties.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "../../../../common/include/lhdc_log.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <string.h>

/* Sum target output bytes across channels. */
LLAC_INT v4f_llac_target_bytes(const LLAC_Enc *encoder) {
    uint32_t num_bytes = 0;
    if (encoder)
        for (LLAC_INT ch = 0; ch < encoder->channels; ++ch)
            num_bytes += (uint32_t)encoder->channel_setup[ch]->targetBytes;
    return (LLAC_INT)num_bytes;
}

/* Compute actual bitrate from target bytes; return 12 when bitrate is unset. */
LLAC_INT v4f_llac_target_bitrate(const LLAC_Enc *encoder) {
    if (!encoder)
        return LHDC_V4_NO_BITRATE;
    if (!encoder->llac_br_set)
        return LLAC_BITRATE_UNSET_ERROR;
    LLAC_INT total = (LLAC_INT)((uint32_t)v4f_llac_target_bytes(encoder) * 80000u);
    LLAC_INT bitrate = encoder->frame_dms ? total / encoder->frame_dms : 0;
    if (encoder->fs_in == LHDC_V4_SAMPLE_RATE_44100) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Within valid bitrate ranges, retain x87 double quotient before truncating with common bit-field conversion. */
        bitrate = audio_math_truncate_i32_f64(((double)bitrate * 220.5) / 220.0);
#else
        bitrate = (LLAC_INT)(((double)bitrate * 220.5) / 220.0);
#endif
    }
    return bitrate;
}

/* Set LLAC extension-header AR/JAS/META flags, storing enable as supplied. */
void v4f_llac_enc_set_extra_func(LLAC_UINT8 func_index, LLAC_UINT8 func_enable,
                                 LLAC_UINT8 *llac_enc_mem_addr) {
    uint16_t *extra_nbyte_addr = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte;
    if (func_index < 3)
        *extra_nbyte_addr = (uint16_t)((*extra_nbyte_addr & ~(1u << (func_index + 13))) |
                                       (uint32_t)func_enable << (func_index + 13));
}

/* Read LLAC extension-header feature flags. */
bool v4f_llac_enc_get_extra_func(LLAC_UINT8 func_index, LLAC_UINT8 *llac_enc_mem_addr) {
    const uint16_t *extra_nbyte_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte;
    return func_index < 3 && ((*extra_nbyte_addr >> (func_index + 13)) & 1u);
}

#include <stdint.h>

/* Validate handle, positive bitrate, and 96 kHz HR requirements before updating the budget. */
LLAC_Error v4f_llac_set_bitrate_checked(LLAC_Enc *encoder, LLAC_INT bitrate) {
    if (!encoder)
        return LLAC_NULL_ERROR;
    if (bitrate <= 0)
        return LLAC_BITRATE_ERROR;
    if (encoder->fs_idx == 5 && !encoder->hrmode)
        return LLAC_HRMODE_ERROR;
    return v4f_llac_set_channel_budget(encoder, bitrate);
}

/* Validate each channel pointer and depth before encoding; callers must initialize instance state first. */
LLAC_Error v4f_llac_encode_checked(LLAC_Enc *encoder, void **input_samples, LLAC_INT bitdepth,
                                   void *output_bytes, LLAC_INT *num_bytes) {
    if (!encoder || !input_samples || !output_bytes || !num_bytes)
        return LLAC_NULL_ERROR;
    for (LLAC_INT ch = encoder->channels; ch > 0;) {
        --ch;
        if (!input_samples[ch])
            return LLAC_NULL_ERROR;
    }
    if (bitdepth != LHDC_V4_PCM_BITS_S16 && bitdepth != LHDC_V4_PCM_BITS_S24 &&
        bitdepth != LHDC_V4_PCM_BITS_S32)
        return LLAC_ERROR;
    *num_bytes = v4f_llac_encode_channels(encoder, input_samples, output_bytes, bitdepth);
    return LLAC_OK;
}

/* Invoke LLAC encoding with planar 16-bit PCM. */
LLAC_Error v4f_llac_encode_s16(LLAC_Enc *encoder, int16_t **input_samples, void *output_bytes,
                               LLAC_INT *num_bytes) {
    return v4f_llac_encode_checked(encoder, (void **)input_samples, 16, output_bytes, num_bytes);
}

/* Write extension headers and encode; return bytes on success or negative LLAC errors. */
LLAC_INT v4f_llac_enc_process(LLAC_INT *input, LLAC_UINT8 *output, LLAC_UINT8 *llac_enc_mem_addr) {
    if (!input || !output || !llac_enc_mem_addr)
        return -LLAC_NULL_ERROR;
    LHDCV4_LLACWorkspaceDirectory *directory = (void *)llac_enc_mem_addr;
    if (!directory->encoder)
        return -LLAC_NULL_ERROR;
    if (*directory->nchannels < 1 || *directory->nchannels > 2)
        return -LLAC_CHANNELS_ERROR;
    if (*directory->bps_in != 16 && *directory->bps_in != 24 && *directory->bps_in != 32)
        return -LLAC_ERROR;
    int16_t bps_in = *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->bps_in;
    uint32_t nSamples = *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->frame_length;
    int16_t nChannels = *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->nchannels;
    LLAC_UINT8 *extra_nbyte_addr =
        (LLAC_UINT8 *)((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte;
    LLAC_UINT8 *extra_nbyte_num_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte_num;
    int16_t *buf_16 = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->buf16;
    LLAC_Enc *encoder = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->encoder;
    void *input24_32_ptr[2], *input16_ptr[2];
    for (uint32_t i = 0; i < *extra_nbyte_num_addr; ++i)
        output[i] = extra_nbyte_addr[i];
    for (LLAC_INT ch = 0; ch < nChannels; ++ch)
        input24_32_ptr[ch] = input + nSamples * (uint32_t)ch;
    LLAC_INT nBytes = 0;
    LLAC_Error err;
    if (bps_in == LHDC_V4_PCM_BITS_S24 || bps_in == LHDC_V4_PCM_BITS_S32)
        err = v4f_llac_encode_checked(encoder, input24_32_ptr, bps_in,
                                      output ? output + *extra_nbyte_num_addr : NULL, &nBytes);
    else {
        for (LLAC_INT ch = 0; ch < nChannels; ++ch)
            input16_ptr[ch] = buf_16 + nSamples * (uint32_t)ch;
        for (LLAC_INT i = 0; i < (LLAC_INT)(nSamples * (uint32_t)nChannels); ++i)
            buf_16[i] = (int16_t)input[i];
        err = v4f_llac_encode_s16(encoder, (int16_t **)input16_ptr,
                                  output ? output + *extra_nbyte_num_addr : NULL, &nBytes);
    }
    if (err == LLAC_OK) {
        return nBytes + *extra_nbyte_num_addr;
    }
    return -(LLAC_INT)err;
}
#include "../include/llac/llac_const.h"

/* Validate four-byte handle alignment, supported rate, and one/two channels, then initialize. */
LLAC_Error v4f_llac_init_checked(LLAC_Enc *encoder, LLAC_INT samplerate, LLAC_INT channels,
                                 LLAC_UINT8 *llac_enc_mem_addr) {
    if (!encoder)
        return LLAC_NULL_ERROR;
    if ((uintptr_t)encoder & 3)
        return LLAC_ALIGN_ERROR;
    switch (samplerate) {
    case LHDC_V4_SAMPLE_RATE_8000:
    case LHDC_V4_SAMPLE_RATE_16000:
    case LHDC_V4_SAMPLE_RATE_24000:
    case LHDC_V4_SAMPLE_RATE_32000:
    case LHDC_V4_SAMPLE_RATE_44100:
    case LHDC_V4_SAMPLE_RATE_48000:
    case LHDC_V4_SAMPLE_RATE_96000:
        break;
    default:
        return LLAC_SAMPLERATE_ERROR;
    }
    if ((uint32_t)channels - 1 > 1)
        return LLAC_CHANNELS_ERROR;
    return v4f_llac_init_state(encoder, samplerate, channels, llac_enc_mem_addr);
}

/* Change duration before setting bitrate; truncate to 0.1 ms for validation but select tables using the input float. */
LLAC_Error v4f_llac_set_frame_duration(LLAC_Enc *encoder, LLAC_FLOAT frame_ms,
                                       LLAC_UINT8 *llac_enc_mem_addr) {
    if (!encoder)
        return LLAC_NULL_ERROR;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Only this bounded interval can truncate to a supported frame length; it also excludes NaN/infinity.
     * Preserve the pre-conversion x87 value with a double product rather than rounding to float first. */
    if (!(frame_ms >= 0.0f && frame_ms <= 11.0f))
        return LLAC_FRAMEMS_ERROR;
    LLAC_INT frame_dms = audio_math_truncate_i32_f64((double)frame_ms * 10.0);
#else
    LLAC_INT frame_dms = (LLAC_INT)(frame_ms * 10.0f);
#endif
    if (frame_dms != LHDC_V4_FRAME_DURATION_2P5MS && frame_dms != LHDC_V4_FRAME_DURATION_5MS &&
        frame_dms != LHDC_V4_FRAME_DURATION_10MS)
        return LLAC_FRAMEMS_ERROR;
    if (encoder->llac_br_set)
        return LLAC_BITRATE_SET_ERROR;
    encoder->frame_dms = frame_dms;
    encoder->frame_ms = frame_ms;
    v4f_llac_configure_frame(encoder, llac_enc_mem_addr);
    return LLAC_OK;
}

/* Reject nonzero HR requests below 48 kHz; otherwise normalize positive values to enabled and rebuild state. */
LLAC_Error v4f_llac_set_high_resolution(LLAC_Enc *encoder, LLAC_INT hrmode,
                                        LLAC_UINT8 *llac_enc_mem_addr) {
    if (!encoder)
        return LLAC_NULL_ERROR;
    if (hrmode && encoder->fs_in < LHDC_V4_SAMPLE_RATE_48000)
        return LLAC_SAMPLERATE_ERROR;
    encoder->hrmode = hrmode > 0;
    v4f_llac_configure_frame(encoder, llac_enc_mem_addr);
    return LLAC_OK;
}
#include "../include/llac/llac_const.h"

/* Decode constants once and initialize LLAC workspace, frames, and bitrate; return samples, actual bitrate, and bytes including extensions.
 */
LLAC_INT v4f_llac_enc_initial(uint32_t sampleRate, int16_t nChannels, int16_t bps_in,
                              int16_t bps_out, LLAC_FLOAT frame_ms, LLAC_INT target_bitrate,
                              LLAC_INT *nBytes, uint32_t *nSamples, LLAC_INT *real_bitrate,
                              LLAC_UINT8 *llac_enc_mem_addr) {
    if (!nBytes || !nSamples || !real_bitrate || !llac_enc_mem_addr)
        return LLAC_NULL_ERROR;
    *nBytes = 0;
    *nSamples = 0;
    *real_bitrate = 0;
    if (!audio_math_is_finite_f32(frame_ms) || frame_ms < 0.0f || frame_ms > 11.0f)
        return LLAC_FRAMEMS_ERROR;
    if (target_bitrate < 1000)
        return LLAC_BITRATE_ERROR;
    if ((bps_in != 16 && bps_in != 24 && bps_in != 32) ||
        (bps_out != 16 && bps_out != 24 && bps_out != 32))
        return LLAC_ERROR;
    LLAC_Enc *encoder = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->encoder;
    LLAC_Error err =
        v4f_llac_init_checked(encoder, (LLAC_INT)sampleRate, nChannels, llac_enc_mem_addr);
    if (err != LLAC_OK)
        return err;
    err = v4f_llac_set_frame_duration(encoder, frame_ms, llac_enc_mem_addr);
    if (err != LLAC_OK)
        return err;
    err = v4f_llac_set_high_resolution(encoder, sampleRate == LHDC_V4_SAMPLE_RATE_96000,
                                       llac_enc_mem_addr);
    if (err != LLAC_OK)
        return err;
    LLAC_INT bitrate = (target_bitrate / 1000) * 1000;
    if (target_bitrate < 1000)
        return LLAC_BITRATE_ERROR;
    err = v4f_llac_set_bitrate_checked(encoder, bitrate);
    if (err != LLAC_OK)
        return err;
    *nSamples = (uint32_t)encoder->frame_length;
    *real_bitrate = v4f_llac_target_bitrate(encoder);
    *nBytes = v4f_llac_target_bytes(encoder);
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->sample_rate = sampleRate;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->nchannels = nChannels;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->bps_in = bps_in;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->bps_out = bps_out;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->frame_ms = frame_ms;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->target_bitrate = bitrate;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->nbytes = *nBytes;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->frame_length = *nSamples;
    *((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->real_bitrate = *real_bitrate;
    uint16_t *extra_nbyte_addr = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte;
    LLAC_UINT8 *extra_nbyte_num_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte_num;
    *extra_nbyte_addr =
        (uint16_t)((*extra_nbyte_addr & 0xe000) | ((uint16_t)(target_bitrate / 1000) & 0xe3ff));
    *extra_nbyte_num_addr = *extra_nbyte_addr > 0x3fff ? 4 : 2;
    *nBytes += *extra_nbyte_num_addr;
    return LLAC_OK;
}

/* Update integer-kbit/s budgets/extensions; cache hits reuse prior actual bitrate and byte count. */
LLAC_INT v4f_llac_enc_set_bitrate(LLAC_INT target_bitrate, LLAC_INT *nBytes, LLAC_INT *real_bitrate,
                                  LLAC_UINT8 *llac_enc_mem_addr) {
    if (!nBytes || !real_bitrate || !llac_enc_mem_addr)
        return LLAC_NULL_ERROR;
    *nBytes = 0;
    *real_bitrate = 0;
    if (target_bitrate < 1000)
        return LLAC_BITRATE_ERROR;
    LLAC_INT *target_bitrate_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->target_bitrate;
    LLAC_INT *real_bitrate_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->real_bitrate;
    LLAC_INT *nBytes_addr = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->nbytes;
    LLAC_UINT8 *extra_nbyte_num_addr =
        ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte_num;
    LLAC_INT bitrate = (target_bitrate / 1000) * 1000;
    if (*target_bitrate_addr != bitrate) {
        LLAC_Enc *encoder = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->encoder;
        if (!encoder)
            return LLAC_NULL_ERROR;
        if (target_bitrate < 1000)
            return LLAC_BITRATE_ERROR;
        LLAC_Error err = v4f_llac_set_bitrate_checked(encoder, bitrate);
        if (err != LLAC_OK)
            return err;
        *real_bitrate = v4f_llac_target_bitrate(encoder);
        *nBytes_addr = v4f_llac_target_bytes(encoder);
        *target_bitrate_addr = bitrate;
        *real_bitrate_addr = *real_bitrate;
        uint16_t *extra_nbyte_addr =
            ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->extra_nbyte;
        *extra_nbyte_addr =
            (uint16_t)((*extra_nbyte_addr & 0xfc00) | (uint16_t)(target_bitrate / 1000));
    } else
        *real_bitrate = *real_bitrate_addr;
    *nBytes = *nBytes_addr + *extra_nbyte_num_addr;
    return LLAC_OK;
}
