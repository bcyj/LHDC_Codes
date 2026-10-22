/* Windows/generic user-mode backend: preserve the successful return value without standard output. */
#define LHDC_LOG_IMPLEMENTATION 1
#include "../../common/include/lhdc_log.h"
#if LHDC_ENABLE_LOGGING
#include <stdarg.h>
#include <stdio.h>
#endif

/* Validate formatting when diagnostics are enabled; otherwise skip variadic handling and formatting entirely. */
int lhdc_log_print(int priority, const char *tag, const char *format, ...) {
    (void)priority;
    (void)tag;
#if LHDC_ENABLE_LOGGING
    va_list args;
    va_start(args, format);
    int result = vsnprintf(NULL, 0, format, args);
    va_end(args);
    return result < 0 ? result : 1;
#else
    (void)format;
    return 1;
#endif
}
