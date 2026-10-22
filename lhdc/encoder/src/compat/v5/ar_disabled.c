#include "../../../../common/include/lhdc/config/status_v5.h"
#include "../../../../ar/include/ar/compat/v5.h"
/* Compatibility interfaces explicitly report unavailable effects when AR is disabled. */
int32_t v5i_encoder_no_ar_process_new(lhdcv5_ar_para_t *handle, uint32_t mem_size) {
    (void)handle;
    (void)mem_size;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v5i_encoder_no_ar_process_run(lhdcv5_ar_para_t *handle, uint8_t *wav, uint32_t wav_bytes) {
    (void)handle;
    (void)wav;
    (void)wav_bytes;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v5i_encoder_no_ar_process_init(lhdcv5_ar_para_t *handle, uint32_t sample_rate,
                                       uint32_t bits_per_sample, uint32_t channels,
                                       uint32_t samples_per_frame) {
    (void)handle;
    (void)sample_rate;
    (void)bits_per_sample;
    (void)channels;
    (void)samples_per_frame;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without changing the handle. */
int32_t v5i_encoder_no_ar_process_free(lhdcv5_ar_para_t *handle) {
    (void)handle;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without writing workspace capacity. */
int32_t v5i_encoder_no_ar_process_get_mem_req(uint32_t *mem_req_bytes) {
    (void)mem_req_bytes;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without reading/writing UI parameter arrays. */
int32_t v5i_encoder_no_ar_ui_prams_update_set(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                              float *gain_ptr) {
    (void)pstLhdcArParams;
    (void)pos_ptr;
    (void)gain_ptr;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without reading/writing UI parameter arrays. */
int32_t v5i_encoder_no_ar_ui_prams_update_get(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                              float *gain_ptr) {
    (void)pstLhdcArParams;
    (void)pos_ptr;
    (void)gain_ptr;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v5i_encoder_no_ar_set_gyro_pos(lhdcv5_ar_para_t *handle, int32_t world_coordinate_x,
                                       int32_t world_coordinate_y, int32_t world_coordinate_z) {
    (void)handle;
    (void)world_coordinate_x;
    (void)world_coordinate_y;
    (void)world_coordinate_z;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v5i_encoder_no_ar_set_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr,
                                  uint32_t app_ar_enabled) {
    (void)handle;
    (void)pos_ptr;
    (void)gain_ptr;
    (void)app_ar_enabled;
    return LHDCV5_FRET_AR_NOT_READY;
}

/* Report unavailable AR without PCM or parameter processing. */
int32_t v5i_encoder_no_ar_get_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr) {
    (void)handle;
    (void)pos_ptr;
    (void)gain_ptr;
    return LHDCV5_FRET_AR_NOT_READY;
}
