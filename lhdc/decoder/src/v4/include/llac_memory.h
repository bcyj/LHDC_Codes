#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_MEMORY_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "llac_workspace_directory_view.h"

typedef struct LLAC_Dec {
    struct DecSetup *channel_setup[2];
    const int *W_fx, *bands_offset, *cutoffBins;
    int fs, fs_out, fs_idx, frame_length, channels;
    float frame_ms;
    int frame_dms, last_size, ep_enabled, error_report, imdct_memLen, imdct_winLen, imdct_laZeros;
    const float *imdct_win;
    int yLen, W_size, la_zeroes, bands_number, BW_cutoff_bits, tilt, hrmode, specflip;
    unsigned char *channel_mode;
} LLAC_Dec;
/* Allocate a four-byte-aligned subblock and update used length; base must be four-byte aligned. */
void *llac_workspace_alloc_aligned4(void *, size_t *, size_t);
/* Calculate decoder and per-channel settings storage from native structure sizes and populate channel pointers. */
int llac_channel_workspace_init(LLAC_Dec *, int);
/* Calculate LLAC workspace size; if nonnull, clear it and construct all directory pointers. */
int llac_dec_mem_process(unsigned char *);
/* Query fixed stereo LLAC workspace size. */
int llac_dec_mem_get_size(void);
/* Initialize caller-provided LLAC workspace. */
void llac_dec_mem_init(void *);
/* Select the FFT handle and scratch area for channel zero or one. */
void llac_dec_mem_alloc_fft_set(int, LLACWorkspaceDirectory *);
#endif
