

#ifndef LHDC_RECOVERED_V4_LLAC_ENC_API_H
#define LHDC_RECOVERED_V4_LLAC_ENC_API_H
#include "llac_status.h"
#include "../../../../src/v4/include/types/lhdc_base_types.h"

LLAC_UINT8 *v4f_llac_enc_mem_alloc(void);

void v4f_llac_enc_set_extra_func(LLAC_UINT8 func_index, LLAC_UINT8 func_enable,
                                 LLAC_UINT8 *llac_enc_mem_addr);

bool v4f_llac_enc_get_extra_func(LLAC_UINT8 func_index, LLAC_UINT8 *llac_enc_mem_addr);

/* Initialization/bitrate updates return zero or positive LLAC errors; processing returns bytes or negative errors. */
LLAC_INT v4f_llac_enc_initial(uint32_t sampleRate, int16_t nChannels, int16_t bps_in, int16_t bps_out,
                          LLAC_FLOAT frame_ms, LLAC_INT target_bitrate, LLAC_INT *nBytes,
                          uint32_t *nSamples, LLAC_INT *real_bitrate,
                          LLAC_UINT8 *llac_enc_mem_addr);

LLAC_INT v4f_llac_enc_set_bitrate(LLAC_INT target_bitrate, LLAC_INT *nBytes, LLAC_INT *real_bitrate,
                              LLAC_UINT8 *llac_enc_mem_addr);

LLAC_INT v4f_llac_enc_process(LLAC_INT *input, LLAC_UINT8 *output, LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_enc_mem_free(LLAC_UINT8 *llac_enc_mem_addr);

#endif /* LHDC_RECOVERED_V4_LLAC_ENC_API_H */
