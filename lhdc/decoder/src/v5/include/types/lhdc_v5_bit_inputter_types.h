
#ifndef LHDC_DECODER_SRC_V5_INCLUDE_TYPES_LHDC_V5_BIT_INPUTTER_TYPES_H
#define LHDC_DECODER_SRC_V5_INCLUDE_TYPES_LHDC_V5_BIT_INPUTTER_TYPES_H
typedef unsigned char undefined;

typedef unsigned char byte;
typedef unsigned int dword;
typedef unsigned int uint;
typedef unsigned short word;
typedef struct nbyte_read_struct nbyte_read_struct;

struct nbyte_read_struct {
    uint *dec_r_mem;
    uint *dec_r_begin;
    uint *dec_r_finish;
    uint dec_r_d32_l;
    uint dec_r_num;
};

#endif
