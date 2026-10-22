#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H
#include "cirbuf.h"
#include "../../codec/lzh_encode.h"
#include "../codec/lpc_analysis.h"
#include "../llac/llac_code_types.h"
#include "../codec/encoder.h"
#include "../codec/resample_state.h"
#include "../../../../include/lhdc/encoder/v4/lhdc_api.h"
#include "../../../../include/lhdc/encoder/v4/llac_enc_api.h"
#include "../../../../../common/include/types/lhdc_dsp_types.h"
#include "../../../../../common/include/types/llac_side_types.h"
#include "../../../../../common/include/compat/v4/lhdc_v4_constants.h"

typedef struct kiss_fft_state_v4 kiss_fft_state_v4, *Pkiss_fft_state_v4;

typedef struct kiss_fft_state_v4 *kiss_fft_cfg_v4;

struct kiss_fft_state_v4 {
    int32_t nfft;
    int32_t inverse;
    int32_t factors[64];
    AudioMathComplex32 twiddles[]; /* Dynamically sized trailing twiddle table. */
};

typedef struct kiss_fftr_state kiss_fftr_state;

typedef struct kiss_fftr_state *kiss_fftr_cfg;

struct kiss_fftr_state {
    kiss_fft_cfg_v4 substate;
    AudioMathComplex32 *tmpbuf;
    AudioMathComplex32 *super_twiddles;
    kiss_fft_cpx *sample_pairs;
};

typedef LhdcV4QualityPreset QUALITY_PRESET;

typedef struct lhdc_Encoder_s lhdc_Encoder_s;

struct lhdc_Encoder_s {
    int32_t bits_per_sample;
    int32_t bytes_per_sample;
    int32_t channel;
    int32_t block_size;
    int32_t chunk_size;
    int32_t chunk_cnt;
    int32_t one_frame_per_channle;
    int32_t max;
    int32_t min;
    uint32_t frames;
    int32_t compression_type[2];
    int32_t use_side;
    int32_t dat[2][512];
    int32_t side[512];
    int32_t residual[2][14][512];
    int32_t side_residual[14][512];
    int32_t lz_evl_cnt;
    int32_t use_lz[2];
    int32_t *rd[2];
    int32_t num_unencoded[2];
    uint32_t sh_rice[2];
    uint32_t sh_lz[2];
    uint32_t max_nb[2];
    int32_t divisor[2];
    int32_t divisor_ext[2];
    uint32_t rd2[2][512];
    uint8_t cp[2][2048];
    uint8_t uc[2][2048];
    /* LPC analysis precedes LZH; LZH rebuilds its dictionary each call, so neither phase retains shared contents. */
    union {
        LhdcV4LzhWorkspace lzh_workspace;
        LhdcV4LpcWorkspace lpc_workspace;
    };
    float pfLpcVWindow[512];
    LhdcV4LpcResult lpc_result[2];
    LhdcV4LpcResult side_lpc_result;
    int32_t lhdc_ver_code;
    uint32_t cnt_encode_type[2][16];
    uint32_t cnt_side[3];
    uint32_t cnt_lz;
    uint8_t extention_flag;
    uint8_t ext_offset;
    uint8_t ext_func;
    uint8_t metadata_len;
    uint8_t metadata[8];
    uint32_t meta_frame_cnt;
};

