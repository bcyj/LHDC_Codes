#include "lhdc_memory.h"
#ifndef LHDC_RECOVERED_V4_LHDC_API_H
#define LHDC_RECOVERED_V4_LHDC_API_H
#include "../../../../../ar/include/ar/compat/v4.h"
#include "../../../../src/v4/include/codec/encoder.h"
#include "../../../../src/v4/include/types/cirbuf.h"
#include "../../../../../common/include/compat/v4/lhdc_v4_constants.h"

typedef enum name {
    LHDCBT_QUALITY_LOW0 = 0,
    LHDCBT_QUALITY_LOW1 = 1,
    LHDCBT_QUALITY_LOW2 = 2,
    LHDCBT_QUALITY_LOW3 = 3,
    LHDCBT_QUALITY_LOW4 = 4,
    LHDCBT_QUALITY_LOW = 5,
    LHDCBT_QUALITY_MID = 6,
    LHDCBT_QUALITY_HIGH = 7,
    LHDCBT_QUALITY_AUTO = 8,
    LHDCBT_QUALITY_MAX = 9
} name;

typedef enum name LHDCBT_QUALITY_T;

typedef enum _lhdc_enc_type_t {
    ENC_TYPE_UNKNOWN = 0,
    ENC_TYPE_LHDC = 1,
    ENC_TYPE_LLAC = 2,
    ENC_TYPE_ERR = 3,
    ENC_TYPE_MAX = 3
} _lhdc_enc_type_t;

typedef enum _lhdc_enc_type_t lhdc_enc_type_t;

typedef struct _llac_para_t _llac_para_t, *P_llac_para_t;

typedef struct _llac_para_t llac_para_t;

struct _llac_para_t {
    uint8_t *lh4_enc;
    int32_t input_scratch[2 * LHDC_V4_LLAC_MAX_FRAME_SAMPLES];
    uint8_t packet_scratch[LHDC_V4_BT_MAX_MTU_BYTES + 1];
    int32_t sample_rate;
    int32_t bits_per_sample;
    LHDCBT_QUALITY_T qualityStatus;
    LHDCBT_QUALITY_T limitBitRateStatus;
    int32_t lastBitrate;
    int32_t real_bitrate;
    uint16_t avg_cnt;
    uint32_t avgValue;
    uint16_t changeBRCnt;
    uint32_t chageBRAvg;
    uint16_t upBitrateCnt;
    uint32_t upBitrateSum;
    uint16_t dnBitrateCnt;
    uint32_t dnBitrateSum;
    _Bool updateFramneInfo;
    uint32_t samples_per_frame;
    savi_circ_buf input_cbuf;
    int32_t out_nbytes;
    uint32_t host_mtu_size;
    uint32_t frame_per_packet;
    uint32_t encode_interval;
    _Bool new_packet_flag;
    struct {
        uint8_t init_calls, autobitrate_enabled, extra_func_mode;
        uint8_t ar_enabled, larc_enabled, exth_enabled;
    } mode;
};

typedef struct _lhdc_para_t _lhdc_para_t, *P_lhdc_para_t;

typedef struct _lhdc_para_t lhdc_para_t;

struct _lhdc_para_t {
    FFT_BLOCK *fft_blk;
    int32_t sample_rate;
    int32_t bits_per_sample;
    LHDCBT_QUALITY_T qualityStatus;
    LHDCBT_QUALITY_T limitBitRateStatus;
    int32_t lastBitrate;
    int32_t version;
    uint16_t avg_cnt;
    uint32_t avgValue;
    uint16_t changeBRCnt;
    uint32_t chageBRAvg;
    uint16_t upBitrateCnt;
    uint32_t upBitrateSum;
    uint16_t dnBitrateCnt;
    uint32_t dnBitrateSum;
    _Bool updateFramneInfo;
    uint32_t block_size;
    uint32_t target_mtu_size;
    uint32_t host_mtu_size;
    uint32_t frame_per_packet;
    uint32_t encode_interval;
    savi_circ_buf input_cbuf;
    uint32_t queue_frame_cnt;
    _Bool hasMinBitrateLimit;
    uint8_t pcm_frame[LHDC_V4_PCM_FRAME_MAX_BYTES];
    uint8_t packet_scratch[LHDC_V4_BT_MAX_MTU_BYTES + 1];
};

typedef union _enc_t _enc_t, *P_enc_t;

typedef union _enc_t enc_t;

union _enc_t {
    lhdc_para_t *lhdc;
    llac_para_t *llac;
};

typedef struct _lhdc_control_block_t _lhdc_control_block_t, *P_lhdc_control_block_t;

struct _lhdc_control_block_t {
    lhdc_enc_type_t enc_type;
    int32_t err;
    enc_t enc;
    lhdc_ar_para_t *ar_filter;
    uint32_t reference_abr_index;
    void *owned_workspace;
};

typedef struct _lhdc_control_block_t lhdc_cb_t;

typedef void *HANDLE_LHDC_BT;

llac_para_t *v4f_llac_encoder_new(void);
llac_para_t *v4f_llac_encoder_new_with_allocator(const LhdcAllocator *allocator);

void v4f_llac_encoder_free(llac_para_t *llac);

int32_t v4f_llac_encoder_get_frame_len(llac_para_t *handle);

int32_t v4f_llac_encoder_get_target_bitrate(llac_para_t *handle);

int32_t v4f_llac_encoder_init(llac_para_t *handle, uint32_t sample_rate, uint32_t bits_per_sample,
                              uint32_t bitrate_inx, uint32_t mtu_size, uint32_t encode_interval);

lhdc_para_t *v4f_lhdc_encoder_new(int32_t version);
lhdc_para_t *v4f_lhdc_encoder_new_with_allocator(int32_t version, const LhdcAllocator *allocator);

void v4f_lhdc_encoder_free(lhdc_para_t *handle);

int32_t v4f_lhdc_encoder_get_frame_len(lhdc_para_t *handle);

int32_t v4f_lhdc_encoder_get_target_bitrate(lhdc_para_t *handle);

int32_t v4f_lhdc_encoder_init(lhdc_para_t *handle, int32_t sampling_freq, uint32_t bitPerSample,
                              uint32_t bitrate_inx, uint32_t dualChannel, uint32_t need_padding,
                              uint32_t mtu, uint32_t interval);

void v4f_lhdc_util_reset_up_bitrate(lhdc_enc_type_t type, void *h);

void v4f_lhdc_util_reset_down_bitrate(lhdc_enc_type_t type, void *h);

bool v4f_lhdc_util_get_ext_func_state(lhdc_enc_type_t type, void *h, lhdc_ext_func_t ext_type);

int32_t v4f_lhdc_util_encv4_process(lhdc_cb_t *handle, void *p_pcm, uint8_t *out_put,
                                    uint32_t *written, uint32_t *out_frames);

uint32_t v4f_lhdc_util_get_bitrate(uint32_t index);

uint8_t *v4f_lhdc_util_get_buffer(lhdc_enc_type_t type, void *handle);

#endif /* LHDC_RECOVERED_V4_LHDC_API_H */
