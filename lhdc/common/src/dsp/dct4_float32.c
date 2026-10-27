#include <audio_math/float_sqrt.h>
#include "../../include/dsp/dct4_float32.h"
#include <audio_math/bits.h>
#include <audio_math/constants.h>
#include <audio_math/math.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#endif
#include "../../include/dsp/fft_float32.h"
#include "dct4_float32_tables.h"
#include <math.h>
#include <string.h>

#if defined(__SSE2__) ||                                                                          \
    (defined(_MSC_VER) && (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86))))
#include <emmintrin.h>
#define LHDC_DCT4_USE_SSE2 1
#else
#define LHDC_DCT4_USE_SSE2 0
#endif

/* Fixed LLAC lengths share verified DCT rotation tables; other lengths use generic initialization. */

/* Validate even length and capacity limits; return rotation-coefficient float count, or zero for invalid lengths. */
size_t lhdc_dct4_f32_rotation_floats(int32_t length) {
    if (length < 2 || (length & 1) || (size_t)length > SIZE_MAX / (3 * sizeof(float)))
        return 0;
    return (size_t)length * 3;
}

/* Build an orthonormal DCT-IV plan with caller-owned rotation tables, read-only during frame processing. */
int lhdc_dct4_f32_init(LhdcDct4F32Plan *plan, int32_t length, kiss_fft_cpx *rotation_storage,
                       size_t storage_floats) {
    size_t required = lhdc_dct4_f32_rotation_floats(length);
    if (!plan || !required || !rotation_storage)
        return LHDC_DCT4_INVALID_ARGUMENT;
    if (storage_floats < required)
        return LHDC_DCT4_BUFFER_TOO_SMALL;
    int32_t fft_length = length / 2;
    plan->length = length;
    plan->normalization = 1.0f / audio_math_sqrt_f32((float)fft_length);
    plan->pre_rotation = rotation_storage;
    plan->post_rotation = rotation_storage + length / 2;
    plan->fft_twiddles = rotation_storage + length;
    lhdc_kiss_fft_factor(fft_length, (int32_t)audio_math_sqrt_floor_u32((uint32_t)fft_length),
                         plan->factors);
    plan->maximum_radix = 1;
    for (unsigned i = 0; i < 64; i += 2) {
        if (plan->factors[i] > plan->maximum_radix)
            plan->maximum_radix = plan->factors[i];
        if (plan->factors[i + 1] == 1)
            break;
    }
    /* All platforms share fixed-length coefficients to prevent libm approximation differences in decoded PCM. */
    const kiss_fft_cpx *fixed_rotations = lhdc_dct4_f32_static_rotations(length);
    if (fixed_rotations) {
        memcpy(rotation_storage, fixed_rotations, (size_t)length * sizeof *rotation_storage);
    } else {
        /* Preserve single-precision phase definitions; explicitly evaluate nonfixed lengths in double before rounding. */
        const float negative_pi = -AUDIO_MATH_PI_F32;
        for (int32_t k = 0; k < fft_length; ++k) {
            float pre_phase = (((float)k + 0.25f) * negative_pi) / (float)length;
            float post_phase = ((float)k * negative_pi) / (float)length;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            /* Both phases are first rounded to float and have magnitude below 2; the software backend produces bit-identical coefficients. */
            audio_math_sincos_bounded_f32_soft(pre_phase, &rotation_storage[k].i,
                                               &rotation_storage[k].r);
            audio_math_sincos_bounded_f32_soft(post_phase, &rotation_storage[fft_length + k].i,
                                               &rotation_storage[fft_length + k].r);
#else
            rotation_storage[k].r = (float)audio_math_host_cos((double)pre_phase);
            rotation_storage[k].i = (float)audio_math_host_sin((double)pre_phase);
            rotation_storage[fft_length + k].r = (float)audio_math_host_cos((double)post_phase);
            rotation_storage[fft_length + k].i = (float)audio_math_host_sin((double)post_phase);
#endif
        }
    }
    lhdc_kiss_fft_twiddles(fft_length, 0, rotation_storage + length);
    return LHDC_DCT4_OK;
}

/* Provide sufficient generic-radix scratch without per-frame heap allocation. */
size_t lhdc_dct4_f32_radix_scratch_floats(const LhdcDct4F32Plan *plan) {
    if (!plan || !lhdc_dct4_f32_rotation_floats(plan->length) || plan->maximum_radix < 1 ||
        plan->maximum_radix > plan->length / 2)
        return 0;
    return (size_t)plan->maximum_radix * 2;
}

