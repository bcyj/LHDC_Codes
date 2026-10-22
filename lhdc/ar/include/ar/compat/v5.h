#ifndef LHDC_AR_COMPAT_V5_H
#define LHDC_AR_COMPAT_V5_H
#include "../../../../common/include/types/lhdc_base_types.h"
typedef struct _lhdcv5_ar_para_t _lhdcv5_ar_para_t, *P_lhdcv5_ar_para_t;

typedef struct _lhdcv5_ar_para_t lhdcv5_ar_para_t;

struct _lhdcv5_ar_para_t {
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    uint32_t channels;
    uint32_t samples_per_frame;
    uint32_t app_ar_enabled;
    uint32_t ulNumBytes;
    uint8_t *pucARParams;
    uint32_t uiARParamsBytes;
    uint8_t *pucLhdcArMem;
    uint32_t uiLhdcArMemBytes;
};
#include "../parameters.h"
#ifdef __cplusplus
extern "C" {
#endif
int32_t v5f_ar_process_new(lhdcv5_ar_para_t *handle, uint32_t mem_size);

int32_t v5f_ar_process_run(lhdcv5_ar_para_t *handle, uint8_t *wav, uint32_t wav_bytes);

int32_t v5f_ar_process_init(lhdcv5_ar_para_t *handle, uint32_t sample_rate,
                            uint32_t bits_per_sample, uint32_t channels,
                            uint32_t samples_per_frame);

int32_t v5f_ar_process_free(lhdcv5_ar_para_t *handle);

int32_t v5f_ar_process_get_mem_req(uint32_t *mem_req_bytes);

int32_t v5f_ar_ui_prams_update_set(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                   float *gain_ptr);

int32_t v5f_ar_ui_prams_update_get(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                   float *gain_ptr);

int32_t v5f_ar_set_gyro_pos(lhdcv5_ar_para_t *handle, int32_t world_coordinate_x,
                            int32_t world_coordinate_y, int32_t world_coordinate_z);

int32_t v5f_ar_set_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr,
                       uint32_t app_ar_enabled);

int32_t v5f_ar_get_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr);
#ifdef __cplusplus
}
#endif
#endif
