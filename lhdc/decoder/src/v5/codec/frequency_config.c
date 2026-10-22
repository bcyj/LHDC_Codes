#include "../../../../common/include/dsp/adsq_tables.h"
#include "../../../include/lhdc/decoder/status.h"
#include <audio_math/constants.h>
#include <audio_math/numeric_bits.h>
#include "../../../../../math/include/audio_math/integer.h"
#include "../include/lhdc_tables.h"
#include <string.h>
#include <limits.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif

/* Update all segments and probability tables from frame bytes, including rate special cases and cutoff endpoints. */
int lhdc_dec_lossy_frame_length_program(tdata_all_buffer_struct *tdata_all_buffer,
                                        int channel_frame_bytes) {
    int ms = tdata_all_buffer->ms, khz = tdata_all_buffer->khz;
    if (channel_frame_bytes < ms * 30 / 100 || channel_frame_bytes > 2000)
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    segment_cutoff_struct *segment_cutoff = lhdc_table_segment_cutoff();
    int rate = channel_frame_bytes * 100 / ms;
    unsigned row = 0;
    while (row < 17 && segment_cutoff[row].segment_size < rate)
        ++row;
    tdata_all_buffer->channel_frame_bytes = channel_frame_bytes;
    int cutoff = ms * segment_cutoff[row].segment_cuteoff / 100;
    tdata_all_buffer->segment_cutoff =
        tdata_all_buffer->frame_samples < cutoff ? tdata_all_buffer->frame_samples : cutoff;
    lhdc_v5_dec_init_gain_table(
        channel_frame_bytes, tdata_all_buffer->resolution, khz, ms, tdata_all_buffer->offset_table,
        &tdata_all_buffer->offset_table_start, &tdata_all_buffer->offset_table_jump);
    int index = tdata_all_buffer->frame_samples;
    if ((khz == 44100 || khz == 48000) && (ms == 100 || ms == 50)) {
        int bit_rate = channel_frame_bytes * 80000 / ms;
        index = bit_rate > 119999 ? 480 : bit_rate <= 95999 ? 482 : 481;
    } else if (khz == 96000 && ms == 50)
        index = 960;
    else if (khz == 192000 && ms == 50)
        index = 1920;
    else if (khz == 192000 && ms == 25)
        index = 481;
    if (lhdc_table_segment_read(&tdata_all_buffer->segment_setting, index))
        return LHDC_V5_DEC_INVALID_ARGUMENT;
    unsigned *read_table1 = lhdc_table_init_fq_table(0), *read_table2 = lhdc_table_init_fq_table(1);
    for (unsigned i = 0; i < 3; ++i) {
        tdata_all_buffer->init_fq_tbl_01[i] = (int)((read_table1[i] >> 2) ^ 0x55aa);
        tdata_all_buffer->init_fq_tbl_02[i] = (int)((read_table2[i] >> 2) ^ 0x55aa);
    }
    int *read_table3 = lhdc_table_power_of_2_table();
    for (unsigned i = 0; i < 161; ++i)
        tdata_all_buffer->power_of_2_tbl[i] = audio_math_int32_from_bits(
            ((unsigned)read_table3[i] + UINT32_C(0xfae87d4e)) ^ UINT32_C(0x55aa55aa));
    int extra = channel_frame_bytes < 160 ? 0 : 4;
    tdata_all_buffer->tdata_quant_rem_data_size = extra;
    tdata_all_buffer->target_range =
        channel_frame_bytes * 8 -
        (tdata_all_buffer->valid_num + tdata_all_buffer->segment_setting.segment_num + 14 + extra);
    return LHDC_DEC_OK;
}

