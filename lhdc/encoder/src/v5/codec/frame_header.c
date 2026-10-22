#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/scalar.h"
#include "../include/codec/lhdcv5_workspace_types.h"
#include "../include/codec/v5_extended.h"
#include <string.h>
#include "frame_header.h"

/* Write LHDC headers according to version/extension flags. */
int32_t v5f_enc_process_header(hdr_s *hdr, int32_t ch, int32_t *enc_frm_len_usable,
                               uint8_t *encoded_frame) {
    return v5_process_header((void *)hdr, ch, (void *)enc_frm_len_usable, (void *)encoded_frame);
}

/* Encoding workspace: read encoded frame size. */
int32_t v5f_lhdc_v5_enc_get_encoded_frame_size(int32_t *encoded_frame_size,
                                               LHDCV5_Workspace *workspace) {
    return v5_get_encoded_frame_size((void *)encoded_frame_size, (void *)workspace);
}

/* Encoding workspace: set bitstream version. */
int32_t v5f_lhdc_v5_enc_set_version(int32_t version, LHDCV5_Workspace *workspace) {
    return v5_set_version(version, (void *)workspace);
}

/* Encoding workspace: set the JAS extension flag. */
int32_t v5f_lhdc_v5_enc_set_jas_flag(int32_t jas_flag, LHDCV5_Workspace *workspace) {
    return v5_set_jas_flag(jas_flag, (void *)workspace);
}

/* Encoding workspace: set the AR extension flag. */
int32_t v5f_lhdc_v5_enc_set_ar_flag(int32_t ar_flag, LHDCV5_Workspace *workspace) {
    return v5_set_ar_flag(ar_flag, (void *)workspace);
}

/* Encoding workspace: set the LARC extension flag. */
int32_t v5f_lhdc_v5_enc_set_larc_flag(int32_t larc_flag, LHDCV5_Workspace *workspace) {
    return v5_set_larc_flag(larc_flag, (void *)workspace);
}

/* Encoding workspace: set metadata flag/data. */
int32_t v5f_lhdc_v5_enc_set_meta_flag_and_data(int32_t meta_flag, uint8_t *meta_data,
                                               int32_t loop_count, LHDCV5_Workspace *workspace) {
    return v5_set_meta(meta_flag, (void *)meta_data, loop_count, (void *)workspace);
}

/* Encoding workspace: read the JAS extension flag. */
int32_t v5f_lhdc_v5_enc_get_jas_flag(int32_t *jas_flag, LHDCV5_Workspace *workspace) {
    return v5_get_jas_flag((void *)jas_flag, (void *)workspace);
}

/* Encoding workspace: read the AR extension flag. */
int32_t v5f_lhdc_v5_enc_get_ar_flag(int32_t *ar_flag, LHDCV5_Workspace *workspace) {
    return v5_get_ar_flag((void *)ar_flag, (void *)workspace);
}

/* Encoding workspace: read the LARC extension flag. */
int32_t v5f_lhdc_v5_enc_get_larc_flag(int32_t *larc_flag, LHDCV5_Workspace *workspace) {
    return v5_get_larc_flag((void *)larc_flag, (void *)workspace);
}

/* Encoding workspace: read metadata flag/data. */
int32_t v5f_lhdc_v5_enc_get_meta_flag_and_data(int32_t *meta_flag, uint8_t *meta_data,
                                               LHDCV5_Workspace *workspace) {
    return v5_get_meta((void *)meta_flag, (void *)meta_data, (void *)workspace);
}

/* Encoding workspace: set bitstream version. */
int32_t v5_set_version(int32_t version, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_set_bits(version, LHDC_V5_HEADER_VERSION_WIDTH,
                                        LHDC_V5_HEADER_VERSION_SHIFT, workspace);
}

/* Encoding workspace: set the JAS extension flag. */
int32_t v5_set_jas_flag(int32_t jas_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_set_bits(jas_flag, 1, LHDC_V5_HEADER_JAS_SHIFT, workspace);
}

/* Encoding workspace: read the JAS extension flag. */
int32_t v5_get_jas_flag(int32_t *jas_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_get_bits(jas_flag, 1, LHDC_V5_HEADER_JAS_SHIFT, workspace);
}

/* Encoding workspace: set the AR extension flag. */
int32_t v5_set_ar_flag(int32_t ar_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_set_bits(ar_flag, 1, LHDC_V5_HEADER_AR_SHIFT, workspace);
}

/* Encoding workspace: read the AR extension flag. */
int32_t v5_get_ar_flag(int32_t *ar_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_get_bits(ar_flag, 1, LHDC_V5_HEADER_AR_SHIFT, workspace);
}

