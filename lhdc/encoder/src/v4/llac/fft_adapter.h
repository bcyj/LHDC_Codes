#ifndef LHDC_ENCODER_LLAC_FFT_ADAPTER_H
#define LHDC_ENCODER_LLAC_FFT_ADAPTER_H
#include "../include/types/lhdc_native_types.h"
#include "../../../../common/include/dsp/fft_float32.h"

/* Validate instance plan/workspace descriptors without inferring external memory capacity. */
static inline IIS_FFT_ERROR llac_fft_plan_status(HANDLE_IIS_FFT handle) {
    if (!handle)
        return IIS_FFT_INTERNAL_ERROR;
    if (handle->iisfft.length < 2 || handle->iisfft.length != handle->len)
        return IIS_FFT_LENGTH_ERROR;
    if (!handle->kiss_input || !handle->kiss_output || !handle->kiss_twiddles || !handle->scratch)
        return IIS_FFT_MEMORY_ERROR;
    return IIS_FFT_NO_ERROR;
}

/* Process complex data already packed into instance input; callers validate plans and gather input first. */
static inline void llac_fft_run_prepared(HANDLE_IIS_FFT handle) {
    const LhdcKissFftPlan plan = {handle->len, handle->sign == IIS_FFT_BWD, handle->kiss_twiddles,
                                  1, handle->scratch->radix};
    lhdc_kiss_fft_work(handle->kiss_output, handle->kiss_input, 1, 1, handle->kiss_factors, &plan);
}

#endif
