#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_TYPES_LHDC_BIT_WRITER_TYPES_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_TYPES_LHDC_BIT_WRITER_TYPES_H
#include <stdint.h>
/* Single V5 bit-writing state; compatibility names alias the same type. */
typedef struct BitWriter {
    uint32_t *buf;
    uint8_t *start;
    uint8_t *end;
    uint32_t u32;
    uint32_t bits;
} BitWriter;
typedef BitWriter v5_bit_writer;
#endif
