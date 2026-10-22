#include "../include/lhdc/decoder/status.h"
#include "../src/v5/include/lhdc_arith.h"
#include <stdio.h>
#include <string.h>

/* Immediate normalization exhaustion: both entries must return consistently without reading implicit bytes. */
int main(void) {
    uint32_t guard[4] = {0};
    for (unsigned seed = 0; seed < 256; ++seed) {
        for (unsigned entry = 0; entry < 2; ++entry) {
            dec_arith_struct arithmetic;
            memset(&arithmetic, 0, sizeof arithmetic);
            arithmetic.dec_arth_para_0 = 0;
            arithmetic.dec_arth_para_1 = 65536;
            arithmetic.dec_arth_para_2 = seed * 257u;
            arithmetic.dec_arth_para_4.sdata_para_0 = 3;
            arithmetic.dec_arth_para_4.sdata_para_1[1] = 1;
            arithmetic.dec_arth_para_4.sdata_para_1[2] = 2;
            arithmetic.dec_arth_para_4.sdata_para_1[3] = 3;
            nbyte_read_struct reader;
            memset(&reader, 0, sizeof reader);
            reader.dec_r_begin = reader.dec_r_mem = reader.dec_r_finish = guard;
            reader.dec_r_d32_l = seed;
            unsigned symbol = 99;
            int status = entry ? lhdc_dec_arith_symbol_3(&arithmetic, &reader, &symbol)
                               : lhdc_dec_arith_top_1(&arithmetic, &reader, &symbol);
            if (status != LHDC_V5_DEC_INPUT_EXHAUSTED || symbol != 0 ||
                arithmetic.dec_arth_para_0 != 0 || arithmetic.dec_arth_para_1 != 2 ||
                arithmetic.dec_arth_para_2 != seed * 257u || reader.dec_r_mem != guard ||
                reader.dec_r_num != 0 || arithmetic.dec_arth_para_3.mdata_para_3 != 0)
                return 1;
        }
    }
    puts("PASS 512 deterministic V5 first-refill exhaustion cases");
    return 0;
}
