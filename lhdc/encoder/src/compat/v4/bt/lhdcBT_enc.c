#include "lhdc_memory.h"
#include "../../../../../common/include/lhdc_log.h"
#include "../../../../../common/include/compat/v4/lhdc_v4_constants.h"

#ifndef LHDC_BT_USE_AOSP_REFERENCE
#define LHDC_BT_USE_AOSP_REFERENCE 0
#endif
#if !LHDC_BT_USE_AOSP_REFERENCE
#include <audio_math/byte_order.h>
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Headers, constants, and state declarations for BT extension configuration. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../../../include/lhdc/encoder/v4/lhdcBT.h"
#include "../../../v4/include/codec/encoder.h"
#include "lhdc_cfg.h"
#include "../../../v4/include/types/cirbuf.h"
#include "../../../../include/lhdc/encoder/v4/llac_enc_api.h"

#define LHDC_LOG_TAG "LHDC.BT.V4"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define UP_RATE_TIME_CNT 3000
#define DOWN_RATE_TIME_CNT 4
#define QUEUE_LENGTH_THRESHOLD 4
#define LHDC_BITRATE_ELEMENTS_SIZE (sizeof(auto_bitrate_adjust_table_lhdc) / sizeof(int32_t))
#define LLAC_BITRATE_ELEMENTS_SIZE (sizeof(auto_bitrate_adjust_table_llac) / sizeof(int32_t))
#define LHDC_ABR_DEFAULT_BITRATE (400)
#define LLAC_ABR_DEFAULT_BITRATE (400)

#define AR_ALWAYS_ONx 1
static const int32_t auto_bitrate_adjust_table_lhdc[] = {320, 350, 380, 440, 580, 600};
static const int32_t auto_bitrate_adjust_table_llac[] = {136, 160, 192, 240, 320, 400};
#else
#include "../../../../include/lhdc/encoder/v4/lhdcBT.h"
#include "../../../../include/lhdc/encoder/v4/llac_enc_api.h"
#include <stdlib.h>
#include <string.h>
#define ABR_DOWN_TICKS 4u
#define ABR_UP_TICKS 3000u
#define ABR_STAGES 6u
#define ABR_QUEUE_THRESHOLD 4u
#define ABR_INITIAL_KBPS 400
#define MIN_KBPS 128
#define LIMITED_MIN_KBPS 320
#define AR_PACKET_BYTES 172
#define META_SET_BYTES 16
#define META_GET_BYTES 15
#define GYRO_PACKET_BYTES 20
static const int32_t abr_lhdc[ABR_STAGES] = {320, 350, 380, 440, 580, 600};
static const int32_t abr_llac[ABR_STAGES] = {136, 160, 192, 240, 320, 320};
#endif

_Static_assert(sizeof(ST_LHDC_BT_AR) == 172, "AR packet size");
_Static_assert(offsetof(ST_LHDC_BT_AR, app_ar_enabled) == 16, "AR enable");
_Static_assert(offsetof(ST_LHDC_BT_AR, Ch1_Pos) == 20, "AR positions");
_Static_assert(offsetof(ST_LHDC_BT_AR, Ch1_L_PreGain) == 44, "AR gains");
_Static_assert(sizeof(ST_LHDC_SET_META) == 16, "META set size");
_Static_assert(sizeof(ST_LHDC_GET_META) == 15, "META get size");
_Static_assert(sizeof(ST_LHDC_BT_AR_GYRO) == 20, "GYRO packet size");

#if !LHDC_BT_USE_AOSP_REFERENCE
/* Read LHDC/LLAC ABR ladder bitrate within the maximum-quality limit. */
static int32_t abr_bitrate(lhdc_cb_t *control, uint32_t stage) {
    int32_t maximum = v4f_lhdc_util_get_bitrate(control->enc_type == ENC_TYPE_LHDC
                                                    ? control->enc.lhdc->limitBitRateStatus
                                                    : control->enc.llac->limitBitRateStatus);
    int32_t rate = (control->enc_type == ENC_TYPE_LHDC ? abr_lhdc : abr_llac)[stage];
    return rate < maximum ? rate : maximum;
}
#endif

