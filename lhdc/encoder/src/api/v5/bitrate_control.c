#include "../../../../common/include/lhdc_log.h"
#include "../../../../common/include/lhdc_alignment.h"
#include <audio_math/float_convert.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../v5/include/encoder_internal.h"
#include "../../v5/include/codec/codec_internal.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bluetooth encoder wrapper: read current target bitrate. */
int32_t v5f_lhdcv5_util_get_target_bitrate(lhdcv5_cb_t *handle, uint32_t *bitrate) {
    uint32_t last_bitrate;
    int32_t func_ret;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (bitrate == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    if ((handle->enc).lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    lhdcv5_para_t *lhdcv5 = handle->enc.lhdcv5;
    last_bitrate = lhdcv5->lastBitrate;
    if (!((last_bitrate < (uint32_t)lhdcv5->bitrate_table[0]) ||
          ((uint32_t)lhdcv5->bitrate_table[8] < last_bitrate))) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *bitrate = last_bitrate * 1000;
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid bit rate (%u kbps)!", __func__,
                   last_bitrate);
    func_ret = LHDCV5_FRET_ERROR;

    return func_ret;
}

/* Bluetooth encoder wrapper: set target bitrate index and synchronize the encoding budget. */
int32_t v5f_lhdcv5_util_set_target_bitrate_inx(lhdcv5_cb_t *handle, uint32_t bitrate_inx,
                                               uint32_t *bitrate_inx_set, bool upd_qual_status) {
    uint32_t clamped_bitrate_inx;
    uint32_t upd_bitrate_inx;
    int32_t func_ret;
    lhdcv5_para_t *lhdcv5;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (bitrate_inx_set == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (bitrate_inx >= 10) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: Input bit rate (index) is invalid (%u)!", __func__, bitrate_inx);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    if ((handle->enc).lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder core is NULL", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    if (upd_qual_status) {
        ((handle->enc).lhdcv5)->qualityStatus = bitrate_inx;
    }
    if (bitrate_inx == 9) {
        lhdcv5 = (handle->enc).lhdcv5;
        if (lhdcv5 == (lhdcv5_para_t *)0x0) {
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        } else {
            lhdcv5->dnBitrateCnt = 0;
            lhdcv5->dnBitrateSum = 0;
        }
        lhdcv5 = (handle->enc).lhdcv5;
        if (lhdcv5 == (lhdcv5_para_t *)0x0) {
            bitrate_inx = 5;
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        } else {
            bitrate_inx = 5;
            lhdcv5->upBitrateCnt = 0;
            lhdcv5->upBitrateSum = 0;
        }
    }
    lhdcv5 = (handle->enc).lhdcv5;
    clamped_bitrate_inx = lhdcv5->min_bitrate_inx;
    if ((int32_t)lhdcv5->min_bitrate_inx <= (int32_t)bitrate_inx) {
        clamped_bitrate_inx = bitrate_inx;
    }
    upd_bitrate_inx = lhdcv5->max_bitrate_inx;
    if ((int32_t)clamped_bitrate_inx <= (int32_t)lhdcv5->max_bitrate_inx) {
        upd_bitrate_inx = clamped_bitrate_inx;
    }
    func_ret = v5f_lhdcv5_encoder_set_target_bitrate_inx(lhdcv5, upd_bitrate_inx);
    if (func_ret == 0) {
        *bitrate_inx_set = upd_bitrate_inx;
        func_ret = LHDCV5_FRET_SUCCESS;

        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Fail to set target bit rate (index) (%d)!", __func__, func_ret);
    func_ret = LHDCV5_FRET_ERROR;
    return func_ret;
}

/* Bluetooth encoder wrapper: reset downward bitrate-adjustment statistics. */
int32_t v5f_lhdcv5_util_reset_down_bitrate(lhdcv5_abr_para_t *handle) {
    if (handle != (lhdcv5_abr_para_t *)0x0) {
        handle->dnBitrateCnt = 0;
        handle->dnBitrateSum = 0;
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
    return LHDCV5_FRET_INVALID_HANDLE_PARA;
}

/* Bluetooth encoder wrapper: reset upward bitrate-adjustment statistics. */
int32_t v5f_lhdcv5_util_reset_up_bitrate(lhdcv5_abr_para_t *handle) {
    if (handle != (lhdcv5_abr_para_t *)0x0) {
        handle->upBitrateCnt = 0;
        handle->upBitrateSum = 0;
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
    return LHDCV5_FRET_INVALID_HANDLE_PARA;
}

/* Return the first level covering the bitrate; the caller has checked it does not exceed level nine. */
static uint32_t supported_bitrate_index(const lhdcv5_para_t *lhdcv5, uint32_t bitrate) {
    uint32_t index = 0;
    while (index < 9 && (uint32_t)lhdcv5->bitrate_table[index] < bitrate)
        ++index;
    return index;
}

/* Set target bitrate level and mark frame information for update when the effective level changes. */
int32_t v5f_lhdcv5_encoder_set_target_bitrate_inx(lhdcv5_para_t *lhdcv5, uint32_t bitrate_inx) {
    if (!lhdcv5) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_PARA;
    }
    if (!lhdcv5->lh5_enc) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Encoder workspace is NULL", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_ENC;
    }
    if (bitrate_inx >= 9) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is invalid (%u)!",
                       __func__, bitrate_inx);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    uint32_t last_bitrate = lhdcv5->lastBitrate;
    if ((uint32_t)lhdcv5->bitrate_table[9] < last_bitrate) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Fail to get bit rate (index) (%u)!",
                       __func__, last_bitrate);
        return LHDCV5_FRET_ERROR;
    }
    if (supported_bitrate_index(lhdcv5, last_bitrate) == bitrate_inx)
        return LHDCV5_FRET_SUCCESS;
    uint32_t target_bitrate = lhdcv5->bitrate_table[bitrate_inx];
    lhdcv5->updateFramneInfo = true;
    lhdcv5->lastBitrate = target_bitrate;
    return LHDCV5_FRET_SUCCESS;
}

