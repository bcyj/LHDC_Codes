#ifndef LHDC_ENCODER_SRC_V4_CODEC_LZH_ENCODE_H
#define LHDC_ENCODER_SRC_V4_CODEC_LZH_ENCODE_H
#include <stdint.h>

#define LHDC_V4_LZH_HASH_BITS 11u
#define LHDC_V4_LZH_HASH_SIZE 2048u
#define LHDC_V4_LZH_INPUT_LIMIT 2048u
#define LHDC_V4_LZH_MIN_MATCH 4u
#define LHDC_V4_LZH_MAX_MATCH 255u
#define LHDC_V4_LZH_MAX_DISTANCE 509u
#define LHDC_V4_LZH_INSERT_STRIDE 8u
#define LHDC_V4_LZH_LITERAL_LIMIT 255u
#define LHDC_V4_LZH_HASH_MULTIPLIER 0x9e3779b1u

/* Clear the dictionary per call; store positions plus one so zero denotes empty. Workspace belongs to the encoder instance. */
typedef struct LhdcV4LzhWorkspace {
    uint16_t positions[LHDC_V4_LZH_HASH_SIZE];
} LhdcV4LzhWorkspace;

/* Output short-distance byte LZ; return zero for invalid input or insufficient capacity. */
uint32_t v4f_lzh_enc(LhdcV4LzhWorkspace *workspace, const uint8_t *input, uint32_t size,
                     uint8_t *output, uint32_t capacity);
/* Compute exact output length using identical parsing without writing output storage. */
uint32_t v4f_lzh_enc_eval(LhdcV4LzhWorkspace *workspace, const uint8_t *input, uint32_t size,
                          uint32_t capacity);
#endif
