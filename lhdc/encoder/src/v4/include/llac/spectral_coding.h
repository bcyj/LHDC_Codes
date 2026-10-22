#ifndef LHDC_ENCODER_V4_LLAC_SPECTRAL_CODING_H
#define LHDC_ENCODER_V4_LLAC_SPECTRAL_CODING_H
#include "llac_code_types.h"
#include "../../../../../common/include/types/llac_side_types.h"

/* Quantize spectrum and produce per-pair arithmetic records and bit budgets. */
void v4f_llac_quantize_spectrum(LLAC_FLOAT *x, LLAC_FLOAT gain, LLAC_INT *xq, LLAC_INT nt,
                                LLAC_INT totalBits, LLAC_INT *nbits, LLAC_INT *nbits2, LLAC_INT fs,
                                LLAC_INT *lastnzout, LlacSpectralPairCode *pair_codes,
                                LLAC_INT *lsbMode, LLAC_INT mode, LLAC_INT target, LLAC_INT hrmode);

/* Encode TNS/spectral-pair records, signs, escaped low bits, and remaining bits. */
void v4f_llac_encode_spectral_symbols(LLAC_UINT8 *bytes, LLAC_INT bp_side, LLAC_INT mask_side,
                                      LLAC_INT *x, const LLACTnsData *tns, LLAC_INT lastnz,
                                      const LlacSpectralPairCode *pair_codes, LLAC_UINT8 *res_bits,
                                      LLAC_INT resBitsLen, LLAC_INT lsbMode, LLAC_INT nbbits,
                                      LLAC_INT enable_lpc_weighting);
#endif
