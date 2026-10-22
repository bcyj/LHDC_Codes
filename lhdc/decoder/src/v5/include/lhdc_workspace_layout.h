/* Native decoder workspace with natural field alignment and sizeof-based capacity queries. */
#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_WORKSPACE_LAYOUT_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_WORKSPACE_LAYOUT_H
#include "lhdc_fft.h"
#include "lhdc_arith.h"
#include "workspace_directory_view.h"
typedef struct LHDCV5_TimeWorkspace {
    time_process_top_struct time_process;
    kiss_fft_state fft;
} LHDCV5_TimeWorkspace;
typedef struct LHDCV5_WorkspacePrefix {
    LHDCV5_DecWorkspaceDirectory directory;
    int parameter_khz_max, parameter_ms_max;
    int parameter_lossless_maybe_enable, parameter_max_freq_repeating;
    tdata_all_buffer_struct dbuffer;
} LHDCV5_WorkspacePrefix;
typedef struct LHDCV5_ArithmeticWorkspace {
    dec_arith_struct dec_arith_s;
    nbyte_read_struct inputter;
    unsigned short seed;
    /* Valid-frame descrambling capacity; word storage ensures natural bit-reader alignment. */
    uint32_t encoded_data_ch[688 / sizeof(uint32_t)];
    int channel_mode;
} LHDCV5_ArithmeticWorkspace;
#endif
