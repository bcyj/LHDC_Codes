#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"
#include "../../core/include/ar_fft_backend.h"
#include "../../core/include/ar_workspace_views.h"
#include "../hrir/upconvolution.h"
#include <string.h>

#if (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)) && \
    !defined(__arm__) && !defined(__aarch64__)
#define LHDC_AR_DUAL_ACCUMULATION 1
#else
#define LHDC_AR_DUAL_ACCUMULATION 0
#endif

/* Test whether two floating-point buffer address ranges overlap. */
static int32_t LHDC_PRIVATE(complex_buffers_overlap)(const float *first_buffer,
                                                     uint32_t first_count,
                                                     const float *second_buffer,
                                                     uint32_t second_count) {
    return (uintptr_t)first_buffer < (uintptr_t)(second_buffer + second_count) &&
           (uintptr_t)second_buffer < (uintptr_t)(first_buffer + first_count);
}

void LHDC_FN(cmplx_multi)(cc_buf_list *list, const float *coef, float *sum);

/* Multiply complex values per frequency bin and accumulate left/right spectral output. */
void LHDC_FN(cmplx_multi)(cc_buf_list *input_buf_list, const float *coef, float *sum) {
    cc_buf *node = input_buf_list->head_buf;
    for (uint32_t part = 0; part < input_buf_list->buf_count; ++part, node = node->next_buf) {
        float *input_spectrum = node->data;
        const float *c = coef + part * LHDC_CONST(AR_SPECTRUM_FLOATS);
        int32_t alias =
            LHDC_PRIVATE(complex_buffers_overlap)(sum, LHDC_CONST(AR_HALF_SPECTRUM_FLOATS),
                                                  input_spectrum,
                                                  LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)) ||
            LHDC_PRIVATE(complex_buffers_overlap)(sum, LHDC_CONST(AR_HALF_SPECTRUM_FLOATS), c,
                                                  LHDC_CONST(AR_HALF_SPECTRUM_FLOATS));
        if (!alias) {
            LHDC_PRIVATE(accumulate_complex_floats)
            (sum, input_spectrum, c, LHDC_CONST(AR_HALF_SPECTRUM_FLOATS));
        } else {
            for (int32_t j = 0; j < LHDC_CONST(AR_HALF_SPECTRUM_FLOATS); j += 2) {
                float input_real = input_spectrum[j], input_imag = input_spectrum[j + 1],
                      coef_real = c[j], coef_imag = c[j + 1];
                float sum_real = sum[j], sum_imag = sum[j + 1];
                sum[j] = sum_real + (input_real * coef_real - input_imag * coef_imag);
#if LHDC_AR_ROUNDING_PROFILE == 64
                sum[j + 1] = (sum_imag + input_imag * coef_real) + input_real * coef_imag;
#else
                sum[j + 1] = (input_real * coef_imag + input_imag * coef_real) + sum_imag;
#endif
            }
        }
        /* The 513-float packed half-spectrum stores only the Nyquist real component; its imaginary
         * component is always zero. Reading index 513 at j=0 would cross the current partition and
         * reinterpret pointer bits from the adjacent directory as float. Handle j=0 before the loop
         * to avoid a branch in every inner iteration. */
        {
            float input_real = input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  input_imag =
                      input_buf_list->data_size == LHDC_CONST(AR_PARTITION_DATA_FLOATS)
                          ? 0.0f
                          : input_spectrum[LHDC_CONST(AR_MIRROR_IMAG_START)],
                  coef_real = c[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  coef_imag = c[LHDC_CONST(AR_MIRROR_IMAG_START)];
            float sum_real = sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  sum_imag = sum[LHDC_CONST(AR_MIRROR_IMAG_START)];
            sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)] =
                sum_real + (input_real * coef_real + input_imag * coef_imag);
            sum[LHDC_CONST(AR_MIRROR_IMAG_START)] =
                (input_real * coef_imag - input_imag * coef_real) + sum_imag;
        }
        int32_t j = 2;
