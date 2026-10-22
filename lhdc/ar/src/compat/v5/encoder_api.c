#include "../../../../common/include/compat/v5/lhdc_v5_constants.h"
#include "../../../../common/include/lhdc_log.h"
#include "../../../../common/include/lhdc_alignment.h"
#include "../../core/include/ar_internal.h"
#include "../../core/include/ar_workspace_views.h"
#include "../../../include/ar/compat/v5.h"
#include "../../../../common/include/lhdc/config/status_v5.h"

/* AR PCM processor: construct the object and workspace partitions in caller memory. */
int32_t v5f_ar_process_new(lhdcv5_ar_para_t *handle, uint32_t mem_size) {
    if (!handle)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    uint8_t *workspace =
        lhdc_align_pointer((uint8_t *)handle + sizeof(*handle), _Alignof(LHDCUpmixWorkspace));
    uint32_t workspace_bytes = v5f_LhdcArMemBytesReq();
    uint8_t *params =
        lhdc_align_pointer(workspace + workspace_bytes, _Alignof(__ST_LHDC_AR_PARAMS));
    size_t required = (size_t)(params - (uint8_t *)handle) + sizeof(__ST_LHDC_AR_PARAMS);
    if (!handle || mem_size < sizeof(*handle))
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    if (!workspace_bytes || mem_size < required)
        return LHDCV5_FRET_ERROR;
    memset(handle, 0, sizeof *handle);
    handle->uiLhdcArMemBytes = workspace_bytes;
    handle->pucLhdcArMem = workspace;
    memset(handle->pucLhdcArMem, 0, workspace_bytes);
    handle->uiARParamsBytes = sizeof(__ST_LHDC_AR_PARAMS);
    handle->pucARParams = params;
    memset(handle->pucARParams, 0, sizeof(__ST_LHDC_AR_PARAMS));
    return LHDCV5_FRET_SUCCESS;
}

/* AR PCM processor: process PCM and save the actual processed byte count. */
int32_t v5f_ar_process_run(lhdcv5_ar_para_t *handle, uint8_t *wav, uint32_t wav_bytes) {
    uint32_t processed_bytes;
    int32_t func_ret;

    if (handle == (lhdcv5_ar_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_AR;
    }
    if (wav == (uint8_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Input pcm buffer is NULL", __func__);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if ((handle->pucLhdcArMem == (uint8_t *)0x0) || (handle->uiLhdcArMemBytes == 0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR is not ready (%p, %u)!", __func__,
                       (void *)handle->pucLhdcArMem, handle->uiLhdcArMemBytes);

        return LHDCV5_FRET_AR_NOT_READY;
    }
    processed_bytes = handle->ulNumBytes;
    if (processed_bytes - 1 >= wav_bytes) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Invalid process length (%u)!", __func__,
                       processed_bytes);
        return LHDCV5_FRET_AR_NOT_READY;
    }
    func_ret = v5f_ProcessLhdcAR(wav, wav, (lhdc_ulong_t)processed_bytes, handle->pucLhdcArMem,
                                 handle->uiLhdcArMemBytes);
    if (-1 < func_ret) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR process error (%d)!", __func__, func_ret);
    return LHDCV5_FRET_ERROR;
}

/* AR PCM processor: initialize configuration and runtime state. */
int32_t v5f_ar_process_init(lhdcv5_ar_para_t *handle, uint32_t sample_rate,
                            uint32_t bits_per_sample, uint32_t channels,
                            uint32_t samples_per_frame) {
    if (!handle) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_AR;
    }
    if (channels != 2) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Invalid channel number (%u)!", __func__,
                       channels);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (sample_rate != LHDC_V5_SAMPLE_RATE_44100 && sample_rate != LHDC_V5_SAMPLE_RATE_48000 &&
        sample_rate != LHDC_V5_SAMPLE_RATE_96000 && sample_rate != LHDC_V5_SAMPLE_RATE_192000) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Invalid sampling frequency (%u)!",
                       __func__, sample_rate);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!(bits_per_sample < 0x21 &&
          (UINT64_C(1) << ((lhdc_ulong_t)bits_per_sample & 0x3f) & 0x101010000U) != 0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Invalid bits per sample (%u)!", __func__,
                       bits_per_sample);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (!samples_per_frame) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Invalid number of samples per frame (%d)!",
                       __func__, 0);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    uint8_t *pucLhdcArMem = handle->pucLhdcArMem;
    uint32_t uiLhdcArMemBytes = handle->uiLhdcArMemBytes;
    if (!pucLhdcArMem || !uiLhdcArMemBytes) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR is not ready (%p, %u)!", __func__,
                       (void *)pucLhdcArMem, uiLhdcArMemBytes);
        return LHDCV5_FRET_AR_NOT_READY;
    }
    handle->channels = 2;
    handle->samples_per_frame = samples_per_frame;
    handle->sample_rate = sample_rate;
    handle->bits_per_sample = bits_per_sample;
    handle->app_ar_enabled = 1;
    handle->ulNumBytes = (bits_per_sample >> 2 & 0x3ffffffe) * samples_per_frame;
    int32_t func_ret = v5f_InitLhdcAR(2, bits_per_sample, sample_rate, 2, bits_per_sample,
                                      pucLhdcArMem, uiLhdcArMemBytes);
    if (func_ret >= 0) {
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR initialization failed (%d)!", __func__,
                   func_ret);
    return LHDCV5_FRET_ERROR;
}

