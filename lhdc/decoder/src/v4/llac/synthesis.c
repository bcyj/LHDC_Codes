#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/fma_native.h>
#include "../include/llac_fft.h"
#include "../include/llac_dct_workspace.h"
#include <math.h>
#include <assert.h>
#include <string.h>

/* Inverse-transform spectrum in place with the instance's common DCT-IV plan; stage pre-rotation in output before window mixing overwrites it.
 * The common core uses separate multiply/add; only lengths 220/240 transform, other lengths do nothing. */
void llac_dct4_transform(Dct4 *dct, float *inout, kiss_fft_cpx *temporary) {
    assert((const void *)inout != (const void *)temporary);
    if (dct->length != 220 && dct->length != 240)
        return;
    LhdcLlacDctWorkspace *workspace = dct->fft.handle;
    assert(workspace->plan.length == dct->length);
    const LhdcDct4F32Work work = {temporary,
                                  (size_t)dct->length,
                                  workspace->scratch.spectrum,
                                  LHDC_LLAC_DCT_MAX_LENGTH,
                                  workspace->radix_scratch,
                                  LHDC_LLAC_DCT_RADIX_FLOATS};
    int error = lhdc_dct4_f32_transform(&workspace->plan, inout, inout, &work);
    assert(error == LHDC_DCT4_OK);
    (void)error;
}

/* Return the obfuscation key used to generate window tables. */
uint32_t llac_window_scramble_key(int length) {
    return length == 220 ? UINT32_C(0xaf194cc9) : UINT32_C(0x36488475);
}

/* Interpret XOR-decoded window bits as IEEE-754 floating point. */
static float window_value(const float *window, int index, uint32_t key) {
    uint32_t bits;
    float value;
    memcpy(&bits, window + index, 4);
    bits ^= key;
    memcpy(&value, &bits, 4);
    return value;
}

/* Apply 220/240-point LLAC inverse transform, decode obfuscated windows, overlap-add, and retain the last three quarters of frame history. */
void llac_synthesize_frame(float *y, int yLen, float *win, float *mem, LhdcLlacPcmScratch *pcm,
                           Dct4 *dct) {
    llac_dct4_transform(dct, y, pcm->packed);
    float *x = pcm->samples;
    if (yLen != 220 && yLen != 240)
        return;
    int quarter = yLen / 4;
    uint32_t key = llac_window_scramble_key(yLen);
    for (int i = 0; i < quarter; ++i)
        x[i] =
            audio_math_fma_f32(y[3 * quarter + i], window_value(win, 7 * quarter - 1 - i, key), mem[i]);
    for (int i = 0; i < 2 * quarter; ++i)
        x[quarter + i] = audio_math_fma_f32(-y[yLen - 1 - i], window_value(win, 6 * quarter - 1 - i, key),
                                      mem[quarter + i]);
    for (int i = 0; i < quarter; ++i)
        x[3 * quarter + i] = -(y[2 * quarter - 1 - i] * window_value(win, yLen - 1 - i, key));
    for (int i = 0; i < quarter; ++i)
        mem[i] = -(y[quarter - 1 - i] * window_value(win, 3 * quarter - 1 - i, key));
    for (int i = 0; i < 2 * quarter; ++i)
        mem[quarter + i] = -(y[i] * window_value(win, 2 * quarter - 1 - i, key));
}
