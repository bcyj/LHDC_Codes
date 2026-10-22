/* Native workspace pointer directory; accessors map legacy query indices without field-offset dependencies. */
#ifndef LHDC_DECODER_SRC_V5_INCLUDE_WORKSPACE_DIRECTORY_VIEW_H
#define LHDC_DECODER_SRC_V5_INCLUDE_WORKSPACE_DIRECTORY_VIEW_H
#include <stddef.h>
#include <stdint.h>
typedef struct LHDCV5_DecWorkspaceDirectory {
    int *parameter_khz_max;
    int *parameter_ms_max;
    int *parameter_lossless_maybe_enable;
    int *parameter_max_freq_repeating;
    void *dbuffer;
    void *time_mem_s_fft_cpx_coef_in;
    void *time_mem_s_fft_cpx_coef_out;
    void *time_mem_s_fft_fdata;
    void *time_mem_s_dec_tdata;
    void *level_quant_2_symbol;
    void *level_segment;
    void *level_jump_bindata;
    void *dbuffer_freq_heap;
    void *freq_ch;
    void *freq_ch_fprev;
    void *dec_arith_s;
    void *inputter;
    uint16_t *seed;
    void *encoded_data_ch;
    void *channel_mode;
    void *time_pre_twid;
    void *time_pos_twid;
    void *time_window;
    void *fft_twiddle;
} LHDCV5_DecWorkspaceDirectory;
#endif
