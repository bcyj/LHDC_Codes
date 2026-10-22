#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_V4_PCM_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_V4_PCM_H
#include <stdint.h>
#include <stddef.h>
#include "../types/lhdc_native_types.h"
#include "lpc_analysis.h"
#define V4_ENCODER_NATIVE_SIZE sizeof(struct lhdc_Encoder_s)
int32_t v4_Get2ChWavData16(struct FFT_block_s *fb, uint8_t *wav, int32_t ns);
int32_t v4_Get2ChWavData24(struct FFT_block_s *fb, uint8_t *wav, int32_t ns);
void v4_applyQuantization(struct lhdc_Encoder_s *encb, int32_t *src, int32_t *dst, int32_t div,
                          int32_t down_step);
int32_t v4_lz_make_data(uint32_t *rd2, int32_t ns, int32_t sh, int32_t nb, uint8_t *uc);
int32_t v4_encode_residual_block(uint32_t *rd2, int32_t ns, uint32_t nb, uint8_t *out,
                                 int32_t out_len);
#endif
