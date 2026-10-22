#include "lhdc_log.h"
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Calculate top-level AR workspace size. */
uint32_t LHDC_FN(LhdcArMemBytesReq)(void) {
    return LHDC_FN(lhdc_ar_api_mem_bytes)();
}

/* Run the AR PCM chain and translate its return status. */
int32_t LHDC_FN(ProcessLhdcAR)(uint8_t *pucDst, uint8_t *pucSrc, lhdc_ulong_t ulNumBytes,
                               uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes) {
    if (!pucLhdcArMem)
        return LHDC_CONST(AR_WRAPPER_NULL_WORKSPACE);
    if (!LHDC_FN(lhdc_ar_api_ready_to_process)(pucLhdcArMem, uiLhdcArMemBytes))
        return LHDC_CONST(AR_WRAPPER_OPERATION_FAILED);
    return LHDC_FN(lhdc_ar_api_process_pcm)(pucDst, pucSrc, ulNumBytes, pucLhdcArMem,
                                            uiLhdcArMemBytes)
               ? LHDC_CONST(AR_WRAPPER_OPERATION_FAILED)
               : LHDC_CONST(STATUS_OK);
}

/* Bind AR resources and initialize input/output formats. */
int32_t LHDC_FN(InitLhdcAR)(int32_t nLhdcArInChNum, int32_t nLhdcArInBitsPerSample,
                            int32_t nLhdcArInSampleRate, int32_t nLhdcArOutChNum,
                            int32_t nLhdcArOutBitsPerSample, uint8_t *pucLhdcArMem,
                            uint32_t uiLhdcArMemBytes) {
    int32_t nRet;
    int32_t nFnRet;

    if (pucLhdcArMem == (uint8_t *)0x0) {
        nRet = LHDC_CONST(AR_WRAPPER_NULL_WORKSPACE);

        return nRet;
    }
    nRet = LHDC_FN(lhdc_ar_api_res_acquire)(pucLhdcArMem, uiLhdcArMemBytes);
    if (nRet != 0) {
        nRet = LHDC_CONST(AR_WRAPPER_RESOURCE_ERROR);
        return nRet;
    }
    nFnRet = LHDC_FN(lhdc_ar_api_init)(nLhdcArInChNum, nLhdcArInBitsPerSample, nLhdcArInSampleRate,
                                       nLhdcArOutChNum, nLhdcArOutBitsPerSample, pucLhdcArMem,
                                       uiLhdcArMemBytes);
    nRet = LHDC_CONST(STATUS_OK);
    if (nFnRet == 0) {
        return nRet;
    }
    nRet = LHDC_CONST(AR_WRAPPER_OPERATION_FAILED);
    return nRet;
}

/* Update AR gyroscope position and translate error codes. */
int32_t LHDC_FN(SetGyroPosToLhdcAR)(int32_t nX, int32_t nY, int32_t nZ, uint8_t *pucLhdcArMem,
                                    uint32_t uiLhdcArMemBytes) {
    int32_t nFnRet;
    int32_t nRet;

    if (pucLhdcArMem == (uint8_t *)0x0) {
        return LHDC_CONST(AR_WRAPPER_NULL_WORKSPACE);
    }
    nFnRet = LHDC_FN(lhdc_ar_api_set_gyro_pos)(nX, nY, nZ, pucLhdcArMem, uiLhdcArMemBytes);
    nRet = LHDC_CONST(STATUS_OK);
    if (nFnRet == 0) {
        return nRet;
    }
    nRet = LHDC_CONST(AR_WRAPPER_GYRO_FAILED);
    return nRet;
}

/* Write AR parameters and translate error codes. */
int32_t LHDC_FN(SetARParams)(uint8_t *pucARParams, uint32_t uiARParamsBytes, uint8_t *pucLhdcArMem,
                             uint32_t uiLhdcArMemBytes) {
    int32_t nFnRet;
    int32_t nRet;

    if (pucLhdcArMem == (uint8_t *)0x0) {
        return LHDC_CONST(AR_WRAPPER_NULL_WORKSPACE);
    }
    if (pucARParams == (uint8_t *)0x0) {
        return LHDC_CONST(AR_WRAPPER_NULL_PARAMS);
    }
    nFnRet = LHDC_FN(lhdc_ar_api_set_ar_params)(pucARParams, uiARParamsBytes, pucLhdcArMem,
                                                uiLhdcArMemBytes);
    nRet = LHDC_CONST(STATUS_OK);
    if (nFnRet == 0) {
        return nRet;
    }
    nRet = LHDC_CONST(AR_WRAPPER_OPERATION_FAILED);
    return nRet;
}

/* Read AR parameters and translate error codes. */
int32_t LHDC_FN(GetARParams)(uint8_t *pucARParams, uint32_t uiARParamsBytes, uint8_t *pucLhdcArMem,
                             uint32_t uiLhdcArMemBytes) {
    int32_t nFnRet;
    int32_t nRet;

    if (pucLhdcArMem == (uint8_t *)0x0) {
        return LHDC_CONST(AR_WRAPPER_NULL_WORKSPACE);
    }
    if (pucARParams == (uint8_t *)0x0) {
        return LHDC_CONST(AR_WRAPPER_NULL_PARAMS);
    }
    nFnRet = LHDC_FN(lhdc_ar_api_get_ar_params)(pucARParams, uiARParamsBytes, pucLhdcArMem,
                                                uiLhdcArMemBytes);
    nRet = LHDC_CONST(STATUS_OK);
    if (nFnRet == 0) {
        return nRet;
    }
    nRet = LHDC_CONST(AR_WRAPPER_OPERATION_FAILED);
    return nRet;
}
