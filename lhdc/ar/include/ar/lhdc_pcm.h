/* Standalone stereo PCM effects interface with caller-owned workspace, usable before encoding or after decoding. */
#ifndef LHDC_AR_PCM_H
#define LHDC_AR_PCM_H
#include <stddef.h>
#include <stdint.h>
#include "parameters.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum LhdcArProfile { LHDC_AR_PROFILE_V4 = 4, LHDC_AR_PROFILE_V5 = 5 } LhdcArProfile;
typedef enum LhdcArPcmStatus {
    LHDC_AR_PCM_OK = 0,
    LHDC_AR_PCM_INVALID_ARGUMENT = -1000,
    LHDC_AR_PCM_WORKSPACE_TOO_SMALL = -1001,
    LHDC_AR_PCM_NOT_INITIALIZED = -1002
} LhdcArPcmStatus;
#define LHDC_AR_PCM_WORKSPACE_ALIGNMENT 16u
#define LHDC_AR_PCM_MAX_FRAMES_PER_CALL 256u
/* Little-endian interleaved stereo; 24-bit samples occupy three consecutive bytes, not 32-bit containers. */
typedef struct LhdcArPcmConfig {
    uint32_t sample_rate;
    uint32_t bits_per_sample;
} LhdcArPcmConfig;
/* Fields are implementation-managed after binding; the structure and workspace must not overlap. */
typedef struct LhdcArPcm {
    void *workspace;
    uint32_t workspace_bytes;
    LhdcArProfile profile;
    LhdcArPcmConfig config;
} LhdcArPcm;
/* Query workspace size for an effects version; return zero for unknown versions. */
size_t lhdc_ar_pcm_workspace_bytes(LhdcArProfile profile);
/* Initialize a standalone effects chain without allocating memory or requiring a codec handle. */
int lhdc_ar_pcm_init(LhdcArPcm *context, LhdcArProfile profile, void *workspace,
                     size_t workspace_bytes, const LhdcArPcmConfig *config);
/* Process complete PCM frames in place or into separate output; input and output byte counts match. */
int lhdc_ar_pcm_process(LhdcArPcm *context, const void *input, void *output, size_t pcm_bytes);
/* Apply effects parameters without converting LHDC encoder configuration arrays. */
int lhdc_ar_pcm_set_parameters(LhdcArPcm *context, const LhdcArParameters *parameters);
/* Read effects parameters and current runtime state. */
int lhdc_ar_pcm_get_parameters(LhdcArPcm *context, LhdcArParameters *parameters);
/* Update orientation in tenths of a degree. */
int lhdc_ar_pcm_set_gyro(LhdcArPcm *context, int32_t x, int32_t y, int32_t z);
/* Unbind the instance; memory remains caller-owned. */
void lhdc_ar_pcm_uninit(LhdcArPcm *context);
#ifdef __cplusplus
}
#endif
#endif
