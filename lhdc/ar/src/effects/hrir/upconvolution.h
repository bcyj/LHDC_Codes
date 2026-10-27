#ifndef LHDC_AR_SRC_EFFECTS_HRIR_UPCONVOLUTION_H
#define LHDC_AR_SRC_EFFECTS_HRIR_UPCONVOLUTION_H
#include "lhdc_version.h"
#include <stdint.h>

#if defined(__SSE2__) ||                                                                          \
    (defined(_MSC_VER) && (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86))))
#include <emmintrin.h>
#define LHDC_AR_COMPLEX_USE_SSE2 1
#elif (defined(__ARM_NEON) || defined(__ARM_NEON__)) && !defined(__aarch64__) &&                 \
    !defined(_M_ARM64)
#include <arm_neon.h>
#define LHDC_AR_COMPLEX_USE_NEON32 1
#endif

/* Perform per-bin complex multiplication and accumulate convolution spectra. */
static inline void LHDC_PRIVATE(accumulate_complex_floats)(float *sum, const float *data,
                                                           const float *coef, int32_t count) {
#if defined(LHDC_AR_COMPLEX_USE_SSE2)
    const __m128 neg_mask = _mm_set_ps(0.0f, -0.0f, 0.0f, -0.0f);
    int32_t i = 0;
    for (; i <= count - 8; i += 8) {
        __m128 a0 = _mm_loadu_ps(&data[i]);
        __m128 c0 = _mm_loadu_ps(&coef[i]);
        __m128 s0 = _mm_loadu_ps(&sum[i]);
        __m128 a1 = _mm_loadu_ps(&data[i + 4]);
        __m128 c1 = _mm_loadu_ps(&coef[i + 4]);
        __m128 s1 = _mm_loadu_ps(&sum[i + 4]);

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

        __m128 prod2_signed0 = _mm_xor_ps(prod2_0, neg_mask);
        __m128 prod2_signed1 = _mm_xor_ps(prod2_1, neg_mask);

        __m128 cprod0 = _mm_add_ps(prod1_0, prod2_signed0);
        __m128 cprod1 = _mm_add_ps(prod1_1, prod2_signed1);

        _mm_storeu_ps(&sum[i], _mm_add_ps(cprod0, s0));
        _mm_storeu_ps(&sum[i + 4], _mm_add_ps(cprod1, s1));
    }
    for (; i <= count - 4; i += 4) {
        __m128 a = _mm_loadu_ps(&data[i]);
        __m128 c = _mm_loadu_ps(&coef[i]);
        __m128 s = _mm_loadu_ps(&sum[i]);
        __m128 c_rr = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 a_flip = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 prod1 = _mm_mul_ps(a, c_rr);
        __m128 prod2 = _mm_mul_ps(a_flip, c_ii);
        __m128 prod2_signed = _mm_xor_ps(prod2, neg_mask);
        __m128 cprod = _mm_add_ps(prod1, prod2_signed);
        __m128 res = _mm_add_ps(cprod, s);
        _mm_storeu_ps(&sum[i], res);
    }
    for (; i < count; i += 2) {
        float ar = data[i], ai = data[i + 1], cr = coef[i], ci = coef[i + 1], sr = sum[i],
              si = sum[i + 1];
        sum[i] = sr + (ar * cr - ai * ci);
        sum[i + 1] = (ai * cr + ar * ci) + si;
    }
#elif defined(LHDC_AR_COMPLEX_USE_NEON32)
    static const uint32_t mask[4] = {0x80000000u, 0, 0x80000000u, 0};
    uint32x4_t neg_mask = vld1q_u32(mask);
    int32_t i = 0;
    for (; i <= count - 4; i += 4) {
        float32x4_t a = vld1q_f32(&data[i]);
        float32x4_t c = vld1q_f32(&coef[i]);
        float32x4_t s = vld1q_f32(&sum[i]);
        float32x4_t a_flip = vrev64q_f32(a);
        float32x2_t c_low = vget_low_f32(c);
        float32x2_t c_high = vget_high_f32(c);
        float32x4_t c_rr = vcombine_f32(vdup_lane_f32(c_low, 0), vdup_lane_f32(c_high, 0));
        float32x4_t c_ii = vcombine_f32(vdup_lane_f32(c_low, 1), vdup_lane_f32(c_high, 1));
        float32x4_t prod1 = vmulq_f32(a, c_rr);
        float32x4_t prod2 = vmulq_f32(a_flip, c_ii);
        float32x4_t prod2_signed =
            vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(prod2), neg_mask));
        float32x4_t cprod = vaddq_f32(prod1, prod2_signed);
        float32x4_t res = vaddq_f32(cprod, s);
        vst1q_f32(&sum[i], res);
    }
    for (; i < count; i += 2) {
        float ar = data[i], ai = data[i + 1], cr = coef[i], ci = coef[i + 1], sr = sum[i],
              si = sum[i + 1];
        sum[i] = sr + (ar * cr - ai * ci);
        sum[i + 1] = (ai * cr + ar * ci) + si;
    }
