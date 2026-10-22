#ifndef LHDC_AR_PARAMETERS_H
#define LHDC_AR_PARAMETERS_H
#include <stdint.h>
typedef struct __ST_LHDC_AR_PARAMS __ST_LHDC_AR_PARAMS, *P__ST_LHDC_AR_PARAMS;

typedef struct __ST_LHDC_AR_PARAMS *PST_LHDC_AR_PARAMS;

struct __ST_LHDC_AR_PARAMS {
    uint32_t uiTagBegin;
    uint32_t uiVersion;
    float fDistEffect_dreal;
    float fDistEffect_dref;
    float fDistEffect_attack_t;
    float fDistEffect_gain_dist_i;
    float fDistEffect_gain_mem;
    int32_t nChangeHrirCoefChAzimuth;
    int32_t pnArChAzimuth[8];
    int32_t nHrirCoefSel;
    float pfArCh51Gain[2][8];
    float pfArCh51GainPost[8];
    float fReverb_gain_reverb_i;
    float fReverb_phy_cur;
    float fReverb_wscale;
    float fReverb_theta_cur;
    float fReverb_gain_mem_r;
    float fLhdcAr_reverb_gain;
    float fLhdcAr_3d_downmix_gain;
    long long pllReserved[32];
    uint32_t uiTagEnd;
};
typedef __ST_LHDC_AR_PARAMS LhdcArParameters;
#endif
