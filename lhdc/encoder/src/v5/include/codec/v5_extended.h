#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_V5_EXTENDED_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_V5_EXTENDED_H
#include "v5_pcm.h"
#include <stddef.h>

typedef struct {
    uint8_t ma[LHDC_V5_FAC_HISTORY_SIZE];
    int32_t mi, mn, mc;
} v5_mva;
typedef struct {
    uint32_t sn, csc[LHDC_V5_FAC_CUMULATIVE_ENTRIES], sc[LHDC_V5_FAC_MAX_SYMBOLS],
        sc_hi[LHDC_V5_FAC_MAX_SYMBOLS];
} v5_sym;
typedef struct {
    uint32_t base, length;
    uint8_t *out, *cur, *end;
    v5_mva mv;
    v5_sym sym;
} v5_fac;
typedef struct {
    int32_t n, inv;
    const int32_t *off, *scale;
    int32_t scale_shift_bits, scale_down, rice_min_p0;
    int32_t ac_ma0, ac_ma1;
} v5_band;
void v5_FacEncInit(v5_fac *, uint8_t *, int32_t);
void v5_FacEncSetSymbolCount(v5_fac *, uint32_t *, int32_t);
int32_t v5_FacEncodeSymbol(v5_fac *, uint32_t);
void v5_FacEncIncSymbolFreq(v5_fac *, uint32_t);
void v5_FacEncSetMA(v5_fac *, uint32_t *, uint32_t, int32_t);
int32_t v5_FacEncodeSymbolMA(v5_fac *, uint32_t);
int32_t v5_FacEncFlush(v5_fac *, v5_bit_writer *);
int32_t v5_FacEncEval(uint8_t *, int32_t, uint32_t *, uint32_t);
int32_t v5_FacEncEvalMA(uint8_t *, int32_t, uint32_t *, uint32_t, int32_t);
void v5_GenerateGainTable(int32_t, int32_t, int32_t, int32_t, float *, int32_t *, float *);
float v5_EncGainByTable(int32_t, float *, int32_t, float);
int32_t v5_int_log2(uint32_t);
void v5_shift_scf(v5_band *, int32_t *);
void v5_moving_average_scf(v5_band *, int32_t *, int32_t *);
#endif
