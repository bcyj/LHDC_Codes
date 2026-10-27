#include "../../../../platform/windows/include/lhdc_windows_exp_log.h"
#include <audio_math/float_properties.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include <audio_math/math.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_rounding.h>
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../include/codec/codec_internal.h"
#include <math.h>
#include "gain.h"

#if defined(__SSE2__) ||                                                                          \
    (defined(_MSC_VER) && (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86))))
#include <emmintrin.h>
#define LHDC_LLAC_QUANT_USE_SSE2 1
#else
#define LHDC_LLAC_QUANT_USE_SSE2 0
#endif

#ifndef LLAC_QUANT_FAST_RECIPROCAL
#define LLAC_QUANT_FAST_RECIPROCAL 0
#endif

/* Estimate quantization gain from spectral energy/target bits and update cross-frame compensation;
 * HR may add high-frequency regularization; after eight search steps, clamp minimum gain and linearize. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
void v4f_llac_estimate_quantization_gain_with_policy(
    LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ, LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
    LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff, LLAC_FLOAT *targetBitsOff,
    LLAC_INT *old_targetBits, LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
    LLAC_FLOAT frame_ms, unsigned math_policy, LLAC_FLOAT *energy_workspace) {
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                         \
    (defined(_M_IX86) || defined(_M_X64))
/* Reuse energy workspace to estimate gain from energy/bit budget and update budget history. */
void v4f_llac_estimate_quantization_gain(LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ,
                                         LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
                                         LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff,
                                         LLAC_FLOAT *targetBitsOff, LLAC_INT *old_targetBits,
                                         LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
                                         LLAC_FLOAT frame_ms, LLAC_FLOAT *energy_workspace) {
#else
/* Estimate quantization gain/minimum gain using caller energy workspace and update budget history. */
void v4f_llac_estimate_quantization_gain_with_workspace(
    LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ, LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
    LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff, LLAC_FLOAT *targetBitsOff,
    LLAC_INT *old_targetBits, LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
    LLAC_FLOAT frame_ms, LLAC_FLOAT *energy_workspace) {
#endif
    /* Write all grouped energies before reading; spectral quantization then overwrites the same instance partition. */
    LLAC_FLOAT *en = energy_workspace;
    LLAC_FLOAT target_offset = 0.0f;
    if (*old_targetBits >= 0) {
        LLAC_FLOAT tmp = (*targetBitsOff + (LLAC_FLOAT)*old_targetBits) - (LLAC_FLOAT)old_specBits;
        double correction = tmp > 40.0f ? 8.0 : tmp < -40.0f ? -8.0 : (double)tmp * 0.2;
        target_offset = (LLAC_FLOAT)((double)*targetBitsOff * 0.8 + correction);
    }
    *targetBitsOff = target_offset;
    *old_targetBits = nbitsSQ;
    LLAC_FLOAT x_max = 0.0f;
#if LHDC_LLAC_QUANT_USE_SSE2
    if (lg >= 4) {
        __m128 max_v = _mm_setzero_ps();
        __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
        LLAC_INT lg4 = lg & ~3;
        for (LLAC_INT i = 0; i < lg4; i += 4) {
            __m128 v = _mm_loadu_ps(&x[i]);
            v = _mm_and_ps(v, abs_mask);
            max_v = _mm_max_ps(max_v, v);
        }
        max_v = _mm_max_ps(max_v, _mm_shuffle_ps(max_v, max_v, _MM_SHUFFLE(1, 0, 3, 2)));
        max_v = _mm_max_ps(max_v, _mm_shuffle_ps(max_v, max_v, _MM_SHUFFLE(2, 3, 0, 1)));
        _mm_store_ss(&x_max, max_v);
        for (LLAC_INT i = lg4; i < lg; ++i) {
            LLAC_FLOAT val = audio_math_abs_f32(x[i]);
            if (val > x_max)
                x_max = val;
        }
    } else {
        for (LLAC_INT i = 0; i < lg; ++i) {
            LLAC_FLOAT val = audio_math_abs_f32(x[i]);
            if (val > x_max)
                x_max = val;
        }
    }
#else
    for (LLAC_INT i = 0; i < lg; ++i) {
        LLAC_FLOAT val = audio_math_abs_f32(x[i]);
        if (val > x_max)
            x_max = val;
    }
#endif
    LLAC_FLOAT reg_val = 0.0f;
    if (hrmode != 0 && regBits > 0) {
        LLAC_FLOAT M0 = LHDC_V4_LLAC_QUANT_ENERGY_FLOOR, M1 = LHDC_V4_LLAC_QUANT_ENERGY_FLOOR;
        for (LLAC_INT i = 0; i < lg; ++i) {
            LLAC_FLOAT ax = audio_math_abs_f32(x[i]);
            M0 += ax;
            M1 = (LLAC_FLOAT)i * ax + M1;
        }
        LLAC_FLOAT thresh = frame_ms + frame_ms;
        LLAC_FLOAT rB_offset = M1 / M0;
        if (!(rB_offset < thresh))
            rB_offset = thresh;
        reg_val =
            x_max * audio_math_exp2f((LLAC_FLOAT)((double)(LLAC_FLOAT)-regBits +
                                                  (1.0 - (double)(rB_offset / thresh)) * -8.0));
    }
    LLAC_FLOAT ind_min, ind;
    if (lg > 0 && x_max != 0.0f) {
        LLAC_FLOAT limit =
            hrmode == 1 ? LHDC_V4_LLAC_QUANT_LIMIT_HR : LHDC_V4_LLAC_QUANT_LIMIT_NORMAL;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)

        ind_min = (LLAC_FLOAT)audio_math_ceil_f64(
            (double)lhdc_windows_log10f_soft(x_max / limit, math_policy) * 28.0);
#else
        ind_min = (LLAC_FLOAT)audio_math_ceil_f64((double)audio_math_log10f(x_max / limit) * 28.0);
#endif
        const LLAC_FLOAT *xp = x;
        const LLAC_INT n_en = lg >> 2;
        for (LLAC_INT k = 0; k < n_en; ++k, xp += 4) {
            LLAC_FLOAT x0 = xp[0], x1 = xp[1], x2 = xp[2], x3 = xp[3];
            LLAC_FLOAT ener = reg_val + x0 * x0;
            ener += x1 * x1;
            ener += x2 * x2;
            ener += x3 * x3;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
            LLAC_FLOAT tmp =
                lhdc_windows_log10f_soft(ener + AUDIO_MATH_POW2_NEG_31_F32, math_policy);
#else
            LLAC_FLOAT tmp = audio_math_log10f(ener + AUDIO_MATH_POW2_NEG_31_F32);
#endif
            en[k] = (LLAC_FLOAT)(((double)tmp * 10.0 + 7.0) * 1.4);
        }

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        LLAC_FLOAT target =
            (LLAC_FLOAT)((double)audio_math_truncate_i32_f64(
                             (double)nbitsSQ + (double)audio_math_round_f32(target_offset)) *
                         LHDC_V4_LLAC_QUANT_BIT_BUDGET_SCALE);
#else
        LLAC_FLOAT target =
            (LLAC_FLOAT)((double)(LLAC_INT)((double)nbitsSQ +
                                            (double)audio_math_round_f32(target_offset)) *
                         LHDC_V4_LLAC_QUANT_BIT_BUDGET_SCALE);
#endif
        LLAC_INT offset = quantizedGainOff + 255;
        LLAC_FLOAT fac = 256.0f;
        for (LLAC_INT i = 0; i < 8; ++i) {
            fac *= 0.5f;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            /* offset and fac are bounded integers; binary-search float addition/subtraction is exact. */
            LLAC_FLOAT candidate =
                (LLAC_FLOAT)audio_math_truncate_i32_f32((LLAC_FLOAT)offset - fac);
#else
            LLAC_FLOAT candidate = (LLAC_FLOAT)(LLAC_INT)((LLAC_FLOAT)offset - fac);
#endif
            LLAC_FLOAT ener = 0.0f;
            LLAC_INT iszero = 1;
            for (LLAC_INT j = lg / 4 - 1; j >= 0; --j) {
                LLAC_FLOAT tmp = en[j] - candidate;
                if (!((double)tmp < 9.8)) {
                    iszero = 0;
                    if (!(tmp > 70.0f))
                        ener += tmp;
                    else
                        ener = (LLAC_FLOAT)((double)tmp + (double)tmp + (double)ener - 70.0);
                } else if (!iszero) {
                    ener = (LLAC_FLOAT)((double)ener + 3.78);
                }
            }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            offset = audio_math_truncate_i32_f32(fac + candidate);
            if (!(ener > target) || iszero)
                offset = audio_math_truncate_i32_f32(candidate);
#else
            offset = (LLAC_INT)(fac + candidate);
            if (!(ener > target) || iszero)
                offset = (LLAC_INT)candidate;
#endif
        }
        ind = (LLAC_FLOAT)offset;
        if (ind < ind_min) {
            *old_targetBits = -1;
            ind = ind_min;
        }
        ind -= (LLAC_FLOAT)quantizedGainOff;
    } else {
        ind_min = (LLAC_FLOAT)quantizedGainOff;
        ind = 0.0f;
        *old_targetBits = -1;
    }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    *quantizedGainMin = audio_math_truncate_i32_f32(ind_min);
    *quantizedGain = audio_math_truncate_i32_f32(ind);
#else
    *quantizedGainMin = (LLAC_INT)ind_min;
    *quantizedGain = (LLAC_INT)ind;
#endif
    *gain = llac_gain_from_step(ind + (LLAC_FLOAT)quantizedGainOff);
}
#include "../include/llac/llac_const.h"

