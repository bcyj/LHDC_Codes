#include "../../../include/lhdc/decoder/v5.h"
#include "../../../../common/include/lhdc_memory.h"

/* Validate capacity, alignment, and format before binding V5; rejected parameters leave workspace unchanged. */
int32_t lhdcv5_util_init_decoder_in_memory(void *memory, size_t bytes, uint32_t bits, uint32_t rate,
                                           uint32_t format, uint32_t duration, uint32_t lossless,
                                           uint16_t version) {
    uint32_t required = 0;
    if (!memory || (uintptr_t)memory % LHDC_MEMORY_ALIGNMENT ||
        bytes > UINTPTR_MAX - (uintptr_t)memory || (bits != 16 && bits != 24 && bits != 32) ||
        duration != 50 || lossless != 0 ||
        lhdcv5_util_dec_get_mem_req(version, rate, format, lossless, &required) != LHDC_DEC_OK)
        return LHDC_DEC_INVALID_ARGUMENT;
    if (bytes < required)
        return LHDC_DEC_WORKSPACE_TOO_SMALL;
    return lhdcv5_util_init_decoder(memory, bits, rate, format, duration, lossless, version);
}