#if !LHDC_BT_USE_AOSP_REFERENCE
/* Update ABR target bitrate and core frame-length parameters. */
static int32_t abr_apply(lhdc_cb_t *control, int32_t bitrate) {
    if (control->enc_type == ENC_TYPE_LHDC) {
        lhdc_para_t *encoder = control->enc.lhdc;
        encoder->lastBitrate = bitrate;
        if (encoder->version >= 2)
            encoder->updateFramneInfo = true;
        v4f_LossyEncoderSetTargetByteRate(encoder->fft_blk, bitrate * 125);
    } else {
        llac_para_t *encoder = control->enc.llac;
        if (v4f_llac_enc_set_bitrate(bitrate * 1000, &encoder->out_nbytes, &encoder->real_bitrate,
                                     encoder->lh4_enc) != LLAC_OK)
            return LHDC_V4_STATUS_ERROR;
        encoder->lastBitrate = bitrate;
        encoder->updateFramneInfo = true;
    }
    return LHDC_V4_STATUS_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Convert a quality enum to its log label. */
static const char *rate_to_string(LHDCBT_QUALITY_T q) {
    switch (q) {
    case LHDCBT_QUALITY_LOW0:
        return "LHDCBT_QUALITY_LOW0";
    case LHDCBT_QUALITY_LOW1:
        return "LHDCBT_QUALITY_LOW1";
    case LHDCBT_QUALITY_LOW2:
        return "LHDCBT_QUALITY_LOW2";
    case LHDCBT_QUALITY_LOW3:
        return "LHDCBT_QUALITY_LOW3";
    case LHDCBT_QUALITY_LOW4:
        return "LHDCBT_QUALITY_LOW4";
    case LHDCBT_QUALITY_LOW:
        return "LHDCBT_QUALITY_LOW";
    case LHDCBT_QUALITY_MID:
        return "LHDCBT_QUALITY_MID";
    case LHDCBT_QUALITY_HIGH:
        return "LHDCBT_QUALITY_HIGH";
    case LHDCBT_QUALITY_AUTO:
        return "LHDCBT_QUALITY_AUTO";
    default:
        LHDC_LOGE("%s: Incorrect quality(%d)", __func__, q);
        return "UNKNOW_QUALITY";
    }
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Read ABR ladder bitrate subject to the maximum-bitrate limit. */
static int32_t bitrateFromIndex(lhdc_enc_type_t type, void *h, int32_t index) {

    int32_t limit = 0;
    int32_t result = 0;

    if (type == ENC_TYPE_LHDC) {
        lhdc_para_t *lhdc = (lhdc_para_t *)h;
        limit = lhdc_util_get_bitrate((uint32_t)lhdc->limitBitRateStatus);
        result = auto_bitrate_adjust_table_lhdc[index];
    } else if (type == ENC_TYPE_LLAC) {
        llac_para_t *llac = (llac_para_t *)h;
        limit = lhdc_util_get_bitrate((uint32_t)llac->limitBitRateStatus);
        result = auto_bitrate_adjust_table_llac[index];
    }

    return result >= limit ? limit : result;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Compute ABR ladder index from queue length. */
static int32_t bitrateIndexFrom(lhdc_enc_type_t type, size_t queueLength) {

    uint32_t element_size =
        (type == ENC_TYPE_LHDC) ? LHDC_BITRATE_ELEMENTS_SIZE : LLAC_BITRATE_ELEMENTS_SIZE;

    int32_t newBitrateInx = 0;
    if (queueLength < QUEUE_LENGTH_THRESHOLD) {
        float queuePercenty =
            (1 - ((float)queueLength / QUEUE_LENGTH_THRESHOLD)) * (element_size - 1);
        newBitrateInx = (int32_t)queuePercenty;
    }
    return newBitrateInx;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Match the current bitrate to the ABR ladder. */
static int32_t indexOfBitrate(lhdc_enc_type_t type, void *h, int32_t bitrate) {
    uint32_t element_size =
        type == ENC_TYPE_LHDC ? LHDC_BITRATE_ELEMENTS_SIZE : LLAC_BITRATE_ELEMENTS_SIZE;
    for (size_t i = 0; i < element_size; i++) {
        if (bitrateFromIndex(type, h, i) >= bitrate) {
            return i;
        }
    }
    return LHDC_V4_ABR_FALLBACK_INDEX;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Change the LHDC encoder's maximum bitrate. */
static void lhdc_encoder_set_max_bitrate(lhdc_para_t *handle, int32_t max_rate_index) {
    if (handle == NULL || max_rate_index == LHDCBT_QUALITY_AUTO) {
        LHDC_LOGE("%s: Error LHDC instance(%p), max rate(%d)", __func__, handle, max_rate_index);
        return;
    }
    if (max_rate_index != (int32_t)handle->limitBitRateStatus) {

        handle->limitBitRateStatus = (LHDCBT_QUALITY_T)max_rate_index;

        if ((uint32_t)handle->limitBitRateStatus != (uint32_t)handle->qualityStatus) {
            if (handle->qualityStatus != LHDCBT_QUALITY_AUTO) {
                handle->qualityStatus = handle->limitBitRateStatus;
            }

            int32_t newRate =
                TARGET_BITRATE_LIMIT(lhdc_util_get_bitrate(handle->limitBitRateStatus),
                                     handle->hasMinBitrateLimit ? 320 : 128);

            if (handle->lastBitrate >= newRate) {
                handle->lastBitrate = newRate;

                if (handle->version >= 2) {
                    handle->updateFramneInfo = true;
                }
                LossyEncoderSetTargetByteRate(handle->fft_blk, (handle->lastBitrate * 1000) / 8);
                LHDC_LOGD("%s: Update Max target bitrate(%s)", __func__,
                          rate_to_string(handle->limitBitRateStatus));
            }
        }
    }
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Encode one LHDC PCM frame through the single-frame interface. */
static int32_t lhdc_encoder_encode(lhdc_para_t *handle, void *p_pcm, uint8_t *p_stream) {
    if (handle) {
        if (p_pcm == NULL || p_stream == NULL) {
            LHDC_LOGE("%s: Buffer error! source(%p), output(%p)", __func__, p_pcm, p_stream);
            return LHDC_V4_NO_OUTPUT_BYTES;
        }
        int32_t bytesSizePerBlock = 0;
        int32_t encodedSize = -1;
        uint32_t block_size = handle->block_size;
        bytesSizePerBlock = (block_size * (handle->bits_per_sample >> 3)) << 1;
        encodedSize = LossyEncoderProcessWav(handle->fft_blk, (uint8_t *)p_pcm, block_size, 0,
                                             p_stream, bytesSizePerBlock);
        return encodedSize;
    }
    LHDC_LOGE("%s: Handle error!(%p)", __func__, handle);
    return LHDC_V4_NO_OUTPUT_BYTES;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Change LHDC fixed quality or AUTO state. */
static int32_t lhdc_encoder_set_bitrate(lhdc_para_t *handle, int32_t bitrate_inx) {
    if (handle) {

        if (bitrate_inx != (int32_t)handle->qualityStatus) {

            if (bitrate_inx != LHDCBT_QUALITY_AUTO) {
                handle->lastBitrate = TARGET_BITRATE_LIMIT(lhdc_util_get_bitrate(bitrate_inx),
                                                           handle->hasMinBitrateLimit ? 320 : 128);
            } else {
                handle->lastBitrate = LHDC_ABR_DEFAULT_BITRATE;
                lhdc_util_reset_down_bitrate(ENC_TYPE_LHDC, handle);
                lhdc_util_reset_up_bitrate(ENC_TYPE_LHDC, handle);
            }

            handle->qualityStatus = bitrate_inx;

            if ((uint32_t)handle->qualityStatus > (uint32_t)handle->limitBitRateStatus &&
                handle->qualityStatus != LHDCBT_QUALITY_AUTO) {
                handle->lastBitrate =
                    TARGET_BITRATE_LIMIT(lhdc_util_get_bitrate(handle->limitBitRateStatus),
                                         handle->hasMinBitrateLimit ? 320 : 128);
                handle->qualityStatus = handle->limitBitRateStatus;
            }
        }
        if (handle->version >= 2) {
            handle->updateFramneInfo = true;
        }
        LossyEncoderSetTargetByteRate(handle->fft_blk, (handle->lastBitrate * 1000) / 8);
        LHDC_LOGD("%s: Update target bitrate(%s)", __func__, rate_to_string(handle->qualityStatus));
        return LHDC_V4_STATUS_OK;
    }
    LHDC_LOGE("%s: Handle error!(%p)", __func__, handle);
    return LHDC_V4_STATUS_ERROR;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Apply the reference LHDC queue-control policy. */
static int32_t lhdc_encoder_adjust_bitrate(lhdc_para_t *handle, size_t queueLen,
                                           uint32_t *abr_index) {
    if (handle != NULL && handle->qualityStatus == LHDCBT_QUALITY_AUTO) {
        if (handle->dnBitrateCnt >= DOWN_RATE_TIME_CNT) {
            /* code */
            size_t queueLength = handle->dnBitrateSum / handle->dnBitrateCnt;

            handle->dnBitrateSum = 0;
            handle->dnBitrateCnt = 0;
            uint32_t newBitrateInx = bitrateIndexFrom(ENC_TYPE_LHDC, queueLength);

            if (TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                     handle->hasMinBitrateLimit ? 320 : 128) <=
                    handle->lastBitrate &&
                (newBitrateInx < (*abr_index))) {
                handle->lastBitrate =
                    TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                         handle->hasMinBitrateLimit ? 320 : 128);
                if (handle->version >= 2) {
                    handle->updateFramneInfo = true;
                }
                LossyEncoderSetTargetByteRate(handle->fft_blk, (handle->lastBitrate * 1000) / 8);
                LHDC_LOGD("%s:[Down bitrate] Update to bitrate[%u](%d), queue length(%zu)",
                          __func__, newBitrateInx, handle->lastBitrate, queueLength);
                lhdc_util_reset_up_bitrate(ENC_TYPE_LHDC, handle);

                (*abr_index) = newBitrateInx;
            } else {
                LHDC_LOGW(
                    "%s: Down bitrate condition fails, new rate:%d, current rate:%d", __func__,
                    TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                         handle->hasMinBitrateLimit ? 320 : 128),
                    handle->lastBitrate);
            }
        }

        if (handle->upBitrateCnt >= UP_RATE_TIME_CNT) {
            //clear down bitrate parameters...
            size_t queueLength = handle->upBitrateSum / handle->upBitrateCnt;
            uint32_t queuSumTmp = handle->upBitrateSum;

            handle->upBitrateSum = 0;
            handle->upBitrateCnt = 0;
            //int newBitrateInx = bitrateIndexFrom(ENC_TYPE_LHDC, queueLength);
            uint32_t newBitrateInx = indexOfBitrate(ENC_TYPE_LHDC, handle, handle->lastBitrate);
            if (newBitrateInx < (LHDC_BITRATE_ELEMENTS_SIZE - 1)) {
                newBitrateInx++;
            }

            if (TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                     handle->hasMinBitrateLimit ? 320 : 128) >=
                    handle->lastBitrate &&
                (newBitrateInx > (*abr_index)) && queuSumTmp == 0) {
                handle->lastBitrate =
                    TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                         handle->hasMinBitrateLimit ? 320 : 128);

                if (handle->version >= 2) {
                    handle->updateFramneInfo = true;
                }
                LossyEncoderSetTargetByteRate(handle->fft_blk, (handle->lastBitrate * 1000) / 8);
                LHDC_LOGD("%s:[Up bitrate] Update bitrate[%u](%d), queue length(%zu)", __func__,
                          newBitrateInx, handle->lastBitrate, queueLength);
                lhdc_util_reset_down_bitrate(ENC_TYPE_LHDC, handle);

                (*abr_index) = newBitrateInx;
            } else {
                LHDC_LOGW(
                    "%s: Up bitrate condition fails, new rate:%d, current rate:%d, sum of queue "
                    "len:%d",
                    __func__,
                    TARGET_BITRATE_LIMIT(bitrateFromIndex(ENC_TYPE_LHDC, handle, newBitrateInx),
                                         handle->hasMinBitrateLimit ? 320 : 128),
                    handle->lastBitrate, queuSumTmp);
            }
        }

        handle->upBitrateSum += queueLen;
        handle->dnBitrateSum += queueLen;

        handle->upBitrateCnt++;
        handle->dnBitrateCnt++;

        return LHDC_V4_STATUS_OK;
    }
    LHDC_LOGE("%s: Handle error!(%p)", __func__, handle);
    return LHDC_V4_STATUS_ERROR;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Apply the reference LLAC queue-control policy. */
static int32_t llac_encoder_adjust_bitrate(llac_para_t *handle, size_t queueLen,
                                           uint32_t *abr_index) {

    if (handle != NULL && handle->qualityStatus == LHDCBT_QUALITY_AUTO) {
        if (handle->dnBitrateCnt >= DOWN_RATE_TIME_CNT) {
            /* code */
            size_t queueLength = handle->dnBitrateSum / handle->dnBitrateCnt;

            handle->dnBitrateSum = 0;
            handle->dnBitrateCnt = 0;
            //int newBitrateInx = bitrateIndexFrom(ENC_TYPE_LLAC, queueLength);
            if (queueLength) {

                uint32_t newBitrateInx = 0;
                if (bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx) <= handle->lastBitrate &&
                    (newBitrateInx < (*abr_index))) {
                    handle->lastBitrate = bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx);

                    llac_enc_set_bitrate(handle->lastBitrate * 1000, &handle->out_nbytes,
                                         &handle->real_bitrate, handle->lh4_enc);
                    //handle->frame_per_packet = handle->host_mtu_size / handle->out_nbytes;
                    handle->updateFramneInfo = true;

                    LHDC_LOGD("%s:[Down bitrate] Update bitrate[%u](%d), queue length(%zu)",
                              __func__, newBitrateInx, handle->lastBitrate, queueLength);
                    lhdc_util_reset_up_bitrate(ENC_TYPE_LLAC, handle);
                    (*abr_index) = newBitrateInx;
                } else {
                    LHDC_LOGW("%s: Down bitrate condition fails, new rate:%d, current rate:%d",
                              __func__, bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx),
                              handle->lastBitrate);
                }
            }
        }

        if (handle->upBitrateCnt >= UP_RATE_TIME_CNT) {
            //clear down bitrate parameters...
            size_t queueLength = handle->upBitrateSum / handle->upBitrateCnt;
            uint32_t queuSumTmp = handle->upBitrateSum;

            handle->upBitrateSum = 0;
            handle->upBitrateCnt = 0;
            // get the last index in abr table
            uint32_t newBitrateInx = (*abr_index);

            if (newBitrateInx < (LLAC_BITRATE_ELEMENTS_SIZE - 1)) {
                newBitrateInx++;
            }

            if (bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx) >= handle->lastBitrate &&
                (newBitrateInx > (*abr_index)) && queuSumTmp == 0) {
                handle->lastBitrate = bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx);

                llac_enc_set_bitrate(handle->lastBitrate * 1000, &handle->out_nbytes,
                                     &handle->real_bitrate, handle->lh4_enc);
                //handle->frame_per_packet = handle->host_mtu_size / handle->out_nbytes;
                handle->updateFramneInfo = true;

                LHDC_LOGD("%s:[Up bitrate] Update bitrate[%u](%d), queue length(%zu)", __func__,
                          newBitrateInx, handle->lastBitrate, queueLength);
                lhdc_util_reset_down_bitrate(ENC_TYPE_LLAC, handle);
                (*abr_index) = newBitrateInx;
            } else {
                LHDC_LOGW(
                    "%s: Up bitrate condition fails, new rate:%d, current rate:%d, sum of queue "
                    "len:%d",
                    __func__, bitrateFromIndex(ENC_TYPE_LLAC, handle, newBitrateInx),
                    handle->lastBitrate, queuSumTmp);
            }
        }

        handle->upBitrateSum += queueLen;
        handle->dnBitrateSum += queueLen;

        handle->upBitrateCnt++;
        handle->dnBitrateCnt++;

        return LHDC_V4_STATUS_OK;
    }
    LHDC_LOGE("%s: Handle error!(%p)", __func__, handle);
    return LHDC_V4_STATUS_ERROR;
}
#endif

