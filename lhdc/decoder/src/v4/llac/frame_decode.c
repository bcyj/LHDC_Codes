#include "../../../include/lhdc/decoder/status.h"
#include "../../../../common/include/lhdc_log.h"
#include "../include/llac_workspace_directory_view.h"
#include "../include/llac_decode.h"
#include "../include/llac_sns.h"
#include "../include/llac_spectral.h"
#include "../include/llac_side_info.h"
#include "../include/llac_quant.h"
#include "../include/llac_signal.h"
#include "../include/llac_fft.h"
#include "../include/llac_dct_workspace.h"
#include <audio_math/byte_order.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include <string.h>
#include <stdint.h>

/* Output samples using thresholds and fixed-point conversion; the positive 24-bit threshold is 0x7fff00. */
static int output_sample(float sample, int bps, unsigned index) {
    int maximum = bps == 16 ? 32767 : 0x7fff00, minimum = bps == 16 ? -32768 : -8388608;
    if (bps == 16 && index % 4 != 0) {
        if (sample > 32767.0f)
            return maximum;
        if (sample < -32768.0f)
            return minimum;

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Bit depth is already restricted to 16. Unordered comparisons let NaN reach integer conversion,
         * where its conversion result is INT32_MIN. */
        if (audio_math_is_nan_f32(sample))
            return INT32_MIN;
        return audio_math_truncate_i32_f32(sample);
#else
        return (int)(bps == 16 ? sample : sample * 256.0f);
#endif
    }
    if (!(sample <= 32767.0f))
        return maximum;
    if (sample < -32768.0f)
        return minimum;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Clamping excludes nonfinite inputs; multiplying by 256 only shifts the exponent and remains within int32. */
    return audio_math_truncate_i32_f32(bps == 16 ? sample : sample * 256.0f);
#else
    return (int)(bps == 16 ? sample : sample * 256.0f);
#endif
}

/* Decode side information and spectrum in order; bad frames skip subsequent spectral processing only. */
static void decode_llac_spectrum(LLAC_Dec *decoder, DecSetup *h_DecSetup, uint8_t *bs_in,
                                 LhdcLlacTransformScratch *scratch) {
    int32_t *integers = scratch->quantized;
    int mask_side = 0, bp_side = 0, bfi = 0, gg_idx = 0, fac_ns_idx = 0, bw_cutoff_idx = 0,
        lastnz = 0, lsbMode = 0, nf_seed = 0, zero_frame = 0, residualPresent = 0,
        nbits_residual = 0, bitsRead = 0;
    LLACTnsData *tns = &h_DecSetup->tns;
    tns->filter_count = 0;
    memset(tns->order, 0, sizeof tns->order);
    LLACLtpfIndices ltpf = {0};
    llac_read_side_information(bs_in, h_DecSetup->targetBytes, &mask_side, &bp_side, decoder->yLen,
                               decoder->fs_idx,
                               (int)((uint32_t)decoder->BW_cutoff_bits ^ UINT32_C(0x286446fe)),
                               &bfi, &gg_idx, &h_DecSetup->sns_indices, &fac_ns_idx, tns, &ltpf,
                               &bw_cutoff_idx, &lastnz, &lsbMode, decoder->frame_dms);
    if (bfi != 0)
        return;
    llac_decode_spectral_symbols(bs_in, bp_side, mask_side, decoder->yLen, decoder->fs_idx,
                                 h_DecSetup->enable_lpc_weighting, tns, lsbMode, lastnz, &bfi,
                                 fac_ns_idx, gg_idx, h_DecSetup->resBits, integers, &nf_seed,
                                 &zero_frame, h_DecSetup->targetBytes, &nbits_residual,
                                 &residualPresent, h_DecSetup->spectrum.levels, decoder->hrmode);
    for (int i = 0; i < decoder->frame_length; i += 4)
        for (int j = 0; j < 4; ++j)
            h_DecSetup->spectrum.floating[i + j] = (float)integers[i + j];
    if (bfi != 0)
        return;
    if (residualPresent)
        llac_decode_residual_bits(&bitsRead, h_DecSetup->spectrum.floating, decoder->yLen,
                                  h_DecSetup->resBits, nbits_residual);
    uint32_t key = decoder->hrmode ? UINT32_C(0x9f04da71) : UINT32_C(0xad645fe1);
    int cutoff = (int)((uint32_t)decoder->cutoffBins[bw_cutoff_idx] ^ key);
    if (!zero_frame)
        llac_fill_spectral_noise(fac_ns_idx, h_DecSetup->spectrum.floating, nf_seed, cutoff,
                                 decoder->frame_dms, integers);
    llac_apply_global_gain(h_DecSetup->spectrum.floating, decoder->yLen, gg_idx,
                           h_DecSetup->quantizedGainOff);
    llac_apply_tns(h_DecSetup->spectrum.floating, tns, cutoff, decoder->frame_length, decoder->fs,
                   decoder->frame_dms);
    llac_decode_sns_codebooks(&h_DecSetup->sns_indices, h_DecSetup->scf_q);
    int bands = (int)((uint32_t)decoder->bands_number ^ UINT32_C(0x172fdd8c));
    llac_interpolate_sns_gains(h_DecSetup->scf_q, bands, h_DecSetup->int_scf, scratch->floating);
    llac_apply_sns_gains(h_DecSetup->spectrum.floating, h_DecSetup->int_scf,
                         (int *)decoder->bands_offset, bands);
}

