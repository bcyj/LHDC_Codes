#include "../../../include/lhdc/decoder/status.h"
#include "../include/lhdc_dec_core.h"
#include "../../../../../math/include/audio_math/byte_order.h"
#include "../../../../../math/include/audio_math/integer.h"
#include "../include/lhdc_upsample.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Restore the stereo matrix, preserving four-element write order and integer wraparound. */
static void restore_v4_matrix(int **channels, uint32_t count, uint32_t mode) {
    if (count == 0 || (mode != 0x20 && mode != 0x40))
        return;
    uint32_t rounded_count = ((count - 1) & ~3U) + 4;
    for (uint32_t index = 0; index < rounded_count; index += 4) {
        for (uint32_t lane = 0; lane < 4; ++lane) {
            uint32_t position = index + lane;
            if (mode == 0x20)
                channels[1][position] =
                    audio_math_add_wrap_i32(channels[1][position], channels[0][position]);
            else
                channels[0][position] =
                    audio_math_subtract_wrap_i32(channels[1][position], channels[0][position]);
        }
    }
}

/* Scale PCM by channel gains; write four left then four right samples per group, processing only the selected duplicate channel. */
static void scale_v4_channels(lhdc_Decoder_t *state, int **channels, uint32_t count, uint32_t mode,
                              bool shift_output) {
    if (count == 0)
        return;
    unsigned raw_shift = shift_output ? (unsigned)state->shift_bits_down_sample & 0xffu : 0;
    unsigned shift = raw_shift < 32 ? raw_shift : 31;
    uint32_t rounded_count = ((count - 1) & ~3U) + 4;
    for (uint32_t index = 0; index < rounded_count; index += 4) {
        if (mode == 0x60) {
            uint32_t channel = state->dec_ch;
            for (uint32_t lane = 0; lane < 4; ++lane) {
                int product = audio_math_multiply_wrap_i32(state->rmb[channel],
                                                           channels[channel][index + lane]);
                channels[channel][index + lane] =
                    audio_math_shift_right_bounded_i32(product, shift);
            }
        } else {
            for (uint32_t channel = 0; channel < 2; ++channel) {
                for (uint32_t lane = 0; lane < 4; ++lane) {
                    int product = audio_math_multiply_wrap_i32(state->rmb[channel],
                                                               channels[channel][index + lane]);
                    channels[channel][index + lane] =
                        audio_math_shift_right_bounded_i32(product, shift);
                }
            }
        }
    }
}

/* Handle the eight-sample buffer on full-rate mode changes, preserving decimation, upsampling, and bitstream rollback order. */
static void cache_v4_transition(lhdc_Decoder_t *state, int **op, int **out_pcm, uint32_t mode) {
    op[0] = state->up2x_pcm_buf[0];
    op[1] = state->up2x_pcm_buf[1];
    unsigned shift = (unsigned)state->shift_bits_down_sample & 0xffu;
    if (mode == 0x60) {
        for (unsigned i = 0; i < 8; ++i) {
            int product =
                audio_math_multiply_wrap_i32(state->rmb[state->dec_ch], out_pcm[0][2 * i]);
            op[state->dec_ch][i] = audio_math_shift_right_i32(product, shift);
        }
    } else {
        int left_gain = state->rmb[0], right_gain = state->rmb[1];
        for (unsigned i = 0; i < 8; ++i) {
            state->up2x_pcm_buf[0][i] = audio_math_shift_right_i32(
                audio_math_multiply_wrap_i32(left_gain, out_pcm[0][2 * i]), shift);
            state->up2x_pcm_buf[1][i] = audio_math_shift_right_i32(
                audio_math_multiply_wrap_i32(right_gain, out_pcm[1][2 * i]), shift);
        }
    }
    uint32_t bit_depth = state->bits_per_sample;
    if ((state->ftype & 0x60) == 0x60) {
        fast_upsampling_2x(state, op[state->dec_ch], *out_pcm, 8, (uint32_t)state->dec_ch, 0,
                           bit_depth);
    } else {
        fast_upsampling_2x(state, state->up2x_pcm_buf[0], *out_pcm, 8, 0, 0, bit_depth);
        fast_upsampling_2x(state, state->up2x_pcm_buf[1], out_pcm[1], 8, 1, 0, bit_depth);
    }
    state->bit_rb.rb = state->bit_rb.saved_rb;
    state->bit_rb.db = state->bit_rb.saved_db;
    memset(state->lz_buf_len, 0, sizeof(state->lz_buf_len));
    memset(state->lz_buf_idx, 0, sizeof(state->lz_buf_idx));
    return;
}

