#include "lhdc/encoder/v4/lhdcBT.h"
#include "lhdc/encoder/v5/lhdcv5BT.h"
#include "lhdc_memory.h"
#if LHDC_TEST_DECODER
#include "lhdc/decoder/v4.h"
#include "lhdc/decoder/v5.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "memory contract line %d: %s\n", __LINE__, #condition);                \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
typedef struct HostMemory {
    unsigned requests, releases, live, fail_at;
} HostMemory;
/* Optional final-bitstream records belong only to tests; production instances share no files or mutable state. */
static FILE *encoded_records;
/* Simulate host allocations without 16-byte base alignment and verify original-address release. */
static void *allocate(void *context, size_t bytes) {
    HostMemory *host = context;
    ++host->requests;
    if (host->fail_at == host->requests)
        return NULL;
    uint8_t *base = malloc(bytes + 1);
    if (!base)
        return NULL;
    ++host->live;
    return base + 1;
}
/* Count host release callbacks and release corresponding allocations. */
static void release(void *context, void *memory) {
    HostMemory *host = context;
    ++host->releases;
    --host->live;
    free((uint8_t *)memory - 1);
}
/* Check instance-arena alignment, exhaustion, reset, and address boundaries. */
static int arena_contract(void) {
    uint8_t storage[512 + 15];
    uint8_t *base = (void *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
    LhdcMemoryArena arena;
    size_t value = 99;
    REQUIRE(lhdc_memory_align_size(SIZE_MAX, 16, &value) != 0 && value == 99);
    REQUIRE(lhdc_memory_align_size(10, 3, &value) != 0 && value == 99);
    REQUIRE(lhdc_memory_arena_init(&arena, base + 1, 256) != 0);
    REQUIRE(lhdc_memory_arena_init((void *)base, base, 256) != 0);
    REQUIRE(lhdc_memory_arena_init(&arena, base, 256) == 0);
    REQUIRE(lhdc_memory_arena_take(&arena, 1, 16) == base);
    REQUIRE(lhdc_memory_arena_take(&arena, 16, 16) == base + 16);
    REQUIRE(arena.used == 32);
    REQUIRE(!lhdc_memory_arena_take(&arena, SIZE_MAX, 16) && arena.used == 32);
    REQUIRE(!lhdc_memory_arena_take(&arena, 1, 3) && arena.used == 32);
    HostMemory host = {0};
    LhdcAllocator allocator = {&host, allocate, release};
    REQUIRE(!lhdc_memory_calloc(&allocator, SIZE_MAX, 2));
    REQUIRE(!lhdc_memory_alloc(&allocator, SIZE_MAX) && !host.requests);
    void *memory = lhdc_memory_calloc(&allocator, 5, 7);
    REQUIRE(memory && (uintptr_t)memory % 16 == 0);
    for (size_t i = 0; i < 35; ++i)
        REQUIRE(!((uint8_t *)memory)[i]);
    /* Callbacks are captured by value; changing the caller's temporary table does not change release ownership. */
    allocator.release = NULL;
    lhdc_memory_free(memory);
    REQUIRE(host.requests == 1 && host.releases == 1 && !host.live);
    return 0;
}
/* Verify borrowed encoder workspaces, host allocation, failure rollback, and paired release. */
static int encoder_contract(int version, uint32_t rate, uint32_t bits, uint32_t quality) {
    HostMemory host = {0};
    LhdcAllocator allocator = {&host, allocate, release};
    size_t bytes = version == 5 ? lhdcv5BT_workspace_bytes(1) : lhdcBT_workspace_bytes(version);
    REQUIRE(bytes > 0);
    uint8_t *raw = malloc(bytes + 16 + 32);
    REQUIRE(raw);
    uint8_t *memory = (void *)(((uintptr_t)raw + 15) & ~(uintptr_t)15);
    memset(memory, 0xa5, bytes + 32);
    void *borrowed = NULL, *owned = NULL;
    if (version == 5) {
        REQUIRE(lhdcv5BT_get_handle_in_memory(1, memory, bytes - 1, &borrowed) != 0 && !borrowed);
        REQUIRE(lhdcv5BT_get_handle_in_memory(1, memory + 1, bytes, &borrowed) != 0 && !borrowed);
    } else {
        REQUIRE(!lhdcBT_get_handle_in_memory(version, memory, bytes - 1));
        REQUIRE(!lhdcBT_get_handle_in_memory(version, memory + 1, bytes));
    }
    for (size_t i = 0; i < bytes + 32; ++i)
        REQUIRE(memory[i] == 0xa5);
    host.fail_at = 1;
    if (version == 5)
        REQUIRE(lhdcv5BT_get_handle_with_allocator(1, &allocator, &owned) != 0 && !owned);
    else
        REQUIRE(!lhdcBT_get_handle_with_allocator(version, &allocator));
    REQUIRE(!host.live);
    host = (HostMemory){0};
    if (version == 5) {
        REQUIRE(!lhdcv5BT_get_handle_with_allocator(1, &allocator, &owned));
        REQUIRE(!lhdcv5BT_get_handle_in_memory(1, memory, bytes, &borrowed));
    } else {
        owned = lhdcBT_get_handle_with_allocator(version, &allocator);
        borrowed = lhdcBT_get_handle_in_memory(version, memory, bytes);
    }
    REQUIRE(owned && borrowed && host.requests == 1 && host.live == 1);
    uint8_t pcm[8192], input[2][8192], output[2][16384];
    for (size_t i = 0; i < sizeof pcm; ++i)
        pcm[i] = (uint8_t)(i * 29 + (i >> 5));
    void *handles[] = {owned, borrowed};
    for (int side = 0; side < 2; ++side) {
        int status = version == 5
                         ? lhdcv5BT_init_encoder(handles[side], rate, bits, quality, 660, 20)
                         : lhdcBT_init_encoder(handles[side], rate, bits, quality, 0, 0, 660, 20);
        REQUIRE(!status);
    }
    unsigned packets = 0;
    for (int frame = 0; frame < 24; ++frame) {
        uint32_t written[2] = {0}, frames[2] = {0};
        int result[2];
        for (int side = 0; side < 2; ++side) {
            memcpy(input[side], pcm, sizeof pcm);
            memset(output[side], 0, sizeof output[side]);
            if (version == 5)
                result[side] = lhdcv5BT_encode(
                    handles[side], input[side], (rate == 96000 ? 480 : 240) * 2 * (bits / 8),
                    output[side], sizeof output[side], &written[side], &frames[side]);
            else if (version == 1) {
                result[side] = lhdcBT_encode(handles[side], input[side], output[side]);
                REQUIRE(result[side] >= 0);
                written[side] = (uint32_t)result[side];
            } else
                result[side] = lhdcBT_encodeV3(handles[side], input[side], output[side],
                                               &written[side], &frames[side]);
        }
        REQUIRE(result[0] == result[1] && written[0] == written[1] && frames[0] == frames[1]);
        REQUIRE(!memcmp(output[0], output[1], written[0]));
        if (encoded_records) {
            uint32_t header[] = {
                (uint32_t)version,   rate,       bits,     quality, (uint32_t)frame,
                (uint32_t)result[0], written[0], frames[0]};
            REQUIRE(fwrite(header, sizeof header, 1, encoded_records) == 1);
            REQUIRE(fwrite(output[0], 1, written[0], encoded_records) == written[0]);
        }
        packets += written[0] != 0;
    }
    REQUIRE(packets && host.requests == 1 && !host.releases);
    for (int side = 0; side < 2; ++side) {
        int status = version == 5
                         ? lhdcv5BT_init_encoder(handles[side], rate, bits, quality, 660, 20)
                         : lhdcBT_init_encoder(handles[side], rate, bits, quality, 0, 0, 660, 20);
        REQUIRE(!status);
        if (version == 5)
            REQUIRE(!lhdcv5BT_free_handle(handles[side]));
        else
            lhdcBT_free_handle(handles[side]);
    }
    REQUIRE(host.requests == 1 && host.releases == 1 && !host.live);
    for (size_t i = bytes; i < bytes + 32; ++i)
        REQUIRE(memory[i] == 0xa5);
    free(raw);
    return 0;
}
/* Inject failures at each standalone compatibility-constructor stage and check all subblocks are reclaimed. */
static int failure_contract(void) {
    for (unsigned fail = 1; fail <= 5; ++fail) {
        HostMemory host = {0, 0, 0, fail};
        LhdcAllocator allocator = {&host, allocate, release};
        REQUIRE(!v4f_lhdc_encoder_new_with_allocator(3, &allocator));
        REQUIRE(!host.live);
    }
    for (unsigned fail = 1; fail <= 3; ++fail) {
        HostMemory host = {0, 0, 0, fail};
        LhdcAllocator allocator = {&host, allocate, release};
        REQUIRE(!v4f_llac_encoder_new_with_allocator(&allocator));
        REQUIRE(!host.live);
#if LHDC_TEST_ENCODER_WITH_AR
        host = (HostMemory){0, 0, 0, fail};
        REQUIRE(!v4f_ar_process_new_with_allocator(&allocator));
        REQUIRE(!host.live);
#endif
    }
    return 0;
}
#if LHDC_TEST_DECODER
/* Check rejected decoder bindings leave undersized/misaligned storage untouched and valid initialization preserves guards. */
static int decoder_contract(void) {
    const uint16_t versions[] = {200, 300, 400, 500, 550};
    for (size_t i = 0; i < sizeof versions / sizeof versions[0]; ++i) {
        uint16_t version = versions[i];
        uint32_t bytes = 0;
        REQUIRE(!(version == 550 ? lhdcv5_util_dec_get_mem_req(version, 48000, 3, 0, &bytes)
                                 : (int32_t)lhdc_util_dec_get_mem_req(version, &bytes)));
        uint8_t *raw = malloc((size_t)bytes + 16 + 32);
        REQUIRE(raw);
        uint8_t *memory = (void *)(((uintptr_t)raw + 15) & ~(uintptr_t)15);
        memset(memory, 0xa5, (size_t)bytes + 32);
        if (version == 550) {
            REQUIRE(lhdcv5_util_init_decoder_in_memory(memory, bytes - 1, 16, 48000, 3, 50, 0,
                                                       version) == LHDC_DEC_WORKSPACE_TOO_SMALL);
            REQUIRE(lhdcv5_util_init_decoder_in_memory(memory + 1, bytes, 16, 48000, 3, 50, 0,
                                                       version) == LHDC_DEC_INVALID_ARGUMENT);
        } else {
            REQUIRE(lhdc_util_init_decoder_in_memory(memory, bytes - 1, 16, 48000, 400000,
                                                     version) == LHDC_DEC_WORKSPACE_TOO_SMALL);
            REQUIRE(lhdc_util_init_decoder_in_memory(memory + 1, bytes, 16, 48000, 400000,
                                                     version) == LHDC_DEC_INVALID_ARGUMENT);
        }
        for (size_t j = 0; j < (size_t)bytes + 32; ++j)
            REQUIRE(memory[j] == 0xa5);
        REQUIRE(
            !(version == 550
                  ? lhdcv5_util_init_decoder_in_memory(memory, bytes, 16, 48000, 3, 50, 0, version)
                  : lhdc_util_init_decoder_in_memory(memory, bytes, 16, 48000, 400000, version)));
        if (version == 550)
            REQUIRE(!lhdcv5_util_dec_destroy(memory));
        else
            lhdcDestroy((void *)memory);
        for (size_t j = bytes; j < (size_t)bytes + 32; ++j)
            REQUIRE(memory[j] == 0xa5);
        free(raw);
    }
    return 0;
}
#endif
/* Verify codec/AR host allocators, external workspace, and failure rollback. */
int main(int argc, char **argv) {
    if (argc > 1) {
        encoded_records = fopen(argv[1], "wb");
        REQUIRE(encoded_records);
    }
    if (argc > 2)
        REQUIRE(lhdc_memory_default_allocator() == NULL);
    REQUIRE(!arena_contract());
    REQUIRE(!failure_contract());
#if LHDC_TEST_DECODER
    REQUIRE(!decoder_contract());
#endif
    const uint32_t rates[] = {44100, 48000, 96000};
    for (int version = 1; version <= 5; ++version)
        for (size_t rate = 0; rate < sizeof rates / sizeof rates[0]; ++rate)
            for (uint32_t bits = 16; bits <= 24; bits += 8)
                for (uint32_t quality = 5; quality <= 7; quality += 2)
                    REQUIRE(!encoder_contract(version, rates[rate], bits, quality));
    HostMemory host = {0};
    LhdcAllocator allocator = {&host, allocate, release};
    REQUIRE(!lhdcBT_get_handle_with_allocator(5, &allocator) && !host.requests);
    if (encoded_records)
        REQUIRE(fclose(encoded_records) == 0);
    puts("PASS: bounded workspace, host callbacks, OOM, borrowed/owned lifetime, process reuse");
    return 0;
}
