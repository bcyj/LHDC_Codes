#ifndef LHDC_CIRBUF_H
#define LHDC_CIRBUF_H
#include <stdint.h>

/* Caller-owned storage; counts are bytes and valid capacity is 0..INT32_MAX. */
typedef struct cirbuf_s {
    uint32_t idx, odx, s_len, r_len, max_len;
    uint8_t *cbuf;
} LhdcCirbuf;
typedef LhdcCirbuf cirbuf_s, savi_circ_buf;
typedef LhdcCirbuf *Pcirbuf_s;

/* Bind instance storage and clear byte counts; treat negative capacity as zero. */
static inline void lhdc_cirbuf_init_inline(LhdcCirbuf *buffer, uint8_t *storage, int32_t capacity) {
    buffer->idx = buffer->odx = buffer->s_len = 0;
    buffer->r_len = buffer->max_len = capacity > 0 ? (uint32_t)capacity : 0;
    buffer->cbuf = storage;
}

/* Reset instance read/write state while retaining capacity and storage. */
static inline void lhdc_cirbuf_reset_inline(LhdcCirbuf *buffer) {
    buffer->idx = buffer->odx = buffer->s_len = 0;
    buffer->r_len = buffer->max_len;
}

/* Read the instance's committed byte count. */
static inline int32_t lhdc_cirbuf_len_inline(const LhdcCirbuf *buffer) {
    return (int32_t)buffer->s_len;
}

/* Read the instance's remaining writable byte count. */
static inline int32_t lhdc_cirbuf_empty_len_inline(const LhdcCirbuf *buffer) {
    return (int32_t)buffer->r_len;
}

#ifdef __cplusplus
extern "C" {
#endif
void lhdc_cirbuf_init(LhdcCirbuf *buffer, uint8_t *storage, int32_t capacity);
void lhdc_cirbuf_reset(LhdcCirbuf *buffer);
int32_t lhdc_cirbuf_len(const LhdcCirbuf *buffer);
int32_t lhdc_cirbuf_empty_len(const LhdcCirbuf *buffer);
int32_t lhdc_cirbuf_get(LhdcCirbuf *buffer, uint8_t *output, int32_t length);
int32_t lhdc_cirbuf_put(LhdcCirbuf *buffer, const uint8_t *input, int32_t length);
/* Contiguous write storage is committed on return; free space or the tail segment may shorten it. */
int32_t lhdc_cirbuf_put_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length);
/* Contiguous reads must satisfy the entire request; tail-crossing failure still updates output. */
int32_t lhdc_cirbuf_get_no_copy(LhdcCirbuf *buffer, uint8_t **output, int32_t length);
#ifdef __cplusplus
}
#endif
#endif
