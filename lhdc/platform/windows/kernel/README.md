# Windows Kernel Memory Backend

English | [Simplified Chinese](README.zh-CN.md)

`windows_memory.c` and `include/lhdc_kernel_memory.h` adapt host pool tags/context to `LhdcAllocator` using `ExAllocatePool2(POOL_FLAG_NON_PAGED, ...)` and `ExFreePoolWithTag`. Target Windows 10 2004 or later.

This supplies a memory-source adapter, not a complete driver. The host compiles common memory code, selects explicit allocation or borrowed workspace, and manages FP state, execution context, stack budget, and kernel linking.

See [features](../../../../docs/features.md) and [build/integration](../../../../docs/build-and-integration.md).
