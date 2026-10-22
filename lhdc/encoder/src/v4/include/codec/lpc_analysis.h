#ifndef LHDC_ENCODER_V4_LPC_ANALYSIS_H
#define LHDC_ENCODER_V4_LPC_ANALYSIS_H
#include <stdint.h>
#include "compat/v4/lhdc_v4_constants.h"

#define LHDC_V4_LPC_MAX_ORDER 8

/* Large arrays live only during prediction analysis and expire before residual generation. */
typedef struct LhdcV4LpcWorkspace {
    float windowed_samples[LHDC_V4_MAX_BLOCK_SAMPLES];
    float coefficients[LHDC_V4_LPC_MAX_ORDER][LHDC_V4_LPC_MAX_ORDER];
    float errors[LHDC_V4_LPC_MAX_ORDER];
} LhdcV4LpcWorkspace;

/* Order, quantized coefficients, shift, and coding cost for one LPC selection. */
typedef struct LhdcV4LpcResult {
    uint32_t order;
    int32_t coefficients[LHDC_V4_LPC_MAX_ORDER];
    int32_t quantization_shift;
    uint32_t bits_needed;
} LhdcV4LpcResult;

uint32_t v4_lpc_leading_zeros32(uint32_t value);

void v4_lpcV_Carcow(const int32_t *data, uint32_t data_len, const int32_t *qlp_coeff,
                    uint32_t order, int32_t lp_quantization, int32_t *residual);

#endif
