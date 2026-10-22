#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_DECODE_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_DECODE_H
#include "llac_init.h"
/* Decode one channel's spectrum, inverse-transform, and scale PCM. */
int llac_decode_channel(LLAC_Dec *, int, uint8_t *, void *, int, int);
/* Descramble input and organize frame decoding by channel mode. */
uint8_t llac_descramble_and_decode_channels(LLAC_Dec *, uint8_t *, int, void **, int);
/* Validate input, instance, and per-channel output pointers before decoding. */
uint8_t llac_decode_checked(LLAC_Dec *, void *, int, void **, int);
/* Initialize LLAC wrapper directories, frame parameters, and input/output lengths. */
uint8_t llac_dec_initial(unsigned, short, short, float, int, int *, unsigned *,
                         LLACWorkspaceDirectory *);
/* Return consumed bytes or a negative LLAC error; bad-frame concealment still permits PCM output. */
int llac_dec_process(unsigned char *, int *, LLACWorkspaceDirectory *);
#endif
