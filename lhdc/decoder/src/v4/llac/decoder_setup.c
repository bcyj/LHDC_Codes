#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/numeric_bits.h>
#include "../include/llac_workspace_directory_view.h"
#include "../include/llac_init.h"
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include <string.h>
#include "decoder_setup_tables.h"

static const uint32_t init_t3[6] = {

    UINT32_C(0x172fddab), UINT32_C(0x172fddbe), UINT32_C(0x172fddb8),
    UINT32_C(0x172fddba), UINT32_C(0x172fddbb), UINT32_C(0x172fddb6)};
static const uint32_t init_t6[6] = {

    UINT32_C(0x286446fe), UINT32_C(0x286446ff), UINT32_C(0x286446fc),
    UINT32_C(0x286446fc), UINT32_C(0x286446fd), UINT32_C(0x286446fe)};
static const uint32_t init_t7[6] = {

    UINT32_C(0xad645fc9), UINT32_C(0xad645fb1), UINT32_C(0xad645f99),
    UINT32_C(0xad645f41), UINT32_C(0xad645f29), UINT32_C(0xad645f29)};
static const uint32_t init_t8[6] = {

    UINT32_C(0x9f04da59), UINT32_C(0x9f04da21), UINT32_C(0x9f04da09),
    UINT32_C(0x9f04dad1), UINT32_C(0x9f04da81), UINT32_C(0x9f04db91)};
static const uint32_t init_t16[6] = {

    UINT32_C(0x12169a48), UINT32_C(0x12169ab8), UINT32_C(0x12169ae8),
    UINT32_C(0x12169b58), UINT32_C(0x12169bf8), UINT32_C(0x121699d8)};

static const uint32_t *const init_t12[12] = {

    NULL, NULL, NULL, NULL, lhdc_decoder_v4_llac_init_init_t125, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL};
static const uint32_t *const init_t198[6] = {

    NULL, NULL, NULL, NULL, lhdc_decoder_v4_llac_init_init_t196, NULL};
static const uint32_t *const init_t199[6] = {

    NULL, NULL, NULL, NULL, NULL, NULL};

/* Configure transforms, bands, and windows by sample rate/duration, reusing and reinitializing each channel's DCT. */
void llac_configure_frame(LLAC_Dec *decoder, LLACWorkspaceDirectory *directory) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Preserve the integer quotient's binary32 rounding; its magnitude is at most 21474836, making truncation bounded. */
    int length = audio_math_truncate_i32_f32((float)(decoder->fs / 100));
#else
    int length = (int)(float)(decoder->fs / 100);
#endif
    if (decoder->fs_out == 44100)
        length = 440;
    decoder->frame_length = length;
    decoder->yLen = decoder->hrmode == 1 ? length : (length < 400 ? length : 400);
    decoder->bands_number = 64;
    if (decoder->frame_ms == 5.0f) {
        int index = decoder->fs_idx;
        decoder->frame_length = length >> 1;
        decoder->yLen /= 2;
        decoder->bands_number = audio_math_int32_from_bits(init_t3[index]);
        decoder->BW_cutoff_bits = decoder->hrmode ? 0 : audio_math_int32_from_bits(init_t6[index]);
        decoder->bands_offset =
            (const int *)(decoder->hrmode ? init_t199[index] : init_t198[index]);
        decoder->cutoffBins = (const int *)(decoder->hrmode ? init_t8 : init_t7);
        decoder->imdct_win = (const float *)init_t12[index + decoder->hrmode * 6];
        decoder->imdct_laZeros = decoder->frame_length / 4;
        decoder->imdct_winLen = audio_math_int32_from_bits(init_t16[index]);
        if (decoder->fs_out == 44100) {
            decoder->imdct_win = (const float *)lhdc_decoder_v4_llac_init_init_t128;
            decoder->imdct_winLen = 440;
        }
    } else
        decoder->BW_cutoff_bits =
            decoder->hrmode ? 0 : audio_math_int32_from_bits(init_t6[decoder->fs_idx]);
    decoder->la_zeroes = decoder->imdct_laZeros;
    decoder->imdct_memLen = decoder->frame_length - decoder->imdct_laZeros;
    for (int ch = 0; ch < decoder->channels; ++ch) {
        Dct4 *dct = &decoder->channel_setup[ch]->dct4structImdct;
        llac_dec_mem_alloc_fft_set(ch, directory);
        if (dct->length)
            llac_dct4_reset(dct);
        llac_dct4_init(dct, decoder->frame_length, directory);
    }
}

