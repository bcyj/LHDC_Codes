#include <ar/pcm.h>
#include <string.h>

/* Verify dispatch, formats, workspace, and error propagation with a codec-independent mock backend. */
static size_t workspace_bytes(const ArPcmBackend *backend, const ArPcmConfig *config) {
    (void)backend;
    return config->channels == 1 ? 16 : 0;
}
/* Clear mock-backend workspace and report successful initialization. */
static int initialize(const ArPcmBackend *backend, void *workspace, size_t bytes,
                      const ArPcmConfig *config) {
    (void)backend;
    (void)bytes;
    (void)config;
    memset(workspace, 0, 16);
    return 0;
}
/* Count mock-backend calls and copy PCM with overlapping-buffer support. */
static int process(void *workspace, const void *input, void *output, size_t bytes) {
    ++*(unsigned char *)workspace;
    memmove(output, input, bytes);
    return 0;
}
/* Reset the mock-backend call counter. */
static void uninitialize(void *workspace) {
    *(unsigned char *)workspace = 0;
}
static const ArPcmBackend backend = {
    1, NULL, workspace_bytes, initialize, process, NULL, NULL, NULL, uninitialize
};

/* Verify backend dispatch, workspace validation, and error propagation in the generic PCM interface. */
int main(void) {
    ArPcmConfig config = {48000, 16, 1};
    unsigned char storage[16], output[8], input[8] = {1, 2, 3, 4};
    ArPcm context = {0};
    if (ar_pcm_default_backend() || ar_pcm_workspace_bytes(NULL, &config) ||
        ar_pcm_init(&context, NULL, storage, sizeof storage, &config) != AR_PCM_NO_BACKEND ||
        ar_pcm_init(&context, &backend, storage, 15, &config) != AR_PCM_WORKSPACE_TOO_SMALL ||
        ar_pcm_init(&context, &backend, storage, sizeof storage, &config))
        return 1;
    if (ar_pcm_process(&context, input, output, 3) != AR_PCM_INVALID_ARGUMENT ||
        ar_pcm_process(&context, input, output, sizeof input) ||
        memcmp(input, output, sizeof input) || storage[0] != 1 ||
        ar_pcm_set_parameters(&context, input, sizeof input) != AR_PCM_UNSUPPORTED ||
        ar_pcm_set_orientation(&context, 0, 0, 0) != AR_PCM_UNSUPPORTED)
        return 2;
    ar_pcm_uninit(&context);
    if (storage[0] || ar_pcm_process(&context, input, output, 8) != AR_PCM_NOT_INITIALIZED)
        return 3;
    config.channels = 2;
    if (ar_pcm_workspace_bytes(&backend, &config) ||
        ar_pcm_init(&context, &backend, storage, sizeof storage, &config) != AR_PCM_UNSUPPORTED)
        return 4;
    config.bits_per_sample = 20;
    if (ar_pcm_config_valid(&config))
        return 5;
    return 0;
}
