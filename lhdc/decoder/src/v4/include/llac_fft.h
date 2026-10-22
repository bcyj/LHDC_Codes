#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_FFT_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_FFT_H
#include "llac_fft_setup.h"
void llac_dct4_transform(Dct4 *, float *, kiss_fft_cpx *);
void llac_synthesize_frame(float *, int, float *, float *, LhdcLlacPcmScratch *, Dct4 *);
uint32_t llac_window_scramble_key(int length);
#endif