/* Adjust global gain index to actual/target bits, respect the minimum index, and recompute linear gain. */
void v4f_llac_adjust_quantization_gain(LLAC_INT *gg_idx, LLAC_INT gg_idx_min, LLAC_INT gg_idx_off,
                                       LLAC_FLOAT *gain, LLAC_INT target, LLAC_INT nBits,
                                       LLAC_INT *gainChange, LLAC_INT fs_idx) {
    LLAC_FLOAT delta;
    if (nBits < alg_quant_t2[fs_idx])
        delta = (LLAC_FLOAT)(((double)nBits + 48.0) * 0.0625);
    else if (nBits < alg_quant_t3[fs_idx])
        delta = (alg_quant_t6[fs_idx] + (LLAC_FLOAT)nBits) * alg_quant_t5[fs_idx];
    else {
        LLAC_INT limited_bits = nBits < alg_quant_t4[fs_idx] ? nBits : alg_quant_t4[fs_idx];
        delta = (LLAC_FLOAT)((double)limited_bits / 48.0);
    }
    delta = audio_math_round_f32(delta);
    *gainChange = target < nBits && *gg_idx == 255;
    LLAC_INT index = *gg_idx;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    LLAC_INT delta2 = audio_math_truncate_i32_f32(delta + 2.0f);
#else
    LLAC_INT delta2 = (LLAC_INT)(delta + 2.0f);
#endif
    if ((!(target < nBits && index < 255)) && (index < 1 || target - delta2 <= nBits))
        return;
    LLAC_INT change;
    if (nBits < target - delta2)
        change = -1;
    else if (index == 254 || (LLAC_FLOAT)nBits < delta + (LLAC_FLOAT)target)
        change = 1;
    else
        change = 2;
    LLAC_INT minimum_index = gg_idx_min - gg_idx_off;
    index += change;
    if (index < minimum_index)
        index = minimum_index;
    *gg_idx = index;
    *gain = llac_gain_from_step((LLAC_FLOAT)(index + gg_idx_off));
    *gainChange = 1;
}

