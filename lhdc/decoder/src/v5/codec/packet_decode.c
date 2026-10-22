#include "../../../include/lhdc/decoder/status.h"
#include "../../../../common/include/lhdc_alignment.h"
#include "../include/lhdc_workspace_layout.h"
#include "../include/lhdc_decode.h"
#include "../include/lhdc_dec_control.h"
#include <stdio.h>
#include <string.h>
static const unsigned char channel_scramble[16] = {

    4, 0, 1, 5, 7, 3, 2, 6, 6, 3, 7, 0, 2, 1, 4, 5};
/* Parse V5 headers, descramble/decode per-channel payloads, and maintain history buffers/frame counters. */
int lhdc_v5_dec_decode(uint8_t *input_buffer, int *output_buffer, int *input_buffer_size,
                       void *heap_memory) {
    if (!input_buffer || !output_buffer || !input_buffer_size || !heap_memory)
        return LHDC_V5_DEC_CORE_ERROR;

    tdata_all_buffer_struct *tdata_all_buffer =
        lhdc_v5_dec_get_mem_content_addr(heap_memory, DBUFFER_INDEX);
    time_process_top_struct *time_process_top = tdata_all_buffer->time_mem;
    int *stereo_mono = lhdc_v5_dec_get_mem_content_addr(heap_memory, CHANNEL_MODE_INDEX);
    uint8_t *input_buffer_channel =
        lhdc_v5_dec_get_mem_content_addr(heap_memory, ENCODED_DATA_CH_INDEX);
    int input_buffer_bytes = 0;
    int ch_num = tdata_all_buffer->ch_num;
    int time_ov_top = tdata_all_buffer->max_overlap_samples;
    int frame_samples = tdata_all_buffer->frame_samples;
    if (tdata_all_buffer->frame_cnt < 960)
        ++tdata_all_buffer->frame_cnt;
    int header_size = dec_process_header(input_buffer, &input_buffer_bytes);

    for (int channel = 0; channel < ch_num; ++channel) {
        if (*stereo_mono == 0 || *stereo_mono == channel + 1) {
            int *prev_frame_data =
                lhdc_v5_dec_get_mem_content_addr(heap_memory, FREQ_CH_FPREV_INDEX);
            time_process_top->time_process_ch_num->prev_frame_data =
                prev_frame_data + channel * time_ov_top;
            memcpy(input_buffer_channel, input_buffer + channel * input_buffer_bytes + header_size,
                   input_buffer_bytes);
            uint8_t tmp_byte[8] = {0};
            const uint8_t *permutation = channel_scramble + (input_buffer_channel[8] & 1) * 8;
            for (int index = 0; index < 8; ++index)
                tmp_byte[permutation[index]] = input_buffer_channel[index];
            const uint8_t xor_mask[8] = {

                0xff, 0xe7, 0x7a, 0xb3, 0xda, 0xe5, 0xcd, 0x73};
            for (int index = 0; index < 8; ++index)
                input_buffer_channel[index] = tmp_byte[index] ^ xor_mask[index];
            if (lhdc_dec_top(heap_memory, (uint32_t *)input_buffer_channel, input_buffer_bytes,
                             output_buffer, channel) < 0)
                return LHDC_V5_DEC_CORE_ERROR;
        }
        output_buffer += frame_samples;
    }
    *input_buffer_size = tdata_all_buffer->channel_frame_bytes * ch_num + header_size;
    return LHDC_DEC_OK;
}