/* Decode LLAC side information, arithmetic spectrum, residuals, noise, TNS, SNS, and IMDCT; bad frames retain existing spectrum state.
 */
int llac_decode_channel(LLAC_Dec *decoder, int channel, uint8_t *bs_in, void *s_out, int bps,
                        int bfi_ext) {
    DecSetup *h_DecSetup = decoder->channel_setup[channel];
    LhdcLlacDctWorkspace *workspace = h_DecSetup->dct4structImdct.fft.handle;
    memset(workspace->scratch.quantized, 0,
           (size_t)decoder->frame_length * sizeof *workspace->scratch.quantized);
    if (bfi_ext == 0)
        decode_llac_spectrum(decoder, h_DecSetup, bs_in, &workspace->scratch);
    llac_synthesize_frame(h_DecSetup->spectrum.floating, decoder->frame_length,
                          (float *)decoder->imdct_win, h_DecSetup->imdct_mem, &h_DecSetup->pcm,
                          &h_DecSetup->dct4structImdct);
    for (int i = 0; i < decoder->frame_length; i += 4)
        for (int j = 0; j < 4; ++j)
            ((int *)s_out)[i + j] =
                output_sample(h_DecSetup->pcm.samples[i + j], bps, (unsigned)(i + j));
    return LHDC_DEC_OK;
}

/* XOR and rotate input in place before channel-mode decoding; bitrate errors occur after descrambling has modified input. */
uint8_t llac_descramble_and_decode_channels(LLAC_Dec *decoder, uint8_t *input, int num_bytes,
                                            void **output, int bps) {
    int bfi = num_bytes == 0;
    uint8_t first = *input;
    for (int i = 0; i < num_bytes; ++i) {
        uint8_t value = first ^ 0xa6;
        if (i == num_bytes - 1)
            *input = value;
        else {
            first = input[i + 1];
            input[i + 1] = value;
        }
    }
    for (int ch = 0; ch < decoder->channels; ++ch) {
        if (!bfi) {
            uint8_t err = llac_set_channel_budget(decoder, ch, num_bytes);
            if (err)
                return err;
            decoder->last_size = num_bytes;
        }
        if ((ch == 0 && *decoder->channel_mode != 2) || (ch == 1 && *decoder->channel_mode != 1))
            bfi = llac_decode_channel(decoder, ch, input, output[ch], bps, bfi);
        input += decoder->channel_setup[ch]->targetBytes;
    }
    return bfi ? LHDC_LLAC_DEC_BAD_FRAME : LHDC_DEC_OK;
}

/* Validate instance, input, output directory, and each channel pointer; zero-channel instances still reach the core. */
uint8_t llac_decode_checked(LLAC_Dec *decoder, void *input_bytes, int num_bytes,
                            void **output_samples, int bps) {
    if (!decoder || !input_bytes || !output_samples)
        return LHDC_LLAC_DEC_INVALID_POINTER;
    for (int ch = decoder->channels - 1; ch >= 0; --ch)
        if (!output_samples[ch])
            return LHDC_LLAC_DEC_INVALID_POINTER;
    return llac_descramble_and_decode_channels(decoder, input_bytes, num_bytes, output_samples,
                                               bps);
}

