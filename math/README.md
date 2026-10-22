# Shared Mathematics Module

English | [Simplified Chinese](README.zh-CN.md)

Standalone C mathematics with `<audio_math/...>` public headers and CMake target `audio_math::math`. It has no dependency on LHDC types, allocators, or logging.

Provides floating-point bits/constants, conversions, rounding, scaling, square root, FMA, software exponent/log/trigonometry, and floating FFT without internal allocation. Platform code is in `platform/` and `include/audio_math/platform/`. Shared tables have one `.c` definition.

For standalone tests, enable `AUDIO_MATH_BUILD_TESTS=ON`. `AUDIO_MATH_FFT_ITERATIVE_STATIC=ON` selects iterative FFT scheduling.

Headers specify valid domains, rounding, and special values. This is not a full C99 libm replacement. See [third-party notices](../THIRD_PARTY_NOTICES.md) for KISS FFT and [NOTICE.math.txt](platform/windows/NOTICE.math.txt) for platform mathematics.

See [features](../docs/features.md) and [build/integration](../docs/build-and-integration.md).
