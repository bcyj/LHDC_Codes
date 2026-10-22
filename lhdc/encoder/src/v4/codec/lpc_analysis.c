#include <audio_math/float_properties.h>
#include "../../../../common/include/lhdc_log.h"
#include <audio_math/math.h>
#include <audio_math/constants.h>
#include "../../../../../math/include/audio_math/integer.h"
#include <audio_math/float_scaling.h>
#include <audio_math/float_convert.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#endif
#if defined(_KERNEL_MODE) && defined(_MSC_VER)
#include <audio_math/float_log.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/integer_ops.h"
#include "../include/codec/codec_internal.h"
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <limits.h>
#include "../include/codec/scalar.h"
#include "../include/codec/lpc_analysis.h"
#include "encoder_tables.h"
#include "../../../../platform/include/lhdc_print.h"

/* Count leading zeros in 32 bits; zero returns 32. */
uint32_t v4_lpc_leading_zeros32(uint32_t v) {
    return v ? (uint32_t)audio_math_count_leading_zeros32(v) : LHDC_V4_CLZ_ZERO_RESULT;
}

/* Estimate bits per sample from prediction error/scale; negative error returns a penalty. */
float v4f_lpcV_CabRer(float lpc_error, float error_scale) {
    if (!(lpc_error > 0.0f))
        return lpc_error < 0.0f ? LHDC_V4_LPC_INVALID_ERROR_COST : 0.0f;
    float scaled_error = lpc_error * error_scale;
    /* Nonpositive logarithmic costs clamp to zero; the final comparison selects zero for negatives and NaN.
     * Classify the rounded single-precision product first to avoid logarithms for these inputs. */
    if (!(scaled_error > 1.0f))
        return 0.0f;
#if defined(_KERNEL_MODE) && defined(_MSC_VER)
    return (float)((audio_math_log_f32_wide_soft(scaled_error) * 0.5) / AUDIO_MATH_LN_TWO_F64);
#else
    return (float)((audio_math_log((double)scaled_error) * 0.5) / AUDIO_MATH_LN_TWO_F64);
#endif
}

/* Compute integer prediction residuals from quantized LPC coefficients. */
void v4f_lpcV_Carcow(const int32_t *data, uint32_t data_len, const int32_t *qlp_coeff,
                     uint32_t order, int32_t lp_quantization, int32_t *residual) {
    v4_lpcV_Carcow(data, data_len, qlp_coeff, order, lp_quantization, residual);
}

/* Generate a rectangular window. */
void v4f_lpcV_window_rectangle(float *window, const int32_t L) {
    for (int32_t n = 0; n < L; ++n)
        window[n] = 1.0f;
}

/* Generate a symmetric length-L Hann window; native Windows kernel builds use bounded software evaluation.
 * Retain double phase without float rounding; other hosts call host cosine. */
void v4f_lpcV_window_hann(float *window, const int32_t L) {
    for (int32_t n = 0; n < L; ++n)
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
        window[n] = audio_math_hann_bounded_f64_to_f32_soft(((double)n * AUDIO_MATH_TWO_PI_F64) /
                                                            (double)(L - 1));
#else
        window[n] =
            (float)(0.5 -
                    audio_math_cos(((double)n * AUDIO_MATH_TWO_PI_F64) / (double)(L - 1)) * 0.5);
#endif
}

/* Generate cosine-tapered Tukey windows, falling back to rectangular/Hann for out-of-range parameters. */
void v4f_lpcV_Wintu(float *window, const int32_t L, const float p) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* NaN ratios select a rectangular window, preventing nonfinite segment-length conversion. */
    if (audio_math_is_nan_f32(p)) {
        v4f_lpcV_window_rectangle(window, L);
        return;
    }
