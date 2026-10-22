#ifndef LHDC_LLAC_SIDE_TYPES_H
#define LHDC_LLAC_SIDE_TYPES_H
#include "llac_scalar_types.h"

/* LLAC SNS side information shared by encoding and decoding; instances own actual structures, not reinterpreted integer arrays. */
typedef struct LLACSnsIndices {
    LLAC_INT low_codebook;
    LLAC_INT high_codebook;
    LLAC_INT submode;
    LLAC_INT gain;
    LLAC_INT leading_sign;
    LLAC_INT main_mpvq;
    LLAC_INT secondary_mpvq;
} LLACSnsIndices;

/* LTPF presence flag, activation flag, and pitch index. */
typedef struct LLACLtpfIndices {
    LLAC_INT present;
    LLAC_INT active;
    LLAC_INT pitch;
} LLACLtpfIndices;

/* TNS filter count, orders, and per-filter reflection-coefficient indices for one frame. */
typedef struct LLACTnsData {
    LLAC_INT filter_count;
    LLAC_INT order[2];
    LLAC_INT reflection_indices[16];
} LLACTnsData;
#endif