/* Apply matrix reconstruction, gain, upsampling, and duplicate-channel copying in order; return output sample count. */
static uint32_t restore_v4_output(lhdc_Decoder_t *state, int **channels, int **out_pcm,
                                  uint32_t count, uint32_t header_bits, uint32_t mode,
                                  bool *cache_only) {
    *cache_only = false;

    state->last_dat[0] = channels[0][count - 1];
    state->last_dat[1] = channels[1][count - 1];
    restore_v4_matrix(channels, count, mode);

    bool need_upsampling = (header_bits & 0x80) == 0;
    bool need_scaling = need_upsampling;
    if (mode == 0x20 || mode == 0x40) {
        if (count != 0) {
            mode = state->ftype & 0x60;
            need_upsampling = (state->ftype & 0x80) == 0;
            need_scaling = need_upsampling;
        } else {
            /* The zero-sample matrix branch checks upsampling directly and skips gain processing. */
            need_scaling = false;
        }
    }
    if (need_upsampling) {
        if (need_scaling)
            scale_v4_channels(state, channels, count, mode, true);
        if (state->ups == 1) {
            for (uint32_t index = 0; index < 64; ++index) {
                out_pcm[0][index] = 0;
                out_pcm[1][index] = 0;
            }
            count = 64;
        } else {
            uint32_t bit_depth = state->bits_per_sample;
            if ((state->ftype & 0x60) == 0x60) {
                fast_upsampling_2x(state, channels[state->dec_ch], out_pcm[0], count, state->dec_ch,
                                   0, bit_depth);
            } else {
                fast_upsampling_2x(state, channels[0], out_pcm[0], count, 0, 0, bit_depth);
                fast_upsampling_2x(state, channels[1], out_pcm[1], count, 1, 0, bit_depth);
            }
            count <<= 1;
        }
    } else {
        if (state->ups == 5) {
            cache_v4_transition(state, channels, out_pcm, mode);
            *cache_only = true;
            return count;
        }

        if (mode == 0x60) {
            int *duplicated_channels[2] = {out_pcm[0], out_pcm[0]};
            scale_v4_channels(state, duplicated_channels, count, mode, false);
        } else {
            scale_v4_channels(state, out_pcm, count, mode, false);
        }
    }
    if ((state->ftype & 0x60) == 0x60)
        memcpy(out_pcm[1], out_pcm[0], count << 2);
    return count;
}

/* Commit ring-buffer bytes consumed by this frame and mark completion, preserving two-segment boundary consumption order. */
static void finish_v4_frame(lhdc_Decoder_t *state, uint32_t bit_count_or_channel) {
    cirbuf_s *bit_rb;
    uint8_t *dummy;
    uint32_t header_bits_or_residual, field_width_or_count, bit_accumulator;
    bit_rb = state->bit_rb.pcb;
    state->state = '\x02';
    state->data_skipped = '\0';
    header_bits_or_residual = bit_rb->max_len;
    field_width_or_count = bit_rb->odx;
    bit_accumulator = header_bits_or_residual - field_width_or_count;
    if (bit_accumulator < bit_count_or_channel) {
        lhdc_cirbuf_get_no_copy(bit_rb, &dummy, bit_accumulator);
        bit_count_or_channel =
            (state->bit_rb.read_len + field_width_or_count) - header_bits_or_residual;
        state->bit_rb.read_len = bit_count_or_channel;
    }
    if (bit_count_or_channel != 0) {
        lhdc_cirbuf_get_no_copy(state->bit_rb.pcb, &dummy, bit_count_or_channel);
    }
    state->bit_rb.read_len = 0;
}

/* Commit bytes loaded during header search, consuming physical segments in order across the ring tail. */
static void discard_v4_header_bytes(lhdc_Decoder_t *state, uint32_t consumed_bytes) {
    cirbuf_s *ring = state->bit_rb.pcb;
    uint8_t *discarded;
    uint32_t capacity = ring->max_len;
    uint32_t offset = ring->odx;
    uint32_t until_end = capacity - offset;
    if (until_end < consumed_bytes) {
        lhdc_cirbuf_get_no_copy(ring, &discarded, until_end);
        ring = state->bit_rb.pcb;
        consumed_bytes = (state->bit_rb.read_len + offset) - capacity;
        state->bit_rb.read_len = consumed_bytes;
    }
    if (consumed_bytes != 0)
        lhdc_cirbuf_get_no_copy(ring, &discarded, consumed_bytes);
}

/* Expand header exponent and three-bit gain mantissa, handling small exponents and sign-bit overflow separately. */
static uint32_t expand_v4_header_gain(uint32_t code) {
    int32_t exponent_value = audio_math_int32_from_bits(1U << (code >> 3));
    uint32_t gain = code & 7;
    if (exponent_value > 7)
        gain = gain * (exponent_value >> 3) + exponent_value;
    return gain;
}

/* Read V4 extension flags and up to fifteen bytes, storing four-byte groups followed by trailing bytes. */
static void read_v4_extension(lhdc_Decoder_t *state) {
    bit_buf_t *bits = &state->bit_rb;
    uint32_t extension;
    read_uint32(bits, 8, &extension);
    uint32_t length = extension >> 4;
    state->ext_func = extension & 7;
    state->ext_offset = length;
    uint32_t offset = 0;
    while (length >= 4) {
        uint32_t word;
        read_uint32(bits, 32, &word);
        memcpy(state->ext_data + offset, &word, sizeof(word));
        offset += 4;
        length -= 4;
    }
    if (length != 0) {
        uint32_t tail;
        read_uint32(bits, length * 8, &tail);
        state->ext_data[offset + 0] = tail;
        if (length >= 2)
            state->ext_data[offset + 1] = tail >> 8;
        if (length == 3)
            state->ext_data[offset + 2] = tail >> 16;
    }
}

