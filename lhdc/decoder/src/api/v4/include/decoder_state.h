#ifndef LHDC_DECODER_SRC_API_V4_INCLUDE_DECODER_STATE_H
#define LHDC_DECODER_SRC_API_V4_INCLUDE_DECODER_STATE_H
#include "../../../v4/include/lhdc_dec.h"
#include <lhdc/decoder/v4.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef union LhdcV4DecoderCore {
    struct {
        struct lhdc_Decoder_t *priv;
        uint16_t ver_code, block_size;
        uint32_t bitrate;
    } lhdc;
    struct {
        struct LLACWorkspaceDirectory *priv;
        uint32_t input_bytes, output_samples, bitrate;
    } llac;
} LhdcV4DecoderCore;
typedef struct LhdcV4Decoder {
    uint8_t type;
    uint32_t sample_rate, bits_per_samples, target_bitrate;
    uint8_t channel_type;
    uint32_t chunk_cnt;
    uint8_t cbuf_store[2048];
    cirbuf_s cbuf;
    int32_t dec_buf[768];
    LhdcV4DecoderCore dec;
    int32_t *dec_buf_out;
    LhdcV4LogCallback log_callback;
    char log_message[512];
} LhdcV4Decoder;
#endif
