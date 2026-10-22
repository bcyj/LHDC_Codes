# LHDC AR 效果后端

[English](README.md) | 简体中文

本目录实现上混、距离增益、混响、卷积和双耳定位，按实例保存音频历史。效果链直接处理 PCM，可独立于编解码器使用。

通用接口为 `<ar/pcm.h>`；选择 LHDC 后端使用 `<lhdc/ar/pcm_backend.h>`。直接管理 LHDC 参数时使用 `<ar/lhdc_pcm.h>`，按版本选择效果参数组。编码器专用适配位于 `src/compat/`。

CMake 目标为 `lhdc::ar`。见[功能说明](../../docs/zh-CN/features.md)与[构建与接入](../../docs/zh-CN/build-and-integration.md)。
