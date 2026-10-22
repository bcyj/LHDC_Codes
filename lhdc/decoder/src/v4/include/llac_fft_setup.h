#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_FFT_SETUP_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_FFT_SETUP_H
#include <stdint.h>
#include "llac_workspace_directory_view.h"
#include <stddef.h>
#include "../../../include/lhdc/decoder/status.h"
#include "../../../../common/include/types/lhdc_dsp_types.h"

typedef kiss_fft_cpx Complex;
/* DCT pre-rotation uses complex storage first; window mixing overwrites it with PCM without extra instance capacity. */
typedef union LhdcLlacPcmScratch {
    float samples[240];
    kiss_fft_cpx packed[120];
} LhdcLlacPcmScratch;
typedef struct Iisfft {
    float *scratch;
    int *scratch2;
    int length, sign, num_factors, factors[10], isPrime[10];
} Iisfft;
typedef struct Cfft {
    int len, sign;
    float *table;
} Cfft;
typedef struct T_IIS_FFT {
    int8_t sign;
    int len;
    float *buffer, *sine_table;
    Iisfft iisfft;
    Cfft cfft;
} T_IIS_FFT;
typedef T_IIS_FFT *HANDLE_IIS_FFT;
typedef struct Fft {
    int length;
    void *handle;
} Fft;
typedef struct Dct4 {
    int length;
    Complex *twid1, *twid2;
    Fft fft;
} Dct4;
uint8_t llac_fft_plan_bind(HANDLE_IIS_FFT *handle, int length, int8_t sign,
                           LLACWorkspaceDirectory *directory);
void llac_fft_init(Fft *fft, int length, LLACWorkspaceDirectory *directory);
void llac_dct4_init(Dct4 *dct, int length, LLACWorkspaceDirectory *directory);

/* Clear the handle's transform length and caller pointer, preserving other handle data. */
static inline uint8_t llac_fft_plan_reset(HANDLE_IIS_FFT *handle) {
    if (!(handle && *handle)) {
        return LHDC_DEC_OK;
    }
    (*handle)->iisfft.length = 0;
    *handle = NULL;
    return LHDC_DEC_OK;
}

/* Finish the FFT wrapper and clear its internal length; a null wrapper pointer is allowed. */
static inline void llac_fft_reset(Fft *fft) {
    if (fft) {
        HANDLE_IIS_FFT fft_handle = fft->handle;
        (void)llac_fft_plan_reset(&fft_handle);
        fft->length = 0;
        fft->handle = NULL;
    }
}

/* Finish the DCT-IV wrapper, clearing both factor references and FFT state; null arguments are allowed. */
static inline void llac_dct4_reset(Dct4 *dct) {
    if (dct) {
        llac_fft_reset(&dct->fft);
        dct->length = 0;
        dct->twid1 = NULL;
        dct->twid2 = NULL;
    }
}

#endif
