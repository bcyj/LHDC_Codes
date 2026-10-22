#include <audio_math/numeric_bits.h>
#include <audio_math/fma_native.h>
#include "../include/llac_quant.h"
#include <math.h>
#include <string.h>
#include "quantize_tables.h"
/* Fixed-base gain table for powf(10,(float)exponent/28), covering all nonsaturated results.
 * Table range -1265..1080; values beyond it yield positive zero/infinity. Not an audio-sample mapping. */

/* Compute fixed-base gain scaling for every int32 gain exponent. */
static float global_gain_scale(int32_t exponent) {
    if (exponent < -1265)
        return 0.0f;
    if (exponent > 1080)
        return audio_math_float32_from_bits(UINT32_C(0x7f800000));
    return audio_math_float32_from_bits(
        lhdc_decoder_v4_llac_quant_lhdc_v4_global_gain_bits[exponent + 1265]);
}

/* Compute noise amplitude using single-rounding fused multiply-subtract. */
static float noise_amplitude(int index) {
    float value = 0.5f, coefficient = 0.0625f, input = (float)index;
    return audio_math_fma_f32(-input, coefficient, value);
}

/* Find high-frequency zero neighborhoods and fill noise from a 16-bit PRNG seed; always clear all 240 index entries. */
void llac_fill_spectral_noise(int nf_q, float *xq, int nf_seed, int freq_upper_bound, int frame_dms,
                              int *zero_freq) {
    memset(zero_freq, 0, 240 * sizeof *zero_freq);
    int freq_check_width = frame_dms == 50 ? 2 : 1, freq_lower_bound = frame_dms == 50 ? 12 : 6,
        zero_freq_count = 0;
    float nf = noise_amplitude(nf_q);
    if (freq_lower_bound >= freq_upper_bound)
        return;
    for (int start = freq_lower_bound - freq_check_width;
         start < freq_upper_bound - freq_check_width; ++start) {
        int end = start + 2 * freq_check_width;
        if (end >= freq_upper_bound)
            end = freq_upper_bound - 1;
        int j = start;
        while (j <= end && xq[j] == 0.0f)
            ++j;
        if (j > end)
            zero_freq[zero_freq_count++] = start + freq_check_width;
    }
    for (int i = 0; i < zero_freq_count; ++i) {
        uint32_t next = (((uint32_t)nf_seed + 32768u) * 31821u + 13849u) & 65535u;
        nf_seed = (int)next - 32768;
        xq[zero_freq[i]] = nf * (nf_seed < 0 ? 0.5f : -0.5f);
    }
}

/* Scale spectrum by global gain four entries at a time; tails shorter than four are not processed. */
void llac_apply_global_gain(float *x, int xLen, int global_gain_idx, int global_gain_off) {
    uint32_t sum = (uint32_t)global_gain_idx + (uint32_t)global_gain_off;
    int32_t exponent;
    memcpy(&exponent, &sum, 4);
    float gg = global_gain_scale(exponent);
    for (int group = 0; group < xLen / 4; ++group) {
        x[0] *= gg;
        x[1] *= gg;
        x[2] *= gg;
        x[3] *= gg;
        x += 4;
    }
}

/* Correct nonzero spectrum with LSB-first residual bits, write consumed-bit count, and stop at the budget. */
void llac_decode_residual_bits(int *bitsRead, float *x, int L_spec, uint8_t *prm, int resQBits) {
    int used = 0;
    for (int i = 0; i < L_spec && used < resQBits; ++i) {
        float value = x[i];
        if (value != 0.0f) {
            unsigned bit = (prm[(unsigned)used >> 3] >> (used & 7)) & 1;
            x[i] = bit ? value + (value > 0.0f ? 0.3125f : 0.1875f)
                       : value - (value > 0.0f ? 0.1875f : 0.3125f);
            ++used;
        }
    }
    *bitsRead = used;
}
