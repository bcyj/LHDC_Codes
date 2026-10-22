#ifndef LHDC_DECODER_PUBLIC_V5_H
#define LHDC_DECODER_PUBLIC_V5_H
#include <stdint.h>
#include <stddef.h>
#include "status.h"
#include <stdbool.h>
/* Allocate workspace using current target-ABI queries; never interchange V4/V5 instances. */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LhdcV5FrameInfo {
    uint32_t frame_len, isSplit, isLeft;
} LhdcV5FrameInfo;
typedef void (*LhdcV5LogCallback)(char *);
/* Initialize the integrated decoder using the internal short-enum version. */
int32_t lhdcv5_util_init_decoder(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                 uint16_t);
/* Decode a complete input frame, returning status and writing output PCM bytes. */
int32_t lhdcv5_util_dec_process(void *, uint8_t *, uint8_t *, uint32_t, uint32_t *);
/* Register this instance's log callback after initialization; NULL disables it. Register again after reinitialization. */
void lhdcv5_util_dec_register_log_cb(void *handle, LhdcV5LogCallback);
/* Clear the V5 core address while retaining external storage. */
int32_t lhdcv5_util_dec_destroy(void *);
/* Read V5 output sample count, rejecting null instances and unknown types. */
int32_t lhdcv5_util_dec_get_sample_size(void *);
/* Validate minimum frame length and parse the V5 header. */
int32_t lhdcv5_util_dec_fetch_frame_info(void *, uint8_t *, uint32_t, LhdcV5FrameInfo *);
/* Configure output channels, preserving wrapper-field writes after underlying failure. */
int32_t lhdcv5_util_dec_channel_selsect(void *, uint8_t);
/* Query total memory by version, sample rate, and audio format. */
int32_t lhdcv5_util_dec_get_mem_req(uint16_t, uint32_t, uint32_t, uint32_t, uint32_t *);
/* Validate and bind 16-byte-aligned caller workspace without taking ownership. */
int32_t lhdcv5_util_init_decoder_in_memory(void *memory, size_t bytes, uint32_t bits,
                                           uint32_t rate, uint32_t format, uint32_t duration,
                                           uint32_t lossless, uint16_t version);
#ifdef __cplusplus
}
#endif
#endif
