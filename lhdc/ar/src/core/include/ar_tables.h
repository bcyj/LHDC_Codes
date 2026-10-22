/* Version-independent read-only filter data; instance state is not shared here. */
#ifndef LHDC_AR_TABLES_H
#define LHDC_AR_TABLES_H
#define LHDC_AR_ABIR_COEFFICIENT_FLOATS 32768
#define LHDC_AR_HRIR_COEFFICIENT_FLOATS 276480

extern const float lhdc_ar_hrir_table[LHDC_AR_HRIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_w_l[LHDC_AR_ABIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_w_r[LHDC_AR_ABIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_x_l[LHDC_AR_ABIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_x_r[LHDC_AR_ABIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_y_l[LHDC_AR_ABIR_COEFFICIENT_FLOATS];
extern const float lhdc_ar_abir_y_r[LHDC_AR_ABIR_COEFFICIENT_FLOATS];

#define LHDC_AR_VBAP_GAIN_COUNT 181
extern const float lhdc_ar_vbap_gain_values[LHDC_AR_VBAP_GAIN_COUNT];
extern const double lhdc_ar_bank_a_44k_0_1[10];
extern const double lhdc_ar_bank_00_44k_b[10];
extern const double lhdc_ar_bank_01_44k_b[10];
extern const double lhdc_ar_bank_a_44k_2_3[10];
extern const double lhdc_ar_bank_02_44k_b[10];
extern const double lhdc_ar_bank_03_44k_b[10];
extern const double lhdc_ar_bank_a_48k_0_1[10];
extern const double lhdc_ar_bank_00_48k_b[10];
extern const double lhdc_ar_bank_01_48k_b[10];
extern const double lhdc_ar_bank_a_48k_2_3[10];
extern const double lhdc_ar_bank_02_48k_b[10];
extern const double lhdc_ar_bank_03_48k_b[10];
extern const double lhdc_ar_shlv_filter_front_a_44k[3];
extern const double lhdc_ar_shlv_filter_front_b_44k[3];
extern const double lhdc_ar_shlv_filter_rear_a_44k[3];
extern const double lhdc_ar_shlv_filter_rear_b_44k[3];
extern const double lhdc_ar_shlv_filter_front_a_48k[3];
extern const double lhdc_ar_shlv_filter_front_b_48k[3];
extern const double lhdc_ar_shlv_filter_rear_a_48k[3];
extern const double lhdc_ar_shlv_filter_rear_b_48k[3];
#endif
