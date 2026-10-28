#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_BIT_WRITER_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_BIT_WRITER_H
#include "../types/lhdc_bit_writer_types.h"
#include "../../../../../common/include/compat/v5/lhdc_v5_constants.h"
#include <audio_math/byte_order.h>
/* Check remaining capacity with full pointers, requiring four bytes for a whole-word write. */
static inline int32_t v5i_bit_writer_word_fits(const v5_bit_writer *writer) {
    uintptr_t cursor = (uintptr_t)writer->buf, end = (uintptr_t)writer->end;
    return cursor <= end && end - cursor >= sizeof(uint32_t);
}

/* Initialize bit-writer output address, capacity, and cache. */
static inline void v5_bw_init_inline(v5_bit_writer *ptr_bw, uint32_t *buf, uint32_t bytes) {
    ptr_bw->buf = buf;
    ptr_bw->start = (uint8_t *)buf;
    ptr_bw->end = ptr_bw->start + bytes;
    ptr_bw->u32 = 0;
    ptr_bw->bits = 0;
}

/* Write an unsigned field of the specified width to the bitstream. */
static inline int32_t v5_bw_write_uint_inline(v5_bit_writer *ptr_bw, uint32_t val, uint32_t bits) {
    uint32_t left, masked;
    if (bits > 32)
        return LHDC_V5_BIT_WRITER_INVALID_WIDTH;
    if (bits == 0)
        return LHDC_V5_BIT_WRITER_OK;
    left = 32 - ptr_bw->bits;
    masked = val & (UINT32_MAX >> ((32 - bits) & 31));
    if (bits < left) {
        ptr_bw->u32 = (ptr_bw->u32 << bits) | masked;
        ptr_bw->bits += bits;
        return LHDC_V5_BIT_WRITER_OK;
    }
    if (!v5i_bit_writer_word_fits(ptr_bw))
        return LHDC_V5_BIT_WRITER_BUFFER_FULL;
    if (ptr_bw->bits != 0) {
        uint32_t remain = bits - left;
        uint32_t word = (ptr_bw->u32 << left) | (masked >> remain);
        audio_math_write_be32(ptr_bw->buf++, word);
        ptr_bw->bits = remain;
        ptr_bw->u32 = masked;
    } else {
        audio_math_write_be32(ptr_bw->buf++, masked);
    }
    return LHDC_V5_BIT_WRITER_OK;
}

/* Flush the bit cache and return total output bytes. */
static inline int32_t v5_bw_flush_inline(v5_bit_writer *ptr_bw) {
    uint8_t *cursor = (uint8_t *)ptr_bw->buf;
    if (ptr_bw->bits != 0) {
        uint32_t count = (ptr_bw->bits + 7) / 8;
        if ((uintptr_t)cursor > (uintptr_t)ptr_bw->end ||
            (uintptr_t)ptr_bw->end - (uintptr_t)cursor < count)
            return LHDC_V5_BIT_WRITER_BUFFER_FULL;
        uint32_t word = ptr_bw->u32 << (32 - ptr_bw->bits);
        for (uint32_t i = 0; i < count; ++i)
            cursor[i] = (uint8_t)(word >> (24 - 8 * i));
        cursor += count;
    }
    return (int32_t)((uintptr_t)cursor - (uintptr_t)ptr_bw->start);
}

/* Pad with zeros to the next byte boundary. */
static inline int32_t v5_bw_zero_pad_inline(v5_bit_writer *ptr_bw) {
    uint32_t rem = ptr_bw->bits & 7;
    return rem ? v5_bw_write_uint_inline(ptr_bw, 0, 8 - rem) : LHDC_V5_BIT_WRITER_OK;
}
#endif
