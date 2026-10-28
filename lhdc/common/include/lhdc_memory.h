#ifndef LHDC_MEMORY_H
#define LHDC_MEMORY_H
#include "lhdc_resources.h"
#include <audio_common/memory.h>
#include <stddef.h>
#include <stdint.h>
#define LHDC_MEMORY_CALL AUDIO_MEMORY_CALL
#ifdef __cplusplus
extern "C" {
#endif
#define LHDC_MEMORY_ALIGNMENT AUDIO_MEMORY_ALIGNMENT
/* Preserve the LHDC API names while sharing the allocator and arena implementation. */
typedef AudioAllocator LhdcAllocator;
typedef AudioMemoryArena LhdcMemoryArena;
/* Computes an aligned size with overflow checks; alignment must be a power of two. */
int LHDC_MEMORY_CALL lhdc_memory_align_size(size_t bytes, size_t alignment, size_t *result);
/* Binds an aligned workspace; the descriptor must not overlap the supplied memory. */
int LHDC_MEMORY_CALL lhdc_memory_arena_init(LhdcMemoryArena *arena, void *memory, size_t bytes);
/* Takes aligned storage without advancing the cursor on failure. */
void *LHDC_MEMORY_CALL lhdc_memory_arena_take(LhdcMemoryArena *arena, size_t bytes,
                                              size_t alignment);
/* Creates an allocator backed by an arena; release does not reclaim sub-blocks. */
LhdcAllocator LHDC_MEMORY_CALL lhdc_memory_arena_allocator(LhdcMemoryArena *arena);
/* Returns the aligned storage required for a payload and its ownership metadata. */
size_t LHDC_MEMORY_CALL lhdc_memory_block_bytes(size_t payload_bytes);
/* Allocates a 16-byte-aligned object and records the supplied host allocator. */
void *LHDC_MEMORY_CALL lhdc_memory_alloc(const LhdcAllocator *allocator, size_t bytes);
/* Allocates and clears an array with overflow checks. */
void *LHDC_MEMORY_CALL lhdc_memory_calloc(const LhdcAllocator *allocator, size_t count,
                                          size_t bytes);
/* Releases storage through the host callback captured at allocation time. */
void LHDC_MEMORY_CALL lhdc_memory_free(void *memory);
/* Returns the user-mode default allocator, or NULL in external-allocator-only builds. */
const LhdcAllocator *LHDC_MEMORY_CALL lhdc_memory_default_allocator(void);
#ifdef __cplusplus
}
#endif
#endif