/* Encoding workspace: set the LARC extension flag. */
int32_t v5_set_larc_flag(int32_t larc_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_set_bits(larc_flag, 1, LHDC_V5_HEADER_LARC_SHIFT, workspace);
}

/* Encoding workspace: read the LARC extension flag. */
int32_t v5_get_larc_flag(int32_t *larc_flag, v5_workspace_directory *workspace) {
    return v5i_clean_frame_api_get_bits(larc_flag, 1, LHDC_V5_HEADER_LARC_SHIFT, workspace);
}

/* Encoding workspace: set metadata flag/data. */
int32_t v5_set_meta(int32_t meta_flag, const uint8_t *meta_data, int32_t loop_count,
                    v5_workspace_directory *workspace) {
    if ((uint32_t)meta_flag > 1 || meta_data == NULL || loop_count < 0 || workspace == NULL)
        return LHDC_V5_ENC_ERROR;
    v5_frame_header *h = workspace->header;
    h->enc_frm_len_need_update = 1;
    h->info = (uint16_t)((h->info & LHDC_V5_HEADER_WITHOUT_META_MASK) |
                         ((uint32_t)meta_flag << LHDC_V5_HEADER_META_SHIFT));
    if (meta_flag)
        memcpy(h->ext_data + LHDC_V5_META_OFFSET, meta_data, LHDC_V5_META_BYTES);
    h->meta_data_loop_count = meta_flag ? loop_count : 0;
    return LHDC_V5_ENC_OK;
}

/* Encoding workspace: read metadata flag/data. */
int32_t v5_get_meta(int32_t *meta_flag, uint8_t *meta_data, v5_workspace_directory *workspace) {
    if (meta_flag == NULL || meta_data == NULL || workspace == NULL)
        return LHDC_V5_ENC_ERROR;
    const v5_frame_header *h = workspace->header;
    *meta_flag = h->info >> LHDC_V5_HEADER_META_SHIFT;
    if (*meta_flag)
        memcpy(meta_data, h->ext_data + LHDC_V5_META_OFFSET, LHDC_V5_META_BYTES);
    return LHDC_V5_ENC_OK;
}

/* Encoding workspace: read encoded frame size. */
int32_t v5_get_encoded_frame_size(int32_t *encoded_frame_size, v5_workspace_directory *workspace) {
    if (encoded_frame_size == NULL || workspace == NULL)
        return LHDC_V5_ENC_ERROR;
    const v5_frame_header *h = workspace->header;
    const v5_encoder_core *core = workspace->encoder;
    *encoded_frame_size = h->enc_frm_len_provided * core->channels +
                          ((h->info & LHDC_V5_HEADER_LARC_MASK) ? LHDC_V5_HEADER_LARC_BYTES
                                                                : LHDC_V5_HEADER_BASE_BYTES);
    return LHDC_V5_ENC_OK;
}

/* Write LHDC headers according to version/extension flags. */
int32_t v5_process_header(v5_frame_header *hdr, int32_t ch, int32_t *enc_frm_len_usable,
                          uint8_t *encoded_frame) {
    uint16_t original = hdr->info;
    uint16_t info = original;
    int32_t meta_present = 0;
    int32_t header_size = (original & LHDC_V5_HEADER_LARC_MASK) ? LHDC_V5_HEADER_LARC_BYTES
                                                                : LHDC_V5_HEADER_BASE_BYTES;
    *enc_frm_len_usable = hdr->enc_frm_len_provided;
    if (original & LHDC_V5_HEADER_META_MASK) {
        if (hdr->meta_data_loop_count < 1) {
            info &= LHDC_V5_HEADER_WITHOUT_META_MASK;
            hdr->enc_frm_len_need_update = 1;
        } else {
            for (int32_t i = 0; i < LHDC_V5_META_BYTES; ++i)
                encoded_frame[header_size + i] = hdr->ext_data[i + LHDC_V5_META_OFFSET];
            *enc_frm_len_usable -= ch != 0 ? LHDC_V5_META_BYTES / ch : 0;
            hdr->meta_data_loop_count -= 1;
            meta_present = 1;
        }
    }
    info = (uint16_t)((info & LHDC_V5_HEADER_CONTROL_MASK) | (uint16_t)*enc_frm_len_usable);
    hdr->info = info;
    encoded_frame[0] = (uint8_t)info;
    encoded_frame[1] = (uint8_t)(info >> 8);
    return header_size + (meta_present ? LHDC_V5_META_BYTES : 0);
}