/* Bluetooth encoder wrapper: set the maximum bitrate index. */
int32_t v5f_lhdcv5_util_set_max_bitrate_inx(lhdcv5_cb_t *handle, uint32_t max_bitrate_inx,
                                            uint32_t *max_bitrate_inx_set) {
    int32_t func_ret;
    lhdcv5_para_t *lhdcv5;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (max_bitrate_inx_set == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (max_bitrate_inx - 5 >= 4) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: Input MAX. bit rate (index) is invalid (%u)!", __func__,
                       max_bitrate_inx);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    lhdcv5 = (handle->enc).lhdcv5;
    if (lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    if (lhdcv5->max_bitrate_inx == max_bitrate_inx) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *max_bitrate_inx_set = ((handle->enc).lhdcv5)->max_bitrate_inx;
        return func_ret;
    }
    lhdcv5->max_bitrate_inx = max_bitrate_inx;
    if ((max_bitrate_inx < lhdcv5->qualityStatus) && (lhdcv5->qualityStatus != 9)) {
        lhdcv5->qualityStatus = max_bitrate_inx;
    }

    if ((uint32_t)lhdcv5->bitrate_table[max_bitrate_inx] >= lhdcv5->lastBitrate) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *max_bitrate_inx_set = ((handle->enc).lhdcv5)->max_bitrate_inx;
        return func_ret;
    }
    lhdcv5->lastBitrate = lhdcv5->bitrate_table[max_bitrate_inx];
    func_ret = v5f_lhdcv5_encoder_set_target_bitrate_inx(lhdcv5, max_bitrate_inx);
    if (func_ret == 0) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *max_bitrate_inx_set = ((handle->enc).lhdcv5)->max_bitrate_inx;
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Fail to set target bit rate (index) (%d) (%u)!", __func__, func_ret,
                   lhdcv5->max_bitrate_inx);
    return LHDCV5_FRET_ERROR;
}

