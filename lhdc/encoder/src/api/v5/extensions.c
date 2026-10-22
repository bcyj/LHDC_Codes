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

/* Bluetooth encoder wrapper: query extension enable state. */
int32_t v5f_lhdcv5_util_get_ext_func_state(lhdcv5_cb_t *handle, LHDCV5_EXT_FUNC_T ext_type,
                                           bool *enable_ptr) {
    int32_t func_ret;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_CB;
    }
    if (enable_ptr == (bool *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);

        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    *enable_ptr = false;
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        return LHDCV5_FRET_INVALID_CODEC;
    }
    if ((handle->enc).lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    if (ext_type >= LHDCV5_EXT_FUNC_INVALID) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid ext. flag (%u)!", __func__,
                       ext_type);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    func_ret = v5f_lhdcv5_encoder_get_extra_func((handle->enc).lhdcv5, ext_type, enable_ptr);
    if (func_ret == 0) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Failed to query extension %u (%d)",
                   __func__, ext_type, func_ret);
    return LHDCV5_FRET_ERROR;
}

/* LHDC V5 encoder: read extension flags and metadata. */
int32_t v5f_lhdcv5_encoder_get_extra_func(lhdcv5_para_t *lhdcv5, uint32_t func_inx,
                                          bool *func_enable) {
    int32_t operation_result;
    int32_t func_ret;
    void *workspace;
    int32_t ext_flag;
    uint8_t meta_data[128];

    ext_flag = 0;
    if (lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;
        return func_ret;
    }
    workspace = lhdcv5->lh5_enc;
    if (workspace == (void *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_ENC;
        return func_ret;
    }
    if (func_enable == 0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    *func_enable = false;
    switch (func_inx) {
    case 0:
        operation_result = v5f_lhdc_v5_enc_get_ar_flag(&ext_flag, workspace);
        break;
    case 1:
        operation_result = v5f_lhdc_v5_enc_get_larc_flag(&ext_flag, workspace);
        break;
    case 2:
        operation_result = v5f_lhdc_v5_enc_get_jas_flag(&ext_flag, workspace);
        break;
    case 3:
        operation_result = v5f_lhdc_v5_enc_get_meta_flag_and_data(&ext_flag, meta_data, workspace);
        break;
    default:
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid ext. flag (%u)!", __func__,
                       func_inx);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (operation_result == 0) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *func_enable = ext_flag != 0;

        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get ext. flag (%d) (%u)!",
                   __func__, operation_result, func_inx);
    func_ret = LHDCV5_FRET_ERROR;

    return func_ret;
}

/* Bluetooth encoder wrapper: set extension enable state. */
int32_t v5f_lhdcv5_util_set_ext_func_state(lhdcv5_cb_t *handle, LHDCV5_EXT_FUNC_T ext_type,
                                           bool func_enable, uint8_t *data_ptr, uint32_t data_len,
                                           uint32_t loop_cnt) {
#if !LHDC_ENCODER_WITH_AR
    if (handle && ext_type == LHDCV5_EXT_FUNC_AR && func_enable)
        return LHDCV5_FRET_AR_NOT_READY;
#endif
    int32_t operation_result;
    int32_t func_ret;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    if ((handle->enc).lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    if (ext_type >= LHDCV5_EXT_FUNC_INVALID) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid ext. flag (%u)!", __func__,
                       ext_type);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if ((ext_type == LHDCV5_EXT_FUNC_META) && (data_ptr == (uint8_t *)0x0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: ext_type:%u, func_enable:%d", __func__,
                   ext_type, func_enable);
    operation_result = v5f_lhdcv5_encoder_set_extra_func((handle->enc).lhdcv5, ext_type,
                                                         func_enable, data_ptr, data_len, loop_cnt);
    func_ret = LHDCV5_FRET_SUCCESS;
    if (operation_result == 0) {
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to set ext. function state (%d)!",
                   __func__, operation_result);
    func_ret = LHDCV5_FRET_ERROR;
    return func_ret;
}

/* Set AR, LARC, JAS, or metadata flags, validating metadata length/repetition count. */
int32_t v5f_lhdcv5_encoder_set_extra_func(lhdcv5_para_t *lhdcv5, uint32_t func_inx,
                                          bool func_enable, uint8_t *func_data, uint32_t data_len,
                                          uint32_t loop_cnt) {
    if (!lhdcv5 || !lhdcv5->lh5_enc) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        return lhdcv5 ? LHDCV5_FRET_INVALID_HANDLE_ENC : LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    LHDCV5_Workspace *workspace = lhdcv5->lh5_enc;
    int32_t func_ret;
    switch (func_inx) {
    case 0:
        func_ret = v5f_lhdc_v5_enc_set_ar_flag(func_enable, workspace);
        break;
    case 1:
        func_ret = v5f_lhdc_v5_enc_set_larc_flag(func_enable, workspace);
        break;
    case 2:
        func_ret = v5f_lhdc_v5_enc_set_jas_flag(func_enable, workspace);
        break;
    case 3: {
        uint8_t meta_data[32] = {0};
        if (!func_data) {
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!",
                           __func__);
            return LHDCV5_FRET_INVALID_INPUT_PARAM;
        }
        if (loop_cnt > 100) {
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid  loop count (%u)!",
                           __func__, loop_cnt);
            return LHDCV5_FRET_INVALID_INPUT_PARAM;
        }
        uint32_t meta_data_len = data_len < 8 ? data_len : 8;
        if (meta_data_len)
            memcpy(meta_data, func_data, meta_data_len);
        func_ret =
            v5f_lhdc_v5_enc_set_meta_flag_and_data(func_enable, meta_data, loop_cnt, workspace);
        break;
    }
    default:
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid ext. flag (%u)!", __func__,
                       func_inx);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!func_ret)
        return LHDCV5_FRET_SUCCESS;
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to set ext. flag (%d) (%u)!",
                   __func__, func_ret, func_inx);
    return LHDCV5_FRET_ERROR;
}
