#include "../../../../common/include/lhdc_alignment.h"
#include "../../../include/lhdc/decoder/status.h"
#include "../include/workspace_directory_view.h"
#include "../include/lhdc_fft.h"
#include "../include/lhdc_workspace_layout.h"
#include "../include/lhdc_tables.h"
#include <string.h>

/* Build lossy decoder state; null workspace queries capacity without checking other parameters.
 * Reject ms=25/75/100 and khz=8000/16000/24000/32000; other values continue initialization.
 * This entry does not validate channel range; callers must use public validation and provide queried capacity.
 */
int lhdc_dec_lossy_start(void *heap_memory, int heap_memory_size, int ch_num, int resolution,
                         int khz, int ms, int channel_frame_bytes) {
    LHDCV5_DecWorkspaceDirectory *directory = heap_memory;
    int state_bytes = (int)(sizeof(tdata_all_buffer_struct) + sizeof(LHDCV5_TimeWorkspace));
    if (!heap_memory)
        return state_bytes;
    if (heap_memory_size < state_bytes || ms == 25 || ms == 75 || ms == 100 || khz == 8000 ||
        khz == 16000 || khz == 24000 || khz == 32000 || ((unsigned)resolution & ~8u) != 16)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    tdata_all_buffer_struct *tdata_all_buffer = directory->dbuffer;
    memset(tdata_all_buffer, 0, sizeof *tdata_all_buffer);
    tdata_all_buffer->max_sample_rate = *directory->parameter_khz_max;
    tdata_all_buffer->max_frame_duration_dms = *directory->parameter_ms_max;
    if (khz > tdata_all_buffer->max_sample_rate || ms > tdata_all_buffer->max_frame_duration_dms)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    tdata_all_buffer->lossless_maybe_enabled = *directory->parameter_lossless_maybe_enable;
    tdata_all_buffer->max_overlap_samples = *directory->parameter_max_freq_repeating;
    tdata_all_buffer->buf.bpc.fft_input = directory->time_mem_s_fft_cpx_coef_in;
    tdata_all_buffer->buf.bpc.fft_output = directory->time_mem_s_fft_cpx_coef_out;
    tdata_all_buffer->buf.bpc.time_data = directory->time_mem_s_fft_fdata;
    tdata_all_buffer->buf.bpc.decoded_time_data = directory->time_mem_s_dec_tdata;
    tdata_all_buffer->buf.bpc.tdata_q_quant_from_symbol = directory->level_quant_2_symbol;
    tdata_all_buffer->buf.bpc.segment = directory->level_segment;
    tdata_all_buffer->buf.bpc.lossy_jump_adus_parameter = directory->level_jump_bindata;
    tdata_all_buffer->resolution = resolution;
    tdata_all_buffer->ch_num = ch_num;
    tdata_all_buffer->khz = khz;
    tdata_all_buffer->ms = ms;
    tdata_all_buffer->offset_num_value_half = (int)(1u << (30 - resolution));
    tdata_all_buffer->offset_num = 31 - (unsigned)resolution;
    tdata_all_buffer->sample_max = (int)((1u << (resolution - 1)) - 1);
    tdata_all_buffer->sample_min = -(int)(1u << (resolution - 1));
    int repeat = ms == 75 || ms == 100 ? 25 : ms == 50 ? 20 : 0;
    if (khz == 44100) {
        khz = 48000;
        tdata_all_buffer->overlap_samples = ms == 50 ? 96 : ms == 75 || ms == 100 ? 120 : 0;
    } else
        tdata_all_buffer->overlap_samples = khz * repeat / 10000;
    tdata_all_buffer->frame_samples = ms * khz / 10000;
    unsigned n = (unsigned)tdata_all_buffer->frame_samples;
    int bits = -1;
    while (n) {
        ++bits;
        n >>= 1;
    }
    tdata_all_buffer->valid_num = bits;
    if (lhdc_dec_lossy_frame_length_program(tdata_all_buffer, channel_frame_bytes) < 0)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    tdata_all_buffer->time_mem = directory->dbuffer_freq_heap;
    lhdc_dec_lossy_time_table_start(tdata_all_buffer->time_mem, tdata_all_buffer->frame_samples * 2,
                                    tdata_all_buffer->overlap_samples, 31 - resolution,
                                    &tdata_all_buffer->buf.time, directory->time_pre_twid,
                                    directory->time_pos_twid, directory->time_window,
                                    directory->fft_twiddle);
    ((time_process_top_struct *)tdata_all_buffer->time_mem)->time_process_ch_num =
        directory->freq_ch;
    *directory->seed = 0x607f;
    tdata_all_buffer->frame_cnt = 0;
    return state_bytes;
}