struct FFT_block_s {
    int32_t fft_len;
    int32_t input_block_size;
    int32_t enc_block_size;
    int32_t sample_rate;
    int32_t bits_per_sample;
    int32_t ch;
    int32_t brc_block_oldest;
    int32_t brc_block_num;
    int32_t brc_block_len[16];
    int32_t brc_target_block_len;
    int32_t brc_target_block_len_up_limit;
    int32_t brc_target_block_len_low_limit;
    int32_t brc_target_block_mtu_limit;
    int32_t min_bits_reserved;
    int32_t lhdc_ver_code;
    int32_t brc_sink_buf_len;
    int32_t brc_total_len;
    int32_t brc_target_len;
    int32_t brc_quality_up_threshold;
    int32_t brc_quality_up_cnt;
    int32_t no_btr_limit_frame_cnt;
    int32_t down_step;
    int32_t min_btr_down_sample;
    int32_t min_block_len;
    LhdcV4QualityPresets quality_presets;
    int32_t quality;
    float avg_power_shift;
    float noise_floor_shift;
    float min_bits_rms;
    float min_rms;
    float min_noise_floor;
    int32_t upper_freq;
    int32_t bin_low;
    int32_t bin_hi;
    int32_t bin_11025;
    float avgMS[2];
    float *dat[2];
    float dat_b[2][528];
    float fft_result[258];
    float fft_freq_factor[257];
    int32_t g_pcm[2][512];
    int32_t *pcm[2];
    uint32_t out_buf[3088];
    int32_t shift_blk_cnt;
    float HannWindow[512];
    float fft_in[514];
    AudioMathComplex32 fft_out[512];
    struct kiss_fftr_state *cfg;
    size_t cfg_len;
    struct lhdc_Encoder_s *encb;
    float *weighting_filter;
    int32_t btr[2];
    int32_t btr_ext[2];
    int32_t last_div[2];
    int32_t block_num;
    float total_div;
    int32_t quality_count[16];
    int32_t use_avg_num[16];
    int32_t limit_by_RMS[16];
    LhdcV4ResampleState resample;
};

typedef struct EncSetup EncSetup;

typedef struct Mdct Mdct;

typedef struct Dct2 Dct2;

typedef struct Dct4 Dct4;

typedef struct Fft Fft;

typedef struct Complex Complex;

struct Fft {
    LLAC_INT length;
    void *handle;
};

struct Dct4 {
    /* N-point DCT-IV with fft.length=N/2; twid1/2 are pre/post rotations. */
    LLAC_INT length;
    struct Complex *twid1;
    struct Complex *twid2;
    struct Fft fft;
};

struct Mdct {
    /* length=N counts frame samples/output coefficients; window has 2N entries and mem stores unwindowed history. */
    LLAC_INT length;
    LLAC_INT leading_zeros;
    LLAC_INT mem_length;

    const LLAC_FLOAT *window;
    LLAC_FLOAT *mem;
    struct Dct4 dct;
};

struct Dct2 {
    LLAC_INT length;
    struct Fft fft;
};

struct EncSetup {
    LLAC_FLOAT targetBitsOff;
    LLAC_FLOAT attdec_filter_mem[2];
    LLAC_FLOAT attdec_acc_energy;
    LLAC_FLOAT s_in_scaled[960];
    LLAC_FLOAT ener[64];
    LLAC_FLOAT scf_q[16];
    LLAC_FLOAT scf[16];
    LLAC_FLOAT int_scf[64];
    LLAC_INT targetBytes;
    LLAC_INT total_bits;
    LLAC_INT targetBitsInit;
    LLAC_INT targetBitsAri;
    LLAC_INT enable_lpc_weighting;
    LLAC_INT quantizedGainOff;
    LLAC_INT tns_bits;
    LLAC_INT targetBitsQuant;
    LLAC_INT mem_targetBits;
    LLAC_INT mem_specBits;
    LLAC_INT attack_handling;
    LLAC_INT attdec_detected;
    LLAC_INT attdec_position;
    LLACSnsIndices sns_indices;
    LlacSpectralPairCode spectral_pair_codes[960];
    LLAC_UINT8 resBits[625];
    LLAC_INT regBits;
    struct Mdct mdctStruct;
    struct Dct2 dct2StructSNS;
};

struct Complex {
    LLAC_FLOAT r;
    LLAC_FLOAT i;
};

typedef struct LLAC_Enc LLAC_Enc;

