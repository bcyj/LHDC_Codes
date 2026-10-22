#ifndef LHDC_LOG_H
#define LHDC_LOG_H

/* Diagnostic logging is disabled by default; disabled calls evaluate no arguments. */
#ifndef LHDC_ENABLE_LOGGING
#define LHDC_ENABLE_LOGGING 0
#endif
#if LHDC_ENABLE_LOGGING != 0 && LHDC_ENABLE_LOGGING != 1
#error LHDC_ENABLE_LOGGING must be 0 or 1
#endif

typedef enum LhdcLogPriority {
    LHDC_LOG_UNKNOWN = 0,
    LHDC_LOG_DEFAULT = 1,
    LHDC_LOG_VERBOSE = 2,
    LHDC_LOG_DEBUG = 3,
    LHDC_LOG_INFO = 4,
    LHDC_LOG_WARN = 5,
    LHDC_LOG_ERROR = 6,
    LHDC_LOG_FATAL = 7,
    LHDC_LOG_SILENT = 8
} LhdcLogPriority;

#if defined(__GNUC__) || defined(__clang__)
#define LHDC_LOG_PRINTF_FORMAT __attribute__((format(printf, 3, 4)))
#else
#define LHDC_LOG_PRINTF_FORMAT
#endif

#if defined(LHDC_LOG_IMPLEMENTATION)
int lhdc_log_print(int priority, const char *tag, const char *format, ...) LHDC_LOG_PRINTF_FORMAT;
#elif !LHDC_ENABLE_LOGGING
/* Preserve the successful diagnostic return value without formatting, output, or argument evaluation. */
static inline int lhdc_log_disabled(void) {
    return 1;
}
#define lhdc_log_print(...) lhdc_log_disabled()
#elif defined(__ANDROID__)
#include "../../platform/android/include/lhdc_android_log.h"
#else
#ifdef __cplusplus
extern "C" {
#endif
int lhdc_log_print(int priority, const char *tag, const char *format, ...) LHDC_LOG_PRINTF_FORMAT;
#ifdef __cplusplus
}
#endif
#endif

/* BT logging macros use the caller-defined LHDC_LOG_TAG. */
#define LHDC_LOGV(...) lhdc_log_print(LHDC_LOG_VERBOSE, LHDC_LOG_TAG, __VA_ARGS__)
#define LHDC_LOGD(...) lhdc_log_print(LHDC_LOG_DEBUG, LHDC_LOG_TAG, __VA_ARGS__)
#define LHDC_LOGI(...) lhdc_log_print(LHDC_LOG_INFO, LHDC_LOG_TAG, __VA_ARGS__)
#define LHDC_LOGW(...) lhdc_log_print(LHDC_LOG_WARN, LHDC_LOG_TAG, __VA_ARGS__)
#define LHDC_LOGE(...) lhdc_log_print(LHDC_LOG_ERROR, LHDC_LOG_TAG, __VA_ARGS__)

/* Standard-output diagnostic dumps use the same output switch. */
#if LHDC_ENABLE_LOGGING
#include <stdio.h>
#define lhdc_debug_printf(...) printf(__VA_ARGS__)
#define lhdc_debug_puts(...) puts(__VA_ARGS__)
#define lhdc_debug_putchar(...) putchar(__VA_ARGS__)
#else
#define lhdc_debug_printf(...) lhdc_log_disabled()
#define lhdc_debug_puts(...) lhdc_log_disabled()
#define lhdc_debug_putchar(...) lhdc_log_disabled()
#endif
#endif
