#include "../../core/include/pcm_format.h"
#include <math.h>
#include <string.h>
/* Upmix: retrieve processed floating-point PCM. */
int32_t LHDC_FN(lhdc_upmix_output_f)(int32_t nSampleNum, uint8_t *pucMemLhdcUpmix) {
    if (!pucMemLhdcUpmix)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if (nSampleNum <= 0)
        return LHDC_CONST(AR_INVALID_SAMPLE_COUNT);
    PST_LHDC_UPMIX pstLhdcUpmix = (void *)pucMemLhdcUpmix;
    LHDC_PRIVATE(clip_pcm_channels)
    (pstLhdcUpmix->pfPcmOutput, pstLhdcUpmix->nChNumOutput, nSampleNum);
    return LHDC_CONST(STATUS_OK);
}

/* Upmix: buffer and process input floating-point PCM. */
int32_t LHDC_FN(lhdc_upmix_process_f)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmOut0,
                                      float *pfPcmOut1, float *pfPcmOut2, float *pfPcmOut3,
                                      int32_t nSampleNum, uint8_t *pucMemLhdcUpmix) {
    if (!pfPcmIn0 || !pfPcmIn1 || !pfPcmOut0 || !pfPcmOut1 || !pfPcmOut2 || !pfPcmOut3 ||
        !pucMemLhdcUpmix)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if (nSampleNum <= 0)
        return LHDC_CONST(AR_INVALID_SAMPLE_COUNT);
    PST_LHDC_UPMIX pstLhdcUpmix = (void *)pucMemLhdcUpmix;
    pstLhdcUpmix->pfPcm[0] = pfPcmIn0;
    pstLhdcUpmix->pfPcm[1] = pfPcmIn1;
    pstLhdcUpmix->pfPcmOutput[0] = pfPcmOut0;
    pstLhdcUpmix->pfPcmOutput[1] = pfPcmOut1;
    pstLhdcUpmix->pfPcmOutput[2] = pfPcmOut2;
    pstLhdcUpmix->pfPcmOutput[3] = pfPcmOut3;
    return LHDC_FN(lhdc_upmix_one_blk)(nSampleNum, pstLhdcUpmix);
}

/* Upmix: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_upmix_mem_req_bytes)(void) {
    uint32_t n;
    LHDC_FN(lhdc_upmix_get_lib_workspace_size)(&n);
    return n + 16 + sizeof(__ST_LHDC_UPMIX);
}

/* Upmix: initialize configuration and runtime state. */
int32_t LHDC_FN(lhdc_upmix_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                 int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                 int32_t nSampleRate, uint8_t *pucMemLhdcUpmix) {
    if (!pucMemLhdcUpmix)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_UPMIX pstLhdcUpmix = (void *)pucMemLhdcUpmix;
    memset(pstLhdcUpmix, 0, sizeof(*pstLhdcUpmix));
    if (nChNumInput != 2)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcUpmix->nChNumInput = nChNumInput;
    if (nBitsPerSampleInput != 16 && nBitsPerSampleInput != 24 && nBitsPerSampleInput != 32)
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcUpmix->nBitsPerSampleInput = nBitsPerSampleInput;
    if (nChNumOutput != 4)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcUpmix->nChNumOutput = nChNumOutput;
    if (nBitsPerSampleOutput != 16 && nBitsPerSampleOutput != 24 && nBitsPerSampleOutput != 32)
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcUpmix->nBitsPerSampleOutput = nBitsPerSampleOutput;
    if (nSampleRate != LHDC_CONST(SAMPLE_RATE_44100) &&
        nSampleRate != LHDC_CONST(SAMPLE_RATE_48000) &&
        nSampleRate != LHDC_CONST(SAMPLE_RATE_88200) &&
        nSampleRate != LHDC_CONST(SAMPLE_RATE_96000))
        return LHDC_CONST(AR_INIT_INVALID_SAMPLE_RATE);
    pstLhdcUpmix->nSampleRate = nSampleRate;
    pstLhdcUpmix->nUpmixBlkSampleNum = LHDC_CONST(AR_BLOCK_SAMPLES);
    if (LHDC_FN(lhdc_upmix_get_lib_workspace_size)(&pstLhdcUpmix->uiLibWorkspaceSize))
        return LHDC_CONST(AR_UPMIX_LIBRARY_ERROR);
    pstLhdcUpmix->pucLibWorkspace = pucMemLhdcUpmix + sizeof(*pstLhdcUpmix);
    if (LHDC_FN(lhdc_upmix_init_lib)(pstLhdcUpmix))
        return LHDC_CONST(AR_UPMIX_LIBRARY_ERROR);
    pstLhdcUpmix->uiTagBegin = pstLhdcUpmix->uiTagEnd = 0x25109908;
    pstLhdcUpmix->nLhdcUpmixState = 1;
    return LHDC_CONST(STATUS_OK);
}
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <string.h>

/* Upmix: process complete buffered audio blocks. */
int32_t LHDC_FN(lhdc_upmix_one_blk)(int32_t nBlkSampleNum, PST_LHDC_UPMIX pstLhdcUpmix) {
    int32_t r = LHDC_FN(upmix_lib_process)(pstLhdcUpmix, nBlkSampleNum);
    return r ? LHDC_CONST(AR_UPMIX_LIBRARY_ERROR) : LHDC_CONST(STATUS_OK);
}

/* Upmix: query workspace size required by the underlying library. */
int32_t LHDC_FN(lhdc_upmix_get_lib_workspace_size)(uint32_t *puiLibWorkspaceSize) {
    if (!puiLibWorkspaceSize)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    return LHDC_FN(upmix_lib_get_request_workspace_size)(puiLibWorkspaceSize)
               ? LHDC_CONST(AR_UPMIX_LIBRARY_ERROR)
               : LHDC_CONST(STATUS_OK);
}

/* Upmix: initialize the underlying library. */
int32_t LHDC_FN(lhdc_upmix_init_lib)(PST_LHDC_UPMIX pstLhdcUpmix) {
    int32_t nFnRet;
    int32_t result;

    nFnRet =
        LHDC_FN(upmix_lib_init)(pstLhdcUpmix->pucLibWorkspace, pstLhdcUpmix->uiLibWorkspaceSize);
    result = LHDC_CONST(STATUS_OK);
    if (nFnRet != 0) {
        result = -6;
    }
    return result;
}
