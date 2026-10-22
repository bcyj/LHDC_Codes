/* Derive encoding-workspace capacity from actual algorithm-type sizes. */
#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_LHDCV5_WORKSPACE_TYPES_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_LHDCV5_WORKSPACE_TYPES_H
#include "scalar.h"
#include "../types/lhdc_native_types.h"

typedef mdct_ch v5_mdct_channel;

typedef LhdcEncContext v5_encoder_core;

typedef fac_enc v5_arithmetic_state;
#define V5_ALIGN8(n) (((n) + 7u) & ~(size_t)7u)
#define V5_WORKSPACE_FIXED                                                                         \
    (V5_ALIGN8(sizeof(v5_workspace_directory)) + V5_ALIGN8(sizeof(v5_frame_header)) +              \
     V5_ALIGN8(sizeof(v5_encoder_core)) + V5_ALIGN8(sizeof(LHDCV5_MdctWorkspace)) +                \
     V5_ALIGN8(sizeof(v5_arithmetic_state)) + V5_ALIGN8(sizeof(v5_bit_writer)) +                   \
     2u * LHDC_V5_CHANNEL_ENCODED_BYTES + 3u * LHDC_V5_TRANSFORM_BUFFER_BYTES +                    \
     LHDC_V5_MDCT_WINDOW_BYTES)
#endif
