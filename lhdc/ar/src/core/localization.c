#include "include/pcm_format.h"
#include <math.h>
#include <string.h>

/* Spatial audio: initialize configuration and runtime state. */
int32_t LHDC_FN(lhdc_ar_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                              int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                              int32_t nSampleRate, uint32_t uiWorkingHoldSamples,
                              uint8_t *pucMemLhdcAr) {
    if (!pucMemLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    memset(pstLhdcAr, 0, sizeof(*pstLhdcAr));
    if ((uint32_t)nChNumInput >= 5)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcAr->nChNumInput = nChNumInput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleInput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcAr->nBitsPerSampleInput = nBitsPerSampleInput;
    if ((uint32_t)nChNumOutput >= 5)
        return LHDC_CONST(AR_INIT_INVALID_CHANNELS);
    pstLhdcAr->nChNumOutput = nChNumOutput;
    if (!LHDC_PRIVATE(pcm_bit_depth_supported)(nBitsPerSampleOutput))
        return LHDC_CONST(AR_INIT_INVALID_BIT_DEPTH);
    pstLhdcAr->nBitsPerSampleOutput = nBitsPerSampleOutput;
    pstLhdcAr->nSampleRate = nSampleRate;
    pstLhdcAr->uiWorkingHoldSamples = uiWorkingHoldSamples;
    for (int32_t i = 0; i < 4; ++i)
        pstLhdcAr->stChPosCur[i].nAzimuthIdx = pstLhdcAr->pnChAzimuth[i];
    LHDC_FN(lhdc_ar_select_coef)(0, pstLhdcAr);
    LHDC_FN(hrir_load_default_directions)(pstLhdcAr);
    LHDC_FN(hrir_upconv_alloc)(0, &pstLhdcAr->uiHrirUpolsMemBytes);
    pstLhdcAr->pucHrirUpolsMem = pucMemLhdcAr + sizeof(*pstLhdcAr);
    LHDC_FN(hrir_upconv_alloc)(pstLhdcAr->pucHrirUpolsMem, &pstLhdcAr->uiHrirUpolsMemBytes);
    pstLhdcAr->nLhdcArState = 1;
    pstLhdcAr->uiLhdcArTag = LHDC_AR_WORKSPACE_TAG;
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_ar_mem_req_bytes)(void) {
    uint32_t size;
    LHDC_FN(hrir_upconv_alloc)(0, &size);
    return size + sizeof(__ST_LHDC_AR) + 16;
}

/* Spatial audio: retrieve processed floating-point PCM. */
int32_t LHDC_FN(lhdc_ar_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcAr) {
    if (!pucMemLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    if (pstLhdcAr->nLhdcArState != 1)
        return LHDC_CONST(AR_NOT_INITIALIZED);
    LHDC_PRIVATE(clip_pcm_channels)
    (pstLhdcAr->pfPcmOutput, pstLhdcAr->nChNumOutput, nSampleNum);
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: update gyroscope orientation parameters. */
int32_t LHDC_FN(lhdc_ar_set_gyro_pos)(int32_t x_cur, int32_t y_cur, int32_t z_cur,
                                      uint8_t *pucMemLhdcAr) {
    if (!pucMemLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if ((uint32_t)(y_cur - 301) <= 2998u || (uint32_t)x_cur >= LHDC_CONST(AR_GYRO_FULL_TURN) ||
        (uint32_t)y_cur >= LHDC_CONST(AR_GYRO_FULL_TURN) ||
        (uint32_t)z_cur >= LHDC_CONST(AR_GYRO_FULL_TURN))
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    int32_t dx = pstLhdcAr->stGyroPosCur.x - x_cur, dy = pstLhdcAr->stGyroPosCur.y - y_cur;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx > 1799)
        dx = LHDC_CONST(AR_GYRO_FULL_TURN) - dx;
    if (dy > 1799)
        dy = LHDC_CONST(AR_GYRO_FULL_TURN) - dy;
    if (dx >= 20 || dy >= 300) {
        pstLhdcAr->stGyroPosCur.x = x_cur / 20 * 20;
        pstLhdcAr->stGyroPosCur.y = y_cur / 300 * 300;
        pstLhdcAr->stGyroPosCur.z = z_cur / 10 * 10;
        int32_t elev = (y_cur > 600 ? y_cur - LHDC_CONST(AR_GYRO_FULL_TURN) : y_cur) / 300;
        int32_t opp = elev > 0 ? elev + 1 : -elev;
        if (elev < 0)
            elev = 1 - elev;
        for (int32_t i = 0; i < pstLhdcAr->nChNumInput; ++i)
            if (pstLhdcAr->pnChAzimuth[i] >= 0) {
                int32_t az = pstLhdcAr->pnChAzimuth[i] + x_cur / 20;
                if (az >= 180)
                    az -= 180;
                int32_t el = i <= 2 ? elev : ((i & ~1) == 4 ? opp : -1);
                LHDC_FN(lhdc_ar_set_ch_pos)(i, az, el, i > 1, pstLhdcAr);
            }
    }
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: buffer and process input floating-point PCM. */
int32_t LHDC_FN(lhdc_ar_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                     float *pfPcmIn3, float *pfPcmOut0, float *pfPcmOut1,
                                     int32_t nSampleNum, uint8_t *pucMemLhdcAr) {
    if (!pfPcmIn0 || !pfPcmIn1 || !pfPcmIn2 || !pfPcmIn3 || !pfPcmOut0 || !pfPcmOut1 ||
        !pucMemLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    if (pstLhdcAr->nLhdcArState != 1)
        return LHDC_CONST(AR_NOT_INITIALIZED);
    pstLhdcAr->pfPcm[0] = pfPcmIn0;
    pstLhdcAr->pfPcm[1] = pfPcmIn1;
    pstLhdcAr->pfPcm[2] = pfPcmIn2;
    pstLhdcAr->pfPcm[3] = pfPcmIn3;
    pstLhdcAr->pfPcmOutput[0] = pfPcmOut0;
    pstLhdcAr->pfPcmOutput[1] = pfPcmOut1;
    LHDC_FN(lhdc_ar_one_blk)(nSampleNum, pstLhdcAr);
    if (pstLhdcAr->nNewPosChange &&
        pstLhdcAr->uiWorkingHoldSamples <= pstLhdcAr->uiWorkingSamples) {
        pstLhdcAr->nNewPosChange = 0;
        int32_t changed = 0;
        LHDC_FN(hrir_update_directions)(&changed, pstLhdcAr);
        if (changed) {
            pstLhdcAr->uiWorkingSamples = 0;
            pstLhdcAr->nResetRampUpFilterData = 1;
        }
    }
    return LHDC_CONST(STATUS_OK);
}
#include "lhdc_log.h"
#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "include/ar_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LHDC_AR_WORKSPACE_TAG UINT32_C(0x25109908)

/* Spatial audio: select HRIR coefficients for the current direction. */
int32_t LHDC_FN(lhdc_ar_select_coef)(int32_t nCoefIdx, PST_LHDC_AR pstLhdcAr) {
    if (!pstLhdcAr || (nCoefIdx != 0 && nCoefIdx != 1))
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    pstLhdcAr->pfLhdcArHrirCoefCur = lhdc_ar_hrir_table;
    return LHDC_CONST(STATUS_OK);
}

/* Load binaural filter coefficients from the HRIR table using default channel positions. */
int32_t LHDC_FN(hrir_load_default_directions)(PST_LHDC_AR pstLhdcAr) {
    if (!pstLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    for (int32_t i = 0; i < pstLhdcAr->nChNumInput; ++i) {
        int32_t index = pstLhdcAr->stChPosCur[i].nAzimuthIdx * 512 +
                        pstLhdcAr->stChPosCur[i].nElevationIdx * 92160;
        pstLhdcAr->pfFir256CoefCur[i][0] = pstLhdcAr->pfLhdcArHrirCoefCur + index;
        pstLhdcAr->pfFir256CoefCur[i][1] = pstLhdcAr->pfLhdcArHrirCoefCur + index + 256;
    }
    return LHDC_CONST(STATUS_OK);
}

/* Refresh HRIR coefficients only when channel positions change and report the change flag. */
int32_t LHDC_FN(hrir_update_directions)(int32_t *pnHrirCoefUpdate, PST_LHDC_AR pstLhdcAr) {
    if (!pnHrirCoefUpdate || !pstLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    int32_t changed = 0;
    for (int32_t i = 0; i < pstLhdcAr->nChNumInput; ++i)
        if (memcmp(pstLhdcAr->stChPosCur + i, pstLhdcAr->stChPosNext + i, sizeof(ST_LHDC_AR_POS))) {
            changed = 1;
            pstLhdcAr->stChPosCur[i] = pstLhdcAr->stChPosNext[i];
        }
    if (changed)
        LHDC_FN(hrir_load_default_directions)(pstLhdcAr);
    *pnHrirCoefUpdate = changed;
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: process one audio frame and save history. */
int32_t LHDC_FN(lhdc_ar_one_frame)(int32_t nFrameSampleOffset, int32_t nFrameSampleNum,
                                   PST_LHDC_AR pstLhdcAr) {
    (void)nFrameSampleNum;
    if (!pstLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    LHDC_FN(hrir_upconv_process)(pstLhdcAr, nFrameSampleOffset);
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: process complete buffered audio blocks. */
int32_t LHDC_FN(lhdc_ar_one_blk)(int32_t nBlkSampleNum, PST_LHDC_AR pstLhdcAr) {
    if (!pstLhdcAr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if (nBlkSampleNum <= 0) {
        return LHDC_CONST(STATUS_OK);
    }
    LHDC_FN(lhdc_ar_one_frame)(0, nBlkSampleNum, pstLhdcAr);
    if (pstLhdcAr->uiWorkingSamples >> 28 < 15)
        pstLhdcAr->uiWorkingSamples += nBlkSampleNum;
    return LHDC_CONST(STATUS_OK);
}

/* Spatial audio: update the specified channel's position. */
int32_t LHDC_FN(lhdc_ar_set_ch_pos)(int32_t nChId, int32_t nAzimuthIdx, int32_t nElevationIdx,
                                    int32_t nUpdatePosChange, PST_LHDC_AR pstLhdcAr) {
    if (pstLhdcAr == (PST_LHDC_AR)0x0) {
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    }
    if (pstLhdcAr->nLhdcArState != 1) {
        return LHDC_CONST(AR_NOT_INITIALIZED);
    }
    if (3 < (uint32_t)nChId) {
        return LHDC_CONST(AR_INVALID_CHANNEL_INDEX);
    }
    if (0xb3 < (uint32_t)nAzimuthIdx) {
        return LHDC_CONST(AR_INVALID_AZIMUTH_INDEX);
    }
    pstLhdcAr->stChPosNext[(uint32_t)nChId].nAzimuthIdx = nAzimuthIdx;
    if (2 < (uint32_t)nElevationIdx) {
        return LHDC_CONST(AR_INVALID_ELEVATION_INDEX);
    }
    pstLhdcAr->stChPosNext[(uint32_t)nChId].nElevationIdx = nElevationIdx;
    if (nUpdatePosChange != 0) {
        pstLhdcAr->nNewPosChange = 1;
    }
    return LHDC_CONST(STATUS_OK);
}
