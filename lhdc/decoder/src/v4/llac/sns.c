#include <audio_math/float_sqrt.h>
#include "../../../../common/include/types/llac_scalar_types.h"
#include <audio_math/numeric_bits.h>
#include <audio_math/fma_native.h>
#include "../../../../common/include/dsp/float_compat.h"
#include "../include/llac_semantic_records.h"
#include "tables/llac_decode_keys.h"
#include "../include/llac_sns.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "sns_tables.h"

/* Reconstruct a signed integer vector of specified dimension/pulses from an MPVQ rank.
 * Rank halving still rounds through float and saturating conversion; do not replace with an integer right shift. */
static void decode_mpvq_pulses(uint32_t rank, int dimensions, int pulse_count, int sign,
                               int *pulses) {
    const uint32_t key = decode_keys.sns_mpvq;
    for (int position = 0; position < dimensions; ++position) {
        if (rank == 0) {
            pulses[position] = sign * pulse_count;
            break;
        }
        int row = (dimensions - 1 - position) * 11;
        int remaining = 0;
        for (int column = 1; column <= pulse_count + 1; ++column) {
            uint32_t boundary = lhdc_decoder_v4_llac_sns_sns_table_3[row + column - 1] ^ key;
            if (audio_math_int32_from_bits(boundary) <= audio_math_int32_from_bits(rank))
                ++remaining;
        }
        if (remaining != 0)
            --remaining;
        int value = pulse_count - remaining;
        rank -= lhdc_decoder_v4_llac_sns_sns_table_3[row + remaining] ^ key;
        if (value != 0) {
            unsigned sign_bit = rank & 1;
            float half_rank = (float)audio_math_shift_right_bounded_i32(audio_math_int32_from_bits(rank), 1);
            rank = audio_math_truncate_sat_u32_f32(half_rank);
            pulses[position] = value * sign;
            sign = sign_bit ? -1 : 1;
            pulse_count = remaining;
        }
    }
}

/* Select 10+6-dimensional, 16-dimensional eight-pulse, or six-pulse reconstruction by SNS submode. */
static void decode_sns_pulses(const LLACSnsIndices *sns, int *pulses) {
    memset(pulses, 0, 16 * sizeof *pulses);
    int sign = 1 - 2 * sns->leading_sign;
    if (sns->submode < 2) {
        decode_mpvq_pulses(sns->main_mpvq, 10, 10, sign, pulses);
        if (sns->submode == 0) {
            float half_rank = (float)audio_math_shift_right_bounded_i32(sns->secondary_mpvq, 1);
            decode_mpvq_pulses(audio_math_truncate_sat_u32_f32(half_rank), 6, 1,
                               1 - 2 * (int)((uint32_t)sns->secondary_mpvq & 1u), pulses + 10);
        }
    } else {
        decode_mpvq_pulses(sns->main_mpvq, 16, sns->submode == 2 ? 8 : 6, sign, pulses);
    }
}

/* Normalize pulses, perform inverse DCT in fixed fused accumulation order, then apply SNS gains. */
static void apply_sns_pulses(const LLACSnsIndices *sns, const int *pulses, LLAC_FLOAT *scf_q) {
    /* Normalize before inverse DCT and accumulate terms in the specified order. */

    float sum = 0.0f;
    for (int i = 0; i < 16; ++i) {
        int32_t square = audio_math_multiply_wrap_i32(pulses[i], pulses[i]);
        sum = (float)square + sum;
    }
    float norm = audio_math_sqrt_f32(sum);
    float st2_vector[16], st2_vector_idct[16];
    for (int i = 0; i < 16; ++i)
        st2_vector[i] = (float)pulses[i] / norm;
    for (int i = 0; i < 16; ++i) {
        float transformed = 0.0f;
        for (int j = 0; j < 16; ++j)
            transformed = audio_math_fma_f32(
                st2_vector[j],
                audio_math_float32_from_bits(lhdc_decoder_v4_llac_sns_sns_idct_coeff_bits[i][j]),
                transformed);
        st2_vector_idct[i] = transformed * audio_math_float32_from_bits(UINT32_C(0x3eb504f3));
    }
    uint32_t gain_key = decode_keys.sns_gain;
    uint32_t gain_bits = lhdc_decoder_v4_llac_sns_sns_table_4[sns->gain + (sns->submode << 3)];
    float gain = audio_math_float32_from_bits(gain_key ^ gain_bits);
    scf_q[13] = audio_math_fma_f32(gain, st2_vector_idct[13], scf_q[13]);
    scf_q[2] = audio_math_fma_f32(gain, st2_vector_idct[2], scf_q[2]);
    scf_q[7] = audio_math_fma_f32(gain, st2_vector_idct[7], scf_q[7]);
    scf_q[3] = audio_math_fma_f32(gain, st2_vector_idct[3], scf_q[3]);
    scf_q[10] = audio_math_fma_f32(gain, st2_vector_idct[10], scf_q[10]);
    scf_q[6] = audio_math_fma_f32(gain, st2_vector_idct[6], scf_q[6]);
    scf_q[9] = audio_math_fma_f32(gain, st2_vector_idct[9], scf_q[9]);
    scf_q[1] = audio_math_fma_f32(gain, st2_vector_idct[1], scf_q[1]);
    scf_q[4] = audio_math_fma_f32(gain, st2_vector_idct[4], scf_q[4]);
    scf_q[5] = audio_math_fma_f32(gain, st2_vector_idct[5], scf_q[5]);
    scf_q[12] = audio_math_fma_f32(gain, st2_vector_idct[12], scf_q[12]);
    scf_q[0] = audio_math_fma_f32(gain, st2_vector_idct[0], scf_q[0]);
    scf_q[11] = audio_math_fma_f32(gain, st2_vector_idct[11], scf_q[11]);
    scf_q[8] = audio_math_fma_f32(gain, st2_vector_idct[8], scf_q[8]);
    scf_q[14] = audio_math_fma_f32(gain, st2_vector_idct[14], scf_q[14]);
    scf_q[15] = audio_math_fma_f32(gain, st2_vector_idct[15], scf_q[15]);
}

/* Reconstruct low/high codebooks, MPVQ pulses, and final sixteen-dimensional SNS scale factors. */
void llac_decode_sns_codebooks(const LLACSnsIndices *sns, LLAC_FLOAT *scf_q) {
    for (int i = 0; i < 8; ++i) {
        scf_q[i] = audio_math_float32_from_bits(
            lhdc_decoder_v4_llac_sns_sns_table_0[sns->low_codebook + i * 32] ^
            decode_keys.sns_low_codebook);
        scf_q[i + 8] = audio_math_float32_from_bits(
            lhdc_decoder_v4_llac_sns_sns_table_2[sns->high_codebook + i * 32] ^
            decode_keys.sns_high_codebook);
    }
    int pulses[16];
    decode_sns_pulses(sns, pulses);
    apply_sns_pulses(sns, pulses, scf_q);
}