/* Temporary TNS correlation, prediction, and lattice state expires before gain/spectral quantization. */
typedef struct LhdcV4LlacTnsWorkspace {
    LLAC_FLOAT filter_state[LHDC_V4_LLAC_TNS_MAX_ORDER + 1];
    LLAC_FLOAT stage[LHDC_V4_LLAC_TNS_MAX_ORDER + 1];
    LLAC_FLOAT reflections[LHDC_V4_LLAC_TNS_MAX_ORDER + 1];
    LLAC_FLOAT polynomial[LHDC_V4_LLAC_TNS_MAX_ORDER + 1];
    LLAC_FLOAT reversed[LHDC_V4_LLAC_TNS_MAX_ORDER + 1];
    LLAC_INT reflection_indices[LHDC_V4_LLAC_TNS_MAX_ORDER];
} LhdcV4LlacTnsWorkspace;

/* Channels reuse storage sequentially; spectrum and quantized output must coexist until entropy coding finishes. */
typedef struct LhdcV4LlacFrameWorkspace {
    LLAC_FLOAT spectrum[LHDC_V4_LLAC_MAX_FRAME_SAMPLES];
    /* TNS, gain energy, and final quantized spectrum reuse one area by stage without enlarging instances. */
    union {
        LLAC_INT quantized_spectrum[LHDC_V4_LLAC_MAX_FRAME_SAMPLES];
        LLAC_FLOAT gain_energy[LHDC_V4_LLAC_MAX_FRAME_SAMPLES / 4];
        LhdcV4LlacTnsWorkspace tns;
    };
} LhdcV4LlacFrameWorkspace;

struct LLAC_Enc {
    struct EncSetup *channel_setup[2];

    const LLAC_INT *W_fx;

    const LLAC_INT *bands_offset;

    const LLAC_INT *cutoffBins;
    LLAC_INT fs;
    LLAC_INT fs_in;
    LLAC_INT bitrate;
    LLAC_INT fs_idx;
    LLAC_INT frame_length;
    LLAC_INT channels;
    LLAC_INT epmode;
    LLAC_FLOAT frame_ms;
    LLAC_INT frame_dms;
    LLAC_INT tilt;
    LLAC_INT llac_br_set;
    LLAC_INT yLen;
    LLAC_INT W_size;
    LLAC_INT la_zeroes;
    LLAC_INT stEnc_mdct_mem_len;
    LLAC_INT bands_number;
    LLAC_INT nSubdivisions;
    LLAC_INT envelope_bits;
    LLAC_INT global_gain_bits;
    LLAC_INT noise_fac_bits;
    LLAC_INT BW_cutoff_bits;
    LLAC_INT hrmode;
    LLAC_INT bandwidth;
    LLAC_INT bw_index;
    LLAC_FLOAT sns_damping;
    LhdcV4LlacFrameWorkspace frame_workspace;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* Selected at initialization and fixed for instance lifetime; no mutable cache sharing between instances. */
    unsigned math_policy;
#endif
};

