

#ifndef LHDC_RECOVERED_V5_LHDCV5_API_H
#define LHDC_RECOVERED_V5_LHDCV5_API_H
#include "../../../../src/v5/include/types/lhdc_base_types.h"
typedef struct _lhdcv5_para_t lhdcv5_para_t;

typedef enum __LHDCV5_SAMPLE_FREQ__ {
    LHDCV5_SR_44100HZ = 44100,
    LHDCV5_SR_48000HZ = 48000,
    LHDCV5_SR_96000HZ = 96000,
    LHDCV5_SR_192000HZ = 192000
} __LHDCV5_SAMPLE_FREQ__;

typedef enum __LHDCV5BT_SMPL_FMT__ {
    LHDCV5BT_SMPL_FMT_S16 = 16,
    LHDCV5BT_SMPL_FMT_S24 = 24,
    LHDCV5BT_SMPL_FMT_S32 = 32
} __LHDCV5BT_SMPL_FMT__;

typedef enum __LHDCV5_SAMPLE_FRAME__ {
    LHDCV5_SAMPLE_FRAME_5MS_44100KHZ = 240,
    LHDCV5_SAMPLE_FRAME_5MS_48000KHZ = 240,
    LHDCV5_SAMPLE_FRAME_5MS_96000KHZ = 480,
    LHDCV5_MAX_SAMPLE_FRAME = 960,
    LHDCV5_SAMPLE_FRAME_5MS_192000KHZ = 960
} __LHDCV5_SAMPLE_FRAME__;

typedef enum __LHDCV5_FRAME_DURATION__ {
    LHDCV5_FRAME_5MS = 50,
    LHDCV5_FRAME_7P5MS = 75,
    LHDCV5_FRAME_10MS = 100,
    LHDCV5_FRAME_1S = 10000
} __LHDCV5_FRAME_DURATION__;

typedef enum __LHDCV5_ENC_INTERVAL__ {
    LHDCV5_ENC_INTERVAL_10MS = 10,
    LHDCV5_ENC_INTERVAL_20MS = 20
} __LHDCV5_ENC_INTERVAL__;

typedef enum __LHDCV5_QUALITY__ {
    LHDCV5_QUALITY_LOW0 = 0,
    LHDCV5_QUALITY_LOW1 = 1,
    LHDCV5_QUALITY_LOW2 = 2,
    LHDCV5_QUALITY_LOW3 = 3,
    LHDCV5_QUALITY_LOW4 = 4,
    LHDCV5_QUALITY_LOW = 5,
    LHDCV5_QUALITY_MID = 6,
    LHDCV5_QUALITY_HIGH = 7,
    LHDCV5_QUALITY_HIGH1 = 8,
    LHDCV5_QUALITY_AUTO = 9,
    LHDCV5_QUALITY_INVALID = 10
} __LHDCV5_QUALITY__;

typedef enum __LHDCV5_MTU_SIZE__ {
    LHDCV5_MTU_MIN = 300,
    LHDCV5_MTU_2MBPS = 660,
    LHDCV5_MTU_MAX = 1023
} __LHDCV5_MTU_SIZE__;

typedef enum __LHDCV5_VERSION__ {
    LHDCV5_VERSION_1 = 1,
    LHDCV5_VERSION_INVALID = 2
} __LHDCV5_VERSION__;

typedef enum __LHDCV5_ENC_TYPE__ {
    LHDCV5_ENC_TYPE_UNKNOWN = 0,
    LHDCV5_ENC_TYPE_LHDCV5 = 1,
    LHDCV5_ENC_TYPE_INVALID = 2
} __LHDCV5_ENC_TYPE__;

typedef enum __LHDCV5_ENC_TYPE__ LHDCV5_ENC_TYPE_T;

typedef enum __LHDCV5_EXT_FUNC__ {
    LHDCV5_EXT_FUNC_AR = 0,
    LHDCV5_EXT_FUNC_LARC = 1,
    LHDCV5_EXT_FUNC_JAS = 2,
    LHDCV5_EXT_FUNC_META = 3,
    LHDCV5_EXT_FUNC_INVALID = 4
} __LHDCV5_EXT_FUNC__;

typedef enum __LHDCV5_EXT_FUNC__ LHDCV5_EXT_FUNC_T;

typedef enum __LHDCV5_META_PARAM__ {
    LHDCV5_META_LEN_FIXED = 8,
    LHDCV5_META_LOOP_CNT_STD = 20,
    LHDCV5_META_LOOP_CNT_MAX = 100,
    LHDCV5_META_LEN_MAX = 128
} __LHDCV5_META_PARAM__;