/* Release LHDC/LLAC core and AR instances. */
#if LHDC_BT_USE_AOSP_REFERENCE

void lhdcBT_free_handle(HANDLE_LHDC_BT handle) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return;
    }
    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        lhdc_encoder_free(lhdcBT->enc.lhdc);
        break;
    case ENC_TYPE_LLAC:
        llac_encoder_free(lhdcBT->enc.llac);
        break;
    default:
        break;
    }

    ar_process_free(lhdcBT->ar_filter);

    lhdc_memory_free(lhdcBT);
}
#else

/* Release encoding, AR, and instance-owned workspace; null handles are allowed. */
void lhdcBT_free_handle(HANDLE_LHDC_BT handle) {
    lhdc_cb_t *control = handle;
    if (!control)
        return;
    void *owned_workspace = control->owned_workspace;
    if (control->enc_type == ENC_TYPE_LLAC)
        v4f_llac_encoder_free(control->enc.llac);
    else if (control->enc_type == ENC_TYPE_LHDC)
        v4f_lhdc_encoder_free(control->enc.lhdc);
    v4f_ar_process_free(control->ar_filter);
    lhdc_memory_free(control);
    lhdc_memory_free(owned_workspace);
}
#endif

/* Create a V1/V2/V3 LHDC instance or version-four LLAC instance.
 * The default path validates versions before requesting complete instance storage once from the host.
 */
#if LHDC_BT_USE_AOSP_REFERENCE

HANDLE_LHDC_BT lhdcBT_get_handle(int32_t version) {
    LHDC_LOGD("%s: Version number %d", __func__, version);

    if (version <= 0) {
        return NULL;
    }

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)lhdc_memory_alloc(NULL, sizeof(lhdc_cb_t));
    memset(lhdcBT, 0, sizeof(lhdc_cb_t));

#ifdef AR_ALWAYS_ON
    lhdcBT->ar_filter = ar_process_new();
#else
    if (version >= 3) {
        lhdcBT->ar_filter = ar_process_new();
    }
#endif

    if (version <= 3) {
        lhdcBT->enc.lhdc = lhdc_encoder_new(version);
        lhdcBT->enc_type = ENC_TYPE_LHDC;
    } else if (version == 4) {
        lhdcBT->enc.llac = llac_encoder_new();
        lhdcBT->enc_type = ENC_TYPE_LLAC;
    } else {
        lhdcBT->enc_type = ENC_TYPE_UNKNOWN;
        lhdc_memory_free(lhdcBT);
        lhdcBT = NULL;
    }

    return lhdcBT;
}
#else

/* Create an encoder handle for the specified version using the default allocator. */
HANDLE_LHDC_BT lhdcBT_get_handle(int32_t version) {
    return lhdcBT_get_handle_with_allocator(version, NULL);
}
#endif

/* Validate BT configuration and initialize the core and optional AR module. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_init_encoder(HANDLE_LHDC_BT handle, int32_t sampling_freq, int32_t bitPerSample,
                            int32_t bitrate_inx, int32_t dualChannel, int32_t need_padding,
                            int32_t mtu, int32_t interval) {

    int32_t result = LHDC_V4_STATUS_OK;
    uint32_t samples_per_frame = 0;
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }

    if (sampling_freq != LHDC_V4_SAMPLE_RATE_44100 && sampling_freq != LHDC_V4_SAMPLE_RATE_48000 &&
        sampling_freq != LHDC_V4_SAMPLE_RATE_96000) {
        LHDC_LOGE("%s: Invalid Sample Rate (%d)!", __func__, sampling_freq);
        return LHDC_V4_STATUS_ERROR;
    }

    if (bitPerSample != LHDCBT_SMPL_FMT_S16 && bitPerSample != LHDCBT_SMPL_FMT_S24) {
        LHDC_LOGE("%s: Invalid Bit Per Sample (%d)!", __func__, bitPerSample);
        return LHDC_V4_STATUS_ERROR;
    }

    if (bitrate_inx < LHDCBT_QUALITY_LOW0 || bitrate_inx >= LHDCBT_QUALITY_MAX) {
        LHDC_LOGE("%s: invalid bit rate index (%d)!", __func__, bitrate_inx);
        return LHDC_V4_STATUS_ERROR;
    }

    if (dualChannel != 0 && dualChannel != 1) {
        LHDC_LOGE("%s: invalid Channel mode (%d)!", __func__, dualChannel);
        return LHDC_V4_STATUS_ERROR;
    }

    if (need_padding != 0) {
        LHDC_LOGE("%s: invalid need padding (%d)!", __func__, need_padding);
        return LHDC_V4_STATUS_ERROR;
    }

    if (mtu <= 0 || mtu >= 4096) {
        LHDC_LOGE("%s: invalid mtu (%d)!", __func__, mtu);
        return LHDC_V4_STATUS_ERROR;
    }

    if (interval <= 0 || interval > 20) //default: 10ms or 20ms
    {
        LHDC_LOGE("%s: invalid interval (%d)!", __func__, interval);
        return LHDC_V4_STATUS_ERROR;
    }

    enc_t *enc = &lhdcBT->enc;

    //reset ABR table index record
    lhdcBT->reference_abr_index = 0;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        result = lhdc_encoder_init(enc->lhdc, sampling_freq, bitPerSample, bitrate_inx, dualChannel,
                                   need_padding, mtu, interval);
        samples_per_frame = lhdc_encoder_get_frame_len(enc->lhdc);

        //depend on bitrate:400 position in auto_bitrate_adjust_table_lhdc
        lhdcBT->reference_abr_index = 3;
        break;
    case ENC_TYPE_LLAC:
        result =
            llac_encoder_init(enc->llac, sampling_freq, bitPerSample, bitrate_inx, mtu, interval);
        samples_per_frame = llac_encoder_get_frame_len(enc->llac);

        //depend on bitrate:400 position in auto_bitrate_adjust_table_lhdc
        lhdcBT->reference_abr_index = 5;
        break;
    default:
        break;
    }

    if (result >= 0 && samples_per_frame > 0 && lhdcBT->ar_filter != NULL) {
        // number of channels is fixed to "2"
        result =
            ar_process_init(lhdcBT->ar_filter, sampling_freq, bitPerSample, 2, samples_per_frame);
    }

    return result;
}
#else

/* Validate format/packet configuration and initialize the core and optional AR module. */
int32_t lhdcBT_init_encoder(HANDLE_LHDC_BT handle, int32_t sampling_freq, int32_t bitPerSample,
                            int32_t bitrate_inx, int32_t dualChannel, int32_t need_padding,
                            int32_t mtu, int32_t interval) {
    lhdc_cb_t *control = handle;
    if (!control ||
        (sampling_freq != LHDC_V4_SAMPLE_RATE_44100 && sampling_freq != LHDC_V4_SAMPLE_RATE_48000 &&
         sampling_freq != LHDC_V4_SAMPLE_RATE_96000) ||
        (bitPerSample != 16 && bitPerSample != 24) || (uint32_t)bitrate_inx >= 9 ||
        (uint32_t)dualChannel > 1 || need_padding || (uint32_t)(mtu - 1) >= 4095 ||
        (uint32_t)(interval - 1) >= 20)
        return LHDC_V4_STATUS_ERROR;
    int32_t result = 0, samples = 0;
    if (control->enc_type == ENC_TYPE_LLAC) {
        result = v4f_llac_encoder_init(control->enc.llac, sampling_freq, bitPerSample, bitrate_inx,
                                       mtu, interval);
        samples = v4f_llac_encoder_get_frame_len(control->enc.llac);
    } else if (control->enc_type == ENC_TYPE_LHDC) {
        result = v4f_lhdc_encoder_init(control->enc.lhdc, sampling_freq, bitPerSample, bitrate_inx,
                                       dualChannel, 0, mtu, interval);
        samples = v4f_lhdc_encoder_get_frame_len(control->enc.lhdc);
    }
    if (result >= 0 && samples && control->ar_filter)
        result = v4f_ar_process_init(control->ar_filter, sampling_freq, bitPerSample, 2, samples);
    return result;
}
#endif

/* Set maximum LHDC quality; supported only for the LHDC encoder. */
#if LHDC_BT_USE_AOSP_REFERENCE

void lhdcBT_set_max_bitrate(HANDLE_LHDC_BT handle, int32_t max_rate_index) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return;
    }
    if (max_rate_index < LHDCBT_QUALITY_LOW0 || max_rate_index >= LHDCBT_QUALITY_MAX) {
        LHDC_LOGE("%s: invalid bit rate index (%d)!", __func__, max_rate_index);
        return;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        return lhdc_encoder_set_max_bitrate(enc->lhdc, max_rate_index);
    case ENC_TYPE_LLAC: {

        //return llac_encoder_init(enc->llac, sampling_freq, bitPerSample, bitrate_inx, mtu, interval);
        LHDC_LOGD("%s: LLAC not supported", __func__);
    }
    default:
        break;
    }
}
#else

/* Validate bitrate index and update the encoder's upper bitrate limit. */
void lhdcBT_set_max_bitrate(HANDLE_LHDC_BT handle, int32_t max_rate_index) {
    lhdc_cb_t *control = handle;
    if (!control || (uint32_t)max_rate_index > 8 || control->enc_type != ENC_TYPE_LHDC)
        return;
    lhdc_para_t *encoder = control->enc.lhdc;
    if (!encoder || max_rate_index == LHDCBT_QUALITY_AUTO ||
        encoder->limitBitRateStatus == (LHDCBT_QUALITY_T)max_rate_index)
        return;
    encoder->limitBitRateStatus = max_rate_index;
    if (encoder->qualityStatus == (LHDCBT_QUALITY_T)max_rate_index)
        return;
    if (encoder->qualityStatus != LHDCBT_QUALITY_AUTO)
        encoder->qualityStatus = max_rate_index;
    int32_t bitrate = v4f_lhdc_util_get_bitrate(max_rate_index);
    int32_t minimum = encoder->hasMinBitrateLimit ? LIMITED_MIN_KBPS : MIN_KBPS;
    if (bitrate < minimum)
        bitrate = minimum;
    if (bitrate > encoder->lastBitrate)
        return;
    encoder->lastBitrate = bitrate;
    if (encoder->version >= 2)
        encoder->updateFramneInfo = true;
    v4f_LossyEncoderSetTargetByteRate(encoder->fft_blk, bitrate * 125);
}
#endif