/* Initialize constant samples, residual headers, and LPC parameters from the selected channel; retain consumed bits and partial writes on failure. */
static bool read_v4_channel_parameters(lhdc_Decoder_t *state, uint32_t *header_flags) {
    bit_buf_t *bits = &state->bit_rb;
    uint32_t channel = state->dec_ch;
    uint32_t mode = *header_flags & 0x60;
    if (channel >= 2)
        return channel == 2;
    for (;;) {
        int *previous = &state->const_dat[channel];
        uint32_t block_type = state->ctype[channel];
        if (block_type == 0) {
            read_int32(bits, state->bits_per_sample, previous);
            int sample = *previous;
            if (state->sample_max <
                audio_math_int32_from_bits((uint32_t)sample ^ audio_math_sign_mask_i32(sample)))
                return false;
            *header_flags = state->ftype;
            mode = *header_flags & 0x60;
        } else if (block_type != 1) {
            uint32_t residual_header;
            read_uint32(bits, 8, &residual_header);
            state->lpc_hdr[channel] = residual_header;
            if ((residual_header & 0x1f) > state->bits_per_sample)
                return false;
            if ((residual_header & 0x40) != 0) {
                uint32_t compressed_bytes;
                read_uint32(bits, 8, &compressed_bytes);
                state->lz_bytes_per_sample[channel] = compressed_bytes;
                if (compressed_bytes > 4)
                    return false;
            }
            if (state->lhdc_ver_code >= 300 && block_type == 0xf) {
                uint32_t predictor;
                read_uint32(bits, 8, &predictor);
                uint32_t order = (predictor >> 5) + 1;
                state->puiLpcVOrder[channel] = order;
                state->pnQLpcVQuantizeShift[channel] = predictor & 0x1f;
                int *coefficients = state->pnQLpcVCoef[channel];
                for (uint32_t index = 0; index < order; ++index)
                    read_int32(bits, 16, coefficients + index);
                *header_flags = state->ftype;
                mode = *header_flags & 0x60;
            }
        }
        if (mode == 0x60)
            return true;
        if (channel != 0)
            return true;
        channel = 1;
    }
}

/* Parse one sync candidate: return 1 for decodable, 0 for more input, or 2 to continue searching. */
static int parse_v4_header_candidate(lhdc_Decoder_t *state, uint32_t sync_word,
                                     uint32_t *consumed_bytes, uint32_t *header_flags) {
    bit_buf_t *bits = &state->bit_rb;
    cirbuf_s *ring = bits->pcb;
    int version = state->lhdc_ver_code;
    *header_flags = sync_word & 0xff;
    if (version >= 400)
        state->extention_flag = sync_word & 0x10;
    uint32_t mode = sync_word & 0x60;
    uint32_t length_high = sync_word & (version < 400 ? 0x1f : 0xf);
    state->ftype = sync_word;
    state->frame_len = length_high;
    if (mode != 0x60)
        state->dec_ch = 0;
    uint32_t payload;
    read_uint32(bits, mode == 0x60 ? 24 : 32, &payload);
    uint32_t frame_length = (mode == 0x60 ? payload >> 16 : payload >> 24) | (length_high << 8);
    state->frame_len = frame_length;
    uint32_t frame_bytes = (state->block_size * state->bits_per_sample) >> 3;
    if (mode == 0x60) {
        /* Oversize rejection still commits word changes from reading the 24-bit field, not just the sync word. */
        *consumed_bytes = bits->read_len;
        if (frame_length > frame_bytes + 6)
            return LHDC_V4_DEC_HEADER_REJECTED;
        if ((ring->s_len & ~3U) + (state->bit_rb.saved_rb >> 3) < frame_length) {
            bits->rb = state->bit_rb.saved_rb;
            bits->db = state->bit_rb.saved_db;
            return LHDC_V4_DEC_HEADER_NEED_INPUT;
        }
        state->dec_cnt = 0;
        uint32_t channel = (payload >> 8) & 0xf;
        state->dec_ch = channel;
        state->ctype[channel] = (payload >> 12) & 0xf;
        *consumed_bytes = bits->read_len;
        if ((channel & 0xe) != 0)
            return LHDC_V4_DEC_HEADER_REJECTED;
        uint32_t gain = expand_v4_header_gain(payload & 0xff);
        state->rmb[channel] = gain;
        if ((int)gain >= state->sample_max)
            return LHDC_V4_DEC_HEADER_REJECTED;
    } else {
        *consumed_bytes = bits->read_len;
        if (((payload >> 24) & 3) != 0 || frame_length > (frame_bytes + 3) * 2)
            return LHDC_V4_DEC_HEADER_REJECTED;
        if (ring->s_len + (state->bit_rb.saved_rb >> 3) < frame_length) {
            bits->rb = state->bit_rb.saved_rb;
            bits->db = state->bit_rb.saved_db;
            return LHDC_V4_DEC_HEADER_NEED_INPUT;
        }
        state->ctype[0] = (payload >> 20) & 0xf;
        state->ctype[1] = (payload >> 16) & 0xf;
        uint32_t left_gain = expand_v4_header_gain((payload >> 8) & 0xff);
        uint32_t right_gain = expand_v4_header_gain(payload & 0xff);
        state->dec_cnt = 0;
        state->rmb[0] = left_gain;
        state->rmb[1] = right_gain;
        if ((int)left_gain >= state->sample_max || (int)right_gain >= state->sample_max)
            return LHDC_V4_DEC_HEADER_REJECTED;
    }
    if (version >= 400 && state->extention_flag != 0)
        read_v4_extension(state);
    if (!read_v4_channel_parameters(state, header_flags)) {
        *consumed_bytes = bits->read_len;
        return LHDC_V4_DEC_HEADER_REJECTED;
    }
    if (state->data_skipped != 0 && state->state != 1)
        return LHDC_V4_DEC_DISCONTINUITY;
    return LHDC_V4_DEC_HEADER_ACCEPTED;
}