typedef struct _lhdcv5_abr_para_t _lhdcv5_abr_para_t, *P_lhdcv5_abr_para_t;

typedef struct _lhdcv5_abr_para_t lhdcv5_abr_para_t;

struct _lhdcv5_abr_para_t {
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
};

typedef struct _lhdcv5_control_block_t _lhdcv5_control_block_t, *P_lhdcv5_control_block_t;

typedef struct _lhdcv5_control_block_t lhdcv5_cb_t;

typedef struct _lhdcv5_control_block_t LHDCV5_ControlBlock;

typedef void *HANDLE_LHDCV5_BT;
#include "../../../../../common/include/lhdc/config/status_v5.h"

int32_t v5f_lhdcv5_util_free_handle(lhdcv5_cb_t *handle);

int32_t v5f_lhdcv5_util_get_mem_req(uint32_t version, uint32_t *mem_req_bytes);

int32_t v5f_lhdcv5_util_get_handle(uint32_t version, lhdcv5_cb_t *handle, uint32_t mem_size);

int32_t v5f_lhdcv5_util_get_target_bitrate(lhdcv5_cb_t *handle, uint32_t *bitrate);

int32_t v5f_lhdcv5_util_set_target_bitrate_inx(lhdcv5_cb_t *handle, uint32_t bitrate_inx,
                                               uint32_t *bitrate_inx_set, bool upd_qual_status);

int32_t v5f_lhdcv5_util_set_max_bitrate_inx(lhdcv5_cb_t *handle, uint32_t max_bitrate_inx,
                                            uint32_t *max_bitrate_inx_set);

int32_t v5f_lhdcv5_util_set_min_bitrate_inx(lhdcv5_cb_t *handle, uint32_t min_bitrate_inx,
                                            uint32_t *min_bitrate_inx_set);

int32_t v5f_lhdcv5_util_adjust_bitrate(lhdcv5_cb_t *handle, LHDCV5_ENC_TYPE_T *enc_type_ptr,
                                       lhdcv5_abr_para_t **abr_para_ptr);

int32_t v5f_lhdcv5_util_reset_up_bitrate(lhdcv5_abr_para_t *handle);

int32_t v5f_lhdcv5_util_reset_down_bitrate(lhdcv5_abr_para_t *handle);

int32_t v5f_lhdcv5_util_get_ext_func_state(lhdcv5_cb_t *handle, LHDCV5_EXT_FUNC_T ext_type,
                                           bool *enable_ptr);

int32_t v5f_lhdcv5_util_set_ext_func_state(lhdcv5_cb_t *handle, LHDCV5_EXT_FUNC_T ext_type,
                                           bool func_enable, uint8_t *data_ptr, uint32_t data_len,
                                           uint32_t loop_cnt);

int32_t v5f_lhdcv5_util_init_encoder(lhdcv5_cb_t *handle, uint32_t sampling_freq,
                                     uint32_t bits_per_sample, uint32_t bitrate_inx,
                                     uint32_t frame_duration, uint32_t mtu, uint32_t interval);

int32_t v5f_lhdcv5_util_get_block_Size(lhdcv5_cb_t *handle, uint32_t *block_size);

int32_t v5f_lhdcv5_util_enc_process(lhdcv5_cb_t *handle, void *p_pcm, uint32_t pcm_bytes,
                                    uint8_t *out_put, uint32_t out_buf_bytes, uint32_t *written,
                                    uint32_t *out_frames);

int32_t v5f_lhdcv5_util_get_bitrate(lhdcv5_para_t *lhdcv5, uint32_t bitrate_inx, uint32_t *bitrate);

int32_t v5f_lhdcv5_util_get_bitrate_inx(lhdcv5_para_t *lhdcv5, uint32_t bitrate, uint32_t *bitrate_inx);

int32_t v5f_lhdcv5_util_ar_set_gyro_pos(lhdcv5_cb_t *handle, int32_t world_coordinate_x,
                                        int32_t world_coordinate_y, int32_t world_coordinate_z);

int32_t v5f_lhdcv5_util_ar_set_cfg(lhdcv5_cb_t *handle, int32_t *pos_ptr, uint32_t pos_item_num,
                                   float *gain_ptr, uint32_t gain_item_num,
                                   uint32_t app_ar_enabled);

int32_t v5f_lhdcv5_util_ar_get_cfg(lhdcv5_cb_t *handle, int32_t *pos_ptr, uint32_t pos_item_num,
                                   float *gain_ptr, uint32_t gain_item_num);

#endif /* LHDC_RECOVERED_V5_LHDCV5_API_H */
