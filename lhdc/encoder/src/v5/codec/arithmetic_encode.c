#include "../include/codec/bit_operations.h"
#include "../include/codec/fast_math.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/v5_extended.h"
#include <string.h>

/* Generate FAC probabilities and cumulative intervals from symbol counts. */
static void v5i_fac_model_counts(v5_sym *sym) {
    uint32_t total = 0;
    sym->csc[0] = 0;
    for (uint32_t i = 0; i < sym->sn; ++i) {
        total += sym->sc[i];
        sym->csc[i + 1] = total;
        sym->sc_hi[i] = sym->sc[i] + (sym->sc[i] >> 5);
    }
    uint32_t scale = total ? LHDC_V5_FAC_PROBABILITY_SCALE / total : 0;
    for (uint32_t i = 1; i <= sym->sn; ++i)
        sym->csc[i] = (sym->csc[i] * scale) >> 16;
}

/* Propagate carries through emitted FAC bytes. */
static void v5i_fac_add_carry(v5_fac *ptr_fenc_s, uint32_t value) {
    uint32_t old = ptr_fenc_s->base;
    ptr_fenc_s->base += value;
    if (ptr_fenc_s->base < old) {
        uint8_t *p = ptr_fenc_s->cur;
        do {
            --p;
            if (*p != 255)
                break;
            *p = 0;
        } while (1);
        ++*p;
    }
}

/* Normalize FAC intervals and commit high bytes. */
static int32_t v5i_fac_normalize(v5_fac *ptr_fenc_s) {
    while (ptr_fenc_s->length < LHDC_V5_FAC_NORMALIZE_THRESHOLD) {
        *ptr_fenc_s->cur++ = (uint8_t)(ptr_fenc_s->base >> 24);
        if (ptr_fenc_s->cur >= ptr_fenc_s->end)
            return LHDC_V5_FAC_BUFFER_FULL;
        ptr_fenc_s->base <<= 8;
        ptr_fenc_s->length <<= 8;
    }
    return LHDC_V5_FAC_OK;
}

/* Initialize the frequency arithmetic coder's interval/output cache. */
void v5f_FacEncInit(fac_enc *ptr_fenc_s, uint8_t *out, int32_t out_size) {
    v5_FacEncInit((void *)ptr_fenc_s, (void *)out, out_size);
}

/* Set adaptive FAC model frequencies/update window. */
void v5f_FacEncSetMA(fac_enc *ptr_fenc_s, const uint32_t *cnt, uint32_t sn, int32_t mn) {
    v5_FacEncSetMA((void *)ptr_fenc_s, (void *)cnt, sn, mn);
}

/* Encode a symbol and update the adaptive frequency model. */
int32_t v5f_FacEncodeSymbolMA(fac_enc *ptr_fenc_s, uint32_t s) {
    return v5_FacEncodeSymbolMA((void *)ptr_fenc_s, s);
}

/* Finish FAC interval coding and commit to the bit writer. */
int32_t v5f_FacEncFlush(fac_enc *ptr_fenc_s, BitWriter *ptr_bw) {
    return v5_FacEncFlush((void *)ptr_fenc_s, (void *)ptr_bw);
}

/* Estimate adaptive FAC bits for a symbol sequence. */
int32_t v5f_FacEncEvalMA(uint8_t *dat, int32_t n, const uint32_t *cnt_tbl, uint32_t symbols,
                         int32_t avg_n) {
    return v5_FacEncEvalMA((void *)dat, n, (void *)cnt_tbl, symbols, avg_n);
}

/* Initialize the frequency arithmetic coder's interval/output cache. */
void v5_FacEncInit(v5_fac *ptr_fenc_s, uint8_t *out, int32_t out_size) {
    ptr_fenc_s->out = out;
    ptr_fenc_s->cur = out;
    ptr_fenc_s->end = out + out_size;
    ptr_fenc_s->base = 0;
    ptr_fenc_s->length = UINT32_MAX;
}

/* Set initial FAC symbol frequencies/cumulative intervals. */
void v5_FacEncSetSymbolCount(v5_fac *ptr_fenc_s, uint32_t *cnt, int32_t sn) {
    ptr_fenc_s->sym.sn = (uint32_t)sn;
    for (int32_t i = 0; i < sn; ++i)
        ptr_fenc_s->sym.sc[i] = cnt[i];
    v5i_fac_model_counts(&ptr_fenc_s->sym);
}

/* Encode a symbol using the current frequency model. */
int32_t v5_FacEncodeSymbol(v5_fac *ptr_fenc_s, uint32_t s) {
    uint32_t unit = ptr_fenc_s->length >> LHDC_V5_FAC_PROBABILITY_BITS,
             x = unit * ptr_fenc_s->sym.csc[s];
    ptr_fenc_s->length = unit * ptr_fenc_s->sym.csc[s + 1] - x;
    v5i_fac_add_carry(ptr_fenc_s, x);
    return v5i_fac_normalize(ptr_fenc_s);
}

