#include <ar/lhdc_pcm.h>
#include <ar/compat/legacy.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Compare generic and underlying PCM interfaces with equal independent workspaces, covering variable blocks and invalid input. */
static int test_pcm_interface(void) {
    unsigned configurations = 0;
    unsigned compared_frames = 0;
    const uint32_t rates[] = {44100, 48000, 88200, 96000};
    const uint32_t depths[] = {16, 24, 32};
    const size_t frames[] = {1, 17, 64, 128, 240, 256};
    if (lhdc_ar_pcm_workspace_bytes((LhdcArProfile)0) != 0)
        return 10;
    for (int version = 4; version <= 5; ++version) {
        LhdcArProfile profile = (LhdcArProfile)version;
        size_t bytes = lhdc_ar_pcm_workspace_bytes(profile);
        uint8_t *first = calloc(1, bytes + 32);
        uint8_t *second = calloc(1, bytes + 32);
        if (!first || !second) {
            free(first);
            free(second);
            return 11;
        }
        uint8_t *actual_workspace = (void *)(((uintptr_t)first + 15) & ~(uintptr_t)15);
        uint8_t *reference_workspace = (void *)(((uintptr_t)second + 15) & ~(uintptr_t)15);
        for (unsigned rate = 0; rate < sizeof rates / sizeof *rates; ++rate) {
            for (unsigned depth = 0; depth < sizeof depths / sizeof *depths; ++depth) {
                LhdcArPcm context = {0};
                LhdcArPcmConfig config = {rates[rate], depths[depth]};
                if (lhdc_ar_pcm_init(&context, profile, actual_workspace + 1, bytes, &config) !=
                        LHDC_AR_PCM_INVALID_ARGUMENT ||
                    lhdc_ar_pcm_init(&context, profile, actual_workspace, bytes - 1, &config) !=
                        LHDC_AR_PCM_WORKSPACE_TOO_SMALL) {
                    free(first);
                    free(second);
                    return 17;
                }
                memset(reference_workspace, 0, bytes);
                int expected = version == 4
                                   ? v4f_InitLhdcAR(2, depths[depth], rates[rate], 2, depths[depth],
                                                    reference_workspace, (uint32_t)bytes)
                                   : v5f_InitLhdcAR(2, depths[depth], rates[rate], 2, depths[depth],
                                                    reference_workspace, (uint32_t)bytes);
                int actual = lhdc_ar_pcm_init(&context, profile, actual_workspace, bytes, &config);
                if (actual != expected) {
                    free(first);
                    free(second);
                    return 12;
                }
                if (actual)
                    continue;
                ++configurations;
                uint8_t input[2048], output[2080], reference[2080];
                uint32_t seed = 7;
                for (unsigned frame = 0; frame < 18; ++frame) {
                    size_t count = frames[frame % 6] * 2u * (depths[depth] / 8u);
                    for (size_t i = 0; i < count; ++i) {
                        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
                        input[i] = (uint8_t)(seed >> 24);
                    }
                    memset(output, 0x6b, sizeof output);
                    memset(reference, 0x6b, sizeof reference);
                    if (lhdc_ar_pcm_process(&context, input, output, 3) !=
                        LHDC_AR_PCM_INVALID_ARGUMENT) {
                        free(first);
                        free(second);
                        return 13;
                    }
                    expected = version == 4
                                   ? v4f_ProcessLhdcAR(reference, input, (lhdc_ulong_t)count,
                                                       reference_workspace, (uint32_t)bytes)
                                   : v5f_ProcessLhdcAR(reference, input, (lhdc_ulong_t)count,
                                                       reference_workspace, (uint32_t)bytes);
                    if (frame & 1) {
                        memcpy(output, input, count);
                        actual = lhdc_ar_pcm_process(&context, output, output, count);
                    } else {
                        actual = lhdc_ar_pcm_process(&context, input, output, count);
                    }
                    if (actual != expected || memcmp(output, reference, sizeof output)) {
                        free(first);
                        free(second);
                        return 14;
                    }
                    ++compared_frames;
                    if (frame == 5 || frame == 11) {
                        LhdcArParameters parameters;
                        actual = lhdc_ar_pcm_get_parameters(&context, &parameters);
                        parameters.fLhdcAr_3d_downmix_gain = 0.7f;
                        parameters.fLhdcAr_reverb_gain = 0.3f;
                        expected = version == 4
                                       ? v4f_SetARParams((uint8_t *)&parameters, sizeof parameters,
                                                         reference_workspace, (uint32_t)bytes)
                                       : v5f_SetARParams((uint8_t *)&parameters, sizeof parameters,
                                                         reference_workspace, (uint32_t)bytes);
                        actual |= lhdc_ar_pcm_set_parameters(&context, &parameters);
                        int angle = frame == 5 ? 300 : -200;
                        expected |= version == 4
                                        ? v4f_SetGyroPosToLhdcAR(angle, 0, 0, reference_workspace,
                                                                 (uint32_t)bytes)
                                        : v5f_SetGyroPosToLhdcAR(angle, 0, 0, reference_workspace,
                                                                 (uint32_t)bytes);
                        actual |= lhdc_ar_pcm_set_gyro(&context, angle, 0, 0);
                        if (actual != expected) {
                            free(first);
                            free(second);
                            return 18;
                        }
                    }
                }
                LhdcArParameters parameters;
                if (lhdc_ar_pcm_get_parameters(&context, &parameters) ||
                    lhdc_ar_pcm_set_parameters(&context, &parameters) ||
                    lhdc_ar_pcm_set_gyro(&context, 100, 0, 0)) {
                    free(first);
                    free(second);
                    return 15;
                }
                lhdc_ar_pcm_uninit(&context);
                if (lhdc_ar_pcm_process(&context, input, output, 0) !=
                    LHDC_AR_PCM_NOT_INITIALIZED) {
                    free(first);
                    free(second);
                    return 16;
                }
            }
        }
        free(first);
        free(second);
    }
    printf("pcm_common configurations=%u compared_frames=%u\n", configurations, compared_frames);
    return 0;
}
/* Verify AR PCM block processing, sample formats, and parameter error returns. */
int main(void) { return test_pcm_interface(); }
