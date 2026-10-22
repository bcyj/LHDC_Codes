#include <audio_math/constants.h>
#include <audio_math/float_convert.h>
#include "../include/codec/bit_operations.h"
#include "../include/codec/fast_math.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include "tables/native_tables.h"
#include "../include/codec/v5_extended.h"
#include "../include/codec/v5_pcm.h"
#include <string.h>
#include "spectral_shaping.h"

/* Adjust scale-factor shifts to the band configuration. */
void v5f_shift_scf(const BandCfg_t *band, int32_t *scf) {
    v5_shift_scf((void *)band, (void *)scf);
}

/* Moving-average neighboring band scale factors. */
void v5f_moving_average_scf(const BandCfg_t *band, int32_t *in, int32_t *out) {
    v5_moving_average_scf((void *)band, (void *)in, (void *)out);
}

/* Smooth the SNS scale-factor sequence. */
void v5f_sns_smoothing(int32_t n, int32_t *in, int32_t *out) {
    v5_sns_smoothing(n, (void *)in, (void *)out);
}

/* Apply band scale-factor SNS gains to the spectrum. */
void v5f_sns_apply(const BandCfg_t *band, int32_t *scf, float *cf, int32_t cfn) {
    for (int32_t b = 0; b < band->n; ++b) {
        int32_t scale_factor = scf[b];
        float pwr2 =
            scale_factor < 0
                ? (float)native_pow2_tab[(scale_factor + 0xa0f) >> 4] * AUDIO_MATH_POW2_NEG_30_F32
                : (float)native_pow2_tab[scale_factor >> 4] * AUDIO_MATH_POW2_NEG_20_F32;
        int32_t end = band->off[b + 1];
        if (end > cfn)
            end = cfn;
        for (int32_t i = band->off[b]; i < end; ++i)
            cf[i] *= pwr2;
        if (end >= cfn)
            break;
    }
}

/* Scale nonnegative wide energy using split multiplication to avoid 64-bit intermediate overflow.
 * Encoder band sums are at most about 2^42; band scaling uses 25/27/28/29 bits, smoothing 31 bits.
 * The bounded interface accepts counts 0..63, retaining per-term truncation and unsigned modular multiply-add. */
static uint64_t v5i_energy_scale(uint64_t energy, uint32_t coefficient, uint32_t shift) {
    return audio_math_multiply_wrap_u64(audio_math_shift_right_bounded_u64(energy, shift),
                                        coefficient) +
           audio_math_shift_right_bounded_u64(
               audio_math_multiply_wrap_u64(
                   energy & (audio_math_shift_left_bounded_u64(UINT64_C(1), shift) - 1),
                   coefficient),
               shift);
}

/* Smooth wide energy in place with a five-point sliding window, retaining each Q31 truncation.
 * n is at least four; save left neighbors before writes while right neighbors remain unread, needing no second array. */
static void v5i_energy_smooth_in_place(int32_t n, uint64_t *energy) {
    const uint32_t a = 0x59999999, b = 0x33333333, c = 0x26666666;
    const uint32_t d = 0x19999999, e = 0x0ccccccc;
    uint64_t left2 = energy[0], left1 = energy[1];
    uint64_t center = energy[2], right1 = energy[3];
    energy[0] = v5i_energy_scale(left2, a, 31) + v5i_energy_scale(left1, d, 31) +
                v5i_energy_scale(center, e, 31);
    energy[1] = v5i_energy_scale(left2, c, 31) + v5i_energy_scale(left1, b, 31) +
                v5i_energy_scale(center, d, 31) + v5i_energy_scale(right1, e, 31);
    for (int32_t i = 2; i < n - 2; ++i) {
        uint64_t right2 = energy[i + 2];
        energy[i] = v5i_energy_scale(left2 + right2, e, 31) + v5i_energy_scale(center, b, 31) +
                    v5i_energy_scale(left1 + right1, d, 31);
        left2 = left1;
        left1 = center;
        center = right1;
        right1 = right2;
    }
    energy[n - 1] = v5i_energy_scale(right1, a, 31) + v5i_energy_scale(center, d, 31) +
                    v5i_energy_scale(left1, e, 31);
    energy[n - 2] = v5i_energy_scale(right1, c, 31) + v5i_energy_scale(center, b, 31) +
                    v5i_energy_scale(left1, d, 31) + v5i_energy_scale(left2, e, 31);
}

