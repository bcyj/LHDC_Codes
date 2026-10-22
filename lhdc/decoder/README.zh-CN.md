# 解码器

[English](README.md) | 简体中文

公开头文件位于 `include/lhdc/decoder/`，总入口为 `<lhdc/decoder/decoder.h>`。`src/api/v4/`、`src/api/v5/` 负责实例初始化、解码、控制和日志；算法按版本置于 `src/v4/`、`src/v5/`。

V4 包含 LHDC、LLAC 及上采样；V5 包含有损帧解析、定点 FFT 和时域合成。工作区按当前 ABI 查询并由宿主提供。

CMake 总目标为 `lhdc::decoder`。单版静态目标为 `lhdc_decoder_v4`、`lhdc_decoder_v5`，组合目标为 `lhdc_decoder`。这些构建目标名不限制目标处理器。

见[功能说明](../../docs/zh-CN/features.md)与[构建与接入](../../docs/zh-CN/build-and-integration.md)。
