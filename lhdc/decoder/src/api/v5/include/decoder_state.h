#ifndef LHDC_DECODER_SRC_API_V5_INCLUDE_DECODER_STATE_H
#define LHDC_DECODER_SRC_API_V5_INCLUDE_DECODER_STATE_H
#include <lhdc/decoder/v5.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct LhdcV5Decoder {
    uint8_t type;
    uint32_t sample_rate, bits_per_samples, target_bitrate;
    uint8_t channel_type;
    uint32_t channel_num, audio_format;
    bool interleaved;
    struct {
        uint8_t *priv;
        uint32_t input_bytes, output_samples, bitrate;
    } dec;
    int32_t *dec_buf;
    LhdcV5LogCallback log_callback;
    char log_message[512];
} LhdcV5Decoder;
#endif
