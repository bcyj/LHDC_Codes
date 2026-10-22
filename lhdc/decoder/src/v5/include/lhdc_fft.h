#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_FFT_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_FFT_H
#include "lhdc_dec_control.h"

typedef struct kiss_fft_state {
    int nfft, inverse, factors[64];
    kiss_fft_cpx *twiddles;
} kiss_fft_state;
typedef kiss_fft_state *kiss_fft_cfg;
/* Build FFT state, twiddles, and factors in caller memory; return null if capacity is insufficient. */
kiss_fft_cfg lhdc_v5_fft_alloc(int, int, void *, size_t *, kiss_fft_cpx *);
/* Execute the common fixed FFT with instance Q31 twiddles and no per-frame allocation. */
static inline void lhdc_v5_fft(kiss_fft_cfg state, const kiss_fft_cpx *input, kiss_fft_cpx *output) {
    const LhdcFftFixed32Plan plan = {state->nfft, state->inverse, state->twiddles};
    (void)lhdc_fft_fixed32_execute(&plan, input, output);
}

#endif
