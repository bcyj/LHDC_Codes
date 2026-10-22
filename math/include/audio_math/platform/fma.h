#ifndef AUDIO_MATH_PLATFORM_FMA_H
#define AUDIO_MATH_PLATFORM_FMA_H
#if defined(__ANDROID__)
#include <audio_math/platform/android/fma.h>
#elif (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__aarch64__) || (defined(__arm__) && defined(__ARM_FEATURE_FMA)))
#define AUDIO_MATH_PLATFORM_NATIVE_FMA 1
#include <audio_math/platform/arm/fma.h>
#else
#define AUDIO_MATH_PLATFORM_NATIVE_FMA 0
#endif
#endif
