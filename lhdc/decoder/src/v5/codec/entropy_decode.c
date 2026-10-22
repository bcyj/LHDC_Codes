#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/bits.h>
#include "../../../../../math/include/audio_math/byte_order.h"
#include "../include/lhdc_arith.h"

/* Recompute cumulative boundaries and thresholds from frequencies, preserving 32-bit product truncation. */
static void model_counts(arith_sdata_struct *symbol_model, unsigned *count, unsigned symbol_num) {
    unsigned cumulative[5], sum = 0;
    for (unsigned i = 0; i < symbol_num; ++i) {
        sum += count[i];
        cumulative[i] = sum;
    }
    unsigned scale = 0x80000000u / sum;
    for (unsigned i = 0; i < symbol_num; ++i)
        symbol_model->sdata_para_2[i] = count[i];
    for (unsigned i = 0; i < symbol_num; ++i) {
        symbol_model->sdata_para_3[i] = count[i] + (count[i] >> 5);
        symbol_model->sdata_para_1[i + 1] = (scale * cumulative[i]) >> 16;
    }
}

/* Set symbol count; unsupported counts update only the count field. */
void lhdc_dec_arith_symbol_num_program(dec_arith_struct *dec_arith, unsigned *count,
                                       int symbol_num) {
    dec_arith->dec_arth_para_4.sdata_para_0 = (unsigned)symbol_num;
    if (symbol_num >= 3 && symbol_num <= 5)
        model_counts(&dec_arith->dec_arth_para_4, count, (unsigned)symbol_num);
}

/* Set model/window length and reset window counters and position. */
void lhdc_dec_arith_program(dec_arith_struct *dec_arith, unsigned *count, int symbol_num,
                            int shifting_num) {
    lhdc_dec_arith_symbol_num_program(dec_arith, count, symbol_num);
    dec_arith->dec_arth_para_3.mdata_para_2 = shifting_num;
    dec_arith->dec_arth_para_3.mdata_para_3 = 0;
    dec_arith->dec_arth_para_3.mdata_para_1 = 0;
}

/* Increment a symbol, selecting three-to-five-symbol recalculation from the model count's low byte. */
void lhdc_dec_arith_symbol_2(dec_arith_struct *dec_arith, unsigned symbol) {
    arith_sdata_struct *symbol_model = &dec_arith->dec_arth_para_4;
    unsigned value = ++symbol_model->sdata_para_2[symbol];
    unsigned symbol_n = symbol_model->sdata_para_0 & 255;
    if (symbol_model->sdata_para_3[symbol] <= value && symbol_n >= 3 && symbol_n <= 5)
        model_counts(symbol_model, symbol_model->sdata_para_2, symbol_n);
}

/* Normalize bytewise. Immediate exhaustion returns an error without relying on implicit unavailable input.
 * Exhaustion after valid bytes preserves existing tail accumulation and status rules. */
static int normalize(dec_arith_struct *dec_arith, nbyte_read_struct *bit_inputter) {
    int error = LHDC_DEC_OK;
    int refill_known = 0;
    unsigned refill_byte = 0;
    while (dec_arith->dec_arth_para_1 < 0x1000000u) {
        unsigned used = bit_inputter->dec_r_num,
                 mask = audio_math_shift_right_low8_zero32(UINT32_MAX, used);
        if (bit_inputter->dec_r_mem < bit_inputter->dec_r_finish) {
            if (32 - used <= 8) {
                unsigned old = bit_inputter->dec_r_d32_l;
                ++bit_inputter->dec_r_mem;
                bit_inputter->dec_r_d32_l = audio_math_read_be32(bit_inputter->dec_r_mem);
                used -= 24;
                unsigned value = mask & old;
                if (used)
                    value = audio_math_shift_right_low8_zero32(bit_inputter->dec_r_d32_l, 32 - used) |
                            audio_math_shift_left_low8_zero32(value, used);
                bit_inputter->dec_r_num = used;
                refill_byte = value & 255;
            } else {
                bit_inputter->dec_r_num = used + 8;
                refill_byte =
                    audio_math_shift_right_low8_zero32(mask & bit_inputter->dec_r_d32_l, 24 - used) & 255;
            }
            refill_known = 1;
        } else {
            if (!refill_known)
                return LHDC_V5_DEC_INPUT_EXHAUSTED;
            error = LHDC_V5_DEC_INPUT_EXHAUSTED;
        }
        dec_arith->dec_arth_para_0 = (dec_arith->dec_arth_para_0 << 8) | refill_byte;
        dec_arith->dec_arth_para_1 <<= 8;
        dec_arith->dec_arth_para_2 = (dec_arith->dec_arth_para_2 << 8) | refill_byte;
    }
    return error;
}

