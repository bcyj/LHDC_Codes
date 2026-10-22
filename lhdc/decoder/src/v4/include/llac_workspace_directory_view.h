

#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_WORKSPACE_DIRECTORY_VIEW_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_WORKSPACE_DIRECTORY_VIEW_H
#include <stddef.h>
#include <stdint.h>
/* Parameter slots occupy eight bytes; call sites interpret their actual short/int/float values. */
typedef struct LLACWorkspaceParameters {
    uint32_t samplerate;
    int16_t nchannels, bps_out;
    float frame_ms;
    int32_t nbytes, frame_length, target_bitrate;
    uint16_t extra_nbyte;
    uint8_t extra_nbyte_num, channel_mode;
} LLACWorkspaceParameters;
typedef struct LLACWorkspaceDirectory {
    uint32_t *samplerate;
    int16_t *nchannels;
    int16_t *bps_out;
    float *frame_ms;
    int32_t *nbytes;
    int32_t *frame_length;
    int32_t *target_bitrate;
    uint16_t *extra_nbyte;
    uint8_t *extra_nbyte_num;
    uint8_t *channel_mode;
    void *decoder;
    void *dct_twid1;
    void *dct_twid2;
    void *h_ch0_dct4;
    void *h_ch1_dct4;
    void *h;
    void *scratch_ch0_dct4;
    void *scratch_ch1_dct4;
    void *scratch;
} LLACWorkspaceDirectory;
#endif
