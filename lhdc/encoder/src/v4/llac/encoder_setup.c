#include "../../../../platform/windows/include/lhdc_windows_exp_log.h"
#include "../../../../common/include/dsp/float_compat.h"
#include <audio_math/bits.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_rounding.h>
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Reset FFT object and active workspace length. */
void v4f_llac_fft_reset(Fft *fft) {
    if (!fft)
        return;
    if (fft->handle)
        ((HANDLE_IIS_FFT)fft->handle)->iisfft.length = 0;
    memset(fft, 0, sizeof *fft);
}

/* Reset DCT-II and its FFT. */
void v4f_llac_dct2_reset(Dct2 *dct) {
    if (dct) {
        v4f_llac_fft_reset(&dct->fft);
        memset(dct, 0, sizeof *dct);
    }
}

/* Reset DCT-IV, rotation pointers, and its FFT. */
void v4f_llac_dct4_reset(Dct4 *dct) {
    if (dct) {
        v4f_llac_fft_reset(&dct->fft);
        memset(dct, 0, sizeof *dct);
    }
}

/* Reset MDCT state including window/history references and DCT-IV. */
void v4f_llac_mdct_reset(Mdct *mdct) {
    if (mdct) {
        v4f_llac_dct4_reset(&mdct->dct);
        memset(mdct, 0, sizeof *mdct);
    }
}

/* Allocate a four-byte-aligned subblock from contiguous workspace and update total size. */
void *v4f_llac_workspace_alloc_aligned4(void *base, size_t *base_size, size_t size) {
    size_t used = *base_size;
    *base_size = (used + 3 + size) & ~(size_t)3;
    return (void *)(((uintptr_t)base + used + 3) & ~(uintptr_t)3);
}

/* Lay out per-channel EncSetup; a null base only calculates requirements. */
LLAC_INT v4f_llac_channel_workspace_init(LLAC_Enc *encoder, LLAC_INT channels) {
    size_t size = sizeof(LLAC_Enc);
    for (LLAC_INT ch = 0; ch < channels; ++ch) {
        if (encoder)
            encoder->channel_setup[ch] =
                (EncSetup *)(((uintptr_t)encoder + (size | 3)) & ~(uintptr_t)3);
        size = ((size | 3) + sizeof(EncSetup)) & ~(size_t)3;
    }
    return (LLAC_INT)size;
}

#include <stdint.h>
#include "../../../../common/include/dsp/fft_float32.h"