/* Select cumulative boundaries and update code/range; zero-width ranges return -6 without normalization. */
static int range_decode(dec_arith_struct *dec_arith, nbyte_read_struct *bit_inputter,
                        unsigned *symbol, unsigned symbol_num) {
    unsigned unit = dec_arith->dec_arth_para_1 >> 15, lower = 0, upper = 0;
    dec_arith->dec_arth_para_1 = unit;
    for (unsigned i = 0; i < symbol_num; ++i) {
        upper = dec_arith->dec_arth_para_4.sdata_para_1[i + 1] * unit;
        if (dec_arith->dec_arth_para_0 < upper) {
            *symbol = i;
            dec_arith->dec_arth_para_0 -= lower;
            dec_arith->dec_arth_para_1 = upper - lower;
            return dec_arith->dec_arth_para_1 ? normalize(dec_arith, bit_inputter)
                                              : LHDC_V5_DEC_INVALID_SYMBOL;
        }
        lower = upper;
    }
    *symbol = symbol_num;
    dec_arith->dec_arth_para_0 -= upper;
    dec_arith->dec_arth_para_1 = 0;
    return LHDC_V5_DEC_INVALID_SYMBOL;
}

/* Decode an arithmetic symbol from three explicit parameters; report exhausted normalization input. */
int lhdc_dec_arith_top_1(dec_arith_struct *dec_arith, nbyte_read_struct *bit_inputter,
                         unsigned *symbol) {
    return range_decode(dec_arith, bit_inputter, symbol,
                        dec_arith->dec_arth_para_4.sdata_para_0 & 255);
}

/* Decode three symbols, maintain the history window, and update the frequency model at its threshold. */
int lhdc_dec_arith_symbol_3(dec_arith_struct *dec_arith, nbyte_read_struct *bit_inputter,
                            unsigned *symbol) {
    int result = range_decode(dec_arith, bit_inputter, symbol, 3);
    if (result)
        return result;
    arith_mdata_struct *window_model = &dec_arith->dec_arth_para_3;
    arith_sdata_struct *symbol_model = &dec_arith->dec_arth_para_4;
    if (window_model->mdata_para_2 <= window_model->mdata_para_3) {
        int index = window_model->mdata_para_1;
        unsigned old = window_model->mdata_para_0[index];
        if (symbol_model->sdata_para_2[old] > 1)
            --symbol_model->sdata_para_2[old];
        unsigned value = ++symbol_model->sdata_para_2[*symbol];
        if (symbol_model->sdata_para_3[*symbol] <= value && symbol_model->sdata_para_0 >= 3 &&
            symbol_model->sdata_para_0 <= 5)
            model_counts(symbol_model, symbol_model->sdata_para_2, symbol_model->sdata_para_0);
        window_model->mdata_para_0[index] = (unsigned char)*symbol;
        ++index;
        if (index >= window_model->mdata_para_2)
            index = 0;
        window_model->mdata_para_1 = index;
    } else {
        lhdc_dec_arith_symbol_2(dec_arith, *symbol);
        window_model->mdata_para_0[window_model->mdata_para_3] = (unsigned char)*symbol;
        ++window_model->mdata_para_3;
    }
    return LHDC_DEC_OK;
}

/* Keep the low 16 bits and return 16 for narrow ranges; otherwise keep low 24 bits and return 24. */
int lhdc_dec_arith_stop(dec_arith_struct *dec_arith, unsigned *bindata) {
    if (dec_arith->dec_arth_para_1 < 0x2000001u) {
        *bindata = dec_arith->dec_arth_para_2 & 65535;
        return LHDC_V5_DEC_ARITH_TAIL_BITS_SHORT;
    }
    *bindata = dec_arith->dec_arth_para_2 & 0xffffff;
    return LHDC_V5_DEC_ARITH_TAIL_BITS_LONG;
}
#include <audio_math/bits.h>
#include "../../../../../math/include/audio_math/byte_order.h"
#include "../include/lhdc_dec.h"
#include <string.h>

/* Initialize bit reading; input must contain a readable first word and trailing prefetch padding word. */
void lhdc_v5_bit_reader_init(nbyte_read_struct *bit_inputter, unsigned int *umem32,
                             unsigned int size) {
    bit_inputter->dec_r_mem = bit_inputter->dec_r_begin = umem32;
    bit_inputter->dec_r_finish = umem32 + size / 4 + (size % 4 != 0);
    bit_inputter->dec_r_num = 0;
    bit_inputter->dec_r_d32_l = audio_math_read_be32(umem32);
}

