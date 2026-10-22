#include "lhdc_memory.h"
#ifndef LHDC_AR_COMPAT_V4_H
#define LHDC_AR_COMPAT_V4_H
#include "../../../../common/include/types/lhdc_base_types.h"
typedef struct _lhdc_ar_para_t _lhdc_ar_para_t, *P_lhdc_ar_para_t;

typedef struct _lhdc_ar_para_t lhdc_ar_para_t;

struct _lhdc_ar_para_t {
    int32_t sample_rate;
    int32_t bits_per_sample;
    int32_t channels;
    uint32_t samples_per_frame;
    uint32_t app_ar_enabled;
    lhdc_ulong_t ulNumBytes;
    uint8_t *pucARParams;
    uint32_t uiARParamsBytes;
    uint8_t *pucLhdcArMem;
    uint32_t uiLhdcArMemBytes;
};
#include "../parameters.h"
#ifdef __cplusplus
extern "C" {
#endif
lhdc_ar_para_t *v4f_ar_process_new(void);
lhdc_ar_para_t *v4f_ar_process_new_with_allocator(const LhdcAllocator *allocator);
size_t v4f_ar_process_workspace_bytes(void);

void v4f_ar_process_free(lhdc_ar_para_t *handle);

int32_t v4f_ar_process_init(lhdc_ar_para_t *handle, int32_t sample_rate, int32_t bits_per_sample,
                            int32_t channels, uint32_t samples_per_frame);

int32_t v4f_ar_set_gyro_pos(lhdc_ar_para_t *handle, int32_t world_coordinate_x,
                            int32_t world_coordinate_y, int32_t world_coordinate_z);

int32_t v4f_ar_set_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain, uint32_t app_ar_enabled);

int32_t v4f_ar_get_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain);
#ifdef __cplusplus
}
#endif
#endif
