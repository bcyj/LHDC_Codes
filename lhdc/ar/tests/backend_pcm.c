#include <lhdc/ar/pcm_backend.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Compare PCM byte-for-byte across the generic interface, default backend, and both explicit LHDC parameter sets. */
int main(void) {
    const uint32_t rates[] = {44100, 48000, 88200, 96000};
    const uint32_t depths[] = {16, 24, 32};
    const size_t frames[] = {1, 17, 64, 128, 240, 256};
    unsigned configurations = 0, calls = 0;
    if (lhdc_ar_pcm_backend((LhdcArProfile)0))
        return 1;
#if AR_HAS_DEFAULT_BACKEND
    if (ar_pcm_default_backend() != lhdc_ar_pcm_backend(LHDC_AR_PROFILE_V5))
        return 1;
#endif
    for (int v = 4; v <= 5; ++v) {
        LhdcArProfile profile = (LhdcArProfile)v;
        const ArPcmBackend *backend = lhdc_ar_pcm_backend(profile);
        size_t legacy_bytes = lhdc_ar_pcm_workspace_bytes(profile);
        for (unsigned r = 0; r < 4; ++r) {
            for (unsigned d = 0; d < 3; ++d) {
                ArPcmConfig config = {rates[r], depths[d], 2};
                LhdcArPcmConfig legacy_config = {rates[r], depths[d]};
                size_t bytes = ar_pcm_workspace_bytes(backend, &config);
                void *raw = malloc(bytes + 16), *old = malloc(legacy_bytes + 16);
                if (!raw || !old)
                    return 2;
                void *workspace = (void *)(((uintptr_t)raw + 15) & ~(uintptr_t)15);
                void *reference = (void *)(((uintptr_t)old + 15) & ~(uintptr_t)15);
                ArPcm context = {0};
                LhdcArPcm legacy = {0};
                const ArPcmBackend *selected = backend;
#if AR_HAS_DEFAULT_BACKEND
                if (v == 5)
                    selected = NULL;
#endif
                if (ar_pcm_init(&context, selected, workspace, bytes, &config) ||
                    lhdc_ar_pcm_init(&legacy, profile, reference, legacy_bytes, &legacy_config))
                    return 3;
                ++configurations;
                unsigned char input[2048], output[2048], expected[2048];
                uint32_t seed = 7;
                for (unsigned frame = 0; frame < 36; ++frame) {
                    size_t count = frames[frame % 6] * 2 * (depths[d] / 8);
                    for (size_t i = 0; i < count; ++i) {
                        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
                        input[i] = (unsigned char)(seed >> 24);
                    }
                    if (frame == 6 || frame == 18) {
                        LhdcArParameters parameters;
                        if (ar_pcm_get_parameters(&context, &parameters, sizeof parameters))
                            return 4;
                        parameters.fLhdcAr_reverb_gain = .3f;
                        parameters.fLhdcAr_3d_downmix_gain = .7f;
                        if (ar_pcm_set_parameters(&context, &parameters, sizeof parameters) ||
                            lhdc_ar_pcm_set_parameters(&legacy, &parameters) ||
                            ar_pcm_set_orientation(&context, 300, 0, 0) ||
                            lhdc_ar_pcm_set_gyro(&legacy, 300, 0, 0))
                            return 5;
                    }
                    if (lhdc_ar_pcm_process(&legacy, input, expected, count))
                        return 6;
                    if (frame & 1) {
                        memcpy(output, input, count);
                        if (ar_pcm_process(&context, output, output, count))
                            return 7;
                    } else if (ar_pcm_process(&context, input, output, count))
                        return 8;
                    if (memcmp(output, expected, count))
                        return 9;
                    ++calls;
                }
                ar_pcm_uninit(&context);
                lhdc_ar_pcm_uninit(&legacy);
                free(raw);
                free(old);
            }
        }
    }
    printf("generic_backend configurations=%u PCM_calls=%u identical\n", configurations, calls);
    return 0;
}
