/* Check complete standalone/combined component linking and run PCM encoding/effects lifecycles. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if LHDC_TEST_ENCODER
#include <lhdc/encoder/lhdc_bt.h>
#endif
#if LHDC_TEST_AR
#include <ar/lhdc_pcm.h>
#include <ar/compat/legacy.h>
#include <ar/compat/v4.h>
#include <ar/compat/v5.h>
#include <ar/types.h>
#endif
#if LHDC_TEST_DECODER
#include <lhdc/decoder/decoder.h>
#endif
/* Accumulate a bytewise output digest. */
static uint32_t hash_bytes(const void *data, size_t count) {
    const uint8_t *bytes = data;
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < count; ++i)
        hash = (hash ^ bytes[i]) * UINT32_C(16777619);
    return hash;
}
#if LHDC_TEST_AR
/* Optionally record intermediate float state for intermittent silence cases without changing normal test stdout. */
static void diagnose_ar(const uint8_t *workspace, int frame, const void *pcm, size_t bytes) {
    if (!getenv("LHDC_TEST_AR_DIAGNOSTICS"))
        return;
    const __stLHDC_AR_API_BLK *api = (const void *)workspace;
    float *const *groups[] = {api->gpfPcm, api->gpfPcmUpmix, api->gpfPcmDistEff, api->gpfPcmDst,
                              api->gpfPcmReverb};
    const int counts[] = {2, 4, 4, 2, 3};
    fprintf(stderr, "AR_DIAG frame=%d gain=%g,%g output=%08x", frame, (double)api->gnReverbGain,
            (double)api->gn3dDownMixGain, hash_bytes(pcm, bytes));
    for (int group = 0; group < 5; group++) {
        int nonfinite = 0, nonzero = 0;
        for (int ch = 0; ch < counts[group]; ch++)
            for (int i = 0; i < 512; i++) {
                float sample = groups[group][ch][i];
                nonfinite += !isfinite(sample);
                nonzero += sample != 0;
            }
        fprintf(stderr, " stage%d=%d/%d", group, nonfinite, nonzero);
    }
    fprintf(stderr, "\n");
}
#endif
/* Verify optional codec/AR component lifecycles and output result digests. */
int main(int argc, char **argv) {
#if LHDC_TEST_DECODER
    uint32_t bytes4 = 0, bytes5 = 0;
    if (lhdc_util_dec_get_mem_req(400, &bytes4) ||
        lhdcv5_util_dec_get_mem_req(550, 48000, 0, 0, &bytes5))
        return 30;
    printf("decoder_workspace %u %u\n", bytes4, bytes5);
#endif
    int16_t pcm[1920];
    uint8_t output[16384];
    uint32_t random = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 7;
    for (size_t i = 0; i < sizeof(pcm) / sizeof(pcm[0]); ++i) {
        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
        pcm[i] = (int16_t)(random >> 16);
    }
#if LHDC_TEST_ENCODER
    HANDLE_LHDC_BT v4 = lhdcBT_get_handle(4);
    if (!v4 || lhdcBT_init_encoder(v4, 48000, 16, 2, 0, 0, 660, 10))
        return 10;
#if !LHDC_TEST_ENCODER_WITH_AR
    if (!lhdcBT_set_ext_func_state(v4, LHDCBT_EXT_FUNC_AR, 1, NULL, 0))
        return 18;
#endif
    uint32_t written = 0, frames = 0;
    uint32_t total = 0, digest = 0;
    for (int frame = 0; frame < 16; ++frame) {
        if (lhdcBT_encodeV3(v4, pcm, output, &written, &frames))
            return 11;
        total += written;
        digest = digest * 33u + hash_bytes(output, written);
    }
    if (!total)
        return 16;
    printf("encoder_v4 %u %08x\n", total, digest);
    lhdcBT_free_handle(v4);
    HANDLE_LHDCV5_BT v5 = NULL;
    if (lhdcv5BT_get_handle(1, &v5) || !v5)
        return 12;
    if (lhdcv5BT_init_encoder(v5, 48000, 16, 2, 660, 10))
        return 13;
#if !LHDC_TEST_ENCODER_WITH_AR
    if (!lhdcv5BT_set_ext_func_state(v5, LHDCV5_EXT_FUNC_AR, 1, NULL, 0))
        return 19;
#endif
    total = digest = 0;
    for (int frame = 0; frame < 16; ++frame) {
        if (lhdcv5BT_encode(v5, pcm, 960, output, sizeof(output), &written, &frames))
            return 14;
        total += written;
        digest = digest * 33u + hash_bytes(output, written);
    }
    if (!total)
        return 17;
    printf("encoder_v5 %u %08x\n", total, digest);
    if (lhdcv5BT_free_handle(v5))
        return 15;
#endif
#if LHDC_TEST_AR
    uint32_t bytes = 0;
    if (v5f_ar_process_get_mem_req(&bytes) || !bytes)
        return 20;
    void *storage = calloc(1, bytes + 16);
    if (!storage)
        return 21;
    lhdcv5_ar_para_t *ar = (void *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
    if (v5f_ar_process_new(ar, bytes))
        return 22;
    if (v5f_ar_process_init(ar, 48000, 16, 2, 240))
        return 23;
    int32_t positions[6] = {-30, 30, 0, 0, -110, 110};
    float gains[32] = {0};
    gains[0] = gains[3] = 1.0f;
    for (int channel = 12; channel < 18; ++channel)
        gains[channel] = 1.0f;
    gains[20] = gains[21] = gains[31] = 1.0f;
    gains[22] = 0.01f;
    gains[30] = 0.2f;
    if (v5f_ar_set_cfg(ar, positions, gains, 1))
        return 28;
    int16_t original[480];
    memcpy(original, pcm, sizeof(original));
    if (getenv("LHDC_TEST_AR_DIAGNOSTICS"))
        fprintf(stderr, "AR_INPUT %08x\n", hash_bytes(original, sizeof original));
    uint32_t effect_digest = 0;
    for (int frame = 0; frame < 16; ++frame) {
        memcpy(pcm, original, sizeof(original));
        if (v5f_ar_process_run(ar, (uint8_t *)pcm, 960))
            return 24;
        diagnose_ar(ar->pucLhdcArMem, frame, pcm, 960);
        effect_digest = effect_digest * 33u + hash_bytes(pcm, 960);
    }
    printf("ar_v5 %08x\n", effect_digest);
    if (v5f_ar_process_free(ar))
        return 25;
    free(storage);
    lhdc_ar_para_t *ar4 = v4f_ar_process_new();
    if (!ar4 || v4f_ar_process_init(ar4, 48000, 16, 2, 240))
        return 26;
    if (v4f_ar_set_cfg(ar4, positions, gains, 1))
        return 29;
    effect_digest = 0;
    for (int frame = 0; frame < 16; ++frame) {
        memcpy(pcm, original, sizeof(original));
        if (v4f_ProcessLhdcAR((uint8_t *)output, (uint8_t *)pcm, 960, ar4->pucLhdcArMem,
                              ar4->uiLhdcArMemBytes))
            return 27;
        diagnose_ar(ar4->pucLhdcArMem, frame, output, 960);
        effect_digest = effect_digest * 33u + hash_bytes(output, 960);
    }
    printf("ar_v4 %08x\n", effect_digest);
    v4f_ar_process_free(ar4);
#endif
    return 0;
}
