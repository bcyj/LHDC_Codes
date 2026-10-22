#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LHDC_UPSAMPLE_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LHDC_UPSAMPLE_H
#include "lhdc_dec_core.h"
#include "upsample_history_view.h"

/* Clear 24 filter-history entries for the specified channel. */
void ResetDecodeUps2xLPF(lhdc_Decoder_t *, int);
/* Generate 2x interpolated samples with seven-stage recursive floating low-pass filtering, preserving fused rounding and clipping. */
void DecodeUps2xLPF(lhdc_Decoder_t *, int, int *, int *, int, int);
/* Process eight samples and maintain 48 cross-block output entries; ns=32 also processes the remaining 24 samples. */
void fast_upsampling_2x(lhdc_Decoder_t *, int *, int *, int, int, int, int);
#endif