/* Bluetooth encoder wrapper: set the minimum bitrate index. */
int32_t v5f_lhdcv5_util_set_min_bitrate_inx(lhdcv5_cb_t *handle, uint32_t min_bitrate_inx,
                                            uint32_t *min_bitrate_inx_set) {
    int32_t func_ret;
    lhdcv5_para_t *lhdcv5;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_CB;

        return func_ret;
    }
    if (min_bitrate_inx_set == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (min_bitrate_inx >= 6) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                       "%s: Input MIN. bit rate (index) is invalid (%u)!", __func__,
                       min_bitrate_inx);
        func_ret = LHDCV5_FRET_INVALID_INPUT_PARAM;

        return func_ret;
    }
    if (handle->enc_type != 1) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!", __func__,
                       handle->enc_type);
        func_ret = LHDCV5_FRET_INVALID_CODEC;

        return func_ret;
    }
    lhdcv5 = (handle->enc).lhdcv5;
    if (lhdcv5 == (lhdcv5_para_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        func_ret = LHDCV5_FRET_INVALID_HANDLE_PARA;

        return func_ret;
    }
    if (lhdcv5->min_bitrate_inx == min_bitrate_inx) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *min_bitrate_inx_set = ((handle->enc).lhdcv5)->min_bitrate_inx;
        return func_ret;
    }
    lhdcv5->min_bitrate_inx = min_bitrate_inx;
    if (lhdcv5->qualityStatus < min_bitrate_inx) {
        lhdcv5->qualityStatus = min_bitrate_inx;
    }

    if (lhdcv5->lastBitrate >= (uint32_t)lhdcv5->bitrate_table[min_bitrate_inx]) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *min_bitrate_inx_set = ((handle->enc).lhdcv5)->min_bitrate_inx;
        return func_ret;
    }
    lhdcv5->lastBitrate = lhdcv5->bitrate_table[min_bitrate_inx];
    func_ret = v5f_lhdcv5_encoder_set_target_bitrate_inx(lhdcv5, min_bitrate_inx);
    if (func_ret == 0) {
        func_ret = LHDCV5_FRET_SUCCESS;
        *min_bitrate_inx_set = ((handle->enc).lhdcv5)->min_bitrate_inx;
        return func_ret;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Fail to set target bit rate (index) (%d) (%u)!", __func__, func_ret,
                   lhdcv5->min_bitrate_inx);
    return LHDCV5_FRET_ERROR;
}

/* Bluetooth encoder wrapper: adapt bitrate to transmit-queue load. */
int32_t v5f_lhdcv5_util_adjust_bitrate(lhdcv5_cb_t *handle, LHDCV5_ENC_TYPE_T *enc_type_ptr,
                                       lhdcv5_abr_para_t **abr_para_ptr) {
    const char *log_format;

    if (handle == (lhdcv5_cb_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
        return LHDCV5_FRET_INVALID_HANDLE_CB;
    }
    if (enc_type_ptr == (LHDCV5_ENC_TYPE_T *)0x0) {
        log_format = "%s: encode type is NULL!";
    } else {
        if (abr_para_ptr != (lhdcv5_abr_para_t **)0x0) {
            *enc_type_ptr = handle->enc_type;
            if (handle->enc_type != 1) {
                lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode type (%u)!",
                               __func__, handle->enc_type);
                return LHDCV5_FRET_INVALID_CODEC;
            }
            if ((handle->enc).lhdcv5 != (lhdcv5_para_t *)0x0) {
                *abr_para_ptr = (lhdcv5_abr_para_t *)(handle->enc).lhdcv5;
                return LHDCV5_FRET_SUCCESS;
            }
            lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
            return LHDCV5_FRET_INVALID_HANDLE_PARA;
        }
        log_format = "%s: abr para is NULL!";
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", log_format, __func__);
    return LHDCV5_FRET_INVALID_INPUT_PARAM;
}

/* Bluetooth encoder wrapper: look up bitrate by index. */
int32_t v5f_lhdcv5_util_get_bitrate(lhdcv5_para_t *lhdcv5, uint32_t bitrate_inx,
                                    uint32_t *bitrate) {
    if (!lhdcv5)
        return LHDCV5_FRET_INVALID_HANDLE_PARA;

    if (bitrate == (uint32_t *)0x0) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);

        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate_inx < 10) {
        *bitrate = lhdcv5->bitrate_table[bitrate_inx];
        return LHDCV5_FRET_SUCCESS;
    }
    lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                   "%s: Input bit rate (index) is out of range (%u)!", __func__, bitrate_inx);
    return LHDCV5_FRET_INVALID_INPUT_PARAM;
}

/* Bluetooth encoder wrapper: find the target bitrate's index. */
int32_t v5f_lhdcv5_util_get_bitrate_inx(lhdcv5_para_t *lhdcv5, uint32_t bitrate,
                                        uint32_t *bitrate_inx) {
    if (!lhdcv5)
        return LHDCV5_FRET_INVALID_HANDLE_PARA;

    if (!bitrate_inx) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Input parameter is NULL!", __func__);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate > (uint32_t)lhdcv5->bitrate_table[9])
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    *bitrate_inx = supported_bitrate_index(lhdcv5, bitrate);
    return LHDCV5_FRET_SUCCESS;
}

/* Select the sample-rate-specific bitrate table without reading or changing its values. */
static int32_t *sample_rate_bitrate_table(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq) {
    switch (sampling_freq) {
    case LHDC_V5_SAMPLE_RATE_44100:
        return lhdcv5->bitrate_tables[0];
    case LHDC_V5_SAMPLE_RATE_48000:
        return lhdcv5->bitrate_tables[1];
    case LHDC_V5_SAMPLE_RATE_96000:
        return lhdcv5->bitrate_tables[2];
    case LHDC_V5_SAMPLE_RATE_192000:
        return lhdcv5->bitrate_tables[3];
    default:
        return NULL;
    }
}

