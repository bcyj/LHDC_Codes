#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SIDE_INFO_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SIDE_INFO_H
#include <stdint.h>
#include "../../../../common/include/types/llac_side_types.h"
#include "../../../../common/include/types/llac_side_types.h"

void llac_read_side_information(uint8_t *, int, int *, int *, int, int, int, int *, int *,
                                LLACSnsIndices *, int *, LLACTnsData *tns, LLACLtpfIndices *, int *,
                                int *, int *, int);
#endif
