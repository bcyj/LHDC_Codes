#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DEC_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DEC_H
#include <stdint.h>
#include "../../../../common/include/dsp/float_compat.h"
#include "lhdc_v5_core_types.h"
#include "types/lhdc_v5_bit_inputter_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Extract per-channel payload length and return extended header length. */
int dec_process_header(unsigned char *encoded_frame, int *enc_frm_len);
/* Compute total encoded frame bytes using channel count. */
int lhdc_v5_dec_get_encoded_frame_size(unsigned char *encoded_frame, int *encoded_frame_size,
                                       void *workspace);
/* Read the META flag and eight metadata bytes. */
int lhdc_v5_dec_get_meta_flag_and_data(unsigned char *encoded_frame, int *meta_flag,
                                       unsigned char *meta_data);
/* Query workspace-directory pointers by internal index enum. */
void *lhdc_v5_dec_get_mem_content_addr(void *heap_memory,
                                       lhdc_v5_dec_workspace_content_index index);
/* Initialize an input bit reader with trailing prefetch padding. */
void lhdc_v5_bit_reader_init(nbyte_read_struct *bit_inputter, unsigned int *umem32,
                             unsigned int size);
/* Compute level offset, retaining the cap of 30 for indices >= 504. */
float lhdc_v5_dec_gain_offset(int offset_idx, int start, float offset_jump);
/* Clear 512 offsets and compute level origin/step by depth, rate, and duration. */
void lhdc_v5_dec_init_gain_table(int, int, int, int, float *, int *, float *);
#ifdef __cplusplus
}
#endif
#endif
