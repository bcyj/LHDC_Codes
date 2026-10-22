

#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H
#include "../../../../../ar/include/ar/compat/v5.h"
#include "cirbuf.h"
#include "../../../../include/lhdc/encoder/v5/lhdcv5_api.h"
#include "../../../../../common/include/types/lhdc_dsp_types.h"
#include "../codec/lhdc_v5_fft_options.h"

enum { LHDC_V5_ENC_ERROR = -1, LHDC_V5_ENC_OK = 0 };

typedef union _lhdcv5enc_t _lhdcv5enc_t;

typedef union _lhdcv5enc_t lhdcv5_enc_t;

typedef struct _lhdcv5_para_t _lhdcv5_para_t;

union _lhdcv5enc_t {
    lhdcv5_para_t *lhdcv5;
    void *lhdcv5ot;
};

struct _lhdcv5_control_block_t {
    uint32_t enc_type;
    int32_t err;
    lhdcv5_enc_t enc;
    lhdcv5_ar_para_t *ar_filter;
    uint32_t reference_abr_index;
    void *owned_workspace;
};

struct _lhdcv5_para_t {
    uint32_t version;
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    uint32_t bits_per_sample_ui;
    uint32_t upBitrateCnt;
    uint32_t upBitrateSum;
    uint32_t dnBitrateCnt;
    uint32_t dnBitrateSum;
    uint32_t lastBitrate;
    uint32_t qualityStatus;
    uint32_t actual_bitrate;
    uint32_t max_bitrate_inx;
    uint32_t min_bitrate_inx;
    uint32_t samples_per_frame;
    uint32_t frame_duration;
    uint32_t frame_per_packet;
    uint32_t host_mtu_size;
    uint32_t target_mtu_size;
    uint32_t encode_interval;
    uint32_t encoded_frame_size;
    _Bool updateFramneInfo;
    void *lh5_enc;
    uint8_t *cirbuff;
    savi_circ_buf input_cbuf;
    uint32_t enc_in_buf_bytes;
    uint8_t *enc_in_buf;
    /* Each encoder owns its current table and custom tables for four sample rates. */
    int32_t bitrate_table[10];
    int32_t bitrate_tables[4][10];
};

#include "lhdc_bit_writer_types.h"

typedef struct mdct_s mdct_s;

typedef struct kiss_fft_state_v5 kiss_fft_state_v5;

typedef struct kiss_fft_state_v5 *kiss_fft_cfg_v5;

typedef struct mdct_ch mdct_ch;

typedef struct mdct_buf_s mdct_buf_s;

struct kiss_fft_state_v5 {
    int32_t nfft;
    int32_t inverse;
    int32_t factors[64];
    AudioMathComplex32 *twiddles;
};

struct mdct_ch {
    float overlap[LHDC_V5_MDCT_OVERLAP_SAMPLES];
};

struct mdct_s {
    /* Analysis len=2N; N2=N input/output samples, N4=N/2 complex FFT points. */
    int32_t len;
    int32_t overlap;
    int32_t shift_bits;
    int32_t N2;
    int32_t N4;
    int32_t NK; /* O/2: half-overlap length, O=overlap. */
    int32_t ZN; /* (N-O)/2: length of each unwindowed folded section. */
    kiss_fft_cfg_v5 fs;
    float *win;
    AudioMathComplex32 *twid1;
    AudioMathComplex32 *twid2;
    struct mdct_ch *ch;
    struct mdct_buf_s *buf;
#if LHDC_V5_ENCODER_FIXED_FFT
    LhdcV5EncoderFixedFftWork fixed_fft;
#endif
};

struct mdct_buf_s {
    int32_t dat[1344];
    float dct4[960];

    union {
        AudioMathComplex32 cin[480];
        float pcm_in[960];
    };
    AudioMathComplex32 cout[480];
};

typedef union anon_union_4_2_947300a4 anon_union_4_2_947300a4;

union anon_union_4_2_947300a4 {
    float f;
    uint32_t i;
};

typedef struct hdr_s hdr_s;

struct hdr_s {
    uint16_t info;
    uint8_t ext_data[LHDC_V5_HEADER_EXT_BYTES];
    int32_t enc_frm_len_provided;
    uint8_t enc_frm_len_need_udpate;
    int32_t meta_data_loop_count;
};

typedef enum hdr_info_index {
    ENC_SIZE_INDEX = 0,
    VERSION_INDEX = 1,
    JAS_INDEX = 2,
    AR_INDEX = 3,
    LARC_INDEX = 4,
    META_INDEX = 5,
    ALL_HEADER_INFO_NUM = 6
} hdr_info_index;

typedef struct ChannelData ChannelData;

struct ChannelData {
    int32_t gi;
    float gain_step;
};

typedef struct LhdcEncContext LhdcEncContext;

typedef struct BandCfg_t BandCfg_t;

typedef struct EncodeData EncodeData;

typedef union anon_union_19228_1_a8400cd6_for_enc anon_union_19228_1_a8400cd6_for_enc;

typedef union lossy_u lossy_u;

typedef struct bpc_buf_s bpc_buf_s;

