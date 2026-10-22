#include "lhdc_memory.h"

#ifndef _LHDCV5BT_H_
#define _LHDCV5BT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "lhdcv5_api.h"

int32_t lhdcv5BT_free_handle(HANDLE_LHDCV5_BT handle);

int32_t lhdcv5BT_get_handle(uint32_t version, HANDLE_LHDCV5_BT *handle);

/* Query total instance capacity for the current ABI; unknown versions return zero and base alignment is 16 bytes. */
size_t lhdcv5BT_workspace_bytes(uint32_t version);
/* Borrow caller storage; clear the output handle on failure. free_handle does not release workspace. */
int32_t lhdcv5BT_get_handle_in_memory(uint32_t version, void *memory, size_t bytes,
                                      HANDLE_LHDCV5_BT *handle);
/* Create owned storage through instance host callbacks; destroy releases the original allocation address. */
int32_t lhdcv5BT_get_handle_with_allocator(uint32_t version, const LhdcAllocator *allocator,
                                           HANDLE_LHDCV5_BT *handle);


int32_t lhdcv5BT_get_bitrate(HANDLE_LHDCV5_BT handle, uint32_t *bitrate);

int32_t lhdcv5BT_set_bitrate(HANDLE_LHDCV5_BT handle, uint32_t bitrate_inx);

int32_t lhdcv5BT_set_max_bitrate(HANDLE_LHDCV5_BT handle, uint32_t max_bitrate_inx);

int32_t lhdcv5BT_set_min_bitrate(HANDLE_LHDCV5_BT handle, uint32_t min_bitrate_inx);

int32_t lhdcv5BT_adjust_bitrate(HANDLE_LHDCV5_BT handle, uint32_t queueLen);

int32_t lhdcv5BT_set_ext_func_state(HANDLE_LHDCV5_BT handle, LHDCV5_EXT_FUNC_T field, bool enabled,
                                    void *priv, uint32_t priv_data_len);

int32_t lhdcv5BT_init_encoder(HANDLE_LHDCV5_BT handle, uint32_t sampling_freq,
                              uint32_t bits_per_sample, uint32_t bitrate_inx, uint32_t mtu,
                              uint32_t interval);

int32_t lhdcv5BT_get_block_Size(HANDLE_LHDCV5_BT handle, uint32_t *samples_per_frame);

int32_t lhdcv5BT_encode(HANDLE_LHDCV5_BT handle, void *p_in_pcm, uint32_t pcm_bytes,
                        uint8_t *p_out_buf, uint32_t out_buf_bytes, uint32_t *p_out_bytes,
                        uint32_t *p_out_frames);

//
// LHDCV5 Extended APIs
//
// Extended API -- Get Version
int32_t lhdcv5BT_get_user_exApiver(HANDLE_LHDCV5_BT handle, char *userConfig, int32_t clen);

// Extended API -- Get User Config
int32_t lhdcv5BT_get_user_exconfig(HANDLE_LHDCV5_BT handle, char *userConfig, int32_t clen);

// Extended API -- Set User Config
int32_t lhdcv5BT_set_user_exconfig(HANDLE_LHDCV5_BT handle, const char *userConfig,
                                   const int32_t clen);

// Extended API -- Set User Data
void lhdcv5BT_set_user_exdata(HANDLE_LHDCV5_BT handle, const char *userConfig, const int32_t clen);

#ifdef __cplusplus
}
#endif

#endif /* _LHDCV5BT_H_ */