/* Quantize positive 64-bit energy logarithmically using six-bit table lookup. */
static int32_t v5i_energy_log2(uint64_t value) {
    if (value <= UINT32_MAX)
        return v5_int_log2((uint32_t)value);
    int32_t bits = 0;
    for (uint64_t rest = value; rest; rest >>= 1)
        ++bits;
    /* value > UINT32_MAX: width is 33..64 and index shift is 27..58. */
    return native_log2_tab[audio_math_shift_right_bounded_u64(value, (unsigned)(bits - 6))] +
           bits * 256 - 256;
}

/* Estimate band energy and generate SNS/ADSQ scale-factor codes. */
int32_t v5f_sns_encode(LhdcEncContext *state) {
    const BandCfg_t *band = state->band_cfg;
    int32_t band_stop = state->band_stop;
    const float *vector = state->ed.sdf;
    int32_t *scf = state->ed.band_scf, *fi = &state->ed.adsq_index0;
    uint8_t *adsq_code = state->ed.adsq_code;
    int32_t be[32], sum = 0;
    uint64_t energy_by_band[32], total_energy = 0;
    for (int32_t b = 0; b < band->n; ++b) {
        uint64_t energy = 0;
        for (int32_t i = band->off[b]; i < band->off[b + 1]; ++i) {
            /* Input must be finite with an int32-representable integer part; this path does not saturate. */
            int64_t sample = audio_math_truncate_i32_f32(vector[i]);
            /* sample originates from int32, so its square is at most 2^62 and the modular product safely converts to int64. */
            int64_t squared =
                (int64_t)audio_math_multiply_wrap_u64((uint64_t)sample, (uint64_t)sample);
            energy += (uint64_t)(squared >> 31);
        }
        energy_by_band[b] =
            v5i_energy_scale(energy, (uint32_t)band->scale[b], (uint32_t)band->scale_shift_bits);
        total_energy += energy_by_band[b];
    }
    v5i_energy_smooth_in_place(band->n, energy_by_band);
    uint64_t noise_floor =
        v5i_energy_scale(v5i_energy_scale(total_energy, (uint32_t)band->inv, 31), 0x346dc, 31);
    if (noise_floor < 2)
        noise_floor = 1;
    for (int32_t b = 0; b < band->n; ++b) {
        uint64_t energy = energy_by_band[b] > noise_floor ? energy_by_band[b] : noise_floor;
        be[b] = (v5i_energy_log2(energy) + 0x1f00) >> 1;
        sum += be[b];
    }
    v5f_sns_smoothing(band->n, be, scf);
    int32_t mean = v5i_sns_multiply_q31(sum, band->inv);
    if (band_stop > 480 && mean < 0x109a) {
        for (int32_t i = 0; i < band->n - 1; ++i)
            adsq_code[i] = 0;
        *fi = 0x17;
        return mean;
    }
    int32_t shift = v5i_sns_multiply_q31(band->scale_down, scf[0] - mean);
    for (int32_t b = 1; b < band->n; ++b)
        scf[b] = v5i_sns_multiply_q31(band->scale_down, scf[b] - mean) - shift;
    scf[0] = 0;
    *fi = 8;
    v5f_adsq_enc(scf, be, band->n, adsq_code, fi, 22);
    v5f_shift_scf(band, be);
    v5f_moving_average_scf(band, be, scf);
    for (int32_t b = 0; b < band->n; ++b) {
        int32_t bounded_scale_factor = scf[b];
        if (bounded_scale_factor < -2560)
            bounded_scale_factor = -2560;
        if (bounded_scale_factor > 1024)
            bounded_scale_factor = 1024;
        scf[b] = -bounded_scale_factor;
    }
    return mean;
}

