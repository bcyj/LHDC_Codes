#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_V5_PCM_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_V5_PCM_H
#include "scalar.h"

void v5_sns_smoothing(int32_t n, int32_t *in, int32_t *out); /* distinct arrays, n >= 4 */
void v5_adsq_enc(int32_t *din, int32_t *dout, int32_t dn, uint8_t *code, int32_t *fi,
                 int32_t max_fi);
int32_t v5_RiceQuotientEncode(uint32_t *q, int32_t qn, int32_t min_p0, uint8_t *qexp);
int32_t v5_BitWriteZeroPadToByteBoundary(v5_bit_writer *bw);
int32_t v5_BitWriteBitsArrayU8(v5_bit_writer *bw, uint8_t *ba, int32_t bits, int32_t len,
                               int32_t pad_zero);
#endif
