#include "../../include/lhdc_cirbuf.h"
#include <audio_common/ring_buffer.h>

/* Convert the ABI-stable LHDC byte-buffer state into a common element-buffer view. */
static AudioRingBuffer lhdc_cirbuf_view(const LhdcCirbuf *buffer) {
    AudioRingBuffer ring;
    ring.storage = buffer->cbuf;
    ring.element_size = 1;
    ring.capacity = buffer->max_len;
    ring.read_index = buffer->odx;
    ring.write_index = buffer->idx;
    ring.used = buffer->s_len;
    return ring;
}

/* Copy the mutable common ring-buffer state back to the LHDC compatibility structure. */
static void lhdc_cirbuf_sync(LhdcCirbuf *buffer, const AudioRingBuffer *ring) {
    buffer->idx = (uint32_t)ring->write_index;
    buffer->odx = (uint32_t)ring->read_index;
    buffer->s_len = (uint32_t)ring->used;
    buffer->r_len = (uint32_t)audio_ring_free(ring);
}

/* Bind caller-owned storage and initialize an empty buffer; negative capacity means empty. */
void lhdc_cirbuf_init(LhdcCirbuf *buffer, uint8_t *storage, int32_t capacity) {
    lhdc_cirbuf_init_inline(buffer, storage, capacity);
}

/* Reset the read and write positions while retaining storage and capacity. */
void lhdc_cirbuf_reset(LhdcCirbuf *buffer) {
    lhdc_cirbuf_reset_inline(buffer);
}

/* Return the number of readable bytes. */
int32_t lhdc_cirbuf_len(const LhdcCirbuf *buffer) {
    return lhdc_cirbuf_len_inline(buffer);
}

/* Return the number of writable bytes. */
int32_t lhdc_cirbuf_empty_len(const LhdcCirbuf *buffer) {
    return lhdc_cirbuf_empty_len_inline(buffer);
}

/* Read and consume available data, splitting a wrapped transfer into two spans. */
int32_t lhdc_cirbuf_get(LhdcCirbuf *buffer, uint8_t *output, int32_t length) {
    if (length <= 0)
        return 0;
    AudioRingBuffer ring = lhdc_cirbuf_view(buffer);
    size_t count = audio_ring_read(&ring, output, (uint32_t)length);
    lhdc_cirbuf_sync(buffer, &ring);
    return (int32_t)count;
}

/* Write and commit data that fits, splitting a wrapped transfer into two spans. */
int32_t lhdc_cirbuf_put(LhdcCirbuf *buffer, const uint8_t *input, int32_t length) {
    if (length <= 0)
        return 0;
    AudioRingBuffer ring = lhdc_cirbuf_view(buffer);
    size_t count = audio_ring_write(&ring, input, (uint32_t)length);
    lhdc_cirbuf_sync(buffer, &ring);
    return (int32_t)count;
}

/* Return and immediately commit a contiguous writable span without copying. */
int32_t lhdc_cirbuf_put_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length) {
    if (length <= 0 || !buffer->r_len)
        return 0;
    AudioRingBuffer ring = lhdc_cirbuf_view(buffer);
    AudioRingSpan spans[2];
    audio_ring_write_spans(&ring, (uint32_t)length, spans);
    *output = spans[0].data;
    audio_ring_commit(&ring, spans[0].length);
    lhdc_cirbuf_sync(buffer, &ring);
    return (int32_t)spans[0].length;
}

/* Return and consume a contiguous readable span while preserving wrap-failure side effects. */
int32_t lhdc_cirbuf_get_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length) {
    if (length <= 0 || (uint32_t)length > buffer->s_len)
        return 0;
    AudioRingBuffer ring = lhdc_cirbuf_view(buffer);
    AudioRingConstSpan spans[2];
    size_t count = audio_ring_read_spans(&ring, (uint32_t)length, spans);
    *output = (uint8_t *)spans[0].data;
    if (count != (uint32_t)length || spans[1].length)
        return 0;
    audio_ring_consume(&ring, count);
    lhdc_cirbuf_sync(buffer, &ring);
    return (int32_t)count;
}

/* Diagnostic exports use aliases so they share the same implementation and state. */
#if defined(__GNUC__)
#include "../../../platform/gnu/include/lhdc_cirbuf_exports.h"
#endif
