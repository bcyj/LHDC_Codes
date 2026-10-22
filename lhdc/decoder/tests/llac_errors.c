#include "../src/v4/include/llac_memory.h"
#include "../src/v4/include/llac_decode.h"
#include "../include/lhdc/decoder/v4.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "decoder error contract line %d: %s\n", __LINE__, #condition);         \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

/* Verify LLAC decoder errors for null pointers and invalid initialization parameters. */
int main(void) {
    LLACWorkspaceDirectory *memory = malloc(llac_dec_mem_get_size());
    REQUIRE(memory);
    llac_dec_mem_init(memory);
    int bytes = 99;
    unsigned samples = 99;
    REQUIRE(llac_dec_initial(96000, 2, 16, 5, 192000, &bytes, &samples, memory) ==
            LHDC_LLAC_DEC_UNSUPPORTED_RATE);
    REQUIRE(bytes == 0 && samples == 0);
    REQUIRE(llac_dec_initial(48000, 3, 16, 5, 192000, &bytes, &samples, memory) ==
            LHDC_LLAC_DEC_UNSUPPORTED_CHANNELS);
    REQUIRE(llac_dec_initial(48000, 2, 16, 3, 192000, &bytes, &samples, memory) ==
            LHDC_LLAC_DEC_UNSUPPORTED_DURATION);
    REQUIRE(llac_dec_initial(48000, 2, 16, 5, 0, &bytes, &samples, memory) ==
            LHDC_LLAC_DEC_INVALID_BITRATE);
    REQUIRE(llac_dec_initial(48000, 2, 16, 5, 192000, &bytes, &samples, memory) == LHDC_DEC_OK);
    REQUIRE(samples == 240 && bytes > 0);
    uint8_t input[4096] = {8, 0};
    int pcm[480];
    memset(pcm, 0xa5, sizeof pcm);
    REQUIRE(llac_dec_process(input, pcm, memory) == -LHDC_LLAC_DEC_INVALID_BYTE_BUDGET);
    for (unsigned i = 0; i < sizeof pcm; ++i)
        REQUIRE(((uint8_t *)pcm)[i] == 0xa5);
    REQUIRE(llac_dec_process(NULL, pcm, memory) == -LHDC_LLAC_DEC_INVALID_POINTER);
    free(memory);
    uint32_t required;
    REQUIRE(lhdc_util_dec_get_mem_req(500, &required) == LHDC_DEC_OK);
    uint8_t *raw = malloc(required + 15);
    REQUIRE(raw);
    void *aligned = (void *)(((uintptr_t)raw + 15) & ~(uintptr_t)15);
    REQUIRE(lhdc_util_init_decoder_in_memory(aligned, required, 16, 96000, 192000, 500) ==
            LHDC_DEC_INVALID_ARGUMENT);
    lhdcInit(aligned, 16, 96000, 192000, 500);
    REQUIRE(lhdcDecodeProcess(aligned, (void *)pcm, input, sizeof input) == 0);
    REQUIRE(lhdc_util_init_decoder_in_memory(aligned, required, 16, 48000, 192000, 500) ==
            LHDC_DEC_OK);
    free(raw);
    puts("PASS decoder recoverable errors and reinitialization");
    return 0;
}
