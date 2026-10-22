#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SPECTRAL_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SPECTRAL_H
#include <stdint.h>
#include "../../../../common/include/types/llac_side_types.h"

void llac_decode_spectral_symbols(uint8_t *, int, int, int, int, int, LLACTnsData *tns, int, int,
                                  int *, int, int, uint8_t *, int *, int *, int *, int, int *,
                                  int *, int *, int);
#endif
