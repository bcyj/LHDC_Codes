/* Diagnostic output entry: adapt display format without changing platform numeric state. */
#ifndef LHDC_PRINT_H
#define LHDC_PRINT_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include "../windows/include/lhdc_windows_print.h"
#else
/* Generic platforms display the input value directly. */
static inline double lhdc_print_number(double value) {
    return value;
}
#endif
#endif
