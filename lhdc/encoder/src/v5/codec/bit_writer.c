#include "../include/codec/bit_writer.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/codec/v5_extended.h"
#include "../include/codec/v5_pcm.h"
#include <string.h>

/* Write bit fields sequentially into a byte array. */
int32_t v5f_BitWriteBitsArrayU8(BitWriter *ptr_bw, uint8_t *ba, int32_t bits, int32_t len,
                                int32_t pad_zero) {
    return v5_BitWriteBitsArrayU8((void *)ptr_bw, (void *)ba, bits, len, pad_zero);
}

/* Initialize bit-writer output address, capacity, and cache. */
void v5_bw_init(v5_bit_writer *ptr_bw, uint32_t *buf, uint32_t bytes) {
    v5_bw_init_inline(ptr_bw, buf, bytes);
}

/* Write an unsigned field of the specified width to the bitstream. */
int32_t v5_bw_write_uint(v5_bit_writer *ptr_bw, uint32_t val, uint32_t bits) {
    return v5_bw_write_uint_inline(ptr_bw, val, bits);
}

/* Write complete bytes to the output bitstream. */
int32_t v5_bw_write_bytes(v5_bit_writer *ptr_bw, const uint8_t *b, int32_t len) {
    int32_t i = 0, status = LHDC_V5_BIT_WRITER_OK;
    int32_t word_limit = (int32_t)((uint32_t)len - 4u);
    while (ptr_bw->bits != 0 && status == 0 && i < len)
        status = v5_bw_write_uint_inline(ptr_bw, b[i++], 8);
    while (status == LHDC_V5_BIT_WRITER_OK && i <= word_limit) {
        if (!v5i_bit_writer_word_fits(ptr_bw))
            return LHDC_V5_BIT_WRITER_BUFFER_FULL;
        memcpy(ptr_bw->buf++, b + i, 4);
        i += 4;
    }
    while (status == 0 && i < len)
        status = v5_bw_write_uint_inline(ptr_bw, b[i++], 8);
    return status;
}

/* Flush the bit cache and return total output bytes. */
int32_t v5_bw_flush(v5_bit_writer *ptr_bw) {
    return v5_bw_flush_inline(ptr_bw);
}

/* Pad with zeros to the next byte boundary. */
int32_t v5_BitWriteZeroPadToByteBoundary(v5_bit_writer *ptr_bw) {
    return v5_bw_zero_pad_inline(ptr_bw);
}

/* Write bit fields sequentially into a byte array. */
int32_t v5_BitWriteBitsArrayU8(v5_bit_writer *ptr_bw, uint8_t *ba, int32_t bits, int32_t len,
                               int32_t pad_zero) {
    if (len == 0 && bits == 0)
        return LHDC_V5_BIT_WRITER_OK;
    int32_t status = LHDC_V5_BIT_WRITER_OK;
    for (int32_t i = 0; i < len && !status; ++i)
        status = v5_bw_write_uint_inline(ptr_bw, ba[i], (uint32_t)bits);
    if (pad_zero)
        status |= v5_bw_zero_pad_inline(ptr_bw);
    return status;
}
