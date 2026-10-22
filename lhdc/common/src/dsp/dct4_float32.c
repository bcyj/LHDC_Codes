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
    lhdc_kiss_fft_factor(fft_length, (int32_t)audio_math_sqrt_floor_u32((uint32_t)fft_length), plan->factors);
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
            audio_math_sincos_bounded_f32_soft(pre_phase, &rotation_storage[k].i, &rotation_storage[k].r);
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
    for (int32_t k = 0; k < fft_length; ++k) {
        float even = input[2 * k], reversed_odd = input[length - 1 - 2 * k];
        kiss_fft_cpx rotation;
        memcpy(&rotation, (const unsigned char *)kernel->pre_rotation + (size_t)k * sizeof rotation,
               sizeof rotation);
        float real = rotation.r, imag = rotation.i;
        work->packed_input[k].r = even * real - reversed_odd * imag;
        work->packed_input[k].i = reversed_odd * real + even * imag;
    }
    /* 2. FFT is unnormalized; generic-radix scratch belongs to this call. */
    LhdcKissFftPlan fft = {fft_length, 0, kernel->fft_twiddles, kernel->allow_generic_radix,
                           work->radix_scratch};
    lhdc_kiss_fft_work(work->spectrum, work->packed_input, 1, 1, kernel->factors, &fft);
    /* 3. V5's post-table includes scaling; apply post-rotation and omit extra normalization multiplication. */
    if (kernel->scaling == LHDC_DCT4_SCALE_IN_ROTATION_TABLE) {
        for (int32_t k = 0; k < fft_length; ++k) {
            float real = work->spectrum[k].r, imag = work->spectrum[k].i;
            kiss_fft_cpx rotation;
            memcpy(&rotation,
                   (const unsigned char *)kernel->post_rotation + (size_t)k * sizeof rotation,
                   sizeof rotation);
            float rotate_real = rotation.r, rotate_imag = rotation.i;
            output[2 * k] = real * rotate_real - imag * rotate_imag;
            output[length - 1 - 2 * k] = -(real * rotate_imag + imag * rotate_real);
        }
        return LHDC_DCT4_OK;
    }
    /* 4. V4 and generic orthonormal paths post-rotate first, then normalize separately without folding scaling into tables. */
    const float normalization = kernel->normalization;
    for (int32_t k = 0; k < fft_length; ++k) {
        float real = work->spectrum[k].r, imag = work->spectrum[k].i;
        kiss_fft_cpx rotation;
        memcpy(&rotation,
               (const unsigned char *)kernel->post_rotation + (size_t)k * sizeof rotation,
               sizeof rotation);
        float rotate_real = rotation.r, rotate_imag = rotation.i;
        output[2 * k] = normalization * (real * rotate_real - imag * rotate_imag);
        output[length - 1 - 2 * k] = normalization * -(imag * rotate_real + real * rotate_imag);
    }
    return LHDC_DCT4_OK;
}