/* Validate bitrate by frame/rate, assign channel bytes/entropy budgets, and update attack detection. */
LLAC_Error v4f_llac_set_channel_budget(LLAC_Enc *encoder, LLAC_INT bitrate) {
    LLAC_INT minBR, maxBR;
    if (!encoder->hrmode) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Valid initialized frame lengths make these budgets representable; frame frequency rounds to float,
         * then products use double to avoid another float rounding before truncation. */
        volatile LLAC_FLOAT frames_per_second = 1000.0f / encoder->frame_ms;
        minBR = audio_math_truncate_i32_f64((double)frames_per_second * 160.0);
        maxBR =
            audio_math_truncate_i32_f64((double)frames_per_second *
                                  (encoder->fs_in == LHDC_V4_SAMPLE_RATE_44100 ? 5000.0 : 51200.0));
#else
        LLAC_FLOAT frames_per_second = 1000.0f / encoder->frame_ms;
        minBR = (LLAC_INT)(frames_per_second * 160.0f);
        maxBR =
            (LLAC_INT)(frames_per_second *
                       (LLAC_FLOAT)(encoder->fs_in == LHDC_V4_SAMPLE_RATE_44100 ? 5000 : 51200));
#endif
    } else {
        if (encoder->fs != LHDC_V4_SAMPLE_RATE_48000 && encoder->fs != LHDC_V4_SAMPLE_RATE_96000)
            return LLAC_HRMODE_ERROR;
        LLAC_INT high_rate = encoder->fs == LHDC_V4_SAMPLE_RATE_96000;
        switch (encoder->frame_dms) {
        case 100:
            minBR = high_rate ? 149600 : 124800;
            maxBR = 500000;
            break;
        case 50:
            minBR = high_rate ? 174400 : 148800;
            maxBR = 600000;
            break;
        case 25:
            minBR = high_rate ? 198400 : 172800;
            maxBR = 672000;
            break;
        default:
            return LLAC_HRMODE_ERROR;
        }
    }
    if (bitrate < encoder->channels * minBR || bitrate > encoder->channels * maxBR)
        return LLAC_BITRATE_ERROR;

    encoder->llac_br_set = 1;
    if (encoder->channels > 0) {
        LLAC_INT denominator = encoder->fs_in << 3;
        LLAC_INT nbytes =
            denominator ? (LLAC_INT)((int64_t)encoder->frame_length * bitrate / denominator) : 0;
        LLAC_INT channel_bytes = nbytes / encoder->channels;
        LLAC_INT remainder = nbytes - channel_bytes * encoder->channels;
        /* Positive integer frame length needs ceil(log2(N/2)) side bits; integer width removes libm boundary differences. */
        LLAC_FLOAT length_bits =
            (LLAC_FLOAT)audio_math_ceil_log2_u32((uint32_t)(encoder->frame_length / 2));
        for (LLAC_INT ch = 0; ch < encoder->channels; ++ch) {
            EncSetup *setup = encoder->channel_setup[ch];
            LLAC_INT bytes = channel_bytes + (ch < remainder);
            LLAC_INT bitsTmp = bytes * 8;
            LLAC_INT overhead_remaining = bitsTmp - encoder->envelope_bits -
                                          encoder->global_gain_bits - encoder->noise_fac_bits -
                                          encoder->BW_cutoff_bits;
            LLAC_INT budget = audio_math_truncate_sat_i32_f64((double)(LLAC_FLOAT)overhead_remaining -
                                                        (double)length_bits - 2.0 - 1.0);
            setup->targetBytes = bytes;
            setup->total_bits = bitsTmp;
            setup->targetBitsInit =
                budget - (bitsTmp > 1280) - (bitsTmp > 2560) - (encoder->hrmode != 0);
            setup->targetBitsAri = bitsTmp;
            LLAC_INT weighting_threshold = encoder->frame_ms == 2.5f   ? 120
                                           : encoder->frame_ms == 5.0f ? 240
                                                                       : 480;
            setup->enable_lpc_weighting = bitsTmp < weighting_threshold;
            LLAC_INT gain_divisor = encoder->fs_idx * 10 + 10;
            LLAC_INT gain_steps = gain_divisor ? bitsTmp / gain_divisor : 0;
            if (gain_steps > 115)
                gain_steps = 115;
            setup->quantizedGainOff = encoder->fs_idx * -5 - 110 - gain_steps;

            LLAC_INT attack_enabled =
                encoder->frame_ms == 10.0f &&
                ((encoder->fs_in == 32000 && bytes >= 81 && bytes <= 339) ||
                 (encoder->fs_in >= LHDC_V4_SAMPLE_RATE_44100 && bytes >= 100 && bytes <= 339));
            setup->attack_handling = attack_enabled;
            if (!attack_enabled) {
                setup->attdec_filter_mem[0] = 0.0f;
                setup->attdec_filter_mem[1] = 0.0f;
                setup->attdec_acc_energy = 0.0f;
                setup->attdec_detected = 0;
                setup->attdec_position = 0;
            }
            setup->regBits = -1;
            if (encoder->fs_idx > 3 && encoder->hrmode) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                /* Retain integer-numerator rounding to float, then truncate the double quotient.
                 * Successful HR bitrate validation bounds both numerator and quotient to representable values. */
                volatile LLAC_FLOAT scaled_frame_bits = (LLAC_FLOAT)(bytes * 8000);
                LLAC_INT real_rate =
                    audio_math_truncate_i32_f64((double)scaled_frame_bits / (double)encoder->frame_ms) /
                    12500;
#else
                LLAC_INT real_rate =
                    (LLAC_INT)((LLAC_FLOAT)(bytes * 8000) / encoder->frame_ms) / 12500;
#endif
                if (encoder->frame_ms == 2.5f)
                    real_rate -= 6;
                else if (encoder->frame_ms == 10.0f)
                    real_rate += encoder->fs_idx == 5 ? 2 : 5;
                setup->regBits = real_rate;
            }
        }
    }
    encoder->bitrate = bitrate;
    return LLAC_OK;
}
#include "../include/llac/llac_const.h"