/* Clear decoder/channel settings, bind pointers, and initialize a default 10 ms frame using the sample-rate mapping. */
uint8_t llac_init_state(LLAC_Dec *decoder, int samplerate, int channels,
                        LLACWorkspaceDirectory *directory) {
    memset(decoder, 0, sizeof *decoder + (channels > 0 ? (size_t)channels * sizeof(DecSetup) : 0));
    (void)llac_channel_workspace_init(decoder, channels);
    decoder->fs = samplerate == 44100 ? 48000 : samplerate;
    decoder->fs_out = samplerate;
    decoder->fs_idx = samplerate == 44100 ? 4 : (samplerate > 49999 ? 5 : samplerate / 10000);
    decoder->channels = channels;
    decoder->frame_ms = 10.0f;
    decoder->frame_dms = 100;
    decoder->BW_cutoff_bits = audio_math_int32_from_bits(init_t6[decoder->fs_idx]);
    if (samplerate == 44100 || samplerate == 48000)
        decoder->tilt = 30;
    llac_configure_frame(decoder, directory);
    return LHDC_DEC_OK;
}

/* Divide input bytes among channels, validate bitrate, and configure LPC weighting and quantization global-gain offset. */
uint8_t llac_set_channel_budget(LLAC_Dec *decoder, int ch, int nBytes) {
    int minBytes = 19, maxBytes = 625;
    if (decoder->hrmode) {
        if (decoder->frame_dms != 50)
            return LHDC_LLAC_DEC_UNSUPPORTED_HR_MODE;
        maxBytes = 375;
        if (decoder->fs == 48000)
            minBytes = 93;
        else if (decoder->fs == 96000)
            minBytes = 109;
        else
            return LHDC_LLAC_DEC_UNSUPPORTED_HR_MODE;
    }
    int count = nBytes / decoder->channels;
    if (ch < nBytes - decoder->channels * count)
        ++count;
    if (count < minBytes || count > maxBytes)
        return LHDC_LLAC_DEC_INVALID_BYTE_BUDGET;
    DecSetup *setup = decoder->channel_setup[ch];
    int bits = count * 8, index = decoder->fs_idx + 1;
    setup->total_bits = bits;
    setup->enable_lpc_weighting = bits < (decoder->frame_ms == 5.0f ? 240 : 480);
    setup->targetBytes = count;
    int gain = bits / (index * 10);
    if (gain > 114)
        gain = 115;
    setup->quantizedGainOff = index * -5 - (gain + 105);
    return LHDC_DEC_OK;
}

/* Validate public LLAC initialization for 44.1/48 kHz and one/two channels before initializing. */
uint8_t llac_init_checked(LLAC_Dec *decoder, int samplerate, int channels,
                          LLACWorkspaceDirectory *directory) {
    if (!decoder)
        return LHDC_LLAC_DEC_INVALID_POINTER;
    if (samplerate != 44100 && samplerate != 48000)
        return LHDC_LLAC_DEC_UNSUPPORTED_RATE;
    if ((unsigned)channels - 1 >= 2)
        return LHDC_LLAC_DEC_UNSUPPORTED_CHANNELS;
    return llac_init_state(decoder, samplerate, channels, directory);
}

/* Set 2.5/5/10 ms frame duration and reconfigure transforms; reject other durations. */
uint8_t llac_set_frame_duration(LLAC_Dec *decoder, float frame_ms,
                                LLACWorkspaceDirectory *directory) {
    if (!decoder)
        return LHDC_LLAC_DEC_INVALID_POINTER;
    if (frame_ms != 2.5f && frame_ms != 5.0f && frame_ms != 10.0f)
        return LHDC_LLAC_DEC_UNSUPPORTED_DURATION;
    decoder->frame_ms = frame_ms;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Validation already restricts duration to three exactly representable values; no float multiplication/conversion is needed. */
    decoder->frame_dms = frame_ms == 2.5f ? 25 : frame_ms == 5.0f ? 50 : 100;
#else
    decoder->frame_dms = (int)(frame_ms * 10.0f);
#endif
    llac_configure_frame(decoder, directory);
    return LHDC_DEC_OK;
}

/* Read extension-header bits 13/14/15; return 255 for unknown indices. */
uint8_t llac_dec_get_extra_func(uint8_t func_index, LLACWorkspaceDirectory *directory) {
    uint16_t value;
    memcpy(&value, directory->extra_nbyte, 2);
    return func_index < 3 ? (uint8_t)((value >> (13 + func_index)) & 1)
                          : LHDC_LLAC_DEC_UNKNOWN_EXTENSION;
}

/* Accept only channel modes 0/1/2; invalid modes leave the parameter slot unchanged. */
void llac_dec_set_channel_mode(uint8_t mode_index, LLACWorkspaceDirectory *directory) {
    if (mode_index <= 2)
        *(uint8_t *)directory->channel_mode = mode_index;
}
