#include <audio_math/byte_order.h>
#include "../../../include/lhdc/decoder/status.h"
#include "../include/lhdc_arith.h"
#include "../include/lhdc_dec_control.h"
#include "../include/lhdc_decode.h"
#include "../include/lhdc_tables.h"
#include <stdbool.h>
#include <audio_math/bits.h>
#include <audio_math/integer.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "frame_decode_tables.h"
#include "frame_decode.h"

#define DAT_00011670 0x1.0000000000000p+11f
enum { DECODE_GAIN_FRACTION_BITS = 11 };
/* Exhausted bit reads leave output fields untouched; callers must check status before using unassigned locals. */

/* Read an unsigned field, prefetching after an exact word boundary; exhaustion returns -2 without changing output. */
static int read_decode_field(nbyte_read_struct *bit_inputter, unsigned width, uint32_t *field) {
    if (width == 0) {
        *field = 0;
        return LHDC_DEC_OK;
    }
    if (bit_inputter->dec_r_mem >= bit_inputter->dec_r_finish)
        return LHDC_V5_DEC_INPUT_EXHAUSTED;
    unsigned used = bit_inputter->dec_r_num;
    uint32_t value = bit_inputter->dec_r_d32_l & (UINT32_MAX >> used);
    if (width < 32 - used) {
        value >>= 32 - used - width;
        bit_inputter->dec_r_num = used + width;
    } else {
        unsigned extra = used - 32 + width;
        ++bit_inputter->dec_r_mem;
        bit_inputter->dec_r_d32_l = audio_math_read_be32(bit_inputter->dec_r_mem);
        bit_inputter->dec_r_num = extra;
        if (extra != 0)
            value = (value << extra) | (bit_inputter->dec_r_d32_l >> (32 - extra));
    }
    *field = value & (UINT32_MAX >> (32 - width));
    return LHDC_DEC_OK;
}

/* Decode a Rice quotient from three-symbol arithmetic coding, handling two-bit extensions and up to three consecutive ones.
 * False skips subsequent synthesis; true reports arithmetic status through status. */
static bool read_rice_quotient(dec_arith_struct *dec_arith, nbyte_read_struct *bit_inputter,
                               uint32_t *quotient, uint32_t *status) {
    uint32_t symbol;
    *status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
    if (*status != 0) {
        if (symbol != 0)
            return false;
        *quotient = 0;
        return true;
    }
    uint32_t low_digits = 0, digit_count = 0, short_code = 0;
    for (;;) {
        if (symbol == 2) {
            uint32_t high_status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
            uint32_t pair = symbol << 1;
            uint32_t low_status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
            pair |= symbol;
            *status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
            *status |= high_status | low_status;
            low_digits += audio_math_shift_left_low8_zero32(pair, digit_count * 2);
            ++digit_count;
            if (*status != 0)
                break;
            continue;
        }
        if (symbol != 1) {
            *status = 0;
            if (symbol != 0)
                return false;
            break;
        }
        *status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
        short_code = 1;
        if (symbol == 0)
            break;
        if (symbol != 1 || *status != 0)
            return false;
        *status = lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, &symbol);
        short_code = 2;
        if (symbol == 0)
            break;
        if (symbol != 1 || *status != 0)
            return false;
        short_code = 3;
        break;
    }
    if (*status != 0 && symbol != 0)
        return false;
    if (audio_math_int32_from_bits(digit_count) > 0)
        short_code =
            audio_math_shift_left_low8_zero32(short_code + 1, digit_count * 2) + low_digits;
    *quotient = short_code;
    return true;
}

/* Parse spectral headers/active lines, initialize tail budget and arithmetic state, and track word-prefetch exhaustion. */
static int skip_spectral_padding(nbyte_read_struct *bit_inputter, int used_bits, int target_bits) {
    while (used_bits < target_bits) {
        const unsigned width = target_bits - used_bits > 8 ? 8 : 1;
        if (bit_inputter->dec_r_mem >= bit_inputter->dec_r_finish)
            return LHDC_V5_DEC_INPUT_EXHAUSTED;
        unsigned next_count = bit_inputter->dec_r_num + width;
        if (next_count >= 32) {
            ++bit_inputter->dec_r_mem;
            bit_inputter->dec_r_d32_l = audio_math_read_be32(bit_inputter->dec_r_mem);
            next_count -= 32;
        }
        bit_inputter->dec_r_num = next_count;
        used_bits += width;
    }
    return LHDC_DEC_OK;
}


