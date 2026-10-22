#include "lhdc_memory.h"
/* Encoded packets use a separate AR tag from the PCM effect's internal tag. */

#ifndef _LHDCBT_H_
#define _LHDCBT_H_
#ifdef __cplusplus
extern "C" {
#endif
#ifndef LHDCBT_API
#define LHDCBT_API
#endif /* LHDCBT_API  */
#include "lhdc_api.h"
#include "../common/lhdc_bt_ext.h"

typedef enum _lhdcBT_ext_func_field_t {
    LHDCBT_EXT_FUNC_AR = 0,
    LHDCBT_EXT_FUNC_LARC,
    LHDCBT_EXT_FUNC_EXTR,
    LHDCBT_EXT_FUNC_JAS,
    //LHDCBT_EXT_FUNC_META,
    LHDCBT_EXT_FUNC_MAX,
} lhdcBT_ext_func_field_t;

//
// Extend API Basic Settings Definition
//
#define LHDC_EXTEND_FUNC_CODE_A2DP_TYPE_MASK (0x0A)
#define LHDC_EXTEND_FUNC_CODE_LIB_TYPE_MASK (0x0C)
#define EXTEND_FUNC_CODE_MIN_BUFFER_LEN 8

/* **********************************************
 *  API: Version Control
 * ***********************************************/

#define EXTEND_FUNC_MIN_BUFFER_LEN_GET_API_VERSION_V1 8
#define EXTEND_FUNC_MIN_BUFFER_LEN_GET_API_VERSION_V2 16

/* **********************************************
 *  LIB API: Config Type
 * ***********************************************/
/* * * * * * * * * * * * * * * *
 *   META                      *
 * * * * * * * * * * * * * * * */
//   META :: API Code
//   META :: API Version
//   META :: API Min Required Buffer Size

/* * * * * * * * * * * * * * * *
 *   AR                        *
 * * * * * * * * * * * * * * * */
//   AR :: API Code
//   AR :: API Version
//   AR :: API Min Required Buffer Size

/* **********************************************
 *  LIB API: Data Type
 * ***********************************************/
/* * * * * * * * * * * * * * * *
 *   GYRO2D                    *
 * * * * * * * * * * * * * * * */
//   GYRO2D :: API Code
//   GYRO2D :: API Version
//   GYRO2D :: API Min Required Buffer Size

/* **********************************************
 *  A2DP-Extended API:
 * ***********************************************/
/* * * * * * * * * * * * * * * *
 *   GetA2DPSpecifis           *
 * * * * * * * * * * * * * * * */
//   GetA2DPSpecifis :: API Code
//   GetA2DPSpecifis :: API Version
#define EXTEND_FUNC_VER_GET_SPECIFIC_V1 ((uint32_t)0x01000000)

/* ************************************************************************
 * Version format info of EXTEND_FUNC_CODE_GET_SPECIFIC
 * EXTEND_FUNC_VER_GET_SPECIFIC_V1:  Total Size: 41 bytes
   * API Version:                   (4 bytes)
   * API Code:                      (4 bytes)
   * A2DP Codec Config Code:        (1 bytes)
   * A2dp Specific1:                (8 bytes)
   * A2dp Specific2:                (8 bytes)
   * A2dp Specific3:                (8 bytes)
   * A2dp Specific4:                (8 bytes)
 * EXTEND_FUNC_VER_GET_SPECIFIC_V2:  Total Size: 64 bytes
   * API Version:                   (4 bytes)
   * API Code:                      (4 bytes)
   * A2DP Codec Config Code:        (1 bytes)
   * Reserved:                      (7 bytes)
   * A2dp Specific1:                (8 bytes)
   * A2dp Specific2:                (8 bytes)
   * A2dp Specific3:                (8 bytes)
   * A2dp Specific4:                (8 bytes)
   * Capabilities Metadata sub fields:  (7*2 bytes)
     * sub[0~1]:    JAS
     * sub[2~3]:    AR
     * sub[4~5]:    META
     * sub[6~7]:    LLAC
     * sub[8~9]:    MBR
     * sub[10~11]:  LARC
     * sub[12~13]:  LHDCV4
   * Padded:                        (2 bytes)
 * ************************************************************************/
#define LHDC_EXTEND_FUNC_CONFIG_API_VERSION_SIZE 4  /* API version */
#define LHDC_EXTEND_FUNC_CONFIG_API_CODE_SIZE 4     /* API index code */
#define LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_SIZE 1 /* A2DP codec config code */
#define LHDC_EXTEND_FUNC_CONFIG_RESERVED_V2 7       /* V2 Reserved bytes */
#define LHDC_EXTEND_FUNC_CONFIG_SPECIFIC1_SIZE 8    /* Specific 1 */
#define LHDC_EXTEND_FUNC_CONFIG_SPECIFIC2_SIZE 8    /* Specific 2 */
#define LHDC_EXTEND_FUNC_CONFIG_SPECIFIC3_SIZE 8    /* Specific 3 */
#define LHDC_EXTEND_FUNC_CONFIG_SPECIFIC4_SIZE 8    /* Specific 4 */
/* Capabilities metadata fields(2 bytes for each tuple) */
#define LHDC_EXTEND_FUNC_CONFIG_CAPMETA_SIZE_V2 (7 << 1) /* V2 Capabilities */
#define LHDC_EXTEND_FUNC_CONFIG_PADDED_SIZE_V2 2         /* V2 Padded Fields */

/* Total size of buffer */
#define LHDC_EXTEND_FUNC_CONFIG_TOTAL_FIXED_SIZE_V1                                                \
    (LHDC_EXTEND_FUNC_CONFIG_API_VERSION_SIZE + LHDC_EXTEND_FUNC_CONFIG_API_CODE_SIZE +            \
     LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_SIZE + LHDC_EXTEND_FUNC_CONFIG_SPECIFIC1_SIZE +          \
     LHDC_EXTEND_FUNC_CONFIG_SPECIFIC2_SIZE + LHDC_EXTEND_FUNC_CONFIG_SPECIFIC3_SIZE +             \
     LHDC_EXTEND_FUNC_CONFIG_SPECIFIC4_SIZE)
#define LHDC_EXTEND_FUNC_CONFIG_TOTAL_FIXED_SIZE_V2                                                \
    (LHDC_EXTEND_FUNC_CONFIG_API_VERSION_SIZE + LHDC_EXTEND_FUNC_CONFIG_API_CODE_SIZE +            \
     LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_SIZE + LHDC_EXTEND_FUNC_CONFIG_RESERVED_V2 +             \
     LHDC_EXTEND_FUNC_CONFIG_SPECIFIC1_SIZE + LHDC_EXTEND_FUNC_CONFIG_SPECIFIC2_SIZE +             \
     LHDC_EXTEND_FUNC_CONFIG_SPECIFIC3_SIZE + LHDC_EXTEND_FUNC_CONFIG_SPECIFIC4_SIZE +             \
     LHDC_EXTEND_FUNC_CONFIG_CAPMETA_SIZE_V2 + LHDC_EXTEND_FUNC_CONFIG_PADDED_SIZE_V2)
/* Head of each field */
#define LHDC_EXTEND_FUNC_CONFIG_API_VERSION_HEAD (0)
#define LHDC_EXTEND_FUNC_CONFIG_API_CODE_HEAD (LHDC_EXTEND_FUNC_CONFIG_API_VERSION_HEAD + 4)  //4
#define LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD (LHDC_EXTEND_FUNC_CONFIG_API_CODE_HEAD + 4) //8
/* Following part in V1 */
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1                                                   \
    (LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD + 1) //9~16
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1 + 8) //17~24
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1 + 8) //25~32
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1 + 8) //33~40
/* Following part in V2 */
#define LHDC_EXTEND_FUNC_A2DP_RESERVED_HEAD_V2                                                     \
    (LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD + 1) //9~15
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2                                                   \
    (LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD + 8) //16~23
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2 + 8) //24~31
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2 + 8) //32~39
#define LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2                                                   \
    (LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2 + 8)                                           //40~47