/* Public initialization accepts eight sample rates and four durations; unsupported internal parameters return -1. */
int lhdc_v5_dec_init(int channel, int resolution, int khz, int ms, void *heap_memory) {
    LHDCV5_DecWorkspaceDirectory *directory = heap_memory;
    if ((unsigned)(channel - 1) >= 8 || ((unsigned)resolution & ~8u) != 16 || !heap_memory ||
        !(khz == 8000 || khz == 16000 || khz == 24000 || khz == 32000 || khz == 44100 ||
          khz == 48000 || khz == 96000 || khz == 192000) ||
        !(ms == 25 || ms == 50 || ms == 75 || ms == 100))
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    int *stereo_mono = directory->channel_mode;
    if (channel == 1)
        *stereo_mono = 0;
    return lhdc_dec_lossy_start(heap_memory, 99999999, channel, resolution, khz, ms, 2000) < 0
               ? LHDC_V5_DEC_INVALID_ARGUMENT
               : LHDC_DEC_OK;
}
#include "../include/lhdc_dec_control.h"
#include "../include/lhdc_workspace_layout.h"
#include <stdint.h>
#include <string.h>


/* Calculate workspace by channels, sample rate, and duration, optionally initializing caller memory. */
static int configure_workspace(int channels, int hz, int ms, unsigned char *heap) {
    if (hz == 44100)
        hz = 48000;
    unsigned frame = (unsigned)(ms * hz / 10000);
    unsigned repeat = ms == 100 || ms == 75 ? (unsigned)hz / 400
                                            : (ms == 50 ? (unsigned)hz / 500 : (unsigned)hz / 1000);
    unsigned coef_in = frame / 2 * 8, coef_out = frame == 20 ? 160 : coef_in;
    unsigned fft = frame == 20 ? 320 : frame * 8;
    unsigned quant = lhdc_align_size(frame * 4, 8);
    /* The 20-point path reserves 160+320 bytes, including 80 extra scratch bytes. */
    unsigned buffer = frame == 20 ? 480 : lhdc_align_size(quant + fft, 8);
    if (buffer < lhdc_align_size(quant + 160, 8))
        buffer = lhdc_align_size(quant + 160, 8);
    unsigned time = sizeof(LHDCV5_TimeWorkspace),
             history = lhdc_align_size(repeat * (unsigned)channels * 4, 8),
             window = lhdc_align_size(repeat * 4, 8);
    unsigned channel_state = 2 * sizeof(time_process_struct);
    unsigned total = sizeof(LHDCV5_WorkspacePrefix) + buffer + time + channel_state + history +
                     sizeof(LHDCV5_ArithmeticWorkspace) + coef_in * 3 + window;
    if (heap) {
        memset(heap, 0, total);
        LHDCV5_WorkspacePrefix *workspace = (void *)heap;
        LHDCV5_DecWorkspaceDirectory *directory = &workspace->directory;
        unsigned data = sizeof(*workspace), frequency = data + buffer, ch = frequency + time,
                 previous = ch + channel_state;
        unsigned arithmetic = previous + history,
                 pre = arithmetic + sizeof(LHDCV5_ArithmeticWorkspace), pos = pre + coef_in,
                 win = pos + coef_in;
        void *arithmetic_memory = heap + arithmetic;
        LHDCV5_ArithmeticWorkspace *arithmetic_workspace = arithmetic_memory;
        directory->parameter_khz_max = &workspace->parameter_khz_max;
        directory->parameter_ms_max = &workspace->parameter_ms_max;
        directory->parameter_lossless_maybe_enable = &workspace->parameter_lossless_maybe_enable;
        directory->parameter_max_freq_repeating = &workspace->parameter_max_freq_repeating;
        directory->dbuffer = &workspace->dbuffer;
        directory->time_mem_s_fft_cpx_coef_in = heap + data;
        directory->time_mem_s_fft_cpx_coef_out = heap + data + coef_out;
        directory->time_mem_s_fft_fdata = heap + data;
        directory->time_mem_s_dec_tdata = heap + data + fft;
        directory->level_quant_2_symbol = heap + data;
        directory->level_segment = heap + data + quant;
        directory->level_jump_bindata = heap + data + quant + 128;
        directory->dbuffer_freq_heap = heap + frequency;
        directory->freq_ch = heap + ch;
        directory->freq_ch_fprev = heap + previous;
        directory->dec_arith_s = &arithmetic_workspace->dec_arith_s;
        directory->inputter = &arithmetic_workspace->inputter;
        directory->seed = &arithmetic_workspace->seed;
        directory->encoded_data_ch = arithmetic_workspace->encoded_data_ch;
        directory->channel_mode = &arithmetic_workspace->channel_mode;
        directory->time_pre_twid = heap + pre;
        directory->time_pos_twid = heap + pos;
        directory->time_window = heap + win;
        directory->fft_twiddle = heap + win + window;
        workspace->parameter_khz_max = hz;
        workspace->parameter_ms_max = ms;
        workspace->parameter_lossless_maybe_enable = 0;
        workspace->parameter_max_freq_repeating = (int)repeat;
    }
    return (int)total;
}

