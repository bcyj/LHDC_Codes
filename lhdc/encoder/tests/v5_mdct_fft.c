/* Verify selectable FFTs against an independent double DCT-IV formula, including PCM/FFT scratch reuse and history. */
#include "../src/v5/include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../src/v5/include/codec/codec_internal.h"
#include "../src/v5/include/codec/lhdcv5_workspace_types.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t seed = 0x4ac58123;
/* Generate reproducible noise samples. */
static uint32_t noise(void) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}
/* Abort failed checks and identify the test line. */
static void check_at(int ok, int line) {
    if (!ok) {
        fprintf(stderr, "V5 MDCT FFT test failed at line %d\n", line);
        exit(2);
    }
}
#define check(x) check_at((x), __LINE__)
/* Verify both backends' DCT error, workspace boundaries, and consecutive-frame overlap history. */
int main(void) {
    const int configs[][2] = {{120, 96}, {240, 96}, {480, 120}, {480, 192}, {960, 240}, {960, 384}};
    double worst = 0.0;
    unsigned cases = 0;
    for (unsigned config = 0; config < sizeof configs / sizeof *configs; ++config) {
        int n = configs[config][0], overlap = configs[config][1];
        size_t bytes = (size_t)v5f_mdct_get_size();
        unsigned char *memory = malloc(bytes + 32);
        check(memory != NULL);
        mdct_buf_s *buf = calloc(1, sizeof *buf);
        mdct_ch *history = calloc(1, sizeof *history);
        kiss_fft_cpx *tables = calloc(3 * 480, sizeof *tables);
        float *window = calloc(384, sizeof *window);
        float *output = calloc(960, sizeof *output), *saved = calloc(960, sizeof *saved);
        double *matrix = malloc((size_t)n * n * sizeof *matrix);
        check(buf && history && tables && window && output && saved && matrix);
        for (int k = 0; k < n; ++k)
            for (int j = 0; j < n; ++j)
                matrix[(size_t)k * n + j] = audio_math_cos(AUDIO_MATH_PI_F64 / n * (j + 0.5) * (k + 0.5));
        for (int bits = 16; bits <= 24; bits += 8) {
            memset(memory, 0xa5, bytes + 32);
            LHDCV5_MdctWorkspace *workspace = (void *)(memory + 16);
            check(v5f_mdct_get_size() == (int32_t)sizeof *workspace);
            check(v5f_mdct_init(workspace, 2 * n, overlap, 31 - bits, buf, tables, tables + 480,
                                window, tables + 960) == 0);
            workspace->mdct.ch = history;
            const float full = (float)((1u << (bits - 1)) - 1u);
            for (unsigned signal = 0; signal < 8; ++signal) {
                memset(history, 0, sizeof *history);
                for (unsigned frame = 0; frame < 3; ++frame) {
                    for (int j = 0; j < n; ++j) {
                        float x = (float)((int32_t)(noise() & ((1u << bits) - 1u)) -
                                          (int32_t)(1u << (bits - 1)));
                        if (signal == 0)
                            x = 0.0f;
                        if (signal == 1)
                            x = (j == (int)(frame * 17)) ? full : 0.0f;
                        if (signal == 2)
                            x = (j & 1) ? full : -full;
                        if (signal == 3)
                            x = full;
                        if (signal == 4)
                            x = (float)(full * audio_math_sin((j + frame * n) * 0.173));
                        if (signal == 5)
                            x = (j & 1) ? 1.0f : -1.0f;
                        if (signal == 6)
                            x = (j & 1) ? -0.001f : 0.001f;
                        saved[j] = buf->pcm_in[j] = x;
                    }
                    v5f_do_mdct(workspace, 0, buf->pcm_in, output);
                    for (int j = 0; j < overlap; ++j)
                        check(history->overlap[j] == saved[n - overlap + j]);
                    double error = 0.0, power = 0.0;
                    for (int k = 0; k < n; ++k) {
                        double reference = 0.0;
                        for (int j = 0; j < n; ++j)
                            reference += (double)buf->dct4[j] * matrix[(size_t)k * n + j];
                        reference *= ldexp(1.0, 31 - bits) / (n / 2);
                        double d = (double)output[k] - reference;
                        check(isfinite(output[k]));
                        error += d * d;
                        power += reference * reference;
                    }
                    double relative = power ? sqrt(error / power) : sqrt(error);
                    if (relative > worst)
                        worst = relative;
                    check(relative < 8e-6);
                    for (unsigned j = 0; j < 16; ++j)
                        check(memory[j] == 0xa5 && memory[bytes + 16 + j] == 0xa5);
                    ++cases;
                }
            }
        }
        free(matrix);
        free(saved);
        free(output);
        free(window);
        free(tables);
        free(history);
        free(buf);
        free(memory);
    }
    printf(
        "{\"fixed_fft\":%d,\"cases\":%u,\"max_relative_l2_vs_dct4\":%.12g,\"mdct_bytes\":%d,\"extra_bytes\":%u}\n",
        LHDC_V5_ENCODER_FIXED_FFT, cases, worst, v5f_mdct_get_size(),
        (unsigned)LHDC_V5_ENCODER_FIXED_FFT_EXTRA_BYTES);
    return 0;
}
