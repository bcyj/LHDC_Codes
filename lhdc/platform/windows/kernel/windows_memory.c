#include "include/lhdc_kernel_memory.h"

/* Obtain storage accessible at DISPATCH_LEVEL; return NULL on failure without exceptions. */
static void *LHDC_MEMORY_CALL kernel_allocate(void *context, size_t bytes) {
    const LhdcKernelMemoryContext *owner = context;
    return ExAllocatePool2(POOL_FLAG_NON_PAGED, bytes, owner->tag);
}

/* Release the original allocation address retained by the common memory layer. */
static void LHDC_MEMORY_CALL kernel_release(void *context, void *memory) {
    const LhdcKernelMemoryContext *owner = context;
    ExFreePoolWithTag(memory, owner->tag);
}

/* Create instance host callbacks without a mutable global allocator/shared cursor. */
NTSTATUS NTAPI lhdc_kernel_memory_allocator_init(LhdcAllocator *allocator,
                                                 LhdcKernelMemoryContext *context, ULONG tag) {
    if (!allocator || !context || !tag)
        return STATUS_INVALID_PARAMETER;
    context->tag = tag;
    *allocator = (LhdcAllocator){context, kernel_allocate, kernel_release};
    return STATUS_SUCCESS;
}
