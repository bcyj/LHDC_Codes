#ifndef LHDC_ANDROID_LOG_H
#define LHDC_ANDROID_LOG_H
#include <android/log.h>
/* Android retains native variadic calls/return values without reformatting. */
#define lhdc_log_print __android_log_print
#endif
