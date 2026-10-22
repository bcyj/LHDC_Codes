/* DSP, AR, and Upmix layouts shared by V4/V5.
 * Version-specific encoder and FFT state layouts remain in each version's types directory. */
#ifndef LHDC_AR_TYPES_H
#define LHDC_AR_TYPES_H
#include "../../../common/include/types/lhdc_base_types.h"

typedef struct __ST_LHDC_DIST_EFFECT __ST_LHDC_DIST_EFFECT, *P__ST_LHDC_DIST_EFFECT;

struct __ST_LHDC_DIST_EFFECT {
    uint32_t uiTagBegin;
    float *pfPcm[4];
    float *pfPcmOutput[4];
    int32_t nChNumInput;
    int32_t nBitsPerSampleInput;
    int32_t nChNumOutput;
    int32_t nBitsPerSampleOutput;
    int32_t nSampleRate;
    int32_t nDistEffectBlkSampleNum;
    float dist_mem_fl[256];
    float dist_mem_fr[256];
    float dist_mem_c[256];
    float dist_mem_lfe[256];
    float dreal;
    float dref;
    float attack_t;
    float gain_dist_i;
    float gain_mem;
    int32_t nLhdcDistEffectState;
    uint32_t uiTagEnd;
};

typedef struct __ST_LHDC_DIST_EFFECT *PST_LHDC_DIST_EFFECT;

typedef struct st_cc_buf st_cc_buf, *Pst_cc_buf;

struct st_cc_buf {
    float *data;
    struct st_cc_buf *next_buf;
};

typedef struct st_cc_buf cc_buf;

typedef struct cc_buf_list cc_buf_list, *Pcc_buf_list;

struct cc_buf_list {
    cc_buf *head_buf;
    cc_buf *tail_buf;
    cc_buf *list_mem;
    uint32_t data_size;
    uint32_t buf_count;
};

typedef enum upconv_mem_content_index {
    INPUT_NEW_DATA_INDEX = 0,
    W_PREV_FRAME_INDEX = 1,
    X_PREV_FRAME_INDEX = 2,
    Y_PREV_FRAME_INDEX = 3,
    W_LIST_INDEX = 4,
    W_LIST_MEM_INDEX = 5,
    W_DATA_MEM_INDEX = 6,
    X_LIST_INDEX = 7,
    X_LIST_MEM_INDEX = 8,
    X_DATA_MEM_INDEX = 9,
    Y_LIST_INDEX = 10,
    Y_LIST_MEM_INDEX = 11,
    Y_DATA_MEM_INDEX = 12,
    L_SUM_INDEX = 13,
    R_SUM_INDEX = 14,
    ALL_CONTENT_NUM = 15
} upconv_mem_content_index;

typedef struct __ST_LHDC_AR __ST_LHDC_AR, *P__ST_LHDC_AR;

typedef struct __ST_LHDC_AR_GYRO_POS __ST_LHDC_AR_GYRO_POS, *P__ST_LHDC_AR_GYRO_POS;

typedef struct __ST_LHDC_AR_GYRO_POS ST_LHDC_AR_GYRO_POS;

typedef struct __ST_LHDC_AR_POS __ST_LHDC_AR_POS, *P__ST_LHDC_AR_POS;

typedef struct __ST_LHDC_AR_POS ST_LHDC_AR_POS;

struct __ST_LHDC_AR_POS {
    int32_t nAzimuthIdx;
    int32_t nElevationIdx;
};

