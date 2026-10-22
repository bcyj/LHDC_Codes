#ifndef LHDC_DECODER_V4_BIT_READER_H
#define LHDC_DECODER_V4_BIT_READER_H
#include "lhdc_dec.h"
#include <audio_math/bits.h>
#include <audio_math/integer.h>
#include <audio_math/byte_order.h>

typedef struct bit_buf_t {
    unsigned rb, db, saved_rb, saved_db;
    cirbuf_s *pcb;
    uint8_t *in_max, *in;
    unsigned read_len;
} bit_buf_t;

/* Advance four bytes, accumulate consumption, and wrap at the input tail. */
static inline void MoveInU32(bit_buf_t *bit_rb) {
    bit_rb->in += sizeof(uint32_t);
    bit_rb->read_len += 4;
    if (bit_rb->in >= bit_rb->in_max)
        bit_rb->in = bit_rb->pcb->cbuf;
}

/* Read unsigned fields most-significant-bit first, combining the next input word when needed. */
static inline void read_uint32(bit_buf_t *bit_rb, unsigned nb, unsigned *dat) {
    unsigned shift = 32 - nb, remain = bit_rb->rb;
    *dat = audio_math_shift_right_low8_zero32(bit_rb->db, shift);
    if (remain < nb) {
        nb -= remain;
        bit_rb->db = audio_math_read_le32(bit_rb->in);
        bit_rb->rb = 32;
        *dat |= audio_math_shift_right_low8_zero32(bit_rb->db, shift + remain);
        MoveInU32(bit_rb);
        remain = bit_rb->rb;
    }
    bit_rb->rb = remain - nb;
    bit_rb->db = audio_math_shift_left_low8_zero32(bit_rb->db, nb);
}

/* Reuse identical bit-reader state transitions and sign-extend arithmetically to field width. */
static inline void read_int32(bit_buf_t *bit_rb, unsigned nb, int *dat) {
    unsigned value;
    read_uint32(bit_rb, nb, &value);
    *dat = audio_math_int32_from_bits(
        audio_math_shift_arithmetic_low8_sign32(audio_math_shift_left_low8_zero32(value, 32 - nb), 32 - nb));
}

/* Return the cached word's high byte; refill directly if fewer than eight bits remain, discarding those bits. */
static inline unsigned char read_u8(bit_buf_t *bit_rb) {
    if (bit_rb->rb < 8) {
        bit_rb->db = audio_math_read_le32(bit_rb->in);
        bit_rb->rb = 32;
        MoveInU32(bit_rb);
    }
    unsigned char u8 = (unsigned char)(bit_rb->db >> 24);
    bit_rb->rb -= 8;
    bit_rb->db <<= 8;
    return u8;
}

#endif
