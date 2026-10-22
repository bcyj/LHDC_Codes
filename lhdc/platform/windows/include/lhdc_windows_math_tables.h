#ifndef LHDC_WINDOWS_CODEC_MATH_TABLES_H
#define LHDC_WINDOWS_CODEC_MATH_TABLES_H
#include <audio_math/platform/windows/tables.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
#include "lhdc_windows_float_math_constants.h"
extern const float lhdc_win_log10f_table[129][2];
extern const uint16_t lhdc_win_log10f_high_bits[129];
extern const double lhdc_win_pow_log_table[LHDC_WIN_POW_LOG_TABLE_ROWS][2];
#endif
#endif
