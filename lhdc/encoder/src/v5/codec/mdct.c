#include <audio_math/float_properties.h>
/* V5 low-delay MDCT: short-overlap folding, DCT-IV pre-rotation, complex FFT, then post-rotation
 * reordering. The analysis length is 2N, each frame consumes N points and emits N coefficients,
 * and FFT length is N/2. Internal fields retain names/layout len=2N, N2=N, and N4=N/2; local
 * variables provide semantic names.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/float_rounding.h>
#include <audio_math/float_scaling.h>
#include <audio_math/float_convert.h>

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_mdct.h>
#endif
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../include/codec/codec_internal.h"
#include "../../../../common/include/dsp/dct4_float32.h"
#include "mdct_tables.h"
#include <math.h>
#include <string.h>

#if defined(__SSE2__) ||                                                                          \
    (defined(_MSC_VER) && (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86))))
#include <emmintrin.h>
#define LHDC_V5_MDCT_USE_SSE2 1
#else
#define LHDC_V5_MDCT_USE_SSE2 0
#endif

#if LHDC_V5_ENCODER_FIXED_FFT
/* Generate Q31 FFT twiddles from double phase using nearest-integer rounding. */
static int init_fixed_fft(mdct_s *mdct) {
    const int32_t length = mdct->N4;
    if (length != 60 && length != 120 && length != 240 && length != 480)
        return LHDC_V5_ENC_ERROR;
    if (!lhdc_v5_mdct_copy_fixed_twiddles(length, mdct->fixed_fft.twiddles)) {
        for (int32_t k = 0; k < length; ++k) {
            double phase = -AUDIO_MATH_TWO_PI_F64 * (double)k / (double)length;
            mdct->fixed_fft.twiddles[k].r = audio_math_truncate_i32_f64(
                audio_math_floor_f64(0.5 + (double)INT32_MAX * audio_math_cos(phase)));
            mdct->fixed_fft.twiddles[k].i = audio_math_truncate_i32_f64(
                audio_math_floor_f64(0.5 + (double)INT32_MAX * audio_math_sin(phase)));
        }
    }
    return LHDC_V5_ENC_OK;
}

