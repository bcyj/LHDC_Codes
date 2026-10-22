#include "../../include/ar/lhdc_pcm.h"
#include "../../include/ar/compat/legacy.h"
#include <limits.h>
#include <string.h>

/* Check whether the effects context is successfully bound to workspace. */
static int initialized(const LhdcArPcm *context) {
    return context && context->workspace &&
           (context->profile == LHDC_AR_PROFILE_V4 || context->profile == LHDC_AR_PROFILE_V5);
}

/* Query effects-chain workspace independently of encoder frame length or handles. */
size_t lhdc_ar_pcm_workspace_bytes(LhdcArProfile profile) {
    switch (profile) {
    case LHDC_AR_PROFILE_V4:
        return v4f_LhdcArMemBytesReq();
    case LHDC_AR_PROFILE_V5:
        return v5f_LhdcArMemBytesReq();
    default:
        return 0;
    }
}

/* Reset all caller workspace, then initialize the specified effects version. */
int lhdc_ar_pcm_init(LhdcArPcm *context, LhdcArProfile profile, void *workspace,
                     size_t workspace_bytes, const LhdcArPcmConfig *config) {
    if (!context || !workspace || !config ||
        ((uintptr_t)workspace % LHDC_AR_PCM_WORKSPACE_ALIGNMENT) != 0 ||
        (config->bits_per_sample != 16 && config->bits_per_sample != 24 &&
         config->bits_per_sample != 32) ||
        config->sample_rate > INT32_MAX)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    size_t required = lhdc_ar_pcm_workspace_bytes(profile);
    if (!required)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    if (workspace_bytes < required)
        return LHDC_AR_PCM_WORKSPACE_TOO_SMALL;
    if ((uintptr_t)context - (uintptr_t)workspace < required ||
        (uintptr_t)workspace - (uintptr_t)context < sizeof *context)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    LhdcArPcmConfig format = *config;
    memset(context, 0, sizeof *context);
    memset(workspace, 0, required);
    int result;
    if (profile == LHDC_AR_PROFILE_V4)
        result = v4f_InitLhdcAR(2, (int32_t)format.bits_per_sample, (int32_t)format.sample_rate, 2,
                                (int32_t)format.bits_per_sample, workspace, (uint32_t)required);
    else
        result = v5f_InitLhdcAR(2, (int32_t)format.bits_per_sample, (int32_t)format.sample_rate, 2,
                                (int32_t)format.bits_per_sample, workspace, (uint32_t)required);
    if (result)
        return result;
    context->workspace = workspace;
    context->workspace_bytes = (uint32_t)required;
    context->profile = profile;
    context->config = format;
    return LHDC_AR_PCM_OK;
}

/* Process stereo PCM with the per-call sample limit matching underlying queue capacity. */
int lhdc_ar_pcm_process(LhdcArPcm *context, const void *input, void *output, size_t pcm_bytes) {
    if (!initialized(context))
        return LHDC_AR_PCM_NOT_INITIALIZED;
    size_t frame_bytes = 2u * (context->config.bits_per_sample / 8u);
    if (!input || !output || !frame_bytes || pcm_bytes % frame_bytes ||
        pcm_bytes / frame_bytes > LHDC_AR_PCM_MAX_FRAMES_PER_CALL)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    if (context->profile == LHDC_AR_PROFILE_V4)
        return v4f_ProcessLhdcAR(output, (uint8_t *)input, (lhdc_ulong_t)pcm_bytes,
                                 context->workspace, context->workspace_bytes);
    return v5f_ProcessLhdcAR(output, (uint8_t *)input, (lhdc_ulong_t)pcm_bytes, context->workspace,
                             context->workspace_bytes);
}

/* Write complete effects parameters independently of encoder extension settings. */
int lhdc_ar_pcm_set_parameters(LhdcArPcm *context, const LhdcArParameters *parameters) {
    if (!initialized(context))
        return LHDC_AR_PCM_NOT_INITIALIZED;
    if (!parameters)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    if (context->profile == LHDC_AR_PROFILE_V4)
        return v4f_SetARParams((uint8_t *)parameters, sizeof *parameters, context->workspace,
                               context->workspace_bytes);
    return v5f_SetARParams((uint8_t *)parameters, sizeof *parameters, context->workspace,
                           context->workspace_bytes);
}

/* Read back effects parameters and state. */
int lhdc_ar_pcm_get_parameters(LhdcArPcm *context, LhdcArParameters *parameters) {
    if (!initialized(context))
        return LHDC_AR_PCM_NOT_INITIALIZED;
    if (!parameters)
        return LHDC_AR_PCM_INVALID_ARGUMENT;
    if (context->profile == LHDC_AR_PROFILE_V4)
        return v4f_GetARParams((uint8_t *)parameters, sizeof *parameters, context->workspace,
                               context->workspace_bytes);
    return v5f_GetARParams((uint8_t *)parameters, sizeof *parameters, context->workspace,
                           context->workspace_bytes);
}

/* Forward orientation directly in tenths of a degree. */
int lhdc_ar_pcm_set_gyro(LhdcArPcm *context, int32_t x, int32_t y, int32_t z) {
    if (!initialized(context))
        return LHDC_AR_PCM_NOT_INITIALIZED;
    if (context->profile == LHDC_AR_PROFILE_V4)
        return v4f_SetGyroPosToLhdcAR(x, y, z, context->workspace, context->workspace_bytes);
    return v5f_SetGyroPosToLhdcAR(x, y, z, context->workspace, context->workspace_bytes);
}

/* Unbind workspace without releasing caller-owned memory. */
void lhdc_ar_pcm_uninit(LhdcArPcm *context) {
    if (context)
        memset(context, 0, sizeof *context);
}
