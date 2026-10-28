#ifndef LHDC_ENCODER_SRC_V5_CODEC_SPECTRAL_SHAPING_H
#define LHDC_ENCODER_SRC_V5_CODEC_SPECTRAL_SHAPING_H
#include <stdint.h>
#include <audio_math/integer.h>
/* Multiply integers, shift right 31 bits, and return modulo 2^32. */
static inline int32_t v5i_sns_multiply_q31(int32_t a, int32_t b) {
    return audio_math_multiply_shift_wrap_i32(a, b, 31);
}

/* Multiply integers, shift right 31 bits, and return modulo 2^32. */
static inline int32_t v5i_pcm_clean_sns_adsq_mul31(int64_t a, int32_t b) {
    return audio_math_multiply_shift_wrap_i32(a, b, 31);
}
#endif
