#ifndef LHDC_ENCODER_V5_CODEC_MDCT_TABLES_H
#define LHDC_ENCODER_V5_CODEC_MDCT_TABLES_H
#include "../../../../common/include/types/lhdc_dsp_types.h"
/* Copy fixed production encoding windows; unsupported sizes/null output return zero without writes. */
int lhdc_v5_mdct_copy_window(int overlap, float *window);
/* Copy production pre/post rotations; misses/null output return zero without writes. */
int lhdc_v5_mdct_copy_rotations(int frame_samples, int shift_bits,
                                kiss_fft_cpx *pre, kiss_fft_cpx *post);
#if LHDC_V5_ENCODER_FIXED_FFT
#include "../../../../common/include/dsp/fft_fixed32.h"
/* Optional fixed encoding path only; failed copies fall back to host evaluation. */
int lhdc_v5_mdct_copy_fixed_twiddles(int length, LhdcFftFixed32Complex *twiddles);
#endif
#endif