/* Estimate noise-fill amplitude from quantized zero-bin neighborhoods, returning an index clamped to 0..7. */
void v4f_llac_estimate_noise_factor(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gg,
                                    LLAC_INT freq_upper_bound, LLAC_INT frame_dms, LLAC_INT *nf_q) {
#if defined(__arm__) || defined(__aarch64__) ||                                                    \
    (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                          \
     (defined(_M_IX86) || defined(_M_X64)))
    /* Accumulate in the same bin order without a 960-entry stack index array. */
    LLAC_INT zero_freq_count = 0, zero_freq_sum = 0;
    LLAC_FLOAT sum = 0.0f;
#else
    LLAC_INT zero_freq[LHDC_V4_LLAC_MAX_FRAME_SAMPLES], zero_freq_count = 0, zero_freq_sum = 0;
#endif
    LLAC_INT freq_lower_bound = frame_dms == LHDC_V4_FRAME_DURATION_5MS ? 12 : 6;
    LLAC_INT freq_check_width = frame_dms == LHDC_V4_FRAME_DURATION_5MS ? 2 : 1;
    for (LLAC_INT i = freq_lower_bound; i < freq_upper_bound; ++i) {
        LLAC_INT start = i - freq_check_width;
        LLAC_INT end = i + freq_check_width;
        if (end >= freq_upper_bound)
            end = freq_upper_bound - 1;
        LLAC_INT zero_flag = 1;
        for (LLAC_INT j = start; j <= end; ++j) {
            if (xq[j]) {
                zero_flag = 0;
                break;
            }
        }
        if (zero_flag) {
#if defined(__arm__) || defined(__aarch64__) ||                                                    \
    (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                          \
     (defined(_M_IX86) || defined(_M_X64)))
            ++zero_freq_count;
            zero_freq_sum += i + 1;
            LLAC_FLOAT ratio = x[i] / gg;
            if (ratio < 0.0f)
                ratio = -x[i] / gg;
            sum = sum + ratio;
#else
            zero_freq[zero_freq_count++] = i + 1;
            zero_freq_sum += i + 1;
#endif
        }
    }
    double nf_numerator = 0.0;
    if (zero_freq_count > 0 && zero_freq_sum > 0) {
#if !(defined(__arm__) || defined(__aarch64__) ||                                                  \
      (defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                        \
       (defined(_M_IX86) || defined(_M_X64))))
        LLAC_FLOAT sum = 0.0f;
        for (LLAC_INT i = 0; i < zero_freq_count; ++i) {
            LLAC_INT j = zero_freq[i] - 1;
            LLAC_FLOAT ratio = x[j] / gg;
            if (ratio < 0.0f)
                ratio = -x[j] / gg;
            sum = sum + ratio;
        }
#endif
        LLAC_FLOAT mean = sum / (LLAC_FLOAT)zero_freq_count;
        nf_numerator = (double)mean + (double)mean;
    }
    LLAC_FLOAT nf = (LLAC_FLOAT)(1.0 - nf_numerator);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* nf has rounded to float; scale by a power of two and add one half in extended precision before truncating. */
    LLAC_INT index = audio_math_truncate_i32_f64((double)nf * 8.0 + (nf < 0.0f ? -0.5 : 0.5));
