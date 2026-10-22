# LHDC 组件

[English](README.md) | 简体中文

`encoder/`、`decoder/`、`ar/` 分别提供编码、解码和 PCM 音效；`common/` 管理跨组件实例内存、缓冲和变换；`platform/` 提供宿主适配。

独立公共数学模块位于相邻 `../math/`，通用 PCM 后端契约位于 `../ar/`。版本相关实例和状态分别维护，共享只读表只定义一份。

见[功能说明](../docs/zh-CN/features.md)与[构建与接入](../docs/zh-CN/build-and-integration.md)。