/* Read global gain, SNS mode, active lines, and noise scale, preserving aggregated errors. */
static int read_v5_spectral_header(tdata_all_buffer_struct *tdata_all_buffer,
                                   nbyte_read_struct *bit_inputter, V5ChannelFrame *frame) {
    uint8_t *jump;
    uint32_t segment_jump_bit;
    uint32_t noise_scale_bits;
    /* Header order: global gain, SNS mode/direction, active spectral-pair count, prediction mode, initial mantissa. */
    jump = (tdata_all_buffer->buf).bpc.lossy_jump_adus_parameter;

    frame->header_status = read_decode_field(bit_inputter, 9, &frame->offset_index);
    frame->frequency_shift = 8;
    if (frame->header_status == 0 && bit_inputter->dec_r_mem < bit_inputter->dec_r_finish) {
        uint32_t sns_mode;
        read_decode_field(bit_inputter, 4, &sns_mode);
        frame->frequency_shift = (sns_mode & 15) + 8;
        for (int segment_index = 0;
             segment_index < tdata_all_buffer->segment_setting.segment_num - 1; ++segment_index) {
            frame->header_status = read_decode_field(bit_inputter, 1, &segment_jump_bit);
            jump[segment_index] = (uint8_t)segment_jump_bit;
            if (frame->header_status != 0)
                break;
        }
    } else {
        frame->header_status = (uint32_t)LHDC_V5_DEC_INPUT_EXHAUSTED;
    }
    if (frame->frequency_shift != 23)
        lhdc_dec_freq_shift(&tdata_all_buffer->segment_setting, jump, frame->frequency_shift,
                            tdata_all_buffer->buf.bpc.segment);

    frame->spectral_count = 2;
    if (tdata_all_buffer->valid_num != 0) {
        uint32_t encoded_pairs;
        if (read_decode_field(bit_inputter, tdata_all_buffer->valid_num, &encoded_pairs) == 0)
            frame->spectral_count = (encoded_pairs + 1) * 2;
        else
            frame->header_status = (uint32_t)LHDC_V5_DEC_INPUT_EXHAUSTED;
    }
    if (tdata_all_buffer->frame_samples < audio_math_int32_from_bits(frame->spectral_count))
        return LHDC_V5_DEC_INVALID_FRAME;
    if (read_decode_field(bit_inputter, 1, &frame->frequency_mode_bit) != 0)
        frame->header_status = (uint32_t)LHDC_V5_DEC_INPUT_EXHAUSTED;
    frame->noise_scale_q7 = 0;
    if (tdata_all_buffer->tdata_quant_rem_data_size != 0) {
        if (read_decode_field(bit_inputter, tdata_all_buffer->tdata_quant_rem_data_size,
                              &noise_scale_bits) != 0)
            frame->header_status = (uint32_t)LHDC_V5_DEC_INPUT_EXHAUSTED;
        frame->noise_scale_q7 = noise_scale_bits << 7;
    }

    /* Snapshot the arithmetic-stream start for later budgets; exhaustion leaves the workspace seed unchanged. */
    return LHDC_DEC_OK;
}

