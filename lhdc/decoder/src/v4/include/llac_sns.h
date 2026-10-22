#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SNS_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SNS_H
#include "../../../../common/include/types/llac_side_types.h"
/* Reconstruct sixteen floating scale factors from seven SNS codebook, submode, gain, and pulse indices. */
void llac_decode_sns_codebooks(const LLACSnsIndices *, float *);
#endif