/* Encode one LHDC PCM frame through the single-frame interface; LLAC returns unsupported. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_encode(HANDLE_LHDC_BT handle, void *p_pcm, uint8_t *p_stream) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (!p_pcm) {
        LHDC_LOGE("%s: p_pcm is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (!p_stream) {
        LHDC_LOGE("%s: p_stream is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        return lhdc_encoder_encode(enc->lhdc, p_pcm, p_stream);

    case ENC_TYPE_LLAC: {

        //return llac_encoder_init(enc->llac, sampling_freq, bitPerSample, bitrate_inx, mtu, interval);
        LHDC_LOGD("%s: LLAC not supported", __func__);
        break;
    }
    default:
        break;
    }
    return LHDC_V4_STATUS_ERROR;
}
#else

/* Encode one PCM block and return produced bytes. */
int32_t lhdcBT_encode(HANDLE_LHDC_BT handle, void *p_pcm, uint8_t *p_stream) {
    lhdc_cb_t *control = handle;
    if (!control || !p_pcm || !p_stream || control->enc_type != ENC_TYPE_LHDC)
        return LHDC_V4_STATUS_ERROR;
    lhdc_para_t *encoder = control->enc.lhdc;
    if (!encoder)
        return LHDC_V4_NO_OUTPUT_BYTES;
    return v4f_LossyEncoderProcessWav(encoder->fft_blk, p_pcm, encoder->block_size, 0, p_stream,
                                      encoder->block_size * (encoder->bits_per_sample >> 3) * 2);
}
#endif

/* Submit PCM to core framing/packetization and return output counts. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_encodeV3(HANDLE_LHDC_BT handle, void *p_pcm, uint8_t *out_put, uint32_t *written,
                        uint32_t *out_frames) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (p_pcm == NULL) {
        LHDC_LOGE("%s: input pcm buffer ptr is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (out_put == NULL) {
        LHDC_LOGE("%s: Encoded output buffer is NULL", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (written == NULL) {
        LHDC_LOGE("%s: written address is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (out_frames == NULL) {
        LHDC_LOGE("%s: out_frames address is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }

    return lhdc_util_encv4_process(handle, p_pcm, out_put, written, out_frames);
}
#else

/* Submit PCM to packetized encoding and return output bytes/frames. */
int32_t lhdcBT_encodeV3(HANDLE_LHDC_BT handle, void *p_pcm, uint8_t *out_put, uint32_t *written,
                        uint32_t *out_fraems) {
    if (!handle || !p_pcm || !out_put || !written || !out_fraems)
        return LHDC_V4_STATUS_ERROR;
    return v4f_lhdc_util_encv4_process(handle, p_pcm, out_put, written, out_fraems);
}
#endif

/* Read LHDC or LLAC samples per frame. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_get_block_Size(HANDLE_LHDC_BT handle) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        return lhdc_encoder_get_frame_len(enc->lhdc);

    case ENC_TYPE_LLAC: {
        return llac_encoder_get_frame_len(
            enc->llac); //llac_encoder_encode(enc->llac, p_pcm, out_put, written, out_fraems);
    }
    default:
        break;
    }
    return LHDC_V4_NO_SAMPLES;
}
#else

/* Query samples per frame for the LHDC or LLAC encoder. */
int32_t lhdcBT_get_block_Size(HANDLE_LHDC_BT handle) {
    lhdc_cb_t *control = handle;
    if (!control)
        return LHDC_V4_STATUS_ERROR;
    if (control->enc_type == ENC_TYPE_LLAC)
        return v4f_llac_encoder_get_frame_len(control->enc.llac);
    if (control->enc_type == ENC_TYPE_LHDC)
        return v4f_lhdc_encoder_get_frame_len(control->enc.lhdc);
    return LHDC_V4_NO_SAMPLES;
}
#endif

/* Read the LHDC or LLAC target bitrate. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_get_bitrate(HANDLE_LHDC_BT handle) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        return lhdc_encoder_get_target_bitrate(enc->lhdc);

    case ENC_TYPE_LLAC: {
        return llac_encoder_get_target_bitrate(
            enc->llac); //llac_encoder_encode(enc->llac, p_pcm, out_put, written, out_fraems);
    }
    default:
        break;
    }
    return LHDC_V4_STATUS_ERROR;
}
#else

/* Read the current encoder target bitrate. */
int32_t lhdcBT_get_bitrate(HANDLE_LHDC_BT handle) {
    lhdc_cb_t *control = handle;
    if (!control)
        return LHDC_V4_STATUS_ERROR;
    if (control->enc_type == ENC_TYPE_LLAC)
        return v4f_llac_encoder_get_target_bitrate(control->enc.llac);
    if (control->enc_type == ENC_TYPE_LHDC)
        return v4f_lhdc_encoder_get_target_bitrate(control->enc.lhdc);
    return LHDC_V4_STATUS_ERROR;
}
#endif

/* Change LHDC fixed/AUTO quality and notify the core to update byte budgets. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_set_bitrate(HANDLE_LHDC_BT handle, int32_t bitrate_inx) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }

    if (bitrate_inx < LHDCBT_QUALITY_LOW0 || bitrate_inx >= LHDCBT_QUALITY_MAX) {
        LHDC_LOGE("%s: invalid bit rate index (%d)!", __func__, bitrate_inx);
        return LHDC_V4_STATUS_ERROR;
    }

    enc_t *enc = &lhdcBT->enc;

    lhdcBT->reference_abr_index = 0;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC: {
        lhdc_para_t *lhdc = enc->lhdc;
        if (lhdc == NULL) {
            LHDC_LOGD("%s: LHDC [Reset bitrate] null ptr!", __func__);
            return LHDC_V4_STATUS_ERROR;
        }

        if (bitrate_inx == LHDCBT_QUALITY_RESET_AUTO) {
            if (lhdc->qualityStatus != LHDCBT_QUALITY_AUTO) {
                LHDC_LOGD("%s: LHDC [Reset bitrate] only work in ABR! (%d)", __func__,
                          lhdc->qualityStatus);
                return LHDC_V4_STATUS_ERROR;
            }
            // change bitrate only, do not update qualityStatus
            lhdc->lastBitrate = LHDC_ABR_DEFAULT_BITRATE;
            lhdc_util_reset_up_bitrate(ENC_TYPE_LHDC, lhdc);
            lhdc_util_reset_down_bitrate(ENC_TYPE_LHDC, lhdc);
            if (lhdc->version >= 2) {
                lhdc->updateFramneInfo = true;
            }
            LossyEncoderSetTargetByteRate(lhdc->fft_blk, (lhdc->lastBitrate * 1000) / 8);
            LHDC_LOGD("%s: LHDC [Reset bitrate] Reset bitrate to (%d)", __func__,
                      lhdc->lastBitrate);
            lhdcBT->reference_abr_index =
                3; //depend on bitrate:400 position in auto_bitrate_adjust_table_lhdc
            return LHDC_V4_STATUS_OK;
        } else {
            // normal case, will update qualityStatus
            if (bitrate_inx == LHDCBT_QUALITY_AUTO) {
                //depend on bitrate:400 position in auto_bitrate_adjust_table_lhdc
                lhdcBT->reference_abr_index = 3;
            }
            LHDC_LOGD("%s: LHDC set bitrate_inx %d", __func__, bitrate_inx);
            return lhdc_encoder_set_bitrate(lhdc, bitrate_inx);
        }
    }

    case ENC_TYPE_LLAC: {
        llac_para_t *llac = enc->llac;
        if (llac == NULL) {
            LHDC_LOGD("%s: LLAC [Reset bitrate] null ptr!", __func__);
            return LHDC_V4_STATUS_ERROR;
        }

        if (bitrate_inx == LHDCBT_QUALITY_RESET_AUTO) {
            if (llac->qualityStatus != LHDCBT_QUALITY_AUTO) {
                LHDC_LOGD("%s: LLAC [Reset bitrate] only work in ABR! (%d)", __func__,
                          llac->qualityStatus);
                return LHDC_V4_STATUS_ERROR;
            }
            // change bitrate only, do not update qualityStatus
            llac->lastBitrate = LLAC_ABR_DEFAULT_BITRATE;
            LHDC_LOGD("%s: LLAC [Reset bitrate] Reset bitrate to (%d)", __func__,
                      llac->lastBitrate);
            llac_enc_set_bitrate(llac->lastBitrate * 1000, &llac->out_nbytes, &llac->real_bitrate,
                                 llac->lh4_enc);
            llac->updateFramneInfo = true;
            lhdc_util_reset_up_bitrate(ENC_TYPE_LLAC, llac);
            lhdc_util_reset_down_bitrate(ENC_TYPE_LLAC, llac);
            lhdcBT->reference_abr_index =
                5; //depend on bitrate:400 position in auto_bitrate_adjust_table_llac
            return LHDC_V4_STATUS_OK;
        } else {
            //return llac_encoder_init(enc->llac, sampling_freq, bitPerSample, bitrate_inx, mtu, interval);
            LHDC_LOGD("%s: LLAC not supported", __func__);
            return LHDC_V4_STATUS_ERROR;
        }
    }
    default:
        break;
    }
    return LHDC_V4_STATUS_ERROR;
}
#else

/* Set a fixed bitrate level or automatic mode and synchronize the core budget. */
int32_t lhdcBT_set_bitrate(HANDLE_LHDC_BT handle, int32_t bitrate_inx) {
    lhdc_cb_t *control = handle;
    if (!control || (uint32_t)bitrate_inx > 8 || control->enc_type != ENC_TYPE_LHDC ||
        !control->enc.lhdc)
        return LHDC_V4_STATUS_ERROR;
    lhdc_para_t *encoder = control->enc.lhdc;
    if (encoder->qualityStatus != (LHDCBT_QUALITY_T)bitrate_inx) {
        encoder->qualityStatus = bitrate_inx;
        int32_t bitrate = ABR_INITIAL_KBPS;
        if (bitrate_inx == LHDCBT_QUALITY_AUTO) {
            v4f_lhdc_util_reset_up_bitrate(ENC_TYPE_LHDC, encoder);
            v4f_lhdc_util_reset_down_bitrate(ENC_TYPE_LHDC, encoder);
        } else {
            bitrate = v4f_lhdc_util_get_bitrate(bitrate_inx);
            int32_t minimum = encoder->hasMinBitrateLimit ? LIMITED_MIN_KBPS : MIN_KBPS;
            if (bitrate < minimum)
                bitrate = minimum;
        }
        int32_t maximum = v4f_lhdc_util_get_bitrate(encoder->limitBitRateStatus);
        encoder->lastBitrate = bitrate < maximum ? bitrate : maximum;
    }
    if (encoder->version >= 2)
        encoder->updateFramneInfo = true;
    v4f_LossyEncoderSetTargetByteRate(encoder->fft_blk, encoder->lastBitrate * 125);
    return LHDC_V4_STATUS_OK;
}
#endif