#if defined(LHDC_AR_COMPLEX_USE_SSE2)
        const __m128 neg_odd_mask = _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f);
        for (; j <= LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 8; j += 8) {
            __m128 c0 = _mm_loadu_ps(&c[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0 = _mm_loadu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 raw_in0 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 2 - j]);
            __m128 a0 = _mm_shuffle_ps(raw_in0, raw_in0, _MM_SHUFFLE(1, 0, 3, 2));

            __m128 c1 = _mm_loadu_ps(&c[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);
            __m128 s1 = _mm_loadu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);
            __m128 raw_in1 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 6 - j]);
            __m128 a1 = _mm_shuffle_ps(raw_in1, raw_in1, _MM_SHUFFLE(1, 0, 3, 2));

            __m128 c_rr0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 a_flip0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 3, 0, 1));

            __m128 c_rr1 = _mm_shuffle_ps(c1, c1, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii1 = _mm_shuffle_ps(c1, c1, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 a_flip1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(2, 3, 0, 1));

            __m128 prod1_0 = _mm_mul_ps(a0, c_rr0);
            __m128 prod2_0 = _mm_mul_ps(a_flip0, c_ii0);
            __m128 prod1_1 = _mm_mul_ps(a1, c_rr1);
            __m128 prod2_1 = _mm_mul_ps(a_flip1, c_ii1);

            __m128 prod1_signed0 = _mm_xor_ps(prod1_0, neg_odd_mask);
            __m128 prod1_signed1 = _mm_xor_ps(prod1_1, neg_odd_mask);

            __m128 term0 = _mm_add_ps(prod2_0, prod1_signed0);
            __m128 term1 = _mm_add_ps(prod2_1, prod1_signed1);

            _mm_storeu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0, term0));
            _mm_storeu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4], _mm_add_ps(s1, term1));
        }
        for (; j <= LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 4; j += 4) {
            __m128 c0 = _mm_loadu_ps(&c[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0 = _mm_loadu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 raw_in0 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 2 - j]);
            __m128 a0 = _mm_shuffle_ps(raw_in0, raw_in0, _MM_SHUFFLE(1, 0, 3, 2));

            __m128 c_rr0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 a_flip0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 3, 0, 1));

            __m128 prod1_0 = _mm_mul_ps(a0, c_rr0);
            __m128 prod2_0 = _mm_mul_ps(a_flip0, c_ii0);
            __m128 prod1_signed0 = _mm_xor_ps(prod1_0, neg_odd_mask);
            __m128 term0 = _mm_add_ps(prod2_0, prod1_signed0);

            _mm_storeu_ps(&sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0, term0));
        }
#endif
        for (; j < LHDC_CONST(AR_HALF_SPECTRUM_FLOATS); j += 2) {
            float input_real = input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - j],
                  input_imag = input_spectrum[LHDC_CONST(AR_MIRROR_IMAG_START) - j],
                  coef_real = c[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  coef_imag = c[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            float sum_real = sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  sum_imag = sum[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            sum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j] =
                sum_real + (input_real * coef_real + input_imag * coef_imag);
            sum[LHDC_CONST(AR_MIRROR_IMAG_START) + j] =
                (input_real * coef_imag - input_imag * coef_real) + sum_imag;
        }
    }
}

/* Multiply each partition spectrum by the left/right impulse responses per frequency bin and
 * accumulate into the corresponding frequency-domain outputs. */
#if LHDC_AR_DUAL_ACCUMULATION
static void LHDC_FN(cmplx_multi_dual)(cc_buf_list *input_buf_list, const float *coef_l,
                                      const float *coef_r, float *sum_l, float *sum_r) {
    cc_buf *node = input_buf_list->head_buf;
    for (uint32_t part = 0; part < input_buf_list->buf_count; ++part, node = node->next_buf) {
        float *input_spectrum = node->data;
        const float *c_l = coef_l + part * LHDC_CONST(AR_SPECTRUM_FLOATS);
        const float *c_r = coef_r + part * LHDC_CONST(AR_SPECTRUM_FLOATS);
        LHDC_PRIVATE(accumulate_complex_floats_dual)
        (sum_l, sum_r, input_spectrum, c_l, c_r, LHDC_CONST(AR_HALF_SPECTRUM_FLOATS));

        /* The 513-float packed half-spectrum stores only the Nyquist real component; imaginary is zero. */
        {
            float input_real = input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  input_imag =
                      input_buf_list->data_size == LHDC_CONST(AR_PARTITION_DATA_FLOATS)
                          ? 0.0f
                          : input_spectrum[LHDC_CONST(AR_MIRROR_IMAG_START)];
            float c_l_real = c_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  c_l_imag = c_l[LHDC_CONST(AR_MIRROR_IMAG_START)];
            float sum_l_real = sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  sum_l_imag = sum_l[LHDC_CONST(AR_MIRROR_IMAG_START)];
            sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)] =
                sum_l_real + (input_real * c_l_real + input_imag * c_l_imag);
            sum_l[LHDC_CONST(AR_MIRROR_IMAG_START)] =
                (input_real * c_l_imag - input_imag * c_l_real) + sum_l_imag;

            float c_r_real = c_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  c_r_imag = c_r[LHDC_CONST(AR_MIRROR_IMAG_START)];
            float sum_r_real = sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)],
                  sum_r_imag = sum_r[LHDC_CONST(AR_MIRROR_IMAG_START)];
            sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS)] =
                sum_r_real + (input_real * c_r_real + input_imag * c_r_imag);
            sum_r[LHDC_CONST(AR_MIRROR_IMAG_START)] =
                (input_real * c_r_imag - input_imag * c_r_real) + sum_r_imag;
        }
        int32_t j = 2;
