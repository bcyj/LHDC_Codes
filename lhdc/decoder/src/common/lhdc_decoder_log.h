#ifndef LHDC_DECODER_SRC_COMMON_LHDC_DECODER_LOG_H
#define LHDC_DECODER_SRC_COMMON_LHDC_DECODER_LOG_H
#include "../../../common/include/lhdc_log.h"
/* Portable per-instance decoder logging; disabled logs neither format, invoke callbacks, nor evaluate arguments. */
#if LHDC_ENABLE_LOGGING
#include <stdio.h>
#define LHDC_DECODER_LOG(callback, buffer, ...)                                                    \
    do {                                                                                           \
        if (callback) {                                                                            \
            snprintf(buffer, sizeof(buffer), __VA_ARGS__);                                                          \
            callback(buffer);                                                                      \
        }                                                                                          \
    } while (0)
#else
#define LHDC_DECODER_LOG(...) ((void)0)
#endif
#endif
