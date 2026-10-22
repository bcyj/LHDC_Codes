/* Generic allocation-free PCM interface. The integration supplies a default backend, or each instance selects one explicitly. */
#ifndef AR_PCM_H
#define AR_PCM_H
#include "backend.h"
#ifndef AR_HAS_DEFAULT_BACKEND
#define AR_HAS_DEFAULT_BACKEND 0
#endif
#ifdef __cplusplus
extern "C" {
#endif
#if AR_HAS_DEFAULT_BACKEND
#ifndef AR_DEFAULT_BACKEND_PROVIDER
#define AR_DEFAULT_BACKEND_PROVIDER ar_pcm_default_backend_impl
#endif
const ArPcmBackend *AR_DEFAULT_BACKEND_PROVIDER(void);
/* Obtain the default backend through the integration-selected provider, avoiding competing definitions of one exported symbol. */
static inline const ArPcmBackend *ar_pcm_default_backend(void) {
    return AR_DEFAULT_BACKEND_PROVIDER();
}
#else
/* Standalone builds without a default backend accept only an explicitly supplied backend. */
static inline const ArPcmBackend *ar_pcm_default_backend(void) { return NULL; }
#endif
#ifdef __cplusplus
}
#endif

typedef struct ArPcm {
    void *workspace;
    size_t workspace_bytes;
    const ArPcmBackend *backend;
    ArPcmConfig config;
} ArPcm;

/* Validate generic little-endian interleaved integer PCM without imposing algorithm-specific sample rates or block lengths. */
static inline int ar_pcm_config_valid(const ArPcmConfig *config) {
    return config && config->sample_rate && config->channels &&
           (config->bits_per_sample == 16 || config->bits_per_sample == 24 ||
            config->bits_per_sample == 32) &&
           config->channels <= SIZE_MAX / (config->bits_per_sample / 8u);
}

/* Select the default or specified backend and query workspace for the format; return zero if unsupported. */
static inline size_t ar_pcm_workspace_bytes(const ArPcmBackend *backend,
                                            const ArPcmConfig *config) {
    if (!backend)
        backend = ar_pcm_default_backend();
    if (!ar_pcm_config_valid(config) || !backend || !backend->workspace_bytes)
        return 0;
    return backend->workspace_bytes(backend, config);
}

/* Bind caller-owned workspace and initialize the backend; leave no usable context on failure. */
static inline int ar_pcm_init(ArPcm *context, const ArPcmBackend *backend,
                              void *workspace, size_t bytes, const ArPcmConfig *config) {
    if (!context || !workspace || !ar_pcm_config_valid(config))
        return AR_PCM_INVALID_ARGUMENT;
    if (!backend)
        backend = ar_pcm_default_backend();
    if (!backend)
        return AR_PCM_NO_BACKEND;
    if (!backend->init || !backend->process || !backend->workspace_bytes ||
        !backend->workspace_alignment ||
        (backend->workspace_alignment & (backend->workspace_alignment - 1)) ||
        (uintptr_t)workspace % backend->workspace_alignment)
        return AR_PCM_INVALID_ARGUMENT;
    size_t required = backend->workspace_bytes(backend, config);
    if (!required)
        return AR_PCM_UNSUPPORTED;
    if (bytes < required)
        return AR_PCM_WORKSPACE_TOO_SMALL;
    uintptr_t start = (uintptr_t)workspace, ctx = (uintptr_t)context;
    if (bytes > UINTPTR_MAX - start || sizeof *context > UINTPTR_MAX - ctx ||
        (ctx < start + bytes && start < ctx + sizeof *context))
        return AR_PCM_INVALID_ARGUMENT;
    ArPcmConfig format = *config;
    context->workspace = NULL;
    context->workspace_bytes = 0;
    context->backend = NULL;
    int status = backend->init(backend, workspace, bytes, &format);
    if (status)
        return status;
    context->workspace = workspace;
    context->workspace_bytes = bytes;
    context->backend = backend;
    context->config = format;
    return AR_PCM_OK;
}

/* Pass complete PCM frames to the instance backend, preserving its status and output-byte conventions. */
static inline int ar_pcm_process(ArPcm *context, const void *input, void *output, size_t bytes) {
    if (!context || !context->backend || !context->workspace)
        return AR_PCM_NOT_INITIALIZED;
    if ((bytes && (!input || !output)) ||
        bytes % ((size_t)context->config.channels * (context->config.bits_per_sample / 8u)))
        return AR_PCM_INVALID_ARGUMENT;
    if (!bytes)
        return AR_PCM_OK;
    return context->backend->process(context->workspace, input, output, bytes);
}

/* Supply a backend-specific parameter block without interpreting its layout. */
static inline int ar_pcm_set_parameters(ArPcm *context, const void *parameters, size_t bytes) {
    if (!context || !context->backend || !context->workspace)
        return AR_PCM_NOT_INITIALIZED;
    if (!parameters)
        return AR_PCM_INVALID_ARGUMENT;
    return context->backend->set_parameters
               ? context->backend->set_parameters(context->workspace, parameters, bytes)
               : AR_PCM_UNSUPPORTED;
}

/* Read a backend-specific parameter block; the backend validates capacity. */
static inline int ar_pcm_get_parameters(ArPcm *context, void *parameters, size_t bytes) {
    if (!context || !context->backend || !context->workspace)
        return AR_PCM_NOT_INITIALIZED;
    if (!parameters)
        return AR_PCM_INVALID_ARGUMENT;
    return context->backend->get_parameters
               ? context->backend->get_parameters(context->workspace, parameters, bytes)
               : AR_PCM_UNSUPPORTED;
}

/* Pass angles to a backend that supports orientation control. */
static inline int ar_pcm_set_orientation(ArPcm *context, int32_t x, int32_t y, int32_t z) {
    if (!context || !context->backend || !context->workspace)
        return AR_PCM_NOT_INITIALIZED;
    return context->backend->set_orientation
               ? context->backend->set_orientation(context->workspace, x, y, z)
               : AR_PCM_UNSUPPORTED;
}

/* Release backend resources and unbind the instance; workspace remains caller-owned. */
static inline void ar_pcm_uninit(ArPcm *context) {
    if (!context)
        return;
    if (context->workspace && context->backend && context->backend->uninit)
        context->backend->uninit(context->workspace);
    context->workspace = NULL;
    context->workspace_bytes = 0;
    context->backend = NULL;
}
#endif
