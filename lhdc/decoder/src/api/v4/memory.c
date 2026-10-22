#include "../../../include/lhdc/decoder/v4.h"
#include "../../../../common/include/lhdc_memory.h"

/* Validate caller workspace and common formats, then bind a V4 instance without taking ownership. */
int32_t lhdc_util_init_decoder_in_memory(void *memory, size_t bytes, uint32_t bits, uint32_t rate,
                                         uint32_t bitrate, uint16_t version) {
    uint32_t required = 0;
    if (!memory || (uintptr_t)memory % LHDC_MEMORY_ALIGNMENT ||
        bytes > UINTPTR_MAX - (uintptr_t)memory || (bits != 16 && bits != 24 && bits != 32) ||
        (rate != 44100 && rate != 48000 && rate != 96000) ||
        lhdc_util_dec_get_mem_req(version, &required) != LHDC_DEC_OK)
        return LHDC_DEC_INVALID_ARGUMENT;
    if (version == 500 && (rate == 96000 || bitrate < 1000))
        return LHDC_DEC_INVALID_ARGUMENT;
    if (bytes < required)
        return LHDC_DEC_WORKSPACE_TOO_SMALL;
    lhdcInit(memory, bits, rate, bitrate, version);
    return LHDC_DEC_OK;
}
