#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_BIT_OPERATIONS_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_BIT_OPERATIONS_H
#include <audio_math/bits.h>
/* Obtain significant width using shared leading-zero count; zero explicitly returns zero. */
static inline int32_t v5_calc_bits_inline(uint32_t value) {
    return (int32_t)(32u - audio_math_count_leading_zeros32(value));
}
#endif