/* Increment a symbol frequency and update cumulative intervals. */
void v5_FacEncIncSymbolFreq(v5_fac *ptr_fenc_s, uint32_t s) {
    if (++ptr_fenc_s->sym.sc[s] >= ptr_fenc_s->sym.sc_hi[s])
        v5i_fac_model_counts(&ptr_fenc_s->sym);
}

/* Set adaptive FAC model frequencies/update window. */
void v5_FacEncSetMA(v5_fac *ptr_fenc_s, uint32_t *cnt, uint32_t sn, int32_t mn) {
    v5_FacEncSetSymbolCount(ptr_fenc_s, cnt, (int32_t)sn);
    ptr_fenc_s->mv.mn = mn;
    ptr_fenc_s->mv.mc = 0;
    ptr_fenc_s->mv.mi = 0;
}

/* Encode a symbol and update the adaptive frequency model. */
int32_t v5_FacEncodeSymbolMA(v5_fac *ptr_fenc_s, uint32_t s) {
    int32_t status = v5_FacEncodeSymbol(ptr_fenc_s, s);
    if (status)
        return status;
    if (ptr_fenc_s->mv.mc < ptr_fenc_s->mv.mn) {
        v5_FacEncIncSymbolFreq(ptr_fenc_s, s);
        ptr_fenc_s->mv.ma[ptr_fenc_s->mv.mc++] = (uint8_t)s;
    } else {
        uint8_t *p = ptr_fenc_s->mv.ma + ptr_fenc_s->mv.mi;
        if (ptr_fenc_s->sym.sc[*p] > 1)
            --ptr_fenc_s->sym.sc[*p];
        v5_FacEncIncSymbolFreq(ptr_fenc_s, s);
        *p = (uint8_t)s;
        ptr_fenc_s->mv.mi = ptr_fenc_s->mv.mi + 1 < ptr_fenc_s->mv.mn ? ptr_fenc_s->mv.mi + 1 : 0;
    }
    return LHDC_V5_FAC_OK;
}

/* Finish FAC interval coding and commit to the bit writer. */
int32_t v5_FacEncFlush(v5_fac *ptr_fenc_s, v5_bit_writer *ptr_bw) {
    uint32_t increment = ptr_fenc_s->length <= LHDC_V5_FAC_FLUSH_THRESHOLD
                             ? LHDC_V5_FAC_FLUSH_SMALL_INCREMENT
                             : LHDC_V5_FAC_FLUSH_LARGE_INCREMENT;
    ptr_fenc_s->length = ptr_fenc_s->length <= LHDC_V5_FAC_FLUSH_THRESHOLD
                             ? LHDC_V5_FAC_FLUSH_SMALL_RANGE
                             : LHDC_V5_FAC_FLUSH_LARGE_RANGE;
    v5i_fac_add_carry(ptr_fenc_s, increment);
    if (v5i_fac_normalize(ptr_fenc_s))
        return LHDC_V5_FAC_BUFFER_FULL;
    int32_t len = (int32_t)((uintptr_t)ptr_fenc_s->cur - (uintptr_t)ptr_fenc_s->out);
    (void)v5_bw_write_bytes(ptr_bw, ptr_fenc_s->out, len);
    return len;
}

/* Estimate static FAC bits for a symbol sequence. */
int32_t v5_FacEncEval(uint8_t *dat, int32_t n, uint32_t *cnt_tbl, uint32_t symbols) {
    uint32_t counts[LHDC_V5_FAC_MAX_SYMBOLS], total = 0;
    for (uint32_t i = 0; i < symbols; ++i) {
        counts[i] = cnt_tbl[i];
        total += cnt_tbl[i];
    }
    for (int32_t i = 0; i < n; ++i)
        ++counts[dat[i]];
    uint32_t denom = total + (uint32_t)n, inv = denom ? LHDC_V5_FAC_EVAL_SCALE / denom : 0;
    float sum = 0.0f;
    for (uint32_t i = 0; i < symbols; ++i) {
        float p = (float)(counts[i] * inv) * LHDC_V5_FAC_EVAL_INVERSE_SCALE;
        float bits = v5_fast_nlog2_inline(p);
        sum += bits * (float)(int32_t)(counts[i] - cnt_tbl[i]);
    }
    return audio_math_truncate_i32_f32(sum);
}

/* Estimate adaptive FAC bits for a symbol sequence. */
int32_t v5_FacEncEvalMA(uint8_t *dat, int32_t n, uint32_t *cnt_tbl, uint32_t symbols,
                        int32_t avg_n) {
    (void)avg_n;
    return v5_FacEncEval(dat, n, cnt_tbl, symbols);
}
