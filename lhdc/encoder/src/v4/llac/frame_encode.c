#include <audio_math/math.h>
#include <audio_math/constants.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/codec/v4_pcm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Encode planar PCM through MDCT, SNS, TNS, gain, and spectral quantization; finish with cyclic shift and XOR. */
LLAC_INT v4f_llac_encode_channels(LLAC_Enc *encoder, void **input, LLAC_UINT8 *output,
                                  LLAC_INT bps) {
    LLAC_INT output_size = 0;
    LLAC_UINT8 *llacbuf = output;
    LhdcV4LlacFrameWorkspace *workspace = &encoder->frame_workspace;
    for (LLAC_INT ch = 0; ch < encoder->channels; ++ch) {
        EncSetup *h_EncSetup = encoder->channel_setup[ch];
        LLAC_FLOAT *spectrum = workspace->spectrum;
        LLAC_INT *quantized_spectrum = workspace->quantized_spectrum;
        LLAC_FLOAT gain = 0.0f;
        LLACTnsData tns = {0};
        /* Fully clear each channel to prevent tails from short frames, retries, or previous channels. */
        memset(spectrum, 0, sizeof workspace->spectrum);
        LLAC_INT BW_cutoff_idx = 0;
        LLAC_INT quantizedGain = 0, quantizedGainMin = 0, nbits = 0, nbits2 = 0;
        LLAC_INT lastnz = 0, lsbMode = 0, fac_ns_idx = 0, numResBits = 0, gainChange = 0;
        LLACLtpfIndices ltpf = {0};
        LLAC_INT bp_side = 0, mask_side = 0;
        memset(llacbuf, 0, (size_t)h_EncSetup->targetBytes);
        for (LLAC_INT i = 0; i < encoder->frame_length; ++i) {
            if (bps == 16)
                h_EncSetup->s_in_scaled[i] = (LLAC_FLOAT)((int16_t *)input[ch])[i];
            else if (bps == 24)
                h_EncSetup->s_in_scaled[i] =
                    (LLAC_FLOAT)((LLAC_INT *)input[ch])[i] * AUDIO_MATH_POW2_NEG_8_F32;
            else if (bps == 32)
                h_EncSetup->s_in_scaled[i] =
                    (LLAC_FLOAT)((LLAC_INT *)input[ch])[i] * AUDIO_MATH_POW2_NEG_16_F32;
        }
        v4f_llac_mdct_analyze(h_EncSetup->s_in_scaled, spectrum, &h_EncSetup->mdctStruct);
        v4f_llac_band_mean_square(encoder->bands_number, encoder->bands_offset, h_EncSetup->ener,
                                  spectrum);
        if (encoder->fs_idx < 1 || encoder->hrmode)
            BW_cutoff_idx = encoder->fs_idx;
        else
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
            v4f_llac_estimate_bandwidth_with_policy(h_EncSetup->ener, encoder->fs_idx,
                                                    encoder->frame_dms, &BW_cutoff_idx,
                                                    encoder->math_policy);
#else
            v4f_llac_estimate_bandwidth(h_EncSetup->ener, encoder->fs_idx, encoder->frame_dms,
                                        &BW_cutoff_idx);
#endif
        v4f_llac_detect_attack(
            h_EncSetup, encoder->fs_in == LHDC_V4_SAMPLE_RATE_44100 ? 240 : encoder->frame_length);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
        v4f_llac_analyze_sns_with_policy(h_EncSetup->ener, encoder->tilt, encoder->bands_number,
                                         h_EncSetup->scf, h_EncSetup->attdec_detected,
                                         encoder->sns_damping, encoder->math_policy);
#else
        v4f_llac_analyze_sns(h_EncSetup->ener, encoder->tilt, encoder->bands_number,
                             h_EncSetup->scf, h_EncSetup->attdec_detected, encoder->sns_damping);
#endif
        v4f_llac_quantize_sns(h_EncSetup);
        v4f_llac_interpolate_sns_gains(h_EncSetup->scf_q, 1, encoder->bands_number,
                                       h_EncSetup->int_scf);
        v4f_llac_apply_sns_gains(spectrum, h_EncSetup->int_scf, encoder->bands_offset,
                                 encoder->bands_number);
        v4f_llac_analyze_and_apply_tns_with_workspace(
            spectrum, BW_cutoff_idx, encoder->cutoffBins[BW_cutoff_idx], encoder->fs,
            encoder->frame_length, encoder->frame_dms, h_EncSetup->total_bits, &tns,
            &h_EncSetup->tns_bits, &workspace->tns);
        /* TNS has finished using shared storage; clear it fully for gain/quantization to prevent dirty short-frame tails. */
        memset(quantized_spectrum, 0, sizeof workspace->quantized_spectrum);
        h_EncSetup->targetBitsQuant = h_EncSetup->targetBitsInit - h_EncSetup->tns_bits - 1;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
        v4f_llac_estimate_quantization_gain_with_policy(
            spectrum, encoder->yLen, h_EncSetup->targetBitsQuant, &gain, &quantizedGain,
            &quantizedGainMin, h_EncSetup->quantizedGainOff, &h_EncSetup->targetBitsOff,
            &h_EncSetup->mem_targetBits, h_EncSetup->mem_specBits, encoder->hrmode,
            h_EncSetup->regBits, encoder->frame_ms, encoder->math_policy, workspace->gain_energy);
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                         \
    (defined(_M_IX86) || defined(_M_X64))
        v4f_llac_estimate_quantization_gain(
            spectrum, encoder->yLen, h_EncSetup->targetBitsQuant, &gain, &quantizedGain,
            &quantizedGainMin, h_EncSetup->quantizedGainOff, &h_EncSetup->targetBitsOff,
            &h_EncSetup->mem_targetBits, h_EncSetup->mem_specBits, encoder->hrmode,
            h_EncSetup->regBits, encoder->frame_ms, workspace->gain_energy);
#else
        v4f_llac_estimate_quantization_gain_with_workspace(
            spectrum, encoder->yLen, h_EncSetup->targetBitsQuant, &gain, &quantizedGain,
            &quantizedGainMin, h_EncSetup->quantizedGainOff, &h_EncSetup->targetBitsOff,
            &h_EncSetup->mem_targetBits, h_EncSetup->mem_specBits, encoder->hrmode,
            h_EncSetup->regBits, encoder->frame_ms, workspace->gain_energy);
#endif
        v4f_llac_quantize_spectrum(spectrum, gain, quantized_spectrum, encoder->yLen,
                                   h_EncSetup->total_bits, &nbits, &nbits2, encoder->fs, &lastnz,
                                   h_EncSetup->spectral_pair_codes, &lsbMode, -1,
                                   h_EncSetup->targetBitsQuant, encoder->hrmode);
        h_EncSetup->mem_specBits = nbits;
        v4f_llac_adjust_quantization_gain(
            &quantizedGain, quantizedGainMin, h_EncSetup->quantizedGainOff, &gain,
            h_EncSetup->targetBitsQuant, nbits, &gainChange, encoder->fs_idx);
        if (gainChange)
            v4f_llac_quantize_spectrum(spectrum, gain, quantized_spectrum, encoder->yLen,
                                       h_EncSetup->total_bits, &nbits, &nbits2, encoder->fs,
                                       &lastnz, h_EncSetup->spectral_pair_codes, &lsbMode, 0,
                                       h_EncSetup->targetBitsQuant, encoder->hrmode);
        v4f_llac_estimate_noise_factor(spectrum, quantized_spectrum, gain,
                                       encoder->cutoffBins[BW_cutoff_idx], encoder->frame_dms,
                                       &fac_ns_idx);
        if (!lsbMode)
            v4f_llac_encode_residual_bits(spectrum, quantized_spectrum, gain, encoder->yLen,
                                          h_EncSetup->targetBitsQuant, nbits2, h_EncSetup->resBits,
                                          &numResBits, encoder->hrmode);
        v4f_llac_write_side_information(llacbuf, &bp_side, &mask_side, h_EncSetup->targetBytes,
                                        encoder->BW_cutoff_bits, BW_cutoff_idx, lastnz,
                                        encoder->yLen, lsbMode, quantizedGain, &tns, &ltpf,
                                        &h_EncSetup->sns_indices, fac_ns_idx);
        v4f_llac_encode_spectral_symbols(
            llacbuf, bp_side, mask_side, quantized_spectrum, &tns, lastnz,
            h_EncSetup->spectral_pair_codes, h_EncSetup->resBits, numResBits, lsbMode,
            h_EncSetup->targetBitsAri, h_EncSetup->enable_lpc_weighting);
        llacbuf += h_EncSetup->targetBytes;
        output_size += h_EncSetup->targetBytes;
    }
    if (output_size) {
        LLAC_UINT8 preserv_in = output[0];
        for (LLAC_INT i = 0; i < output_size; ++i)
            output[i] = (i == output_size - 1 ? preserv_in : output[i + 1]) ^ 0xa6;
    }
    return output_size;
}
#include "../include/codec/scalar.h"