/* Search 16-bit sync words in order, preserving scan flags, read-count resets, and rollback on insufficient input. */
static int read_v4_frame_header(lhdc_Decoder_t *state, uint32_t *header_flags) {
    bit_buf_t *bits = &state->bit_rb;
    uint32_t minimum_bytes = ((state->bits_per_sample >> 3) + 3) * 2;
    state->ctype[0] = 0;
    state->ctype[1] = 0;
    if (bits->pcb->s_len < minimum_bytes)
        return LHDC_V4_DEC_HEADER_NEED_INPUT;
    for (;;) {
        uint32_t sync_word;
        read_uint32(bits, 16, &sync_word);
        uint32_t consumed_bytes = bits->read_len;
        if ((sync_word >> 8) == 0x4c) {
            int result = parse_v4_header_candidate(state, sync_word, &consumed_bytes, header_flags);
            if (result != 2)
                return result;
        }
        discard_v4_header_bytes(state, consumed_bytes);
        state->data_skipped = 1;
        bits->read_len = 0;
        if (bits->pcb->s_len < minimum_bytes)
            return LHDC_V4_DEC_HEADER_NEED_INPUT;
    }
}

/* Build finite differences from starting samples, retaining each order's latest value modulo 2^32. */
static void initialize_v4_differences(int *history, unsigned order, const int *samples,
                                      unsigned warmup_count) {
    for (unsigned level = 0; level < order; ++level)
        history[level] = 0;
    for (unsigned i = 1; i < warmup_count; ++i) {
        int difference = audio_math_subtract_wrap_i32(samples[i], samples[i - 1]);
        for (unsigned level = 1; level < warmup_count; ++level) {
            int previous = history[level];
            history[level] = difference;
            difference = audio_math_subtract_wrap_i32(difference, previous);
        }
    }
}

/* Integrate one residual backward into difference state; first-order processing still records history[1]. */
static int integrate_v4_difference(int *history, unsigned order, int residual) {
    if (order == 1) {
        history[1] = residual;
        return residual;
    }
    for (unsigned level = order - 1; level != 0; --level) {
        residual = audio_math_add_wrap_i32(residual, history[level]);
        history[level] = residual;
    }
    return residual;
}

/* Read raw integer samples and validate amplitude, preserving word changes, wrapping, and consumed input on rejection. */
static int decode_v4_raw_samples(lhdc_Decoder_t *state, uint32_t sample_count, int *sample_cursor,
                                 uint32_t *header_flags) {
    uint32_t header_bits_or_residual;
    uint32_t next_bit_count;
    uint32_t bit_accumulator;
    uint32_t field_width_or_count;
    uint32_t *sample_word_cursor;
    bool ring_wrap;
    uint32_t samples_in_chunk;
    uint32_t header_bits;
    uint32_t *sample_words = (uint32_t *)sample_cursor;
    samples_in_chunk = sample_count;
    header_bits = *header_flags;

    if (sample_count == 0)
        return LHDC_DEC_OK;
    sample_word_cursor = sample_words - 1;
    do {
        field_width_or_count = (uint32_t)state->bits_per_sample;
        bit_accumulator = 0x20 - field_width_or_count;
        sample_word_cursor = sample_word_cursor + 1;
        *sample_word_cursor = audio_math_shift_right_low8_zero32(state->bit_rb.db, bit_accumulator);
        header_bits_or_residual = state->bit_rb.rb;
        if (header_bits_or_residual < field_width_or_count) {
            next_bit_count = audio_math_read_le32(state->bit_rb.in);
            state->bit_rb.rb = 32;
            state->bit_rb.db = next_bit_count;
            header_bits = (header_bits_or_residual + 0x20) - field_width_or_count;
            field_width_or_count = field_width_or_count - header_bits_or_residual;
            *sample_word_cursor = *sample_word_cursor |
                                  audio_math_shift_right_low8_zero32(next_bit_count, header_bits);
            state->bit_rb.read_len = state->bit_rb.read_len + 4;
            state->bit_rb.in += sizeof(uint32_t);
            ring_wrap = state->bit_rb.in_max <= state->bit_rb.in;
            header_bits_or_residual = state->bit_rb.rb;
            if (ring_wrap) {
                state->bit_rb.in = state->bit_rb.pcb->cbuf;
            }
        }
        state->bit_rb.rb = header_bits_or_residual - field_width_or_count;
        state->bit_rb.db =
            audio_math_shift_left_low8_zero32(state->bit_rb.db, field_width_or_count);
        bit_accumulator = audio_math_shift_arithmetic_low8_sign32(
            audio_math_shift_left_low8_zero32(*sample_word_cursor, bit_accumulator),
            bit_accumulator);
        *sample_word_cursor = bit_accumulator;
        if (state->sample_max <
            audio_math_int32_from_bits(
                bit_accumulator ^
                audio_math_sign_mask_i32(audio_math_int32_from_bits(bit_accumulator))))
            return LHDC_V4_DEC_CHANNEL_REJECTED;
    } while (sample_words + ((int)samples_in_chunk - 1) != sample_word_cursor);

    *header_flags = state->ftype;
    return LHDC_DEC_OK;
}

