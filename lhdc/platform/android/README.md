# Android Platform

English | [Simplified Chinese](README.zh-CN.md)

`include/` supplies Android ABI, logging, and numeric-policy adaptation. `Android.cmake` provides CMake settings; `Android.mk`, `Application.mk`, and `sources.mk` support direct NDK builds.

Logging maps to `__android_log_print`. BT packetization, ABR, MTU, and extension packets are cross-platform encoder adaptation in `lhdc/encoder/src/compat/`.

Run from the release root:

```powershell
python tools/generate_android_sources.py --check
python tools/build_android.py --ndk <NDK_PATH> --out build/android --abi both
```

See [features](../../../docs/features.md) and [build/integration](../../../docs/build-and-integration.md).
