/* BT extension constants and packed packet layouts shared by V4/V5.
 * Public names and packing remain unchanged; core encoder state types remain separate. */
#ifndef LHDC_COMMON_BT_EXT_H
#define LHDC_COMMON_BT_EXT_H
#include <stdint.h>

#define EXTEND_FUNC_RET_OK ((int32_t)0)
#define EXTEND_FUNC_RET_INVALID_HANDLE ((int32_t)0xE01)
#define EXTEND_FUNC_RET_INVALID_PARAMETER ((int32_t)0xE02)
#define EXTEND_FUNC_RET_FUNC_NOT_SUPPORT ((int32_t)0xE03)
#define EXTEND_FUNC_RET_VERSION_NOT_SUPPORT ((int32_t)0xE04)
#define EXTEND_FUNC_RET_BUF_UNDERRUN ((int32_t)0xE05)
#define EXTEND_FUNC_RET_ERROR ((int32_t)0xE06)
#define EXTEND_FUNC_RET_NOT_READY ((int32_t)0xE07)
#define EXTEND_FUNC_RET_REQ_BUFSIZE_NOT_MATCH ((int32_t)0xE08)

/* **********************************************
 *  API: Version Control
 * ***********************************************/
#define EXTEND_FUNC_GENERIC_VERSION_NUMBER_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_GENERIC_VERSION_NUMBER_V2 ((uint32_t)0x02000000)
#define EXTEND_FUNC_MIN_BUFFER_LEN (8)

/* **********************************************
 *  LIB API: Config Type
 * ***********************************************/
/* * * * * * * * * * * * * * * *
 *   META                      *
 * * * * * * * * * * * * * * * */
//   META :: API Code
#define EXTEND_FUNC_CODE_SET_CONFIG_META ((uint32_t)0x0C000001)
#define EXTEND_FUNC_CODE_GET_CONFIG_META ((uint32_t)0x0C010001)
//   META :: API Version
#define EXTEND_FUNC_VER_SET_CONFIG_META_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_VER_SET_CONFIG_META_V2 ((uint32_t)0x02000000)
#define EXTEND_FUNC_VER_GET_CONFIG_META_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_VER_GET_CONFIG_META_V2 ((uint32_t)0x02000000)
//   META :: API Min Required Buffer Size
#define EXTEND_FUNC_VER_SET_CONFIG_META_REQ_BUFSIZE_V2 16 //TBD
#define EXTEND_FUNC_VER_GET_CONFIG_META_REQ_BUFSIZE_V2 16 //TBD

/* * * * * * * * * * * * * * * *
 *   AR                        *
 * * * * * * * * * * * * * * * */
//   AR :: API Code
#define EXTEND_FUNC_CODE_SET_CONFIG_AR ((uint32_t)0x0C000002)
#define EXTEND_FUNC_CODE_GET_CONFIG_AR ((uint32_t)0x0C010002)
//   AR :: API Version
#define EXTEND_FUNC_VER_SET_CONFIG_AR_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_VER_SET_CONFIG_AR_V2 ((uint32_t)0x02000000)
#define EXTEND_FUNC_VER_SET_CONFIG_AR_V3 ((uint32_t)0x03000000)
#define EXTEND_FUNC_VER_GET_CONFIG_AR_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_VER_GET_CONFIG_AR_V2 ((uint32_t)0x02000000)
//   AR :: API Min Required Buffer Size
#define EXTEND_FUNC_VER_SET_CONFIG_AR_REQ_BUFSIZE_V2 16 //TBD
#define EXTEND_FUNC_VER_GET_CONFIG_AR_REQ_BUFSIZE_V2 16 //TBD

/* **********************************************
 *  LIB API: Data Type
 * ***********************************************/
/* * * * * * * * * * * * * * * *
 *   GYRO2D                    *
 * * * * * * * * * * * * * * * */
