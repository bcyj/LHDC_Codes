/* Instance-owned common DCT plan, rotation tables, and scratch for LLAC decoding; no per-frame allocation. */
#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_DCT_WORKSPACE_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_DCT_WORKSPACE_H
#include "llac_fft_setup.h"
#include "../../../../common/include/dsp/dct4_float32.h"
enum { LHDC_LLAC_DCT_MAX_LENGTH = 480, LHDC_LLAC_DCT_RADIX_FLOATS = 22 };
/* SNS and DCT may overwrite the same buffer after integer spectrum/noise processing ends. */
typedef union LhdcLlacTransformScratch {
    float floating[LHDC_LLAC_DCT_MAX_LENGTH];
    int32_t quantized[LHDC_LLAC_DCT_MAX_LENGTH];
    kiss_fft_cpx spectrum[LHDC_LLAC_DCT_MAX_LENGTH / 2];
} LhdcLlacTransformScratch;
/* fft must be first: decoding accesses shared spectrum through this handle.
 * Initialization sets 10 ms before switching to 5 ms; tables and buffers cover both lengths.
 * Channels execute sequentially using one plan and scratch area; history remains in each channel's DecSetup. */
typedef struct LhdcLlacDctWorkspace {
    T_IIS_FFT fft;
    LhdcDct4F32Plan plan;
    kiss_fft_cpx rotations[3 * LHDC_LLAC_DCT_MAX_LENGTH / 2];
    LhdcLlacTransformScratch scratch;
    kiss_fft_cpx radix_scratch[LHDC_LLAC_DCT_RADIX_FLOATS / 2];
} LhdcLlacDctWorkspace;
#endif
