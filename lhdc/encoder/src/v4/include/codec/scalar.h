/* V4 scalar, bit-writing, and cost interfaces.
 * lhdc_v4_numeric_policy.h defines shift counts to avoid undefined oversized C shifts.
 */
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_SCALAR_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_SCALAR_H
#include <stdint.h>
#include <stddef.h>
#include "../types/lhdc_native_types.h"
typedef struct V4BitWrite {
    uint32_t rb;
    union {
        uint32_t u32;
        uint8_t u8[4];
    } db;
} V4BitWrite;
_Static_assert(sizeof(V4BitWrite) == 8, "V4 native bit_write_s size");
_Static_assert(offsetof(V4BitWrite, db) == 4, "V4 native db offset");
/* LLAC workspace directory: llac_mem_content_index defines slots 0..42 with sentinel 43.
 */
typedef LHDCV4_LLACWorkspaceDirectory V4LlacDirectory;
/* Named initialization partition offsets; each transform-buffer set serves both channels by DCT mode. */
typedef struct V4LlacWorkspaceLayout {
    size_t pcm;
    size_t encoder;
    size_t pre_rotation;
    size_t post_rotation;
    size_t mdct_history[2];
    size_t transform_table[2];
    size_t transform_handle[2];
    size_t transform_buffer[2];
    size_t transform_scratch[2];
    size_t transform_scratch2[2];
} V4LlacWorkspaceLayout;
_Static_assert(sizeof(V4LlacDirectory) == 43 * sizeof(void *), "LLAC directory");
void v4_llac_enc_mem_alloc_fft_set(LLAC_INT ch, LLAC_INT dct, LLAC_UINT8 *llac_enc_mem_addr);
int32_t v4_eval_rice_encode_len(uint32_t *rd2, int32_t ns, uint32_t nb);
int32_t v4_BTRtoDIV(int32_t btr, int32_t btr_ext);
void v4_DIVtoBTR(int32_t div, int32_t *btr, int32_t *btr_ext);
void v4_byte_write_int32(int32_t dat, int32_t nb, uint8_t *out);
uint8_t *v4_MoveOutBytes(V4BitWrite *bw, uint8_t *out);
void v4_llac_band_mean_square(LLAC_INT bands_number, const LLAC_INT *acc_coeff_per_band,
                              LLAC_FLOAT *d2, LLAC_FLOAT *d);
void v4_llac_encode_residual_bits(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gain, LLAC_INT L_spec,
                                  LLAC_INT targetBits, LLAC_INT nBits, LLAC_UINT8 *resBits,
                                  LLAC_INT *numResBits, LLAC_INT hrmode);
#endif