enum {
    SAMPLERATE_INDEX = 0,
    NCHANNELS_INDEX = 1,
    BPS_IN_INDEX = 2,
    BPS_OUT_INDEX = 3,
    FRAME_MS_INDEX = 4,
    NBYTES_INDEX = 5,
    FRAME_LENGTH_INDEX = 6,
    TARGET_BITRATE_INDEX = 7,
    REAL_BITRATE_INDEX = 8,
    EXTRA_NBYTE_INDEX = 9,
    EXTRA_NBYTE_NUM_INDEX = 10,
    BUF_16_INDEX = 11,
    PARAM_NUM = 11,
    ENCODER_INDEX = 12,
    DCT_TWID1_INDEX = 13,
    DCT_TWID2_INDEX = 14,
    MDCT_MEM_CH0_INDEX = 15,
    MDCT_MEM_CH1_INDEX = 16,
    MDCT_MEM_INDEX = 17,
    HANDLE_TABLE_CH0_DCT2_INDEX = 18,
    HANDLE_TABLE_CH0_DCT4_INDEX = 19,
    HANDLE_TABLE_CH1_DCT2_INDEX = 20,
    HANDLE_TABLE_CH1_DCT4_INDEX = 21,
    HANDLE_TABLE_INDEX = 22,
    H_CH0_DCT2_INDEX = 23,
    H_CH0_DCT4_INDEX = 24,
    H_CH1_DCT2_INDEX = 25,
    H_CH1_DCT4_INDEX = 26,
    H_INDEX = 27,
    H_BUFFER_CH0_DCT2_INDEX = 28,
    H_BUFFER_CH0_DCT4_INDEX = 29,
    H_BUFFER_CH1_DCT2_INDEX = 30,
    H_BUFFER_CH1_DCT4_INDEX = 31,
    H_BUFFER_INDEX = 32,
    HANDLE_SCRATCH_CH0_DCT2_INDEX = 33,
    HANDLE_SCRATCH_CH0_DCT4_INDEX = 34,
    HANDLE_SCRATCH_CH1_DCT2_INDEX = 35,
    HANDLE_SCRATCH_CH1_DCT4_INDEX = 36,
    HANDLE_SCRATCH_INDEX = 37,
    HANDLE_SCRATCH2_CH0_DCT2_INDEX = 38,
    HANDLE_SCRATCH2_CH0_DCT4_INDEX = 39,
    HANDLE_SCRATCH2_CH1_DCT2_INDEX = 40,
    HANDLE_SCRATCH2_CH1_DCT4_INDEX = 41,
    HANDLE_SCRATCH2_INDEX = 42,
    LLAC_MEM_CONTENT_INDEX_ALL_CONTENT_NUM = 43
};

typedef enum dct_type { DCT2 = 0, DCT4 = 1, DCT_NUM = 2 } dct_type;

typedef struct bit_write_s bit_write_s;

typedef union anon_union_4_2_c9610c8e_for_db anon_union_4_2_c9610c8e_for_db;

union anon_union_4_2_c9610c8e_for_db {
    uint32_t u32;
    uint8_t u8[4];
};

struct bit_write_s {
    uint32_t rb;
    union anon_union_4_2_c9610c8e_for_db db;
};

typedef union anon_union_4_2_c95b981a anon_union_4_2_c95b981a;

union anon_union_4_2_c95b981a {
    int32_t i32;
    uint8_t u8[4];
};

typedef struct T_IIS_FFT T_IIS_FFT;

typedef struct T_IIS_FFT *HANDLE_IIS_FFT;

typedef enum IIS_FFT_DIR { IIS_FFT_FWD = -1, IIS_FFT_BWD = 1 } IIS_FFT_DIR;

typedef struct Iisfft Iisfft;

typedef struct Cfft Cfft;

struct Cfft {
    LLAC_INT len;
    LLAC_INT sign;
    LLAC_FLOAT *table;
};

struct Iisfft {
    LLAC_FLOAT *scratch;
    LLAC_INT *scratch2;
    LLAC_INT length;
    LLAC_INT sign;
    LLAC_INT num_factors;
    LLAC_INT factors[10];
    LLAC_INT isPrime[10];
};

/* FFT butterflies may overwrite the completed MDCT analysis block; the phases never read it concurrently. */
typedef union LHDCV4_LLACFftScratch {
    LLAC_FLOAT analysis[2048];
    kiss_fft_cpx radix[1024];
} LHDCV4_LLACFftScratch;

struct T_IIS_FFT {
    enum IIS_FFT_DIR sign;
    LLAC_INT len;
    LHDCV4_LLACFftScratch *scratch;
    LLAC_FLOAT *sine_table;
    struct Iisfft iisfft;
    struct Cfft cfft;
    /* Common backend plan with LLAC-instance-allocated workspace, not independent heap objects. */
    int32_t kiss_factors[64];
    kiss_fft_cpx *kiss_twiddles;
    kiss_fft_cpx *kiss_input;
    kiss_fft_cpx *kiss_output;
};