/* Copy a constant channel, preserving per-sample read order when output overlaps state. */
static int decode_v4_constant_samples(lhdc_Decoder_t *state, uint32_t channel,
                                      uint32_t sample_count, int *sample_cursor,
                                      uint32_t *header_flags) {
    int sample_or_version;
    int *sample_end;
    uint32_t samples_in_chunk;
    uint32_t header_bits;
    int *channel_state = &state->const_dat[channel];
    samples_in_chunk = sample_count;
    header_bits = *header_flags;

    if (sample_count == 0) {
        return LHDC_DEC_OK;
    }
    if (((int)samples_in_chunk - 1U < 0xc ||
         (sample_cursor < channel_state + 1 &&
          channel_state < sample_cursor + (int)samples_in_chunk)) ||
        ((((uintptr_t)sample_cursor | (uintptr_t)channel_state) & 7) != 0)) {
        sample_end = sample_cursor + -1;
        do {
            sample_end = sample_end + 1;
            *sample_end = *channel_state;
        } while (sample_cursor + ((int)samples_in_chunk - 1) != sample_end);
    } else {
        sample_or_version = *channel_state;
        sample_end = sample_cursor;
        do {
            *sample_end = sample_or_version;
            sample_end[1] = sample_or_version;
            sample_end = sample_end + 2;
        } while (sample_cursor + ((uint32_t)samples_in_chunk & 0xfffffffe) != sample_end);
        if (((uint32_t)samples_in_chunk & 0xfffffffe) != samples_in_chunk) {
            sample_cursor[(uint32_t)samples_in_chunk & 0xfffffffe] = *channel_state;
        }
    }

    *header_flags = state->ftype;
    return LHDC_DEC_OK;
}

/* Read LPC warm-up samples, unpack residuals, and reconstruct integer PCM from channel prediction history. */
static int decode_v4_lpc_samples(lhdc_Decoder_t *state, uint32_t channel, uint32_t sample_count,
                                 int *sample_cursor, uint32_t *header_flags) {
    uint8_t header_byte;
    uint32_t bit_count_or_channel;
    uint32_t header_bits_or_residual;
    uint32_t next_bit_count;
    uint32_t bit_accumulator;
    uint32_t field_width_or_count;
    uint32_t *sample_word_cursor;
    uint32_t remaining_samples;
    uint32_t *word_cursor;
    bool ring_wrap;
    uint32_t samples_in_chunk;
    int *lpc_history;
    int *lpc_coefficients;
    int lpc_order;
    uint32_t header_bits;
    uint32_t *sample_words = (uint32_t *)sample_cursor;
    samples_in_chunk = sample_count;
    header_bits = *header_flags;
    bit_count_or_channel = channel;
    remaining_samples = sample_count;
    lpc_coefficients = state->pnQLpcVCoef[channel];
    lpc_history = state->pnLpcVPrevSample[channel] + 8;

    if (state->ck == 0) {
        if (bit_count_or_channel == 0) {
            bit_accumulator = 2;
        } else {
            bit_accumulator = 1;
        }
        lpc_order = state->puiLpcVOrder[channel];
        field_width_or_count = (uint32_t)state->bits_per_sample;
        if ((header_bits & 0x7f) >> 5 == bit_accumulator) {
            field_width_or_count = field_width_or_count + 8;
        }
        if (lpc_order < 1) {
            sample_cursor = sample_cursor + lpc_order;
            remaining_samples = (int)samples_in_chunk - lpc_order;
        } else {
            word_cursor = (uint32_t *)state->pnLpcVPrevSample[channel] + 7 - lpc_order;
            bit_accumulator = 0x20 - field_width_or_count;
            sample_word_cursor = sample_words - 1;

            for (int warmup_index = 0; warmup_index < lpc_order; ++warmup_index) {
                sample_word_cursor = sample_word_cursor + 1;
                *sample_word_cursor =
                    audio_math_shift_right_low8_zero32(state->bit_rb.db, bit_accumulator);
                next_bit_count = state->bit_rb.rb;
                header_bits_or_residual = field_width_or_count;
                if (next_bit_count < field_width_or_count) {
                    header_bits = audio_math_read_le32(state->bit_rb.in);
                    state->bit_rb.rb = 32;
                    state->bit_rb.db = header_bits;
                    header_bits_or_residual = field_width_or_count - next_bit_count;
                    *sample_word_cursor =
                        *sample_word_cursor |
                        audio_math_shift_right_low8_zero32(header_bits, (next_bit_count + 0x20) -
                                                                            field_width_or_count);
                    state->bit_rb.read_len = state->bit_rb.read_len + 4;
                    state->bit_rb.in += sizeof(uint32_t);
                    ring_wrap = state->bit_rb.in_max <= state->bit_rb.in;
                    next_bit_count = state->bit_rb.rb;
                    if (ring_wrap) {
                        state->bit_rb.in = state->bit_rb.pcb->cbuf;
                    }
                }
                state->bit_rb.rb = next_bit_count - header_bits_or_residual;
                state->bit_rb.db =
                    audio_math_shift_left_low8_zero32(state->bit_rb.db, header_bits_or_residual);
                header_bits_or_residual = audio_math_shift_arithmetic_low8_sign32(
                    audio_math_shift_left_low8_zero32(*sample_word_cursor, bit_accumulator),
                    bit_accumulator);
                *sample_word_cursor = header_bits_or_residual;
                if (state->bit_rb.rb == 0)
                    state->bit_rb.db = 0;
                ++word_cursor;
                *word_cursor = *sample_word_cursor;
            }
            sample_cursor = sample_cursor + lpc_order;
            remaining_samples = (int)samples_in_chunk - lpc_order;
        }
    } else {

        lpc_order = 0;
    }
    header_byte = state->lpc_hdr[channel];
    if ((header_byte & 0x40u) != 0) {
        bit_accumulator = lz_decode(state, bit_count_or_channel,
                                    (uint32_t)state->chunk_size / (uint32_t)samples_in_chunk,
                                    (int)remaining_samples, lpc_order, header_byte,
                                    state->lz_bytes_per_sample[channel], sample_cursor);
    } else {
        bit_accumulator =
            rice_decode(&state->bit_rb, (int)remaining_samples, header_byte & 0x1f, sample_cursor);
    }
    if (bit_accumulator != remaining_samples)
        return LHDC_V4_DEC_CHANNEL_REJECTED;
    bit_accumulator = state->bit_rb.rb & 7;
    if (bit_accumulator != 0) {
        if ((UINT32_C(0xff000000) << ((8 - bit_accumulator) & 0xff) & state->bit_rb.db) != 0)
            return LHDC_V4_DEC_NONZERO_PADDING;
        state->bit_rb.rb -= bit_accumulator;
        state->bit_rb.db <<= bit_accumulator;
    }
    if (8 >= (uint32_t)state->puiLpcVOrder[channel])
        lpcV_Ressw(sample_cursor, (int)samples_in_chunk - lpc_order, lpc_coefficients,
                   state->puiLpcVOrder[channel], state->pnQLpcVQuantizeShift[channel],
                   sample_cursor, lpc_history);
    *header_flags = state->ftype;
    return LHDC_DEC_OK;
}

