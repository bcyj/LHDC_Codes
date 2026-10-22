#include "../include/codec/bit_operations.h"
#include "../include/codec/fast_math.h"
/* V5 KISS FFT adapter: V5 owns state/external twiddles; the common backend performs computation. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../../../../common/include/dsp/fft_float32.h"
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <string.h>

/* Initialize KISS FFT factors and twiddles. */
kiss_fft_cfg_v5 v5f_lhdc_v5_fft_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem,
                                      kiss_fft_cpx *ptr_fft_twid) {
    if (!lenmem)
        return NULL;
    if (!mem || *lenmem < sizeof(kiss_fft_state_v5)) {
        *lenmem = sizeof(kiss_fft_state_v5);
        return NULL;
    }
    *lenmem = sizeof(kiss_fft_state_v5);
    kiss_fft_cfg_v5 st = mem;
    st->nfft = nfft;
    st->inverse = inverse_fft;
    st->twiddles = ptr_fft_twid;
    lhdc_kiss_fft_twiddles(nfft, inverse_fft, st->twiddles);
    /* Positive int32 lengths approximate below 47000; evaluate the bit-pattern approximation at single precision. */
    const int32_t threshold =
        audio_math_truncate_i32_f32(v5_fast_sqrt_inline((float)nfft) + .5f);
    lhdc_kiss_fft_factor(nfft, threshold, st->factors);
    return st;
}

