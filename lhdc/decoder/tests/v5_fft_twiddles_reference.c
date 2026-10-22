#include "v5_fft_twiddles_reference.h"
#include <audio_math/float_rounding.h>
#include "../../common/include/dsp/float_compat.h"
#include <audio_math/constants.h>
#include <math.h>
#include "../src/v5/codec/fft_adapter_tables.h"
#include <audio_math/math.h>

/* Generate quantized twiddles using host trig/factorization and compare table-based initialization. */
kiss_fft_cfg lhdc_test_v5_fft_alloc_reference(int nfft, int inverse_fft, void *mem, size_t *lenmem,
                                              kiss_fft_cpx *ptr_fft_twid) {
    if (!lenmem)
        return NULL;
    if (!mem || *lenmem < sizeof(kiss_fft_state)) {
        *lenmem = sizeof(kiss_fft_state);
        return NULL;
    }
    *lenmem = sizeof(kiss_fft_state);
    kiss_fft_cfg st = mem;
    st->nfft = nfft;
    st->inverse = inverse_fft;
    st->twiddles = ptr_fft_twid;
    for (int i = 0; i < nfft; ++i) {
        double phase = (double)i * -(double)AUDIO_MATH_TWO_PI_F32 / (double)nfft;
        if (st->inverse)
            phase = -phase;
        st->twiddles[i].r =
            audio_math_truncate_sat_i32_f64(audio_math_floor_f64(cos(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
        st->twiddles[i].i =
            audio_math_truncate_sat_i32_f64(audio_math_floor_f64(sin(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
    }
    int bound = (int)(lhdc_approx_sqrt_fused_f32((float)nfft) + 0.5f);
    int radix = 4;
    int *factors = st->factors;
    do {
        while (nfft % radix) {
            if (radix == 4)
                radix = 2;
            else if (radix == 2)
                radix = 3;
            else
                radix += 2;
            if (radix > bound)
                radix = nfft;
        }
        nfft /= radix;
        *factors++ = radix;
        *factors++ = nfft;
    } while (nfft > 1);
    return st;
}

/* Retain the previous full table-based initializer with floating factorization thresholds for independent incremental checks. */
kiss_fft_cfg lhdc_test_v5_fft_alloc_cached_reference(int nfft, int inverse_fft, void *mem,
                                                     size_t *lenmem, kiss_fft_cpx *ptr_fft_twid) {
    if (!lenmem)
        return NULL;
    if (!mem || *lenmem < sizeof(kiss_fft_state)) {
        *lenmem = sizeof(kiss_fft_state);
        return NULL;
    }
    *lenmem = sizeof(kiss_fft_state);
    kiss_fft_cfg st = mem;
    st->nfft = nfft;
    st->inverse = inverse_fft;
    st->twiddles = ptr_fft_twid;
    if (!lhdc_v5_fft_copy_twiddles(nfft, st->inverse, st->twiddles)) {
        for (int i = 0; i < nfft; ++i) {
            double phase = (double)i * -(double)AUDIO_MATH_TWO_PI_F32 / (double)nfft;
            if (st->inverse)
                phase = -phase;
            st->twiddles[i].r = audio_math_truncate_sat_i32_f64(
                audio_math_floor_f64(audio_math_host_cos(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
            st->twiddles[i].i = audio_math_truncate_sat_i32_f64(
                audio_math_floor_f64(audio_math_host_sin(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
        }
    }
    int bound = (int)(lhdc_approx_sqrt_fused_f32((float)nfft) + 0.5f);
    int radix = 4;
    int *factors = st->factors;
    do {
        while (nfft % radix) {
            if (radix == 4)
                radix = 2;
            else if (radix == 2)
                radix = 3;
            else
                radix += 2;
            if (radix > bound)
                radix = nfft;
        }
        nfft /= radix;
        *factors++ = radix;
        *factors++ = nfft;
    } while (nfft > 1);
    return st;
}