#define LHDC_EXTEND_FUNC_A2DP_CAPMETA_HEAD_V2 (LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2 + 8) //48~61
#define LHDC_EXTEND_FUNC_A2DP_PADDED_HEAD_V2                                                       \
    (LHDC_EXTEND_FUNC_A2DP_CAPMETA_HEAD_V2 + LHDC_EXTEND_FUNC_CONFIG_CAPMETA_SIZE_V2) //62~63

/* code definition mapping to A2DP codec specific in a2dp_codec_api.h
 * 0x01: codec_config_
 * 0x02: codec_capability_
 * 0x03: codec_local_capability_
 * 0x04: codec_selectable_capability_
 * 0x05: codec_user_config_
 * 0x06: codec_audio_config_
 */
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_FINAL_CFG (0x01)
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_FINAL_CAP (0x02)
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_LOCAL_CAP (0x03)
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_SELECTABLE_CAP (0x04)
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_USER_CFG (0x05)
#define LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_AUDIO_CFG (0x06)

/************************
 * Capability Meta Format: (denotes where source capabilities bits are stored in specifics)
   * Capability Code:                   (1 byte)
   * Saving Position Info:              (1 byte)
 ************************/
/* Capabilities's code: */
#define LHDC_EXTEND_FUNC_A2DP_LHDC_JAS_CODE (0x01)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_AR_CODE (0x02)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_META_CODE (0x03)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_LLAC_CODE (0x04)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_MBR_CODE (0x05)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_LARC_CODE (0x06)
#define LHDC_EXTEND_FUNC_A2DP_LHDC_V4_CODE (0x07)