/* Decode two passes of spectral quotients and save cached low bits after FAC stops. */
static bool decode_v5_spectral_quotients(tdata_all_buffer_struct *tdata_all_buffer,
                                         nbyte_read_struct *bit_inputter,
                                         dec_arith_struct *dec_arith, const V5ChannelFrame *frame,
                                         V5ResidualBits *bits) {
    uint32_t *bit_state_or_sample_cursor;
    int segment_index_or_sample;
    uint32_t cached_bits;
    uint32_t swapped_word_or_bit_count;
    uint32_t bits_used_or_sample_count;
    uint32_t input_word_or_address;
    uint32_t *quantized_symbols;
    quantized_symbols = tdata_all_buffer->buf.bpc.tdata_q_quant_from_symbol;
    bits->bit_mask_or_shift = frame->spectral_count;
    if (tdata_all_buffer->segment_setting.initial_spectral_limit <=
        audio_math_int32_from_bits(frame->spectral_count))
        bits->bit_mask_or_shift = tdata_all_buffer->segment_setting.initial_spectral_limit;
    read_decode_field(bit_inputter, 32, &dec_arith->dec_arth_para_0);

    dec_arith->dec_arth_para_1 = 0xffffffff;
    swapped_word_or_bit_count = 0;
    input_word_or_address = frame->spectral_count & 0xfffffffc;
    dec_arith->dec_arth_para_2 = 0;
    lhdc_dec_arith_program(dec_arith, (uint32_t *)tdata_all_buffer->init_fq_tbl_01, 3,
                           (tdata_all_buffer->segment_setting).arith_init_0);

    bits_used_or_sample_count = 0;
    bit_state_or_sample_cursor = quantized_symbols;
    if (0 < audio_math_int32_from_bits(input_word_or_address)) {
        do {
            lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, bit_state_or_sample_cursor);
            if (*bit_state_or_sample_cursor == 2) {
                bits_used_or_sample_count = swapped_word_or_bit_count;
            }
            lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, bit_state_or_sample_cursor + 1);
            if (bit_state_or_sample_cursor[1] == 2) {
                bits_used_or_sample_count = swapped_word_or_bit_count + 1;
            }
            lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, bit_state_or_sample_cursor + 2);
            if (bit_state_or_sample_cursor[2] == 2) {
                bits_used_or_sample_count = swapped_word_or_bit_count + 2;
            }
            lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, bit_state_or_sample_cursor + 3);
            if (bit_state_or_sample_cursor[3] == 2) {
                bits_used_or_sample_count = swapped_word_or_bit_count + 3;
            }
            swapped_word_or_bit_count = swapped_word_or_bit_count + 4;
            bit_state_or_sample_cursor = bit_state_or_sample_cursor + 4;
        } while (audio_math_int32_from_bits(swapped_word_or_bit_count) <
                 audio_math_int32_from_bits(input_word_or_address));
        segment_index_or_sample = ((input_word_or_address - 1) >> 2) + 1;
        swapped_word_or_bit_count = segment_index_or_sample * 4;
        bit_state_or_sample_cursor = quantized_symbols + segment_index_or_sample * 4;
    }
    if (audio_math_int32_from_bits(swapped_word_or_bit_count) <
        audio_math_int32_from_bits(frame->spectral_count)) {
        do {
            lhdc_dec_arith_symbol_3(dec_arith, bit_inputter, bit_state_or_sample_cursor);
            cached_bits = *bit_state_or_sample_cursor;
            bit_state_or_sample_cursor = bit_state_or_sample_cursor + 1;
            if (cached_bits == 2) {
                bits_used_or_sample_count = swapped_word_or_bit_count;
            }
            swapped_word_or_bit_count = swapped_word_or_bit_count + 1;
        } while (swapped_word_or_bit_count != frame->spectral_count);
    }
    if (audio_math_int32_from_bits(bits_used_or_sample_count) <
        audio_math_int32_from_bits(frame->spectral_count - bits->bit_mask_or_shift)) {
        bits_used_or_sample_count = frame->spectral_count;
    }
    lhdc_dec_arith_program(dec_arith, (uint32_t *)tdata_all_buffer->init_fq_tbl_02, 3,
                           (tdata_all_buffer->segment_setting).arith_init_1);
    /* First-pass marker 2 separates second-pass quotient coding from low-bit merging. */
    const int quotient_pivot = audio_math_int32_from_bits(bits_used_or_sample_count);
    bool arithmetic_stopped_early = false;
    for (int coefficient_index = 0;
         coefficient_index < audio_math_int32_from_bits(frame->spectral_count);
         ++coefficient_index) {
        uint32_t *coefficient = quantized_symbols + coefficient_index;
        if (coefficient_index > quotient_pivot || *coefficient == 2) {
            uint32_t quotient;
            if (!read_rice_quotient(dec_arith, bit_inputter, &quotient, &bits->bit_mask_or_shift))
                return false;
            *coefficient =
                coefficient_index > quotient_pivot ? *coefficient + quotient * 2 : quotient + 2;
            if (bits->bit_mask_or_shift != 0) {
                arithmetic_stopped_early = true;
                break;
            }
        }
    }
    if (!arithmetic_stopped_early)
        bits->bit_mask_or_shift = lhdc_dec_arith_stop(dec_arith, &bits->bindata);
    if (audio_math_int32_from_bits(bits->bit_mask_or_shift) < 0)
        return false;
    return true;
}

/* Consume FAC-cached low bits first, then fill one residual field from the bitstream if necessary. */
static int read_v5_residual_field(nbyte_read_struct *bit_inputter, V5ResidualBits *bits,
                                  uint32_t width, uint32_t *value) {
    if (audio_math_int32_from_bits(bits->bit_mask_or_shift) < 1)
        return read_decode_field(bit_inputter, width, value);
    if (audio_math_int32_from_bits(bits->bit_mask_or_shift) < audio_math_int32_from_bits(width)) {
        uint32_t needed = width - bits->bit_mask_or_shift;
        int status = read_decode_field(bit_inputter, needed, value);
        uint32_t cached = bits->bit_mask_or_shift & 0xff;
        bits->bit_mask_or_shift = 0;
        *value |= audio_math_shift_left_low8_zero32(
            bits->bindata & (audio_math_shift_left_low8_zero32(1u, cached) - 1u), needed);
        return status;
    }
    bits->bit_mask_or_shift -= width;
    *value = audio_math_shift_right_low8_zero32(bits->bindata, bits->bit_mask_or_shift) &
             (audio_math_shift_left_low8_zero32(1u, width) - 1u);
    return LHDC_DEC_OK;
}

