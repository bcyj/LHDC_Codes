/* Generic single-precision orthonormal DCT-IV; no platform instructions or internal allocation, using caller workspace. */
#ifndef LHDC_DSP_DCT4_FLOAT32_H
#define LHDC_DSP_DCT4_FLOAT32_H
#include <stddef.h>
#include <stdint.h>
#include "../types/lhdc_dsp_types.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { LHDC_DCT4_OK = 0, LHDC_DCT4_INVALID_ARGUMENT = -1, LHDC_DCT4_BUFFER_TOO_SMALL = -2 };
/* Read-only after initialization and shareable across instances; all three rotation tables interleave real/imaginary parts. */
typedef struct LhdcDct4F32Plan {
    int32_t length;
    int32_t factors[64];
    int32_t maximum_radix;
    float normalization;
    const kiss_fft_cpx *pre_rotation;
    const kiss_fft_cpx *post_rotation;
    const kiss_fft_cpx *fft_twiddles;
} LhdcDct4F32Plan;
/* Each concurrent call owns separate actual complex workspace; capacity remains measured in floats.
 * The three work areas must not overlap each other, rotation tables, input, or output. input==output is allowed.
 */
typedef struct LhdcDct4F32Work {
    kiss_fft_cpx *packed_input;
    size_t packed_input_floats;
    kiss_fft_cpx *spectrum;
    size_t spectrum_floats;
    kiss_fft_cpx *radix_scratch;
    size_t radix_scratch_floats;
} LhdcDct4F32Work;
/* Whether the post-rotation table includes output scaling; selected outside the loop to avoid multiplying V5 values by 1. */
typedef enum LhdcDct4F32Scaling {
    LHDC_DCT4_SCALE_AFTER_ROTATION = 0,
    LHDC_DCT4_SCALE_IN_ROTATION_TABLE = 1
} LhdcDct4F32Scaling;
/* Borrow existing instance rotation tables and a forward FFT plan without generating, copying, or rescaling factors.
 * Fixed-size memcpy reads pre/post rotations, allowing differently named two-float complex structures.
 * FFT twiddles and work buffers must be kiss_fft_cpx objects, not cast float arrays.
 * factors points to at least 64 int32_t KISS factors; all pointers remain valid throughout the call.
 */
typedef struct LhdcDct4F32Kernel {
    int32_t length;
    const int32_t *factors;
    const void *pre_rotation;
    const void *post_rotation;
    const kiss_fft_cpx *fft_twiddles;
    int allow_generic_radix;
    LhdcDct4F32Scaling scaling;
    float normalization;
} LhdcDct4F32Kernel;
/* Execute DCT-IV using a borrowed plan with Work buffer constraints; radix_scratch may be omitted if generic radix is disabled. */
int lhdc_dct4_f32_execute(const LhdcDct4F32Kernel *kernel, const float *input, float *output,
                          const LhdcDct4F32Work *work);
/* Query rotation-table float count; return zero for unsupported lengths or size overflow. */
size_t lhdc_dct4_f32_rotation_floats(int32_t length);
/* Initialize three complex rotation tables; storage_floats counts floats, and only initialization calls libm. */
int lhdc_dct4_f32_init(LhdcDct4F32Plan *plan, int32_t length, kiss_fft_cpx *rotation_storage,
                       size_t storage_floats);
/* Query generic-butterfly scratch floats; each of the other two work buffers needs length floats. */
size_t lhdc_dct4_f32_radix_scratch_floats(const LhdcDct4F32Plan *plan);
/* Execute sqrt(2/N)-normalized DCT-IV, supporting in-place operation without per-frame allocation. */
int lhdc_dct4_f32_transform(const LhdcDct4F32Plan *plan, const float *input, float *output,
                            const LhdcDct4F32Work *work);
#ifdef __cplusplus
}
#endif
#endif
