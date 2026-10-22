#include "../../include/dsp/adsq_tables.h"

/* Read-only codebooks; prediction state, transition direction, and mean removal remain codec-instance-specific. */
const int32_t lhdc_v5_adsq_step_sizes[LHDC_V5_ADSQ_TABLE_SIZE] = {
    6, 13, 19, 26, 32, 38, 45, 51,
    58, 64, 77, 90, 102, 115, 128, 141,
    154, 166, 186, 205, 224, 243, 262, 282,
    301, 326, 352, 378, 403, 429, 461, 493,
    525, 557, 595, 634, 672, 717, 762, 813,
    870, 934, 1005, 1082, 1165, 1254, 1350, 1453,
    1562, 1677, 1798, 1926, 2061, 2202, 2349, 2502,
    2662, 2829, 3002, 3187, 3386, 3603, 3840, 4096};