#else
    LLAC_INT index = (LLAC_INT)(nf * 8.0f + (nf < 0.0f ? -0.5f : 0.5f));
#endif
    if (index < 0)
        index = 0;
    if (index > 7)
        index = 7;
    *nf_q = index;
}
#include "../include/llac/llac_const.h"
#include <stdlib.h>
#include <string.h>

/* Scalar-quantize spectrum with a signed offset and estimate entropy bits using two-sample contexts. */

void v4f_llac_quantize_spectrum(LLAC_FLOAT *x, LLAC_FLOAT gain, LLAC_INT *xq, LLAC_INT nt,
                                LLAC_INT totalBits, LLAC_INT *nbits, LLAC_INT *nbits2, LLAC_INT fs,
                                LLAC_INT *lastnzout, LlacSpectralPairCode *pair_codes,
                                LLAC_INT *lsbMode, LLAC_INT mode, LLAC_INT target,
                                LLAC_INT hrmode) {
    LLAC_FLOAT offset = hrmode ? 0.5f : 0.375f;
    LLAC_INT i = 0;
#if LHDC_LLAC_QUANT_USE_SSE2
    __m128 zero = _mm_setzero_ps();
    __m128 offset_pos = _mm_set1_ps(offset);
    __m128 offset_neg = _mm_set1_ps(-offset);
#if LLAC_QUANT_FAST_RECIPROCAL
    __m128 inv_gain_vec = _mm_set1_ps(1.0f / gain);
#else
    __m128 gain_vec = _mm_set1_ps(gain);
#endif
    for (; i <= nt - 4; i += 4) {
        __m128 x_vec = _mm_loadu_ps(&x[i]);
        __m128 gt = _mm_cmpgt_ps(x_vec, zero);
        __m128 lt = _mm_cmplt_ps(x_vec, zero);
        __m128 sign_val = _mm_or_ps(_mm_and_ps(gt, offset_pos), _mm_and_ps(lt, offset_neg));
#if LLAC_QUANT_FAST_RECIPROCAL
        __m128 scaled = _mm_mul_ps(x_vec, inv_gain_vec);
#else
        __m128 scaled = _mm_div_ps(x_vec, gain_vec);
#endif
        __m128 val = _mm_add_ps(scaled, sign_val);
        __m128i int_val = _mm_cvttps_epi32(val);
        if (!hrmode) {
            __m128i max_val = _mm_set1_epi32(32767);
            __m128i min_val = _mm_set1_epi32(-32768);
            __m128i gt_max = _mm_cmpgt_epi32(int_val, max_val);
            int_val =
                _mm_or_si128(_mm_and_si128(gt_max, max_val), _mm_andnot_si128(gt_max, int_val));
            __m128i lt_min = _mm_cmpgt_epi32(min_val, int_val);
            int_val =
                _mm_or_si128(_mm_and_si128(lt_min, min_val), _mm_andnot_si128(lt_min, int_val));
        }
        _mm_storeu_si128((__m128i *)&xq[i], int_val);
    }
#endif
    for (; i < nt; ++i) {
        LLAC_FLOAT sign = x[i] > 0.0f ? 1.0f : x[i] < 0.0f ? -1.0f : 0.0f;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* Evaluate division and bias addition in extended precision, then convert directly to integer without intermediate float stores. */
        xq[i] = audio_math_truncate_i32_f64((double)x[i] / (double)gain + (double)(offset * sign));
#else
        xq[i] = (LLAC_INT)(x[i] / gain + offset * sign);
#endif
        if (!hrmode) {
            if (xq[i] < -32768)
                xq[i] = -32768;
            if (xq[i] > 32767)
                xq[i] = 32767;
        }
    }
    LLAC_INT rateFlag;
    if (fs < LHDC_V4_SAMPLE_RATE_48000) {
        LLAC_INT rate_limit = (fs / 8000) * 160;
        rateFlag = totalBits > rate_limit ? 512 : 0;
        if (mode == 0 && totalBits >= rate_limit + 320)
            mode = 1;
    } else {
        rateFlag = fs == LHDC_V4_SAMPLE_RATE_48000 && totalBits > 800 ? 512 : 0;
        if (mode == 0 && fs == LHDC_V4_SAMPLE_RATE_48000 && totalBits > 1119)
            mode = 1;
    }
    LLAC_INT lastnz = nt;
    while (lastnz >= 4 && xq[lastnz - 1] == 0 && xq[lastnz - 2] == 0)
        lastnz -= 2;
    if (lastnz < 4)
        lastnz = 2;
    LLAC_INT lastnz2 = mode >= 0 ? 2 : lastnz;
    *nbits = 0;
    *nbits2 = 0;
    LLAC_INT c = 0, nbits_lsb = 0;
    for (LLAC_INT k = 0; k < lastnz - 1; k += 2) {
        LLAC_INT t = c + rateFlag + (k > nt / 2 ? 256 : 0);
        LLAC_INT a = abs(xq[k]), b = abs(xq[k + 1]);
        LLAC_INT pair_nonzero = a != 0 || b != 0;
        pair_codes->context = t;
        if (mode < 1)
            *nbits += (xq[k] != 0 ? 2048 : 0) + (xq[k + 1] != 0 ? 2048 : 0);
        LLAC_INT lev1 = 0;
        LLAC_INT escape_count = 0;
        while (a >= 4 || b >= 4) {
            LLAC_INT pki = alg_code_t3[t + lev1 * 1024];
            *nbits += alg_quant_t1[pki][16];
            if (mode < 1 || lev1 != 0)
                *nbits += 4096;
            else
                nbits_lsb += 2;
            a >>= 1;
            b >>= 1;
            ++escape_count;
            if (lev1 > 1)
                lev1 = 2;
            ++lev1;
        }
        /* Full escape counts match integer splitting; lev1 is only the probability context capped at level three. */
        pair_codes->escape_count = pair_nonzero ? escape_count : -1;
        LLAC_INT pki = alg_code_t3[t + lev1 * 1024];
        LLAC_INT sym = a + 4 * b;
        pair_codes->symbol = sym;
        *nbits += alg_quant_t1[pki][sym];
        if (mode > 0) {
            if (lev1 == 0) {
                *nbits += (xq[k] != 0 ? 2048 : 0) + (xq[k + 1] != 0 ? 2048 : 0);
            } else {
                LLAC_INT a1_msb = abs(xq[k]) & ~1;
                LLAC_INT b1_msb = abs(xq[k + 1]) & ~1;
                nbits_lsb += xq[k] != 0 && a1_msb == 0;
                nbits_lsb += xq[k + 1] != 0 && b1_msb == 0;
                *nbits += (a1_msb != 0 ? 2048 : 0) + (b1_msb != 0 ? 2048 : 0);
            }
        }
        if (mode >= 0 && (xq[k] != 0 || xq[k + 1] != 0) && *nbits <= target * 2048) {
            *nbits2 = *nbits;
            lastnz2 = k + 2;
        }
        c = (c & 15) * 16 + (lev1 < 2 ? (lev1 + 1) * (a + b) + 1 : lev1 + 12);
        ++pair_codes;
    }
    *nbits = (*nbits + 2047) >> 11;
    if (mode >= 0) {
        *nbits2 = (*nbits2 + 2047) >> 11;
        if (mode != 0) {
            *nbits += nbits_lsb;
            *nbits2 += nbits_lsb;
        }
    } else {
        *nbits2 = *nbits;
    }
    if (lastnz2 < lastnz)
        memset(xq + lastnz2, 0, (size_t)(lastnz - lastnz2) * sizeof(LLAC_INT));
    *lsbMode = mode > 0 && *nbits > target;
    *lastnzout = lastnz2;
}

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Stateless algorithm entry selecting host policy automatically; frame encoding uses the instance-policy entry. */
void v4f_llac_estimate_quantization_gain(LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ,
                                         LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
                                         LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff,
                                         LLAC_FLOAT *targetBitsOff, LLAC_INT *old_targetBits,
                                         LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
                                         LLAC_FLOAT frame_ms, LLAC_FLOAT *energy_workspace) {
    v4f_llac_estimate_quantization_gain_with_policy(
        x, lg, nbitsSQ, gain, quantizedGain, quantizedGainMin, quantizedGainOff, targetBitsOff,
        old_targetBits, old_specBits, hrmode, regBits, frame_ms, lhdc_windows_log10_policy(),
        energy_workspace);
}
#endif

#if !(defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                         \
      (defined(_M_IX86) || defined(_M_X64)))
/* Convenience entry for standalone algorithms; frame encoding always uses caller-provided instance workspace. */
void v4f_llac_estimate_quantization_gain(LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ,
                                         LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
                                         LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff,
                                         LLAC_FLOAT *targetBitsOff, LLAC_INT *old_targetBits,
                                         LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
                                         LLAC_FLOAT frame_ms) {
    LLAC_FLOAT energy_workspace[LHDC_V4_LLAC_MAX_FRAME_SAMPLES / 4];
    v4f_llac_estimate_quantization_gain_with_workspace(
        x, lg, nbitsSQ, gain, quantizedGain, quantizedGainMin, quantizedGainOff, targetBitsOff,
        old_targetBits, old_specBits, hrmode, regBits, frame_ms, energy_workspace);
}
#endif