/* Read difference warm-up samples and initialize history; return difference order and actual warm-up count. */
static uint32_t read_v4_difference_warmup(lhdc_Decoder_t *state, uint32_t channel,
                                          uint32_t frame_kind, uint32_t header_bits,
                                          int *sample_cursor, uint32_t *warmup_count) {
    uint32_t bit_count_or_channel;
    uint32_t header_bits_or_residual;
    uint32_t next_bit_count;
    uint32_t bit_accumulator;
    uint32_t field_width_or_count;
    uint32_t consumed_bytes_or_address;
    uint32_t *sample_word_cursor;
    uint32_t bit_shift;
    uint32_t *word_cursor;
    bool ring_wrap;
    uint32_t samples_in_chunk;
    uint32_t *sample_words = (uint32_t *)sample_cursor;
    int *difference_history = state->rs[channel];
    bit_count_or_channel = channel;
    samples_in_chunk = (uint32_t)state->chunk_size;
    bit_accumulator = frame_kind == 0xf ? 13 : (uint32_t)(uint8_t)(frame_kind - 2);
    if (state->ck == 0) {
        field_width_or_count = bit_accumulator;
        if (bit_accumulator != 0 && (state->lpc_hdr[channel] & 0x80u) == 0)
            field_width_or_count = 1;
        if (bit_count_or_channel == 0) {
            header_bits_or_residual = 2;
        } else {
            header_bits_or_residual = 1;
        }
        next_bit_count = (uint32_t)state->bits_per_sample;
        if ((header_bits & 0x7f) >> 5 == header_bits_or_residual) {
            next_bit_count = next_bit_count + 8;
        }
        if (field_width_or_count != 0) {
            sample_word_cursor = sample_words - 1;
            header_bits_or_residual = 0x20 - next_bit_count;
            word_cursor = sample_word_cursor + field_width_or_count;
            do {
                sample_word_cursor = sample_word_cursor + 1;
                *sample_word_cursor =
                    audio_math_shift_right_low8_zero32(state->bit_rb.db, header_bits_or_residual);
                bit_shift = state->bit_rb.rb;
                header_bits = next_bit_count;
                if (bit_shift < next_bit_count) {
                    header_bits = next_bit_count - bit_shift;
                    consumed_bytes_or_address = audio_math_read_le32(state->bit_rb.in);
                    state->bit_rb.rb = 32;
                    state->bit_rb.db = consumed_bytes_or_address;
                    consumed_bytes_or_address =
                        audio_math_shift_right_low8_zero32(consumed_bytes_or_address,
                                                           (bit_shift + 0x20) - next_bit_count) |
                        *sample_word_cursor;
                    *sample_word_cursor = consumed_bytes_or_address;
                    state->bit_rb.read_len = state->bit_rb.read_len + 4;
                    ring_wrap = state->bit_rb.in_max <= state->bit_rb.in + sizeof(uint32_t);
                    state->bit_rb.in += sizeof(uint32_t);
                    bit_shift = state->bit_rb.rb;
                    if (ring_wrap) {
                        state->bit_rb.in = state->bit_rb.pcb->cbuf;
                    }
                }
                state->bit_rb.rb = bit_shift - header_bits;
                state->bit_rb.db = audio_math_shift_left_low8_zero32(state->bit_rb.db, header_bits);
                *sample_word_cursor = audio_math_shift_arithmetic_low8_sign32(
                    audio_math_shift_left_low8_zero32(*sample_word_cursor, header_bits_or_residual),
                    header_bits_or_residual);
                if (state->bit_rb.rb == 0) {
                    state->bit_rb.db = 0;
                }
            } while (sample_word_cursor != word_cursor);
        }
        initialize_v4_differences(difference_history, bit_accumulator, sample_cursor,
                                  field_width_or_count);

    } else {
        field_width_or_count = 0;
    }
    *warmup_count = field_width_or_count;
    return bit_accumulator;
}

