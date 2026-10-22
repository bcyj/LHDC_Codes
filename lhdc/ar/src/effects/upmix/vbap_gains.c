#include "lhdc_version.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../core/include/ar_internal.h"

#include <math.h>
#include <string.h>

/* Generate a VBAP directional gain table with extended parameters. */
int32_t LHDC_FN(vbap_gain_get_table_ex)(const float **rtn_ptr_table) {
    if (rtn_ptr_table == 0) {
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    }
    *rtn_ptr_table = lhdc_ar_vbap_gain_values;
    return LHDC_CONST(STATUS_OK);
}


/* Generate a VBAP directional gain table. */
int32_t LHDC_FN(vbap_gain_get_table)(float *rtn_ptr_table) {
    if (rtn_ptr_table == (float *)0x0) {
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    }
    memcpy(rtn_ptr_table, lhdc_ar_vbap_gain_values, sizeof lhdc_ar_vbap_gain_values);
    return LHDC_CONST(STATUS_OK);
}

/* Return the VBAP gain table element count. */
int32_t LHDC_FN(vbap_gain_table_element_count)(int32_t *rtn_element_cnt) {
    if (!rtn_element_cnt)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    *rtn_element_cnt = LHDC_AR_VBAP_GAIN_COUNT;
    return LHDC_CONST(STATUS_OK);
}
