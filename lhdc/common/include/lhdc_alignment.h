#ifndef LHDC_ALIGNMENT_H
#define LHDC_ALIGNMENT_H
#include <stdint.h>
#include <stddef.h>
/* Round up to the target object's natural alignment; alignment must be a power of two. */
static inline size_t lhdc_align_size(size_t bytes, size_t alignment) {
    return (bytes + alignment - 1) & ~(alignment - 1);
}
/* Obtain a partition address aligned for the stored object's natural alignment. */
static inline uint8_t *lhdc_align_pointer(void *address, size_t alignment) {
    return (void *)lhdc_align_size((uintptr_t)address, alignment);
}
/* Round a caller memory address up to a 16-byte boundary. */
static inline uint8_t *lhdc_align16(void *address) {
    return (void *)(((uintptr_t)address + 15) & ~(uintptr_t)15);
}
#endif
