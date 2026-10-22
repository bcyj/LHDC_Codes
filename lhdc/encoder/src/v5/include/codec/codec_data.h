#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_DATA_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_DATA_H
#include "../types/lhdc_native_types.h"

extern const uint16_t hdr_info_mask[6];

extern const int32_t hdr_info_offset[6];

extern const BandStop_t band_stop[18];

extern const BandCfg_t band_480_HR;

extern const BandCfg_t band_480;

extern const BandCfg_t band_480_LB;

extern const BandCfg_t band_960;

extern const BandCfg_t band_1920;

extern float gpfLhdcArHrirCoefB[3][180][2][256];

extern const double bank_00_44k_a[10];

extern const double bank_00_44k_b[10];

extern const double bank_01_44k_a[10];

extern const double bank_01_44k_b[10];

extern const double bank_02_44k_a[10];

extern const double bank_02_44k_b[10];

extern const double bank_03_44k_a[10];

extern const double bank_03_44k_b[10];

extern const double bank_00_48k_a[10];

extern const double bank_00_48k_b[10];

extern const double bank_01_48k_a[10];

extern const double bank_01_48k_b[10];

extern const double bank_02_48k_a[10];

extern const double bank_02_48k_b[10];

extern const double bank_03_48k_a[10];

extern const double bank_03_48k_b[10];









extern const double shlv_filter_front_a_44k[];

extern const double shlv_filter_front_a_48k[];

extern const double shlv_filter_front_b_44k[];

extern const double shlv_filter_front_b_48k[];

extern const double shlv_filter_rear_a_44k[];

extern const double shlv_filter_rear_a_48k[];

extern const double shlv_filter_rear_b_44k[];

extern const double shlv_filter_rear_b_48k[];


#endif /* LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_DATA_H */
