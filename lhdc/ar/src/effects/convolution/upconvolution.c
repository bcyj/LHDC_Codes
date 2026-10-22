#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"
#include "../../core/include/ar_workspace_views.h"
#include <string.h>

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
        for (int32_t j = 0; j < LHDC_CONST(AR_HALF_SPECTRUM_FLOATS); j += 2) {
            float input_real = input_spectrum[j], input_imag = input_spectrum[j + 1],
                  coef_real = c[j], coef_imag = c[j + 1];
            float sum_real = sum[j], sum_imag = sum[j + 1];
            sum[j] = sum_real + (input_real * coef_real - input_imag * coef_imag);
#if LHDC_AR_ROUNDING_PROFILE == 64
            sum[j + 1] = alias ? (sum_imag + input_imag * coef_real) + input_real * coef_imag
                               : (input_imag * coef_real + input_real * coef_imag) + sum_imag;
#else
            (void)alias;
            sum[j + 1] = (input_real * coef_imag + input_imag * coef_real) + sum_imag;
#endif
        }
        for (int32_t j = 0; j < LHDC_CONST(AR_HALF_SPECTRUM_FLOATS); j += 2) {
            /* The packed 513-float half-spectrum stores only the Nyquist real part; its imaginary part is zero.
             * Reading index 513 at j=0 would overrun the partition and interpret adjacent directory pointer bits as float.
             * Pointer bits representing NaN/Inf could contaminate all reverb output even with zero input. */
            float input_real = input_spectrum[LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) - j],
                  input_imag =
                      j == 0 && input_buf_list->data_size == LHDC_CONST(AR_PARTITION_DATA_FLOATS)
                          ? 0.0f
                          : input_spectrum[LHDC_CONST(AR_MIRROR_IMAG_START) - j],
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
        audio_math_fft_512(data, 0, workspace->fft_work);
        LHDC_FN(cc_buf_list_update)(workspace->channel[j].buf_list, data);
    }
    float *sum[2] = {workspace->sum[0], workspace->sum[1]};
    const float *coef[2][3] = {{lhdc_ar_abir_w_l, lhdc_ar_abir_x_l, lhdc_ar_abir_y_l},
                               {lhdc_ar_abir_w_r, lhdc_ar_abir_x_r, lhdc_ar_abir_y_r}};
    float *out[2] = {l_out_frame, r_out_frame};
    for (int32_t j = 0; j < 2; ++j) {
        memset(sum[j], 0, LHDC_CONST(AR_SPECTRUM_BYTES));
        for (int32_t i = 0; i < 3; ++i)
            LHDC_FN(cmplx_multi)(workspace->channel[i].buf_list, coef[j][i], sum[j]);
    }
    for (int32_t j = 0; j < 2; ++j) {
        audio_math_fft_512(sum[j], 1, workspace->fft_work);
        for (int32_t i = 0; i < LHDC_CONST(AR_BLOCK_SAMPLES); ++i)
            out[j][i] = sum[j][LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + i * 2];
    }
}

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
