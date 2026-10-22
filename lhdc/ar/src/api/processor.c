#include "lhdc_log.h"
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../core/include/ar_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LHDC_AR_WORKSPACE_TAG UINT32_C(0x25109908)

/* Remove consumed floating-point PCM and shift the remaining queued data forward. */
static void LHDC_PRIVATE(shift_pcm_queue)(float **channel_buffers, int32_t channel_count,
                                          int32_t consumed, int32_t remaining) {
    for (int32_t c = 0; c < channel_count; ++c)
        for (int32_t i = 0; i < remaining; ++i)
            channel_buffers[c][i] = channel_buffers[c][i + consumed];
}

/* AR API: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_ar_api_mem_bytes)(void) {
    return LHDC_FN(lhdc_upmix_mem_req_bytes)() + LHDC_FN(lhdc_dist_effect_mem_req_bytes)() +
           LHDC_FN(lhdc_ar_mem_req_bytes)() + LHDC_FN(lhdc_reverb_mem_req_bytes)() +
           LHDC_FN(lhdc_ar_params_mem_req_bytes)() + sizeof(__stLHDC_AR_API_BLK) +
           15 * 512 * sizeof(float);
}

/* AR API: initialize configuration and runtime state. */
int32_t LHDC_FN(lhdc_ar_api_init)(int32_t nArInChNum, int32_t nArInBitsPerSample,
                                  int32_t nArInSampleRate, int32_t nArOutChNum,
                                  int32_t nArOutBitsPerSample, uint8_t *pucLhdcArMem,
                                  uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    if (pLhdcArApiBlk->gnLhdcArResAcquireOK != 1)
        return LHDC_CONST(STATUS_OK);
    if (LHDC_FN(lhdc_upmix_init)(nArInChNum, nArInBitsPerSample, pLhdcArApiBlk->gnArUpmixOutChNum,
                                 nArInBitsPerSample, nArInSampleRate,
                                 pLhdcArApiBlk->gpucArUpmixMem))
        return LHDC_CONST(AR_API_INIT_UPMIX_FAILED);
    pLhdcArApiBlk->gnArUpmixInNumSampleQueue = LHDC_CONST(AR_BLOCK_SAMPLES);
    for (int32_t i = 0; i < nArInChNum; ++i)
        memset(pLhdcArApiBlk->gpfPcm[i], 0, LHDC_CONST(AR_BLOCK_BYTES));
    if (LHDC_FN(lhdc_dist_effect_init)(pLhdcArApiBlk->gnArDistEffectInChNum, nArInBitsPerSample,
                                       pLhdcArApiBlk->gnArDistEffectOutChNum, nArInBitsPerSample,
                                       nArInSampleRate, pLhdcArApiBlk->gpucArDistEffectMem))
        return LHDC_CONST(AR_API_INIT_DISTANCE_FAILED);
    pLhdcArApiBlk->gnArDistEffectInNumSampleQueue = 0;
    if (LHDC_FN(lhdc_ar_init)(pLhdcArApiBlk->gnAr3dDownMixInChNum, nArInBitsPerSample, nArOutChNum,
                              nArOutBitsPerSample, nArInSampleRate, nArInSampleRate / 10,
                              pLhdcArApiBlk->gpucLhdcArMem))
        return LHDC_CONST(AR_API_INIT_LOCALIZATION_FAILED);
    pLhdcArApiBlk->gnAr3dDownMixInNumSampleQueue = 0;
    if (LHDC_FN(lhdc_reverb_init)(pLhdcArApiBlk->gnArReverbInChNum, nArInBitsPerSample,
                                  pLhdcArApiBlk->gnArReverbOutChNum, nArInBitsPerSample,
                                  nArInSampleRate, pLhdcArApiBlk->gpucArReverbMem))
        return LHDC_CONST(AR_API_INIT_REVERB_FAILED);
    pLhdcArApiBlk->gnArReverbInNumSampleQueue = pLhdcArApiBlk->gnAr3dOutputInNumSampleQueue = 0;
    if (LHDC_FN(lhdc_ar_params_default)(pLhdcArApiBlk->gpucArParamMem))
        return LHDC_CONST(AR_API_INIT_LOCALIZATION_FAILED);
    if (LHDC_FN(lhdc_ar_params_set)(pLhdcArApiBlk))
        return LHDC_CONST(AR_API_INIT_PARAMS_FAILED);
    pLhdcArApiBlk->gnArInPcmChNum = nArInChNum;
    pLhdcArApiBlk->gnArInPcmBitsPerSample = nArInBitsPerSample;
    pLhdcArApiBlk->gnArOutPcmChNum = nArOutChNum;
    pLhdcArApiBlk->gnArOutPcmBitsPerSample = nArOutBitsPerSample;
    pLhdcArApiBlk->gulFrameCnt = 0;
    pLhdcArApiBlk->gnLhdcArInitOK = 1;
    return LHDC_CONST(STATUS_OK);
}

