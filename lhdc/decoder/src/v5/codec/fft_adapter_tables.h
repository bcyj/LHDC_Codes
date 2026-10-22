#ifndef LHDC_DECODER_V5_FFT_ADAPTER_TABLES_H
#define LHDC_DECODER_V5_FFT_ADAPTER_TABLES_H
#include "../../../../common/include/dsp/fft_fixed32.h"

/* Fixed lengths 60/120/240/480 contain only factors 2/3/5; radix-4-first trial division stops at five. */
#define LHDC_V5_FIXED_FFT_MAX_RADIX 5

/* V5 float full-turn policy; not interchangeable with FFT tables using full double-precision pi. */
int lhdc_v5_fft_copy_twiddles(int length, int inverse, LhdcFftFixed32Complex *twiddles);
#endif