/* Adjust LHDC/LLAC AUTO bitrate from the previous queue-statistics window.
 * Access actual structure fields rather than alias-casting similar prefixes of different structures.
 */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_adjust_bitrate(HANDLE_LHDC_BT handle, size_t queueLen) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }

    if (queueLen < 0) {
        LHDC_LOGE("%s: Invalid queue Len (%zu)!", __func__, queueLen);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        return lhdc_encoder_adjust_bitrate(enc->lhdc, queueLen, &lhdcBT->reference_abr_index);

    case ENC_TYPE_LLAC: {
        return llac_encoder_adjust_bitrate(enc->llac, queueLen, &lhdcBT->reference_abr_index);
    }
    default:
        break;
    }
    return LHDC_V4_STATUS_ERROR;
}
#else

/* Adjust encoding bitrate from transmit-queue feedback and accumulated statistics. */
int32_t lhdcBT_adjust_bitrate(HANDLE_LHDC_BT handle, size_t queueLength) {
    lhdc_cb_t *control = handle;
    if (!control)
        return LHDC_V4_STATUS_ERROR;
    uint16_t *up_count, *down_count;
    uint32_t *up_sum, *down_sum;
    int32_t *last;
    void *encoder;
    int32_t minimum = 0;
    if (control->enc_type == ENC_TYPE_LHDC && control->enc.lhdc) {
        lhdc_para_t *p = control->enc.lhdc;
        if (p->qualityStatus != LHDCBT_QUALITY_AUTO)
            return LHDC_V4_STATUS_ERROR;
        encoder = p;
        up_count = &p->upBitrateCnt;
        down_count = &p->dnBitrateCnt;
        up_sum = &p->upBitrateSum;
        down_sum = &p->dnBitrateSum;
        last = &p->lastBitrate;
        minimum = p->hasMinBitrateLimit ? LIMITED_MIN_KBPS : MIN_KBPS;
    } else if (control->enc_type == ENC_TYPE_LLAC && control->enc.llac) {
        llac_para_t *p = control->enc.llac;
        if (p->qualityStatus != LHDCBT_QUALITY_AUTO)
            return LHDC_V4_STATUS_ERROR;
        encoder = p;
        up_count = &p->upBitrateCnt;
        down_count = &p->dnBitrateCnt;
        up_sum = &p->upBitrateSum;
        down_sum = &p->dnBitrateSum;
        last = &p->lastBitrate;
    } else
        return LHDC_V4_STATUS_ERROR;
    if (*down_count >= ABR_DOWN_TICKS) {
        uint32_t average = *down_sum / *down_count;
        *down_sum = 0;
        *down_count = 0;
        uint32_t stage = 0;
        if (control->enc_type == ENC_TYPE_LHDC && average < ABR_QUEUE_THRESHOLD)
            stage = (uint32_t)((1.0f - (float)average * 0.25f) * 5.0f);
        int32_t target = abr_bitrate(control, stage);
        if (target < minimum)
            target = minimum;
        if ((control->enc_type == ENC_TYPE_LHDC || average > 0) && target < *last) {
            if (abr_apply(control, target) != LHDC_V4_STATUS_OK)
                return LHDC_V4_STATUS_ERROR;
            v4f_lhdc_util_reset_up_bitrate(control->enc_type, encoder);
        }
    }
    if (*up_count >= ABR_UP_TICKS) {
        uint32_t sum = *up_sum;
        *up_sum = 0;
        *up_count = 0;
        uint32_t stage = 0;
        for (uint32_t i = 0; i < ABR_STAGES; i++) {
            if (*last <= abr_bitrate(control, i)) {
                stage = i;
                break;
            }
        }
        if (stage < ABR_STAGES - 1)
            stage++;
        int32_t target = abr_bitrate(control, stage);
        if (target < minimum)
            target = minimum;
        if (!sum && target > *last) {
            if (abr_apply(control, target) != LHDC_V4_STATUS_OK)
                return LHDC_V4_STATUS_ERROR;
            v4f_lhdc_util_reset_down_bitrate(control->enc_type, encoder);
        }
    }
    *up_sum += (uint32_t)queueLength;
    *down_sum += (uint32_t)queueLength;
    (*up_count)++;
    (*down_count)++;
    return LHDC_V4_STATUS_OK;
}
#endif

/* Switch LHDC AR/JAS or LLAC AR/LARC; other features are unsupported. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_set_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool enabled, void *priv /*nullable*/, int32_t priv_data_len) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (field < LHDCBT_EXT_FUNC_AR || field >= LHDCBT_EXT_FUNC_MAX) {
        LHDC_LOGE("%s: invalid field (%d) !", __func__, field);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    //void LhdcExtFuncArEnable(FFT_BLOCK *fb, int enable_ar);
    //void LhdcExtFuncJasEnable(FFT_BLOCK *fb, int enable_Jas);
    //void LhdcExtFuncMetaEnable(FFT_BLOCK *fb, int enable_meta, unsigned char *pmeta_data, int meta_data_len, int conti_frame_cnt);
    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC: {
        switch ((lhdc_ext_func_t)field) {
        case EXT_FUNC_AR:
            LhdcExtFuncArEnable(enc->lhdc->fft_blk, enabled ? 1 : 0);
            LHDC_LOGD("%s: lhdc AR func = %d", __func__, enabled);
            return LHDC_V4_STATUS_OK;
            break;

        case EXT_FUNC_JAS:
            LhdcExtFuncJasEnable(enc->lhdc->fft_blk, enabled ? 1 : 0);
            LHDC_LOGD("%s: lhdc JAS func = %d", __func__, enabled);
            return LHDC_V4_STATUS_OK;
            break;

        case EXT_FUNC_META:
            if (!priv || !priv_data_len) {
                return LHDC_V4_STATUS_ERROR;
            }
            LhdcExtFuncMetaEnable(enc->lhdc->fft_blk, enabled ? 1 : 0, priv, priv_data_len, 8);
            LHDC_LOGD("%s: lhdc Meta func = %d", __func__, enabled);
            return LHDC_V4_STATUS_OK;
            break;

        default:
            break;
        }
    } break;

    case ENC_TYPE_LLAC: {
        int8_t f_inx = -1;
        uint8_t f_enbaled = enabled ? 1 : 0;
        switch ((lhdc_ext_func_t)field) {
        case EXT_FUNC_AR:
            f_inx = EXTRA_FUNC_AR;
            break;

        case EXT_FUNC_LARC:
            f_inx = EXTRA_FUNC_LARC;
            break;

        default:
            break;
        }
        if (f_inx >= 0) {
            LHDC_LOGD("%s: flag_index:%d, enabled:%d, enc->llac->lh4_enc(%p)", __func__, f_inx,
                      f_enbaled, enc->llac->lh4_enc);
            llac_enc_set_extra_func(f_inx, f_enbaled, enc->llac->lh4_enc);
            return LHDC_V4_STATUS_OK;
        }
    } break;

    default:
        break;
    }

    return LHDC_V4_STATUS_ERROR;
}
#else

/* Set extension flags and synchronize encoder/optional AR state. */
int32_t lhdcBT_set_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool enabled, void *priv, int32_t priv_data_len) {
    (void)priv;
    (void)priv_data_len;
    lhdc_cb_t *control = handle;
    if (!control || (uint32_t)field >= LHDCBT_EXT_FUNC_MAX)
        return LHDC_V4_STATUS_ERROR;
#if defined(LHDC_ENCODER_WITH_AR) && !LHDC_ENCODER_WITH_AR
    if (field == LHDCBT_EXT_FUNC_AR && enabled)
        return LHDC_V4_STATUS_ERROR;
#endif
    if (control->enc_type == ENC_TYPE_LLAC) {
        if (field != LHDCBT_EXT_FUNC_AR && field != LHDCBT_EXT_FUNC_LARC)
            return LHDC_V4_STATUS_ERROR;
        v4f_llac_enc_set_extra_func(field, enabled & 1, control->enc.llac->lh4_enc);

        return LHDC_V4_STATUS_OK;
    }
    if (control->enc_type != ENC_TYPE_LHDC)
        return LHDC_V4_STATUS_ERROR;
    if (field == LHDCBT_EXT_FUNC_AR)
        v4f_LhdcExtFuncArEnable(control->enc.lhdc->fft_blk, enabled & 1);
    else if (field == LHDCBT_EXT_FUNC_JAS)
        v4f_LhdcExtFuncJasEnable(control->enc.lhdc->fft_blk, enabled & 1);
    else
        return LHDC_V4_STATUS_ERROR;
    return LHDC_V4_STATUS_OK;
}
#endif

