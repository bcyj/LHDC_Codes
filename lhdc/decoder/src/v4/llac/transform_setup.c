#include "../../../include/lhdc/decoder/status.h"
#include "../include/llac_workspace_directory_view.h"
#include "../include/llac_fft_setup.h"
#include "../include/llac_memory.h"
#include "../include/llac_dct_workspace.h"
#include <assert.h>
#include <string.h>

/* Validate transform direction/length and initialize the FFT handle and scratch pointers in the caller directory. */
uint8_t llac_fft_plan_bind(HANDLE_IIS_FFT *handle, int len, int8_t sign,
                           LLACWorkspaceDirectory *directory) {
    if (sign != -1 && sign != 1)
        return LHDC_LLAC_FFT_INVALID_DIRECTION;
    if (len <= 1)
        return LHDC_LLAC_FFT_INVALID_LENGTH;
    T_IIS_FFT *fft_handle = directory->h;
    if (!fft_handle)
        return LHDC_LLAC_FFT_MISSING_WORKSPACE;
    fft_handle->len = len;
    fft_handle->sign = sign;
    memset(&fft_handle->iisfft, 0, sizeof fft_handle->iisfft);
    fft_handle->iisfft.length = len;
    fft_handle->iisfft.sign = sign;
    fft_handle->iisfft.scratch = directory->scratch;
    *handle = fft_handle;
    return LHDC_DEC_OK;
}

/* Initialize an even-length forward FFT wrapper; assert on invalid length or allocation failure. */
void llac_fft_init(Fft *fft, int length, LLACWorkspaceDirectory *directory) {
    assert((length & 1) == 0);
    fft->length = length;
    HANDLE_IIS_FFT handle = NULL;
    uint8_t error = llac_fft_plan_bind(&handle, length, -1, directory);
    assert(error == 0);
    (void)error;
    fft->handle = handle;
}

/* Build an instance common single-precision DCT-IV plan, reusing caller-allocated tables and buffers. */
void llac_dct4_init(Dct4 *dct, int length, LLACWorkspaceDirectory *directory) {
    assert((length & 1) == 0);
    dct->length = length;
    llac_fft_init(&dct->fft, length / 2, directory);
    LhdcLlacDctWorkspace *work = (void *)dct->fft.handle;
    assert(length > 0 && length <= LHDC_LLAC_DCT_MAX_LENGTH);
    int error = lhdc_dct4_f32_init(&work->plan, length, work->rotations,
                                   2 * (sizeof work->rotations / sizeof work->rotations[0]));
    assert(error == LHDC_DCT4_OK);
    (void)error;
    dct->twid1 = (Complex *)work->plan.pre_rotation;
    dct->twid2 = (Complex *)work->plan.post_rotation;
    directory->dct_twid1 = dct->twid1;
    directory->dct_twid2 = dct->twid2;
}