/* Execute DCT-IV through a half-length forward FFT; complete pre-rotation first to allow input/output aliasing. */
int lhdc_dct4_f32_transform(const LhdcDct4F32Plan *plan, const float *input, float *output,
                            const LhdcDct4F32Work *work) {
    size_t radix_floats = lhdc_dct4_f32_radix_scratch_floats(plan);
    if (!radix_floats)
        return LHDC_DCT4_INVALID_ARGUMENT;
    const LhdcDct4F32Kernel kernel = {plan->length,
                                      plan->factors,
                                      plan->pre_rotation,
                                      plan->post_rotation,
                                      plan->fft_twiddles,
                                      1,
                                      LHDC_DCT4_SCALE_AFTER_ROTATION,
                                      plan->normalization};
    return lhdc_dct4_f32_execute(&kernel, input, output, work);
}

/* Validate factors and scratch capacity, then use instance tables for common pre-rotation, FFT, and post-rotation. */
int lhdc_dct4_f32_execute(const LhdcDct4F32Kernel *kernel, const float *input, float *output,
                          const LhdcDct4F32Work *work) {
    if (!kernel || !lhdc_dct4_f32_rotation_floats(kernel->length) || !kernel->factors || !input ||
        !output || !work || !kernel->pre_rotation || !kernel->post_rotation ||
        !kernel->fft_twiddles || !work->packed_input || !work->spectrum ||
        (kernel->scaling != LHDC_DCT4_SCALE_AFTER_ROTATION &&
         kernel->scaling != LHDC_DCT4_SCALE_IN_ROTATION_TABLE))
        return LHDC_DCT4_INVALID_ARGUMENT;
    const int32_t length = kernel->length, fft_length = length / 2;
    int32_t remaining = fft_length, maximum_radix = 1;
    for (unsigned i = 0;; i += 2) {
        if (i >= 64)
            return LHDC_DCT4_INVALID_ARGUMENT;
        int32_t radix = kernel->factors[i];
        if (radix < 1 || radix > remaining || remaining % radix || (radix == 1 && remaining != 1))
            return LHDC_DCT4_INVALID_ARGUMENT;
        remaining /= radix;
        if (kernel->factors[i + 1] != remaining)
            return LHDC_DCT4_INVALID_ARGUMENT;
        if (radix > maximum_radix)
            maximum_radix = radix;
        if (remaining == 1)
            break;
    }
    if (maximum_radix > 5 && !kernel->allow_generic_radix)
        return LHDC_DCT4_INVALID_ARGUMENT;
    size_t radix_floats = kernel->allow_generic_radix ? (size_t)maximum_radix * 2 : 0;
    if (radix_floats && !work->radix_scratch)
        return LHDC_DCT4_INVALID_ARGUMENT;
    if (work->packed_input_floats < (size_t)length || work->spectrum_floats < (size_t)length ||
        work->radix_scratch_floats < radix_floats)
        return LHDC_DCT4_BUFFER_TOO_SMALL;
    /* 1. Pack even samples forward and odd samples backward, then apply pre-rotation. */
    const kiss_fft_cpx *pre = kernel->pre_rotation;
    int32_t k = 0;
#if LHDC_DCT4_USE_SSE2
    const __m128 sign_mask_odd = _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f);
    for (; k + 2 <= fft_length; k += 2) {
        float even0 = input[2 * k];
        float rev0 = input[length - 1 - 2 * k];
        float even1 = input[2 * k + 2];
        float rev1 = input[length - 3 - 2 * k];
        __m128 X = _mm_setr_ps(even0, rev0, even1, rev1);
        __m128 W = _mm_loadu_ps((const float *)&pre[k]);

        __m128 X_rr = _mm_shuffle_ps(X, X, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 X_ii = _mm_shuffle_ps(X, X, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 W_ir = _mm_shuffle_ps(W, W, _MM_SHUFFLE(2, 3, 0, 1));

        __m128 P1 = _mm_mul_ps(X_rr, W);
        __m128 P2 = _mm_mul_ps(X_ii, W_ir);

        __m128 P2_signed = _mm_xor_ps(P2, sign_mask_odd);
        __m128 packed = _mm_sub_ps(P1, P2_signed);
        _mm_storeu_ps((float *)&work->packed_input[k], packed);
    }
#endif
    for (; k < fft_length; ++k) {
        float even = input[2 * k], reversed_odd = input[length - 1 - 2 * k];
        float real = pre[k].r, imag = pre[k].i;
        work->packed_input[k].r = even * real - reversed_odd * imag;
        work->packed_input[k].i = reversed_odd * real + even * imag;
    }
    /* 2. FFT is unnormalized; generic-radix scratch belongs to this call. */
    LhdcKissFftPlan fft = {fft_length, 0, kernel->fft_twiddles, kernel->allow_generic_radix,
                           work->radix_scratch};
    lhdc_kiss_fft_work(work->spectrum, work->packed_input, 1, 1, kernel->factors, &fft);
    /* 3. V5's post-table includes scaling; apply post-rotation and omit extra normalization multiplication. */
    const kiss_fft_cpx *post = kernel->post_rotation;
    if (kernel->scaling == LHDC_DCT4_SCALE_IN_ROTATION_TABLE) {
        k = 0;
#if LHDC_DCT4_USE_SSE2
        const __m128 sign_mask = _mm_set1_ps(-0.0f);
        for (; k + 2 <= fft_length; k += 2) {
            __m128 S = _mm_loadu_ps((const float *)&work->spectrum[k]);
            __m128 W = _mm_loadu_ps((const float *)&post[k]);

            __m128 S_rr = _mm_shuffle_ps(S, S, _MM_SHUFFLE(2, 2, 0, 0));
            __m128 S_ii = _mm_shuffle_ps(S, S, _MM_SHUFFLE(3, 3, 1, 1));
            __m128 W_ir = _mm_shuffle_ps(W, W, _MM_SHUFFLE(2, 3, 0, 1));

            __m128 P1 = _mm_mul_ps(S_rr, W);
            __m128 P2 = _mm_mul_ps(S_ii, W_ir);

            __m128 P_sub = _mm_sub_ps(P1, P2);
            __m128 P_add = _mm_add_ps(P1, P2);
            __m128 P_add_neg = _mm_xor_ps(P_add, sign_mask);

            float sub_buf[4];
            float add_buf[4];
            _mm_storeu_ps(sub_buf, P_sub);
            _mm_storeu_ps(add_buf, P_add_neg);

            output[2 * k] = sub_buf[0];
            output[length - 1 - 2 * k] = add_buf[1];
            output[2 * k + 2] = sub_buf[2];
            output[length - 3 - 2 * k] = add_buf[3];
        }
#endif
        for (; k < fft_length; ++k) {
            float real = work->spectrum[k].r, imag = work->spectrum[k].i;
            float rotate_real = post[k].r, rotate_imag = post[k].i;
            output[2 * k] = real * rotate_real - imag * rotate_imag;
            output[length - 1 - 2 * k] = -(real * rotate_imag + imag * rotate_real);
        }
        return LHDC_DCT4_OK;
    }
    /* 4. V4 and generic orthonormal paths post-rotate first, then normalize separately without folding scaling into tables. */
    const float normalization = kernel->normalization;
    k = 0;
#if LHDC_DCT4_USE_SSE2
    const __m128 norm_vec = _mm_set1_ps(normalization);
    const __m128 sign_mask = _mm_set1_ps(-0.0f);
    for (; k + 2 <= fft_length; k += 2) {
        __m128 S = _mm_loadu_ps((const float *)&work->spectrum[k]);
        __m128 W = _mm_loadu_ps((const float *)&post[k]);

        __m128 S_rr = _mm_shuffle_ps(S, S, _MM_SHUFFLE(2, 2, 0, 0));
        __m128 S_ii = _mm_shuffle_ps(S, S, _MM_SHUFFLE(3, 3, 1, 1));
        __m128 W_ir = _mm_shuffle_ps(W, W, _MM_SHUFFLE(2, 3, 0, 1));

        __m128 P1 = _mm_mul_ps(S_rr, W);
        __m128 P2 = _mm_mul_ps(S_ii, W_ir);

        __m128 P_sub = _mm_mul_ps(_mm_sub_ps(P1, P2), norm_vec);
        __m128 P_add = _mm_add_ps(P1, P2);
        __m128 P_add_neg = _mm_mul_ps(_mm_xor_ps(P_add, sign_mask), norm_vec);

        float sub_buf[4];
        float add_buf[4];
        _mm_storeu_ps(sub_buf, P_sub);
        _mm_storeu_ps(add_buf, P_add_neg);

        output[2 * k] = sub_buf[0];
        output[length - 1 - 2 * k] = add_buf[1];
        output[2 * k + 2] = sub_buf[2];
        output[length - 3 - 2 * k] = add_buf[3];
    }
#endif
    for (; k < fft_length; ++k) {
        float real = work->spectrum[k].r, imag = work->spectrum[k].i;
        float rotate_real = post[k].r, rotate_imag = post[k].i;
        output[2 * k] = normalization * (real * rotate_real - imag * rotate_imag);
        output[length - 1 - 2 * k] = normalization * -(imag * rotate_real + real * rotate_imag);
    }
    return LHDC_DCT4_OK;
}