/* Read core extension flags and normalize to a one-byte boolean. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_get_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool *enabled) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    if (!enabled) {
        LHDC_LOGE("%s: enabled is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;
    *enabled = lhdc_util_get_ext_func_state(lhdcBT->enc_type, enc, (lhdc_ext_func_t)field);

    return LHDC_V4_STATUS_OK;
}
#else

/* Validate an extension identifier and query its enable state. */
int32_t lhdcBT_get_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool *enabled) {
    lhdc_cb_t *control = handle;
    if (!control || !enabled)
        return LHDC_V4_STATUS_ERROR;
    *enabled =
        v4f_lhdc_util_get_ext_func_state(control->enc_type, &control->enc, (lhdc_ext_func_t)field) &
        1;
    return LHDC_V4_STATUS_OK;
}
#endif

/* Set LHDC's 320 kbps minimum switch; LLAC succeeds without changing state. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_set_hasMinBitrateLimit(HANDLE_LHDC_BT handle, bool enabled) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGE("%s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        LHDC_LOGD("%s:set value (%d)", __func__, enabled);
        enc->lhdc->hasMinBitrateLimit = enabled;
        break;

    case ENC_TYPE_LLAC: {
        LHDC_LOGD("%s: ENC_TYPE_LLAC unsupported!", __func__);
        break;
    }
    default:
        break;
    }

    return LHDC_V4_STATUS_OK;
}
#else

/* Set the encoder minimum-bitrate switch. */
int32_t lhdcBT_set_hasMinBitrateLimit(HANDLE_LHDC_BT handle, bool enabled) {
    lhdc_cb_t *control = handle;
    if (!control)
        return LHDC_V4_STATUS_ERROR;
    if (control->enc_type == ENC_TYPE_LHDC)
        control->enc.lhdc->hasMinBitrateLimit = enabled & 1;
    return LHDC_V4_STATUS_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Parse META configuration payload and configure core features. */
static int32_t lhdcBT_set_cfg_meta_v1(HANDLE_LHDC_BT handle, const char *userConfig,
                                      const int32_t configLen) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    PST_LHDC_SET_META pset_meta = (PST_LHDC_SET_META)userConfig;

    uint8_t *pmeta_metadata = NULL;

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (configLen < (int32_t)sizeof(ST_LHDC_SET_META)) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small(req:%d)", __func__,
                  (int32_t)sizeof(ST_LHDC_SET_META));
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    if (pset_meta->meta_ver != META_ST_VER_V2) {
        LHDC_LOGE("(LHDC-exAPI) %s: Version mismatch", __func__);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (pset_meta->meta_mem_size != (int32_t)sizeof(ST_LHDC_SET_META)) {
        LHDC_LOGE("(LHDC-exAPI) %s: metadata size not match", __func__);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (configLen <
        ((int32_t)pset_meta->meta_metadata_length) + ((int32_t)sizeof(ST_LHDC_SET_META))) {
        LHDC_LOGE("(LHDC-exAPI) %s: cfg size too small", __func__);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC:
        LHDC_LOGD("(LHDC-exAPI) %s: ENC_TYPE_LHDC", __func__);
        pmeta_metadata = (uint8_t *)(pset_meta + 1);

        LhdcExtFuncMetaEnable(enc->lhdc->fft_blk, pset_meta->meta_enable, pmeta_metadata,
                              pset_meta->meta_metadata_length, pset_meta->meta_set);
        break;

    case ENC_TYPE_LLAC:
        LHDC_LOGD("(LHDC-exAPI) %s: ENC_TYPE_LLAC", __func__);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        break;

    default:
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        break;
    }

    return EXTEND_FUNC_RET_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Query and populate META status payload. */
static int32_t lhdcBT_get_cfg_meta_v1(HANDLE_LHDC_BT handle, char *userConfig,
                                      const int32_t configLen) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    PST_LHDC_GET_META pget_meta = (PST_LHDC_GET_META)userConfig;
    bool jas_enabled = false;

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (configLen < (int32_t)sizeof(ST_LHDC_GET_META)) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small(req:%d)", __func__,
                  (int32_t)sizeof(ST_LHDC_GET_META));
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC: {
        LHDC_LOGD("(LHDC-exAPI) %s: ENC_TYPE_LHDC", __func__);
        pget_meta->meta_ver = META_ST_VER_V2;
        pget_meta->meta_mem_size = (int32_t)sizeof(ST_LHDC_GET_META);
        pget_meta->meta_st = (LhdcGetExtFuncState(enc->lhdc->fft_blk, EXT_FUNC_META) << 1) |
                             0x01; // Get current frame include metadata or not

        //2021/06/10: Append JAS status her ,let UI can get JAS status from LHDC,
        jas_enabled = lhdc_util_get_ext_func_state(lhdcBT->enc_type, enc, EXT_FUNC_JAS);
        if (jas_enabled) {
            pget_meta->jas_status = 1;
        } else {
            pget_meta->jas_status = 0;
        }
    } break;

    case ENC_TYPE_LLAC:
        LHDC_LOGD("(LHDC-exAPI) %s: ENC_TYPE_LLAC", __func__);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        break;

    default:
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        break;
    }

    return EXTEND_FUNC_RET_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Set the AR enable flag by encoder type. */
static int32_t Ar_set_ext_func_state(HANDLE_LHDC_BT handle, bool enabled) {
    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;
    if (!lhdcBT) {
        LHDC_LOGD("(LHDC-exAPI) %s: Handle is NULL!", __func__);
        return LHDC_V4_STATUS_ERROR;
    }
    enc_t *enc = &lhdcBT->enc;

    switch (lhdcBT->enc_type) {
    case ENC_TYPE_LHDC: {
        LhdcExtFuncArEnable(enc->lhdc->fft_blk, enabled ? 1 : 0);
        LHDC_LOGD("(LHDC-exAPI) %s: lhdc AR func = %d", __func__, enabled);
    } break;
    case ENC_TYPE_LLAC: {
        int8_t f_inx = EXTRA_FUNC_AR;
        uint8_t f_enbaled = enabled ? 1 : 0;
        LHDC_LOGD("(LHDC-exAPI) %s: enabled:%d, enc->llac->lh4_enc(%p)", __func__, f_enbaled,
                  enc->llac->lh4_enc);
        llac_enc_set_extra_func(f_inx, f_enbaled, enc->llac->lh4_enc);
    } break;
    default:
        break;
    }
    return LHDC_V4_STATUS_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Parse three-axis orientation and submit it to AR. */
static int32_t lhdcBT_set_data_gyro_2d_v1(HANDLE_LHDC_BT handle, const char *userData,
                                          const int32_t dataLen) {

    PST_LHDC_BT_AR_GYRO pargyro = (PST_LHDC_BT_AR_GYRO)userData;

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;

    enc_t *enc = &lhdcBT->enc;

    bool ar_enabled = false;

    int32_t res;

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userData == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Data error!(%p)", __func__, userData);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (dataLen < (int32_t)sizeof(ST_LHDC_BT_AR_GYRO)) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    LHDC_LOGD("(LHDC-exAPI) %s: gyro[%d %d %d]", __func__, pargyro->world_coordinate_x,
              pargyro->world_coordinate_y, pargyro->world_coordinate_z);

    ar_enabled = lhdc_util_get_ext_func_state(lhdcBT->enc_type, enc, EXT_FUNC_AR);

#ifdef AR_ALWAYS_ON
    ar_enabled = true;
#endif

    if (ar_enabled) {
        res = ar_set_gyro_pos(lhdcBT->ar_filter, pargyro->world_coordinate_x,
                              pargyro->world_coordinate_y, pargyro->world_coordinate_z);
    } else {
        LHDC_LOGD("(LHDC-exAPI) %s: AR not enabled!", __func__);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    }

    if (res != 0) {
        LHDC_LOGD("(LHDC-exAPI) %s: set_gyro_pos error(%d)", __func__, res);
        return EXTEND_FUNC_RET_ERROR;
    }

    return EXTEND_FUNC_RET_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Parse AR configuration payload and apply channel positions/gains. */
static int32_t lhdcBT_set_cfg_ar_v3(HANDLE_LHDC_BT handle, const char *userConfig,
                                    const int32_t configLen) {

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;

    enc_t *enc = &lhdcBT->enc;

    PST_LHDC_BT_AR pset_ar_cfg = (PST_LHDC_BT_AR)userConfig;

    bool ar_enabled = false;

    int32_t res;

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (configLen < (int32_t)sizeof(ST_LHDC_BT_AR)) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }
    LHDC_LOGD("(LHDC-exAPI) %s: config AR %d", __func__, pset_ar_cfg->app_ar_enabled);

    ar_enabled = pset_ar_cfg->app_ar_enabled ? 1 : 0;

    Ar_set_ext_func_state(handle, ar_enabled);
    //LhdcExtFuncArEnable(enc->lhdc->fft_blk, pset_ar_cfg->app_ar_enabled ? 1 : 0);

    ar_enabled = lhdc_util_get_ext_func_state(lhdcBT->enc_type, enc, EXT_FUNC_AR);

    LHDC_LOGD("(LHDC-exAPI) %s: AR enabled %d", __func__, ar_enabled);

#ifdef AR_ALWAYS_ON
    ar_enabled = true;
#endif

    if (ar_enabled) {
        res = ar_set_cfg(lhdcBT->ar_filter, &pset_ar_cfg->Ch1_Pos, &pset_ar_cfg->Ch1_L_PreGain,
                         pset_ar_cfg->app_ar_enabled);
    } else {
        LHDC_LOGD("(LHDC-exAPI) %s: AR not enabled!", __func__);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    }

    if (res != 0) {
        return EXTEND_FUNC_RET_ERROR;
    }

    return EXTEND_FUNC_RET_OK;
}
#endif

