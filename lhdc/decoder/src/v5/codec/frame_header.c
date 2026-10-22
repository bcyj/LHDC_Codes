#include "../../../include/lhdc/decoder/status.h"
#include "../include/lhdc_dec.h"
#include <string.h>
#include "../include/workspace_directory_view.h"

/* Read an unaligned little-endian 16-bit header field. */
static unsigned read_header(unsigned char *encoded_frame) {
    return encoded_frame[0] | (unsigned)encoded_frame[1] << 8;
}

/* Extract ten-bit per-channel payload length and return header bytes including LARC/META extensions. */
int dec_process_header(unsigned char *encoded_frame, int *enc_frm_len) {
    unsigned hdr_info = read_header(encoded_frame);
    *enc_frm_len = (int)(hdr_info & 0x3ff);
    return 2 + ((hdr_info >> 14) & 1) * 2 + ((hdr_info >> 15) & 1) * 8;
}

/* Query a V5 native workspace directory member using the workspace-member index enum. */
void *lhdc_v5_dec_get_mem_content_addr(void *heap_memory,
                                       lhdc_v5_dec_workspace_content_index index) {
    const LHDCV5_DecWorkspaceDirectory *directory = heap_memory;
    switch (index) {
    case PARAMETER_KHZ_MAX_INDEX:
        return directory->parameter_khz_max;
    case PARAMETER_MS_MAX_INDEX:
        return directory->parameter_ms_max;
    case PARAMETER_LOSSLESS_MAYBE_ENABLE_INDEX:
        return directory->parameter_lossless_maybe_enable;
    case PARAMETER_MAX_FREQ_REPEATING_INDEX:
        return directory->parameter_max_freq_repeating;
    case DBUFFER_INDEX:
        return directory->dbuffer;
    case TIME_MEM_S_FFT_CPX_COEF_IN_INDEX:
        return directory->time_mem_s_fft_cpx_coef_in;
    case TIME_MEM_S_FFT_CPX_COEF_OUT_INDEX:
        return directory->time_mem_s_fft_cpx_coef_out;
    case TIME_MEM_S_FFT_FDATA_INDEX:
        return directory->time_mem_s_fft_fdata;
    case TIME_MEM_S_DEC_TDATA_INDEX:
        return directory->time_mem_s_dec_tdata;
    case LEVEL_QUANT_2_SYMBOL_INDEX:
        return directory->level_quant_2_symbol;
    case LEVEL_SEGMENT_INDEX:
        return directory->level_segment;
    case LEVEL_JUMP_BINDATA_INDEX:
        return directory->level_jump_bindata;
    case DBUFFER_FREQ_HEAP_INDEX:
        return directory->dbuffer_freq_heap;
    case FREQ_CH_INDEX:
        return directory->freq_ch;
    case FREQ_CH_FPREV_INDEX:
        return directory->freq_ch_fprev;
    case DEC_ARITH_S_INDEX:
        return directory->dec_arith_s;
    case INPUTTER_INDEX:
        return directory->inputter;
    case SEED_INDEX:
        return directory->seed;
    case ENCODED_DATA_CH_INDEX:
        return directory->encoded_data_ch;
    case CHANNEL_MODE_INDEX:
        return directory->channel_mode;
    case TIME_PRE_TWID_INDEX:
        return directory->time_pre_twid;
    case TIME_POS_TWID_INDEX:
        return directory->time_pos_twid;
    case TIME_WINDOW_INDEX:
        return directory->time_window;
    case FFT_TWIDDLE_INDEX:
        return directory->fft_twiddle;
    default:
        return NULL;
    }
}

/* Return complete frame bytes: header length plus channel count times per-channel payload length. */
int lhdc_v5_dec_get_encoded_frame_size(unsigned char *encoded_frame, int *encoded_frame_size,
                                       void *workspace) {
    if (!encoded_frame || !encoded_frame_size || !workspace)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    tdata_all_buffer_struct *dcb = lhdc_v5_dec_get_mem_content_addr(workspace, DBUFFER_INDEX);
    int enc_frm_len;
    int hdr_size = dec_process_header(encoded_frame, &enc_frm_len);
    *encoded_frame_size = dcb->ch_num * enc_frm_len + hdr_size;
    return LHDC_DEC_OK;
}

/* Read the META flag and copy eight metadata bytes sequentially, supporting overlapping input/output. */
int lhdc_v5_dec_get_meta_flag_and_data(unsigned char *encoded_frame, int *meta_flag,
                                       unsigned char *meta_data) {
    if (!encoded_frame || !meta_flag || !meta_data)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    unsigned hdr_info = read_header(encoded_frame);
    *meta_flag = (hdr_info >> 15) & 1;
    if (*meta_flag) {
        unsigned char *encoded_frame_meta = encoded_frame + 2 + ((hdr_info >> 14) & 1) * 2;
        if ((uintptr_t)meta_data - ((uintptr_t)encoded_frame_meta + 1) < 3) {
            for (int i = 0; i < 8; ++i)
                meta_data[i] = encoded_frame_meta[i];
        } else {
            uint32_t word;
            memcpy(&word, encoded_frame_meta, 4);
            memcpy(meta_data, &word, 4);
            memcpy(&word, encoded_frame_meta + 4, 4);
            memcpy(meta_data + 4, &word, 4);
        }
    }
    return LHDC_DEC_OK;
}