/* Merge spectral quotients, low bits, and signs in reverse order; validate residual budget and actual input consumption. */
static bool read_v5_spectral_residuals(tdata_all_buffer_struct *tdata_all_buffer,
                                       nbyte_read_struct *bit_inputter, int *output_buffer,
                                       V5ChannelFrame *frame, V5ResidualBits *bits) {
    uint32_t *bit_state_or_sample_cursor;
    int segment_index_or_sample;
    uint32_t swapped_word_or_bit_count;
    int decoded_value_or_limit;
    uint32_t bits_used_or_sample_count;
    int coefficient_index;
    int result_or_sample;
    int interpolated_scale_or_sample;
    uint32_t coefficient_bit_total;
    int bit_budget;
    uint32_t coefficient_sign_bit;
    uint32_t residual_coefficient;
    bit_state_or_sample_cursor = (uint32_t *)output_buffer + frame->spectral_count - 1;
    if (frame->frequency_mode_bit == 0) {
        segment_index_or_sample = 5;
        decoded_value_or_limit = 3;
    } else {
        segment_index_or_sample = 1;
        decoded_value_or_limit = 7;
    }
    bool all_coefficients_read = false;
    if (audio_math_int32_from_bits(frame->spectral_count) < 1) {
        result_or_sample = 0;
        bit_budget = 0;
    } else {
        coefficient_index = 0;
        coefficient_bit_total = 0;
        bit_budget = 0;
        interpolated_scale_or_sample = frame->noise_scale_q7;
        do {
            frame->noise_scale_q7 = audio_math_shift_right_bounded_i32(
                audio_math_int32_from_bits(
                    (uint32_t)segment_index_or_sample * (uint32_t)interpolated_scale_or_sample +
                    (uint32_t)decoded_value_or_limit * (uint32_t)frame->noise_scale_q7),
                3);
            bits_used_or_sample_count = audio_math_shift_right_bounded_i32(
                audio_math_add_wrap_i32(frame->noise_scale_q7, 0x40), 7);
            coefficient_bit_total = coefficient_bit_total + bits_used_or_sample_count;
            result_or_sample = read_v5_residual_field(bit_inputter, bits, bits_used_or_sample_count,
                                                      &residual_coefficient);
            bits_used_or_sample_count =
                audio_math_shift_left_low8_zero32(
                    (tdata_all_buffer->buf).bpc.tdata_q_quant_from_symbol[coefficient_index],
                    bits_used_or_sample_count) |
                residual_coefficient;
            if (bits_used_or_sample_count == 0) {
                *bit_state_or_sample_cursor = 0;
                interpolated_scale_or_sample = 0;
                residual_coefficient = 0;
            } else {

                swapped_word_or_bit_count =
                    32u - audio_math_count_leading_zeros32(bits_used_or_sample_count);
                interpolated_scale_or_sample = swapped_word_or_bit_count << 7;
                if (audio_math_int32_from_bits(bits->bit_mask_or_shift) < 1) {
                    if (read_decode_field(bit_inputter, 1, &coefficient_sign_bit) != 0)
                        result_or_sample = -2;
                } else {
                    bits->bit_mask_or_shift = bits->bit_mask_or_shift - 1;
                    coefficient_sign_bit =
                        audio_math_shift_right_low8_zero32(bits->bindata, bits->bit_mask_or_shift) &
                        1;
                }
                bit_budget = bit_budget + 1;
                residual_coefficient =
                    (-coefficient_sign_bit ^ bits_used_or_sample_count) + coefficient_sign_bit;
                *bit_state_or_sample_cursor = residual_coefficient;
            }
            if (coefficient_index + 1U == frame->spectral_count) {
                all_coefficients_read = true;
                break;
            }
            bit_state_or_sample_cursor = bit_state_or_sample_cursor + -1;
            coefficient_index = coefficient_index + 1;
        } while (result_or_sample == 0);
        bit_budget =
            audio_math_add_wrap_i32(audio_math_int32_from_bits(coefficient_bit_total), bit_budget);
        if (!all_coefficients_read && 0x17 < bit_budget)
            return false;
    }
    if (bit_budget < 24) {
        bits->bit_mask_or_shift = (0x18 - bits->bit_mask_or_shift) - bit_budget;
        if (bits->bit_mask_or_shift != 0 &&
            read_decode_field(bit_inputter, bits->bit_mask_or_shift, &bits->bindata) != 0)
            return false;
    }
    if ((result_or_sample != 0) ||
        (bits->bit_mask_or_shift =
             ((bit_inputter->dec_r_num + (uint32_t)((uintptr_t)bit_inputter->dec_r_mem -
                                                    (uintptr_t)bit_inputter->dec_r_begin) *
                                             8) -
              frame->cached_bit_count) +
             (uint32_t)((uintptr_t)frame->input_word_cursor - (uintptr_t)frame->input_begin) * -8,
         audio_math_int32_from_bits(bits->bit_mask_or_shift) < 1))
        return false;
    return true;
}

