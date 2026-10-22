#include "../../v4/include/encoder_internal.h"
#include "../../../include/lhdc/encoder/v4/lhdcBT.h"
#include "lhdc_memory.h"
#include <string.h>

/* Query total instance capacity using the same subblock requests as constructors. */
size_t lhdcBT_workspace_bytes(int32_t version) {
    if (version < 1 || version > 4)
        return 0;
    size_t prefix;
    if (lhdc_memory_align_size(sizeof(LhdcMemoryArena), LHDC_MEMORY_ALIGNMENT, &prefix))
        return 0;
    size_t bytes = prefix + lhdc_memory_block_bytes(sizeof(lhdc_cb_t));
#if LHDC_ENCODER_WITH_AR
    if (version >= 3)
        bytes += v4f_ar_process_workspace_bytes();
#endif
    if (version == 4) {
        bytes += lhdc_memory_block_bytes(sizeof(llac_para_t));
        bytes += lhdc_memory_block_bytes(v4f_llac_enc_workspace_bytes());
        bytes += lhdc_memory_block_bytes(LHDC_V4_INPUT_FIFO_BYTES);
    } else {
        size_t fft_bytes = 0;
        v4f_kiss_fftr_alloc(LHDC_V4_ANALYSIS_FFT_MAX_POINTS, 0, NULL, &fft_bytes);
        bytes += lhdc_memory_block_bytes(sizeof(lhdc_para_t));
        bytes += lhdc_memory_block_bytes(sizeof(FFT_BLOCK));
        bytes += lhdc_memory_block_bytes(fft_bytes);
        bytes += lhdc_memory_block_bytes(sizeof(lhdc_Encoder_s));
        if (version >= 2)
            bytes += lhdc_memory_block_bytes(LHDC_V4_INPUT_FIFO_BYTES);
    }
    return bytes;
}

/* Create a borrowed instance in one caller workspace; destruction does not release the whole block. */
HANDLE_LHDC_BT lhdcBT_get_handle_in_memory(int32_t version, void *memory, size_t bytes) {
    size_t required = lhdcBT_workspace_bytes(version), prefix;
    if (!required || !memory || bytes < required || bytes > UINTPTR_MAX - (uintptr_t)memory ||
        (uintptr_t)memory % LHDC_MEMORY_ALIGNMENT ||
        lhdc_memory_align_size(sizeof(LhdcMemoryArena), LHDC_MEMORY_ALIGNMENT, &prefix))
        return NULL;
    LhdcMemoryArena *arena = memory;
    if (lhdc_memory_arena_init(arena, (uint8_t *)memory + prefix, bytes - prefix))
        return NULL;
    LhdcAllocator allocator = lhdc_memory_arena_allocator(arena);
    lhdc_cb_t *control = lhdc_memory_calloc(&allocator, 1, sizeof *control);
    if (!control)
        return NULL;
#if LHDC_ENCODER_WITH_AR
    if (version >= 3) {
        control->ar_filter = v4f_ar_process_new_with_allocator(&allocator);
        if (!control->ar_filter) {
            lhdc_memory_free(control);
            return NULL;
        }
    }
#endif
    if (version == 4) {
        control->enc_type = ENC_TYPE_LLAC;
        control->enc.llac = v4f_llac_encoder_new_with_allocator(&allocator);
    } else {
        control->enc_type = ENC_TYPE_LHDC;
        control->enc.lhdc = v4f_lhdc_encoder_new_with_allocator(version, &allocator);
    }
    if (!control->enc.lhdc) {
        v4f_ar_process_free(control->ar_filter);
        lhdc_memory_free(control);
        return NULL;
    }
    return control;
}

/* Allocate one complete instance through the host, capture ownership, and release the whole block on failure. */
HANDLE_LHDC_BT lhdcBT_get_handle_with_allocator(int32_t version, const LhdcAllocator *allocator) {
    size_t bytes = lhdcBT_workspace_bytes(version);
    if (!bytes)
        return NULL;
    void *memory = lhdc_memory_alloc(allocator, bytes);
    if (!memory)
        return NULL;
    lhdc_cb_t *control = lhdcBT_get_handle_in_memory(version, memory, bytes);
    if (!control) {
        lhdc_memory_free(memory);
        return NULL;
    }
    control->owned_workspace = memory;
    return control;
}
