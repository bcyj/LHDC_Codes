#include "../../../include/lhdc/decoder/status.h"
#include "../include/lhdc_fft.h"
#include <stdbool.h>
#include "synthesis.h"
#include "synthesis_tables.h"
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#include <audio_math/float_window.h>
#endif

/* Perform pre-rotation, fixed FFT, post-rotation, time rearrangement, window mixing, and history updates.
 * Stages follow integer modular arithmetic and Q31 truncation; all_time_ac selects window mixing.
 */
void lhdc_dec_lossy_time_operation(void *heap_memory, int *vec_data, int all_time_ac) {
    time_process_top_struct *time_process = heap_memory;
    time_buffer_struct *time_buffer = time_process->time_buffer;
    int *time_ac = time_buffer->fft.tdata;
    kiss_fft_cpx *fft_coef_in = time_buffer->fft.cpx.fft_coef_in;
    kiss_fft_cpx *fft_coef_out = time_buffer->fft.cpx.fft_coef_out;
    int *new_frame_data = time_buffer->dec_tdata;
    int *prev_frame_data = time_process->time_process_ch_num->prev_frame_data;
    int num1 = time_process->num1;
    int num2 = time_process->num2;
    int num4 = time_process->num4;

    /* Even spectrum entries and reversed odd tail entries form complex FFT input. */
    for (int i = 0; i < num2; ++i) {
        int pdr_tmp = vec_data[i * 2];
        int pdi_tmp = vec_data[num1 - 1 - i * 2];
        int time_pre_twid_r_tmp = time_process->pre_twid[i].r;
        int time_pre_twid_i_tmp = time_process->pre_twid[i].i;
        uint64_t real_product =
            time_product(pdr_tmp, time_pre_twid_r_tmp) - time_product(pdi_tmp, time_pre_twid_i_tmp);
        uint64_t imag_product =
            time_product(pdi_tmp, time_pre_twid_r_tmp) + time_product(pdr_tmp, time_pre_twid_i_tmp);

        fft_coef_in[i].i = time_q31(imag_product);
        fft_coef_in[i].r = time_q31(real_product);
    }
    lhdc_v5_fft(time_process->kiss_fft_cfg, fft_coef_in, fft_coef_out);
    for (int i = 0; i < num2; ++i) {
        fft_coef_out[i].r = audio_math_multiply_wrap_i32(num2, fft_coef_out[i].r);
        fft_coef_out[i].i = audio_math_multiply_wrap_i32(num2, fft_coef_out[i].i);
    }

    /* Post-rotation writes evens forward and odds backward; negate before 64-bit truncation. */
    for (int i = 0; i < num2; ++i) {
        int cout_r_tmp = fft_coef_out[i].r;
        int cout_i_tmp = fft_coef_out[i].i;
        int time_pos_twid_r_tmp = time_process->pos_twid[i].r;
        int time_pos_twid_i_tmp = time_process->pos_twid[i].i;
        uint64_t real_product = time_product(cout_r_tmp, time_pos_twid_r_tmp) -
                                time_product(cout_i_tmp, time_pos_twid_i_tmp);
        uint64_t imag_product = time_product(cout_i_tmp, time_pos_twid_r_tmp) +
                                time_product(cout_r_tmp, time_pos_twid_i_tmp);
        new_frame_data[i * 2] = time_q31(real_product);
        new_frame_data[num1 - 1 - i * 2] = time_q31(UINT64_C(0) - imag_product);
    }
    /* Expand four IMDCT symmetry sections in sequence; merging loops would change overwrite order. */
    for (int i = 0; i < num2; ++i)
        time_ac[i] = new_frame_data[num2 + i];
    for (int i = 0; i < num2; ++i)
        time_ac[num2 + i] = audio_math_subtract_wrap_i32(0, new_frame_data[num1 - 1 - i]);
    for (int i = 0; i < num2; ++i)
        time_ac[num1 + i] = audio_math_subtract_wrap_i32(0, new_frame_data[num2 - 1 - i]);
    for (int i = 0; i < num2; ++i)
        time_ac[num1 + num2 + i] = audio_math_subtract_wrap_i32(0, new_frame_data[i]);

    int repeating = time_process->repeating;
    const int *win_hann = time_process->freqency_win;
    if (all_time_ac != 0) {
        for (int i = 0; i < repeating; ++i) {
            uint64_t window_product = time_product(time_ac[num4 + i], win_hann[i]) +
                                      time_product(prev_frame_data[i], win_hann[repeating - 1 - i]);
            vec_data[i] = time_q31(window_product);
        }
    }
    for (int i = repeating; i < num1; ++i)
        vec_data[i] = time_ac[num4 + i];
    for (int i = 0; i < repeating; ++i)
        prev_frame_data[i] = time_ac[num4 + num1 + i];
}
#include "../include/lhdc_fft.h"
#include <audio_math/constants.h>
#include <audio_math/math.h>
#include <audio_math/float_convert.h>
#include "../include/lhdc_workspace_layout.h"
#include <limits.h>
#include <math.h>

