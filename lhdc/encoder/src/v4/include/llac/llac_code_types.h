/* Internal LLAC arithmetic-coder state, extracted from implementation with field order unchanged. */
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_LLAC_LLAC_CODE_TYPES_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_LLAC_LLAC_CODE_TYPES_H
#include "../types/lhdc_base_types.h"
#include <stdint.h>

/* Arithmetic context, full escape count, and final symbol for each spectral pair. */
typedef struct LlacSpectralPairCode {
    LLAC_INT context;
    LLAC_INT escape_count;
    LLAC_INT symbol;
} LlacSpectralPairCode;
typedef struct {
    uint32_t low, range;
    LLAC_INT bp, cache, carry, carry_count;
    LLAC_UINT8 *ptr;
} RangeEncoder;
#endif
