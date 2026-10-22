/* Per-instance seven-stage interpolation history; each stage stores only its two used delay samples. */
#ifndef LHDC_DECODER_SRC_V4_INCLUDE_UPSAMPLE_HISTORY_VIEW_H
#define LHDC_DECODER_SRC_V4_INCLUDE_UPSAMPLE_HISTORY_VIEW_H
#include <stddef.h>
typedef struct LHDCUpsampleStage {
    float previous_1;
    float previous_2;
} LHDCUpsampleStage;
typedef struct LHDCUpsampleHistory {
    LHDCUpsampleStage stages[7];
} LHDCUpsampleHistory;
#endif
