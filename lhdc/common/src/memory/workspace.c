#include "workspace.h"
#include <string.h>

/* Check power-of-two requirements and overflow when rounding sizes upward. */
int lhdc_memory_align_size(size_t bytes, size_t alignment, size_t *result) {
    if (!result || !alignment || (alignment & (alignment - 1)) ||
        bytes > SIZE_MAX - (alignment - 1))
        return -1;
    *result = (bytes + alignment - 1) & ~(alignment - 1);
    return 0;
}

/* Initialize the instance cursor without clearing caller storage. */
int lhdc_memory_arena_init(LhdcMemoryArena *arena, void *memory, size_t bytes) {
    if (!arena || !memory || !bytes || (uintptr_t)memory % LHDC_MEMORY_ALIGNMENT ||
        bytes > UINTPTR_MAX - (uintptr_t)memory || (uintptr_t)arena - (uintptr_t)memory < bytes ||
        (uintptr_t)memory - (uintptr_t)arena < sizeof *arena)
        return -1;
    *arena = (LhdcMemoryArena){memory, bytes, 0};
    return 0;
}

/* Align and allocate a subblock within remaining capacity without implicit growth. */
void *lhdc_memory_arena_take(LhdcMemoryArena *arena, size_t bytes, size_t alignment) {
    size_t address, aligned;
    if (!arena || !arena->base || !bytes || arena->used > arena->capacity ||
        arena->used > UINTPTR_MAX - (uintptr_t)arena->base)
        return NULL;
    address = (uintptr_t)arena->base + arena->used;
    if (lhdc_memory_align_size(address, alignment, &aligned) || aligned < (uintptr_t)arena->base)
        return NULL;
    size_t offset = aligned - (uintptr_t)arena->base;
    if (offset > arena->capacity || bytes > arena->capacity - offset)
        return NULL;
    arena->used = offset + bytes;
    return arena->base + offset;
}

/* Allocate arena storage through a host callback. */
static void *arena_allocate(void *context, size_t bytes) {
    return lhdc_memory_arena_take(context, bytes, LHDC_MEMORY_ALIGNMENT);
}
/* The arena owner releases all subblocks together; individual releases do not change the cursor. */
static void arena_release(void *context, void *memory) {
    (void)context;
    (void)memory;
}
/* Adapt the instance cursor to the generic host interface. */
LhdcAllocator lhdc_memory_arena_allocator(LhdcMemoryArena *arena) {
    return (LhdcAllocator){arena, arena_allocate, arena_release};
}

/* Calculate raw request size including ownership metadata and worst-case alignment slack. */
static size_t allocation_bytes(size_t bytes) {
    const size_t overhead = sizeof(LhdcMemoryBlock) + LHDC_MEMORY_ALIGNMENT - 1;
    return bytes && bytes <= SIZE_MAX - overhead ? bytes + overhead : 0;
}
/* Accumulate arena capacity including alignment between adjacent blocks. */
size_t lhdc_memory_block_bytes(size_t payload_bytes) {
    size_t bytes = allocation_bytes(payload_bytes), aligned;
    return bytes && !lhdc_memory_align_size(bytes, LHDC_MEMORY_ALIGNMENT, &aligned) ? aligned : 0;
}
/* Capture host callbacks and the original address for correct release of an aligned object. */
void *lhdc_memory_alloc(const LhdcAllocator *allocator, size_t bytes) {
    size_t total = allocation_bytes(bytes);
    if (!total)
        return NULL;
    if (!allocator)
        allocator = lhdc_memory_default_allocator();
    if (!allocator || !allocator->allocate || !allocator->release)
        return NULL;
    LhdcAllocator owner = *allocator;
    void *original = owner.allocate(owner.context, total);
    if (!original)
        return NULL;
    uintptr_t raw = (uintptr_t)original;
    if (raw > UINTPTR_MAX - total) {
        owner.release(owner.context, original);
        return NULL;
    }
    uintptr_t aligned = (raw + sizeof(LhdcMemoryBlock) + LHDC_MEMORY_ALIGNMENT - 1) &
                        ~(uintptr_t)(LHDC_MEMORY_ALIGNMENT - 1);
    LhdcMemoryBlock *block = (void *)(aligned - sizeof *block);
    *block = (LhdcMemoryBlock){owner, original};
    return (void *)aligned;
}
/* Reject array-size overflow and explicitly clear memory. */
void *lhdc_memory_calloc(const LhdcAllocator *allocator, size_t count, size_t bytes) {
    if (!count || !bytes || count > SIZE_MAX / bytes)
        return NULL;
    size_t total = count * bytes;
    void *memory = lhdc_memory_alloc(allocator, total);
    if (memory)
        memset(memory, 0, total);
    return memory;
}
/* Read captured ownership before release to avoid accessing freed metadata. */
void lhdc_memory_free(void *memory) {
    if (!memory)
        return;
    LhdcMemoryBlock block = *(LhdcMemoryBlock *)((uint8_t *)memory - sizeof(LhdcMemoryBlock));
    block.allocator.release(block.allocator.context, block.original);
}
