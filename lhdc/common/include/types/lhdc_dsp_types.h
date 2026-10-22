#ifndef LHDC_COMMON_DSP_PRIMITIVES_H
#define LHDC_COMMON_DSP_PRIMITIVES_H
#include "lhdc_base_types.h"
typedef union dat32_t dat32_t, *Pdat32_t;

union dat32_t {
    int32_t i32;
    uint16_t u16[2];
    int16_t i16[2];
    uint8_t byte[4];
};

#include <audio_math/fft_float32.h>
typedef AudioMathComplex32 kiss_fft_cpx, *Pkiss_fft_cpx;

typedef struct IEEESingle IEEESingle, *PIEEESingle;

typedef union anon_union_4_2_333ea59d_for_wordL anon_union_4_2_333ea59d_for_wordL,
    *Panon_union_4_2_333ea59d_for_wordL;

union anon_union_4_2_333ea59d_for_wordL {
    float wordLreal;
    uint32_t wordLuint;
};

struct IEEESingle {
    union anon_union_4_2_333ea59d_for_wordL wordL;
};

#endif