typedef enum IIS_FFT_ERROR {
    IIS_FFT_NO_ERROR = 0,
    IIS_FFT_INTERNAL_ERROR = 1,
    IIS_FFT_LENGTH_ERROR = 2,
    IIS_FFT_MEMORY_ERROR = 3
} IIS_FFT_ERROR;

typedef struct Encoder_State_fl Encoder_State_fl;

struct Encoder_State_fl {
    LLAC_INT bp;
    LLAC_INT low;
    LLAC_INT range;
    LLAC_INT cache;
    LLAC_INT carry;
    LLAC_INT carry_count;
    LLAC_UINT8 *ptr;
    LLAC_INT *bp_side;
    LLAC_INT *mask_side;
};
#include "../../../../include/lhdc/encoder/v4/llac_status.h"

/* Analysis view: llac_mem_content_index defines member order; size varies with pointer width. */
typedef struct LHDCV4_FftDirectoryGroup {
    void *by_channel_dct[2][2];
    void *active;
} LHDCV4_FftDirectoryGroup;
typedef struct LHDCV4_LlacWorkspaceParameters {
    uint32_t sample_rate;
    int16_t nchannels, bps_in, bps_out;
    LLAC_FLOAT frame_ms;
    LLAC_INT nbytes;
    uint32_t frame_length;
    LLAC_INT target_bitrate, real_bitrate;
    uint16_t extra_nbyte;
    LLAC_UINT8 extra_nbyte_num;
} LHDCV4_LlacWorkspaceParameters;
typedef struct LHDCV4_LLACWorkspaceDirectory {
    uint32_t *sample_rate;
    int16_t *nchannels;
    int16_t *bps_in;
    int16_t *bps_out;
    LLAC_FLOAT *frame_ms;
    LLAC_INT *nbytes;
    uint32_t *frame_length;
    LLAC_INT *target_bitrate;
    LLAC_INT *real_bitrate;
    uint16_t *extra_nbyte;
    LLAC_UINT8 *extra_nbyte_num;
    int16_t *buf16;
    LLAC_Enc *encoder;
    void *dct_twid1;
    void *dct_twid2;
    void *mdct_mem_ch[2];
    void *mdct_mem;
    LHDCV4_FftDirectoryGroup handle_table;
    LHDCV4_FftDirectoryGroup h;
    LHDCV4_FftDirectoryGroup h_buffer;
    LHDCV4_FftDirectoryGroup handle_scratch;
    LHDCV4_FftDirectoryGroup handle_scratch2;
} LHDCV4_LLACWorkspaceDirectory;
_Static_assert(sizeof(LHDCV4_LLACWorkspaceDirectory) == 43 * sizeof(void *), "LLAC directory");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, sample_rate) == 0 * sizeof(void *),
               "LLAC slot 0");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, nchannels) == 1 * sizeof(void *),
               "LLAC slot 1");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, bps_in) == 2 * sizeof(void *),
               "LLAC slot 2");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, bps_out) == 3 * sizeof(void *),
               "LLAC slot 3");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, frame_ms) == 4 * sizeof(void *),
               "LLAC slot 4");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, nbytes) == 5 * sizeof(void *),
               "LLAC slot 5");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, frame_length) == 6 * sizeof(void *),
               "LLAC slot 6");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, target_bitrate) == 7 * sizeof(void *),
               "LLAC slot 7");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, real_bitrate) == 8 * sizeof(void *),
               "LLAC slot 8");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, extra_nbyte) == 9 * sizeof(void *),
               "LLAC slot 9");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, extra_nbyte_num) == 10 * sizeof(void *),
               "LLAC slot 10");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, buf16) == 11 * sizeof(void *),
               "LLAC slot 11");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, encoder) == 12 * sizeof(void *),
               "LLAC slot 12");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, dct_twid1) == 13 * sizeof(void *),
               "LLAC slot 13");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, dct_twid2) == 14 * sizeof(void *),
               "LLAC slot 14");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, mdct_mem_ch) + 0 * sizeof(void *)) ==
                   15 * sizeof(void *),
               "LLAC slot 15");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, mdct_mem_ch) + 1 * sizeof(void *)) ==
                   16 * sizeof(void *),
               "LLAC slot 16");