/* Apply SNS shifts and fill consecutive zero lines using the instance seed. */
static void fill_v5_spectral_noise(tdata_all_buffer_struct *tdata_all_buffer, uint16_t *nf_seed,
                                   uint32_t *time_data, uint32_t spectral_count,
                                   int frequency_shift, float offset_scale) {
    float signed_noise;
    uint32_t *bit_state_or_sample_cursor;
    int segment_index_or_sample;
    int decoded_value_or_limit;
    int target_range_or_clipped_sample;
    int coefficient_index;
    int result_or_sample;
    int interpolated_scale_or_sample;
    if (frequency_shift != 0x17) {
        lhdc_freq_shift_apply_decode(
            &tdata_all_buffer->segment_setting, tdata_all_buffer->power_of_2_tbl,
            (tdata_all_buffer->buf).bpc.segment, (int *)time_data, spectral_count);
        offset_scale = offset_scale * (tdata_all_buffer->segment_setting).noise_fill_scale;
        segment_index_or_sample = tdata_all_buffer->segment_cutoff;

        target_range_or_clipped_sample = tdata_all_buffer->frame_samples / 20;
        while (target_range_or_clipped_sample < segment_index_or_sample + -2) {
            decoded_value_or_limit = target_range_or_clipped_sample + -2;
            if (decoded_value_or_limit < segment_index_or_sample) {
                bit_state_or_sample_cursor = time_data + target_range_or_clipped_sample - 3;
                do {
                    bit_state_or_sample_cursor = bit_state_or_sample_cursor + 1;
                    if (*bit_state_or_sample_cursor != 0)
                        break;
                    decoded_value_or_limit = decoded_value_or_limit + 1;
                } while (segment_index_or_sample != decoded_value_or_limit);
            }
            interpolated_scale_or_sample = target_range_or_clipped_sample + 2;
            result_or_sample = target_range_or_clipped_sample;
            if ((interpolated_scale_or_sample < decoded_value_or_limit) &&
                (coefficient_index = target_range_or_clipped_sample,
                 result_or_sample = interpolated_scale_or_sample,
                 target_range_or_clipped_sample < decoded_value_or_limit + -2)) {
                do {
                    uint32_t noise_word = (uint32_t)*nf_seed * UINT32_C(0x320b) + UINT32_C(0x41c5);
                    result_or_sample = audio_math_int32_from_bits(noise_word);
                    *nf_seed = (uint16_t)result_or_sample;
                    signed_noise = offset_scale;
                    if ((noise_word & UINT32_C(0x8000)) != 0) {
                        signed_noise = -offset_scale;
                    }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                    time_data[coefficient_index] = audio_math_truncate_i32_f32(signed_noise);
#else
                    time_data[coefficient_index] = (int)signed_noise;
#endif
                    coefficient_index = coefficient_index + 2;
                } while (coefficient_index < decoded_value_or_limit + -2);
                result_or_sample =
                    (((-3 - target_range_or_clipped_sample) + decoded_value_or_limit) &
                     0xfffffffeU) +
                    interpolated_scale_or_sample + 2;
            }
            target_range_or_clipped_sample = result_or_sample + 1;
        }
    }
}

