#include "../../core/include/pcm_format.h"
#include <math.h>
#include <string.h>
/* Distance effect: initialize configuration and runtime state. */
int32_t LHDC_FN(lhdc_dist_effect_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                       int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                       int32_t nSampleRate, uint8_t *pucMemLhdcDistEffect) {
    if (!pucMemLhdcDistEffect)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_DIST_EFFECT pstLhdcDistEffect = (void *)pucMemLhdcDistEffect;
    memset(pstLhdcDistEffect, 0, sizeof(*pstLhdcDistEffect));
    if (nChNumInput != 4)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcDistEffect->nChNumInput = nChNumInput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleInput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcDistEffect->nBitsPerSampleInput = nBitsPerSampleInput;
    if (nChNumOutput != 4)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcDistEffect->nChNumOutput = nChNumOutput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleOutput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcDistEffect->nBitsPerSampleOutput = nBitsPerSampleOutput;
    if (!LHDC_PRIVATE(pcm_sample_rate_supported)(nSampleRate))
        return LHDC_CONST(AR_INIT_INVALID_SAMPLE_RATE);
    pstLhdcDistEffect->nSampleRate = nSampleRate;
    pstLhdcDistEffect->nDistEffectBlkSampleNum = LHDC_CONST(AR_BLOCK_SAMPLES);
    pstLhdcDistEffect->uiTagBegin = LHDC_AR_WORKSPACE_TAG;
    pstLhdcDistEffect->nLhdcDistEffectState = 1;
    pstLhdcDistEffect->uiTagEnd = LHDC_AR_WORKSPACE_TAG;
    return LHDC_CONST(STATUS_OK);
}

/* Distance effect: retrieve processed floating-point PCM. */
int32_t LHDC_FN(lhdc_dist_effect_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcDistEffect) {
    if (nSampleNum <= 0 || !pucMemLhdcDistEffect)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_DIST_EFFECT pstLhdcDistEffect = (void *)pucMemLhdcDistEffect;
    LHDC_PRIVATE(clip_pcm_channels)
    (pstLhdcDistEffect->pfPcmOutput, pstLhdcDistEffect->nChNumOutput, nSampleNum);
    return LHDC_CONST(STATUS_OK);
}

/* Distance effect: buffer and process input floating-point PCM. */
int32_t LHDC_FN(lhdc_dist_effect_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                              float *pfPcmIn3, float *pfPcmOut0, float *pfPcmOut1,
                                              float *pfPcmOut2, float *pfPcmOut3,
                                              int32_t nSampleNum, uint8_t *pucMemLhdcDistEffect) {
    if (nSampleNum <= 0 || !pfPcmIn0 || !pfPcmIn1 || !pfPcmIn2 || !pfPcmIn3 || !pfPcmOut0 ||
        !pfPcmOut1 || !pfPcmOut2 || !pfPcmOut3 || !pucMemLhdcDistEffect)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_DIST_EFFECT pstLhdcDistEffect = (void *)pucMemLhdcDistEffect;
    pstLhdcDistEffect->pfPcm[0] = pfPcmIn0;
    pstLhdcDistEffect->pfPcm[1] = pfPcmIn1;
    pstLhdcDistEffect->pfPcm[2] = pfPcmIn2;
    pstLhdcDistEffect->pfPcm[3] = pfPcmIn3;
    pstLhdcDistEffect->pfPcmOutput[0] = pfPcmOut0;
    pstLhdcDistEffect->pfPcmOutput[1] = pfPcmOut1;
    pstLhdcDistEffect->pfPcmOutput[2] = pfPcmOut2;
    pstLhdcDistEffect->pfPcmOutput[3] = pfPcmOut3;
    return LHDC_FN(lhdc_dist_effect_one_blk)(nSampleNum, pstLhdcDistEffect);
}

/* Distance effect: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_dist_effect_mem_req_bytes)(void) {
    return 16 + sizeof(__ST_LHDC_DIST_EFFECT);
}
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

/* Distance effect: process complete buffered audio blocks. */
int32_t LHDC_FN(lhdc_dist_effect_one_blk)(int32_t nBlkSampleNum,
                                          PST_LHDC_DIST_EFFECT pstLhdcDistEffect) {
    LHDC_FN(dist_effect_reduce)(pstLhdcDistEffect, nBlkSampleNum);
    return LHDC_CONST(STATUS_OK);
}