struct bpc_buf_s {
    int32_t rd[960];
    int32_t rb[960];
    int32_t bna[1921];
    uint32_t q[960];
    int32_t nzc;
    int32_t alpha;
    int32_t qenc_len;
    int32_t sbn;
    int32_t sp_eval_bits;
    int32_t sp_enc_bits;
};

union lossy_u {
    struct mdct_buf_s mdct;
    struct bpc_buf_s bpc;
};

union anon_union_19228_1_a8400cd6_for_enc {
    union lossy_u lossy;
};

struct BandCfg_t {
    int32_t n;
    int32_t inv;

    const int32_t *off;

    const int32_t *scale;
    int32_t scale_shift_bits;
    int32_t scale_down;
    int32_t rice_min_p0;
    int32_t ac_ma0;
    int32_t ac_ma1;
};

struct EncodeData {
    float sdf[LHDC_V5_MAX_FRAME_SAMPLES];
    int32_t band_scf[LHDC_V5_SNS_MAX_BANDS];
    int32_t be_mean;
    uint8_t adsq_code[LHDC_V5_SNS_MAX_DIFFERENCES];
    int32_t adsq_index0;
    int32_t gi_highest;
    union anon_union_19228_1_a8400cd6_for_enc enc;
};

struct LhdcEncContext {
    int32_t sample_rate;
    int32_t bits_per_sample;
    int32_t channels;
    int32_t frm_size;
    int32_t overlap;
    int32_t frm_duration;
    int32_t nzc_bits;
    int32_t init_bn_bits;
    int32_t max_init_bn;
    int32_t side_info_bits;
    int32_t enc_frm_len;
    int32_t enc_frm_bits;
    int32_t ava_bits;
    int32_t ava_bits_low;
    int32_t eval_cp;
    float gain_tab[LHDC_V5_GAIN_TABLE_SIZE];
    int32_t gain_tab_base;
    float gain_tab_step;

    const BandCfg_t *band_cfg;
    int32_t band_stop;
    void *mdct_ws;
    uint8_t *lossy_uc;
    struct ChannelData chd[2];
    struct EncodeData ed;
    int32_t *pcm;
    int32_t fc;
};

typedef struct BandStop_t BandStop_t;

struct BandStop_t {
    int32_t frm_len;
    int32_t stop;
};

typedef struct mva_s mva_s;

struct mva_s {
    uint8_t ma[LHDC_V5_FAC_HISTORY_SIZE];
    int32_t mi;
    int32_t mn;
    int32_t mc;
};

typedef struct fac_enc fac_enc;

typedef struct sym_s sym_s;

struct sym_s {
    uint32_t sn;
    uint32_t csc[LHDC_V5_FAC_CUMULATIVE_ENTRIES];
    uint32_t sc[LHDC_V5_FAC_MAX_SYMBOLS];
    uint32_t sc_hi[LHDC_V5_FAC_MAX_SYMBOLS];
};

struct fac_enc {
    uint32_t base;
    uint32_t length;
    uint8_t *out;
    uint8_t *cur;
    uint8_t *end;
    struct mva_s mv;
    struct sym_s sym;
};

typedef enum lhdc_v5_enc_workspace_content_index {
    HDR_S_INDEX = 0,
    ECB_INDEX = 1,
    ECB_MDCT_WS_INDEX = 2,
    MDCT_CH_INDEX = 3,
    FENC_S_INDEX = 4,
    BW_INDEX = 5,
    ENCODED_DATA_CH0_INDEX = 6,
    ENCODED_DATA_CH1_INDEX = 7,
    MDCT_TWID1_INDEX = 8,
    MDCT_TWID2_INDEX = 9,
    MDCT_WIN_INDEX = 10,
    FFT_TWID_INDEX = 11,
    LHDC_V5_ENC_WORKSPACE_CONTENT_INDEX_ALL_CONTENT_NUM = 12
} lhdc_v5_enc_workspace_content_index;

typedef struct LHDCV5_MdctWorkspace {
    mdct_s mdct;
    kiss_fft_state_v5 fft;
} LHDCV5_MdctWorkspace;

typedef struct LHDCV5_Workspace {
    hdr_s *header;            /* 0x00 HDR_S_INDEX */
    LhdcEncContext *encoder;  /* 0x08 ECB_INDEX */
    mdct_s *mdct;             /* 0x10 ECB_MDCT_WS_INDEX */
    mdct_ch *channels;        /* 0x18 MDCT_CH_INDEX */
    fac_enc *arithmetic;      /* 0x20 FENC_S_INDEX */
    BitWriter *bit_writer;    /* 0x28 BW_INDEX */
    uint32_t *encoded_ch0;    /* 0x30 ENCODED_DATA_CH0_INDEX */
    uint32_t *encoded_ch1;    /* 0x38 ENCODED_DATA_CH1_INDEX */
    kiss_fft_cpx *mdct_twid1; /* 0x40 MDCT_TWID1_INDEX */
    kiss_fft_cpx *mdct_twid2; /* 0x48 MDCT_TWID2_INDEX */
    float *window;            /* 0x50 MDCT_WIN_INDEX */
    kiss_fft_cpx *fft_twid;   /* 0x58 FFT_TWID_INDEX */
} LHDCV5_Workspace;

#endif /* LHDC_ENCODER_SRC_V5_INCLUDE_TYPES_LHDC_NATIVE_TYPES_H */
