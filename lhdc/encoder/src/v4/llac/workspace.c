#include "lhdc_memory.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../common/include/lhdc_alignment.h"

/* Release LLAC workspace; null is allowed. */
void v4f_llac_enc_mem_free(LLAC_UINT8 *llac_enc_mem_addr) {
    lhdc_memory_free(llac_enc_mem_addr);
}
#include "../include/codec/scalar.h"
#include "../include/codec/v4_pcm.h"

/* Select LLAC workspace by channel and transform type. */
void v4f_llac_enc_mem_alloc_fft_set(LLAC_INT ch, enum dct_type dct, LLAC_UINT8 *llac_enc_mem_addr) {
    v4_llac_enc_mem_alloc_fft_set(ch, dct, llac_enc_mem_addr);
}

/* Bind parameters and offset-allocated FFT/DCT partitions into the instance directory. */
static void bind_llac_workspace_directory(V4LlacDirectory *directory,
                                          LHDCV4_LlacWorkspaceParameters *parameters,
                                          LLAC_UINT8 *memory, const V4LlacWorkspaceLayout *layout) {
    directory->sample_rate = &parameters->sample_rate;
    directory->nchannels = &parameters->nchannels;
    directory->bps_in = &parameters->bps_in;
    directory->bps_out = &parameters->bps_out;
    directory->frame_ms = &parameters->frame_ms;
    directory->nbytes = &parameters->nbytes;
    directory->frame_length = &parameters->frame_length;
    directory->target_bitrate = &parameters->target_bitrate;
    directory->real_bitrate = &parameters->real_bitrate;
    directory->extra_nbyte = &parameters->extra_nbyte;
    directory->extra_nbyte_num = &parameters->extra_nbyte_num;
    directory->buf16 = (void *)(memory + layout->pcm);
    directory->encoder = (void *)(memory + layout->encoder);
    directory->dct_twid1 = (void *)(memory + layout->pre_rotation);
    directory->dct_twid2 = (void *)(memory + layout->post_rotation);
    for (int channel = 0; channel < 2; ++channel) {
        directory->mdct_mem_ch[channel] = (void *)(memory + layout->mdct_history[channel]);
        for (int dct = 0; dct < 2; ++dct) {
            directory->handle_table.by_channel_dct[channel][dct] =
                (void *)(memory + layout->transform_table[dct]);
            directory->h.by_channel_dct[channel][dct] =
                (void *)(memory + layout->transform_handle[dct]);
            directory->h_buffer.by_channel_dct[channel][dct] =
                (void *)(memory + layout->transform_buffer[dct]);
            directory->handle_scratch.by_channel_dct[channel][dct] =
                (void *)(memory + layout->transform_scratch[dct]);
            directory->handle_scratch2.by_channel_dct[channel][dct] =
                (void *)(memory + layout->transform_scratch2[dct]);
        }
    }
}

/* Calculate aligned LLAC instance layout and bind partition addresses when memory is supplied. */
static size_t llac_workspace_layout(LLAC_UINT8 *memory) {
    size_t directory_size = sizeof(V4LlacDirectory);
    size_t encoder_size = sizeof(LLAC_Enc);
    v4f_llac_workspace_alloc_aligned4(NULL, &encoder_size, sizeof(EncSetup));
    v4f_llac_workspace_alloc_aligned4(NULL, &encoder_size, sizeof(EncSetup));
    encoder_size = lhdc_align_size(encoder_size, 8);
    size_t parameter_offset = directory_size;
    size_t pcm_offset = parameter_offset + sizeof(LHDCV4_LlacWorkspaceParameters);
    size_t encoder_offset =
        lhdc_align_size(pcm_offset + 1920 * sizeof(int16_t), _Alignof(LLAC_Enc));
    size_t work_offset = encoder_offset + encoder_size;
    size_t cursor = work_offset;
    V4LlacWorkspaceLayout layout = {0};
    layout.pcm = pcm_offset;
    layout.encoder = encoder_offset;
    layout.pre_rotation = cursor;
    cursor += 480 * sizeof(kiss_fft_cpx);
    layout.post_rotation = cursor;
    cursor += 480 * sizeof(kiss_fft_cpx);
    layout.mdct_history[0] = cursor;
    cursor += 600 * sizeof(LLAC_FLOAT);
    for (LLAC_INT dct = 0; dct < 2; ++dct) {
        LLAC_INT length = LHDC_V4_LLAC_FFT_WORKSPACE_POINTS;
        layout.transform_table[dct] = cursor;
        cursor += lhdc_align_size((size_t)length * sizeof(kiss_fft_cpx), 8);
        layout.transform_handle[dct] = cursor;
        cursor += lhdc_align_size(sizeof(T_IIS_FFT), 8);
        layout.transform_buffer[dct] = cursor;
        cursor += (size_t)length * sizeof(kiss_fft_cpx);
        layout.transform_scratch[dct] = cursor;
        cursor += (size_t)length * sizeof(kiss_fft_cpx);
        layout.transform_scratch2[dct] = cursor;
        cursor += (size_t)length * sizeof(kiss_fft_cpx);
    }
    layout.mdct_history[1] = cursor;
    cursor += 600 * sizeof(LLAC_FLOAT);
    if (memory) {
        memset(memory, 0, cursor);
        V4LlacDirectory *directory = (void *)memory;
        LHDCV4_LlacWorkspaceParameters *parameters = (void *)(memory + parameter_offset);
        bind_llac_workspace_directory(directory, parameters, memory, &layout);
    }
    return cursor;
}
#include "../include/codec/scalar.h"

/* Switch FFT workspace aliases by channel/DCT mode, preserving channel-one asymmetry. */

void v4_llac_enc_mem_alloc_fft_set(LLAC_INT ch, LLAC_INT dct, LLAC_UINT8 *llac_enc_mem_addr) {
    V4LlacDirectory *directory = (V4LlacDirectory *)llac_enc_mem_addr;
    if (ch != 0 && ch != 1)
        return;
    directory->mdct_mem = directory->mdct_mem_ch[ch];
    if (dct != 0 && dct != 1)
        return;
    directory->handle_table.active = directory->handle_table.by_channel_dct[ch][dct];
    directory->h.active = directory->h.by_channel_dct[ch][dct];

    directory->h_buffer.active = directory->h_buffer.by_channel_dct[ch][ch ? 0 : dct];
    directory->handle_scratch.active = directory->handle_scratch.by_channel_dct[ch][dct];
    directory->handle_scratch2.active = directory->handle_scratch2.by_channel_dct[ch][dct];
}

/* Query LLAC core capacity using the same layout as workspace binding. */
size_t v4f_llac_enc_workspace_bytes(void) {
    return llac_workspace_layout(NULL);
}
/* Construct contiguous core workspace through the specified host. */
LLAC_UINT8 *v4f_llac_enc_mem_alloc_with_allocator(const LhdcAllocator *allocator) {
    LLAC_UINT8 *memory = lhdc_memory_alloc(allocator, llac_workspace_layout(NULL));
    if (memory)
        llac_workspace_layout(memory);
    return memory;
}
/* Retain the default user-mode constructor. */
LLAC_UINT8 *v4f_llac_enc_mem_alloc(void) {
    return v4f_llac_enc_mem_alloc_with_allocator(NULL);
}