#else
    for (int32_t i = 0; i < count; i += 2) {
        float ar = data[i], ai = data[i + 1], cr = coef[i], ci = coef[i + 1], sr = sum[i],
              si = sum[i + 1];
        sum[i] = sr + (ar * cr - ai * ci);
        sum[i + 1] = (ai * cr + ar * ci) + si;
    }
#endif
}

/* Perform paired complex multiplication per frequency bin for left and right ears, loading and
 * shuffling each input complex value only once. */
static inline void LHDC_PRIVATE(accumulate_complex_floats_dual)(float *sum_l, float *sum_r,
                                                                const float *data,
                                                                const float *coef_l,
                                                                const float *coef_r,
                                                                int32_t count) {
#if defined(LHDC_AR_COMPLEX_USE_SSE2)
    const __m128 neg_mask = _mm_set_ps(0.0f, -0.0f, 0.0f, -0.0f);
    int32_t i = 0;
    for (; i <= count - 8; i += 8) {
        __m128 a0 = _mm_loadu_ps(&data[i]);
        __m128 a1 = _mm_loadu_ps(&data[i + 4]);
        __m128 a_flip0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 a_flip1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(2, 3, 0, 1));

        /* Left ear */
        __m128 c0_l = _mm_loadu_ps(&coef_l[i]);
        __m128 s0_l = _mm_loadu_ps(&sum_l[i]);
        __m128 c1_l = _mm_loadu_ps(&coef_l[i + 4]);
        __m128 s1_l = _mm_loadu_ps(&sum_l[i + 4]);

        __m128 c_rr0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii0_l = _mm_shuffle_ps(c0_l, c0_l, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 c_rr1_l = _mm_shuffle_ps(c1_l, c1_l, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii1_l = _mm_shuffle_ps(c1_l, c1_l, _MM_SHUFFLE(3, 3, 1, 1));

        __m128 prod1_0_l = _mm_mul_ps(a0, c_rr0_l);
        __m128 prod2_0_l = _mm_mul_ps(a_flip0, c_ii0_l);
        __m128 prod1_1_l = _mm_mul_ps(a1, c_rr1_l);
        __m128 prod2_1_l = _mm_mul_ps(a_flip1, c_ii1_l);

        __m128 cprod0_l = _mm_add_ps(prod1_0_l, _mm_xor_ps(prod2_0_l, neg_mask));
        __m128 cprod1_l = _mm_add_ps(prod1_1_l, _mm_xor_ps(prod2_1_l, neg_mask));

        _mm_storeu_ps(&sum_l[i], _mm_add_ps(cprod0_l, s0_l));
        _mm_storeu_ps(&sum_l[i + 4], _mm_add_ps(cprod1_l, s1_l));

        /* Right ear */
        __m128 c0_r = _mm_loadu_ps(&coef_r[i]);
        __m128 s0_r = _mm_loadu_ps(&sum_r[i]);
        __m128 c1_r = _mm_loadu_ps(&coef_r[i + 4]);
        __m128 s1_r = _mm_loadu_ps(&sum_r[i + 4]);

        __m128 c_rr0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii0_r = _mm_shuffle_ps(c0_r, c0_r, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 c_rr1_r = _mm_shuffle_ps(c1_r, c1_r, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii1_r = _mm_shuffle_ps(c1_r, c1_r, _MM_SHUFFLE(3, 3, 1, 1));

        __m128 prod1_0_r = _mm_mul_ps(a0, c_rr0_r);
        __m128 prod2_0_r = _mm_mul_ps(a_flip0, c_ii0_r);
        __m128 prod1_1_r = _mm_mul_ps(a1, c_rr1_r);
        __m128 prod2_1_r = _mm_mul_ps(a_flip1, c_ii1_r);

        __m128 cprod0_r = _mm_add_ps(prod1_0_r, _mm_xor_ps(prod2_0_r, neg_mask));
        __m128 cprod1_r = _mm_add_ps(prod1_1_r, _mm_xor_ps(prod2_1_r, neg_mask));

        _mm_storeu_ps(&sum_r[i], _mm_add_ps(cprod0_r, s0_r));
        _mm_storeu_ps(&sum_r[i + 4], _mm_add_ps(cprod1_r, s1_r));
    }
    for (; i <= count - 4; i += 4) {
        __m128 a = _mm_loadu_ps(&data[i]);
        __m128 a_flip = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));

        /* Left ear */
        __m128 c_l = _mm_loadu_ps(&coef_l[i]);
        __m128 s_l = _mm_loadu_ps(&sum_l[i]);
        __m128 c_rr_l = _mm_shuffle_ps(c_l, c_l, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii_l = _mm_shuffle_ps(c_l, c_l, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 prod1_l = _mm_mul_ps(a, c_rr_l);
        __m128 prod2_l = _mm_mul_ps(a_flip, c_ii_l);
        __m128 cprod_l = _mm_add_ps(prod1_l, _mm_xor_ps(prod2_l, neg_mask));
        _mm_storeu_ps(&sum_l[i], _mm_add_ps(cprod_l, s_l));

        /* Right ear */
        __m128 c_r = _mm_loadu_ps(&coef_r[i]);
        __m128 s_r = _mm_loadu_ps(&sum_r[i]);
        __m128 c_rr_r = _mm_shuffle_ps(c_r, c_r, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 c_ii_r = _mm_shuffle_ps(c_r, c_r, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 prod1_r = _mm_mul_ps(a, c_rr_r);
        __m128 prod2_r = _mm_mul_ps(a_flip, c_ii_r);
        __m128 cprod_r = _mm_add_ps(prod1_r, _mm_xor_ps(prod2_r, neg_mask));
        _mm_storeu_ps(&sum_r[i], _mm_add_ps(cprod_r, s_r));
    }
    for (; i < count; i += 2) {
        float ar = data[i], ai = data[i + 1];
        float cr_l = coef_l[i], ci_l = coef_l[i + 1], sr_l = sum_l[i], si_l = sum_l[i + 1];
        sum_l[i] = sr_l + (ar * cr_l - ai * ci_l);
        sum_l[i + 1] = (ai * cr_l + ar * ci_l) + si_l;

        float cr_r = coef_r[i], ci_r = coef_r[i + 1], sr_r = sum_r[i], si_r = sum_r[i + 1];
        sum_r[i] = sr_r + (ar * cr_r - ai * ci_r);
        sum_r[i + 1] = (ai * cr_r + ar * ci_r) + si_r;
    }
#elif defined(LHDC_AR_COMPLEX_USE_NEON32)
    LHDC_PRIVATE(accumulate_complex_floats)(sum_l, data, coef_l, count);
    LHDC_PRIVATE(accumulate_complex_floats)(sum_r, data, coef_r, count);
#else
    for (int32_t i = 0; i < count; i += 2) {
        float ar = data[i], ai = data[i + 1];
        float cr_l = coef_l[i], ci_l = coef_l[i + 1], sr_l = sum_l[i], si_l = sum_l[i + 1];
        sum_l[i] = sr_l + (ar * cr_l - ai * ci_l);
        sum_l[i + 1] = (ai * cr_l + ar * ci_l) + si_l;

        float cr_r = coef_r[i], ci_r = coef_r[i + 1], sr_r = sum_r[i], si_r = sum_r[i + 1];
        sum_r[i] = sr_r + (ar * cr_r - ai * ci_r);
        sum_r[i + 1] = (ai * cr_r + ar * ci_r) + si_r;
    }
#endif
}

static inline void LHDC_PRIVATE(accumulate_complex_spectrum)(float *sum, const float *data,
                                                             const float *coef) {
    LHDC_PRIVATE(accumulate_complex_floats)(sum, data, coef, LHDC_CONST(AR_SPECTRUM_FLOATS));
}
#undef LHDC_AR_COMPLEX_USE_SSE2
#undef LHDC_AR_COMPLEX_USE_NEON32
#endif
