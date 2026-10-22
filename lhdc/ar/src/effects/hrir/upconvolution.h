#ifndef LHDC_AR_SRC_EFFECTS_HRIR_UPCONVOLUTION_H
#define LHDC_AR_SRC_EFFECTS_HRIR_UPCONVOLUTION_H
#include "lhdc_version.h"
#include <stdint.h>
/* Perform per-bin complex multiplication and accumulate convolution spectra. */
static inline void LHDC_PRIVATE(accumulate_complex_spectrum)(float *sum, const float *data,
                                                             const float *coef) {
    for (int32_t i = 0; i < LHDC_CONST(AR_SPECTRUM_FLOATS); i += 2) {
        float ar = data[i], ai = data[i + 1], cr = coef[i], ci = coef[i + 1], sr = sum[i],
              si = sum[i + 1];
        sum[i] = sr + (ar * cr - ai * ci);
        sum[i + 1] = (ai * cr + ar * ci) + si;
    }
}
#endif