#endif
    if (p <= 0.0f) {
        v4f_lpcV_window_rectangle(window, L);
        return;
    }
    if (p >= 1.0f) {
        v4f_lpcV_window_hann(window, L);
        return;
    }
    v4f_lpcV_window_rectangle(window, L);
    /* With valid length and 0 < p < 1, taper length is below 2^30 and its integer part fits.
     * Win32 kernel builds retain x87 product precision; length first converts and rounds to float. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    int32_t Np = audio_math_truncate_i32_f64((double)p * 0.5 * (double)(float)L);
#else
    int32_t Np = audio_math_truncate_i32_f32((p * 0.5f) * (float)L);
#endif
    if (Np <= 1)
        return;
    if (Np == LHDC_V4_LPC_TUKEY_TAPER_SAMPLES) {
        v4_copy_lpc_tukey_taper(window, window + L - Np);
        return;
    }
    for (int32_t n = 0; n < Np; ++n) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
        window[n] = audio_math_hann_bounded_f64_to_f32_soft(((double)n * AUDIO_MATH_PI_F64) /
                                                            (double)(Np - 1));
        window[L - Np + n] = audio_math_hann_bounded_f64_to_f32_soft(
            ((double)(Np - 1 + n) * AUDIO_MATH_PI_F64) / (double)(Np - 1));
#else
        window[n] =
            (float)(0.5 - audio_math_cos(((double)n * AUDIO_MATH_PI_F64) / (double)(Np - 1)) * 0.5);
        window[L - Np + n] =
            (float)(0.5 -
                    audio_math_cos(((double)(Np - 1 + n) * AUDIO_MATH_PI_F64) / (double)(Np - 1)) *
                        0.5);
#endif
    }
}

/* Compute LPC coefficients/errors per order with Levinson-Durbin recursion, stopping early at zero error. */
void v4f_lpcV_Calpcor(const float *autoc, uint32_t *max_order, LhdcV4LpcWorkspace *workspace) {
    float lpc[8];
    float err = autoc[0];
    for (uint32_t i = 0; i < *max_order; ++i) {
        float r = -autoc[i + 1];
        for (uint32_t j = 0; j < i; ++j)
            r = r - lpc[j] * autoc[i - j];
        r = r / err;
        lpc[i] = r;
        for (uint32_t j = 0; j < i / 2; ++j) {
            float tmp = lpc[j];
            lpc[j] = tmp + r * lpc[i - 1 - j];
            lpc[i - 1 - j] = r * tmp + lpc[i - 1 - j];
        }
        if (i & 1)
            lpc[i / 2] = lpc[i / 2] + r * lpc[i / 2];
        for (uint32_t j = 0; j <= i; ++j)
            workspace->coefficients[i][j] = -lpc[j];
        /* 1.0 is double; r*r rounds to float before promotion for the error update. */
        err = (float)((1.0 - (double)(r * r)) * (double)err);
        workspace->errors[i] = err;
        if (err == 0.0f) {
            *max_order = i + 1;
            return;
        }
    }
}

/* Select the LPC order with minimum estimated residual-bit and coefficient cost. */
uint32_t v4f_lpcV_Calpco(const float *lpc_error, uint32_t max_order, uint32_t total_samples,
                         uint32_t overhead_bits_per_order) {
    float best_bits = AUDIO_MATH_POW2_32_F32;
    uint32_t best_index = 0;
    float error_scale = 0.5f / (float)total_samples;
    uint32_t overhead = overhead_bits_per_order;
    for (uint32_t indx = 0; indx < max_order; ++indx) {
        float bits =
            v4f_lpcV_CabRer(lpc_error[indx], error_scale) * (float)(total_samples - indx - 1) +
            (float)overhead;

        if (bits < best_bits) {
            best_bits = bits;
            best_index = indx;
        }
        overhead += overhead_bits_per_order;
    }
    return best_index + 1;
}

/* Select quantization shift from maximum coefficient, apply error-feedback rounding, and bound signed quantization. */
int32_t v4f_lpcV_Quco(const float *lp_coeff, uint32_t order, uint32_t precision, int32_t *qlp_coeff,
                      int32_t *shift) {
    float cmax = 0.0f;
    for (uint32_t i = 0; i < order; ++i) {
        float magnitude = audio_math_abs_f32(lp_coeff[i]);
        if (magnitude > cmax)
            cmax = magnitude;
    }
    if (!(cmax > 0.0f))
        return LHDC_V4_LPC_QUANT_NO_NONZERO_COEFFICIENT;
    int32_t exponent = audio_math_frexp_exponent_f32(cmax);
    int32_t quant_shift = (int32_t)(precision - 1u - (uint32_t)exponent);
    *shift = quant_shift;
    if (quant_shift < -16)
        return LHDC_V4_LPC_QUANT_SHIFT_TOO_SMALL;
    if (quant_shift > 15) {
        quant_shift = 15;
        *shift = 15;
    }
    int32_t qlimit = (int32_t)(1u << ((precision - 1u) & 31));
    float error = 0.0f;
    for (uint32_t i = 0; i < order; ++i) {
        float scaled = quant_shift < 0 ? lp_coeff[i] / (float)(1u << ((uint32_t)-quant_shift & 31))
                                       : lp_coeff[i] * (float)(1u << ((uint32_t)*shift & 31));
        error = error + scaled;
        int32_t q = (int32_t)v4f_lround(error);
        if (q < -qlimit)
            q = -qlimit;
        if (q >= qlimit)
            q = qlimit - 1;
        qlp_coeff[i] = q;
        error = error - (float)q;
    }
    if (quant_shift < 0)
        *shift = 0;
    return LHDC_V4_LPC_QUANT_OK;
}

