#include "../../encoder/src/v4/include/codec/integer_ops.h"
#include <stdio.h>
/* Check V4 platform shift-count policy and pointer-width configuration. */
int main(void) {
    if (v4_shift_left_compat32(3u, 1u) != 6u || v4_shift_right_compat32(6u, 1u) != 3u)
        return 1;
#if LHDC_V4_SHIFT_COUNT_BITS == 8
    if (v4_shift_left_compat32(1u, 32u) != 0 || v4_shift_right_compat32(UINT32_MAX, 32u) != 0)
        return 2;
#else
    if (v4_shift_left_compat32(1u, 32u) != 1 ||
        v4_shift_right_compat32(UINT32_MAX, 32u) != UINT32_MAX)
        return 3;
#endif
    printf("PASS explicit V4 shift model %d on %u-bit host\n", LHDC_V4_SHIFT_COUNT_BITS,
           (unsigned)(sizeof(void *) * 8));
    return 0;
}