/* Initialize fixed workspace: zero succeeds, positive values are LLAC errors; clear output length on failure. */
uint8_t llac_dec_initial(unsigned sampleRate, short nChannels, short bps_out, float frame_ms,
                         int target_bitrate, int *nBytes, unsigned *nSamples,
                         LLACWorkspaceDirectory *directory) {
    if (!nBytes || !nSamples || !directory)
        return LHDC_LLAC_DEC_INVALID_POINTER;
    *nBytes = 0;
    *nSamples = 0;
    if (target_bitrate < 1000)
        return LHDC_LLAC_DEC_INVALID_BITRATE;
    unsigned *sampleRate_addr = directory->samplerate;
    short *nChannels_addr = directory->nchannels, *bps_out_addr = directory->bps_out;
    float *frame_ms_addr = directory->frame_ms;
    int *target_bitrate_addr = directory->target_bitrate, *nBytes_addr = directory->nbytes;
    int32_t *nSamples_addr = directory->frame_length;
    uint16_t *extra_nbyte_addr = directory->extra_nbyte;
    uint8_t *extra_nbyte_num_addr = directory->extra_nbyte_num;
    LLAC_Dec *decoder = directory->decoder;
    uint8_t *channel_mode_addr = directory->channel_mode;
    uint8_t err = llac_init_checked(decoder, (int)sampleRate, nChannels, directory);
    if (err)
        return err;
    decoder->hrmode = 0;
    llac_configure_frame(decoder, directory);
    err = llac_set_frame_duration(decoder, frame_ms, directory);
    if (err)
        return err;
    int kbps = target_bitrate / 1000, rounded = kbps * 1000;
    *nSamples = (unsigned)decoder->frame_length;
    *nBytes = (int)((uint32_t)rounded * *nSamples / (sampleRate << 3));
    *sampleRate_addr = sampleRate;
    *nChannels_addr = nChannels;
    *bps_out_addr = bps_out;
    *frame_ms_addr = frame_ms;
    *target_bitrate_addr = rounded;
    *nBytes_addr = *nBytes;
    *nSamples_addr = *nSamples;
    *extra_nbyte_addr = (uint16_t)kbps | (*extra_nbyte_addr & 0xfc00);
    *extra_nbyte_num_addr = 2;
    *nBytes += 2;
    decoder->channel_mode = channel_mode_addr;
    return LHDC_DEC_OK;
}

/* Update bitrate/extension length from the input header and decode channels; four extension-header bytes persist across frames. */
int llac_dec_process(unsigned char *input, int *output, LLACWorkspaceDirectory *directory) {
    if (!input || !output || !directory)
        return -LHDC_LLAC_DEC_INVALID_POINTER;
    if (!directory->decoder || *directory->nchannels < 1 || *directory->nchannels > 2 ||
        !*directory->samplerate || !*directory->frame_length)
        return -LHDC_LLAC_DEC_INVALID_POINTER;
    short bps_out = *directory->bps_out;
    int nSamples = *directory->frame_length;
    short nChannels = *directory->nchannels;
    unsigned nBytes = *directory->nbytes;
    int sampleRate = *directory->samplerate;
    int *target_bitrate_addr = directory->target_bitrate;
    int32_t *nBytes_addr = directory->nbytes;
    uint16_t *extra_nbyte_addr = directory->extra_nbyte;
    uint8_t *extra_nbyte_num_addr = directory->extra_nbyte_num;
    LLAC_Dec *decoder = directory->decoder;
    uint16_t header = audio_math_read_le16(input);
    if (((header ^ *extra_nbyte_addr) & 1023) != 0) {
        *target_bitrate_addr = (header & 1023) * 1000;
        nBytes = (uint32_t)nSamples * (uint32_t)*target_bitrate_addr / ((uint32_t)sampleRate << 3);
        *nBytes_addr = nBytes;
    }
    if (header & 0xc000)
        *extra_nbyte_num_addr = 4;
    for (unsigned i = 0; i < *extra_nbyte_num_addr; ++i)
        ((uint8_t *)extra_nbyte_addr)[i] = input[i];
    void *output_ptr[2];
    for (int ch = 0; ch < nChannels; ++ch)
        output_ptr[ch] = output + ch * nSamples;
    uint8_t err = llac_decode_checked(decoder, input + *extra_nbyte_num_addr, (int)nBytes,
                                      output_ptr, bps_out);
    if (err & 0xfd)
        return -(int)err;
    return (int)(*extra_nbyte_num_addr + nBytes);
}
