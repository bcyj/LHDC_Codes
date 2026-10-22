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
#include "parameter_defaults.h"

/* AR parameters: initialize defaults. */
int32_t LHDC_FN(lhdc_ar_params_default)(uint8_t *pucMemLhdcArParams) {
    if (!pucMemLhdcArParams)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    memcpy(pucMemLhdcArParams, lhdc_ar_default_parameter_bytes,
           sizeof lhdc_ar_default_parameter_bytes);
    return LHDC_CONST(STATUS_OK);
}

/* AR parameters: validate and write the parameter block. */
int32_t LHDC_FN(lhdc_ar_params_set)(PLHDC_AR_API_BLK pLhdcArApiBlk) {
    if (!pLhdcArApiBlk)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    uint8_t *pucMemLhdcArParams = pLhdcArApiBlk->gpucArParamMem;
    uint8_t *pucMemLhdcAr = pLhdcArApiBlk->gpucLhdcArMem;
    uint8_t *pucMemLhdcUpmix = pLhdcArApiBlk->gpucArUpmixMem;
    uint8_t *pucMemLhdcDistEffect = pLhdcArApiBlk->gpucArDistEffectMem;
    uint8_t *pucMemLhdcReverb = pLhdcArApiBlk->gpucArReverbMem;
    if (!pucMemLhdcArParams || !pucMemLhdcAr || !pucMemLhdcUpmix || !pucMemLhdcDistEffect ||
        !pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR_PARAMS pstLhdcArParams = (void *)pucMemLhdcArParams;
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    PST_LHDC_DIST_EFFECT pstLhdcDistEffect = (void *)pucMemLhdcDistEffect;
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    pstLhdcDistEffect->dreal = pstLhdcArParams->fDistEffect_dreal;
    pstLhdcDistEffect->dref = pstLhdcArParams->fDistEffect_dref;
    pstLhdcDistEffect->attack_t = pstLhdcArParams->fDistEffect_attack_t;
    if (pstLhdcArParams->nChangeHrirCoefChAzimuth) {
        for (int32_t i = 0; i < 8; ++i)
            pstLhdcAr->pnChAzimuth[i] = pstLhdcArParams->pnArChAzimuth[i] < 0
                                            ? -1
                                            : (uint32_t)pstLhdcArParams->pnArChAzimuth[i] / 20;
        LHDC_FN(lhdc_ar_select_coef)(pstLhdcArParams->nHrirCoefSel != 0, pstLhdcAr);
        LHDC_FN(hrir_load_default_directions)(pstLhdcAr);
    }
    memcpy(pstLhdcAr->pfCh51Gain, pstLhdcArParams->pfArCh51Gain, sizeof pstLhdcAr->pfCh51Gain);
    memcpy(pstLhdcAr->pfCh51GainPost, pstLhdcArParams->pfArCh51GainPost,
           sizeof pstLhdcAr->pfCh51GainPost);
    pstLhdcReverb->dreal = pstLhdcArParams->fDistEffect_dreal;
    pstLhdcReverb->dref = pstLhdcArParams->fDistEffect_dref;
    pstLhdcReverb->attack_t = pstLhdcArParams->fDistEffect_attack_t;
    memcpy(&pLhdcArApiBlk->gnReverbGain, &pstLhdcArParams->fLhdcAr_reverb_gain, 8);
    return LHDC_CONST(STATUS_OK);
}

/* AR parameters: read the parameter block. */
int32_t LHDC_FN(lhdc_ar_params_get)(uint8_t *pucMemLhdcArParams, PLHDC_AR_API_BLK pLhdcArApiBlk) {
    if (!pLhdcArApiBlk)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    uint8_t *pucMemLhdcAr = pLhdcArApiBlk->gpucLhdcArMem;
    uint8_t *pucMemLhdcUpmix = pLhdcArApiBlk->gpucArUpmixMem;
    uint8_t *pucMemLhdcDistEffect = pLhdcArApiBlk->gpucArDistEffectMem;
    uint8_t *pucMemLhdcReverb = pLhdcArApiBlk->gpucArReverbMem;
    if (!pucMemLhdcArParams || !pucMemLhdcAr || !pucMemLhdcUpmix || !pucMemLhdcDistEffect ||
        !pucMemLhdcReverb)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    PST_LHDC_AR_PARAMS pstLhdcArParams = (void *)pucMemLhdcArParams;
    PST_LHDC_AR pstLhdcAr = (void *)pucMemLhdcAr;
    PST_LHDC_DIST_EFFECT pstLhdcDistEffect = (void *)pucMemLhdcDistEffect;
    PST_LHDC_REVERB pstLhdcReverb = (void *)pucMemLhdcReverb;
    pstLhdcArParams->uiTagBegin = pstLhdcArParams->uiTagEnd = 0x25109908;
    pstLhdcArParams->uiVersion = 0x10000;
    pstLhdcArParams->fDistEffect_dreal = pstLhdcDistEffect->dreal;
    pstLhdcArParams->fDistEffect_dref = pstLhdcDistEffect->dref;
    pstLhdcArParams->fDistEffect_attack_t = pstLhdcDistEffect->attack_t;
    pstLhdcArParams->fDistEffect_gain_dist_i = pstLhdcDistEffect->gain_dist_i;
    pstLhdcArParams->fDistEffect_gain_mem = pstLhdcDistEffect->gain_mem;
    pstLhdcArParams->nChangeHrirCoefChAzimuth = 0;
    for (int32_t i = 0; i < 8; ++i)
        pstLhdcArParams->pnArChAzimuth[i] =
            pstLhdcAr->pnChAzimuth[i] < 0 ? -1
                                          : (int32_t)((uint32_t)pstLhdcAr->pnChAzimuth[i] * 20);
    memcpy(pstLhdcArParams->pfArCh51Gain, pstLhdcAr->pfCh51Gain,
           sizeof pstLhdcArParams->pfArCh51Gain);
    memcpy(pstLhdcArParams->pfArCh51GainPost, pstLhdcAr->pfCh51GainPost,
           sizeof pstLhdcArParams->pfArCh51GainPost);
    pstLhdcArParams->fReverb_gain_reverb_i = pstLhdcReverb->gain_reverb_i;
    pstLhdcArParams->fReverb_phy_cur = pstLhdcReverb->phy_cur;
    pstLhdcArParams->fReverb_wscale = pstLhdcReverb->wscale;
    pstLhdcArParams->fReverb_theta_cur = pstLhdcReverb->theta_cur;
    pstLhdcArParams->fReverb_gain_mem_r = pstLhdcReverb->gain_mem_r;
    memcpy(&pstLhdcArParams->fLhdcAr_reverb_gain, &pLhdcArApiBlk->gnReverbGain, 8);
    return LHDC_CONST(STATUS_OK);
}

/* AR parameters: calculate required workspace bytes. */
uint32_t LHDC_FN(lhdc_ar_params_mem_req_bytes)(void) {
    return sizeof(__ST_LHDC_AR_PARAMS);
}