#if LHDC_BT_USE_AOSP_REFERENCE
/* Validate and query AR configuration. */
static int32_t lhdcBT_get_cfg_ar_v1(HANDLE_LHDC_BT handle, char *userConfig,
                                    const int32_t configLen) {

    //PST_LHDC_BT_AR pset_ar_cfg = (PST_LHDC_BT_AR)userConfig;

    lhdc_cb_t *lhdcBT = (lhdc_cb_t *)handle;

    enc_t *enc = &lhdcBT->enc;

    bool ar_enabled = false;

    int32_t res = 0;

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (configLen <= (int32_t)0) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    ar_enabled = lhdc_util_get_ext_func_state(lhdcBT->enc_type, enc, EXT_FUNC_AR);

#ifdef AR_ALWAYS_ON
    ar_enabled = true;
#endif

    if (ar_enabled) {
        LHDC_LOGD("(LHDC-exAPI) %s: AR enabled", __func__);
        //2021/11/09: ar_get_cfg not supported yet
        //res = ar_get_cfg(lhdcBT->ar_filter, &pset_ar_cfg->Ch1_Pos, &pset_ar_cfg->Ch1_L_PreGain);
        return EXTEND_FUNC_RET_ERROR;
    } else {
        LHDC_LOGD("(LHDC-exAPI) %s: AR not enabled!", __func__);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    }

    if (res != 0) {
        return EXTEND_FUNC_RET_ERROR;
    }

    LHDC_LOGD("(LHDC-exAPI) %s: End OK", __func__);
    return EXTEND_FUNC_RET_OK;
}
#endif

/* Parse META/AR settings; AR switches the core flag before deciding whether to apply parameters. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_set_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen) {

    uint8_t *pucConfig = (uint8_t *)userConfig;
    uint32_t exFuncVer;
    uint32_t exFuncCode;
    int32_t result = EXTEND_FUNC_RET_OK;

    //LHDC_LOGD("(LHDC-exAPI) %s: enter\n",  __func__);

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (clen < (int32_t)EXTEND_FUNC_CODE_MIN_BUFFER_LEN) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    exFuncVer = (((uint32_t)pucConfig[3]) & ((uint32_t)0xff)) |
                ((((uint32_t)pucConfig[2]) & ((uint32_t)0xff)) << 8) |
                ((((uint32_t)pucConfig[1]) & ((uint32_t)0xff)) << 16) |
                ((((uint32_t)pucConfig[0]) & ((uint32_t)0xff)) << 24);
    exFuncCode = (((uint32_t)pucConfig[7]) & ((uint32_t)0xff)) |
                 ((((uint32_t)pucConfig[6]) & ((uint32_t)0xff)) << 8) |
                 ((((uint32_t)pucConfig[5]) & ((uint32_t)0xff)) << 16) |
                 ((((uint32_t)pucConfig[4]) & ((uint32_t)0xff)) << 24);

    switch (exFuncCode) {
    case EXTEND_FUNC_CODE_SET_CONFIG_META:

        switch (exFuncVer) {
        case EXTEND_FUNC_VER_SET_CONFIG_META_V1:
            LHDC_LOGD("(LHDC-exAPI) %s: SET_CONFIG_META\n", __func__);
            result = lhdcBT_set_cfg_meta_v1(handle, userConfig, clen);
            break;

        default:
            LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Version!(0x%X)", __func__, exFuncVer);
            return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
        }
        break;

    case EXTEND_FUNC_CODE_SET_CONFIG_AR:
        switch (exFuncVer) {
        case EXTEND_FUNC_VER_SET_CONFIG_AR_V3:
            LHDC_LOGD("(LHDC-exAPI) %s: SET_CONFIG_AR\n", __func__);
            result = lhdcBT_set_cfg_ar_v3(handle, userConfig, clen);
            break;

        default:
            LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Version!(0x%X)", __func__, exFuncVer);
            return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
        }
        break;

    default:
        LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Code!(0x%X)", __func__, exFuncCode);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    } // switch (exFuncCode)

    return result;
}
#else

/* Parse extension configuration, validate version/length, and apply the corresponding settings. */
int32_t lhdcBT_set_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen) {
    lhdc_cb_t *control = handle;
    if (!control)
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    if (!userConfig)
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    if (clen < 8)
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    uint8_t *packet = (uint8_t *)userConfig;
    ST_LHDC_BT_AR *ar_config = (void *)packet;
    ST_LHDC_SET_META *meta_config = (void *)packet;
    uint32_t code = audio_math_read_be32(packet + 4), version = audio_math_read_be32(packet);
    if (code == EXTEND_FUNC_CODE_SET_CONFIG_AR) {
        if (version != EXTEND_FUNC_VER_SET_CONFIG_AR_V3)
            return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
        if (clen < AR_PACKET_BYTES)
            return EXTEND_FUNC_RET_BUF_UNDERRUN;
        uint32_t enabled = audio_math_read_native32(&ar_config->app_ar_enabled);
        lhdcBT_set_ext_func_state(handle, LHDCBT_EXT_FUNC_AR, enabled != 0, NULL, 0);
        if (!(v4f_lhdc_util_get_ext_func_state(control->enc_type, &control->enc, EXT_FUNC_AR) & 1))
            return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        return v4f_ar_set_cfg(control->ar_filter, &ar_config->Ch1_Pos, &ar_config->Ch1_L_PreGain,
                              enabled)
                   ? EXTEND_FUNC_RET_ERROR
                   : EXTEND_FUNC_RET_OK;
    }
    if (code != EXTEND_FUNC_CODE_SET_CONFIG_META)
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    if (version != EXTEND_FUNC_VER_SET_CONFIG_META_V1)
        return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
    if (clen < META_SET_BYTES)
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    if (audio_math_read_native32(&meta_config->meta_ver) != META_ST_VER_V2 ||
        meta_config->meta_mem_size != META_SET_BYTES ||
        clen < META_SET_BYTES + meta_config->meta_metadata_length)
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    if (control->enc_type != ENC_TYPE_LHDC)
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    v4f_LhdcExtFuncMetaEnable(control->enc.lhdc->fft_blk, meta_config->meta_enable,
                              (uint8_t *)(meta_config + 1), meta_config->meta_metadata_length,
                              meta_config->meta_set);
    return EXTEND_FUNC_RET_OK;
}
#endif

/* Read META status; AR queries check enable state without retrieving configuration.
 * Enabled AR returns E06; disabled AR returns E03.
 */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_get_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen) {

    uint8_t *pucConfig = (uint8_t *)userConfig;
    uint32_t exFuncVer;
    uint32_t exFuncCode;
    int32_t result = EXTEND_FUNC_RET_OK;

    //LHDC_LOGD("%s:(LHDC-exAPI) enter\n",  __func__);

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userConfig == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Config error!(%p)", __func__, userConfig);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (clen < (int32_t)EXTEND_FUNC_CODE_MIN_BUFFER_LEN) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    exFuncVer = (((uint32_t)pucConfig[3]) & ((uint32_t)0xff)) |
                ((((uint32_t)pucConfig[2]) & ((uint32_t)0xff)) << 8) |
                ((((uint32_t)pucConfig[1]) & ((uint32_t)0xff)) << 16) |
                ((((uint32_t)pucConfig[0]) & ((uint32_t)0xff)) << 24);
    exFuncCode = (((uint32_t)pucConfig[7]) & ((uint32_t)0xff)) |
                 ((((uint32_t)pucConfig[6]) & ((uint32_t)0xff)) << 8) |
                 ((((uint32_t)pucConfig[5]) & ((uint32_t)0xff)) << 16) |
                 ((((uint32_t)pucConfig[4]) & ((uint32_t)0xff)) << 24);

    switch (exFuncCode) {
    case EXTEND_FUNC_CODE_GET_CONFIG_META:

        switch (exFuncVer) {
        case EXTEND_FUNC_VER_GET_CONFIG_META_V1:
            LHDC_LOGD("(LHDC-exAPI) %s: GET_CONFIG_META\n", __func__);
            result = lhdcBT_get_cfg_meta_v1(handle, userConfig, clen);
            break;

        default:
            LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Version!(0x%X)", __func__, exFuncVer);
            return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
        }
        break;

    case EXTEND_FUNC_CODE_GET_CONFIG_AR:

        switch (exFuncVer) {
        case EXTEND_FUNC_VER_GET_CONFIG_AR_V1:
            LHDC_LOGD("(LHDC-exAPI) %s: GET_CONFIG_AR\n", __func__);
            result = lhdcBT_get_cfg_ar_v1(handle, userConfig, clen);
            break;

        default:
            LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Version!(0x%X)", __func__, exFuncVer);
            return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
        }
        break;

    default:
        LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Code!(0x%X)", __func__, exFuncCode);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    } // switch (exFuncCode)

    return result;
}
#else

/* Parse extension queries and write configuration status into caller storage. */
int32_t lhdcBT_get_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen) {
    lhdc_cb_t *control = handle;
    if (!control)
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    if (!userConfig)
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    if (clen < 8)
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    uint8_t *packet = (uint8_t *)userConfig;
    ST_LHDC_GET_META *meta_status = (void *)packet;
    uint32_t code = audio_math_read_be32(packet + 4), version = audio_math_read_be32(packet);
    if (code != EXTEND_FUNC_CODE_GET_CONFIG_META && code != EXTEND_FUNC_CODE_GET_CONFIG_AR)
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    if (version != EXTEND_FUNC_GENERIC_VERSION_NUMBER_V1)
        return EXTEND_FUNC_RET_VERSION_NOT_SUPPORT;
    if (code == EXTEND_FUNC_CODE_GET_CONFIG_AR)
        return v4f_lhdc_util_get_ext_func_state(control->enc_type, &control->enc, EXT_FUNC_AR) & 1
                   ? EXTEND_FUNC_RET_ERROR
                   : EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    if (clen < META_GET_BYTES)
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    if (control->enc_type != ENC_TYPE_LHDC)
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    uint32_t meta_version = META_ST_VER_V2;
    memcpy(&meta_status->meta_ver, &meta_version, sizeof(meta_version));
    meta_status->meta_mem_size = META_GET_BYTES;
    meta_status->meta_st =
        (v4f_LhdcGetExtFuncState(control->enc.lhdc->fft_blk, EXT_FUNC_META) & 1) ? 3 : 1;
    meta_status->jas_status =
        v4f_lhdc_util_get_ext_func_state(control->enc_type, &control->enc, EXT_FUNC_JAS) & 1;
    return EXTEND_FUNC_RET_OK;
}
#endif