/* Validate length/direction, bind instance buffers, and initialize common KISS factors/twiddles. */
IIS_FFT_ERROR v4f_llac_fft_plan_init(HANDLE_IIS_FFT *handle, LLAC_INT len, IIS_FFT_DIR sign,
                                     LLAC_UINT8 *llac_enc_mem_addr) {
    if (sign != IIS_FFT_FWD && sign != IIS_FFT_BWD)
        return IIS_FFT_INTERNAL_ERROR;
    if (len < 2 || len > LHDC_V4_LLAC_FFT_WORKSPACE_POINTS)
        return IIS_FFT_LENGTH_ERROR;
    HANDLE_IIS_FFT h = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->h.active;
    if (!h)
        return IIS_FFT_MEMORY_ERROR;
    h->sign = sign;
    h->len = len;
    h->scratch = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->h_buffer.active;
    if (!h->scratch)
        return IIS_FFT_MEMORY_ERROR;
    Iisfft *fft = &h->iisfft;
    memset(fft, 0, sizeof *fft);
    fft->length = len;
    fft->sign = sign;
    const LHDCV4_LLACWorkspaceDirectory *directory = (const void *)llac_enc_mem_addr;
    h->kiss_twiddles = (void *)directory->handle_table.active;
    h->kiss_input = (void *)directory->handle_scratch.active;
    h->kiss_output = (void *)directory->handle_scratch2.active;
    if (!h->kiss_twiddles || !h->kiss_input || !h->kiss_output)
        return IIS_FFT_MEMORY_ERROR;
    lhdc_kiss_fft_factor(len, (int32_t)audio_math_sqrt_floor_u32((uint32_t)len), h->kiss_factors);
    lhdc_kiss_fft_twiddles(len, sign == IIS_FFT_BWD, h->kiss_twiddles);
    *handle = h;
    return IIS_FFT_NO_ERROR;
}

/* Set ordinary FFT length/forward handle; failures clear the handle without returning status. */
void v4f_llac_fft_init(Fft *fft, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr) {
    HANDLE_IIS_FFT handle = NULL;
    fft->length = length;
    v4f_llac_fft_plan_init(&handle, length, IIS_FFT_FWD, llac_enc_mem_addr);
    fft->handle = handle;
}

/* Build a same-length forward FFT for DCT-II without changing DCT-state padding. */
void v4f_llac_dct2_init(Dct2 *dct, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr) {
    dct->length = length;
    v4f_llac_fft_init(&dct->fft, length, llac_enc_mem_addr);
}

/* Copy standard DCT-IV rotations or generate other lengths from phase formulas, then build a half-length FFT. */
void v4f_llac_dct4_init(Dct4 *dct, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr) {
    dct->length = length;
    dct->twid1 = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->dct_twid1;
    dct->twid2 = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->dct_twid2;
    const Complex *rotations = v4_llac_dct4_rotations(length);
    if (rotations) {
        size_t bytes = (size_t)(length / 2) * sizeof *rotations;
        memcpy(dct->twid1, rotations, bytes);
        memcpy(dct->twid2, rotations + length / 2, bytes);
    } else {
        const double negative_pi = -AUDIO_MATH_PI_F64;
        for (LLAC_INT i = 0; i < length / 2; ++i) {
            LLAC_FLOAT angle = (LLAC_FLOAT)(((double)i + 0.25) * negative_pi / (double)length);
            audio_math_sincosf(angle, &dct->twid1[i].i, &dct->twid1[i].r);
            angle = (LLAC_FLOAT)((double)i * negative_pi / (double)length);
            audio_math_sincosf(angle, &dct->twid2[i].i, &dct->twid2[i].r);
        }
    }
    v4f_llac_fft_init(&dct->fft, length / 2, llac_enc_mem_addr);
}
#include "../include/llac/llac_const.h"

