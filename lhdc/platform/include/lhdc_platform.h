/* Platform selection entry; math compatibility and ABI details belong in their platform directories. */
#ifndef LHDC_RECOVERED_PLATFORM_H
#define LHDC_RECOVERED_PLATFORM_H
#include <stdint.h>
#include <audio_math/bits.h>
#include <audio_math/math.h>
#ifdef _WIN32
#include "../windows/include/lhdc_windows_abi.h"
#elif defined(__ANDROID__)
#include "../android/include/lhdc_android_abi.h"
#else
#include "../generic/include/lhdc_generic_abi.h"
#endif
#endif