/* AR API: update gyroscope orientation parameters. */
int32_t LHDC_FN(lhdc_ar_api_set_gyro_pos)(int32_t nGyroX, int32_t nGyroY, int32_t nGyroZ,
                                          uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    if (LHDC_FN(lhdc_ar_set_gyro_pos)(nGyroX, nGyroY, nGyroZ, pLhdcArApiBlk->gpucLhdcArMem))
        return LHDC_CONST(AR_API_GYRO_LOCALIZATION_FAILED);
    if (LHDC_FN(lhdc_reverb_set_gyro_pos)(nGyroX, nGyroY, nGyroZ, pLhdcArApiBlk->gpucArReverbMem))
        return LHDC_CONST(AR_API_GYRO_REVERB_FAILED);
    return LHDC_CONST(STATUS_OK);
}

/* AR API: update the AR parameter block. */
int32_t LHDC_FN(lhdc_ar_api_set_ar_params)(uint8_t *pucArParams, uint32_t uiArParamsBufBytes,
                                           uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    if (!pucArParams)
        return LHDC_CONST(AR_API_NULL_PARAMS);
    if (uiArParamsBufBytes < sizeof(__ST_LHDC_AR_PARAMS))
        return LHDC_CONST(AR_API_PARAMS_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    if (pLhdcArApiBlk->gnLhdcArResAcquireOK != 1)
        return LHDC_CONST(AR_API_RESOURCES_NOT_ACQUIRED);
    memcpy(pLhdcArApiBlk->gpucArParamMem, pucArParams, sizeof(__ST_LHDC_AR_PARAMS));
    return LHDC_FN(lhdc_ar_params_set)(pLhdcArApiBlk) ? LHDC_CONST(AR_API_PARAMS_APPLY_FAILED)
                                                      : LHDC_CONST(STATUS_OK);
}

/* AR API: read the AR parameter block. */
int32_t LHDC_FN(lhdc_ar_api_get_ar_params)(uint8_t *pucArParams, uint32_t uiArParamsBufBytes,
                                           uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    if (!pucArParams)
        return LHDC_CONST(AR_API_NULL_PARAMS);
    if (uiArParamsBufBytes < sizeof(__ST_LHDC_AR_PARAMS))
        return LHDC_CONST(AR_API_PARAMS_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    if (pLhdcArApiBlk->gnLhdcArResAcquireOK != 1)
        return LHDC_CONST(AR_API_RESOURCES_NOT_ACQUIRED);
    memset(pucArParams, 0, sizeof(__ST_LHDC_AR_PARAMS));
    return LHDC_FN(lhdc_ar_params_get)(pucArParams, pLhdcArApiBlk)
               ? LHDC_CONST(AR_API_PARAMS_APPLY_FAILED)
               : LHDC_CONST(STATUS_OK);
}

/* AR API: check workspace readiness for processing. */
int32_t LHDC_FN(lhdc_ar_api_ready_to_process)(uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    return pLhdcArApiBlk->gnLhdcArResAcquireOK == 1 && pLhdcArApiBlk->gnLhdcArInitOK == 1;
}

/* AR API: partition workspace and bind processing modules. */
int32_t LHDC_FN(lhdc_ar_api_res_acquire)(uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    if (pLhdcArApiBlk->gnLhdcArResAcquireOK == 1)
        return LHDC_CONST(STATUS_OK);
    memset(pLhdcArApiBlk, 0, sizeof(*pLhdcArApiBlk));
    pLhdcArApiBlk->ulLhdcArBlkTagBegin = pLhdcArApiBlk->ulLhdcArBlkTagEnd = LHDC_AR_WORKSPACE_TAG;
    pLhdcArApiBlk->gnReverbGain = pLhdcArApiBlk->gn3dDownMixGain = 1.f;
    pLhdcArApiBlk->gnArUpmixEnable = pLhdcArApiBlk->gnArDistEffectEnable =
        pLhdcArApiBlk->gnLhdcAr3DLocalizeDownMixEnable = pLhdcArApiBlk->gnArReverbEnable = 1;
    pLhdcArApiBlk->gnArUpmixOutChNum = pLhdcArApiBlk->gnArDistEffectInChNum =
        pLhdcArApiBlk->gnArDistEffectOutChNum = pLhdcArApiBlk->gnAr3dDownMixInChNum = 4;
    pLhdcArApiBlk->gnAr3dDownMixOutChNum = pLhdcArApiBlk->gnArReverbOutChNum = 2;
    pLhdcArApiBlk->gnArReverbInChNum = 3;
    float **slots[5] = {pLhdcArApiBlk->gpfPcm, pLhdcArApiBlk->gpfPcmUpmix,
                        pLhdcArApiBlk->gpfPcmDistEff, pLhdcArApiBlk->gpfPcmDst,
                        pLhdcArApiBlk->gpfPcmReverb};
    uint32_t count[5] = {2, 4, 4, 2, 3}, cur = sizeof(*pLhdcArApiBlk);
    for (int32_t j = 0; j < 5; ++j)
        for (uint32_t i = 0; i < count[j]; ++i) {
            slots[j][i] = (void *)(pucLhdcArMem + cur);
            memset(pucLhdcArMem + cur, 0, 512 * sizeof(float));
            cur += 512 * sizeof(float);
        }
    uint8_t **blocks[5] = {&pLhdcArApiBlk->gpucArUpmixMem, &pLhdcArApiBlk->gpucArDistEffectMem,
                           &pLhdcArApiBlk->gpucLhdcArMem, &pLhdcArApiBlk->gpucArReverbMem,
                           &pLhdcArApiBlk->gpucArParamMem};
    uint32_t lengths[5] = {LHDC_FN(lhdc_upmix_mem_req_bytes)(),
                           LHDC_FN(lhdc_dist_effect_mem_req_bytes)(),
                           LHDC_FN(lhdc_ar_mem_req_bytes)(), LHDC_FN(lhdc_reverb_mem_req_bytes)(),
                           LHDC_FN(lhdc_ar_params_mem_req_bytes)()};
    for (int32_t i = 0; i < 5; ++i) {
        *blocks[i] = pucLhdcArMem + cur;
        memset(pucLhdcArMem + cur, 0, lengths[i]);
        cur += lengths[i];
    }
    pLhdcArApiBlk->gnLhdcArResAcquireOK = 1;
    return LHDC_CONST(STATUS_OK);
}

/* Upmix complete blocks; copy input channels if not ready. */
static void LHDC_PRIVATE(process_upmix_blocks)(PLHDC_AR_API_BLK pLhdcArApiBlk,
                                               int32_t nTtlPrcsSmplNumUpmix) {
    for (int32_t i = 0; i < nTtlPrcsSmplNumUpmix; i += LHDC_CONST(AR_BLOCK_SAMPLES)) {
        int32_t nOffset = pLhdcArApiBlk->gnArDistEffectInNumSampleQueue + i, output_ready = 0;
        if (pLhdcArApiBlk->gnArUpmixEnable) {
            LHDC_FN(lhdc_upmix_process_f)
            (pLhdcArApiBlk->gpfPcm[0] + i, pLhdcArApiBlk->gpfPcm[1] + i,
             pLhdcArApiBlk->gpfPcmUpmix[0] + nOffset, pLhdcArApiBlk->gpfPcmUpmix[1] + nOffset,
             pLhdcArApiBlk->gpfPcmUpmix[2] + nOffset, pLhdcArApiBlk->gpfPcmUpmix[3] + nOffset,
             LHDC_CONST(AR_BLOCK_SAMPLES), pLhdcArApiBlk->gpucArUpmixMem);
            output_ready = LHDC_FN(lhdc_upmix_output_f)(LHDC_CONST(AR_BLOCK_SAMPLES),
                                                        pLhdcArApiBlk->gpucArUpmixMem) == 0;
        }
        if (!output_ready)
            for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArInPcmChNum; ++nCh)
                memcpy(pLhdcArApiBlk->gpfPcmUpmix[nCh] + nOffset, pLhdcArApiBlk->gpfPcm[nCh] + i,
                       LHDC_CONST(AR_BLOCK_BYTES));
    }
}

/* Apply reverb to complete blocks; copy direct sound if not ready. */
static void LHDC_PRIVATE(process_reverb_blocks)(PLHDC_AR_API_BLK pLhdcArApiBlk,
                                                int32_t nTtlPrcsSmplNumReverb) {
    for (int32_t i = 0; i < nTtlPrcsSmplNumReverb; i += LHDC_CONST(AR_BLOCK_SAMPLES)) {
        int32_t output_ready = 0;
        if (pLhdcArApiBlk->gnArReverbEnable) {
            LHDC_FN(lhdc_reverb_process_f_o)
            (pLhdcArApiBlk->gpfPcmUpmix[0] + i, pLhdcArApiBlk->gpfPcmUpmix[1] + i,
             pLhdcArApiBlk->gpfPcmUpmix[2] + i, pLhdcArApiBlk->gpfPcmReverb[0] + i,
             pLhdcArApiBlk->gpfPcmReverb[1] + i, LHDC_CONST(AR_BLOCK_SAMPLES),
             pLhdcArApiBlk->gpucArReverbMem);
            output_ready = LHDC_FN(lhdc_reverb_output_f_o)(LHDC_CONST(AR_BLOCK_SAMPLES),
                                                           pLhdcArApiBlk->gpucArReverbMem) == 0;
        }
        if (!output_ready)
            for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArReverbOutChNum; ++nCh)
                memcpy(pLhdcArApiBlk->gpfPcmReverb[nCh] + i, pLhdcArApiBlk->gpfPcmUpmix[nCh] + i,
                       LHDC_CONST(AR_BLOCK_BYTES));
    }
}