/* Smooth the SNS scale-factor sequence. */
void v5_sns_smoothing(int32_t n, int32_t *in, int32_t *out) {
    const int32_t a = 0x59999999, b = 0x33333333, c = 0x26666666, d = 0x19999999, e = 0x0ccccccc;
    out[0] = v5i_pcm_clean_sns_adsq_mul31(in[0], a) + v5i_pcm_clean_sns_adsq_mul31(in[1], d) +
             v5i_pcm_clean_sns_adsq_mul31(in[2], e);
    out[1] = v5i_pcm_clean_sns_adsq_mul31(in[0], c) + v5i_pcm_clean_sns_adsq_mul31(in[1], b) +
             v5i_pcm_clean_sns_adsq_mul31(in[2], d) + v5i_pcm_clean_sns_adsq_mul31(in[3], e);
    out[n - 1] = v5i_pcm_clean_sns_adsq_mul31(in[n - 1], a) +
                 v5i_pcm_clean_sns_adsq_mul31(in[n - 2], d) +
                 v5i_pcm_clean_sns_adsq_mul31(in[n - 3], e);
    out[n - 2] =
        v5i_pcm_clean_sns_adsq_mul31(in[n - 1], c) + v5i_pcm_clean_sns_adsq_mul31(in[n - 2], b) +
        v5i_pcm_clean_sns_adsq_mul31(in[n - 3], d) + v5i_pcm_clean_sns_adsq_mul31(in[n - 4], e);
    for (int32_t i = 2; i < n - 2; ++i) {
        int64_t outer_sum = (int64_t)in[i - 2] + in[i + 2];
        int64_t inner_sum = (int64_t)in[i - 1] + in[i + 1];
        out[i] = v5i_pcm_clean_sns_adsq_mul31(outer_sum, e) +
                 v5i_pcm_clean_sns_adsq_mul31(in[i], b) +
                 v5i_pcm_clean_sns_adsq_mul31(inner_sum, d);
    }
}

/* Estimate an unsigned integer's fixed-point binary logarithm. */
int32_t v5_int_log2(uint32_t x) {
    if (x < 64)
        return native_log2_tab[x];
    int32_t bits = v5_calc_bits_inline(x);
    return (int32_t)((uint32_t)native_log2_tab[x >> ((bits - 6) & 31)] + (uint32_t)bits * 256 -
                     256);
}

/* Adjust scale-factor shifts to the band configuration. */
void v5_shift_scf(v5_band *band, int32_t *scf) {
    uint32_t sum = (uint32_t)scf[0];
    for (int32_t i = 1; i < band->n; ++i)
        sum += (uint32_t)scf[i];
    int32_t shift =
        audio_math_multiply_shift_wrap_i32(audio_math_int32_from_bits(0u - sum), band->inv, 31);
    scf[0] = shift;
    for (int32_t i = 1; i < band->n; ++i)
        scf[i] = audio_math_add_wrap_i32(scf[i], shift);
}

/* Moving-average neighboring band scale factors. */
void v5_moving_average_scf(v5_band *band, int32_t *in, int32_t *out) {
    out[0] = in[0];
    int32_t sum = audio_math_add_wrap_i32(audio_math_add_wrap_i32(in[0], in[1]),
                                          audio_math_add_wrap_i32(in[2], in[3]));
    out[1] = audio_math_shift_right_i32(audio_math_add_wrap_i32(sum, 2), 2);
    int32_t i = 2;
    for (; i < band->n - 2; ++i) {
        sum = audio_math_add_wrap_i32(audio_math_subtract_wrap_i32(sum, in[i - 2]), in[i + 2]);
        out[i] = audio_math_shift_right_i32(audio_math_add_wrap_i32(sum, 2), 2);
    }
    out[i] = audio_math_multiply_shift_wrap_i32((int64_t)sum - in[i - 2], 0x2aaaaaaa, 31);
    out[i + 1] = in[i + 1];
}
