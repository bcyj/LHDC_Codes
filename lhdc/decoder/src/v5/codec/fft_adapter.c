#include <audio_math/float_rounding.h>
#include "../include/lhdc_fft.h"
#include <audio_math/constants.h>
#include <audio_math/math.h>
#include "../../../../common/include/dsp/float_compat.h"
#include "fft_adapter_tables.h"
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig_q31.h>
#endif

/* Copy fixed-length read-only coefficients; other lengths retain phase precision and factorization thresholds.
 * Native Windows kernel builds use bounded software Q31 trig; SDK builds retain host return policy.
 * All four executable lengths use read-only tables; other lengths support internal plan construction only.
 * x64 dynamic tables may differ from host CRT by one integer at rare rounding boundaries and are unused in supported decoding. */
kiss_fft_cfg lhdc_v5_fft_alloc(int nfft, int inverse_fft, void *mem, size_t *lenmem,
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
    const int fixed_length = lhdc_v5_fft_copy_twiddles(nfft, st->inverse, st->twiddles);
    if (!fixed_length) {
        for (int i = 0; i < nfft; ++i) {
            double phase = (double)i * -(double)AUDIO_MATH_TWO_PI_F32 / (double)nfft;
            if (st->inverse)
                phase = -phase;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            /* Positive length and i<nfft bound phase magnitude below float 2pi, inside the backend's valid interval. */
            (void)audio_math_sincos_bounded_f64_to_q31_soft(phase, &st->twiddles[i].i,
                                                      &st->twiddles[i].r);
#else
            st->twiddles[i].r = audio_math_truncate_sat_i32_f64(
                audio_math_floor_f64(audio_math_host_cos(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
            st->twiddles[i].i = audio_math_truncate_sat_i32_f64(
                audio_math_floor_f64(audio_math_host_sin(phase) * AUDIO_MATH_INT32_MAX_F64 + 0.5));
#endif
        }
    }
    /* Fixed-length trial division never exceeds five; no approximate square root or FMA is needed. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Thresholds for positive int32 lengths are bounded; all reachable float inputs have been enumerated.
     * Add 0.5 then truncate at single precision to obtain the integer threshold. */
    int bound = fixed_length
                    ? LHDC_V5_FIXED_FFT_MAX_RADIX
                    : audio_math_truncate_i32_f32(lhdc_approx_sqrt_fused_f32((float)nfft) + 0.5f);
#else
    int bound = fixed_length ? LHDC_V5_FIXED_FFT_MAX_RADIX
                             : (int)(lhdc_approx_sqrt_fused_f32((float)nfft) + 0.5f);
#endif
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

