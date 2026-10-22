#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/bits.h>
#include "../../../../../math/include/audio_math/integer.h"
#include "../../../../../math/include/audio_math/byte_order.h"
#include "../include/lhdc_dec_core.h"
#include <string.h>

/* Refill one word and check the three-byte tolerance, retaining prefetch state even on failure. */
static int rice_fill(bit_buf_t *bit_rb) {
    bit_rb->db = audio_math_read_le32(bit_rb->in);
    bit_rb->rb = 32;
    MoveInU32(bit_rb);
    return bit_rb->pcb->s_len + 3 >= bit_rb->read_len;
}

/* Read Rice quotient/remainder, preserving escapes, sign folding, partial output, and final capacity checks. */
int rice_decode(bit_buf_t *bit_rb, int ns, unsigned lpc_m, int *rd_i) {
    if (ns < 1)
        ns = 0;
    for (int i = 0; i < ns; ++i) {
        unsigned quotient = 0;
        for (;;) {
            unsigned zeros = 0;
            while (zeros < 10) {
                if (!bit_rb->rb && !rice_fill(bit_rb))
                    return LHDC_DEC_NO_OUTPUT;
                if (bit_rb->db & 0x80000000u)
                    break;
                bit_rb->db <<= 1;
                --bit_rb->rb;
                ++zeros;
            }
            if (zeros < 10) {
                bit_rb->db <<= 1;
                --bit_rb->rb;
                quotient += zeros;
                break;
            }
            unsigned escape, old_read = bit_rb->read_len;
            read_uint32(bit_rb, 6, &escape);
            if (bit_rb->read_len != old_read && bit_rb->pcb->s_len + 3 < bit_rb->read_len)
                return LHDC_DEC_NO_OUTPUT;
            quotient += 10 + escape;
            if (escape != 63)
                break;
        }
        unsigned value = quotient;
        if (lpc_m) {
            unsigned remainder;
            read_uint32(bit_rb, lpc_m, &remainder);
            value = audio_math_shift_left_low8_zero32(quotient, lpc_m) | remainder;
        }
        rd_i[i] = audio_math_rice_unfold_u32(value);
    }
    return bit_rb->pcb->s_len + 3 < bit_rb->read_len ? LHDC_DEC_NO_OUTPUT : ns;
}

