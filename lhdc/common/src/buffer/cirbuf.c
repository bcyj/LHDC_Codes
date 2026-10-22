#include "../../include/lhdc_cirbuf.h"
#include <string.h>

/* Bind caller storage and reset state; negative capacity produces an empty buffer. */
void lhdc_cirbuf_init(LhdcCirbuf *buffer, uint8_t *storage, int32_t capacity) {
    lhdc_cirbuf_init_inline(buffer, storage, capacity);
}

/* Reset instance read/write positions while retaining storage and capacity. */
void lhdc_cirbuf_reset(LhdcCirbuf *buffer) {
    lhdc_cirbuf_reset_inline(buffer);
}

/* Query used bytes. */
int32_t lhdc_cirbuf_len(const LhdcCirbuf *buffer) {
    return lhdc_cirbuf_len_inline(buffer);
}

/* Query writable bytes. */
int32_t lhdc_cirbuf_empty_len(const LhdcCirbuf *buffer) {
    return lhdc_cirbuf_empty_len_inline(buffer);
}

/* Read and consume up to the used byte count, splitting tail-crossing reads into two segments. */
int32_t lhdc_cirbuf_get(LhdcCirbuf *buffer, uint8_t *output, int32_t length) {
    if (length <= 0 || !buffer->s_len)
        return 0;
    uint32_t count = (uint32_t)length < buffer->s_len ? (uint32_t)length : buffer->s_len;
    uint32_t tail = buffer->max_len - buffer->odx;
    uint32_t first = count < tail ? count : tail;
    memcpy(output, buffer->cbuf + buffer->odx, first);
    buffer->odx = first == tail ? 0 : buffer->odx + first;
    if (count > first) {
        memcpy(output + first, buffer->cbuf, count - first);
        buffer->odx = count - first;
    }
    buffer->s_len -= count;
    buffer->r_len += count;
    return (int32_t)count;
}

/* Write and commit up to the free byte count, splitting tail-crossing writes into two segments. */
int32_t lhdc_cirbuf_put(LhdcCirbuf *buffer, const uint8_t *input, int32_t length) {
    if (length <= 0 || !buffer->r_len)
        return 0;
    uint32_t count = (uint32_t)length < buffer->r_len ? (uint32_t)length : buffer->r_len;
    uint32_t tail = buffer->max_len - buffer->idx;
    uint32_t first = count < tail ? count : tail;
    memcpy(buffer->cbuf + buffer->idx, input, first);
    buffer->idx = first == tail ? 0 : buffer->idx + first;
    if (count > first) {
        memcpy(buffer->cbuf, input + first, count - first);
        buffer->idx = count - first;
    }
    buffer->s_len += count;
    buffer->r_len -= count;
    return (int32_t)count;
}

/* Return contiguous writable storage and commit immediately; reject negative lengths without changing the output pointer. */
int32_t lhdc_cirbuf_put_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length) {
    if (length <= 0 || !buffer->r_len)
        return 0;
    uint32_t count = (uint32_t)length < buffer->r_len ? (uint32_t)length : buffer->r_len;
    uint32_t tail = buffer->max_len - buffer->idx;
    *output = buffer->cbuf + buffer->idx;
    if (count >= tail) {
        count = tail;
        buffer->idx = 0;
    } else
        buffer->idx += count;
    buffer->s_len += count;
    buffer->r_len -= count;
    return (int32_t)count;
}

/* Return contiguous readable storage and consume immediately, preserving output-pointer effects on tail-crossing failure. */
int32_t lhdc_cirbuf_get_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length) {
    if (length <= 0 || (uint32_t)length > buffer->s_len)
        return 0;
    *output = buffer->cbuf + buffer->odx;
    uint32_t tail = buffer->max_len - buffer->odx;
    if ((uint32_t)length > tail)
        return 0;
    buffer->odx = (uint32_t)length == tail ? 0 : buffer->odx + (uint32_t)length;
    buffer->s_len -= (uint32_t)length;
    buffer->r_len += (uint32_t)length;
    return length;
}

/* Diagnostic entry points use symbol aliases and share the function body and state. */
#if defined(__GNUC__)
#include "../../../platform/gnu/include/lhdc_cirbuf_exports.h"
#endif
