#ifndef LHDC_COMMON_INCLUDE_DSP_ADSQ_TABLES_H
#define LHDC_COMMON_INCLUDE_DSP_ADSQ_TABLES_H
#include <stdint.h>
#include "../compat/v5/lhdc_v5_constants.h"

/* Integer steps indexed by V5 ADSQ, shared by encoding difference prediction and decoding offset reconstruction. */
extern const int32_t lhdc_v5_adsq_step_sizes[LHDC_V5_ADSQ_TABLE_SIZE];
#endif
