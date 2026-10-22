#include "lhdc_log.h"
#ifndef LHDC_AR_SRC_CORE_INCLUDE_AR_INTERNAL_H
#define LHDC_AR_SRC_CORE_INCLUDE_AR_INTERNAL_H
#include "ar_numeric_policy.h"
#include "ar_tables.h"
#include "lhdc_version.h"
#include "../../../include/ar/types.h"
#include "ar_interfaces.h"
#include "ar_workspace_views.h"
#if LHDC_CODE_VERSION == 4
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#else
#include "../../../../common/include/compat/v5/lhdc_v5_constants.h"
#endif
#include <math.h>
#include <stdlib.h>
#include <string.h>

void v4f_cmplx_multi(cc_buf_list *input_buf_list, const float *coef, float *sum);
#endif
