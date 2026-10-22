#ifndef LHDC_DECODER_V5_SYNTHESIS_TABLES_H
#define LHDC_DECODER_V5_SYNTHESIS_TABLES_H
#include "../../../../common/include/dsp/fft_fixed32.h"

/* Return one for fixed configurations; unlisted sizes/null output return zero without writes, preserving dynamic fallback. */
int lhdc_v5_time_copy_rotations(int size, LhdcFftFixed32Complex *pre, LhdcFftFixed32Complex *post);
int lhdc_v5_time_copy_window(int length, int32_t *window);

#endif