/* Capabilities's saving position Info:
 *  1. in which specific                        (represented in leftmost 2-bits)
 *  2. at which bit position of the specific    (represented in rightmost 6-bits)
 * */
#define LHDC_EXTEND_FUNC_A2DP_SPECIFIC1_INDEX (0x00) //2-bit:00
#define LHDC_EXTEND_FUNC_A2DP_SPECIFIC2_INDEX (0x40) //2-bit:01
#define LHDC_EXTEND_FUNC_A2DP_SPECIFIC3_INDEX (0x80) //2-bit:10
#define LHDC_EXTEND_FUNC_A2DP_SPECIFIC4_INDEX (0xC0) //2-bit:11
/********************************************************************************/

//
// META
//

//
// AR
//

#ifdef NEW_API_SET
//for NEW API used!!!!
typedef struct {
    uint32_t sample_rate;
    LHDCBT_SMPL_FMT_T bits_per_sample;
    LHDCBT_QUALITY_T audio_quality;
    bool channel_split_enabled;
    uint32_t packet_mtu;
    uint32_t encode_interval;

    bool output_size_cal;
} lhdc_init_param_t;

int32_t lhdcBT_init(HANDLE_LHDC_BT handle, lhdc_init_param_t *param);
#else
HANDLE_LHDC_BT lhdcBT_get_handle(int32_t version);

/* Query total instance capacity for the current ABI; unknown versions return zero and base alignment is 16 bytes. */
size_t lhdcBT_workspace_bytes(int32_t version);
/* Borrow caller workspace; free_handle does not release it. */
HANDLE_LHDC_BT lhdcBT_get_handle_in_memory(int32_t version, void *memory, size_t bytes);
/* Allocate instance storage once through host callbacks; free_handle releases the original allocation. */
HANDLE_LHDC_BT lhdcBT_get_handle_with_allocator(int32_t version, const LhdcAllocator *allocator);

int32_t lhdcBT_init_encoder(HANDLE_LHDC_BT handle, int32_t sampling_freq, int32_t bitPerSample,
                            int32_t bitrate_inx, int32_t dualChannel, int32_t need_padding,
                            int32_t mtu, int32_t interval);
void lhdcBT_set_max_bitrate(HANDLE_LHDC_BT handle, int32_t max_rate_index);
int32_t lhdcBT_set_hasMinBitrateLimit(HANDLE_LHDC_BT handle, bool enabled);

void lhdcBT_free_handle(HANDLE_LHDC_BT handle);

//static const char* LHDC_ENCODE_NAME = "lhdcBT_encode";
//Encoder for V2
int32_t lhdcBT_encode(HANDLE_LHDC_BT hLhdcParam, void *p_pcm, uint8_t *p_stream);
//Encoder for V3
int32_t lhdcBT_encodeV3(HANDLE_LHDC_BT hLhdcParam, void *p_pcm, uint8_t *out_put, uint32_t *written,
                        uint32_t *out_fraems);

int32_t lhdcBT_get_bitrate(HANDLE_LHDC_BT hLhdcParam);

int32_t lhdcBT_set_bitrate(HANDLE_LHDC_BT handle, int32_t bitrate_inx);

int32_t lhdcBT_adjust_bitrate(HANDLE_LHDC_BT handle, size_t queueLength);

int32_t lhdcBT_get_block_Size(HANDLE_LHDC_BT handle);

int32_t lhdcBT_set_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool enabled, void *priv, int32_t priv_data_len);

int32_t lhdcBT_get_ext_func_state(HANDLE_LHDC_BT handle, lhdcBT_ext_func_field_t field,
                                  bool *enabled);

//
// Extra API
//
// 1. API -- Set User Config (Extend)
int32_t lhdcBT_set_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen);
// 2. API -- Get User Config (Extend)
int32_t lhdcBT_get_user_exconfig(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen);
// 3. API -- Set User Data (Extend)
void lhdcBT_set_user_exdata(HANDLE_LHDC_BT handle, char *userConfig, int32_t clen);
// 4. API -- Get Version
int32_t lhdcBT_get_user_exApiver(HANDLE_LHDC_BT handle, char *version, int32_t clen);

#endif
#ifdef __cplusplus
}
#endif
#endif /* _LHDCBT_H_ */
