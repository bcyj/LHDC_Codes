#ifndef LHDC_MEMORY_WORKSPACE_PRIVATE_H
#define LHDC_MEMORY_WORKSPACE_PRIVATE_H
#include "../../include/lhdc_memory.h"
/* Private ownership record preceding each aligned object. */
typedef struct LhdcMemoryBlock {
    LhdcAllocator allocator;
    void *original;
} LhdcMemoryBlock;
#endif
