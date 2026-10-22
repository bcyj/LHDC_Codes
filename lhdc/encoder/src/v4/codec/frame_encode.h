#ifndef LHDC_ENCODER_SRC_V4_CODEC_FRAME_ENCODE_H
#define LHDC_ENCODER_SRC_V4_CODEC_FRAME_ENCODE_H
#include <stdint.h>
#include "../../../../../math/include/audio_math/integer.h"
/* Return unsigned bit width, at least one; zero residual has width one. */
static inline uint32_t v4i_block_encode_residual_width(uint32_t value) {
    uint32_t bits = 1;
    while (value > 1) {
        ++bits;
        value >>= 1;
    }
    return bits;
}

/* Compute the unsigned magnitude of a folded residual. */
static inline uint32_t v4i_prediction_folded_magnitude(int32_t value) {
    return (uint32_t)value ^ audio_math_sign_mask_i32(value);
}

#endif