/* Block-scale finite float samples to integers with two headroom bits and symmetric nearest rounding. */
static int32_t quantize_fft_input(float value, int exponent_shift) {
    double scaled = audio_math_scale_pow2_normal_f64((double)value, exponent_shift);
    return audio_math_truncate_i32_f64(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

/* Perform floating-point pre-rotation, block scaling, fixed-point FFT, 1/N scale restoration,
 * and floating-point post-rotation. Windows, rotation tables, and spectral interfaces remain
 * floating point. Nonfinite input returns zero so the caller can select the floating path. */
static int fixed_fft_mdct(mdct_s *mdct, const float *folded, float *coefficients) {
    const int32_t length = mdct->N4;
    float peak = 0.0f;
    for (int32_t k = 0; k < length; ++k) {
        float even = folded[2 * k], odd = folded[mdct->N2 - 1 - 2 * k];
        float real = even * mdct->twid1[k].r - odd * mdct->twid1[k].i;
        float imag = odd * mdct->twid1[k].r + even * mdct->twid1[k].i;
        if (!audio_math_is_finite_f32(real) || !audio_math_is_finite_f32(imag))
            return 0;
        mdct->buf->cin[k] = (kiss_fft_cpx){real, imag};
        if (audio_math_abs_f32(real) > peak)
            peak = audio_math_abs_f32(real);
        if (audio_math_abs_f32(imag) > peak)
            peak = audio_math_abs_f32(imag);
    }
    int exponent = audio_math_frexp_exponent_f32(peak);
    const int exponent_shift = peak == 0.0f ? 0 : 29 - exponent;
    LhdcV5EncoderFixedFftWork *work = &mdct->fixed_fft;
    for (int32_t k = 0; k < length; ++k) {
        work->input[k].r = quantize_fft_input(mdct->buf->cin[k].r, exponent_shift);
        work->input[k].i = quantize_fft_input(mdct->buf->cin[k].i, exponent_shift);
    }
    const LhdcFftFixed32Plan plan = {length, 0, work->twiddles};
    (void)lhdc_fft_fixed32_execute(&plan, work->input, work->output);
    for (int32_t k = 0; k < length; ++k) {
        float real = (float)audio_math_scale_pow2_normal_f64((double)work->output[k].r * length,
                                                             -exponent_shift);
        float imag = (float)audio_math_scale_pow2_normal_f64((double)work->output[k].i * length,
                                                             -exponent_shift);
        mdct->buf->cout[k] = (kiss_fft_cpx){real, imag};
        coefficients[2 * k] = real * mdct->twid2[k].r - imag * mdct->twid2[k].i;
        coefficients[mdct->N2 - 1 - 2 * k] = -(real * mdct->twid2[k].i + imag * mdct->twid2[k].r);
    }
    return 1;
}
#endif

/* Return bytes for MDCT state and the immediately following complex FFT state. */
int32_t v5f_mdct_get_size(void) {
    return (int32_t)(sizeof(mdct_s) + sizeof(kiss_fft_state_v5));
}

/* Configure analysis length, overlap, Vorbis-shaped window, and DCT-IV pre/post-rotation tables.
 * mdct_len is 2N, overlap is O history samples, and the post-rotation table embeds
 * 2^shift_bits/(N/2) scaling.
 */
int32_t v5f_mdct_init(void *workspace, int32_t mdct_len, int32_t overlap, int32_t shift_bits,
                      mdct_buf_s *buf, kiss_fft_cpx *ptr_mdct_twid1, kiss_fft_cpx *ptr_mdct_twid2,
                      float *ptr_mdct_win, kiss_fft_cpx *ptr_fft_twid) {
    size_t fft_state_bytes = sizeof(kiss_fft_state_v5);
    if (!workspace)
        return (int32_t)(sizeof(mdct_s) + fft_state_bytes);

    /* 1. Workspace holds state/pointers only; callers provide rotations, windows, and computation buffers. */
    mdct_s *mdct = workspace;
    mdct->len = mdct_len;
    mdct->overlap = overlap;
    mdct->shift_bits = shift_bits;
    mdct->N2 = mdct_len / 2;
    mdct->N4 = mdct_len / 4;
    mdct->NK = overlap / 2;
    mdct->ZN = (mdct_len - mdct_len / 2 - overlap) / 2;
    mdct->buf = buf;
    mdct->win = ptr_mdct_win;
    mdct->twid1 = ptr_mdct_twid1;
    mdct->twid2 = ptr_mdct_twid2;
#if LHDC_V5_ENCODER_FIXED_FFT
    if (init_fixed_fft(mdct) != LHDC_V5_ENC_OK)
        return LHDC_V5_ENC_ERROR;
#endif

    /* 2. DCT-IV pre-rotation exp(-j*pi*(k+1/4)/N), post-rotation exp(-j*pi*k/N).
     * Production frame lengths reuse read-only coefficients. Other configurations retain the
     * order of first rounding phase, then evaluating double trigonometry and scaling.
     */
    const int32_t frame_samples = mdct->N2;
    const int32_t fft_length = mdct->N4;
    if (!lhdc_v5_mdct_copy_rotations(frame_samples, shift_bits, ptr_mdct_twid1, ptr_mdct_twid2)) {
        double output_scale = (double)((float)(1 << (shift_bits & 31)) / (float)fft_length);
        for (int32_t k = 0; k < fft_length; ++k) {
            double sine, cosine;
            float phase = (float)(((k + .25) * -AUDIO_MATH_PI_F64) / (double)frame_samples);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            audio_math_sincos_quarter_turn_f64_soft((double)phase, &sine, &cosine);
#else
            audio_math_sincos((double)phase, &sine, &cosine);
#endif
            ptr_mdct_twid1[k].r = (float)cosine;
            ptr_mdct_twid1[k].i = (float)sine;

            phase = (float)(((double)k * -AUDIO_MATH_PI_F64) / (double)frame_samples);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            audio_math_sincos_quarter_turn_f64_soft((double)phase, &sine, &cosine);
#else
            audio_math_sincos((double)phase, &sine, &cosine);
#endif
            ptr_mdct_twid2[k].r = (float)(cosine * output_scale);
            ptr_mdct_twid2[k].i = (float)(sine * output_scale);
        }
    }

    /* 3. Short-overlap window w[n]=audio_math_sin(pi/2 * audio_math_sin(pi/2*(n+1/2)/O)^2). */
    if (!lhdc_v5_mdct_copy_window(overlap, ptr_mdct_win)) {
        for (int32_t n = 0; n < overlap; ++n) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            ptr_mdct_win[n] = audio_math_vorbis_window_bounded_f64_to_f32_soft(
                ((n + .5) * AUDIO_MATH_HALF_PI_F64) / (double)overlap);
#else
            double window_sine =
                audio_math_sin(((n + .5) * AUDIO_MATH_HALF_PI_F64) / (double)overlap);
            ptr_mdct_win[n] =
                (float)audio_math_sin(window_sine * window_sine * AUDIO_MATH_HALF_PI_F64);
#endif
        }
    }

    /* 4. Configure the N/2 forward FFT with state immediately following MDCT state. */
    mdct->fs = &((LHDCV5_MdctWorkspace *)workspace)->fft;
    v5f_lhdc_v5_fft_alloc(fft_length, 0, mdct->fs, &fft_state_bytes, ptr_fft_twid);
    return LHDC_V5_MDCT_INITIALIZED;
}

/* Fold an N-sample PCM frame plus O history samples into DCT-IV input and emit N MDCT coefficients.
 * The caller selects channel history through mdct->ch; channel is not used as an index.
 */
void v5f_do_mdct(void *workspace, int32_t channel, const float *pcm, float *coefficients) {
    (void)channel;
    mdct_s *mdct = workspace;
    const int32_t frame_samples = mdct->N2;
    const int32_t fft_length = mdct->N4;
    const int32_t half_overlap = mdct->NK;
    const int32_t unwindowed_pairs = mdct->ZN;
    float *folded = mdct->buf->dct4;
    float *history = mdct->ch->overlap;

    /* 1. Short-overlap time-domain folding. Let H=N/2, K=O/2, and Z=(N-O)/2:
     * folded[0..K)   is the windowed negative sum at the current frame tail;
     * folded[K..H)   is the reversed negative current non-overlap region;
     * folded[H..H+Z) is the reversed negative current frame head;
     * folded[H+Z..N) is the windowed difference from prior-frame overlap.
     * Separate loops handle unwindowed and overlap regions without constructing a zero-padded
     * full 2N-point analysis block.
     */
#if LHDC_V5_MDCT_USE_SSE2
    int32_t n = 0;
    const __m128 sign_mask = _mm_set1_ps(-0.0f);
    for (; n + 4 <= unwindowed_pairs; n += 4) {
        __m128 v1_raw = _mm_loadu_ps(&pcm[frame_samples - mdct->overlap - 4 - n]);
        __m128 v1 = _mm_shuffle_ps(v1_raw, v1_raw, _MM_SHUFFLE(0, 1, 2, 3));
        _mm_storeu_ps(&folded[half_overlap + n], _mm_xor_ps(v1, sign_mask));

        __m128 v2_raw = _mm_loadu_ps(&pcm[unwindowed_pairs - 4 - n]);
        __m128 v2 = _mm_shuffle_ps(v2_raw, v2_raw, _MM_SHUFFLE(0, 1, 2, 3));
        _mm_storeu_ps(&folded[fft_length + n], _mm_xor_ps(v2, sign_mask));
    }
    for (; n < unwindowed_pairs; ++n) {
        folded[half_overlap + n] = -pcm[frame_samples - mdct->overlap - n - 1];
        folded[fft_length + n] = -pcm[unwindowed_pairs - n - 1];
    }

    n = 0;
    for (; n + 4 <= half_overlap; n += 4) {
        __m128 pcm_rev1_raw = _mm_loadu_ps(&pcm[frame_samples - half_overlap - 4 - n]);
        __m128 pcm_rev1 = _mm_shuffle_ps(pcm_rev1_raw, pcm_rev1_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 win_fwd1 = _mm_loadu_ps(&mdct->win[half_overlap + n]);
        __m128 t1 = _mm_mul_ps(pcm_rev1, win_fwd1);

        __m128 pcm_fwd2 = _mm_loadu_ps(&pcm[frame_samples - half_overlap + n]);
        __m128 win_rev2_raw = _mm_loadu_ps(&mdct->win[half_overlap - 4 - n]);
        __m128 win_rev2 = _mm_shuffle_ps(win_rev2_raw, win_rev2_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 t2 = _mm_mul_ps(pcm_fwd2, win_rev2);
        _mm_storeu_ps(&folded[n], _mm_sub_ps(_mm_xor_ps(t1, sign_mask), t2));

        __m128 hist_fwd = _mm_loadu_ps(&history[n]);
        __m128 win_fwd = _mm_loadu_ps(&mdct->win[n]);
        __m128 t3 = _mm_mul_ps(hist_fwd, win_fwd);

        __m128 hist_rev_raw = _mm_loadu_ps(&history[mdct->overlap - 4 - n]);
        __m128 hist_rev = _mm_shuffle_ps(hist_rev_raw, hist_rev_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 win_rev_raw = _mm_loadu_ps(&mdct->win[mdct->overlap - 4 - n]);
        __m128 win_rev = _mm_shuffle_ps(win_rev_raw, win_rev_raw, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 t4 = _mm_mul_ps(hist_rev, win_rev);
        _mm_storeu_ps(&folded[fft_length + unwindowed_pairs + n], _mm_sub_ps(t3, t4));
    }
    for (; n < half_overlap; ++n) {
        folded[n] = -(pcm[frame_samples - half_overlap - n - 1] * mdct->win[half_overlap + n]) -
                    pcm[frame_samples - half_overlap + n] * mdct->win[half_overlap - n - 1];
        folded[fft_length + unwindowed_pairs + n] =
            history[n] * mdct->win[n] -
            history[mdct->overlap - n - 1] * mdct->win[mdct->overlap - n - 1];
    }
#else
    for (int32_t n = 0; n < unwindowed_pairs; ++n) {
        folded[half_overlap + n] = -pcm[frame_samples - mdct->overlap - n - 1];
        folded[fft_length + n] = -pcm[unwindowed_pairs - n - 1];
    }
    for (int32_t n = 0; n < half_overlap; ++n) {
        folded[n] = -(pcm[frame_samples - half_overlap - n - 1] * mdct->win[half_overlap + n]) -
                    pcm[frame_samples - half_overlap + n] * mdct->win[half_overlap - n - 1];
        folded[fft_length + unwindowed_pairs + n] =
            history[n] * mdct->win[n] -
            history[mdct->overlap - n - 1] * mdct->win[mdct->overlap - n - 1];
    }
#endif

    /* 2. Save next-frame history before writing cin: encoder pcm and cin share storage. */
    memcpy(history, pcm + frame_samples - mdct->overlap, mdct->overlap * sizeof(float));

#if LHDC_V5_ENCODER_FIXED_FFT
    if (fixed_fft_mdct(mdct, folded, coefficients))
        return;
#endif

    /* 3. Borrow instance rotations, factors, cin/cout; twid2 includes encoding scaling, so no extra normalization. */
    const LhdcDct4F32Kernel kernel = {frame_samples,
                                      mdct->fs->factors,
                                      mdct->twid1,
                                      mdct->twid2,
                                      mdct->fs->twiddles,
                                      0,
                                      LHDC_DCT4_SCALE_IN_ROTATION_TABLE,
                                      1.0f};
    LhdcDct4F32Work work = {
        mdct->buf->cin, 2 * (size_t)fft_length, mdct->buf->cout, 2 * (size_t)fft_length, NULL, 0};
    (void)lhdc_dct4_f32_execute(&kernel, folded, coefficients, &work);
}
