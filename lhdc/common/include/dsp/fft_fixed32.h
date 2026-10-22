/* Shared KISS-style FFT with 32-bit integer samples and Q31 twiddles; excludes PCM format conversion. */
#ifndef LHDC_DSP_FFT_FIXED32_H
#define LHDC_DSP_FFT_FIXED32_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LhdcFftFixed32Complex {
    int32_t r, i;
} LhdcFftFixed32Complex;
typedef struct LhdcFftFixed32Plan {
    int32_t length;
    int32_t inverse; /* Zero selects forward, nonzero inverse; twiddle direction must match. */
    const LhdcFftFixed32Complex *twiddles;
} LhdcFftFixed32Plan;
enum {
    LHDC_FFT_FIXED32_OK = 0,
    LHDC_FFT_FIXED32_INVALID_ARGUMENT = -1,
    LHDC_FFT_FIXED32_UNSUPPORTED_LENGTH = -2
};
/* Execute a 60/120/240/480-point integer FFT, producing approximately 1/N of the mathematical FFT.
 * Input, output, and twiddles each contain length complex values; output must not overlap input or twiddles.
 * Radix 2/4 uses arithmetic shifts; radix 3/5 uses rounded Q31 reciprocal multiplication. No saturation; 32/64-bit sums wrap.
 * Plans and tables are read-only; no recursion or heap allocation. The caller defines input amplitude format, not necessarily normalized Q31.
 */
int lhdc_fft_fixed32_execute(const LhdcFftFixed32Plan *plan, const LhdcFftFixed32Complex *input,
                             LhdcFftFixed32Complex *output);
#ifdef __cplusplus
}
#endif
#endif