struct __ST_LHDC_AR_GYRO_POS {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct __ST_LHDC_AR {
    float *pfPcm[4];
    float *pfPcmOutput[2];
    const float *pfFir256CoefCur[4][2];
    int32_t nChNumInput;
    int32_t nBitsPerSampleInput;
    int32_t nChNumOutput;
    int32_t nBitsPerSampleOutput;
    int32_t nSampleRate;
    uint32_t uiWorkingHoldSamples;
    ST_LHDC_AR_GYRO_POS stGyroPosCur;
    ST_LHDC_AR_POS stChPosCur[4];
    ST_LHDC_AR_POS stChPosNext[4];
    int32_t nNewPosChange;
    uint32_t uiWorkingSamples;
    int32_t nResetRampUpFilterData;
    int32_t pnChAzimuth[8];
    float pfCh51Gain[2][8];
    float pfCh51GainPost[8];
    const float *pfLhdcArHrirCoefCur;
    uint32_t uiHrirUpolsMemBytes;
    uint8_t *pucHrirUpolsMem;
    int32_t nLhdcArState;
    uint32_t uiLhdcArTag;
};

typedef struct __ST_LHDC_AR *PST_LHDC_AR;

typedef struct __ST_LHDC_UPMIX __ST_LHDC_UPMIX, *P__ST_LHDC_UPMIX;

struct __ST_LHDC_UPMIX {
    uint32_t uiTagBegin;
    float *pfPcm[2];
    float *pfPcmOutput[4];
    int32_t nChNumInput;
    int32_t nBitsPerSampleInput;
    int32_t nChNumOutput;
    int32_t nBitsPerSampleOutput;
    int32_t nSampleRate;
    int32_t nUpmixBlkSampleNum;
    uint32_t uiLibWorkspaceSize;
    uint8_t *pucLibWorkspace;
    int32_t nLhdcUpmixState;
    uint32_t uiTagEnd;
};

typedef struct __ST_LHDC_UPMIX *PST_LHDC_UPMIX;

typedef struct __stLHDC_AR_API_BLK __stLHDC_AR_API_BLK, *P__stLHDC_AR_API_BLK;

struct __stLHDC_AR_API_BLK {
    uint32_t ulLhdcArBlkTagBegin;
    float *gpfPcm[2];
    float *gpfPcmUpmix[4];
    float *gpfPcmDistEff[4];
    float *gpfPcmDst[2];
    float *gpfPcmReverb[3];
    int32_t gnArInPcmChNum;
    int32_t gnArInPcmBitsPerSample;
    int32_t gnArOutPcmChNum;
    int32_t gnArOutPcmBitsPerSample;
    uint8_t *gpucArUpmixMem;
    int32_t gnArUpmixEnable;
    int32_t gnArUpmixOutChNum;
    int32_t gnArUpmixInNumSampleQueue;
    uint8_t *gpucArDistEffectMem;
    int32_t gnArDistEffectEnable;
    int32_t gnArDistEffectInChNum;
    int32_t gnArDistEffectOutChNum;
    int32_t gnArDistEffectInNumSampleQueue;
    uint8_t *gpucLhdcArMem;
    int32_t gnLhdcAr3DLocalizeDownMixEnable;
    int32_t gnAr3dDownMixInChNum;
    int32_t gnAr3dDownMixOutChNum;
    int32_t gnAr3dDownMixInNumSampleQueue;
    uint8_t *gpucArReverbMem;
    int32_t gnArReverbEnable;
    int32_t gnArReverbInChNum;
    int32_t gnArReverbOutChNum;
    int32_t gnArReverbInNumSampleQueue;
    float gnReverbGain;
    float gn3dDownMixGain;
    int32_t gnAr3dOutputInNumSampleQueue;
    uint8_t *gpucArParamMem;
    int32_t gnLhdcArResAcquireOK;
    int32_t gnLhdcArInitOK;
    lhdc_ulong_t gulFrameCnt;
    uint32_t ulLhdcArBlkTagEnd;
};

typedef struct __stLHDC_AR_API_BLK *PLHDC_AR_API_BLK;

typedef enum upmix_content_index {
    UPMIX_PROCESS_PARAMETER = 0,
    UPMIX_IIR_FILTER_IN_BUF = 1,
    UPMIX_IIR_FILTER_OUT_BUF = 2,
    UPMIX_TEMP_BUF_AREA_1 = 3,
    UPMIX_TEMP_BUF_AREA_2 = 4,
    UPMIX_COMP_FILTER_Z_BUF = 5,
    UPMIX_SHLV_FILTER_Z_BUF = 6,
    UPMIX_BANK_RESULT = 7,
    UPMIX_PRE_POWER_VALUE_BUF = 8,
    UPMIX_PROTECT_AREA = 9,
    UPMIX_CONTENT_INDEX_ALL_CONTENT_NUM = 10
} upmix_content_index;

typedef enum upmix_operate_option {
    ASK_WORKSPACE_SIZE = 0,
    CONFIG_MEMORY_ADDRESS = 1,
    RESET_TEMP_BUF_ONLY = 2,
    ALL_OPERATION_NUM = 3
} upmix_operate_option;

typedef struct upmix_process_parameter upmix_process_parameter, *Pupmix_process_parameter;

struct upmix_process_parameter {
    float power_alpha;
    float gain_filter_bank;
};
#include "parameters.h"

typedef struct __ST_LHDC_REVERB __ST_LHDC_REVERB, *P__ST_LHDC_REVERB;

struct __ST_LHDC_REVERB {
    uint32_t uiTagBegin;
    float *pfPcm[3];
    float *pfPcmOutput[2];
    int32_t nChNumInput;
    int32_t nBitsPerSampleInput;
    int32_t nChNumOutput;
    int32_t nBitsPerSampleOutput;
    int32_t nSampleRate;
    int32_t nReverbBlkSampleNum;
    float dist_mem_r_fr[256];
    float dist_mem_r_fl[256];
    float dist_mem_r_c[256];
    float dreal;
    float dref;
    float attack_t;
    float gain_reverb_i;
    float phy_cur;
    float wscale;
    float theta_cur;
    float gain_mem_r;
    uint32_t uiUpolsMemBytes;
    uint8_t *pucUpolsMem;
    int32_t nLhdcReverbState;
    uint32_t uiTagEnd;
};

typedef struct __ST_LHDC_REVERB *PST_LHDC_REVERB;

#endif
