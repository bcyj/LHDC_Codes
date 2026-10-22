#ifndef LHDC_ENCODER_SRC_V5_CODEC_FRAME_HEADER_H
#define LHDC_ENCODER_SRC_V5_CODEC_FRAME_HEADER_H
#include "../include/codec/scalar.h"
/* Update masked bits in the specified byte. */
static inline int32_t v5i_clean_frame_api_set_bits(int32_t value, uint32_t width, uint32_t shift,
                                                   v5_workspace_directory *ws) {
    if ((uint32_t)value >= (1u << width) || ws == NULL)
        return LHDC_V5_ENC_ERROR;
    v5_frame_header *h = ws->header;
    uint16_t mask = (uint16_t)(((1u << width) - 1u) << shift);
    h->info = (uint16_t)((h->info & ~mask) | ((uint32_t)value << shift));
    return LHDC_V5_ENC_OK;
}

/* Read masked bits from the specified byte. */
static inline int32_t v5i_clean_frame_api_get_bits(int32_t *value, uint32_t width, uint32_t shift,
                                                   v5_workspace_directory *ws) {
    if (value == NULL || ws == NULL)
        return LHDC_V5_ENC_ERROR;
    const v5_frame_header *h = ws->header;
    *value = (h->info >> shift) & ((1u << width) - 1u);
    return LHDC_V5_ENC_OK;
}
#endif
