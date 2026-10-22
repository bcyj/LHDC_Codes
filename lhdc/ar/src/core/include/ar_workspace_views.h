

#ifndef LHDC_AR_SRC_CORE_INCLUDE_AR_WORKSPACE_VIEWS_H
#define LHDC_AR_SRC_CORE_INCLUDE_AR_WORKSPACE_VIEWS_H
#include "../../../include/ar/types.h"
#include "../../../../../math/include/audio_math/fft_float32.h"
#include <stddef.h>

/* Read-only IIR coefficients and order; the caller separately owns history and input/output buffers. */
typedef struct LHDCIirPlan {
    const double *denominator;
    const double *numerator;
    int32_t order;
} LHDCIirPlan;

typedef struct LHDCUpmixDirectory {
    struct upmix_process_parameter *process_parameter;
    double *iir_filter_in_buf;
    double *iir_filter_out_buf;
    float *temp_buf_area_1;
    float *temp_buf_area_2;
    double *comp_filter_z_buf;
    double *shlv_filter_z_buf;
    float *bank_result;
    float *pre_power_value_buf;
    uint8_t *protect_area;
} LHDCUpmixDirectory;

typedef struct LHDCUpmixStorage {
    struct upmix_process_parameter process_parameter;
    double iir_filter_in_buf[256];
    double iir_filter_out_buf[256];
    float temp_buf_area_1[512];
    float temp_buf_area_2[1024];
    double comp_filter_z_buf[72];
    double shlv_filter_z_buf[8];
    float bank_result[2048];
    float pre_power_value_buf[4];
    uint8_t protect_area[8];
} LHDCUpmixStorage;
typedef struct LHDCUpmixWorkspace {
    LHDCUpmixDirectory directory;
    LHDCUpmixStorage storage;
} LHDCUpmixWorkspace;

typedef struct LHDCConvolutionChannel {
    cc_buf_list *buf_list;
    cc_buf *list_mem;
    float *data_mem;
} LHDCConvolutionChannel;
typedef struct LHDCConvolutionDirectory {
    float *fft_data;
    float *input_history[3];
    LHDCConvolutionChannel channel[3];
    float *sum[2];
    AudioMathFft512Work *fft_work;
} LHDCConvolutionDirectory;
typedef struct LHDCHrirDirectory {
    float *fft_data;
    float *input_history[3];
    float *kernel;
    float *sum[2];
    AudioMathFft512Work *fft_work;
} LHDCHrirDirectory;

/* Carve a byte-sized partition from dynamic workspace; not for addressing structure fields. */
static inline void *lhdc_workspace_take(uint8_t *base, uint32_t *cursor, uint32_t bytes) {
    void *result = base + *cursor;
    *cursor += bytes;
    return result;
}
/* Round a 32-bit length up to an 8-byte boundary. */
static inline uint32_t lhdc_workspace_align8(uint32_t size_bytes) {
    return (size_bytes + 7) & ~7u;
}
#endif
