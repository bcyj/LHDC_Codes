#include <lhdc/ar/pcm_backend.h>
#include <string.h>

/* Store the backend instance at the workspace head and align the remaining algorithm storage. */
static size_t engine_offset(void) {
    return (sizeof(LhdcArPcm) + LHDC_AR_PCM_WORKSPACE_ALIGNMENT - 1u) &
           ~(size_t)(LHDC_AR_PCM_WORKSPACE_ALIGNMENT - 1u);
}

/* Query total capacity for the LHDC backend instance and algorithm workspace. */
static size_t workspace_bytes(const ArPcmBackend *backend, const ArPcmConfig *config) {
    if (!ar_pcm_config_valid(config) || config->channels != 2)
        return 0;
    LhdcArProfile profile = *(const LhdcArProfile *)backend->userdata;
    size_t engine = lhdc_ar_pcm_workspace_bytes(profile);
    return engine ? engine_offset() + engine : 0;
}

/* Initialize the specified LHDC effects parameter set from generic PCM configuration. */
static int initialize(const ArPcmBackend *backend, void *workspace, size_t bytes,
                      const ArPcmConfig *config) {
    LhdcArProfile profile = *(const LhdcArProfile *)backend->userdata;
    LhdcArPcmConfig format = {config->sample_rate, config->bits_per_sample};
    LhdcArPcm *instance = workspace;
    memset(instance, 0, sizeof *instance);
    return lhdc_ar_pcm_init(instance, profile, (uint8_t *)workspace + engine_offset(),
                            bytes - engine_offset(), &format);
}

/* Process instance PCM with validated LHDC blocking and output timing. */
static int process(void *workspace, const void *input, void *output, size_t bytes) {
    return lhdc_ar_pcm_process(workspace, input, output, bytes);
}

/* Validate LHDC parameter-block size before writing the instance. */
static int set_parameters(void *workspace, const void *parameters, size_t bytes) {
    if (bytes != sizeof(LhdcArParameters))
        return AR_PCM_INVALID_ARGUMENT;
    return lhdc_ar_pcm_set_parameters(workspace, parameters);
}

/* Validate destination capacity before reading LHDC parameters and current state. */
static int get_parameters(void *workspace, void *parameters, size_t bytes) {
    if (bytes < sizeof(LhdcArParameters))
        return AR_PCM_WORKSPACE_TOO_SMALL;
    return lhdc_ar_pcm_get_parameters(workspace, parameters);
}

/* Forward orientation control in tenths of a degree. */
static int set_orientation(void *workspace, int32_t x, int32_t y, int32_t z) {
    return lhdc_ar_pcm_set_gyro(workspace, x, y, z);
}

/* Unbind the instance without freeing caller memory in the backend. */
static void uninitialize(void *workspace) {
    lhdc_ar_pcm_uninit(workspace);
}

static const LhdcArProfile profiles[2] = {LHDC_AR_PROFILE_V4, LHDC_AR_PROFILE_V5};
static const ArPcmBackend backends[2] = {
    {LHDC_AR_PCM_WORKSPACE_ALIGNMENT, &profiles[0], workspace_bytes, initialize, process,
     set_parameters, get_parameters, set_orientation, uninitialize},
    {LHDC_AR_PCM_WORKSPACE_ALIGNMENT, &profiles[1], workspace_bytes, initialize, process,
     set_parameters, get_parameters, set_orientation, uninitialize}
};

/* Return the read-only backend descriptor for a parameter set; all mutable state belongs to the instance. */
const ArPcmBackend *lhdc_ar_pcm_backend(LhdcArProfile profile) {
    if (profile == LHDC_AR_PROFILE_V4)
        return &backends[0];
    if (profile == LHDC_AR_PROFILE_V5)
        return &backends[1];
    return NULL;
}

/* Default to LHDC V5 effects parameters; explicit selection permits other backends in the same process. */
const ArPcmBackend *lhdc_ar_default_backend(void) {
    return &backends[1];
}
