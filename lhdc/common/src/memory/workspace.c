#include "../../include/lhdc_memory.h"

/* Aligns a size through the shared memory module. */
int lhdc_memory_align_size(size_t bytes, size_t alignment, size_t *result) {
    return audio_memory_align_size(bytes, alignment, result);
}

/* Initializes an instance arena through the shared memory module. */
int lhdc_memory_arena_init(LhdcMemoryArena *arena, void *memory, size_t bytes) {
    return audio_memory_arena_init(arena, memory, bytes);
}

/* Takes an aligned sub-block through the shared memory module. */
void *lhdc_memory_arena_take(LhdcMemoryArena *arena, size_t bytes, size_t alignment) {
    return audio_memory_arena_take(arena, bytes, alignment);
}

/* Adapts an instance arena to the allocator interface. */
LhdcAllocator lhdc_memory_arena_allocator(LhdcMemoryArena *arena) {
    return audio_memory_arena_allocator(arena);
}

/* Computes workspace capacity including ownership metadata. */
size_t lhdc_memory_block_bytes(size_t payload_bytes) {
    return audio_memory_block_bytes(payload_bytes);
}

/* Allocates an aligned object after selecting the LHDC default host allocator. */
void *lhdc_memory_alloc(const LhdcAllocator *allocator, size_t bytes) {
    if (!allocator)
        allocator = lhdc_memory_default_allocator();
    return audio_memory_alloc(allocator, bytes);
}

/* Allocates and clears an array after selecting the LHDC default host allocator. */
void *lhdc_memory_calloc(const LhdcAllocator *allocator, size_t count, size_t bytes) {
    if (!allocator)
        allocator = lhdc_memory_default_allocator();
    return audio_memory_calloc(allocator, count, bytes);
}

/* Releases the original allocation using the recorded ownership information. */
void lhdc_memory_free(void *memory) {
    audio_memory_free(memory);
}
