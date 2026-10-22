#ifndef LHDC_DECODER_SRC_V5_CODEC_SYNTHESIS_H
#define LHDC_DECODER_SRC_V5_CODEC_SYNTHESIS_H
#include <stdint.h>
#include "../../../../../math/include/audio_math/integer.h"
/* Extract Q31 middle bits from a modulo-2^64 result, preserving truncation and wraparound. */
static inline int time_q31(uint64_t product) {
    return audio_math_q31_truncate_wrap_i64(product);
}

/* Compute a signed 32-by-32 product as modulo-2^64 bits for subsequent arithmetic. */
static inline uint64_t time_product(int sample, int coefficient) {
    return (uint64_t)((int64_t)sample * coefficient);
}
#endif
