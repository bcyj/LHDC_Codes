/* Verify native full-workspace layout, queried sizes, and allocation bounds for every public parameter combination. */
#include "decoder_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if TEST_V4
#include "../src/v4/include/llac_init.h"
#include "../src/v4/include/lhdc_dec_core.h"
#else
#include "../src/v5/include/lhdc_workspace_layout.h"
#endif
/* Report structure sizes for exact same-ABI comparison and verify initialization stays within queried capacity. */
int main(void) {
#if TEST_V4
    uint32_t required = 0;
    if (lhdc_util_dec_get_mem_req(400, &required))
        return 1;
    printf(
        "{\"pointer_bits\":%u,\"wrapper\":%u,\"core\":%u,\"llac\":%u,\"setup\":%u,\"fft_handle\":%u,\"required_48k\":%u}\n",
        (unsigned)sizeof(void *) * 8, (unsigned)sizeof(LhdcV4Decoder),
        (unsigned)sizeof(lhdc_Decoder_t), (unsigned)llac_dec_mem_get_size(),
        (unsigned)sizeof(DecSetup), (unsigned)sizeof(T_IIS_FFT), required);
#else
    static const int rates[] = {8000, 16000, 24000, 32000, 44100, 48000, 96000, 192000};
    static const int durations[] = {25, 50, 75, 100};
    for (int channels = 1; channels <= 8; ++channels)
        for (unsigned r = 0; r < 8; ++r)
            for (unsigned d = 0; d < 4; ++d) {
                int size = 0;
                if (lhdc_v5_dec_workspace_get_size(channels, rates[r], durations[d], 0, &size) ||
                    size <= 0)
                    return 2;
                unsigned char *memory = malloc((size_t)size + 32);
                if (!memory)
                    return 3;
                memset(memory, 0x5a, (size_t)size + 32);
                if (lhdc_v5_dec_workspace_init(channels, rates[r], durations[d], 0, memory + 16))
                    return 4;
                for (unsigned i = 0; i < 16; ++i)
                    if (memory[i] != 0x5a || memory[size + 16 + i] != 0x5a)
                        return 5;
                free(memory);
            }
    uint32_t required = 0;
    if (lhdcv5_util_dec_get_mem_req(550, 48000, 0, 0, &required))
        return 6;
    int state_bytes = lhdc_dec_lossy_start(NULL, 0, 0, 0, 0, 0, 0), workspace_bytes = 0;
    if (state_bytes != (int)(sizeof(tdata_all_buffer_struct) + sizeof(LHDCV5_TimeWorkspace)))
        return 7;
    if (lhdc_v5_dec_workspace_get_size(2, 48000, 50, 0, &workspace_bytes))
        return 8;
    unsigned char *storage = malloc((size_t)workspace_bytes + 32);
    if (!storage)
        return 9;
    memset(storage, 0x5a, (size_t)workspace_bytes + 32);
    if (lhdc_v5_dec_workspace_init(2, 48000, 50, 0, storage + 16))
        return 10;
    if (lhdc_dec_lossy_start(storage + 16, state_bytes - 1, 2, 16, 48000, 50, 2000) != -1)
        return 11;
    if (lhdc_dec_lossy_start(storage + 16, state_bytes, 2, 16, 48000, 50, 2000) != state_bytes)
        return 12;
    if (lhdc_dec_lossy_start(storage + 16, state_bytes + 16, 2, 16, 48000, 50, 2000) != state_bytes)
        return 13;
    for (unsigned i = 0; i < 16; ++i)
        if (storage[i] != 0x5a || storage[workspace_bytes + 16 + i] != 0x5a)
            return 14;
    free(storage);
    printf(
        "{\"pointer_bits\":%u,\"wrapper\":%u,\"prefix\":%u,\"time\":%u,\"arithmetic\":%u,\"fft\":%u,\"required_48k\":%u,\"workspace_guard_cases\":256,\"lossy_state\":%u,\"lossy_bounds_cases\":4}\n",
        (unsigned)sizeof(void *) * 8, (unsigned)sizeof(LhdcV5Decoder),
        (unsigned)sizeof(LHDCV5_WorkspacePrefix), (unsigned)sizeof(LHDCV5_TimeWorkspace),
        (unsigned)sizeof(LHDCV5_ArithmeticWorkspace), (unsigned)sizeof(kiss_fft_state), required,
        (unsigned)state_bytes);
#endif
    return 0;
}
