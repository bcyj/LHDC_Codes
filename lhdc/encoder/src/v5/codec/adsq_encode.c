#include "../../../../common/include/dsp/adsq_tables.h"


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "../include/codec/v5_extended.h"
#include "../include/codec/v5_pcm.h"



/* Quantize band scale factors and generate ADSQ indices/difference codes. */
void v5f_adsq_enc(int32_t *din, int32_t *dout, int32_t dn, uint8_t *code, int32_t *fi,
                  int32_t max_fi) {
    v5_adsq_enc((void *)din, (void *)dout, dn, (void *)code, (void *)fi, max_fi);
}

/* Quantize band scale factors and generate ADSQ indices/difference codes. */
void v5_adsq_enc(int32_t *din, int32_t *dout, int32_t dn, uint8_t *code, int32_t *fi,
                 int32_t max_fi) {
    int32_t start = *fi, idx = start, acc = 1;
    int32_t magnitude = (int32_t)(din[1] < 0 ? 0u - (uint32_t)din[1] : (uint32_t)din[1]);
    while (idx <= max_fi && magnitude > lhdc_v5_adsq_step_sizes[idx])
        ++idx;
    if (idx > start)
        --idx;
    *fi = idx;
    int32_t predict = din[0];
    dout[0] = predict;
    for (int32_t i = 1; i < dn; ++i) {
        uint32_t sign = din[i] < predict;
        code[i - 1] = (uint8_t)sign;
        if (i > 1) {
            if (sign == code[i - 2]) {
                idx += ++acc;
                if (idx > LHDC_V5_ADSQ_INDEX_SATURATION_THRESHOLD)
                    idx = LHDC_V5_ADSQ_INDEX_MAX;
            } else {
                idx = (idx * 3 + 2) >> 2;
                acc = 1;
            }
        }
        uint32_t step = (uint32_t)lhdc_v5_adsq_step_sizes[idx];
        predict = (int32_t)((uint32_t)predict + (sign ? 0u - step : step));
        dout[i] = predict;
    }
}
