#ifndef LHDC_COMMON_SRC_DSP_DCT4_FLOAT32_TABLES_H
#define LHDC_COMMON_SRC_DSP_DCT4_FLOAT32_TABLES_H
#include "../../include/dsp/fft_float32.h"

/* length/2 pre-rotations followed by length/2 post-rotations; read-only with one storage definition. */
const kiss_fft_cpx *lhdc_dct4_f32_static_rotations(int32_t length);
#endif
