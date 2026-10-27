#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/bits.h>
#include <audio_math/byte_order.h>
#include "../../../../../math/include/audio_math/integer.h"
#include "../include/lhdc_dec_core.h"
#include <string.h>

/* Expand alternating literal/back-reference blocks, retaining the initial control-byte read even for zero output. */
unsigned mcu_lzh_dec(bit_buf_t *bit_rb, unsigned char *out, unsigned out_len) {
    unsigned pos = 0, reference = 0;
    do {
        unsigned length = read_u8(bit_rb);
        if (length) {
            if (length > out_len - pos)
                return LHDC_DEC_NO_OUTPUT;
            if (!reference) {
                for (unsigned i = 0; i < length; ++i)
                    out[pos + i] = read_u8(bit_rb);
            } else {
                unsigned offset = read_u8(bit_rb);
                if (!offset)
                    return LHDC_DEC_NO_OUTPUT;
                if (offset == 255)
                    offset += read_u8(bit_rb);
                if (offset > pos)
                    return LHDC_DEC_NO_OUTPUT;
                unsigned i = 0;
                if (offset >= 4 && length >= 9) {
                    for (; i + 4 <= length; i += 4) {
                        uint32_t word;
                        memcpy(&word, out + pos - offset + i, 4);
                        memcpy(out + pos + i, &word, 4);
                    }
                }
                for (; i < length; ++i)
                    out[pos + i] = out[pos - offset + i];
            }
            pos += length;
        }
        reference ^= 1;
    } while (pos < out_len);
    return pos == out_len ? out_len : LHDC_DEC_NO_OUTPUT;
}

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
/* Combine LZH high bits and Rice low bits with a compact loop on MSVC x86/x64. */
static __forceinline void lzh_decode_width_compact(bit_buf_t *bit_rb,
                                                   unsigned char *residual_bytes,
                                                   unsigned count, unsigned byte_count,
                                                   unsigned width, int *output) {
    for (unsigned i = 0; i < count; ++i) {
        unsigned value;
        if (byte_count == 1) {
            value = *residual_bytes++;
        } else if (byte_count == 2) {
            value = audio_math_read_be16(residual_bytes);
            residual_bytes += 2;
        } else if (byte_count == 3) {
            value = audio_math_read_be24(residual_bytes);
            residual_bytes += 3;
        } else {
            value = audio_math_read_be32(residual_bytes);
            residual_bytes += 4;
        }
        unsigned low;
        read_uint32(bit_rb, width, &low);
        output[i] = audio_math_rice_unfold_u32((value << width) | low);
    }
}
#endif

/* Expand LZ data in the first partition, append low-bit remainders, and apply Rice sign folding. */
int lz_decode(lhdc_Decoder_t *decb, int ch, int up_factor, int ns, int n_unenc,
              unsigned char lpc_hdr, unsigned char db, int *rd_i) {
    if (!decb->ck) {
        unsigned length = (unsigned)db * (unsigned)((int)decb->block_size / up_factor - n_unenc);
        if (mcu_lzh_dec(&decb->bit_rb, decb->lz_dec_buf[ch], length) != length)
            return LHDC_DEC_NO_OUTPUT;
    }
    unsigned char *residual_bytes = decb->lz_dec_buf[ch] + decb->lz_buf_idx[ch];
    unsigned width = lpc_hdr & 31;
    decb->lz_buf_idx[ch] = (unsigned short)(decb->lz_buf_idx[ch] + (unsigned short)ns * db);
    unsigned n = db < 2 ? 1 : (db == 2 ? 2 : (db == 3 ? 3 : 4));
    if (width) {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        lzh_decode_width_compact(&decb->bit_rb, residual_bytes, (unsigned)ns, n, width, rd_i);
#else
        switch (n) {
        case 1:
            for (int i = 0; i < ns; ++i) {
                unsigned value = *residual_bytes++;
                unsigned low;
                read_uint32(&decb->bit_rb, width, &low);
                value = (value << width) | low;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        case 2:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be16(residual_bytes);
                residual_bytes += 2;
                unsigned low;
                read_uint32(&decb->bit_rb, width, &low);
                value = (value << width) | low;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        case 3:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be24(residual_bytes);
                residual_bytes += 3;
                unsigned low;
                read_uint32(&decb->bit_rb, width, &low);
                value = (value << width) | low;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        default:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be32(residual_bytes);
                residual_bytes += 4;
                unsigned low;
                read_uint32(&decb->bit_rb, width, &low);
                value = (value << width) | low;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        }
#endif
    } else {
        switch (n) {
        case 1:
            for (int i = 0; i < ns; ++i) {
                unsigned value = *residual_bytes++;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        case 2:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be16(residual_bytes);
                residual_bytes += 2;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        case 3:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be24(residual_bytes);
                residual_bytes += 3;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        default:
            for (int i = 0; i < ns; ++i) {
                unsigned value = audio_math_read_be32(residual_bytes);
                residual_bytes += 4;
                rd_i[i] = audio_math_rice_unfold_u32(value);
            }
            break;
        }
    }
    if ((unsigned)decb->ck != (unsigned)decb->chunk_cnt - 1) {
        return ns;
    }
    decb->lz_buf_idx[ch] = 0;
    decb->lz_buf_len[ch] = 0;
    return ns;
}
