#ifndef LHDC_MEMORY_H
#define LHDC_MEMORY_H
#include "lhdc_resources.h"
#include <stddef.h>
#include <stdint.h>
#if defined(_MSC_VER)
#define LHDC_MEMORY_CALL __cdecl
#elif defined(_WIN32) && defined(__i386__)
#define LHDC_MEMORY_CALL __attribute__((cdecl))
#else
#define LHDC_MEMORY_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define LHDC_MEMORY_ALIGNMENT 16u
/* The host supplies memory; context must outlive all associated instances. Callbacks return NULL on failure. */
typedef struct LhdcAllocator {
    void *context;
    void *(LHDC_MEMORY_CALL *allocate)(void *context, size_t bytes);
    void(LHDC_MEMORY_CALL *release)(void *context, void *memory);
} LhdcAllocator;
/* Sequential initialization-time partitions with per-instance cursors; no individual-block release. */
typedef struct LhdcMemoryArena {
    uint8_t *base;
    size_t capacity;
    size_t used;
} LhdcMemoryArena;
/* Check aligned-size calculation; leave the result unchanged on failure. alignment must be a power of two. */
int LHDC_MEMORY_CALL lhdc_memory_align_size(size_t bytes, size_t alignment, size_t *result);
/* Bind aligned workspace; arena and allocated storage must not overlap. Leave arena unchanged on failure. */
int LHDC_MEMORY_CALL lhdc_memory_arena_init(LhdcMemoryArena *arena, void *memory, size_t bytes);
/* Allocate aligned space without advancing the cursor on failure. */
void *LHDC_MEMORY_CALL lhdc_memory_arena_take(LhdcMemoryArena *arena, size_t bytes,
                                              size_t alignment);
/* Create an arena-backed allocator interface; release does not reclaim individual blocks. */
LhdcAllocator LHDC_MEMORY_CALL lhdc_memory_arena_allocator(LhdcMemoryArena *arena);
/* Query payload capacity including allocation metadata and arena alignment; return zero on overflow. */
size_t LHDC_MEMORY_CALL lhdc_memory_block_bytes(size_t payload_bytes);
/* Allocate through the specified host, copy callbacks, and return a 16-byte-aligned object; NULL selects the platform default. */
void *LHDC_MEMORY_CALL lhdc_memory_alloc(const LhdcAllocator *allocator, size_t bytes);
/* Check array allocation size and clear allocated memory. */
void *LHDC_MEMORY_CALL lhdc_memory_calloc(const LhdcAllocator *allocator, size_t count,
                                          size_t bytes);
/* Release the original address through captured allocation callbacks; NULL is allowed. */
void LHDC_MEMORY_CALL lhdc_memory_free(void *memory);
/* Default user-mode allocator; external-only builds return NULL and do not import CRT heap functions. */
const LhdcAllocator *LHDC_MEMORY_CALL lhdc_memory_default_allocator(void);
#ifdef __cplusplus
}
#endif
#endif
