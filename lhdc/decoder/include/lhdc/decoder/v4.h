#ifndef LHDC_DECODER_PUBLIC_V4_H
#define LHDC_DECODER_PUBLIC_V4_H
#include <stdint.h>
#include <stddef.h>
#include "status.h"
#include <stdbool.h>
/* Allocate workspace using current target-ABI queries; never interchange V4/V5 instances. */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LhdcV4FrameInfo {
    uint32_t frame_len, isSplit, isLeft;
} LhdcV4FrameInfo;
typedef void (*LhdcV4LogCallback)(char *);
/* Initialize caller-provided integrated workspace using short-enum state. */
void lhdcInit(uint32_t *, uint32_t, uint32_t, uint32_t, uint16_t);
/* Decode complete frames and return the interleaved PCM byte count. */
uint32_t lhdcDecodeProcess(uint32_t *, uint8_t *, uint8_t *, uint32_t);
/* Register this instance's log callback after initialization; NULL disables it. Register again after reinitialization. */
void lhdc_register_log_cb(void *handle, LhdcV4LogCallback);
/* Clear the core address without releasing caller workspace. */
void lhdcDestroy(uint32_t *);
/* Read frame sample count from the LHDC/LLAC branch. */
uint32_t lhdcGetSampleSize(uint32_t *);
/* Parse LHDC/LLAC frame length and split-channel information. */
bool lhdcFetchFrameInfo(uint32_t *, uint8_t *, LhdcV4FrameInfo *);
/* Set output channels and synchronize LLAC core state. */
uint32_t lhdcChannelSelsect(uint32_t *, uint8_t);
/* Query full wrapper memory requirements for a version. */
uint32_t lhdc_util_dec_get_mem_req(uint16_t, uint32_t *);
/* Validate and bind 16-byte-aligned caller workspace without allocating or releasing storage. */
int32_t lhdc_util_init_decoder_in_memory(void *memory, size_t bytes, uint32_t bits,
                                         uint32_t rate, uint32_t bitrate, uint16_t version);
#ifdef __cplusplus
}
#endif
#endif
