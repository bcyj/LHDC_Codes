# Windows 内核内存后端

[English](README.md) | 简体中文

`windows_memory.c` 与 `include/lhdc_kernel_memory.h` 将宿主的 pool tag 和 context 适配为 `LhdcAllocator`，使用 `ExAllocatePool2(POOL_FLAG_NON_PAGED, ...)` 及 `ExFreePoolWithTag`。目标为 Windows 10 2004 或更新系统。

此目录提供内存来源适配，不包含完整驱动。宿主须编入公共内存实现，选择显式分配或借用工作区，并负责浮点状态、执行上下文、栈预算和内核链接。

见[构建与接入](../../../../docs/zh-CN/build-and-integration.md)。
