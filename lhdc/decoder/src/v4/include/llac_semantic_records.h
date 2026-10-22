

#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SEMANTIC_RECORDS_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SEMANTIC_RECORDS_H
#include <stddef.h>
#include <stdint.h>
#include "../../../../common/include/types/llac_side_types.h"

typedef struct LLACOrderBoundaries {
    uint32_t before_order_1, before_order_2, before_order_3, before_order_4;
    uint32_t before_order_5, before_order_6, before_order_7, before_order_8, total;
} LLACOrderBoundaries;
typedef union LLACOrderCDF {
    LLACOrderBoundaries order;
    uint32_t by_symbol[9];
} LLACOrderCDF;
typedef struct LLACOrderModel {
    uint32_t frequency[8];
    LLACOrderCDF cumulative;
} LLACOrderModel;

typedef struct LLACReflectionBoundaries {
    uint32_t before_index_0, before_index_1, before_index_2, before_index_3;
    uint32_t before_index_4, before_index_5, before_index_6, before_index_7;
    uint32_t before_index_8, before_index_9, before_index_10, before_index_11;
    uint32_t before_index_12, before_index_13, before_index_14, before_index_15;
    uint32_t before_index_16, total;
} LLACReflectionBoundaries;
typedef union LLACReflectionCDF {
    LLACReflectionBoundaries index;
    uint32_t by_symbol[18];
} LLACReflectionCDF;
typedef struct LLACReflectionModel {
    uint32_t frequency[17];
    LLACReflectionCDF cumulative;
} LLACReflectionModel;

/* The symbol's low two bits encode the first spectral value, high bits the second; 16 continues escape decoding. */
typedef struct LLACPairBoundaries {
    uint32_t before_pair_0_0, before_pair_1_0, before_pair_2_0, before_pair_3_0;
    uint32_t before_pair_0_1, before_pair_1_1, before_pair_2_1, before_pair_3_1;
    uint32_t before_pair_0_2, before_pair_1_2, before_pair_2_2, before_pair_3_2;
    uint32_t before_pair_0_3, before_pair_1_3, before_pair_2_3, before_pair_3_3;
    uint32_t before_escape, total;
} LLACPairBoundaries;
typedef union LLACPairCDF {
    LLACPairBoundaries pair;
    uint32_t by_symbol[18];
} LLACPairCDF;
typedef struct LLACPairModel {
    uint32_t frequency[17];
    LLACPairCDF cumulative;
} LLACPairModel;

/* Per-frame forward arithmetic and backward side-information cursors share input but advance independently. */
typedef struct LLACArithmeticReader {
    const uint8_t *bytes;
    uint32_t range;
    uint32_t low;
    int byte_position;
    int exhausted;
} LLACArithmeticReader;

typedef struct LLACSideReader {
    const uint8_t *bytes;
    int byte_position;
    uint32_t mask;
} LLACSideReader;

/* Eight keys used by the descrambler. */
typedef struct LLACDecodeKeys {
    uint32_t tns_order_cdf, tns_reflection_cdf, spectral_context, spectral_cdf;
    uint32_t sns_low_codebook, sns_high_codebook, sns_mpvq;
    uint32_t sns_gain;
} LLACDecodeKeys;

#endif
