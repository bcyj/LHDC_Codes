# AOSP / Android LHDC 参考说明

[English](aosp-reference.md) | 简体中文
本项目参考 Google Android Open Source Project 的 V5 Rust 编码实现，感谢对应作者与维护者。两组材料分别记录，不将旧版厂商文件统一归属为 Google 或 Android Open Source Project 的作品。

## Google AOSP 的 V5 编码实现

已在 Google 官方源码服务器确认：

- 仓库：[platform/packages/modules/Bluetooth](https://android.googlesource.com/platform/packages/modules/Bluetooth/)。
- 核对标签：[android-vts-17.0_r1](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1)，指向提交 `c77db469de80f86660bc053bec4dee0c5d4b947c`。
- 模块：[system/audio/codecs/lhdcv5](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5)。
- [Android.bp](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5/Android.bp) 明确声明 `default_applicable_licenses: ["Android-Apache-2.0"]`。
- [src/lhdcv5BT_enc.c](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5/src/lhdcv5BT_enc.c) 明确保留 `Copyright (C) 2025 The Android Open Source Project` 及 Apache-2.0 文件头。

| 参考范围 | 当前相关实现 | 引用与修改情况 |
|---|---|---|
| Rust V5 编码流程、数值类型及定点变换 | `lhdc/encoder/src/v5/codec/mdct.c` 等 V5 编码实现 | 用于流程分析及可选定点 FFT 接入参考；当前编码主路径仍包含浮点运算，并非原样翻译整套 Rust 编码器 |
| Rust `src/kiss_fft.rs` | `lhdc/common/src/dsp/fft_fixed32.c` 及其头文件 | 定点蝶形、逐级缩放与数值语义参考；当前版本经过 C 改写、迭代调度及实例工作区适配 |

Apache-2.0 全文随包保存于 [Apache-2.0.txt](Apache-2.0.txt)。AOSP 相关参考与改写部分保留相应版权及许可；当前 C 版的接口、布局、工作区和算法组织已有修改。分发受许可覆盖的衍生部分时，应提供许可副本、保留适用通知，并在修改文件中标明修改；以上说明不替代许可正文。

`src/kiss_fft.rs` 自身保留 Mark Borgerding 的 BSD 三条款文件头，其原文摘录见 [AOSP-Rust-KISS-FFT-BSD-3-Clause.txt](AOSP-Rust-KISS-FFT-BSD-3-Clause.txt)。模块默认 Apache-2.0 声明不替代这项第三方通知。该 FFT 引用同时见 [工程引用说明](../THIRD_PARTY_NOTICES.zh-CN.md#kiss-fft)。


本说明和第三方通知随源码发布版一并提供；它们只增加引用及许可记录，不改变任何接口或算法。