/* Dispatch raw, constant, LPC or difference channel reconstruction. */
static int decode_v4_channel(lhdc_Decoder_t *state, uint32_t channel, uint32_t sample_count,
                             int *sample_cursor, uint32_t *header_flags) {

    uint8_t header_byte;
    uint32_t bit_count_or_channel;
    uint32_t header_bits_or_residual;
    uint32_t bit_accumulator;
    int sample_or_version;
    uint32_t field_width_or_count;

    int *sample_end;
    uint8_t frame_kind;
    uint32_t remaining_samples;
    int predictor_or_block_limit;
    int *difference_history;
    uint32_t samples_in_chunk;

    uint32_t header_bits = *header_flags;
    bit_count_or_channel = channel;
    samples_in_chunk = sample_count;
    difference_history = state->rs[channel];
    header_byte = state->ctype[channel];
    frame_kind = header_byte & 0xf;
    if (frame_kind == 1)
        return decode_v4_raw_samples(state, sample_count, sample_cursor, header_flags);
    if (frame_kind == 0)
        return decode_v4_constant_samples(state, channel, sample_count, sample_cursor,
                                          header_flags);
    sample_end = sample_cursor;
    remaining_samples = samples_in_chunk;
    if (frame_kind == 0xf && state->lhdc_ver_code >= 300)
        return decode_v4_lpc_samples(state, channel, sample_count, sample_cursor, header_flags);
    bit_accumulator = read_v4_difference_warmup(state, channel, frame_kind, header_bits,
                                                sample_cursor, &field_width_or_count);
    sample_end = sample_cursor + field_width_or_count;
    remaining_samples = samples_in_chunk - field_width_or_count;
    header_byte = state->lpc_hdr[channel];
    if ((header_byte & 0x40u) != 0) {
        header_bits_or_residual = lz_decode(
            state, bit_count_or_channel, (uint32_t)state->chunk_size / (uint32_t)samples_in_chunk,
            (int)remaining_samples, field_width_or_count, header_byte,
            state->lz_bytes_per_sample[channel], sample_end);
    } else {
        header_bits_or_residual =
            rice_decode(&state->bit_rb, (int)remaining_samples, header_byte & 0x1f, sample_end);
    }
    if (header_bits_or_residual != remaining_samples) {
        return LHDC_V4_DEC_CHANNEL_REJECTED;
    }
    header_bits_or_residual = state->bit_rb.rb & 7;
    if (header_bits_or_residual != 0) {
        if ((UINT32_C(0xff000000) << ((8 - header_bits_or_residual) & 0xff) & state->bit_rb.db) !=
            0) {
            samples_in_chunk = 0xfffffffd;
            { return (uint32_t)samples_in_chunk; }
        }
        state->bit_rb.rb = state->bit_rb.rb - header_bits_or_residual;
        state->bit_rb.db = state->bit_rb.db << header_bits_or_residual;
    }
    if (bit_accumulator == 0 || field_width_or_count >= (unsigned)samples_in_chunk) {
        *header_flags = state->ftype;
        return LHDC_DEC_OK;
    }
    sample_end = sample_cursor + field_width_or_count - 1;
    while (field_width_or_count < (uint32_t)samples_in_chunk) {
        sample_or_version =
            integrate_v4_difference(difference_history, bit_accumulator, sample_end[1]);
        if (field_width_or_count == 0) {
            ++sample_end;
            *sample_cursor = audio_math_add_wrap_i32(sample_or_version, state->last_dat[channel]);
            field_width_or_count = 1;
        } else {
            predictor_or_block_limit = *sample_end;
            ++field_width_or_count;
            ++sample_end;
            *sample_end = audio_math_add_wrap_i32(predictor_or_block_limit, sample_or_version);
        }
    }
    *header_flags = state->ftype;
    return LHDC_DEC_OK;
}

/* Commit input consumed by a failed frame, preserving two-segment ring consumption and skip-frame flags. */
static void discard_v4_rejected_input(lhdc_Decoder_t *state) {
    uint8_t *dummy;
    uint32_t bit_count_or_channel;
    uint32_t header_bits_or_residual;
    uint32_t bit_accumulator;
    uint32_t field_width_or_count;
    cirbuf_s *bit_rb;
    bit_rb = state->bit_rb.pcb;
    state->data_skipped = '\x01';
    header_bits_or_residual = bit_rb->max_len;
    field_width_or_count = bit_rb->odx;
    bit_count_or_channel = state->bit_rb.read_len;
    bit_accumulator = header_bits_or_residual - field_width_or_count;
    if (bit_accumulator < bit_count_or_channel) {
        lhdc_cirbuf_get_no_copy(bit_rb, &dummy, bit_accumulator);
        bit_rb = state->bit_rb.pcb;
        bit_count_or_channel =
            (state->bit_rb.read_len + field_width_or_count) - header_bits_or_residual;
        state->bit_rb.read_len = bit_count_or_channel;
    }
    if (bit_count_or_channel != 0) {
        lhdc_cirbuf_get_no_copy(bit_rb, &dummy, bit_count_or_channel);
        bit_rb = state->bit_rb.pcb;
    }
}

