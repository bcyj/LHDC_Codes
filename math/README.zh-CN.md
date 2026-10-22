# 公共数学模块

[English](README.md) | 简体中文

独立 C 数学模块，公开头文件使用 `<audio_math/...>`，CMake 目标为 `audio_math::math`。不依赖 LHDC 的类型、分配器或日志。

提供浮点位操作、常量、转换、取整、缩放、平方根、FMA、软件指数/对数/三角函数，以及无内部内存分配的浮点 FFT。平台实现位于 `platform/` 和 `include/audio_math/platform/`。共享表在 `.c` 唯一定义。

独立使用时可设置 `AUDIO_MATH_BUILD_TESTS=ON` 运行数值契约测试；`AUDIO_MATH_FFT_ITERATIVE_STATIC=ON` 选择非递归 FFT 调度。

数值函数的有效输入域、舍入及特殊值规则以头文件为准。本模块不替代完整 C99 libm。KISS FFT 引用及条款见[开源引用与许可](../THIRD_PARTY_NOTICES.zh-CN.md)，平台数学说明位于 [NOTICE.math.txt](platform/windows/NOTICE.math.txt)。

见[功能说明](../docs/zh-CN/features.md)与[构建与接入](../docs/zh-CN/build-and-integration.md)。
