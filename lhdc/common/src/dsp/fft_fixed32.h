#ifndef LHDC_COMMON_SRC_DSP_FFT_FIXED32_H
#define LHDC_COMMON_SRC_DSP_FFT_FIXED32_H
#include "../../include/dsp/fft_fixed32.h"
#include <audio_math/numeric_bits.h>
/* Perform defined 32-bit modular addition. */
static inline int32_t add(int32_t a, int32_t b) {
    return audio_math_int32_from_bits((uint32_t)a + (uint32_t)b);
}

/* Perform defined 32-bit modular subtraction. */
static inline int32_t sub(int32_t a, int32_t b) {
    return audio_math_int32_from_bits((uint32_t)a - (uint32_t)b);
}

/* Perform sign-extending right shifts; this module uses only shift counts 1/2. */
static inline int32_t shift(int32_t value, unsigned bits) {
    uint32_t shifted = (uint32_t)value >> bits;
    if (value < 0)
        shifted |= UINT32_MAX << (32 - bits);
    return audio_math_int32_from_bits(shifted);
}

/* Compute a safe signed 32-by-32 product and return its 64-bit pattern. */
static inline uint64_t product(int32_t a, int32_t b) {
    return (uint64_t)((int64_t)a * b);
}

/* Add half an LSB modulo 2^64 and extract the middle Q31 word, following arithmetic-shift rules for negatives. */
static inline int32_t rounded_q31(uint64_t bits) {
    return audio_math_int32_from_bits((uint32_t)((bits + UINT64_C(0x40000000)) >> 31));
}

/* Multiply a Q31 coefficient by an integer sample using biased rounding. */
static inline int32_t multiply(int32_t value, int32_t coefficient) {
    return rounded_q31(product(value, coefficient));
}

/* Complex modular addition. */
static inline LhdcFftFixed32Complex plus(LhdcFftFixed32Complex a, LhdcFftFixed32Complex b) {
    LhdcFftFixed32Complex c = {add(a.r, b.r), add(a.i, b.i)};
    return c;
}

/* Complex modular subtraction. */
static inline LhdcFftFixed32Complex minus(LhdcFftFixed32Complex a, LhdcFftFixed32Complex b) {
    LhdcFftFixed32Complex c = {sub(a.r, b.r), sub(a.i, b.i)};
    return c;
}

/* Combine two wide products before rounding; do not replace with separately rounded scalar products. */
static inline LhdcFftFixed32Complex rotate(LhdcFftFixed32Complex value,
                                           LhdcFftFixed32Complex twiddle) {
    LhdcFftFixed32Complex c = {
        rounded_q31(product(value.r, twiddle.r) - product(value.i, twiddle.i)),
        rounded_q31(product(value.r, twiddle.i) + product(value.i, twiddle.r))};
    return c;
}

/* Scale radix 2/4 with arithmetic shifts, rounding negative odd values toward negative infinity. */
static inline LhdcFftFixed32Complex shifted(LhdcFftFixed32Complex value, unsigned bits) {
    LhdcFftFixed32Complex c = {shift(value.r, bits), shift(value.i, bits)};
    return c;
}

/* Scale radix 3/5 with Q31 reciprocal multiplication. */
static inline LhdcFftFixed32Complex divided(LhdcFftFixed32Complex value, int32_t divisor) {
    LhdcFftFixed32Complex c = {multiply(value.r, INT32_MAX / divisor),
                               multiply(value.i, INT32_MAX / divisor)};
    return c;
}
#endif
