# Encoders

English | [Simplified Chinese](README.zh-CN.md)

Public headers are in `include/lhdc/encoder/`, with umbrella `<lhdc/encoder/lhdc_bt.h>`. V4/V5 algorithms live in `src/v4/` and `src/v5/`; `src/api/` supplies entry points, and `src/compat/` supplies cross-platform BT wrappers and adaptation for builds without AR.

CMake target `lhdc::encoder` optionally integrates AR. V4 includes LHDC/LLAC. V5 offers an optional fixed FFT backend within MDCT, disabled by default.

See [features](../../docs/features.md) and [build/integration](../../docs/build-and-integration.md).
