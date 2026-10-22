/* Generic PCM backend contract; independent of codec formats, algorithm parameters, and host allocators. */
#ifndef AR_BACKEND_H
#define AR_BACKEND_H
#include <stddef.h>
#include <stdint.h>

typedef struct ArPcmConfig {
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    uint32_t channels;
} ArPcmConfig;

typedef enum ArPcmStatus {
    AR_PCM_OK = 0,
    AR_PCM_INVALID_ARGUMENT = -1000,
    AR_PCM_WORKSPACE_TOO_SMALL = -1001,
    AR_PCM_NOT_INITIALIZED = -1002,
    AR_PCM_NO_BACKEND = -1003,
    AR_PCM_UNSUPPORTED = -1004
} ArPcmStatus;

typedef struct ArPcmBackend ArPcmBackend;
/* Descriptors are read-only and must outlive all instances. Mutable state resides in caller-owned workspace.
 * Parameter layouts are backend-defined; the public interface passes only addresses and byte counts. Angles use tenths of a degree.
 * The backend must release acquired resources if init fails; process supports in-place or separate output. */
struct ArPcmBackend {
    size_t workspace_alignment;
    const void *userdata;
    size_t (*workspace_bytes)(const ArPcmBackend *, const ArPcmConfig *);
    int (*init)(const ArPcmBackend *, void *, size_t, const ArPcmConfig *);
    int (*process)(void *, const void *, void *, size_t);
    int (*set_parameters)(void *, const void *, size_t);
    int (*get_parameters)(void *, void *, size_t);
    int (*set_orientation)(void *, int32_t, int32_t, int32_t);
    void (*uninit)(void *);
};
#endif