/* Convert bounded trig coefficients to saturated Q31, preserving power-of-two scaling and truncation toward zero.
 * Win32 kernel builds use common saturation directly, avoiding CRT helpers from intermediate int64 conversion. */
static int q31(float value) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    return audio_math_truncate_sat_i32_f32(value * AUDIO_MATH_POW2_31_F32);
#else
    int64_t value_local = (int64_t)(value * AUDIO_MATH_POW2_31_F32);
    if (value_local > INT32_MAX)
        return INT32_MAX;
    if (value_local < INT32_MIN)
        return INT32_MIN;
    return (int)value_local;
#endif
}

/* Bind caller buffers, copy fixed Q31 coefficients, and initialize the internal integer FFT.
 * Other sizes retain stepwise float phase; Windows kernel builds generate twiddles with bounded software trig.
 * Window generation retains inner double sine/multiplication and round to float only after the outer sine.
 */
int lhdc_dec_lossy_time_table_start(void *heap_memory, int frequency_size, int repeating,
                                    int sft_num, time_buffer_struct *time_buf,
                                    kiss_fft_cpx *p_pre_twid, kiss_fft_cpx *p_pos_twid,
                                    int *p_time_window, kiss_fft_cpx *p_fft_twiddle) {
    if (!heap_memory)
        return (int)sizeof(LHDCV5_TimeWorkspace);
    LHDCV5_TimeWorkspace *workspace = heap_memory;
    time_process_top_struct *time_process = &workspace->time_process;
    int num2 = frequency_size / 4;
    time_process->size = frequency_size;
    time_process->repeating = repeating;
    time_process->num1 = frequency_size / 2;
    time_process->num2 = num2;
    time_process->num4 = num2 - repeating / 2;
    time_process->time_buffer = time_buf;
    time_process->pre_twid = p_pre_twid;
    time_process->pos_twid = p_pos_twid;
    time_process->freqency_win = p_time_window;
    if (!lhdc_v5_time_copy_rotations(frequency_size, p_pre_twid, p_pos_twid)) {
        for (int i = 0; i < num2; ++i) {
            double phase =
                (double)(((float)i + 0.25f) * -AUDIO_MATH_PI_F32 / (float)(frequency_size / 2));
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            float sine, cosine;
            audio_math_sincos_bounded_f64_to_f32_soft(phase, &sine, &cosine);
            time_process->pre_twid[i].r = q31(cosine);
            time_process->pre_twid[i].i = q31(sine);
#else
            time_process->pre_twid[i].r = q31((float)audio_math_host_cos(phase));
            time_process->pre_twid[i].i = q31((float)audio_math_host_sin(phase));
#endif
            phase = (double)((float)i * -AUDIO_MATH_PI_F32 / (float)(frequency_size / 2));
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            audio_math_sincos_bounded_f64_to_f32_soft(phase, &sine, &cosine);
            time_process->pos_twid[i].r = q31(cosine);
            time_process->pos_twid[i].i = q31(sine);
#else
            time_process->pos_twid[i].r = q31((float)audio_math_host_cos(phase));
            time_process->pos_twid[i].i = q31((float)audio_math_host_sin(phase));
#endif
        }
    }
    if (!lhdc_v5_time_copy_window(repeating, p_time_window)) {
        for (int i = 0; i < repeating; ++i) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            double phase = (double)(((float)i + 0.5f) * AUDIO_MATH_HALF_PI_F32 / (float)repeating);
            time_process->freqency_win[i] =
                q31(audio_math_sine_squared_window_bounded_f64_to_f32_soft(phase));
#else
            double phase = audio_math_host_sin(
                (double)(((float)i + 0.5f) * AUDIO_MATH_HALF_PI_F32 / (float)repeating));
            double window_phase = (phase * (double)AUDIO_MATH_HALF_PI_F32) * phase;
            time_process->freqency_win[i] = q31((float)audio_math_host_sin(window_phase));
#endif
        }
    }
    time_process->kiss_fft_cfg = &workspace->fft;
    size_t fft_window_size = sizeof(workspace->fft);
    lhdc_v5_fft_alloc(num2, 0, time_process->kiss_fft_cfg, &fft_window_size, p_fft_twiddle);
    time_process->offset_num = (unsigned)sft_num;
    unsigned n = ((unsigned)sft_num - 1) & 255;
    time_process->offset_num_value_half = audio_math_int32_from_bits(audio_math_shift_left_low8_zero32(1u, n));
    return LHDC_DEC_OK;
}
#include <audio_math/bits.h>
#include "../include/lhdc_dec_control.h"
#include <string.h>