/* Apply distance effects to complete blocks; copy upmix output if not ready. */
static void LHDC_PRIVATE(process_distance_blocks)(PLHDC_AR_API_BLK pLhdcArApiBlk,
                                                  int32_t nTtlPrcsSmplNumDistEffect) {
    for (int32_t i = 0; i < nTtlPrcsSmplNumDistEffect; i += LHDC_CONST(AR_BLOCK_SAMPLES)) {
        int32_t nOffset = pLhdcArApiBlk->gnAr3dDownMixInNumSampleQueue + i, output_ready = 0;
        if (pLhdcArApiBlk->gnArDistEffectEnable) {
            LHDC_FN(lhdc_dist_effect_process_f_o)
            (pLhdcArApiBlk->gpfPcmUpmix[0] + i, pLhdcArApiBlk->gpfPcmUpmix[1] + i,
             pLhdcArApiBlk->gpfPcmUpmix[2] + i, pLhdcArApiBlk->gpfPcmUpmix[3] + i,
             pLhdcArApiBlk->gpfPcmDistEff[0] + nOffset, pLhdcArApiBlk->gpfPcmDistEff[1] + nOffset,
             pLhdcArApiBlk->gpfPcmDistEff[2] + nOffset, pLhdcArApiBlk->gpfPcmDistEff[3] + nOffset,
             LHDC_CONST(AR_BLOCK_SAMPLES), pLhdcArApiBlk->gpucArDistEffectMem);
            output_ready =
                LHDC_FN(lhdc_dist_effect_output_f_o)(LHDC_CONST(AR_BLOCK_SAMPLES),
                                                     pLhdcArApiBlk->gpucArDistEffectMem) == 0;
        }
        if (!output_ready)
            for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArDistEffectOutChNum; ++nCh)
                memcpy(pLhdcArApiBlk->gpfPcmDistEff[nCh] + nOffset,
                       pLhdcArApiBlk->gpfPcmUpmix[nCh] + i, LHDC_CONST(AR_BLOCK_BYTES));
    }
}

