#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/bits.h>
#include <audio_math/numeric_bits.h>
#include "../include/lhdc_dec_core.h"
#include <string.h>

/* Query decoder-state size including this instance's filter history. */
uint32_t lhdc_dec_mem_get_size(void) {
    return (uint32_t)sizeof(lhdc_Decoder_t);
}

/* Reset parsing state while retaining same-stream upsampling history for error recovery. */
int lhdc_dec_restart(lhdc_Decoder_t *decb, unsigned char bits_per_sample, unsigned short block_size,
                     cirbuf_s *pcb, unsigned char *dec_buf, int lhdc_ver) {
    /* Error recovery resets only bitstream state, preserving this stream's upsampling delay history.
     * Outer initialization clears all state for a new stream. */
    memset(decb, 0, offsetof(lhdc_Decoder_t, filter_history));
    unsigned top = audio_math_shift_left_low8_zero32(1, (unsigned)bits_per_sample - 1);
    decb->sample_max = audio_math_int32_from_bits(top - 1);
    decb->sample_min = audio_math_int32_from_bits(0u - top);
    decb->bits_per_sample = bits_per_sample;
    decb->block_size = block_size;
    decb->chunk_size = 64;
    decb->chunk_cnt = block_size >> 6;
    decb->lz_dec_buf[0] = dec_buf;
    decb->lz_dec_buf[1] = dec_buf + 768;
    decb->bit_rb.pcb = pcb;
    decb->bit_rb.in_max = pcb->cbuf + pcb->max_len;
    decb->lhdc_ver_code = lhdc_ver;
    decb->state = 1;
    return LHDC_DEC_OK;
}

/* Initialize a new decoder instance and clear its private upsampling history. */
int lhdc_dec_init(lhdc_Decoder_t *decb, unsigned char bits_per_sample, unsigned short block_size,
                  cirbuf_s *pcb, unsigned char *dec_buf, int lhdc_ver) {
    memset(decb, 0, sizeof *decb);
    return lhdc_dec_restart(decb, bits_per_sample, block_size, pcb, dec_buf, lhdc_ver);
}

/* Read extension metadata; choose byte/word copying by buffer separation and update the destination in order. */
void lhdc_chk_ext_header(lhdc_Decoder_t *decb, unsigned char *pext_flag, unsigned char *pext_offset,
                         unsigned char *pext_func, unsigned char *pext_data, int nExtDataBytes) {
    *pext_flag = decb->extention_flag;
    *pext_offset = decb->ext_offset;
    *pext_func = decb->ext_func;
    if (nExtDataBytes <= 0)
        return;
    unsigned n = nExtDataBytes > 16 ? 16 : (unsigned)nExtDataBytes, i = 0;
    unsigned char *src = decb->ext_data;
    if ((uintptr_t)pext_data - ((uintptr_t)src + 1) >= 3 && nExtDataBytes >= 9) {
        for (; i + 4 <= n; i += 4) {
            uint32_t word;
            memcpy(&word, src + i, 4);
            memcpy(pext_data + i, &word, 4);
        }
    }
    for (; i < n; ++i)
        pext_data[i] = src[i];
}