/* Synthesize time-domain samples, then round, clamp, and write PCM for the output depth. */
static void synthesize_v5_pcm(tdata_all_buffer_struct *tdata_all_buffer, uint32_t *time_data,
                              int *output_buffer) {
    const unsigned pcm_shift = tdata_all_buffer->offset_num;
    const uint32_t rounding_bias = tdata_all_buffer->offset_num_value_half;
    lhdc_dec_lossy_time_operation(tdata_all_buffer->time_mem, (int *)time_data, 1);
    const int pcm_top = tdata_all_buffer->sample_max;
    const int pcm_bottom = tdata_all_buffer->sample_min;
    for (int sample_index = 0; sample_index < tdata_all_buffer->frame_samples; ++sample_index) {
        int sample = audio_math_shift_right_i32(
            audio_math_int32_from_bits(time_data[sample_index] + rounding_bias), pcm_shift & 0xff);
        if (sample > pcm_top)
            sample = pcm_top;
        else if (sample < pcm_bottom)
            sample = pcm_bottom;
        output_buffer[sample_index] = sample;
    }
}

/* Cache global gain, scale active spectral lines in Q11, and clear the tail. */
static float scale_v5_spectrum(tdata_all_buffer_struct *tdata_all_buffer, uint32_t *time_data,
                               int *output_buffer, uint32_t spectral_count, uint32_t offset_index) {
    uint32_t *bit_state_or_sample_cursor;
    int segment_index_or_sample;
    int *output_cursor;
    uint32_t swapped_word_or_bit_count;
    uint32_t bits_used_or_sample_count;
    uint32_t input_word_or_address;
    uint32_t *input_end_or_cursor;
    float offset_scale;
    int64_t scale_q11;
    offset_scale = tdata_all_buffer->offset_table[offset_index];
    if (offset_scale == 0.0) {
        if (tdata_all_buffer->offset_table_start < audio_math_int32_from_bits(offset_index)) {
            offset_scale =
                lhdc_v5_dec_gain_offset(offset_index, tdata_all_buffer->offset_table_start,
                                        tdata_all_buffer->offset_table_jump);
            offset_scale = offset_scale < LHDC_FAST_EXP2_BOUNDED_LIMIT_F32
                               ? lhdc_approx_exp2_fused_bounded_f32(offset_scale)
                               : lhdc_approx_exp2_fused_f32(offset_scale);
        } else {
            offset_scale = (float)audio_math_int32_from_bits(
                audio_math_shift_left_low8_zero32(1u, offset_index));
        }
        tdata_all_buffer->offset_table[offset_index] = offset_scale;
    }
    input_word_or_address = spectral_count & 0xfffffffcu;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Multiplication by 2^11 is exact; Q11 requires int64 and must not be narrowed to int32. */
    scale_q11 = audio_math_truncate_i64_f32(offset_scale * DAT_00011670);
#else
    scale_q11 = (long long)(offset_scale * DAT_00011670);
#endif
    bit_state_or_sample_cursor = time_data;
    output_cursor = output_buffer;
    if (0 < audio_math_int32_from_bits(input_word_or_address)) {
        segment_index_or_sample = 0;
        do {
            *bit_state_or_sample_cursor = audio_math_multiply_shift_wrap_i32(
                scale_q11, *output_cursor, DECODE_GAIN_FRACTION_BITS);
            bit_state_or_sample_cursor[1] = audio_math_multiply_shift_wrap_i32(
                scale_q11, output_cursor[1], DECODE_GAIN_FRACTION_BITS);
            segment_index_or_sample = segment_index_or_sample + 4;
            bit_state_or_sample_cursor[2] = audio_math_multiply_shift_wrap_i32(
                scale_q11, output_cursor[2], DECODE_GAIN_FRACTION_BITS);
            bit_state_or_sample_cursor[3] = audio_math_multiply_shift_wrap_i32(
                scale_q11, output_cursor[3], DECODE_GAIN_FRACTION_BITS);
            bit_state_or_sample_cursor = bit_state_or_sample_cursor + 4;
            output_cursor = output_cursor + 4;
        } while (segment_index_or_sample < audio_math_int32_from_bits(input_word_or_address));
        bit_state_or_sample_cursor = time_data + input_word_or_address;
        output_cursor = output_buffer + input_word_or_address;
    }
    if ((spectral_count & 3) != 0) {
        *bit_state_or_sample_cursor = audio_math_multiply_shift_wrap_i32(scale_q11, *output_cursor,
                                                                         DECODE_GAIN_FRACTION_BITS);
        bit_state_or_sample_cursor[1] = audio_math_multiply_shift_wrap_i32(
            scale_q11, output_cursor[1], DECODE_GAIN_FRACTION_BITS);
        bit_state_or_sample_cursor = bit_state_or_sample_cursor + (spectral_count & 3);
    }
    bits_used_or_sample_count = tdata_all_buffer->frame_samples - spectral_count;
    input_word_or_address = bits_used_or_sample_count & 0xfffffffc;
    bits_used_or_sample_count = bits_used_or_sample_count & 3;
    if (0 < audio_math_int32_from_bits(input_word_or_address)) {
        swapped_word_or_bit_count = input_word_or_address - 1;
        if ((((uintptr_t)bit_state_or_sample_cursor & 7) == 0) &&
            (0x13 < swapped_word_or_bit_count)) {
            input_word_or_address = 0;
            input_end_or_cursor = bit_state_or_sample_cursor;
            do {
                input_word_or_address = input_word_or_address + 1;
                *input_end_or_cursor = 0;
                input_end_or_cursor[1] = 0;
                input_end_or_cursor[2] = 0;
                input_end_or_cursor[3] = 0;
                input_end_or_cursor = input_end_or_cursor + 4;
            } while (input_word_or_address < (swapped_word_or_bit_count >> 2) + 1);
        } else {
            segment_index_or_sample = 0;
            input_end_or_cursor = bit_state_or_sample_cursor;
            do {
                segment_index_or_sample = segment_index_or_sample + 4;
                *input_end_or_cursor = 0;
                input_end_or_cursor[1] = 0;
                input_end_or_cursor[2] = 0;
                input_end_or_cursor[3] = 0;
                input_end_or_cursor = input_end_or_cursor + 4;
            } while (segment_index_or_sample < audio_math_int32_from_bits(input_word_or_address));
        }
        bit_state_or_sample_cursor =
            bit_state_or_sample_cursor + ((swapped_word_or_bit_count >> 2) + 1) * 4;
    }
    if (((bits_used_or_sample_count != 0) &&
         (*bit_state_or_sample_cursor = 0, bits_used_or_sample_count != 1)) &&
        (bit_state_or_sample_cursor[1] = 0, bits_used_or_sample_count == 3)) {
        bit_state_or_sample_cursor[2] = 0;
    }

    return offset_scale;
}