#if defined(LHDC_AR_COMPLEX_USE_SSE2)
        const __m128 neg_odd_mask = _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f);
        for (; j <= LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 8; j += 8) {
            __m128 raw_in0 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 2 - j]);
            __m128 a0 = _mm_shuffle_ps(raw_in0, raw_in0, _MM_SHUFFLE(1, 0, 3, 2));
            __m128 a_flip0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 3, 0, 1));

            __m128 raw_in1 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 6 - j]);
            __m128 a1 = _mm_shuffle_ps(raw_in1, raw_in1, _MM_SHUFFLE(1, 0, 3, 2));
            __m128 a_flip1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(2, 3, 0, 1));

            /* Left ear */
            __m128 c0_l = _mm_loadu_ps(&c_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0_l = _mm_loadu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 c1_l = _mm_loadu_ps(&c_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);
            __m128 s1_l = _mm_loadu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);

            __m128 c_rr0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 c_rr1_l = _mm_shuffle_ps(c1_l, c1_l, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii1_l = _mm_shuffle_ps(c1_l, c1_l, _MM_SHUFFLE(3, 3, 1, 1));

            __m128 prod1_0_l = _mm_mul_ps(a0, c_rr0_l);
            __m128 prod2_0_l = _mm_mul_ps(a_flip0, c_ii0_l);
            __m128 prod1_1_l = _mm_mul_ps(a1, c_rr1_l);
            __m128 prod2_1_l = _mm_mul_ps(a_flip1, c_ii1_l);

            __m128 term0_l = _mm_add_ps(prod2_0_l, _mm_xor_ps(prod1_0_l, neg_odd_mask));
            __m128 term1_l = _mm_add_ps(prod2_1_l, _mm_xor_ps(prod1_1_l, neg_odd_mask));

            _mm_storeu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0_l, term0_l));
            _mm_storeu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4], _mm_add_ps(s1_l, term1_l));

            /* Right ear */
            __m128 c0_r = _mm_loadu_ps(&c_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0_r = _mm_loadu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 c1_r = _mm_loadu_ps(&c_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);
            __m128 s1_r = _mm_loadu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4]);

            __m128 c_rr0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 c_rr1_r = _mm_shuffle_ps(c1_r, c1_r, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii1_r = _mm_shuffle_ps(c1_r, c1_r, _MM_SHUFFLE(3, 3, 1, 1));

            __m128 prod1_0_r = _mm_mul_ps(a0, c_rr0_r);
            __m128 prod2_0_r = _mm_mul_ps(a_flip0, c_ii0_r);
            __m128 prod1_1_r = _mm_mul_ps(a1, c_rr1_r);
            __m128 prod2_1_r = _mm_mul_ps(a_flip1, c_ii1_r);

            __m128 term0_r = _mm_add_ps(prod2_0_r, _mm_xor_ps(prod1_0_r, neg_odd_mask));
            __m128 term1_r = _mm_add_ps(prod2_1_r, _mm_xor_ps(prod1_1_r, neg_odd_mask));

            _mm_storeu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0_r, term0_r));
            _mm_storeu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j + 4], _mm_add_ps(s1_r, term1_r));
        }
        for (; j <= LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 4; j += 4) {
            __m128 raw_in0 = _mm_loadu_ps(&input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - 2 - j]);
            __m128 a0 = _mm_shuffle_ps(raw_in0, raw_in0, _MM_SHUFFLE(1, 0, 3, 2));
            __m128 a_flip0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 3, 0, 1));

            /* Left ear */
            __m128 c0_l = _mm_loadu_ps(&c_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0_l = _mm_loadu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 c_rr0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 prod1_0_l = _mm_mul_ps(a0, c_rr0_l);
            __m128 prod2_0_l = _mm_mul_ps(a_flip0, c_ii0_l);
            __m128 term0_l = _mm_add_ps(prod2_0_l, _mm_xor_ps(prod1_0_l, neg_odd_mask));
            _mm_storeu_ps(&sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0_l, term0_l));

            /* Right ear */
            __m128 c0_r = _mm_loadu_ps(&c_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 s0_r = _mm_loadu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j]);
            __m128 c_rr0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 c_ii0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 prod1_0_r = _mm_mul_ps(a0, c_rr0_r);
            __m128 prod2_0_r = _mm_mul_ps(a_flip0, c_ii0_r);
            __m128 term0_r = _mm_add_ps(prod2_0_r, _mm_xor_ps(prod1_0_r, neg_odd_mask));
            _mm_storeu_ps(&sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j], _mm_add_ps(s0_r, term0_r));
        }
