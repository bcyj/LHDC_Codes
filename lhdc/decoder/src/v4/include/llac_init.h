#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_INIT_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_INIT_H
#include "llac_memory.h"
#include "llac_fft_setup.h"
#include "../../../../common/include/types/llac_side_types.h"

enum { LHDC_LLAC_DECODE_SPECTRUM_CAPACITY = 240 };
/* Reuse channel spectrum storage by phase: arithmetic escape levels first, floating spectrum only after conversion. */
typedef union LhdcLlacSpectrumScratch {
    float floating[LHDC_LLAC_DECODE_SPECTRUM_CAPACITY];
    int32_t levels[LHDC_LLAC_DECODE_SPECTRUM_CAPACITY];
} LhdcLlacSpectrumScratch;

typedef struct DecSetup {
    int *stDec_ola_mem_fx;
    int total_bits, enable_lpc_weighting, targetBytes, quantizedGainOff, nbLostFramesInRow,
        spec_inv_idx, concealMethod;
    LLACSnsIndices sns_indices;
    uint8_t resBits[625];
    LLACTnsData tns;
    LhdcLlacSpectrumScratch spectrum;
    float scf_q[16], int_scf[64];
    LhdcLlacPcmScratch pcm;
    float imdct_mem[240];
    Dct4 dct4structImdct;
} DecSetup;
void llac_configure_frame(LLAC_Dec *, LLACWorkspaceDirectory *);
uint8_t llac_init_state(LLAC_Dec *, int samplerate, int channels, LLACWorkspaceDirectory *);
uint8_t llac_set_channel_budget(LLAC_Dec *, int channel, int bytes);
uint8_t llac_init_checked(LLAC_Dec *, int samplerate, int channels, LLACWorkspaceDirectory *);
uint8_t llac_set_frame_duration(LLAC_Dec *, float frame_ms, LLACWorkspaceDirectory *);
uint8_t llac_dec_get_extra_func(uint8_t index, LLACWorkspaceDirectory *);
void llac_dec_set_channel_mode(uint8_t mode, LLACWorkspaceDirectory *);
#endif