_Static_assert(offsetof(LHDCV4_LLACWorkspaceDirectory, mdct_mem) == 17 * sizeof(void *),
               "LLAC slot 17");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_table) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 0 * sizeof(void *)) ==
                   18 * sizeof(void *),
               "LLAC slot 18");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_table) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 1 * sizeof(void *)) ==
                   19 * sizeof(void *),
               "LLAC slot 19");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_table) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 2 * sizeof(void *)) ==
                   20 * sizeof(void *),
               "LLAC slot 20");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_table) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 3 * sizeof(void *)) ==
                   21 * sizeof(void *),
               "LLAC slot 21");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_table) +
                offsetof(LHDCV4_FftDirectoryGroup, active)) == 22 * sizeof(void *),
               "LLAC slot 22");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 0 * sizeof(void *)) ==
                   23 * sizeof(void *),
               "LLAC slot 23");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 1 * sizeof(void *)) ==
                   24 * sizeof(void *),
               "LLAC slot 24");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 2 * sizeof(void *)) ==
                   25 * sizeof(void *),
               "LLAC slot 25");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 3 * sizeof(void *)) ==
                   26 * sizeof(void *),
               "LLAC slot 26");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h) +
                offsetof(LHDCV4_FftDirectoryGroup, active)) == 27 * sizeof(void *),
               "LLAC slot 27");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h_buffer) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 0 * sizeof(void *)) ==
                   28 * sizeof(void *),
               "LLAC slot 28");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h_buffer) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 1 * sizeof(void *)) ==
                   29 * sizeof(void *),
               "LLAC slot 29");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h_buffer) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 2 * sizeof(void *)) ==
                   30 * sizeof(void *),
               "LLAC slot 30");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h_buffer) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 3 * sizeof(void *)) ==
                   31 * sizeof(void *),
               "LLAC slot 31");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, h_buffer) +
                offsetof(LHDCV4_FftDirectoryGroup, active)) == 32 * sizeof(void *),
               "LLAC slot 32");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 0 * sizeof(void *)) ==
                   33 * sizeof(void *),
               "LLAC slot 33");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 1 * sizeof(void *)) ==
                   34 * sizeof(void *),
               "LLAC slot 34");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 2 * sizeof(void *)) ==
                   35 * sizeof(void *),
               "LLAC slot 35");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 3 * sizeof(void *)) ==
                   36 * sizeof(void *),
               "LLAC slot 36");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch) +
                offsetof(LHDCV4_FftDirectoryGroup, active)) == 37 * sizeof(void *),
               "LLAC slot 37");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch2) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 0 * sizeof(void *)) ==
                   38 * sizeof(void *),
               "LLAC slot 38");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch2) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 1 * sizeof(void *)) ==
                   39 * sizeof(void *),
               "LLAC slot 39");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch2) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 2 * sizeof(void *)) ==
                   40 * sizeof(void *),
               "LLAC slot 40");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch2) +
                offsetof(LHDCV4_FftDirectoryGroup, by_channel_dct) + 3 * sizeof(void *)) ==
                   41 * sizeof(void *),
               "LLAC slot 41");
_Static_assert((offsetof(LHDCV4_LLACWorkspaceDirectory, handle_scratch2) +
                offsetof(LHDCV4_FftDirectoryGroup, active)) == 42 * sizeof(void *),
               "LLAC slot 42");

#endif /* LHDC_ENCODER_SRC_V4_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H */
