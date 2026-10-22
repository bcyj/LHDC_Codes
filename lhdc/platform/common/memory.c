#include "../../common/include/lhdc_memory.h"
#ifndef LHDC_EXTERNAL_ALLOCATOR_ONLY
#define LHDC_EXTERNAL_ALLOCATOR_ONLY 0
#endif
#if LHDC_EXTERNAL_ALLOCATOR_ONLY
/* Driver/embedded hosts must supply allocators explicitly without implicit user-mode CRT heap dependencies. */
const LhdcAllocator *lhdc_memory_default_allocator(void) {
    return NULL;
}
#else
#include <stdlib.h>
/* Obtain uninitialized storage for user mode. */
static void *host_allocate(void *context, size_t bytes) {
    (void)context;
    return malloc(bytes);
}
/* Release the original user-mode heap address. */
static void host_release(void *context, void *memory) {
    (void)context;
    free(memory);
}
/* Return an immutable default callback table without a writable global allocator. */
const LhdcAllocator *lhdc_memory_default_allocator(void) {
    static const LhdcAllocator allocator = {NULL, host_allocate, host_release};
    return &allocator;
}
#endif