/* Smooth neighboring segment offsets; always read/write at least four entries, requiring sufficient caller storage. */
void lhdc_shift_by_offset_2(segment_setting_struct *segment, int *input, int *output) {
    uint32_t sum =
        (uint32_t)input[0] + (uint32_t)input[1] + (uint32_t)input[2] + (uint32_t)input[3];
    output[0] = input[0];
    output[1] = audio_math_shift_right_i32(audio_math_int32_from_bits(sum + 2), 2);
    unsigned previous = 0, last = 3, tail = 2;
    for (int i = 2; i < segment->segment_num - 2; ++i) {
        sum += (uint32_t)input[i + 2] - (uint32_t)input[i - 2];
        output[i] = audio_math_shift_right_i32(audio_math_int32_from_bits(sum + 2), 2);
        previous = (unsigned)i - 1;
        tail = (unsigned)i + 1;
        last = (unsigned)i + 2;
    }
    int reduced = audio_math_int32_from_bits(sum - (uint32_t)input[previous]);
    int saved = input[tail];
    output[tail] = audio_math_multiply_shift_wrap_i32(reduced, 0x2aaaaaaa, 31);
    output[last] = saved;
}

/* Generate mean-removed segment offsets from the 64-entry integer step table and smooth them.
 * segment_num must be 4..32 and the direction array must contain segment_num-1 entries. */
void lhdc_dec_freq_shift(segment_setting_struct *segment, unsigned char *jump, int first_idx,
                         int *data_sft) {

    int ss[32];
    uint32_t sum = 0, total = 0;
    int run = 1;
    for (int i = 0; i < segment->segment_num - 1; ++i) {
        if (i) {
            if (jump[i] == jump[i - 1]) {
                ++run;
                first_idx += run;
                if (first_idx > 63)
                    first_idx = 63;
            } else {
                first_idx = (first_idx * 3 + 2) >> 2;
                if (first_idx < 0)
                    first_idx = 0;
                run = 1;
            }
        }
        sum += jump[i] ? 0u - (unsigned)lhdc_v5_adsq_step_sizes[first_idx] : (unsigned)lhdc_v5_adsq_step_sizes[first_idx];
        ss[i + 1] = audio_math_int32_from_bits(sum);
        total += sum;
    }
    int mean = audio_math_multiply_shift_wrap_i32(audio_math_int32_from_bits(0u - total),
                                            segment->segment_num_inv, 31);
    ss[0] = mean;
    for (int i = 1; i < segment->segment_num; ++i)
        ss[i] = audio_math_int32_from_bits((unsigned)ss[i] + (unsigned)mean);
    lhdc_shift_by_offset_2(segment, ss, data_sft);
    for (int i = 0; i < segment->segment_num; ++i) {
        if (data_sft[i] > 1023)
            data_sft[i] = 1024;
        if (data_sft[i] < -2560)
            data_sft[i] = -2560;
    }
}

/* Emulate VCVT.S32.F32 #20 truncation/saturation without out-of-range C float-to-integer conversion. */
static int fixed20(float value) {
    value *= AUDIO_MATH_POW2_20_F32;
    if (value >= AUDIO_MATH_POW2_31_F32)
        return INT_MAX;
    if (value <= -AUDIO_MATH_POW2_31_F32)
        return INT_MIN;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Integer-power-table input remains finite after scaling; saturation bounds conversion to int32. */
    return audio_math_truncate_i32_f32(value);
#else
    return (int)value;
#endif
}

/* Select signed Q30/Q20 power-table regions; process four entries in order, then any trailing entries individually. */
void lhdc_freq_shift_apply_decode(segment_setting_struct *segment, int *power_of_2_tbl,
                                  int *data_sft, int *data, int size) {
    if (size <= 0)
        return;
    for (int i = 0; i < segment->segment_num; ++i) {
        int shift_index = data_sft[i];
        float power2 =
            shift_index < 0
                ? (float)power_of_2_tbl[(shift_index + 2575) >> 4] * AUDIO_MATH_POW2_NEG_30_F32
                : (float)power_of_2_tbl[shift_index >> 4] * AUDIO_MATH_POW2_NEG_20_F32;
        int fixed_power2 = fixed20(power2), begin = segment->segment_offset[i];
        int bottom = begin < size ? begin : size,
            top = segment->segment_offset[i + 1] < size ? segment->segment_offset[i + 1] : size;
        int count = top - bottom;
        for (int j = 0; j < count; ++j)
            data[begin + j] = audio_math_multiply_shift_wrap_i32(data[begin + j], fixed_power2, 20);
        if ((count & 3) && (count & 3) >= size)
            return;
    }
}
