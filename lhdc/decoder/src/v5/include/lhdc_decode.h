#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DECODE_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_DECODE_H
#include <stdint.h>
/* Decode mono lossy data to internal integer PCM; return sample count or error. */
int lhdc_dec_top(void *, unsigned *, int, int *, int);
/* Parse public V5 frames, descramble channel data, and invoke the core. */
int lhdc_v5_dec_decode(unsigned char *, int *, int *, void *);
#endif