/* Consume trailing zero padding; commit completed frames and resynchronize on nonzero bytes. */
static bool finish_v4_zero_padding(lhdc_Decoder_t *state) {
    int sample_or_version = state->dec_cnt;
    int predictor_or_block_limit = state->frame_len;
    uint32_t bit_accumulator = state->bit_rb.rb;
    uint32_t bit_count_or_channel = state->bit_rb.read_len;
    uint32_t field_width_or_count;
    uint8_t *input_word_end;
    for (;;) {

        sample_or_version = sample_or_version + 1;
        if (bit_accumulator < 8) {
            input_word_end = state->bit_rb.in + sizeof(uint32_t);
            field_width_or_count = audio_math_read_le32(state->bit_rb.in);
            bit_count_or_channel = bit_count_or_channel + 4;
            state->bit_rb.in = input_word_end;
            state->bit_rb.read_len = bit_count_or_channel;
            state->bit_rb.rb = 32;
            state->bit_rb.db = field_width_or_count;
            if (state->bit_rb.in_max <= input_word_end) {
                state->bit_rb.in = state->bit_rb.pcb->cbuf;
            }
            bit_accumulator = 0x18;
        } else {
            field_width_or_count = state->bit_rb.db;
            bit_accumulator = bit_accumulator - 8;
        }
        state->bit_rb.rb = bit_accumulator;
        state->bit_rb.db = field_width_or_count << 8;
        if (field_width_or_count >> 0x18 != 0)
            return false;
        state->dec_cnt = sample_or_version;
        if (sample_or_version == predictor_or_block_limit) {
            finish_v4_frame(state, bit_count_or_channel);
            return true;
        }
    }
}

/* Parse and decode LHDC frames from the input ring, update channel state, and return PCM output. */
int lhdc_dec_decode(lhdc_Decoder_t *state, int **out_pcm)

{
    short decoded_blocks;
    uint32_t bit_count_or_channel;
    uint32_t bit_accumulator;
    int sample_or_version;
    uint32_t field_width_or_count;
    uint8_t channel_block_type;

    uint8_t *ring_data;
    int predictor_or_block_limit;
    cirbuf_s *bit_rb;
    uint32_t header_bits;
    uint32_t samples_in_chunk;
    int *op[2];

    if (state->state == '\0')
        return (uint32_t)UINT32_MAX;
    bit_rb = state->bit_rb.pcb;

    do {
        bit_count_or_channel = state->bit_rb.rb;
        bit_accumulator = state->bit_rb.db;
        field_width_or_count = bit_rb->odx;
        ring_data = bit_rb->cbuf;
        state->bit_rb.saved_rb = bit_count_or_channel;
        state->bit_rb.saved_db = bit_accumulator;
        state->bit_rb.in = ring_data + field_width_or_count;
        state->bit_rb.read_len = 0;
        if (state->ck == 0) {
            int header_result = read_v4_frame_header(state, &header_bits);
            if (header_result <= 0)
                return header_result;
            bit_rb = state->bit_rb.pcb;
        } else {
            header_bits = state->ftype;
        }
        samples_in_chunk = (uint32_t)state->chunk_size;
        channel_block_type = state->ups;
        if ((header_bits & 0x80) == 0) {
            if (channel_block_type == '\0') {
                channel_block_type = '\x01';
            } else if (channel_block_type == '\x01') {
                channel_block_type = '\x02';
            } else {
                channel_block_type = '\x03';
            }
            state->ups = channel_block_type;
            op[0] = state->up2x_pcm_buf[0];
            op[1] = state->up2x_pcm_buf[1];
            samples_in_chunk = ((int)samples_in_chunk >> 1);
        } else {
            if (channel_block_type == '\x03') {
                state->ups = '\x05';
            } else if (channel_block_type == '\x05') {
                state->ups = '\0';
            }
            op[1] = *out_pcm;
            op[0] = op[1];
            if ((header_bits & 0x60) != 0x60) {
                op[1] = out_pcm[1];
            }
        }
        bool retry_block = false;
        bit_count_or_channel = state->dec_ch;
        if (bit_count_or_channel < 2) {
            do {
                int channel_result =
                    decode_v4_channel(state, bit_count_or_channel, (uint32_t)samples_in_chunk,
                                      op[bit_count_or_channel], &header_bits);
                if (channel_result < 0)
                    return channel_result;
                if (channel_result != 0) {
                    retry_block = true;
                    break;
                }
                bit_accumulator = header_bits & 0x60;
                if (bit_accumulator == 0x60)
                    break;
                ++bit_count_or_channel;
            } while (bit_count_or_channel != 2);
        } else {
            bit_accumulator = header_bits & 0x60;
        }
        if (!retry_block) {
            bool cache_only;
            samples_in_chunk = restore_v4_output(state, op, out_pcm, (uint32_t)samples_in_chunk,
                                                 header_bits, bit_accumulator, &cache_only);
            if (cache_only)
                return (uint32_t)samples_in_chunk;
            bit_count_or_channel = state->bit_rb.read_len;
            bit_accumulator = state->bit_rb.rb;
            predictor_or_block_limit = state->frame_len;
            sample_or_version =
                (state->dec_cnt + (state->bit_rb.saved_rb >> 3) + bit_count_or_channel) -
                (bit_accumulator >> 3);
            state->dec_cnt = sample_or_version;
            if (sample_or_version <= predictor_or_block_limit) {
                decoded_blocks = state->ck;
                state->ck = decoded_blocks + 1;
                if (((short)state->chunk_cnt != (short)(decoded_blocks + 1)) ||
                    (state->ck = 0, predictor_or_block_limit <= sample_or_version)) {
                    finish_v4_frame(state, bit_count_or_channel);
                    { return (uint32_t)samples_in_chunk; }
                }
                if (finish_v4_zero_padding(state))
                    return (uint32_t)samples_in_chunk;
            }
        }
        if (state->state != '\x01') {
            samples_in_chunk = 0xfffffffe;
            { return (uint32_t)samples_in_chunk; }
        }

        discard_v4_rejected_input(state);
        state->bit_rb.read_len = 0;
    } while (true);
}
#include "../include/lhdc_dec.h"