/* Submit three-axis orientation only when AR is enabled; ignore invalid data. */
#if LHDC_BT_USE_AOSP_REFERENCE

void lhdcBT_set_user_exdata(HANDLE_LHDC_BT handle, char *userData, int32_t clen) {

    uint8_t *pucData = (uint8_t *)userData;
    uint32_t exFuncVer;
    uint32_t exFuncCode;
    int32_t result = EXTEND_FUNC_RET_OK;

    //LHDC_LOGD("(LHDC-exAPI) %s: enter\n",  __func__);

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return; // EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (userData == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: User Data error!(%p)", __func__, userData);
        return; // EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (clen < (int32_t)EXTEND_FUNC_CODE_MIN_BUFFER_LEN) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return;
    }

    exFuncVer = (((uint32_t)pucData[3]) & ((uint32_t)0xff)) |
                ((((uint32_t)pucData[2]) & ((uint32_t)0xff)) << 8) |
                ((((uint32_t)pucData[1]) & ((uint32_t)0xff)) << 16) |
                ((((uint32_t)pucData[0]) & ((uint32_t)0xff)) << 24);
    exFuncCode = (((uint32_t)pucData[7]) & ((uint32_t)0xff)) |
                 ((((uint32_t)pucData[6]) & ((uint32_t)0xff)) << 8) |
                 ((((uint32_t)pucData[5]) & ((uint32_t)0xff)) << 16) |
                 ((((uint32_t)pucData[4]) & ((uint32_t)0xff)) << 24);

    switch (exFuncCode) {
    case EXTEND_FUNC_CODE_SET_DATA_GYRO2D:

        switch (exFuncVer) {
        case EXTEND_FUNC_VER_SET_DATA_GYRO2D_V1:
            LHDC_LOGD("(LHDC-exAPI) %s: SET_DATA_GYRO\n", __func__);
            result = lhdcBT_set_data_gyro_2d_v1(handle, userData, clen);
            break;

        default:
            LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Version!(0x%X)", __func__, exFuncVer);
            return; // EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
        }
        break;

    default:
        LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Code!(0x%X)", __func__, exFuncCode);
        return; // EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    }           // switch (exFuncCode)

    //return result;
}
#else

/* Validate gyroscope packets and update AR orientation. */
void lhdcBT_set_user_exdata(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen) {
    lhdc_cb_t *control = handle;
    if (!control || !userConfig || clen < GYRO_PACKET_BYTES)
        return;
    uint8_t *packet = (uint8_t *)userConfig;
    ST_LHDC_BT_AR_GYRO *gyro = (void *)packet;
    if (audio_math_read_be32(packet + 4) != EXTEND_FUNC_CODE_SET_DATA_GYRO2D ||
        audio_math_read_be32(packet) != EXTEND_FUNC_VER_SET_DATA_GYRO2D_V1)
        return;
    if (v4f_lhdc_util_get_ext_func_state(control->enc_type, &control->enc, EXT_FUNC_AR) & 1)
        v4f_ar_set_gyro_pos(control->ar_filter,
                            (int32_t)audio_math_read_native32(&gyro->world_coordinate_x),
                            (int32_t)audio_math_read_native32(&gyro->world_coordinate_y),
                            (int32_t)audio_math_read_native32(&gyro->world_coordinate_z));
}
#endif

/* Return the extension API version; sufficiently large V4 query buffers also receive interface-specific lengths. */
#if LHDC_BT_USE_AOSP_REFERENCE

int32_t lhdcBT_get_user_exApiver(HANDLE_LHDC_BT handle, char *version, int32_t clen) {

    uint8_t *pucApiVer = (uint8_t *)version;
    uint32_t exFuncVer = 0;
    uint32_t exFuncCode = 0;
    uint32_t exFuncMinBufSize = 0;
    int32_t result = EXTEND_FUNC_RET_OK;

    //LHDC_LOGD("(LHDC-exAPI) %s: enter\n",  __func__);

    if (version == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: API version buffer error!(%p)", __func__, version);
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    }

    if (handle == NULL) {
        LHDC_LOGE("(LHDC-exAPI) %s: Handle error!(%p)", __func__, handle);
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    }

    if (clen < (int32_t)EXTEND_FUNC_MIN_BUFFER_LEN_GET_API_VERSION_V1) {
        // Buffer is to small
        LHDC_LOGE("(LHDC-exAPI) %s: Buffer too small", __func__);
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    }

    /*
    LHDC_LOGD("%s: clen=%d, APICode[4:7] = [0x%02X, 0x%02X, 0x%02X, 0x%02X]",  __func__,
        clen, pucApiVer[4], pucApiVer[5], pucApiVer[6], pucApiVer[7]);
        */

    exFuncCode = (((uint32_t)pucApiVer[7]) & ((uint32_t)0xff)) |
                 ((((uint32_t)pucApiVer[6]) & ((uint32_t)0xff)) << 8) |
                 ((((uint32_t)pucApiVer[5]) & ((uint32_t)0xff)) << 16) |
                 ((((uint32_t)pucApiVer[4]) & ((uint32_t)0xff)) << 24);

    switch (exFuncCode) {
    // Config API
    case EXTEND_FUNC_CODE_SET_CONFIG_META:
        exFuncVer = EXTEND_FUNC_VER_SET_CONFIG_META_V1;
        break;

    case EXTEND_FUNC_CODE_SET_CONFIG_AR:
        exFuncVer = EXTEND_FUNC_VER_SET_CONFIG_AR_V3;
        break;

    case EXTEND_FUNC_CODE_GET_CONFIG_META:
        exFuncVer = EXTEND_FUNC_VER_GET_CONFIG_META_V1;
        break;

    case EXTEND_FUNC_CODE_GET_CONFIG_AR:
        exFuncVer = EXTEND_FUNC_VER_GET_CONFIG_AR_V1;
        break;

    // Data API
    case EXTEND_FUNC_CODE_SET_DATA_GYRO2D:
        exFuncVer = EXTEND_FUNC_VER_SET_DATA_GYRO2D_V1;
        break;

    // A2DP codec Specific API
    case EXTEND_FUNC_CODE_GET_SPECIFIC:
        exFuncVer = EXTEND_FUNC_VER_GET_SPECIFIC_V2;
        exFuncMinBufSize = LHDC_EXTEND_FUNC_CONFIG_TOTAL_FIXED_SIZE_V2;
        break;

    default:
        LHDC_LOGE("(LHDC-exAPI) %s: Invalid Ex. Function Code!(0x%X)", __func__, exFuncCode);
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    } // switch (exFuncCode)

    /* fill in current version of target API */
    pucApiVer[3] = (uint8_t)(exFuncVer & ((uint32_t)0xff));
    pucApiVer[2] = (uint8_t)((exFuncVer >> 8) & ((uint32_t)0xff));
    pucApiVer[1] = (uint8_t)((exFuncVer >> 16) & ((uint32_t)0xff));
    pucApiVer[0] = (uint8_t)((exFuncVer >> 24) & ((uint32_t)0xff));

    /**** Version Control Field: *****/
    if (exFuncVer >= EXTEND_FUNC_GENERIC_VERSION_NUMBER_V2 &&
        clen >= EXTEND_FUNC_MIN_BUFFER_LEN_GET_API_VERSION_V2) {
        /* After API V2.0.0.0, append "minimum required buffer size" into API response */
        pucApiVer[11] = (uint8_t)(exFuncMinBufSize & ((uint32_t)0xff));
        pucApiVer[10] = (uint8_t)((exFuncMinBufSize >> 8) & ((uint32_t)0xff));
        pucApiVer[9] = (uint8_t)((exFuncMinBufSize >> 16) & ((uint32_t)0xff));
        pucApiVer[8] = (uint8_t)((exFuncMinBufSize >> 24) & ((uint32_t)0xff));
    }

    LHDC_LOGD("(LHDC-exAPI) %s: return Ver=0x[%02X %02X %02X %02X]", __func__, pucApiVer[4],
              pucApiVer[5], pucApiVer[6], pucApiVer[7]);

    return result;
}
#else

/* Validate extension feature codes and write supported protocol versions. */
int32_t lhdcBT_get_user_exApiver(HANDLE_LHDC_BT handle, char *version, int32_t clen) {
    if (!version)
        return EXTEND_FUNC_RET_INVALID_PARAMETER;
    if (!handle)
        return EXTEND_FUNC_RET_INVALID_HANDLE;
    if (clen < 8)
        return EXTEND_FUNC_RET_BUF_UNDERRUN;
    uint32_t api_version = EXTEND_FUNC_GENERIC_VERSION_NUMBER_V1, required = 0;
    int32_t writes_length = 0;
    switch (audio_math_read_be32(version + 4)) {
    case EXTEND_FUNC_CODE_GET_SPECIFIC:
        api_version = EXTEND_FUNC_VER_GET_SPECIFIC_V2;
        required = 64;
        writes_length = 1;
        break;
    case EXTEND_FUNC_CODE_SET_CONFIG_AR:
        api_version = EXTEND_FUNC_VER_SET_CONFIG_AR_V3;
        writes_length = 1;
        break;
    case EXTEND_FUNC_CODE_GET_CONFIG_META:
    case EXTEND_FUNC_CODE_GET_CONFIG_AR:
    case EXTEND_FUNC_CODE_SET_CONFIG_META:
    case EXTEND_FUNC_CODE_SET_DATA_GYRO2D:
        break;
    default:
        return EXTEND_FUNC_RET_FUNC_NOT_SUPPORT;
    }
    audio_math_write_be32(version, api_version);
    if (clen >= 16 && writes_length)
        audio_math_write_be32(version + 8, required);
    return EXTEND_FUNC_RET_OK;
}
#endif