/* Validate channels, sample rate, frame length, and lossless flag supported by the API. */
static int valid_workspace(int ch, int hz, int ms, int lossless) {
    return (unsigned)(ch - 1) < 8 &&
           (hz == 8000 || hz == 16000 || hz == 24000 || hz == 32000 || hz == 44100 || hz == 48000 ||
            hz == 96000 || hz == 192000) &&
           (ms == 25 || ms == 50 || ms == 75 || ms == 100) && lossless == 0;
}

/* Validate parameters and query native-ABI workspace size; failures leave size unchanged. */
int lhdc_v5_dec_workspace_get_size(int ch, int khz_max, int ms_max, int lossless_maybe_enabled,
                                   int *size) {
    if (!size || !valid_workspace(ch, khz_max, ms_max, lossless_maybe_enabled))
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    *size = configure_workspace(ch, khz_max, ms_max, NULL);
    return LHDC_DEC_OK;
}

/* Validate parameters, clear workspace, and build native pointer directories/parameter slots. */
int lhdc_v5_dec_workspace_init(int ch, int khz_max, int ms_max, int lossless_maybe_enabled,
                               void *heap_memory) {
    if (!heap_memory || !valid_workspace(ch, khz_max, ms_max, lossless_maybe_enabled))
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    (void)configure_workspace(ch, khz_max, ms_max, heap_memory);
    return LHDC_DEC_OK;
}
#include "../include/lhdc_dec_control.h"
#include "../include/lhdc_workspace_layout.h"

/* Retrieve internal decoder state from the workspace directory. */
static tdata_all_buffer_struct *decoder_state(void *heap) {
    return lhdc_v5_dec_get_mem_content_addr(heap, DBUFFER_INDEX);
}

/* Validate output/workspace and return frame sample count. */
int lhdc_v5_dec_get_samples_per_frame(int *s_fps, void *heap_memory) {
    if (!s_fps || !heap_memory)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    *s_fps = decoder_state(heap_memory)->frame_samples;
    return LHDC_DEC_OK;
}

/* Validate and store channel mode; mono ignores the requested mode and stores zero. */
int lhdc_v5_dec_set_channel_mode(int ch_option, void *heap_memory) {
    if ((unsigned)ch_option > 2 || !heap_memory)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    int *stereo_mono = lhdc_v5_dec_get_mem_content_addr(heap_memory, CHANNEL_MODE_INDEX);
    *stereo_mono = decoder_state(heap_memory)->ch_num == 1 ? 0 : ch_option;
    return LHDC_DEC_OK;
}

