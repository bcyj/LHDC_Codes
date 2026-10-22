#ifndef LHDC_ENCODER_SRC_V4_CODEC_ENCODER_TABLES_H
#define LHDC_ENCODER_SRC_V4_CODEC_ENCODER_TABLES_H
#include "../include/types/lhdc_native_types.h"

extern const QUALITY_PRESET def_q_set16[16];
extern const QUALITY_PRESET def_q_set24[16];
extern const double ITUR468_44100[233];
extern const double ITUR468_48000[214];
extern const double ITUR468_96000[107];
/* Standard lengths return 1; other lengths/null output return 0 without writes. */
int v4_copy_analysis_window(int32_t length, float *window);
/* Output two power coefficients for any valid preset; nondefaults use the dynamic formula. */
void v4_quality_power_factors(const QUALITY_PRESET *preset, float *min_bits_rms,
                              float *min_noise_floor);
/* Each output range holds LHDC_V4_LPC_TUKEY_TAPER_SAMPLES floats. */
void v4_copy_lpc_tukey_taper(float *fade_in, float *fade_out);
#endif
