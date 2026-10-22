#ifndef LHDC_DECODER_V5_FRAME_DECODE_H
#define LHDC_DECODER_V5_FRAME_DECODE_H
#include <stdint.h>

/* Current channel-frame header and input-start snapshot, valid for this decode call only. */
typedef struct V5ChannelFrame {
    uint32_t spectral_count;
    uint32_t offset_index;
    int frequency_shift;
    uint32_t frequency_mode_bit;
    int noise_scale_q7;
    uint32_t header_status;
    uint32_t *input_word_cursor;
    uint32_t *input_begin;
    uint32_t cached_bit_count;
} V5ChannelFrame;

/* Cached low bits after FAC stops and the final spectral budget; not shared outside the instance. */
typedef struct V5ResidualBits {
    uint32_t bit_mask_or_shift;
    uint32_t bindata;
} V5ResidualBits;
#endif