//   GYRO2D :: API Code
#define EXTEND_FUNC_CODE_SET_DATA_GYRO2D ((uint32_t)0x0D000001)
//   GYRO2D :: API Version
#define EXTEND_FUNC_VER_SET_DATA_GYRO2D_V1 ((uint32_t)0x01000000)
#define EXTEND_FUNC_VER_SET_DATA_GYRO2D_V2 ((uint32_t)0x02000000)
//   GYRO2D :: API Min Required Buffer Size
#define EXTEND_FUNC_VER_SET_DATA_GYRO2D_REQ_BUFSIZE_V2 16 //TBD (8 + 2gyro + 6pad)

/* * * * * * * * * * * * * * * *
 *   GetA2DPSpecifis           *
 * * * * * * * * * * * * * * * */
//   GetA2DPSpecifis :: API Code
#define EXTEND_FUNC_CODE_GET_SPECIFIC ((uint32_t)0x0A010001)
//   GetA2DPSpecifis :: API Version
#define EXTEND_FUNC_VER_GET_SPECIFIC_V2 ((uint32_t)0x02000000)

//
// META
//
#define META_ST_VER_V1 ((uint32_t)0x00010000)
#define META_ST_VER_V2 ((uint32_t)0x00020000)
#pragma pack(push)
#pragma pack(1)
typedef struct __ST_LHDC_SET_META {

    uint8_t header[8];
    uint32_t meta_ver;
    uint8_t meta_mem_size;
    uint8_t meta_enable;
    uint8_t meta_set;
    uint8_t meta_metadata_length;
    //  unsigned char     meta_metadata[64];

} ST_LHDC_SET_META, *PST_LHDC_SET_META;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct __ST_LHDC_GET_META {

    uint8_t header[8];
    uint32_t meta_ver;
    uint8_t meta_mem_size;
    uint8_t meta_st; // [0] 1: reserved bit, [1] 1: in meta mode/0: not in meta mode
    uint8_t jas_status;

} ST_LHDC_GET_META, *PST_LHDC_GET_META;
#pragma pack(pop)

//
// AR
//

#pragma pack(push)
#pragma pack(1)
typedef struct __ST_LHDC_BT_AR {
    uint8_t header[8];
    uint32_t ver;
    uint32_t size;
    uint32_t app_ar_enabled;
    int32_t Ch1_Pos;
    int32_t Ch2_Pos;
    int32_t Ch3_Pos;
    int32_t Ch4_Pos;
    int32_t Ch5_Pos;
    int32_t Ch6_Pos;
    float Ch1_L_PreGain;
    float Ch1_R_PreGain;
    float Ch2_L_PreGain;
    float Ch2_R_PreGain;
    float Ch3_L_PreGain;
    float Ch3_R_PreGain;
    float Ch4_L_PreGain;
    float Ch4_R_PreGain;
    float Ch5_L_PreGain;
    float Ch5_R_PreGain;
    float Ch6_L_PreGain;
    float Ch6_R_PreGain;
    float Ch1_PostGain;
    float Ch2_PostGain;
    float Ch3_PostGain;
    float Ch4_PostGain;
    float Ch5_PostGain;
    float Ch6_PostGain;
    float Dry_Val;
    float Wet_Val;
    float Dis_1;
    float Dis_2;
    float Dis_3;
    float Dis_4;
    float Dis_5;
    float Rev_1;
    float Rev_2;
    float Rev_3;
    float Rev_4;
    float Rev_5;
    float Rev_gain;
    float ThreeD_gain;
} ST_LHDC_BT_AR, *PST_LHDC_BT_AR;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct __ST_LHDC_BT_AR_GYRO {
    uint8_t header[8];
    int32_t world_coordinate_x;
    int32_t world_coordinate_y;
    int32_t world_coordinate_z;

} ST_LHDC_BT_AR_GYRO, *PST_LHDC_BT_AR_GYRO;
#pragma pack(pop)

#endif
