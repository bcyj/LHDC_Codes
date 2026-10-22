#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_ARITH_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_ARITH_H
#include "lhdc_dec.h"

typedef struct arith_mdata_struct {
    unsigned char mdata_para_0[128];
    int mdata_para_1, mdata_para_2, mdata_para_3;
} arith_mdata_struct;
typedef struct arith_sdata_struct {
    unsigned sdata_para_0, sdata_para_1[6], sdata_para_2[5], sdata_para_3[5];
} arith_sdata_struct;
typedef struct dec_arith_struct {
    unsigned dec_arth_para_0, dec_arth_para_1, dec_arth_para_2;
    arith_mdata_struct dec_arth_para_3;
    arith_sdata_struct dec_arth_para_4;
} dec_arith_struct;
/* Select an interval and read normalization bytes; return the symbol or arithmetic error. */
int lhdc_dec_arith_top_1(dec_arith_struct *, nbyte_read_struct *, unsigned *);
/* Increment a symbol frequency and recompute the model at the threshold. */
void lhdc_dec_arith_symbol_2(dec_arith_struct *, unsigned);
/* Set symbol count and the three-, four-, or five-symbol frequency model. */
void lhdc_dec_arith_symbol_num_program(dec_arith_struct *, unsigned *, int);
/* Set model and sliding-window length, then reset window position. */
void lhdc_dec_arith_program(dec_arith_struct *, unsigned *, int, int);
/* Decode the three-symbol branch and update its model with a sliding window. */
int lhdc_dec_arith_symbol_3(dec_arith_struct *, nbyte_read_struct *, unsigned *);
/* Read valid tail bits and length from the remaining range width. */
int lhdc_dec_arith_stop(dec_arith_struct *, unsigned *);
#endif