/* Apply binaural positioning to complete blocks; copy distance-effect output if not ready. */
static void LHDC_PRIVATE(process_localization_blocks)(PLHDC_AR_API_BLK pLhdcArApiBlk,
                                                      int32_t nTtlPrcsSmplNumAR) {
    for (int32_t i = 0; i < nTtlPrcsSmplNumAR; i += LHDC_CONST(AR_BLOCK_SAMPLES)) {
        int32_t nOffset = pLhdcArApiBlk->gnAr3dOutputInNumSampleQueue + i, output_ready = 0;
        if (pLhdcArApiBlk->gnLhdcAr3DLocalizeDownMixEnable) {
            LHDC_FN(lhdc_ar_process_f_o)
            (pLhdcArApiBlk->gpfPcmDistEff[0] + i, pLhdcArApiBlk->gpfPcmDistEff[1] + i,
             pLhdcArApiBlk->gpfPcmDistEff[2] + i, pLhdcArApiBlk->gpfPcmDistEff[3] + i,
             pLhdcArApiBlk->gpfPcmDst[0] + nOffset, pLhdcArApiBlk->gpfPcmDst[1] + nOffset,
             LHDC_CONST(AR_BLOCK_SAMPLES), pLhdcArApiBlk->gpucLhdcArMem);
            output_ready = LHDC_FN(lhdc_ar_output_f_o)(LHDC_CONST(AR_BLOCK_SAMPLES),
                                                       pLhdcArApiBlk->gpucLhdcArMem) == 0;
        }
        if (!output_ready)
            for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnAr3dDownMixOutChNum; ++nCh)
                memcpy(pLhdcArApiBlk->gpfPcmDst[nCh] + nOffset,
                       pLhdcArApiBlk->gpfPcmDistEff[nCh] + i, LHDC_CONST(AR_BLOCK_BYTES));
    }
}

