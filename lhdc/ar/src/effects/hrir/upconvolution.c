#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"
#include "../../core/include/ar_workspace_views.h"
#include <string.h>

void LHDC_FN(cmplx_multi)(cc_buf_list *list, const float *coef, float *sum);
#include "upconvolution.h"

/* Apply binaural HRIR convolution to three directional sources, mix low frequencies, and output stereo. */
void LHDC_FN(hrir_upconv_process)(PST_LHDC_AR state, int32_t sample_offset) {
    float *fl_in_frame = state->pfPcm[0] + sample_offset,
          *fr_in_frame = state->pfPcm[1] + sample_offset,
          *c_in_frame = state->pfPcm[2] + sample_offset,
          *lfe_in_frame = state->pfPcm[3] + sample_offset;
    const float *hrir_fl_l = state->pfFir256CoefCur[0][0],
                *hrir_fl_r = state->pfFir256CoefCur[0][1],
                *hrir_fr_l = state->pfFir256CoefCur[1][0],
                *hrir_fr_r = state->pfFir256CoefCur[1][1], *hrir_c_l = state->pfFir256CoefCur[2][0],
                *hrir_c_r = state->pfFir256CoefCur[2][1];
    float *ch_mix_gain = state->pfCh51GainPost,
          *l_out_frame = state->pfPcmOutput[0] + sample_offset,
          *r_out_frame = state->pfPcmOutput[1] + sample_offset;
    uint8_t *upconv_mem = state->pucHrirUpolsMem;
    LHDCHrirDirectory *workspace = (void *)upconv_mem;
    float *data = workspace->fft_data, *kernel = workspace->kernel,
          *sums[2] = {workspace->sum[0], workspace->sum[1]};
    float *inputs[3] = {c_in_frame, fl_in_frame, fr_in_frame};
    const float *coefs[3][2] = {
        {hrir_c_l, hrir_c_r}, {hrir_fl_l, hrir_fl_r}, {hrir_fr_l, hrir_fr_r}};
    int32_t index[3] = {2, 0, 1};
    memset(sums[0], 0, LHDC_CONST(AR_SPECTRUM_BYTES));
    memset(sums[1], 0, LHDC_CONST(AR_SPECTRUM_BYTES));
    for (int32_t source_channel = 0; source_channel < 3; ++source_channel) {
        LHDC_FN(fft_buffer_n_to_4n)
        (inputs[source_channel], workspace->input_history[source_channel], data,
         LHDC_CONST(AR_BLOCK_SAMPLES), LHDC_CONST(AR_SPECTRUM_FLOATS));
        audio_math_fft_512(data, 0, workspace->fft_work);
        for (int32_t i = 0; i < LHDC_CONST(AR_SPECTRUM_FLOATS); ++i)
            data[i] *= ch_mix_gain[index[source_channel]];
        for (int32_t ear = 0; ear < 2; ++ear) {
            LHDC_FN(fft_buffer_n_to_4n)
            (coefs[source_channel][ear], 0, kernel, LHDC_CONST(AR_BLOCK_SAMPLES),
             LHDC_CONST(AR_SPECTRUM_FLOATS));
            audio_math_fft_512(kernel, 0, workspace->fft_work);
            LHDC_PRIVATE(accumulate_complex_spectrum)(sums[ear], data, kernel);
        }
    }
    float *out[2] = {l_out_frame, r_out_frame};
    for (int32_t ear = 0; ear < 2; ++ear) {
        audio_math_fft_512(sums[ear], 1, workspace->fft_work);
        for (int32_t i = 0; i < LHDC_CONST(AR_BLOCK_SAMPLES); ++i)
            out[ear][i] = sums[ear][LHDC_CONST(AR_HALF_SPECTRUM_FLOATS) + i * 2];
    }
    for (int32_t i = 0; i < LHDC_CONST(AR_BLOCK_SAMPLES); ++i) {
        l_out_frame[i] += ch_mix_gain[3] * lfe_in_frame[i];
        r_out_frame[i] += ch_mix_gain[3] * lfe_in_frame[i];
    }
}

/* Partition aligned HRIR-convolution workspace and return its total size. */
void LHDC_FN(hrir_upconv_alloc)(uint8_t *allocated_mem_addr, uint32_t *mem_size) {
    uint32_t total = lhdc_workspace_align8(LHDC_CONST(AR_HRIR_SLOT_COUNT) * sizeof(void *)) +
                     LHDC_CONST(AR_HRIR_DATA_BYTES) + sizeof(AudioMathFft512Work);
    if (!allocated_mem_addr) {
        *mem_size = total;
        return;
    }
    if (*mem_size != total)
        return;
    memset(allocated_mem_addr, 0, total);
    LHDCHrirDirectory *workspace = (void *)allocated_mem_addr;
    uint32_t cursor = lhdc_workspace_align8(sizeof(*workspace));
    workspace->fft_data =
        lhdc_workspace_take(allocated_mem_addr, &cursor, LHDC_CONST(AR_SPECTRUM_BYTES));
    for (int32_t ch = 0; ch < 3; ++ch)
        workspace->input_history[ch] =
            lhdc_workspace_take(allocated_mem_addr, &cursor, LHDC_CONST(AR_BLOCK_BYTES));
    workspace->kernel =
        lhdc_workspace_take(allocated_mem_addr, &cursor, LHDC_CONST(AR_SPECTRUM_BYTES));
    for (int32_t ear = 0; ear < 2; ++ear)
        workspace->sum[ear] =
            lhdc_workspace_take(allocated_mem_addr, &cursor, LHDC_CONST(AR_SPECTRUM_BYTES));
    workspace->fft_work =
        lhdc_workspace_take(allocated_mem_addr, &cursor, sizeof(AudioMathFft512Work));
}