/* AR PCM processor: clear object resource references. */
int32_t v5f_ar_process_free(lhdcv5_ar_para_t *handle) {
    int32_t func_ret;
    if (handle == (lhdcv5_ar_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_AR;

        return func_ret;
    }
    if (handle->pucARParams != (uint8_t *)0x0) {
        handle->pucARParams = (uint8_t *)0x0;
    }
    func_ret = LHDCV5_FRET_SUCCESS;
    if (handle->pucLhdcArMem == (uint8_t *)0x0) {
        return func_ret;
    }
    func_ret = LHDCV5_FRET_SUCCESS;
    handle->pucLhdcArMem = (uint8_t *)0x0;
    return func_ret;
}

/* AR PCM processor: calculate memory for the object and its partitions. */
int32_t v5f_ar_process_get_mem_req(uint32_t *mem_req_bytes) {
    if (!mem_req_bytes)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    /* Caller addresses can have any valid handle alignment; reserve worst-case subobject slack. */
    *mem_req_bytes = (uint32_t)(sizeof(lhdcv5_ar_para_t) + _Alignof(LHDCUpmixWorkspace) - 1 +
                                v5f_LhdcArMemBytesReq() + _Alignof(__ST_LHDC_AR_PARAMS) - 1 +
                                sizeof(__ST_LHDC_AR_PARAMS));
    return LHDCV5_FRET_SUCCESS;
}

/* Map application AR UI parameters to the internal parameter structure. */
int32_t v5f_ar_ui_prams_update_set(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                   float *gain_ptr) {
    if (!pstLhdcArParams || !pos_ptr || !gain_ptr)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    int32_t change = 0;
    for (int32_t i = 0; i < 6; ++i)
        if (pstLhdcArParams->pnArChAzimuth[i] != pos_ptr[i] * 10)
            change = 1;
    memset(pstLhdcArParams, 0, sizeof(*pstLhdcArParams));
    pstLhdcArParams->nChangeHrirCoefChAzimuth = change;
    for (int32_t i = 0; i < 6; ++i) {
        pstLhdcArParams->pnArChAzimuth[i] = pos_ptr[i] * 10;
        pstLhdcArParams->pfArCh51Gain[0][i] = gain_ptr[2 * i];
        pstLhdcArParams->pfArCh51Gain[1][i] = gain_ptr[2 * i + 1];
        pstLhdcArParams->pfArCh51GainPost[i] = gain_ptr[12 + i];
    }
    pstLhdcArParams->fDistEffect_dreal = gain_ptr[20];
    pstLhdcArParams->fDistEffect_dref = gain_ptr[21];
    pstLhdcArParams->fDistEffect_attack_t = gain_ptr[22];
    pstLhdcArParams->fLhdcAr_reverb_gain = gain_ptr[30];
    pstLhdcArParams->fLhdcAr_3d_downmix_gain = gain_ptr[31];
    return LHDCV5_FRET_SUCCESS;
}

/* Extract AR positions and selected gains, leaving gain_ptr[30..31] unchanged. */
int32_t v5f_ar_ui_prams_update_get(PST_LHDC_AR_PARAMS pstLhdcArParams, int32_t *pos_ptr,
                                   float *gain_ptr) {
    if (!pstLhdcArParams || !pos_ptr || !gain_ptr)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    for (int32_t i = 0; i < 6; ++i) {
        pos_ptr[i] = pstLhdcArParams->pnArChAzimuth[i] / 10;
        gain_ptr[2 * i] = pstLhdcArParams->pfArCh51Gain[0][i];
        gain_ptr[2 * i + 1] = pstLhdcArParams->pfArCh51Gain[1][i];
        gain_ptr[12 + i] = pstLhdcArParams->pfArCh51GainPost[i];
    }
    gain_ptr[20] = pstLhdcArParams->fDistEffect_dreal;
    gain_ptr[21] = pstLhdcArParams->fDistEffect_dref;
    gain_ptr[22] = pstLhdcArParams->fDistEffect_attack_t;
    return LHDCV5_FRET_SUCCESS;
}

/* AR: update gyroscope orientation parameters. */
int32_t v5f_ar_set_gyro_pos(lhdcv5_ar_para_t *handle, int32_t world_coordinate_x,
                            int32_t world_coordinate_y, int32_t world_coordinate_z) {
    int32_t operation_result;
    int32_t func_ret;

    if (handle == (lhdcv5_ar_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_AR;

        return func_ret;
    }
    if ((handle->pucLhdcArMem == (uint8_t *)0x0) || (handle->uiLhdcArMemBytes == 0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR is not ready (%p, %u)!", __func__,
                       (void *)handle->pucLhdcArMem, handle->uiLhdcArMemBytes);
        func_ret = LHDCV5_FRET_AR_NOT_READY;

        return func_ret;
    }
    operation_result = v5f_SetGyroPosToLhdcAR(world_coordinate_x * 10, world_coordinate_y * 10,
                                              world_coordinate_z * 10, handle->pucLhdcArMem,
                                              handle->uiLhdcArMemBytes);
    if (operation_result >= 0) {
        func_ret = LHDCV5_FRET_SUCCESS;
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5",
                   "%s: Failed to update gyro (%d); coordinates=(%d, %d, %d)", __func__,
                   operation_result, world_coordinate_x, world_coordinate_y, world_coordinate_z);
    func_ret = LHDCV5_FRET_ERROR;

    return func_ret;
}

/* Write internal AR parameters and apply channel positions and gains; app_ar_enabled does not affect processing. */
int32_t v5f_ar_set_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr,
                       uint32_t app_ar_enabled) {
    int32_t operation_result;
    int32_t func_ret;
    PST_LHDC_AR_PARAMS pstLhdcArParams;
    (void)app_ar_enabled;

    if (handle == (lhdcv5_ar_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if ((pos_ptr == (int32_t *)0x0) || (gain_ptr == (float *)0x0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    pstLhdcArParams = (PST_LHDC_AR_PARAMS)handle->pucARParams;
    if ((((pstLhdcArParams == (PST_LHDC_AR_PARAMS)0x0) || (handle->uiARParamsBytes == 0)) ||
         (handle->pucLhdcArMem == (uint8_t *)0x0)) ||
        (handle->uiLhdcArMemBytes == 0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR is not ready (%p, %u) (%p, %u)!",
                       __func__, (void *)pstLhdcArParams, handle->uiARParamsBytes,
                       (void *)handle->pucLhdcArMem, handle->uiLhdcArMemBytes);
        func_ret = LHDCV5_FRET_AR_NOT_READY;

        return func_ret;
    }
    v5f_ar_ui_prams_update_set(pstLhdcArParams, pos_ptr, gain_ptr);
    operation_result = v5f_SetARParams(handle->pucARParams, handle->uiARParamsBytes,
                                       handle->pucLhdcArMem, handle->uiLhdcArMemBytes);
    func_ret = LHDCV5_FRET_SUCCESS;
    if (operation_result == 0) {
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Fail to set AR parameters (%d)!", __func__,
                   operation_result);
    func_ret = LHDCV5_FRET_ERROR;
    return func_ret;
}

/* Read internal AR parameters and populate the UI array. */
int32_t v5f_ar_get_cfg(lhdcv5_ar_para_t *handle, int32_t *pos_ptr, float *gain_ptr) {
    uint32_t uiARParamsBytes;
    int32_t operation_result;
    int32_t func_ret;
    const char *log_format;
    uint8_t *pucARParams;

    if (handle == (lhdcv5_ar_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if ((pos_ptr == (int32_t *)0x0) || (gain_ptr == (float *)0x0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    pucARParams = handle->pucARParams;
    uiARParamsBytes = handle->uiARParamsBytes;
    if ((((pucARParams == (uint8_t *)0x0) || (uiARParamsBytes == 0)) ||
         (handle->pucLhdcArMem == (uint8_t *)0x0)) ||
        (handle->uiLhdcArMemBytes == 0)) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", "%s: AR is not ready (%p, %u) (%p, %u)!",
                       __func__, (void *)pucARParams, uiARParamsBytes, (void *)handle->pucLhdcArMem,
                       handle->uiLhdcArMemBytes);
        func_ret = LHDCV5_FRET_AR_NOT_READY;

        return func_ret;
    }
    operation_result = v5f_GetARParams(pucARParams, uiARParamsBytes, handle->pucLhdcArMem,
                                       handle->uiLhdcArMemBytes);
    if (operation_result != 0) {
        log_format = "%s: Fail to get AR parameters (%d)!";

        lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", log_format, __func__, operation_result);
        func_ret = LHDCV5_FRET_ERROR;
        return func_ret;
    }
    func_ret =
        v5f_ar_ui_prams_update_get((PST_LHDC_AR_PARAMS)handle->pucARParams, pos_ptr, gain_ptr);
    if (func_ret == 0) {
        return LHDCV5_FRET_SUCCESS;
    }
    log_format = "%s: Fail to get AR parameters (UI) (%d)!";
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.AR.V5", log_format, __func__, func_ret);
    func_ret = LHDCV5_FRET_ERROR;
    return func_ret;
}
