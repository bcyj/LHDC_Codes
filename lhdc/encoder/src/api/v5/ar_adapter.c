#include "../../../../common/include/lhdc_log.h"
#include "../../../../common/include/lhdc_alignment.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../v5/include/encoder_internal.h"
#include "../../v5/include/codec/codec_internal.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Validate the AR wrapper handle/configuration array, record errors in order, and return status. */
static int32_t validate_ar_config(lhdcv5_cb_t *handle, int32_t *pos_ptr, uint32_t pos_item_num,
                                  float *gain_ptr, uint32_t gain_item_num, const char *function) {
    if (!handle || !handle->ar_filter) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       !handle ? "(LHDC-exAPI) %s: Handle is NULL!"
                               : "(LHDC-exAPI) %s: AR handle is NULL!",
                       function);
        return !handle ? LHDCV5_FRET_INVALID_HANDLE_CB : LHDCV5_FRET_INVALID_HANDLE_AR;
    }
    if (!pos_ptr || !gain_ptr) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "(LHDC-exAPI) %s: Input parameter is NULL!", function);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (pos_item_num < 6) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "(LHDC-exAPI) %s: Invalid number of channel position (%u)!", function,
                       pos_item_num);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (gain_item_num >= 32) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "(LHDC-exAPI) %s: Invalid number of gain (%u)!", function, gain_item_num);
    return LHDCV5_FRET_INVALID_INPUT_PARAM;
}

/* Read the AR enable flag; treat query failure as disabled. */
static bool ar_enabled_for_update(lhdcv5_cb_t *handle, const char *function) {
    bool ar_enabled = false;
    int32_t func_ret = v5f_lhdcv5_util_get_ext_func_state(handle, LHDCV5_EXT_FUNC_AR, &ar_enabled);
    if (func_ret)
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "(LHDC-exAPI) %s: Fail to get ext. function state (%d)!", function,
                       func_ret);
    return !func_ret && ar_enabled;
}

/* Validate gyroscope angles; update orientation if AR is enabled, otherwise succeed. */
int32_t v5f_lhdcv5_util_ar_set_gyro_pos(lhdcv5_cb_t *handle, int32_t world_coordinate_x,
                                        int32_t world_coordinate_y, int32_t world_coordinate_z) {
    const char *function = __func__;
    if (!handle || !handle->ar_filter) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       !handle ? "(LHDC-exAPI) %s: Handle is NULL!"
                               : "(LHDC-exAPI) %s: AR handle is NULL!",
                       function);
        return !handle ? LHDCV5_FRET_INVALID_HANDLE_CB : LHDCV5_FRET_INVALID_HANDLE_AR;
    }
    if ((uint32_t)world_coordinate_x > 359 || (uint32_t)world_coordinate_y > 359 ||
        (uint32_t)world_coordinate_z > 359) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "(LHDC-exAPI) %s: ar coordinate error (%d, %d, %d)!", function,
                       world_coordinate_x, world_coordinate_y, world_coordinate_z);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!ar_enabled_for_update(handle, function)) {
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "(LHDC-exAPI) %s: AR is not enabled!",
                       function);
        return LHDCV5_FRET_SUCCESS;
    }
    int32_t func_ret = v5f_ar_set_gyro_pos(handle->ar_filter, world_coordinate_x,
                                           world_coordinate_y, world_coordinate_z);
    if (!func_ret)
        return LHDCV5_FRET_SUCCESS;
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "(LHDC-exAPI) %s: Fail to set Gyro's position (%d)!", function, func_ret);
    return LHDCV5_FRET_ERROR;
}

/* Update AR enable state and, when enabled, write channel positions/gains to internal configuration. */
int32_t v5f_lhdcv5_util_ar_set_cfg(lhdcv5_cb_t *handle, int32_t *pos_ptr, uint32_t pos_item_num,
                                   float *gain_ptr, uint32_t gain_item_num,
                                   uint32_t app_ar_enabled) {
    const char *function = __func__;
    int32_t func_ret =
        validate_ar_config(handle, pos_ptr, pos_item_num, gain_ptr, gain_item_num, function);
    if (func_ret)
        return func_ret;
    v5f_lhdcv5_util_set_ext_func_state(handle, LHDCV5_EXT_FUNC_AR, app_ar_enabled != 0, NULL, 0, 0);
    bool ar_enabled = ar_enabled_for_update(handle, function);
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "(LHDC-exAPI) %s: ar enabled %d", function,
                   ar_enabled);
    if (!ar_enabled) {
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "(LHDC-exAPI) %s: ar not enabled!",
                       function);
        return LHDCV5_FRET_SUCCESS;
    }
    /* The inner layer ignores the enable parameter; the extension-flag interface above already updated enable state. */
    func_ret = v5f_ar_set_cfg(handle->ar_filter, pos_ptr, gain_ptr, app_ar_enabled);
    if (!func_ret)
        return LHDCV5_FRET_SUCCESS;
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "(LHDC-exAPI) %s: Fail to set AR config (%d)!",
                   function, func_ret);
    return LHDCV5_FRET_ERROR;
}

/* Validate AR query storage and return enable state without writing position/gain arrays. */
int32_t v5f_lhdcv5_util_ar_get_cfg(lhdcv5_cb_t *handle, int32_t *pos_ptr, uint32_t pos_item_num,
                                   float *gain_ptr, uint32_t gain_item_num) {
    const char *function = __func__;
    int32_t func_ret =
        validate_ar_config(handle, pos_ptr, pos_item_num, gain_ptr, gain_item_num, function);
    if (func_ret)
        return func_ret;
    bool ar_enabled = false;
    func_ret = v5f_lhdcv5_util_get_ext_func_state(handle, LHDCV5_EXT_FUNC_AR, &ar_enabled);
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                   !func_ret && ar_enabled ? "(LHDC-exAPI) %s: AR enabled"
                                           : "(LHDC-exAPI) %s: AR not enabled!",
                   function);
    return LHDCV5_FRET_SUCCESS;
}
