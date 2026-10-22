#ifndef LHDC_WINDOWS_KERNEL_MEMORY_H
#define LHDC_WINDOWS_KERNEL_MEMORY_H
#include <ntddk.h>
#include "lhdc_memory.h"
/* Owned by the device/stream object and outlives associated instances; tag is immutable after initialization. */
typedef struct LhdcKernelMemoryContext {
    ULONG tag;
} LhdcKernelMemoryContext;
#ifdef __cplusplus
extern "C" {
#endif
/* Bind nonpaged, nonexecutable Windows pool allocation; minimum target Windows 10 2004. */
NTSTATUS NTAPI lhdc_kernel_memory_allocator_init(LhdcAllocator *allocator,
                                                 LhdcKernelMemoryContext *context, ULONG tag);
#ifdef __cplusplus
}
#endif
#endif