/* Validate query arguments and copy the current sample-rate bitrate table in order. */
int32_t v5f_lhdcv5_util_get_bitrate_t(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                      int32_t *bitrate_table, uint32_t num_bitrate_items,
                                      uint32_t get_tbl_reserved) {
    if (!lhdcv5)
        return LHDCV5_FRET_INVALID_HANDLE_PARA;

    int32_t *bitrate_tbl_ptr = sample_rate_bitrate_table(lhdcv5, sampling_freq);
    if (!bitrate_tbl_ptr) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid sampling frequency (%u)!",
                       __func__, sampling_freq);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (num_bitrate_items != 10 || !bitrate_table || get_tbl_reserved != 0x20220421)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    /* Read and write each entry in sequence when output overlaps the internal table. */
    for (uint32_t i = 0; i < 10; ++i)
        bitrate_table[i] = bitrate_tbl_ptr[i];
    return LHDCV5_FRET_SUCCESS;
}

/* Bluetooth encoder wrapper: validate and update the current sample-rate bitrate table. */
int32_t v5f_lhdcv5_util_set_bitrate_t(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                      int32_t *bitrate_table, uint32_t num_bitrate_items,
                                      uint32_t get_tbl_reserved) {
    if (!lhdcv5)
        return LHDCV5_FRET_INVALID_HANDLE_PARA;

    int32_t *bitrate_tbl_ptr = sample_rate_bitrate_table(lhdcv5, sampling_freq);
    if (!bitrate_tbl_ptr) {
        lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid sampling frequency (%u)!",
                       __func__, sampling_freq);
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (num_bitrate_items != 10 || !bitrate_table || get_tbl_reserved != 0x20220421)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    int32_t bitrate0, bitrate1, bitrate2, bitrate3;
    bitrate0 = *bitrate_table;
    if (0x270 < (bitrate0 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate1 = bitrate_table[1];
    if (0x270 < (bitrate1 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate1 < bitrate0) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate2 = bitrate_table[2];
    if (0x270 < (bitrate2 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate2 < bitrate1) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate3 = bitrate_table[3];
    if (0x270 < (bitrate3 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate3 < bitrate2) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate2 = bitrate_table[4];
    if (0x270 < (bitrate2 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate2 < bitrate3) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate3 = bitrate_table[5];
    if (0x270 < (bitrate3 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate3 < bitrate2) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate2 = bitrate_table[6];
    if (0x270 < (bitrate2 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate2 < bitrate3) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate3 = bitrate_table[7];
    if (0x270 < (bitrate3 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate3 < bitrate2) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    bitrate2 = bitrate_table[8];
    if (0x270 < (bitrate2 - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate2 < bitrate3) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (0x270 < (bitrate_table[9] - 1U) >> 4) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    if (bitrate_table[9] < bitrate2) {
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    }
    *bitrate_tbl_ptr = bitrate0;
    bitrate_tbl_ptr[1] = bitrate1;
    bitrate_tbl_ptr[2] = bitrate_table[2];
    bitrate_tbl_ptr[3] = bitrate_table[3];
    bitrate_tbl_ptr[4] = bitrate_table[4];
    bitrate_tbl_ptr[5] = bitrate_table[5];
    bitrate_tbl_ptr[6] = bitrate_table[6];
    bitrate_tbl_ptr[7] = bitrate_table[7];
    bitrate_tbl_ptr[8] = bitrate_table[8];
    bitrate_tbl_ptr[9] = bitrate_table[9];
    return LHDCV5_FRET_SUCCESS;
}

/* Compute packet frame count/target length from bitrate, MTU, and scheduling period; return the logging result. */
int32_t v5f_cal_frame_size_and_frames_in_packet(lhdcv5_para_t *handle) {
    if (!handle)
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Handle is NULL!", __func__);
    uint32_t sample_bits = handle->bits_per_sample_ui;
    if (sample_bits > 32 || !(UINT64_C(1) << sample_bits & UINT64_C(0x101010000)))
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid bits per sample (%u)!",
                              __func__, sample_bits);
    if (handle->frame_duration != LHDC_V5_FRAME_DURATION_5MS)
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid frame duration (%u)!",
                              __func__, handle->frame_duration);
    uint32_t expected_samples;
    switch (handle->sample_rate) {
    case LHDC_V5_SAMPLE_RATE_44100:
    case LHDC_V5_SAMPLE_RATE_48000:
        expected_samples = 240;
        break;
    case LHDC_V5_SAMPLE_RATE_96000:
        expected_samples = 480;
        break;
    case LHDC_V5_SAMPLE_RATE_192000:
        expected_samples = 960;
        break;
    default:
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid sample rate (%u)!",
                              __func__, handle->sample_rate);
    }
    if (handle->samples_per_frame != expected_samples)
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5",
                              "%s: Invalid samples per frame (%u)!", __func__,
                              handle->samples_per_frame);
    if (handle->encode_interval != 10 && handle->encode_interval != 20)
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid encode interval (%u)!",
                              __func__, handle->encode_interval);
    uint32_t max_mtu_limit = handle->host_mtu_size;
    if (max_mtu_limit < 300)
        return lhdc_log_print(LHDC_LOG_WARN, "LHDC.Encoder.V5", "%s: Invalid AVDTP MTU (%u)!",
                              __func__, max_mtu_limit);
    uint32_t bytes_per_sample = (sample_bits >> 2) & 0x3ffffffe;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                   "%s: bytes_per_sample(%u), handle->bits_per_sample_ui(%u)", __func__,
                   bytes_per_sample, handle->bits_per_sample_ui);
    uint32_t bytes_per_frame = handle->samples_per_frame * bytes_per_sample;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: bytes_per_frame(%u)", __func__,
                   bytes_per_frame);
    uint32_t target_bytes_per_second = handle->actual_bitrate >> 3;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5",
                   "%s: target_bytes_per_second(%u), handle->actual_bitrate(%u)", __func__,
                   target_bytes_per_second, handle->actual_bitrate);
    uint32_t bytes_per_second = handle->sample_rate * bytes_per_sample;
    lhdc_ulong_t bytes_per_tick = (lhdc_ulong_t)(bytes_per_second * handle->encode_interval) / 1000;
    float compress_rate = (float)target_bytes_per_second / (float)bytes_per_second;
    float max_output_bytes_per_frame = compress_rate * (float)bytes_per_frame;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: bytes_per_tick(%llu)", __func__,
                   (unsigned long long)bytes_per_tick);
    uint32_t frames_per_tick =
        (uint32_t)((double)((float)bytes_per_tick / (float)bytes_per_frame) + 0.5);
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: frames_per_tick(%u)", __func__,
                   frames_per_tick);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Validated sample rate, depth, and period restrict scheduling to two or four frames;
     * this power-of-two product is exactly representable in binary32 without extra double conversion. */
    uint32_t max_output_bytes_per_tick =
        (uint32_t)audio_math_truncate_i32_f32(max_output_bytes_per_frame * (float)frames_per_tick);
#else
    uint32_t max_output_bytes_per_tick =
        (uint32_t)(int32_t)(max_output_bytes_per_frame * (float)frames_per_tick);
#endif
    uint32_t packet_per_tick = max_mtu_limit ? max_output_bytes_per_tick / max_mtu_limit : 0;
    if (max_output_bytes_per_tick < max_mtu_limit)
        packet_per_tick = 1;
    uint32_t frames_per_packet = packet_per_tick ? frames_per_tick / packet_per_tick : 0;
    float packet_bytes = max_output_bytes_per_frame * (float)frames_per_packet;
    handle->frame_per_packet = frames_per_packet;
    if ((float)max_mtu_limit <= packet_bytes) {
        do {
            uint32_t previous_count = frames_per_packet;
            frames_per_packet >>= 1;
            if ((previous_count & 1) && previous_count > 1)
                frames_per_packet = previous_count - 1;
            packet_bytes = max_output_bytes_per_frame * (float)frames_per_packet;
        } while ((float)max_mtu_limit <= packet_bytes);
        handle->frame_per_packet = frames_per_packet;
    }
    handle->target_mtu_size = (float)max_mtu_limit <= packet_bytes
                                  ? (frames_per_packet ? max_mtu_limit / frames_per_packet : 0)
                                  : (uint32_t)max_output_bytes_per_frame;
    lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: Host mtu(%u), byte per frame(%u)",
                   __func__, max_mtu_limit, handle->encoded_frame_size);
    return lhdc_log_print(LHDC_LOG_DEBUG, "LHDC.Encoder.V5", "%s: Update frame_per_packet(%u)",
                          __func__, handle->frame_per_packet);
}
