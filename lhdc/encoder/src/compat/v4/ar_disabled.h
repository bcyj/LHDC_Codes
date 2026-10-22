#ifndef LHDC_ENCODER_SRC_COMPAT_V4_AR_DISABLED_H
#define LHDC_ENCODER_SRC_COMPAT_V4_AR_DISABLED_H
#include "../../../../ar/include/ar/lhdc_pcm.h"
#define v4f_ar_get_cfg v4i_encoder_no_ar_get_cfg
#define v4f_ar_process_free v4i_encoder_no_ar_process_free
#define v4f_ar_process_init v4i_encoder_no_ar_process_init
#define v4f_ar_process_new v4i_encoder_no_ar_process_new
#define v4f_ar_set_cfg v4i_encoder_no_ar_set_cfg
#define v4f_ar_set_gyro_pos v4i_encoder_no_ar_set_gyro_pos
#define v4f_ProcessLhdcAR v4i_encoder_no_ProcessLhdcAR
lhdc_ar_para_t *v4i_encoder_no_ar_process_new(void);
void v4i_encoder_no_ar_process_free(lhdc_ar_para_t *handle);
int32_t v4i_encoder_no_ar_process_init(lhdc_ar_para_t *handle, int32_t sample_rate,
                                       int32_t bits_per_sample, int32_t channels,
                                       uint32_t samples_per_frame);
int32_t v4i_encoder_no_ar_set_gyro_pos(lhdc_ar_para_t *handle, int32_t world_coordinate_x,
                                       int32_t world_coordinate_y, int32_t world_coordinate_z);
int32_t v4i_encoder_no_ar_set_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain,
                                  uint32_t app_ar_enabled);
int32_t v4i_encoder_no_ar_get_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain);
int32_t v4i_encoder_no_ProcessLhdcAR(uint8_t *output, uint8_t *input, lhdc_ulong_t bytes,
                                     uint8_t *workspace, uint32_t workspace_bytes);
#endif
