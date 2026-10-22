#include "../../core/include/pcm_format.h"
#include <math.h>
#include <string.h>
/* Reverb: initialize configuration and runtime state. */
int32_t LHDC_FN(lhdc_reverb_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                  int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                  int32_t nSampleRate, uint8_t *pucMemLhdcReverb) {
    if (!pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    memset(pstLhdcReverb, 0, sizeof(*pstLhdcReverb));
    if (nChNumInput != 3)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcReverb->nChNumInput = nChNumInput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleInput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcReverb->nBitsPerSampleInput = nBitsPerSampleInput;
    if (nChNumOutput != 2)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcReverb->nChNumOutput = nChNumOutput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleOutput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcReverb->nBitsPerSampleOutput = nBitsPerSampleOutput;
    if (!LHDC_PRIVATE(pcm_sample_rate_supported)(nSampleRate))
        return LHDC_CONST(AR_INIT_INVALID_SAMPLE_RATE);
    pstLhdcReverb->nSampleRate = nSampleRate;
    pstLhdcReverb->nReverbBlkSampleNum = LHDC_CONST(AR_BLOCK_SAMPLES);
    uint32_t wscale = LHDC_AR_REVERB_W_GAIN_BITS;
    memcpy(&pstLhdcReverb->wscale, &wscale, 4);
    pstLhdcReverb->uiUpolsMemBytes = LHDC_FN(reverb_workspace_bytes)();
    pstLhdcReverb->pucUpolsMem = pucMemLhdcReverb + sizeof(*pstLhdcReverb);
    LHDC_FN(init_reverb_memory)(pstLhdcReverb->pucUpolsMem, pstLhdcReverb->uiUpolsMemBytes);
    pstLhdcReverb->uiTagBegin = LHDC_AR_WORKSPACE_TAG;
    pstLhdcReverb->nLhdcReverbState = 1;
    pstLhdcReverb->uiTagEnd = LHDC_AR_WORKSPACE_TAG;
    return LHDC_CONST(STATUS_OK);
}

/* Reverb: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_reverb_mem_req_bytes)(void) {
    return LHDC_FN(reverb_workspace_bytes)() + sizeof(__ST_LHDC_REVERB) + 16;
}

/* Reverb: retrieve processed floating-point PCM. */
int32_t LHDC_FN(lhdc_reverb_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcReverb) {
    if (nSampleNum <= 0 || !pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    LHDC_PRIVATE(clip_pcm_channels)
    (pstLhdcReverb->pfPcmOutput, pstLhdcReverb->nChNumOutput, nSampleNum);
    return LHDC_CONST(STATUS_OK);
}

/* Reverb: update gyroscope orientation parameters. */
int32_t LHDC_FN(lhdc_reverb_set_gyro_pos)(int32_t x_cur, int32_t y_cur, int32_t z_cur,
                                          uint8_t *pucMemLhdcReverb) {
    if (!pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if ((uint32_t)(y_cur - 301) <= 2998u || (uint32_t)x_cur >= LHDC_CONST(AR_GYRO_FULL_TURN) ||
        (uint32_t)y_cur >= LHDC_CONST(AR_GYRO_FULL_TURN) ||
        (uint32_t)z_cur >= LHDC_CONST(AR_GYRO_FULL_TURN))
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    pstLhdcReverb->phy_cur = (float)x_cur / 10.0f;
    pstLhdcReverb->theta_cur = (float)y_cur / 10.0f;
    return LHDC_CONST(STATUS_OK);
}

/* Reverb: buffer and process input floating-point PCM. */
int32_t LHDC_FN(lhdc_reverb_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                         float *pfPcmOut0, float *pfPcmOut1, int32_t nSampleNum,
                                         uint8_t *pucMemLhdcReverb) {
    if (nSampleNum <= 0 || !pfPcmIn0 || !pfPcmIn1 || !pfPcmIn2 || !pfPcmOut0 || !pfPcmOut1 ||
        !pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    pstLhdcReverb->pfPcm[0] = pfPcmIn0;
    pstLhdcReverb->pfPcm[1] = pfPcmIn1;
    pstLhdcReverb->pfPcm[2] = pfPcmIn2;
    pstLhdcReverb->pfPcmOutput[0] = pfPcmOut0;
    pstLhdcReverb->pfPcmOutput[1] = pfPcmOut1;
    return LHDC_FN(lhdc_reverb_one_blk)(nSampleNum, pstLhdcReverb);
}

#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

/* Reverb: process complete buffered audio blocks. */
int32_t LHDC_FN(lhdc_reverb_one_blk)(int32_t nBlkSampleNum, PST_LHDC_REVERB pstLhdcReverb) {
    LHDC_FN(reverb_effect_reduce)(pstLhdcReverb, nBlkSampleNum);
    return LHDC_CONST(STATUS_OK);
}
