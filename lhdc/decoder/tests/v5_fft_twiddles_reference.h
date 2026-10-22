#ifndef LHDC_TEST_V5_FFT_TWIDDLES_REFERENCE_H
#define LHDC_TEST_V5_FFT_TWIDDLES_REFERENCE_H
#include "../src/v5/include/lhdc_fft.h"
typedef kiss_fft_cfg (*LhdcTestV5FftAllocator)(int, int, void *, size_t *, kiss_fft_cpx *);
kiss_fft_cfg lhdc_test_v5_fft_alloc_reference(int length, int inverse, void *memory,
                                              size_t *memory_bytes, kiss_fft_cpx *twiddles);
kiss_fft_cfg lhdc_test_v5_fft_alloc_cached_reference(int length, int inverse, void *memory,
                                                     size_t *memory_bytes, kiss_fft_cpx *twiddles);
#endif
