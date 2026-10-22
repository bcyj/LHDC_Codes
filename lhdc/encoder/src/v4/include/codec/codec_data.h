#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_DATA_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_DATA_H
#include "../types/lhdc_native_types.h"

extern LLAC_INT alg_code_t3[4096];

extern LLAC_INT alg_code_t4[64][18];

extern LLAC_INT alg_quant_t1[64][17];

extern LLAC_FLOAT alg_signal_t1[8][32];

extern LLAC_FLOAT alg_signal_t2[8][32];

extern LLAC_INT alg_signal_t3[16][11];

extern LLAC_INT alg_signal_t4[8][17];

extern LLAC_INT alg_signal_t5[8];

extern LLAC_INT alg_signal_t6[8];

extern LLAC_FLOAT alg_signal_t7[9];

extern LLAC_FLOAT alg_signal_t8[17];

extern LLAC_FLOAT alg_signal_t9[18];

extern LLAC_FLOAT alg_signal_t10[8];

extern LLAC_FLOAT alg_signal_t11[4];

extern LLAC_FLOAT alg_signal_t12[4];

extern LLAC_FLOAT alg_signal_t13[6];

extern LLAC_FLOAT alg_signal_t14[2];

extern LLAC_INT alg_quant_t2[6];

extern LLAC_INT alg_quant_t3[6];

extern LLAC_INT alg_quant_t4[6];

extern LLAC_FLOAT alg_quant_t5[6];

extern LLAC_FLOAT alg_quant_t6[6];

extern LLAC_INT alg_mdct_t1[4];

extern LLAC_INT alg_mdct_t2[4];

extern LLAC_INT alg_mdct_t3[4];

extern LLAC_INT *alg_mdct_t48[4];

extern LLAC_INT *alg_mdct_t49[4];

extern LLAC_INT *alg_mdct_t58[4];

extern LLAC_INT *alg_mdct_t59[4];

extern LLAC_INT init_t1[6];

extern LLAC_INT init_t2[6];

extern LLAC_INT alg_mdct_t60[4];

extern LLAC_INT alg_mdct_t61[4];

extern LLAC_INT alg_mdct_t62[4];

extern LLAC_INT alg_mdct_t63[4];

extern LLAC_INT alg_mdct_t64[4];

extern LLAC_INT alg_mdct_t65[4];

extern LLAC_INT alg_mdct_t66[4];

extern LLAC_INT alg_mdct_t67[4];

extern LLAC_INT *alg_mdct_t68[4];

extern LLAC_INT *alg_mdct_t69[4];

extern LLAC_INT init_t3[6];

extern LLAC_INT init_t4[6];

extern LLAC_INT init_t5[6];

extern LLAC_INT init_t6[6];

extern LLAC_INT init_t7[6];

extern LLAC_INT init_t8[6];

extern LLAC_INT init_t9[6];

extern LLAC_INT init_t10[6];

extern LLAC_INT alg_code_t5[8][18];

extern LLAC_INT alg_code_t6[2][9];

extern LLAC_FLOAT init_t121[80];

extern LLAC_FLOAT init_t122[160];

extern LLAC_FLOAT init_t123[240];

extern LLAC_FLOAT init_t124[320];

extern LLAC_FLOAT init_t125[480];

extern LLAC_FLOAT init_t126[480];

extern LLAC_FLOAT init_t127[960];

extern LLAC_FLOAT *init_t13[2][6];

extern LLAC_FLOAT *init_t11[2][6];

extern LLAC_FLOAT *init_t12[2][6];

extern LLAC_INT init_t191[40];

extern LLAC_INT init_t192[51];

extern LLAC_INT init_t193[53];

extern LLAC_INT init_t194[55];

extern LLAC_INT init_t195[56];

extern LLAC_INT init_t196[56];

extern LLAC_INT init_t197[59];

extern LLAC_INT *init_t178[6];

extern LLAC_INT *init_t179[6];

extern LLAC_INT *init_t188[6];

extern LLAC_INT *init_t189[6];

extern LLAC_INT *init_t198[6];

extern LLAC_INT *init_t199[6];

extern LLAC_INT init_t20[63];

extern LLAC_FLOAT init_t128[440];

extern LLAC_INT Table_maxB[158];

extern const double ITUR468_48000[214];

extern const double ITUR468_44100[233];

extern const double ITUR468_96000[107];

extern const float ms_bits_err[24];

extern const float ms_int_err[8];

extern const QUALITY_PRESET def_q_set16[16];

extern const QUALITY_PRESET def_q_set24[16];

extern float gpfLhdcArHrirCoefB[3][180][2][256];

extern const double bank_00_44k_a[10];

extern const double bank_00_44k_b[10];

extern const double bank_01_44k_a[10];

extern const double bank_01_44k_b[10];

extern const double bank_02_44k_a[10];

extern const double bank_02_44k_b[10];

extern const double bank_03_44k_a[10];

extern const double bank_03_44k_b[10];

extern const double bank_00_48k_a[10];

extern const double bank_00_48k_b[10];

extern const double bank_01_48k_a[10];

extern const double bank_01_48k_b[10];

extern const double bank_02_48k_a[10];

extern const double bank_02_48k_b[10];

extern const double bank_03_48k_a[10];

extern const double bank_03_48k_b[10];









extern const double shlv_filter_front_a_44k[];

extern const double shlv_filter_front_a_48k[];

extern const double shlv_filter_front_b_44k[];

extern const double shlv_filter_front_b_48k[];

extern const double shlv_filter_rear_a_44k[];

extern const double shlv_filter_rear_a_48k[];

extern const double shlv_filter_rear_b_44k[];

extern const double shlv_filter_rear_b_48k[];


#endif /* LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_DATA_H */