/* Apply remaining correction bits to nonzero lines, aggregate exhaustion, and update the bit budget. */
static bool restore_v5_residual_refinement(tdata_all_buffer_struct *tdata_all_buffer,
                                           nbyte_read_struct *bit_inputter, V5ChannelFrame *frame,
                                           V5ResidualBits *bits, uint32_t *time_data,
                                           float offset_scale) {
    uint32_t *bit_state_or_sample_cursor;
    int segment_index_or_sample;
    int decoded_value_or_limit;
    uint32_t bits_used_or_sample_count;
    uint32_t input_word_or_address;
    uint32_t residual_read_status;
    uint32_t residual_present_bit;
    segment_index_or_sample = tdata_all_buffer->target_range;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    decoded_value_or_limit = audio_math_truncate_i32_f64((double)offset_scale * 0.25);
#else
    decoded_value_or_limit = (int)(offset_scale * 0.25);
#endif
    if ((audio_math_int32_from_bits(bits->bit_mask_or_shift) < segment_index_or_sample) &&
        (0 < audio_math_int32_from_bits(frame->spectral_count))) {
        bits_used_or_sample_count = 0;

        bit_state_or_sample_cursor = time_data;
        residual_read_status = 0;
        do {
            input_word_or_address = *bit_state_or_sample_cursor;
            if (input_word_or_address == 0) {
                if (segment_index_or_sample <= audio_math_int32_from_bits(bits->bit_mask_or_shift))
                    break;
            } else {
                if (read_decode_field(bit_inputter, 1, &residual_present_bit) != 0)
                    residual_read_status = 0xfffffffe;
                input_word_or_address = *bit_state_or_sample_cursor;
                segment_index_or_sample = decoded_value_or_limit;
                if ((residual_present_bit == 0 &&
                     audio_math_int32_from_bits(input_word_or_address) < 1) ||
                    (residual_present_bit != 0 &&
                     audio_math_int32_from_bits(input_word_or_address) > 0))
                    segment_index_or_sample = -decoded_value_or_limit;
                *bit_state_or_sample_cursor = input_word_or_address + segment_index_or_sample;
                segment_index_or_sample = tdata_all_buffer->target_range;
                bits->bit_mask_or_shift = bits->bit_mask_or_shift + 1;
                if (segment_index_or_sample <= audio_math_int32_from_bits(bits->bit_mask_or_shift))
                    break;
            }
            bits_used_or_sample_count = bits_used_or_sample_count + 1;
            ++bit_state_or_sample_cursor;
        } while (bits_used_or_sample_count != frame->spectral_count);
        if ((residual_read_status != 0) ||
            (residual_read_status = bits->bit_mask_or_shift,
             audio_math_int32_from_bits(bits->bit_mask_or_shift) < 1)) {
            frame->header_status = frame->header_status | (uint32_t)residual_read_status;
            return false;
        }
    }
    return true;
}

