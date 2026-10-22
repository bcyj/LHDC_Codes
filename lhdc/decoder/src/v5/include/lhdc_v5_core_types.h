/* V5 runtime state and shared buffers using native layout and common integer-complex types. */
#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_V5_CORE_TYPES_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_V5_CORE_TYPES_H
typedef unsigned char undefined;

typedef unsigned char byte;
typedef unsigned int dword;
typedef long long longlong;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef unsigned short ushort;
typedef unsigned short word;
typedef struct segment_setting_struct segment_setting_struct;

struct segment_setting_struct {
    float noise_fill_scale;
    int segment_num_inv;
    int initial_spectral_limit;
    int segment_num;
    int arith_init_0;
    int arith_init_1;
    int segment_offset[34];
    int segment_scale[34];
};
#include "../../../../common/include/dsp/fft_fixed32.h"
typedef LhdcFftFixed32Complex kiss_fft_cpx, *Pkiss_fft_cpx;

typedef struct tdata_all_buffer_struct tdata_all_buffer_struct;

typedef union LhdcV5SharedBufferPointers LhdcV5SharedBufferPointers;

typedef struct LhdcV5SymbolBufferPointers LhdcV5SymbolBufferPointers;

typedef struct time_buffer_struct time_buffer_struct;

typedef union LhdcV5TransformScratch LhdcV5TransformScratch;

typedef struct LhdcV5FftBufferPointers LhdcV5FftBufferPointers;

struct LhdcV5FftBufferPointers {
    int *time_data; /* Time-domain workspace shared with fft.tdata, not padding. */
    kiss_fft_cpx *fft_coef_in;
    kiss_fft_cpx *fft_coef_out;
};

struct LhdcV5SymbolBufferPointers {
    int *time_data;
    kiss_fft_cpx *fft_input;
    kiss_fft_cpx *fft_output;
    int *decoded_time_data;
    uint *tdata_q_quant_from_symbol;
    int *segment;
    uchar *lossy_jump_adus_parameter;
};

union LhdcV5TransformScratch {
    struct LhdcV5FftBufferPointers cpx;
    int *tdata;
};

struct time_buffer_struct {
    union LhdcV5TransformScratch fft;
    int *dec_tdata;
};

union LhdcV5SharedBufferPointers {
    struct LhdcV5SymbolBufferPointers bpc;
    struct time_buffer_struct time;
};

struct tdata_all_buffer_struct {
    int max_sample_rate;
    int max_frame_duration_dms;
    int lossless_maybe_enabled;
    int max_overlap_samples;
    int khz;
    int resolution;
    int ch_num;
    int sample_max;
    int sample_min;
    int frame_samples;
    int overlap_samples;
    int valid_num;
    int tdata_quant_rem_data_size;
    int ms;
    int channel_frame_bytes;
    int target_range;
    int lossless_if_en;
    int frame_cnt;
    struct segment_setting_struct segment_setting;
    int segment_cutoff;
    void *time_mem;
    float offset_table[512];
    int offset_table_start;
    float offset_table_jump;
    union LhdcV5SharedBufferPointers buf;
    uint offset_num;
    int offset_num_value_half;
    int init_fq_tbl_01[3];
    int init_fq_tbl_02[3];
    int power_of_2_tbl[161];
};

enum hdr_info_index_values {
    ENC_SIZE_INDEX = 0,
    VERSION_INDEX = 1,
    JAS_INDEX = 2,
    AR_INDEX = 3,
    LARC_INDEX = 4,
    META_INDEX = 5,
    ALL_HEADER_INFO_NUM = 6
};
typedef unsigned char hdr_info_index;

enum lhdc_v5_dec_workspace_content_index_values {
    PARAMETER_KHZ_MAX_INDEX = 0,
    PARAMETER_MS_MAX_INDEX = 1,
    PARAMETER_LOSSLESS_MAYBE_ENABLE_INDEX = 2,
    PARAMETER_MAX_FREQ_REPEATING_INDEX = 3,
    HDR_S_INDEX = 4,
    DBUFFER_INDEX = 5,
    TIME_MEM_S_FFT_CPX_COEF_IN_INDEX = 6,
    TIME_MEM_S_FFT_CPX_COEF_OUT_INDEX = 7,
    TIME_MEM_S_FFT_FDATA_INDEX = 8,
    TIME_MEM_S_DEC_TDATA_INDEX = 9,
    LEVEL_QUANT_2_SYMBOL_INDEX = 10,
    LEVEL_SEGMENT_INDEX = 11,
    LEVEL_JUMP_BINDATA_INDEX = 12,
    DBUFFER_LS_TDATA0_INDEX = 13,
    DBUFFER_LS_TDATA1_INDEX = 14,
    DBUFFER_FREQ_HEAP_INDEX = 15,
    FREQ_CH_INDEX = 16,
    FREQ_CH_FPREV_INDEX = 17,
    DEC_ARITH_S_INDEX = 18,
    INPUTTER_INDEX = 19,
    SEED_INDEX = 20,
    ENCODED_DATA_CH_INDEX = 21,
    CHANNEL_MODE_INDEX = 22,
    TIME_PRE_TWID_INDEX = 23,
    TIME_POS_TWID_INDEX = 24,
    TIME_WINDOW_INDEX = 25,
    FFT_TWIDDLE_INDEX = 26,
    ALL_CONTENT_NUM = 27
};
typedef unsigned char lhdc_v5_dec_workspace_content_index;

enum lhdc_v5_dec_error_values { LHDC_V5_DEC_ERROR = -1, LHDC_V5_DEC_OK = 0 };

#endif
