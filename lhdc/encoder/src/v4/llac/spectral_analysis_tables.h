#include "../../../../platform/include/lhdc_numeric_policy.h"
#ifndef LHDC_ENCODER_LLAC_SPECTRAL_ANALYSIS_TABLES_H
#define LHDC_ENCODER_LLAC_SPECTRAL_ANALYSIS_TABLES_H
#include "../../../../common/include/types/lhdc_dsp_types.h"

/* Host availability for verified fixed tables; providers and kernel consumers share one condition. */
#if LHDC_PLATFORM_CALIBRATED_FLOAT_MATH
#define LHDC_LLAC_SNS_FIXED_TABLES_AVAILABLE 1
#else
#define LHDC_LLAC_SNS_FIXED_TABLES_AVAILABLE 0
#endif

/* Shared 16x16 double coefficients verified after multiply-to-float rounding on four ABIs; other hosts use dynamic formulas. */
const double (*lhdc_llac_sns_inverse_cosines(void))[16];

/* Verified normalized complex rotations for forward 16-point DCT-II; return null for other lengths/hosts. */
const kiss_fft_cpx *lhdc_llac_sns_forward_rotations(int length);

/* Verified six-tilt 64-band coefficients; return null for other parameters/hosts. */
const float *lhdc_llac_sns_tilt_gains(int tilt, int length);

#endif
