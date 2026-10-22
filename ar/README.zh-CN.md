# 通用 PCM 效果接口

[English](README.md) | 简体中文

本目录定义 PCM 格式、实例、工作区和 `ArPcmBackend` 回调；不含特定编码格式的状态或效果表。公开入口为 `<ar/pcm.h>`、`<ar/backend.h>`。

根 CMake 构建默认链接 LHDC AR，公开目标为 `audio_ar::ar`。设置 `AR_DEFAULT_BACKEND=NONE` 可单独构建头文件接口，调用时显式提供其他后端。

后端描述符必须在实例使用期间保持有效且只读；可变状态放入工作区。PCM 的采样率、声道数和最大块长由后端决定。

见[功能说明](../docs/zh-CN/features.md)与[构建与接入](../docs/zh-CN/build-and-integration.md)。