/* Configure MDCT windows, overlap history, and DCT-IV by duration/length. */
void v4f_llac_mdct_init(Mdct *mdct, LLAC_INT length, LLAC_INT frame_dms, LLAC_INT hrmode,
                        LLAC_UINT8 *llac_enc_mem_addr) {
    mdct->length = length;
    if (frame_dms == LHDC_V4_FRAME_DURATION_2P5MS)
        mdct->leading_zeros = 0;
    else if (frame_dms == LHDC_V4_FRAME_DURATION_5MS)
        mdct->leading_zeros = length / 4;
    else if (frame_dms == LHDC_V4_FRAME_DURATION_10MS)
        mdct->leading_zeros = length * 3 / 8;
    mdct->mem_length = length - mdct->leading_zeros;
    mdct->window = NULL;
    static const LLAC_INT lengths[3][6] = {

        {20, 40, 60, 80, 120, 240}, {40, 80, 120, 160, 240, 480}, {80, 160, 240, 320, 480, 960}};
    const LLAC_FLOAT *const(*windows)[6] = NULL;
    LLAC_INT mode = -1;
    if (frame_dms == LHDC_V4_FRAME_DURATION_2P5MS) {
        windows = init_t11;
        mode = 0;
    } else if (frame_dms == LHDC_V4_FRAME_DURATION_5MS) {
        windows = init_t12;
        mode = 1;
    } else if (frame_dms == LHDC_V4_FRAME_DURATION_10MS) {
        windows = init_t13;
        mode = 2;
    }
    if (mode >= 0) {
        for (LLAC_INT i = 0; i < 6; ++i)
            if (length == lengths[mode][i])
                mdct->window = windows[hrmode][i];
    }
    if (frame_dms == LHDC_V4_FRAME_DURATION_5MS && length == 220)
        mdct->window = init_t128;
    mdct->mem = ((LHDCV4_LLACWorkspaceDirectory *)llac_enc_mem_addr)->mdct_mem;
    v4f_llac_dct4_init(&mdct->dct, length, llac_enc_mem_addr);
}