#endif
        for (; j < LHDC_CONST(AR_HALF_SPECTRUM_FLOATS); j += 2) {
            float in_real = input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - j],
                  in_imag = input_spectrum[LHDC_CONST(AR_MIRROR_IMAG_START) - j];
            float cr_l = c_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  ci_l = c_l[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            float sr_l = sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  si_l = sum_l[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            sum_l[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j] =
                sr_l + (in_real * cr_l + in_imag * ci_l);
            sum_l[LHDC_CONST(AR_MIRROR_IMAG_START) + j] =
                (in_real * ci_l - in_imag * cr_l) + si_l;

            float cr_r = c_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  ci_r = c_r[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            float sr_r = sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j],
                  si_r = sum_r[LHDC_CONST(AR_MIRROR_IMAG_START) + j];
            sum_r[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + j] =
                sr_r + (in_real * cr_r + in_imag * ci_r);
            sum_r[LHDC_CONST(AR_MIRROR_IMAG_START) + j] =
                (in_real * ci_r - in_imag * cr_r) + si_r;
        }
    }
}
#endif

/* Perform partitioned frequency-domain convolution, inverse FFT, and overlap-add. */
void LHDC_FN(upconv_process)(float *w_in_frame, float *x_in_frame, float *y_in_frame,
                             float *l_out_frame, float *r_out_frame,
                             LHDCConvolutionDirectory *workspace) {
    float *data = workspace->fft_data;
    float *input[3] = {w_in_frame, x_in_frame, y_in_frame};
    /* Reverb may stage three inputs in separate regions of sum[0]. Save all input spectra
     * before clearing sum; do not move subsequent clearing/accumulation into this loop. */
    for (int32_t j = 0; j < 3; ++j) {
        LHDC_FN(fft_buffer_n_to_4n)
        (input[j], workspace->input_history[j], data, LHDC_CONST(AR_BLOCK_SAMPLES),
         LHDC_CONST(AR_SPECTRUM_FLOATS));
        lhdc_ar_fft_512(data, 0, workspace->fft_work);
        LHDC_FN(cc_buf_list_update)(workspace->channel[j].buf_list, data);
    }
    float *sum[2] = {workspace->sum[0], workspace->sum[1]};
    const float *coef[2][3] = {{lhdc_ar_abir_w_l, lhdc_ar_abir_x_l, lhdc_ar_abir_y_l},
                               {lhdc_ar_abir_w_r, lhdc_ar_abir_x_r, lhdc_ar_abir_y_r}};
    float *out[2] = {l_out_frame, r_out_frame};
    memset(sum[0], 0, LHDC_CONST(AR_SPECTRUM_BYTES));
    memset(sum[1], 0, LHDC_CONST(AR_SPECTRUM_BYTES));
    for (int32_t i = 0; i < 3; ++i) {
#if LHDC_AR_DUAL_ACCUMULATION
        LHDC_FN(cmplx_multi_dual)(workspace->channel[i].buf_list, coef[0][i], coef[1][i], sum[0],
                                  sum[1]);
#else
        LHDC_FN(cmplx_multi)(workspace->channel[i].buf_list, coef[0][i], sum[0]);
        LHDC_FN(cmplx_multi)(workspace->channel[i].buf_list, coef[1][i], sum[1]);
#endif
    }
    for (int32_t j = 0; j < 2; ++j) {
        lhdc_ar_fft_512(sum[j], 1, workspace->fft_work);
        for (int32_t i = 0; i < LHDC_CONST(AR_BLOCK_SAMPLES); ++i)
            out[j][i] = sum[j][LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + i * 2];
    }
}

#undef LHDC_AR_DUAL_ACCUMULATION

