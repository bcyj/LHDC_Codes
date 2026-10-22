# 编码器

[English](README.md) | 简体中文

公开头文件位于 `include/lhdc/encoder/`，总入口为 `<lhdc/encoder/lhdc_bt.h>`。V4/V5 算法位于 `src/v4/`、`src/v5/`；`src/api/` 提供调用入口，`src/compat/` 提供跨平台 BT 包装及关闭 AR 时的适配。

CMake 目标为 `lhdc::encoder`，可选择是否接入 AR。V4 包含 LHDC/LLAC；V5 提供可选的 MDCT 定点 FFT 后端，默认关闭。

见[功能说明](../../docs/zh-CN/features.md)与[构建与接入](../../docs/zh-CN/build-and-integration.md)。
