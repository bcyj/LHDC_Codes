#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_LLAC_LLAC_CONST_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_LLAC_LLAC_CONST_H
#include "../types/lhdc_base_types.h"
#include "../types/lhdc_native_types.h"
typedef union {
    uint32_t bits[4096];
    LLAC_INT value[4096];
} V4LlacTable_alg_code_t3;
extern const V4LlacTable_alg_code_t3 v4llac_alg_code_t3;
#define alg_code_t3 (v4llac_alg_code_t3.value)
typedef union {
    uint32_t bits[1152];
    LLAC_INT value[64][18];
} V4LlacTable_alg_code_t4;
extern const V4LlacTable_alg_code_t4 v4llac_alg_code_t4;
#define alg_code_t4 (v4llac_alg_code_t4.value)
typedef union {
    uint32_t bits[1088];
    LLAC_INT value[64][17];
} V4LlacTable_alg_quant_t1;
extern const V4LlacTable_alg_quant_t1 v4llac_alg_quant_t1;
#define alg_quant_t1 (v4llac_alg_quant_t1.value)
typedef union {
    uint32_t bits[256];
    LLAC_FLOAT value[8][32];
} V4LlacTable_alg_signal_t1;
extern const V4LlacTable_alg_signal_t1 v4llac_alg_signal_t1;
#define alg_signal_t1 (v4llac_alg_signal_t1.value)
typedef union {
    uint32_t bits[256];
    LLAC_FLOAT value[8][32];
} V4LlacTable_alg_signal_t2;
extern const V4LlacTable_alg_signal_t2 v4llac_alg_signal_t2;
#define alg_signal_t2 (v4llac_alg_signal_t2.value)
typedef union {
    uint32_t bits[176];
    LLAC_INT value[16][11];
} V4LlacTable_alg_signal_t3;
extern const V4LlacTable_alg_signal_t3 v4llac_alg_signal_t3;
#define alg_signal_t3 (v4llac_alg_signal_t3.value)
typedef union {
    uint32_t bits[136];
    LLAC_INT value[8][17];
} V4LlacTable_alg_signal_t4;
extern const V4LlacTable_alg_signal_t4 v4llac_alg_signal_t4;
#define alg_signal_t4 (v4llac_alg_signal_t4.value)
typedef union {
    uint32_t bits[8];
    LLAC_INT value[8];
} V4LlacTable_alg_signal_t5;
extern const V4LlacTable_alg_signal_t5 v4llac_alg_signal_t5;
#define alg_signal_t5 (v4llac_alg_signal_t5.value)
typedef union {
    uint32_t bits[8];
    LLAC_INT value[8];
} V4LlacTable_alg_signal_t6;
extern const V4LlacTable_alg_signal_t6 v4llac_alg_signal_t6;
#define alg_signal_t6 (v4llac_alg_signal_t6.value)
typedef union {
    uint32_t bits[9];
    LLAC_FLOAT value[9];
} V4LlacTable_alg_signal_t7;
extern const V4LlacTable_alg_signal_t7 v4llac_alg_signal_t7;
#define alg_signal_t7 (v4llac_alg_signal_t7.value)
typedef union {
    uint32_t bits[17];
    LLAC_FLOAT value[17];
} V4LlacTable_alg_signal_t8;
extern const V4LlacTable_alg_signal_t8 v4llac_alg_signal_t8;
#define alg_signal_t8 (v4llac_alg_signal_t8.value)
typedef union {
    uint32_t bits[18];
    LLAC_FLOAT value[18];
} V4LlacTable_alg_signal_t9;
extern const V4LlacTable_alg_signal_t9 v4llac_alg_signal_t9;
#define alg_signal_t9 (v4llac_alg_signal_t9.value)
typedef union {
    uint32_t bits[8];
    LLAC_FLOAT value[8];
} V4LlacTable_alg_signal_t10;
extern const V4LlacTable_alg_signal_t10 v4llac_alg_signal_t10;
#define alg_signal_t10 (v4llac_alg_signal_t10.value)
typedef union {
    uint32_t bits[4];
    LLAC_FLOAT value[4];
} V4LlacTable_alg_signal_t11;
extern const V4LlacTable_alg_signal_t11 v4llac_alg_signal_t11;
#define alg_signal_t11 (v4llac_alg_signal_t11.value)
typedef union {
    uint32_t bits[4];
    LLAC_FLOAT value[4];
} V4LlacTable_alg_signal_t12;
extern const V4LlacTable_alg_signal_t12 v4llac_alg_signal_t12;
#define alg_signal_t12 (v4llac_alg_signal_t12.value)
typedef union {
    uint32_t bits[6];
    LLAC_FLOAT value[6];
} V4LlacTable_alg_signal_t13;
extern const V4LlacTable_alg_signal_t13 v4llac_alg_signal_t13;
#define alg_signal_t13 (v4llac_alg_signal_t13.value)
typedef union {
    uint32_t bits[2];
    LLAC_FLOAT value[2];
} V4LlacTable_alg_signal_t14;
extern const V4LlacTable_alg_signal_t14 v4llac_alg_signal_t14;
#define alg_signal_t14 (v4llac_alg_signal_t14.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_alg_quant_t2;
extern const V4LlacTable_alg_quant_t2 v4llac_alg_quant_t2;
#define alg_quant_t2 (v4llac_alg_quant_t2.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_alg_quant_t3;
extern const V4LlacTable_alg_quant_t3 v4llac_alg_quant_t3;
#define alg_quant_t3 (v4llac_alg_quant_t3.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_alg_quant_t4;
extern const V4LlacTable_alg_quant_t4 v4llac_alg_quant_t4;
#define alg_quant_t4 (v4llac_alg_quant_t4.value)
typedef union {
    uint32_t bits[6];
    LLAC_FLOAT value[6];
} V4LlacTable_alg_quant_t5;
extern const V4LlacTable_alg_quant_t5 v4llac_alg_quant_t5;
#define alg_quant_t5 (v4llac_alg_quant_t5.value)
typedef union {
    uint32_t bits[6];
    LLAC_FLOAT value[6];
} V4LlacTable_alg_quant_t6;
extern const V4LlacTable_alg_quant_t6 v4llac_alg_quant_t6;
#define alg_quant_t6 (v4llac_alg_quant_t6.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t1;
extern const V4LlacTable_alg_mdct_t1 v4llac_alg_mdct_t1;
#define alg_mdct_t1 (v4llac_alg_mdct_t1.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t2;
extern const V4LlacTable_alg_mdct_t2 v4llac_alg_mdct_t2;
#define alg_mdct_t2 (v4llac_alg_mdct_t2.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t3;
extern const V4LlacTable_alg_mdct_t3 v4llac_alg_mdct_t3;
#define alg_mdct_t3 (v4llac_alg_mdct_t3.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t1;
extern const V4LlacTable_init_t1 v4llac_init_t1;
#define init_t1 (v4llac_init_t1.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t2;
extern const V4LlacTable_init_t2 v4llac_init_t2;
#define init_t2 (v4llac_init_t2.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t60;
extern const V4LlacTable_alg_mdct_t60 v4llac_alg_mdct_t60;
#define alg_mdct_t60 (v4llac_alg_mdct_t60.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t61;
extern const V4LlacTable_alg_mdct_t61 v4llac_alg_mdct_t61;
#define alg_mdct_t61 (v4llac_alg_mdct_t61.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t62;
extern const V4LlacTable_alg_mdct_t62 v4llac_alg_mdct_t62;
#define alg_mdct_t62 (v4llac_alg_mdct_t62.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t63;
extern const V4LlacTable_alg_mdct_t63 v4llac_alg_mdct_t63;
#define alg_mdct_t63 (v4llac_alg_mdct_t63.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t64;
extern const V4LlacTable_alg_mdct_t64 v4llac_alg_mdct_t64;
#define alg_mdct_t64 (v4llac_alg_mdct_t64.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t65;
extern const V4LlacTable_alg_mdct_t65 v4llac_alg_mdct_t65;
#define alg_mdct_t65 (v4llac_alg_mdct_t65.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t66;
extern const V4LlacTable_alg_mdct_t66 v4llac_alg_mdct_t66;
#define alg_mdct_t66 (v4llac_alg_mdct_t66.value)
typedef union {
    uint32_t bits[4];
    LLAC_INT value[4];
} V4LlacTable_alg_mdct_t67;
extern const V4LlacTable_alg_mdct_t67 v4llac_alg_mdct_t67;
#define alg_mdct_t67 (v4llac_alg_mdct_t67.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t3;
extern const V4LlacTable_init_t3 v4llac_init_t3;
#define init_t3 (v4llac_init_t3.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t4;
extern const V4LlacTable_init_t4 v4llac_init_t4;
#define init_t4 (v4llac_init_t4.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t5;
extern const V4LlacTable_init_t5 v4llac_init_t5;
#define init_t5 (v4llac_init_t5.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t6;
extern const V4LlacTable_init_t6 v4llac_init_t6;
#define init_t6 (v4llac_init_t6.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t7;
extern const V4LlacTable_init_t7 v4llac_init_t7;
#define init_t7 (v4llac_init_t7.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t8;
extern const V4LlacTable_init_t8 v4llac_init_t8;
#define init_t8 (v4llac_init_t8.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t9;
extern const V4LlacTable_init_t9 v4llac_init_t9;
#define init_t9 (v4llac_init_t9.value)
typedef union {
    uint32_t bits[6];
    LLAC_INT value[6];
} V4LlacTable_init_t10;
extern const V4LlacTable_init_t10 v4llac_init_t10;
#define init_t10 (v4llac_init_t10.value)
typedef union {
    uint32_t bits[144];
    LLAC_INT value[8][18];
} V4LlacTable_alg_code_t5;
extern const V4LlacTable_alg_code_t5 v4llac_alg_code_t5;
#define alg_code_t5 (v4llac_alg_code_t5.value)
typedef union {
    uint32_t bits[18];
    LLAC_INT value[2][9];
} V4LlacTable_alg_code_t6;
extern const V4LlacTable_alg_code_t6 v4llac_alg_code_t6;
#define alg_code_t6 (v4llac_alg_code_t6.value)
typedef union {
    uint32_t bits[80];
    LLAC_FLOAT value[80];
} V4LlacTable_init_t121;
extern const V4LlacTable_init_t121 v4llac_init_t121;
#define init_t121 (v4llac_init_t121.value)
typedef union {
    uint32_t bits[160];
    LLAC_FLOAT value[160];
} V4LlacTable_init_t122;
extern const V4LlacTable_init_t122 v4llac_init_t122;
#define init_t122 (v4llac_init_t122.value)
typedef union {
    uint32_t bits[240];
    LLAC_FLOAT value[240];
} V4LlacTable_init_t123;
extern const V4LlacTable_init_t123 v4llac_init_t123;
#define init_t123 (v4llac_init_t123.value)
typedef union {
    uint32_t bits[320];
    LLAC_FLOAT value[320];
} V4LlacTable_init_t124;
extern const V4LlacTable_init_t124 v4llac_init_t124;
#define init_t124 (v4llac_init_t124.value)
typedef union {
    uint32_t bits[480];
    LLAC_FLOAT value[480];
} V4LlacTable_init_t125;
extern const V4LlacTable_init_t125 v4llac_init_t125;
#define init_t125 (v4llac_init_t125.value)
typedef union {
    uint32_t bits[480];
    LLAC_FLOAT value[480];
} V4LlacTable_init_t126;
extern const V4LlacTable_init_t126 v4llac_init_t126;
#define init_t126 (v4llac_init_t126.value)
typedef union {
    uint32_t bits[960];
    LLAC_FLOAT value[960];
} V4LlacTable_init_t127;
extern const V4LlacTable_init_t127 v4llac_init_t127;
#define init_t127 (v4llac_init_t127.value)
typedef union {
    uint32_t bits[40];
    LLAC_INT value[40];
} V4LlacTable_init_t191;
extern const V4LlacTable_init_t191 v4llac_init_t191;
#define init_t191 (v4llac_init_t191.value)
typedef union {
    uint32_t bits[51];
    LLAC_INT value[51];
} V4LlacTable_init_t192;
extern const V4LlacTable_init_t192 v4llac_init_t192;
#define init_t192 (v4llac_init_t192.value)
typedef union {
    uint32_t bits[53];
    LLAC_INT value[53];
} V4LlacTable_init_t193;
extern const V4LlacTable_init_t193 v4llac_init_t193;
#define init_t193 (v4llac_init_t193.value)
typedef union {
    uint32_t bits[55];
    LLAC_INT value[55];
} V4LlacTable_init_t194;
extern const V4LlacTable_init_t194 v4llac_init_t194;
#define init_t194 (v4llac_init_t194.value)
typedef union {
    uint32_t bits[56];
    LLAC_INT value[56];
} V4LlacTable_init_t195;
extern const V4LlacTable_init_t195 v4llac_init_t195;
#define init_t195 (v4llac_init_t195.value)
typedef union {
    uint32_t bits[56];
    LLAC_INT value[56];
} V4LlacTable_init_t196;
extern const V4LlacTable_init_t196 v4llac_init_t196;
#define init_t196 (v4llac_init_t196.value)
typedef union {
    uint32_t bits[59];
    LLAC_INT value[59];
} V4LlacTable_init_t197;
extern const V4LlacTable_init_t197 v4llac_init_t197;
#define init_t197 (v4llac_init_t197.value)
typedef union {
    uint32_t bits[63];
    LLAC_INT value[63];
} V4LlacTable_init_t20;
extern const V4LlacTable_init_t20 v4llac_init_t20;
#define init_t20 (v4llac_init_t20.value)
typedef union {
    uint32_t bits[440];
    LLAC_FLOAT value[440];
} V4LlacTable_init_t128;
extern const V4LlacTable_init_t128 v4llac_init_t128;
#define init_t128 (v4llac_init_t128.value)
typedef union {
    uint32_t bits[158];
    LLAC_INT value[158];
} V4LlacTable_Table_maxB;
extern const V4LlacTable_Table_maxB v4llac_Table_maxB;
#define Table_maxB (v4llac_Table_maxB.value)
extern const LLAC_INT *const v4llac_alg_mdct_t48[4];
#define alg_mdct_t48 v4llac_alg_mdct_t48
extern const LLAC_INT *const v4llac_alg_mdct_t49[4];
#define alg_mdct_t49 v4llac_alg_mdct_t49
extern const LLAC_INT *const v4llac_alg_mdct_t58[4];
#define alg_mdct_t58 v4llac_alg_mdct_t58
extern const LLAC_INT *const v4llac_alg_mdct_t59[4];
#define alg_mdct_t59 v4llac_alg_mdct_t59
extern const LLAC_INT *const v4llac_alg_mdct_t68[4];
#define alg_mdct_t68 v4llac_alg_mdct_t68
extern const LLAC_INT *const v4llac_alg_mdct_t69[4];
#define alg_mdct_t69 v4llac_alg_mdct_t69
extern const LLAC_FLOAT *const v4llac_init_t13[2][6];
#define init_t13 v4llac_init_t13
extern const LLAC_FLOAT *const v4llac_init_t11[2][6];
#define init_t11 v4llac_init_t11
extern const LLAC_FLOAT *const v4llac_init_t12[2][6];
#define init_t12 v4llac_init_t12
extern const LLAC_INT *const v4llac_init_t178[6];
#define init_t178 v4llac_init_t178
extern const LLAC_INT *const v4llac_init_t179[6];
#define init_t179 v4llac_init_t179
extern const LLAC_INT *const v4llac_init_t188[6];
#define init_t188 v4llac_init_t188
extern const LLAC_INT *const v4llac_init_t189[6];
#define init_t189 v4llac_init_t189
extern const LLAC_INT *const v4llac_init_t198[6];
#define init_t198 v4llac_init_t198
extern const LLAC_INT *const v4llac_init_t199[6];
#define init_t199 v4llac_init_t199
/* Return length/2 pre/post rotation entries each, or null if unlisted. */
const Complex *v4_llac_dct4_rotations(int32_t length);
#endif
