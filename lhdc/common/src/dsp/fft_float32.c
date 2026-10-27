#include "../../include/lhdc_memory.h"
#include "../../include/dsp/fft_float32.h"
#include <audio_math/float_sqrt.h>
#include <audio_math/fft_float32_tables.h>

/* Prepare generic-radix scratch for calls lacking it, then enter the allocation-free shared kernel. */
void lhdc_kiss_fft_work(kiss_fft_cpx *output, const kiss_fft_cpx *input, size_t twiddle_stride,
                        int32_t input_stride, const int32_t *factors, const LhdcKissFftPlan *plan) {
    LhdcKissFftPlan local = *plan;
    kiss_fft_cpx *allocated = NULL;
    if (plan->allow_generic_radix && !plan->generic_scratch) {
        int maximum = 0;
        for (int stage = 0;; stage += 2) {
            if (factors[stage] > maximum)
                maximum = factors[stage];
            if (factors[stage + 1] == 1)
                break;
        }
        if (maximum > 5) {
            allocated = lhdc_memory_alloc(NULL, (size_t)maximum * sizeof *allocated);
            if (!allocated)
                return;
            local.generic_scratch = allocated;
        }
    }
    audio_math_fft_work(output, input, twiddle_stride, input_stride, factors, &local);
    lhdc_memory_free(allocated);
}

/* Gather complex input, execute the common FFT, and store using caller stride and normalization. */
int lhdc_kiss_fft_split(float *real, float *imag, int32_t length, int32_t stride, int inverse,
                        int normalize) {
    if (!real || !imag || length < 1 || stride < 1 ||
        (size_t)length > SIZE_MAX / (3 * sizeof(kiss_fft_cpx)))
        return -1;
    if (length == 1)
        return 0;
    kiss_fft_cpx *memory =
        lhdc_memory_alloc(NULL, (size_t)length * (length == 512 ? 2 : 3) * sizeof *memory);
    if (!memory)
        return -1;
    kiss_fft_cpx *input = memory, *output = memory + length;
    const kiss_fft_cpx *twiddles =
        length == 512 ? audio_math_fft_512_twiddles[inverse != 0] : memory + 2 * length;
    int32_t factors[64];
    LhdcKissFftPlan plan = {length, inverse != 0, twiddles, 1, NULL};
    lhdc_kiss_fft_factor(length, (int32_t)audio_math_sqrt_floor_u32((uint32_t)length), factors);
    if (length != 512)
        lhdc_kiss_fft_twiddles(length, inverse != 0, memory + 2 * length);
    for (int32_t sample = 0; sample < length; ++sample) {
        size_t index = (size_t)sample * stride;
        input[sample] = (kiss_fft_cpx){real[index], imag[index]};
    }
    lhdc_kiss_fft_work(output, input, 1, 1, factors, &plan);
    for (int32_t sample = 0; sample < length; ++sample) {
        size_t index = (size_t)sample * stride;
        real[index] = inverse && normalize ? output[sample].r / (float)length : output[sample].r;
        imag[index] = inverse && normalize ? output[sample].i / (float)length : output[sample].i;
    }
    lhdc_memory_free(memory);
    return 0;
}

/* Convenience split-channel power-of-two complex FFT entry point, optionally using the
 * split-radix vector kernel. */
int lhdc_kiss_fft_power2_split(float *real, float *imag, int32_t length, int32_t stride,
                               int inverse, int normalize) {
    if (!real || !imag || length < 4 || stride < 1 ||
        (size_t)length > SIZE_MAX / (2 * sizeof(kiss_fft_cpx)))
        return -1;
#if AUDIO_MATH_HAS_FFT_SSE
    if (!audio_math_fft_sse_supported(length))
        return lhdc_kiss_fft_split(real, imag, length, stride, inverse, normalize);

    kiss_fft_cpx *memory = lhdc_memory_alloc(NULL, (size_t)length * 2 * sizeof *memory);
    if (!memory)
        return -1;
    kiss_fft_cpx *input = memory, *scratch = memory + length;
    for (int32_t sample = 0; sample < length; ++sample) {
        size_t index = (size_t)sample * stride;
        input[sample] = (kiss_fft_cpx){real[index], imag[index]};
    }
    if (!audio_math_fft_power2(input, input, scratch, length, inverse != 0)) {
        lhdc_memory_free(memory);
        return -1;
    }
    for (int32_t sample = 0; sample < length; ++sample) {
        size_t index = (size_t)sample * stride;
        real[index] = inverse && normalize ? input[sample].r / (float)length : input[sample].r;
        imag[index] = inverse && normalize ? input[sample].i / (float)length : input[sample].i;
    }
    lhdc_memory_free(memory);
    return 0;
#else
    return lhdc_kiss_fft_split(real, imag, length, stride, inverse, normalize);
#endif
}
