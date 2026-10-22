#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LHDC_DEC_CORE_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LHDC_DEC_CORE_H
#include "lhdc_dec.h"
#include "bit_reader.h"
#include "upsample_history_view.h"

typedef struct lhdc_Decoder_t {
    int sample_max, sample_min, const_dat[2], rmb[2], last_dat[2];
    unsigned short block_size, chunk_size, chunk_cnt;
    unsigned char *lz_dec_buf[2];
    unsigned short lz_buf_len[2], lz_buf_idx[2];
    unsigned char bits_per_sample, state, ftype, ups, dec_ch;
    unsigned char ctype[2], lpc_hdr[2], lz_bytes_per_sample[2];
    unsigned puiLpcVOrder[2];
    int pnQLpcVCoef[2][8], pnQLpcVQuantizeShift[2], pnLpcVPrevSample[2][72];
    int lhdc_ver_code;
    unsigned char data_skipped;
    unsigned short ck;
    int frame_len, dec_cnt, shift_bits_down_sample, rs[2][14];
    bit_buf_t bit_rb;
    int up2x_pcm_buf[2][80];
    unsigned char extention_flag, ext_offset, ext_func, ext_data[16];
    LHDCUpsampleHistory filter_history[2];
    int upsample_cache[2][48];
} lhdc_Decoder_t;
/* Return native decoder-state size including filter history and delay buffers. */
uint32_t lhdc_dec_mem_get_size(void);
/* Clear state and bind ring input, LZ workspace, and decoding parameters. */
int lhdc_dec_init(lhdc_Decoder_t *, unsigned char, unsigned short, cirbuf_s *, unsigned char *,
                  int);
/* Reset parsing for error recovery while preserving this stream's filter and delay history. */
int lhdc_dec_restart(lhdc_Decoder_t *, unsigned char, unsigned short, cirbuf_s *, unsigned char *,
                     int);
/* Decode an integer sample block; return sample count, insufficient-input status, or frame-format error. */
int lhdc_dec_decode(lhdc_Decoder_t *, int **);
/* Decode Rice residuals with ten-zero/six-bit escape segments; return zero on exhaustion and retain partial output. */
int rice_decode(bit_buf_t *, int, unsigned, int *);
/* Decode alternating literal/back-reference blocks; return zero on format errors and retain written data. */
unsigned mcu_lzh_dec(bit_buf_t *, unsigned char *, unsigned);
/* Read LZ blocks and bitstream remainders, fold signs, and clear the LZ index in the final partition. */
int lz_decode(lhdc_Decoder_t *, int, int, int, int, unsigned char, unsigned char, int *);
/* Copy extension flags and up to 16 bytes, handling overlap in copy order. */
void lhdc_chk_ext_header(lhdc_Decoder_t *, unsigned char *, unsigned char *, unsigned char *,
                         unsigned char *, int);
/* Reconstruct LPC samples with wrapping 64-bit multiply-add, supporting orders zero through eight. */
void lpcV_Ressw(int *, unsigned, int *, unsigned, int, int *, int *);
#endif