/* Unpack configured PCM, apply upmix, distance, reverb, and positioning, then write the target format. */
int32_t LHDC_FN(lhdc_ar_api_process_pcm)(uint8_t *pucDst, uint8_t *pucSrc, lhdc_ulong_t ulNumBytes,
                                         uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_API_NULL_WORKSPACE);
    if (uiLhdcArMemBytes < LHDC_FN(lhdc_ar_api_mem_bytes)())
        return LHDC_CONST(AR_API_WORKSPACE_TOO_SMALL);
    PLHDC_AR_API_BLK pLhdcArApiBlk = (void *)pucLhdcArMem;
    int32_t nArInBytePerSample = pLhdcArApiBlk->gnArInPcmBitsPerSample >> 3,
            nArOutBytePerSample = pLhdcArApiBlk->gnArOutPcmBitsPerSample >> 3;
    uint32_t nArInPcmBlkAlign = pLhdcArApiBlk->gnArInPcmChNum * nArInBytePerSample;
    int32_t nSampleNumGot = nArInPcmBlkAlign ? (int32_t)(ulNumBytes / nArInPcmBlkAlign) : 0;
    ++pLhdcArApiBlk->gulFrameCnt;
    for (int32_t i = 0; i < nSampleNumGot; ++i)
        for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArInPcmChNum; ++nCh) {
            float *out = pLhdcArApiBlk->gpfPcm[nCh];
            if (pucSrc && out)
                LHDC_FN(GetOneSample_f)
            (nArInBytePerSample, pucSrc, out + pLhdcArApiBlk->gnArUpmixInNumSampleQueue + i);
            pucSrc += nArInBytePerSample;
        }
    int32_t queued_samples = pLhdcArApiBlk->gnArUpmixInNumSampleQueue + nSampleNumGot,
            nTtlPrcsSmplNumUpmix =
                (queued_samples / LHDC_CONST(AR_BLOCK_SAMPLES)) * LHDC_CONST(AR_BLOCK_SAMPLES);
    LHDC_PRIVATE(process_upmix_blocks)(pLhdcArApiBlk, nTtlPrcsSmplNumUpmix);
    LHDC_PRIVATE(shift_pcm_queue)
    (pLhdcArApiBlk->gpfPcm, pLhdcArApiBlk->gnArInPcmChNum, nTtlPrcsSmplNumUpmix,
     queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES));
    pLhdcArApiBlk->gnArUpmixInNumSampleQueue = queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES);
    queued_samples = pLhdcArApiBlk->gnArDistEffectInNumSampleQueue + nTtlPrcsSmplNumUpmix;
    int32_t nTtlPrcsSmplNumReverb =
        (queued_samples / LHDC_CONST(AR_BLOCK_SAMPLES)) * LHDC_CONST(AR_BLOCK_SAMPLES);
    LHDC_PRIVATE(process_reverb_blocks)(pLhdcArApiBlk, nTtlPrcsSmplNumReverb);
    int32_t nTtlPrcsSmplNumDistEffect =
        (queued_samples / LHDC_CONST(AR_BLOCK_SAMPLES)) * LHDC_CONST(AR_BLOCK_SAMPLES);
    LHDC_PRIVATE(process_distance_blocks)(pLhdcArApiBlk, nTtlPrcsSmplNumDistEffect);
    LHDC_PRIVATE(shift_pcm_queue)
    (pLhdcArApiBlk->gpfPcmUpmix, pLhdcArApiBlk->gnArDistEffectInChNum, nTtlPrcsSmplNumDistEffect,
     queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES));
    pLhdcArApiBlk->gnArDistEffectInNumSampleQueue = queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES);
    queued_samples = pLhdcArApiBlk->gnAr3dDownMixInNumSampleQueue + nTtlPrcsSmplNumDistEffect;
    int32_t nTtlPrcsSmplNumAR =
        (queued_samples / LHDC_CONST(AR_BLOCK_SAMPLES)) * LHDC_CONST(AR_BLOCK_SAMPLES);
    LHDC_PRIVATE(process_localization_blocks)(pLhdcArApiBlk, nTtlPrcsSmplNumAR);
    LHDC_PRIVATE(shift_pcm_queue)
    (pLhdcArApiBlk->gpfPcmDistEff, pLhdcArApiBlk->gnArDistEffectOutChNum, nTtlPrcsSmplNumAR,
     queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES));
    pLhdcArApiBlk->gnAr3dDownMixInNumSampleQueue = queued_samples % LHDC_CONST(AR_BLOCK_SAMPLES);
    for (int32_t i = 0; i < nTtlPrcsSmplNumReverb; ++i)
        for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArOutPcmChNum; ++nCh) {
            int32_t output_sample_index = pLhdcArApiBlk->gnAr3dOutputInNumSampleQueue + i;
            pLhdcArApiBlk->gpfPcmDst[nCh][output_sample_index] =
                pLhdcArApiBlk->gn3dDownMixGain *
                    pLhdcArApiBlk->gpfPcmDst[nCh][output_sample_index] +
                pLhdcArApiBlk->gnReverbGain * pLhdcArApiBlk->gpfPcmReverb[nCh][i];
        }
    int32_t remaining =
        nTtlPrcsSmplNumReverb - nSampleNumGot + pLhdcArApiBlk->gnAr3dOutputInNumSampleQueue;
    for (int32_t i = 0; i < nSampleNumGot; ++i)
        for (int32_t nCh = 0; nCh < pLhdcArApiBlk->gnArOutPcmChNum; ++nCh) {
            LHDC_FN(SaveOneSample_f)(nArOutBytePerSample, pucDst, pLhdcArApiBlk->gpfPcmDst[nCh][i]);
            pucDst += nArOutBytePerSample;
        }
    LHDC_PRIVATE(shift_pcm_queue)
    (pLhdcArApiBlk->gpfPcmDst, pLhdcArApiBlk->gnArOutPcmChNum, nSampleNumGot, remaining);
    pLhdcArApiBlk->gnAr3dOutputInNumSampleQueue = remaining;
    return LHDC_CONST(STATUS_OK);
}
