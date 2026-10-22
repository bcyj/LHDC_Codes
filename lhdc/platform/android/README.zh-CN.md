# Android 平台

[English](README.md) | 简体中文

`include/` 提供 Android ABI、日志和数值策略适配；`Android.cmake` 提供 CMake 编译设置；`Android.mk`、`Application.mk` 和 `sources.mk` 提供直接 NDK 构建。

日志映射到 `__android_log_print`。BT 封包、ABR、MTU 及扩展报文处理属于跨平台编码适配，位于 `lhdc/encoder/src/compat/`。

在发布根目录执行：

```powershell
python tools/generate_android_sources.py --check
python tools/build_android.py --ndk <NDK目录> --out build/android --abi both
```

见[构建与接入](../../../docs/zh-CN/build-and-integration.md)。