/* Accumulate autocorrelations for orders zero through eight in sample order, writing all nine outputs only at the end.
 * Retain this function boundary so the hot loop uses sample pointers instead of rebuilding large instance offsets.
 * Local accumulators can remain in registers; input and output need not be disjoint. */
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
__attribute__((noinline))
#endif
    static void calculate_lpc_autocorrelation(const float *samples, uint32_t count, float *result) {
    float sums[LHDC_V4_LPC_MAX_ORDER + 1] = {0};
    for (uint32_t sample = 0; sample < count; ++sample)
        for (uint32_t lag = 0; lag <= LHDC_V4_LPC_MAX_ORDER && sample + lag < count; ++lag)
            sums[lag] = sums[lag] + samples[sample] * samples[sample + lag];
    memcpy(result, sums, sizeof sums);
}

/* Apply window/autocorrelation, select/quantize LPC, generate residuals, and estimate coding bits. */
uint32_t v4f_lpcV_Cares(lhdc_Encoder_s *encb, int32_t *dat, int32_t *rd,
                        unsigned long long *min_total, int32_t *num_unencoded,
                        LhdcV4LpcResult *lpc) {
    LhdcV4LpcWorkspace *workspace = &encb->lpc_workspace;
    float *windowed_samples = workspace->windowed_samples;
    float autocorrelation[LHDC_V4_LPC_MAX_ORDER + 1];
    float(*coefficients)[LHDC_V4_LPC_MAX_ORDER] = workspace->coefficients;
    float *errors = workspace->errors;
    uint32_t ns = (uint32_t)encb->block_size, nMaxLpcVOrder = LHDC_V4_LPC_MAX_ORDER;
    for (uint32_t i = 0; i < ns; ++i)
        windowed_samples[i] = encb->pfLpcVWindow[i] * (float)dat[i];
    calculate_lpc_autocorrelation(windowed_samples, ns, autocorrelation);
    v4f_lpcV_Calpcor(autocorrelation, &nMaxLpcVOrder, workspace);
    uint32_t order =
        v4f_lpcV_Calpco(errors, nMaxLpcVOrder, ns, (uint32_t)encb->bits_per_sample + 15);
    lpc->order = order;
    int32_t ret = v4f_lpcV_Quco(coefficients[order - 1], order, 15, lpc->coefficients,
                                &lpc->quantization_shift);
    if (ret) {
        /* V4-B002: quantization failure writes an unselectable sentinel instead of an uninitialized total. */
        /* Failed results cannot enter residual selection, avoiding reads of random stack contents. */
        *min_total = UINT64_MAX;
        *num_unencoded = (int32_t)order;
        return LHDC_V4_LPC_EVALUATION_FAILED_BITS;
    }
    uint32_t residual_samples = ns - order;
    v4f_lpcV_Carcow(dat + order, residual_samples, lpc->coefficients, order,
                    lpc->quantization_shift, rd + order);
    uint64_t sum = 0;
    for (uint32_t i = order; i < ns; ++i) {
        uint32_t sample = (uint32_t)rd[i];
        uint32_t magnitude = rd[i] < 0 ? 0u - sample : sample;
        sum += magnitude;
    }
    uint32_t sample_bits = 32 - (uint32_t)v4_lpc_leading_zeros32(ns);
    if ((uint32_t)encb->bits_per_sample + 4 < sample_bits)
        sum = (uint32_t)sum;
    *min_total = sum;
    *num_unencoded = (int32_t)order;
    return residual_samples * 9 - (residual_samples >> 1) +
           ((uint32_t)encb->bits_per_sample + 15) * order + (uint32_t)(sum >> 7) + 8;
}

/* Generate prediction residuals using fixed-point LPC coefficients. */
void v4_lpcV_Carcow(const int32_t *data, uint32_t data_len, const int32_t *qlp_coeff,
                    uint32_t order, int32_t lp_quantization, int32_t *residual) {
    /* data[-order..-1] is valid predictor history. Native supports orders 1..8. */
    if (order < 1 || order > 8)
        return;
    for (uint32_t i = 0; i < data_len; ++i) {
        uint64_t sum = 0;
        for (uint32_t k = 0; k < order; ++k)
            sum += (uint64_t)((int64_t)data[(ptrdiff_t)i - (ptrdiff_t)k - 1] * qlp_coeff[k]);
        /* Codec domain: 0<=shift<=31. Unsigned accumulation specifies wrapping. */
        int64_t prediction =
            audio_math_shift_right_bounded_i64((int64_t)sum, (unsigned)lp_quantization);
        residual[i] = (int32_t)((uint32_t)data[i] - (uint32_t)prediction);
    }
}