/* Decode a V5 channel through header, entropy, scale, noise and PCM stages. */
int lhdc_dec_top(void *heap_memory, uint32_t *input_buffer, int input_buffer_bytes,
                 int *output_buffer, int channel)

{
    tdata_all_buffer_struct *tdata_all_buffer;
    nbyte_read_struct *bit_inputter;
    dec_arith_struct *dec_arith;
    uint16_t *nf_seed;
    int target_range_or_clipped_sample;
    float offset_scale;
    uint32_t result;

    (void)channel;
    tdata_all_buffer = lhdc_v5_dec_get_mem_content_addr(heap_memory, DBUFFER_INDEX);
    if (((input_buffer == (uint32_t *)0x0) ||
         (tdata_all_buffer == (tdata_all_buffer_struct *)0x0)) ||
        (output_buffer == (int *)0x0)) {
        result = 0xffffffff;
        return result;
    }
    if ((tdata_all_buffer->channel_frame_bytes != input_buffer_bytes) &&
        (result = lhdc_dec_lossy_frame_length_program(tdata_all_buffer, input_buffer_bytes),
         result != 0)) {
        return result;
    }
    bit_inputter = lhdc_v5_dec_get_mem_content_addr(heap_memory, INPUTTER_INDEX);
    lhdc_v5_bit_reader_init(bit_inputter, input_buffer, tdata_all_buffer->channel_frame_bytes);
    uint32_t frame_flag;
    int flag_result = read_decode_field(bit_inputter, 1, &frame_flag);
    if (flag_result != 0)
        return flag_result;
    if (frame_flag != 0)
        return LHDC_V5_DEC_INVALID_FRAME;
    dec_arith = lhdc_v5_dec_get_mem_content_addr(heap_memory, DEC_ARITH_S_INDEX);
    bit_inputter = lhdc_v5_dec_get_mem_content_addr(heap_memory, INPUTTER_INDEX);
    nf_seed = lhdc_v5_dec_get_mem_content_addr(heap_memory, SEED_INDEX);

    V5ChannelFrame frame;
    int header_result = read_v5_spectral_header(tdata_all_buffer, bit_inputter, &frame);
    if (header_result != 0)
        return header_result;
    uint32_t *time_data = (uint32_t *)tdata_all_buffer->buf.time.dec_tdata;
    target_range_or_clipped_sample = tdata_all_buffer->target_range;
    frame.input_word_cursor = bit_inputter->dec_r_mem;
    frame.cached_bit_count = bit_inputter->dec_r_num;
    frame.input_begin = bit_inputter->dec_r_begin;
    V5ResidualBits bits;
    if (!decode_v5_spectral_quotients(tdata_all_buffer, bit_inputter, dec_arith, &frame, &bits))
        return frame.header_status != 0 ? audio_math_int32_from_bits(frame.header_status)
                                        : tdata_all_buffer->frame_samples;
    if (!read_v5_spectral_residuals(tdata_all_buffer, bit_inputter, output_buffer, &frame, &bits))
        return frame.header_status != 0 ? audio_math_int32_from_bits(frame.header_status)
                                        : tdata_all_buffer->frame_samples;
    offset_scale = scale_v5_spectrum(tdata_all_buffer, time_data, output_buffer,
                                     frame.spectral_count, frame.offset_index);
    if (!restore_v5_residual_refinement(tdata_all_buffer, bit_inputter, &frame, &bits, time_data,
                                        offset_scale))
        return frame.header_status != 0 ? audio_math_int32_from_bits(frame.header_status)
                                        : tdata_all_buffer->frame_samples;
    /* No active lines remain in the tail: consume eight-bit batches and trailing single bits, retaining cross-word prefetch. */
    int padding_result =
        skip_spectral_padding(bit_inputter, audio_math_int32_from_bits(bits.bit_mask_or_shift),
                              target_range_or_clipped_sample);
    if (padding_result != 0)
        return padding_result;
    if (frame.header_status != 0) {
        return frame.header_status;
    }
    fill_v5_spectral_noise(tdata_all_buffer, nf_seed, time_data, frame.spectral_count,
                           frame.frequency_shift, offset_scale);
    synthesize_v5_pcm(tdata_all_buffer, time_data, output_buffer);
    frame.header_status = tdata_all_buffer->frame_samples;
    return frame.header_status;
}

/* Read-only initialization data with a single definition referenced through the matching header. */
#include "frame_decode_tables.h"
