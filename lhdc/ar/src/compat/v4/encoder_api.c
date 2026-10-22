#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "lhdc_memory.h"
#include "../../core/include/ar_internal.h"
#include "../../../include/ar/compat/v4.h"
/* Allocate the V4 AR wrapper handle, effects workspace, and separate parameter block. */
lhdc_ar_para_t *v4f_ar_process_new(void) {
    return v4f_ar_process_new_with_allocator(NULL);
}

/* Query the three arena partitions required by the encoder adapter. */
size_t v4f_ar_process_workspace_bytes(void) {
    return lhdc_memory_block_bytes(sizeof(lhdc_ar_para_t)) +
           lhdc_memory_block_bytes(v4f_LhdcArMemBytesReq()) +
           lhdc_memory_block_bytes(sizeof(__ST_LHDC_AR_PARAMS));
}

/* Capture the specified host callbacks and unwind allocations on failure. */
lhdc_ar_para_t *v4f_ar_process_new_with_allocator(const LhdcAllocator *allocator) {
    lhdc_ar_para_t *handle = lhdc_memory_alloc(allocator, sizeof *handle);
    if (!handle)
        return NULL;
    memset(handle, 0, sizeof *handle);
    handle->uiLhdcArMemBytes = v4f_LhdcArMemBytesReq();
    handle->pucLhdcArMem = lhdc_memory_alloc(allocator, handle->uiLhdcArMemBytes);

    if (handle->pucLhdcArMem)
        memset(handle->pucLhdcArMem, 0, sizeof(__stLHDC_AR_API_BLK));
    handle->uiARParamsBytes = sizeof(__ST_LHDC_AR_PARAMS);
    handle->pucARParams = lhdc_memory_calloc(allocator, 1, handle->uiARParamsBytes);
    if (!handle->uiLhdcArMemBytes || !handle->pucLhdcArMem || !handle->pucARParams) {
        lhdc_memory_free(handle->pucARParams);
        lhdc_memory_free(handle->pucLhdcArMem);
        lhdc_memory_free(handle);
        return NULL;
    }
    return handle;
}

/* Release the V4 AR handle's parameter block and effects workspace. */
void v4f_ar_process_free(lhdc_ar_para_t *handle) {
    if (!handle)
        return;
    lhdc_memory_free(handle->pucARParams);
    handle->pucARParams = NULL;
    lhdc_memory_free(handle->pucLhdcArMem);
    lhdc_memory_free(handle);
}

/* Initialize AR for stereo processing without writing back samples_per_frame. */
int32_t v4f_ar_process_init(lhdc_ar_para_t *handle, int32_t sample_rate, int32_t bits_per_sample,
                            int32_t channels, uint32_t samples_per_frame) {
    (void)channels;
    if (!handle)
        return LHDC_V4_STATUS_ERROR;
    handle->channels = 2;
    handle->ulNumBytes = ((uint32_t)bits_per_sample * samples_per_frame >> 2) & 0x1fffffff;
    handle->sample_rate = sample_rate;
    handle->bits_per_sample = bits_per_sample;
    handle->app_ar_enabled = 1;
    return v4f_InitLhdcAR(2, bits_per_sample, sample_rate, 2, bits_per_sample, handle->pucLhdcArMem,
                          handle->uiLhdcArMemBytes);
}

/* Convert integer degrees to tenths of a degree and return the underlying AR error code directly. */
int32_t v4f_ar_set_gyro_pos(lhdc_ar_para_t *handle, int32_t world_coordinate_x,
                            int32_t world_coordinate_y, int32_t world_coordinate_z) {
    if (!handle)
        return LHDC_V4_STATUS_ERROR;
    return v4f_SetGyroPosToLhdcAR((int32_t)((uint32_t)world_coordinate_x * 10u),
                                  (int32_t)((uint32_t)world_coordinate_y * 10u),
                                  (int32_t)((uint32_t)world_coordinate_z * 10u),
                                  handle->pucLhdcArMem, handle->uiLhdcArMemBytes);
}

/* Set six-channel positions and selected gains from the UI array; app_ar_enabled does not affect processing. */
int32_t v4f_ar_set_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain, uint32_t app_ar_enabled) {
    (void)app_ar_enabled;
    if (!handle)
        return LHDC_V4_STATUS_ERROR;
    PST_LHDC_AR_PARAMS params = (void *)handle->pucARParams;
    if (params) {
        int32_t nChangeHrirCoefChAzimuth = 0;
        for (int32_t i = 0; i < 6; ++i)
            nChangeHrirCoefChAzimuth |=
                params->pnArChAzimuth[i] != (int32_t)((uint32_t)Pos[i] * 10u);
        memset(params, 0, sizeof *params);
        params->nChangeHrirCoefChAzimuth = nChangeHrirCoefChAzimuth;
        for (int32_t i = 0; i < 6; ++i) {
            params->pnArChAzimuth[i] = (int32_t)((uint32_t)Pos[i] * 10u);
            params->pfArCh51Gain[0][i] = Gain[2 * i];
            params->pfArCh51Gain[1][i] = Gain[2 * i + 1];
            params->pfArCh51GainPost[i] = Gain[12 + i];
        }
        params->fDistEffect_dreal = Gain[20];
        params->fDistEffect_dref = Gain[21];
        params->fDistEffect_attack_t = Gain[22];
        params->fLhdcAr_reverb_gain = Gain[30];
        params->fLhdcAr_3d_downmix_gain = Gain[31];
    }
    return v4f_SetARParams(handle->pucARParams, handle->uiARParamsBytes, handle->pucLhdcArMem,
                           handle->uiLhdcArMemBytes);
}

/* Safely read AR parameters and map positions and valid gains back to the UI array. */

int32_t v4f_ar_get_cfg(lhdc_ar_para_t *handle, int32_t *Pos, float *Gain) {
    if (!handle || !Pos || !Gain)
        return LHDC_V4_STATUS_ERROR;
    int32_t result = v4f_GetARParams(handle->pucARParams, handle->uiARParamsBytes,
                                     handle->pucLhdcArMem, handle->uiLhdcArMemBytes);
    if (result)
        return result;
    PST_LHDC_AR_PARAMS params = (void *)handle->pucARParams;
    /* Reverse-map valid UI fields from ar_set_cfg; undefined Gain slots retain caller values. */
    for (int32_t i = 0; i < 6; ++i) {
        Pos[i] = params->pnArChAzimuth[i] / 10;
        Gain[2 * i] = params->pfArCh51Gain[0][i];
        Gain[2 * i + 1] = params->pfArCh51Gain[1][i];
        Gain[12 + i] = params->pfArCh51GainPost[i];
    }
    Gain[20] = params->fDistEffect_dreal;
    Gain[21] = params->fDistEffect_dref;
    Gain[22] = params->fDistEffect_attack_t;
    Gain[30] = params->fLhdcAr_reverb_gain;
    Gain[31] = params->fLhdcAr_3d_downmix_gain;
    return result;
}
