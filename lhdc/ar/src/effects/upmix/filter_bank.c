#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <string.h>

static const double *const LHDC_PRIVATE(filter_bank_coefficients)[2][4][2] = {
    {{lhdc_ar_bank_a_44k_0_1, lhdc_ar_bank_00_44k_b},
     {lhdc_ar_bank_a_44k_0_1, lhdc_ar_bank_01_44k_b},
     {lhdc_ar_bank_a_44k_2_3, lhdc_ar_bank_02_44k_b},
     {lhdc_ar_bank_a_44k_2_3, lhdc_ar_bank_03_44k_b}},
    {{lhdc_ar_bank_a_48k_0_1, lhdc_ar_bank_00_48k_b},
     {lhdc_ar_bank_a_48k_0_1, lhdc_ar_bank_01_48k_b},
     {lhdc_ar_bank_a_48k_2_3, lhdc_ar_bank_02_48k_b},
     {lhdc_ar_bank_a_48k_2_3, lhdc_ar_bank_03_48k_b}}};

/* Compute filter-bank coefficients from frequency and gain parameters. */
int32_t LHDC_FN(comple_filter_bank_get_coefficient_ex)(int32_t bank_index, int32_t sample_rate,
                                                       LHDCIirPlan *plan) {
    plan->denominator = 0;
    plan->numerator = 0;
    if (sample_rate != LHDC_CONST(SAMPLE_RATE_44100) &&
        sample_rate != LHDC_CONST(SAMPLE_RATE_48000))
        return LHDC_CONST(FILTER_UNSUPPORTED_SAMPLE_RATE);
    if ((uint32_t)bank_index >= 4) {
        return LHDC_CONST(STATUS_OK);
    }
    plan->denominator = LHDC_PRIVATE(
        filter_bank_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)][bank_index][0];
    plan->numerator = LHDC_PRIVATE(
        filter_bank_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)][bank_index][1];
    if (plan)
        plan->order = 9;
    return LHDC_CONST(STATUS_OK);
}

/* Compute filter-bank coefficients from frequency and gain parameters. */
int32_t LHDC_FN(comple_filter_bank_get_coefficient)(int32_t bank_index, int32_t sample_rate,
                                                    double *rtn_coef_a, double *rtn_coef_b) {
    if (!rtn_coef_a || !rtn_coef_b)
        return LHDC_CONST(FILTER_NULL_COEFFICIENT_OUTPUT);
    if ((uint32_t)bank_index >= 4 || (sample_rate != LHDC_CONST(SAMPLE_RATE_44100) &&
                                      sample_rate != LHDC_CONST(SAMPLE_RATE_48000)))
        return LHDC_CONST(FILTER_INVALID_SELECTION);
    const double **coefficient_table = (const double **)LHDC_PRIVATE(
        filter_bank_coefficients)[sample_rate == LHDC_CONST(SAMPLE_RATE_48000)][bank_index];
    memcpy(rtn_coef_a, coefficient_table[0], sizeof lhdc_ar_bank_a_44k_0_1);
    memcpy(rtn_coef_b, coefficient_table[1], sizeof lhdc_ar_bank_00_44k_b);
    return LHDC_CONST(STATUS_OK);
}
