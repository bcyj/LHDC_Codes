#include "../include/llac_memory.h"
#include "../include/llac_init.h"
#include "../include/llac_workspace_directory_view.h"
#include "../include/llac_dct_workspace.h"
#include <assert.h>
#include <string.h>
#include "../../../../common/include/lhdc_alignment.h"

/* Round length and returned address upward to four-byte alignment independently. */
void *llac_workspace_alloc_aligned4(void *base, size_t *base_size, size_t size) {
    assert(((uintptr_t)base & 3) == 0);
    size_t old = *base_size;
    *base_size = (old + size + 3) & ~(size_t)3;
    return (void *)(((uintptr_t)base + old + 3) & ~(uintptr_t)3);
}

/* Allocate per-channel settings; zero/negative channels return base decoder size only. */
int llac_channel_workspace_init(LLAC_Dec *decoder, int channels) {
    size_t size = sizeof(LLAC_Dec);
    if (channels <= 0)
        return (int)size;
    assert(((uintptr_t)decoder & 3) == 0);
    for (int ch = 0; ch < channels; ++ch) {
        void *setup_memory = llac_workspace_alloc_aligned4(decoder, &size, sizeof(DecSetup));
        if (decoder && ch < 2)
            decoder->channel_setup[ch] = setup_memory;
        else if (decoder)

            memcpy((unsigned char *)decoder + (size_t)ch * sizeof(void *), &setup_memory,
                   sizeof setup_memory);
    }
    return (int)size;
}

/* Allocate directories, parameters, stereo state, and instance-owned common DCT plan/scratch. */
int llac_dec_mem_process(unsigned char *mem) {
    unsigned dir = sizeof(LLACWorkspaceDirectory),
             parameter = (unsigned)lhdc_align_size(dir + sizeof(LLACWorkspaceParameters),
                                                   _Alignof(LLAC_Dec));
    unsigned decoder = (unsigned)llac_channel_workspace_init(NULL, 2),
             aligned = (decoder + 7) & ~7u;
    unsigned handles = parameter + aligned;
    unsigned total = handles + sizeof(LhdcLlacDctWorkspace);
    if (mem) {
        memset(mem, 0, total);
        LLACWorkspaceDirectory *directory = (LLACWorkspaceDirectory *)mem;
        LLACWorkspaceParameters *parameters = (LLACWorkspaceParameters *)(mem + dir);
        directory->samplerate = &parameters->samplerate;
        directory->nchannels = &parameters->nchannels;
        directory->bps_out = &parameters->bps_out;
        directory->frame_ms = &parameters->frame_ms;
        directory->nbytes = &parameters->nbytes;
        directory->frame_length = &parameters->frame_length;
        directory->target_bitrate = &parameters->target_bitrate;
        directory->extra_nbyte = &parameters->extra_nbyte;
        directory->extra_nbyte_num = &parameters->extra_nbyte_num;
        directory->channel_mode = &parameters->channel_mode;
        directory->decoder = mem + parameter;
        LhdcLlacDctWorkspace *work = (void *)(mem + handles);
        directory->dct_twid1 = work->rotations;
        directory->dct_twid2 = work->rotations + LHDC_LLAC_DCT_MAX_LENGTH / 2;
        directory->h_ch0_dct4 = directory->h_ch1_dct4 = &work->fft;
        directory->scratch_ch0_dct4 = directory->scratch_ch1_dct4 = work->scratch.floating;
    }
    return (int)total;
}

/* Query native stereo workspace including common DCT tables/buffers for caller allocation. */
int llac_dec_mem_get_size(void) {
    return llac_dec_mem_process(NULL);
}

/* Initialize workspace without returning its size. */
void llac_dec_mem_init(void *mem) {
    (void)llac_dec_mem_process(mem);
}

/* Copy the selected channel's FFT handle and scratch pointers to the current directory slot. */
void llac_dec_mem_alloc_fft_set(int ch, LLACWorkspaceDirectory *directory) {
    if (ch == 0) {
        directory->h = directory->h_ch0_dct4;
        directory->scratch = directory->scratch_ch0_dct4;
    } else if (ch == 1) {
        directory->h = directory->h_ch1_dct4;
        directory->scratch = directory->scratch_ch1_dct4;
    }
}
