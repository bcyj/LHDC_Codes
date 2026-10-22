#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DEC_CONTROL_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DEC_CONTROL_H
#include "lhdc_dec.h"
#include <stddef.h>

typedef struct time_process_struct {
    int *prev_frame_data;
} time_process_struct;
typedef struct time_process_top_struct {
    int size, repeating, num1, num2, num4;
    struct kiss_fft_state *kiss_fft_cfg;
    kiss_fft_cpx *pre_twid, *pos_twid;
    int *freqency_win;
    time_process_struct *time_process_ch_num;
    struct time_buffer_struct *time_buffer;
    unsigned offset_num;
    int offset_num_value_half;
} time_process_top_struct;
/* Query samples per frame, rejecting null output or workspace. */
int lhdc_v5_dec_get_samples_per_frame(int *, void *);
/* Set channel mode 0..2; mono forces mode zero. */
int lhdc_v5_dec_set_channel_mode(int, void *);
/* Initialize synthesis pre/post rotations, sine windows, and FFT state. */
int lhdc_dec_lossy_time_table_start(void *, int, int, int, time_buffer_struct *, kiss_fft_cpx *,
                                    kiss_fft_cpx *, int *, kiss_fft_cpx *);
/* Synthesize frequency-to-time output and update frame history; all_time_ac controls window mixing. */
void lhdc_dec_lossy_time_operation(void *, int *, int);
/* Query native-ABI workspace size, allowing only lossless zero. */
int lhdc_v5_dec_workspace_get_size(int, int, int, int, int *);
/* Initialize workspace using native structure sizes; callers must provide queried capacity. */
int lhdc_v5_dec_workspace_init(int, int, int, int, void *);
/* Build lossy parameters, segment tables, and synthesis state; validate supported internal rates/durations. */
int lhdc_dec_lossy_start(void *, int, int, int, int, int, int);
/* Validate public initialization parameters before internal lossy initialization. */
int lhdc_v5_dec_init(int, int, int, int, void *);
#endif