/* Update bands from sample rate, frame length, and HR mode; initialize/rebuild channel transforms. */
void v4f_llac_configure_frame(LLAC_Enc *encoder, LLAC_UINT8 *llac_enc_mem_addr) {
    LLAC_INT length = encoder->fs_in == LHDC_V4_SAMPLE_RATE_44100 ? 440 : encoder->fs / 100;
    LLAC_INT spectral_length = length;
    if (spectral_length > 399 && encoder->hrmode != 1)
        spectral_length = 400;
    encoder->frame_length = length;
    encoder->yLen = spectral_length;
    encoder->stEnc_mdct_mem_len = length - encoder->la_zeroes;
    encoder->sns_damping = encoder->hrmode == 1 ? 0.6f : 0.85f;
    encoder->bands_number = 64;
    encoder->nSubdivisions = 3;
    LLAC_INT fs_idx = encoder->fs_idx;
    LLAC_INT hr = encoder->hrmode != 0;
    encoder->BW_cutoff_bits = hr ? 0 : init_t6[fs_idx];
    if (encoder->frame_ms == 10.0f) {
        encoder->bands_offset = hr ? init_t178[fs_idx] : init_t179[fs_idx];
        encoder->cutoffBins = hr ? init_t5 : init_t4;
    } else if (encoder->frame_ms == 2.5f) {
        length >>= 2;
        spectral_length /= 4;
        encoder->bands_offset = hr ? init_t189[fs_idx] : init_t188[fs_idx];
        encoder->cutoffBins = hr ? init_t10 : init_t9;
        encoder->bands_number = (hr ? init_t1 : init_t2)[fs_idx];
        encoder->nSubdivisions = 2;
    } else if (encoder->frame_ms == 5.0f) {
        length >>= 1;
        spectral_length /= 2;
        encoder->bands_offset = hr ? init_t199[fs_idx] : init_t198[fs_idx];
        encoder->cutoffBins = hr ? init_t8 : init_t7;
        encoder->bands_number = init_t3[fs_idx];
        encoder->nSubdivisions = 2;
    }
    encoder->frame_length = length;
    encoder->yLen = spectral_length;
    encoder->stEnc_mdct_mem_len = length - encoder->la_zeroes;
    for (LLAC_INT ch = 0; ch < encoder->channels; ++ch) {
        EncSetup *setup = encoder->channel_setup[ch];
        LLAC_INT initialized = setup->mdctStruct.mem != NULL;
        v4f_llac_enc_mem_alloc_fft_set(ch, DCT4, llac_enc_mem_addr);
        if (initialized)
            v4f_llac_mdct_reset(&setup->mdctStruct);
        v4f_llac_mdct_init(&setup->mdctStruct, length, encoder->frame_dms, encoder->hrmode,
                           llac_enc_mem_addr);
        v4f_llac_enc_mem_alloc_fft_set(ch, DCT2, llac_enc_mem_addr);
        if (initialized)
            v4f_llac_dct2_reset(&setup->dct2StructSNS);
        v4f_llac_dct2_init(&setup->dct2StructSNS, 16, llac_enc_mem_addr);
    }
}

/* Clear contiguous encoder/channel memory and set initial 10 ms parameters; this low-level entry does not validate rate. */
LLAC_Error v4f_llac_init_state(LLAC_Enc *encoder, LLAC_INT samplerate, LLAC_INT channels,
                               LLAC_UINT8 *llac_enc_mem_addr) {
    size_t size = sizeof *encoder;
    if (channels > 0)
        size += (size_t)channels * sizeof(EncSetup);
    memset(encoder, 0, size);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    encoder->math_policy = lhdc_windows_log10_policy();
#endif
    if (channels > 0)
        v4f_llac_channel_workspace_init(encoder, channels);
    encoder->fs = samplerate == LHDC_V4_SAMPLE_RATE_44100 ? LHDC_V4_SAMPLE_RATE_48000 : samplerate;
    encoder->fs_in = samplerate;
    encoder->fs_idx = encoder->fs > 49999 ? 5 : encoder->fs / 10000;
    encoder->channels = channels;
    encoder->frame_dms = 100;
    encoder->frame_ms = 10.0f;
    encoder->noise_fac_bits = 3;
    encoder->envelope_bits = 38;
    encoder->global_gain_bits = 8;
    encoder->BW_cutoff_bits = init_t6[encoder->fs_idx];
    switch (encoder->fs) {
    case LHDC_V4_SAMPLE_RATE_8000:
        encoder->tilt = 14;
        break;
    case LHDC_V4_SAMPLE_RATE_16000:
        encoder->tilt = 18;
        break;
    case LHDC_V4_SAMPLE_RATE_24000:
        encoder->tilt = 22;
        break;
    case LHDC_V4_SAMPLE_RATE_32000:
        encoder->tilt = 26;
        break;
    case LHDC_V4_SAMPLE_RATE_48000:
        encoder->tilt = 30;
        break;
    case LHDC_V4_SAMPLE_RATE_96000:
        encoder->tilt = 34;
        break;
    }
    v4f_llac_configure_frame(encoder, llac_enc_mem_addr);
    return LLAC_OK;
}