/* Build the circular list of convolution-history partitions. */
void LHDC_FN(cc_buf_list_init)(cc_buf_list *list, cc_buf *list_mem, float *data_mem, uint32_t size,
                               uint32_t count) {
    list->data_size = size;
    list->buf_count = count;
    list->list_mem = list->head_buf = list_mem;
    list->tail_buf = list_mem + count - 1;
    for (uint32_t i = 0; i < count; ++i) {
        list_mem[i].data = data_mem + i * size;
        list_mem[i].next_buf = list_mem + (i + 1) % count;
    }
}

/* Advance the circular list and store the new input spectrum. */
void LHDC_FN(cc_buf_list_update)(cc_buf_list *list, float *new_data) {
    memcpy(list->tail_buf->data, new_data, list->data_size * 4);
    cc_buf *tail = list->tail_buf;
    list->head_buf = tail;
    if (tail == list->list_mem)
        tail += list->buf_count;
    list->tail_buf = tail - 1;
}

/* Calculate or partition aligned workspace for partitioned convolution. */
void LHDC_FN(upconv_alloc)(uint8_t *allocated_mem_addr, uint32_t *mem_size) {
    uint32_t sizes[LHDC_CONST(AR_UPCONV_SLOT_COUNT)] = {
        lhdc_workspace_align8(LHDC_CONST(AR_UPCONV_SLOT_COUNT) * sizeof(void *)),
        LHDC_CONST(AR_SPECTRUM_BYTES),
        LHDC_CONST(AR_BLOCK_BYTES),
        LHDC_CONST(AR_BLOCK_BYTES),
        LHDC_CONST(AR_BLOCK_BYTES),
        lhdc_workspace_align8(sizeof(cc_buf_list)),
        LHDC_CONST(AR_CONV_PARTITIONS) * sizeof(cc_buf),
        LHDC_CONST(AR_CONV_PARTITIONS) * LHDC_CONST(AR_PARTITION_DATA_FLOATS) * 4,
        lhdc_workspace_align8(sizeof(cc_buf_list)),
        LHDC_CONST(AR_CONV_PARTITIONS) * sizeof(cc_buf),
        LHDC_CONST(AR_CONV_PARTITIONS) * LHDC_CONST(AR_PARTITION_DATA_FLOATS) * 4,
        lhdc_workspace_align8(sizeof(cc_buf_list)),
        LHDC_CONST(AR_CONV_PARTITIONS) * sizeof(cc_buf),
        LHDC_CONST(AR_CONV_PARTITIONS) * LHDC_CONST(AR_PARTITION_DATA_FLOATS) * 4,
        LHDC_CONST(AR_SPECTRUM_BYTES),
        sizeof(AudioMathFft512Work)};
    uint32_t total = LHDC_CONST(AR_SPECTRUM_BYTES);
    for (int32_t partition_index = 0; partition_index < LHDC_CONST(AR_UPCONV_SLOT_COUNT);
         ++partition_index)
        total += sizes[partition_index];
    if (!allocated_mem_addr) {
        *mem_size = total;
        return;
    }
    if (*mem_size != total)
        return;
    memset(allocated_mem_addr, 0, total);
    LHDCConvolutionDirectory *workspace = (void *)allocated_mem_addr;
    uint32_t cursor = sizes[0];
    workspace->fft_data = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[1]);
    for (int32_t ch = 0; ch < 3; ++ch)
        workspace->input_history[ch] =
            lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[2 + ch]);
    for (int32_t ch = 0; ch < 3; ++ch) {
        LHDCConvolutionChannel *channel = &workspace->channel[ch];
        channel->buf_list = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[5 + 3 * ch]);
        channel->list_mem = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[6 + 3 * ch]);
        channel->data_mem = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[7 + 3 * ch]);
    }
    workspace->sum[0] = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[14]);
    workspace->sum[1] =
        lhdc_workspace_take(allocated_mem_addr, &cursor, LHDC_CONST(AR_SPECTRUM_BYTES));
    workspace->fft_work = lhdc_workspace_take(allocated_mem_addr, &cursor, sizes[15]);
}

/* Initialize partitioned-convolution workspace and history. */
void LHDC_FN(upconv_init)(LHDCConvolutionDirectory *workspace) {
    for (int32_t ch = 0; ch < 3; ++ch) {
        LHDCConvolutionChannel *channel = &workspace->channel[ch];
        LHDC_FN(cc_buf_list_init)
        (channel->buf_list, channel->list_mem, channel->data_mem,
         LHDC_CONST(AR_PARTITION_DATA_FLOATS), LHDC_CONST(AR_CONV_PARTITIONS));
    }
}
