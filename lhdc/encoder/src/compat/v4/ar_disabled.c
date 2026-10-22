#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "../../../../ar/include/ar/compat/v4.h"
/* Compatibility interfaces explicitly report unavailable effects when AR is disabled. */
lhdc_ar_para_t *v4i_encoder_no_ar_process_new(void) {
    return NULL;
}

/* Return immediately without freeing or changing caller data when AR is absent. */
void v4i_encoder_no_ar_process_free(lhdc_ar_para_t *handle) {
    (void)handle;
    return;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v4i_encoder_no_ar_process_init(lhdc_ar_para_t *handle, int32_t sample_rate,
                                       int32_t bits_per_sample, int32_t channels,
                                       uint32_t samples_per_frame) {
    (void)handle;
    (void)sample_rate;
    (void)bits_per_sample;
    (void)channels;
    (void)samples_per_frame;
    return LHDC_V4_STATUS_ERROR;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v4i_encoder_no_ar_set_gyro_pos(lhdc_ar_para_t *handle, int32_t world_coordinate_x,
                                       int32_t world_coordinate_y, int32_t world_coordinate_z) {
    (void)handle;
    (void)world_coordinate_x;
    (void)world_coordinate_y;
    (void)world_coordinate_z;
    return LHDC_V4_STATUS_ERROR;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v4i_encoder_no_ar_set_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain,
                                  uint32_t app_ar_enabled) {
    (void)handle;
    (void)Pos;
    (void)Gain;
    (void)app_ar_enabled;
    return LHDC_V4_STATUS_ERROR;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v4i_encoder_no_ar_get_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain) {
    (void)handle;
    (void)Pos;
    (void)Gain;
    return LHDC_V4_STATUS_ERROR;
}

/* Report unavailable AR without reading/writing PCM buffers. */
int32_t v4i_encoder_no_ProcessLhdcAR(uint8_t *output, uint8_t *input, lhdc_ulong_t bytes,
                                     uint8_t *workspace, uint32_t workspace_bytes) {
    (void)output;
    (void)input;
    (void)bytes;
    (void)workspace;
    (void)workspace_bytes;
    return LHDC_V4_STATUS_ERROR;
}
