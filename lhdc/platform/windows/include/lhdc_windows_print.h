#include <audio_math/float_properties.h>
#ifndef LHDC_WINDOWS_PRINT_H
#define LHDC_WINDOWS_PRINT_H
/* Normalize NaN to a quiet NaN on Windows output paths; leave other values unchanged. */
static inline double lhdc_print_number(double value) {
#ifdef _WIN32
    if (audio_math_is_nan_f64(value)) {
        uint64_t quiet_nan = UINT64_C(0x7ff8000000000000);
        memcpy(&value, &quiet_nan, sizeof value);
    }
#endif
    return value;
}
#endif
