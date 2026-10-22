#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <string.h>

static const double *const LHDC_PRIVATE(shelving_coefficients)[2][2][2] = {
    {{lhdc_ar_shlv_filter_front_a_44k, lhdc_ar_shlv_filter_front_b_44k},
     {lhdc_ar_shlv_filter_rear_a_44k, lhdc_ar_shlv_filter_rear_b_44k}},
    {{lhdc_ar_shlv_filter_front_a_48k, lhdc_ar_shlv_filter_front_b_48k},
     {lhdc_ar_shlv_filter_rear_a_48k, lhdc_ar_shlv_filter_rear_b_48k}}};

/* Compute filter-bank coefficients from frequency and gain parameters. */
int32_t LHDC_FN(shelving_filter_get_coefficient_ex)(int32_t sample_rate, int32_t front_0_rear_1,
                                                    LHDCIirPlan *plan) {
    if (sample_rate != LHDC_CONST(SAMPLE_RATE_44100) &&
        sample_rate != LHDC_CONST(SAMPLE_RATE_48000))
        return LHDC_CONST(FILTER_UNSUPPORTED_SAMPLE_RATE);
    if (plan)
        plan->denominator =
            LHDC_PRIVATE(shelving_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)]
                                               [front_0_rear_1 != 0][0];
    if (plan)
        plan->numerator =
            LHDC_PRIVATE(shelving_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)]
                                               [front_0_rear_1 != 0][1];
    if (plan)
        plan->order = 2;
    return LHDC_CONST(STATUS_OK);
}

/* Compute filter-bank coefficients from frequency and gain parameters. */
int32_t LHDC_FN(shelving_filter_get_coefficient)(int32_t sample_rate, int32_t front_0_rear_1,
                                                 double *rtn_coef_a, double *rtn_coef_b) {
    const double *const *coefficient_table = LHDC_PRIVATE(
        shelving_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)][front_0_rear_1 != 0];
    memcpy(rtn_coef_a, coefficient_table[0], sizeof lhdc_ar_shlv_filter_front_a_44k);
    memcpy(rtn_coef_b, coefficient_table[1], sizeof lhdc_ar_shlv_filter_front_b_44k);
    return LHDC_CONST(STATUS_OK);
}
