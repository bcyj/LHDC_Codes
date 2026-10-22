#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_QUANT_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_QUANT_H
#include <stdint.h>

void llac_fill_spectral_noise(int nf_q, float *xq, int nf_seed, int freq_upper_bound, int frame_dms,
                              int *zero_freq);
void llac_apply_global_gain(float *x, int xLen, int global_gain_idx, int global_gain_off);
void llac_decode_residual_bits(int *bitsRead, float *x, int L_spec, uint8_t *prm, int resQBits);
#endif
