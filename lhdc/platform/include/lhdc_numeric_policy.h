#ifndef LHDC_PLATFORM_NUMERIC_POLICY_H
#define LHDC_PLATFORM_NUMERIC_POLICY_H

/* Platform entry points select policy; algorithms do not test Android/Bionic/Windows directly. */
#if defined(__ANDROID__)
#include "../android/include/lhdc_android_numeric_policy.h"
#elif defined(_WIN32)
#include "../windows/include/lhdc_windows_numeric_policy.h"
#else
#include "../generic/include/lhdc_generic_numeric_policy.h"
#endif

#endif
