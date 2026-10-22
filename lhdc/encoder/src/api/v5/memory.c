#include "../../v5/include/encoder_internal.h"
#include "../../../include/lhdc/encoder/v5/lhdcv5BT.h"
#include "lhdc_memory.h"

/* Query complete V5 BT instance capacity without allocating. */
size_t lhdcv5BT_workspace_bytes(uint32_t version) {
    uint32_t bytes = 0;
    return v5f_lhdcv5_util_get_mem_req(version, &bytes) ? 0 : bytes;
}

/* Validate capacity/alignment and bind caller workspace; clear the output handle on failure. */
int32_t lhdcv5BT_get_handle_in_memory(uint32_t version, void *memory, size_t bytes,
                                      HANDLE_LHDCV5_BT *handle) {
    if (!handle)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    *handle = NULL;
    size_t required = lhdcv5BT_workspace_bytes(version);
    if (!required || !memory || bytes < required || required > UINT32_MAX ||
        bytes > UINTPTR_MAX - (uintptr_t)memory || (uintptr_t)memory % LHDC_MEMORY_ALIGNMENT)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    int32_t result = v5f_lhdcv5_util_get_handle(version, memory, (uint32_t)required);
    if (!result)
        *handle = memory;
    return result;
}

/* Create one V5 instance through host callbacks, retaining owned storage for destruction. */
int32_t lhdcv5BT_get_handle_with_allocator(uint32_t version, const LhdcAllocator *allocator,
                                           HANDLE_LHDCV5_BT *handle) {
    if (!handle)
        return LHDCV5_FRET_INVALID_INPUT_PARAM;
    *handle = NULL;
    if (version != LHDCV5_VERSION_1)
        return LHDCV5_FRET_ERROR;
    size_t bytes = lhdcv5BT_workspace_bytes(version);
    if (!bytes)
        return LHDCV5_FRET_ERROR;
    void *memory = lhdc_memory_alloc(allocator, bytes);
    if (!memory)
        return LHDCV5_FRET_ERROR;
    int32_t result = lhdcv5BT_get_handle_in_memory(version, memory, bytes, handle);
    if (result) {
        lhdc_memory_free(memory);
        return result;
    }
    ((lhdcv5_cb_t *)memory)->owned_workspace = memory;
    return LHDCV5_FRET_SUCCESS;
}
