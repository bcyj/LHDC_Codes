/* Actual LLAC common-DCT tests: interleaved instances, reinitialization, formula, and allocation bounds. */
#include "../src/v4/include/llac_init.h"
#include <audio_math/constants.h>
#include "../src/v4/include/llac_decode.h"
#include "../src/v4/include/llac_workspace_directory_view.h"
#include "../src/v4/include/llac_fft.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Report failed test conditions and terminate the process. */
static void check(int ok) {
    if (!ok) {
        fputs("LLAC DCT integration failure\n", stderr);
        exit(2);
    }
}
static uint32_t random_state = 0x38bc6451;
/* Generate bounded fixed-seed input for reproducible instance numeric checks. */
static float next_sample(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return (float)(int32_t)random_state / 2147483648.0f;
}
/* Check both DCT lengths, read-only tables, and workspace tails through actual initialization. */
int main(void) {
    unsigned size = (unsigned)llac_dec_mem_get_size();
    unsigned char *memory[2] = {malloc(size + 32), malloc(size + 32)};
    check(memory[0] != NULL && memory[1] != NULL);
    float input[240], original[240];
    kiss_fft_cpx temporary[120];
    double worst = 0.0, worst_inverse = 0.0;
    unsigned cases = 0;
    for (unsigned round = 0; round < 2; ++round) {
        for (unsigned slot = 0; slot < 2; ++slot) {
            memset(memory[slot], 0xa5, size + 32);
            llac_dec_mem_init(memory[slot]);
            int bytes = 0;
            unsigned samples = 0;
            llac_dec_initial(slot ? 48000 : 44100, 2, 24, 5.0f, 192000, &bytes, &samples,
                             (LLACWorkspaceDirectory *)memory[slot]);
            check(samples == (slot ? 240u : 220u));
        }
        for (unsigned test = 0; test < 32; ++test)
            for (unsigned slot = 0; slot < 2; ++slot) {
                LLACWorkspaceDirectory *directory = (void *)memory[slot];
                LLAC_Dec *decoder = directory->decoder;
                Dct4 *dct = &decoder->channel_setup[test & 1]->dct4structImdct;
                int n = dct->length;
                float tables[480];
                memcpy(tables, dct->twid1, n * sizeof(float));
                memcpy(tables + n, dct->twid2, n * sizeof(float));
                float scale = test % 3 == 0 ? 1e6f : test % 3 == 1 ? 1e-12f : 1.0f;
                for (int i = 0; i < n; ++i)
                    input[i] = original[i] =
                        scale * (test == 0 ? (i == 0 ? 1.0f : 0.0f) : next_sample());
                llac_dct4_transform(dct, input, temporary);
                double power = 0.0, error = 0.0;
                for (int k = 0; k < n; ++k) {
                    double expected = 0.0;
                    for (int i = 0; i < n; ++i)
                        expected += original[i] * cos(AUDIO_MATH_PI_F64 / n * (i + .5) * (k + .5));
                    expected *= sqrt(2.0 / n);
                    double delta = input[k] - expected;
                    power += expected * expected;
                    error += delta * delta;
                }
                double relative = sqrt(error / power);
                if (relative > worst)
                    worst = relative;
                check(relative < 1e-5);
                llac_dct4_transform(dct, input, temporary);
                power = error = 0.0;
                for (int i = 0; i < n; ++i) {
                    double delta = input[i] - original[i];
                    power += (double)original[i] * original[i];
                    error += delta * delta;
                }
                relative = sqrt(error / power);
                if (relative > worst_inverse)
                    worst_inverse = relative;
                check(relative < 1e-5);
                check(!memcmp(tables, dct->twid1, n * sizeof(float)) &&
                      !memcmp(tables + n, dct->twid2, n * sizeof(float)));
                for (unsigned i = size; i < size + 32; ++i)
                    check(memory[slot][i] == 0xa5);
                ++cases;
            }
    }
    free(memory[0]);
    free(memory[1]);
    printf(
        "{\"cases\":%u,\"instances\":2,\"workspace_bytes\":%u,\"relative_l2\":%.12g,\"self_inverse_l2\":%.12g,\"guards\":\"passed\"}\n",
        cases, size, worst, worst_inverse);
    return 0;
}
