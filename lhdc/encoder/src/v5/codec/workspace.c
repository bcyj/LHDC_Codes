#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/codec/lhdcv5_workspace_types.h"
#include "../include/codec/v5_extended.h"
#include <string.h>

/* Query an internal encoding-workspace address by partition index. */
void *v5f_lhdc_v5_get_mem_content_addr(LHDCV5_Workspace *workspace,
                                       enum lhdc_v5_enc_workspace_content_index index) {
    return v5_workspace_get((void *)workspace, index);
}

/* Encoding workspace: calculate workspace size. */
int32_t v5f_lhdc_v5_enc_workspace_get_size(int32_t ch, int32_t *size) {
    return v5_workspace_get_size(ch, (void *)size);
}

/* Encoding workspace: establish partitions and internal pointers. */
int32_t v5f_lhdc_v5_enc_workspace_init(int32_t ch, LHDCV5_Workspace *workspace) {
    return v5_workspace_init(ch, (void *)workspace);
}

/* Encoding workspace: calculate workspace size. */
int32_t v5_workspace_get_size(int32_t ch, int32_t *size) {
    if ((uint32_t)ch - 1u >= LHDC_V5_WORKSPACE_MAX_CHANNELS || size == NULL)
        return LHDC_V5_ENC_ERROR;
    *size = (int32_t)(V5_WORKSPACE_FIXED + (size_t)ch * sizeof(v5_mdct_channel));
    return LHDC_V5_ENC_OK;
}

/* Encoding workspace: establish partitions and internal pointers. */
int32_t v5_workspace_init(int32_t ch, void *workspace) {
    int32_t total;
    if (workspace == NULL || v5_workspace_get_size(ch, &total) != 0)
        return LHDC_V5_ENC_ERROR;
    memset(workspace, 0, (size_t)total);
    v5_workspace_directory *ws = workspace;
    uint8_t *base = workspace;
    size_t cursor = V5_ALIGN8(sizeof(*ws));
    const size_t sizes[LHDC_V5_WORKSPACE_SLOT_COUNT] = {
        sizeof(v5_frame_header),        sizeof(v5_encoder_core),
        sizeof(LHDCV5_MdctWorkspace),   (size_t)ch * sizeof(v5_mdct_channel),
        sizeof(v5_arithmetic_state),    sizeof(v5_bit_writer),
        LHDC_V5_CHANNEL_ENCODED_BYTES,  LHDC_V5_CHANNEL_ENCODED_BYTES,
        LHDC_V5_TRANSFORM_BUFFER_BYTES, LHDC_V5_TRANSFORM_BUFFER_BYTES,
        LHDC_V5_MDCT_WINDOW_BYTES,      LHDC_V5_TRANSFORM_BUFFER_BYTES};
    ws->header = base + cursor;
    cursor += V5_ALIGN8(sizes[0]);
    ws->encoder = base + cursor;
    cursor += V5_ALIGN8(sizes[1]);
    ws->mdct = base + cursor;
    cursor += V5_ALIGN8(sizes[2]);
    ws->channels = base + cursor;
    cursor += V5_ALIGN8(sizes[3]);
    ws->arithmetic = base + cursor;
    cursor += V5_ALIGN8(sizes[4]);
    ws->bit_writer = base + cursor;
    cursor += V5_ALIGN8(sizes[5]);
    ws->encoded_ch0 = base + cursor;
    cursor += V5_ALIGN8(sizes[6]);
    ws->encoded_ch1 = base + cursor;
    cursor += V5_ALIGN8(sizes[7]);
    ws->mdct_twid1 = base + cursor;
    cursor += V5_ALIGN8(sizes[8]);
    ws->mdct_twid2 = base + cursor;
    cursor += V5_ALIGN8(sizes[9]);
    ws->window = base + cursor;
    cursor += V5_ALIGN8(sizes[10]);
    ws->fft_twid = base + cursor;
    cursor += V5_ALIGN8(sizes[11]);

    return LHDC_V5_ENC_OK;
}

/* Query an internal encoding-workspace address by partition index. */
void *v5_workspace_get(v5_workspace_directory *workspace, uint32_t index) {
    void *contents[LHDC_V5_WORKSPACE_SLOT_COUNT] = {
        workspace->header,      workspace->encoder,     workspace->mdct,
        workspace->channels,    workspace->arithmetic,  workspace->bit_writer,
        workspace->encoded_ch0, workspace->encoded_ch1, workspace->mdct_twid1,
        workspace->mdct_twid2,  workspace->window,      workspace->fft_twid,
    };
    return contents[index];
}
