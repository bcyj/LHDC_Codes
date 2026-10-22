#include <audio_math/fma_native.h>
#include <audio_math/constants.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif
#include "../include/llac_signal.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include "spectral_tables.h"

static const float reflection_coefficients[17] = {

    -0.99573416f, -0.96182567f, -0.8951633f, -0.7980172f, -0.6736956f, -0.52643216f,
    -0.36124167f, -0.18374951f, 0.0f,        0.18374951f, 0.36124167f, 0.52643216f,
    0.6736956f,   0.7980172f,   0.8951633f,  0.96182567f, 0.99573416f};
#define LLAC_BAND_OFFSET_KEY UINT32_C(0x07221613)

static const float power_integer[26] = {

    1.0f,       2.0f,       4.0f,       8.0f,        16.0f,      32.0f,     64.0f,
    128.0f,     256.0f,     512.0f,     1024.0f,     2048.0f,    4096.0f,   8192.0f,
    16384.0f,   32768.0f,   65536.0f,   131072.0f,   262144.0f,  524288.0f, 1048576.0f,
    2097152.0f, 4194304.0f, 8388608.0f, 16777216.0f, 33554432.0f};

/* Apply TNS inverse filtering over one/two bands, sharing nine filter-history entries within this call. */
void llac_apply_tns(float *x, const LLACTnsData *tns, int bw_fcbin, int N, int fs, int frame_dms) {
    float cap = (float)frame_dms * AUDIO_MATH_ONE_TENTH_F32;
    cap *= 40.0f;
    if ((float)N > cap) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        N = audio_math_truncate_i32_f32(cap);
#else
        N = (int)cap;
#endif
        fs = 40000;
    }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    /* Convert the integer quotient to float before extended-precision addition of one and integer conversion. */
    int startfreq[2] = {audio_math_truncate_i32_f64((double)(float)(N * 1200 / fs) + 1.0), 0},
        stopfreq[2] = {bw_fcbin, 0};
#else
    int startfreq[2] = {(int)((float)(N * 1200 / fs) + 1.0f), 0}, stopfreq[2] = {bw_fcbin, 0};
#endif
    if (tns->filter_count == 2) {
        stopfreq[0] = bw_fcbin >> 1;
        startfreq[1] = (bw_fcbin >> 1) + 1;
        stopfreq[1] = bw_fcbin;
    }
    float st[9] = {0};
    for (int f = 0; f < tns->filter_count; ++f) {
        if (tns->order[f] <= 0)
            continue;
        float rc[8];
        for (int j = 0; j < 8; ++j)
            rc[j] = reflection_coefficients[tns->reflection_indices[8 * f + j]];
        for (int k = startfreq[f] - 1; k < stopfreq[f]; ++k) {
            int n = tns->order[f];
            float value = audio_math_fma_f32(-rc[n - 1], st[n - 1], x[k]);
            for (int j = n - 2; j >= 0; --j) {
                value = audio_math_fma_f32(-rc[j], st[j], value);
                st[j + 1] = audio_math_fma_f32(rc[j], value, st[j]);
            }
            x[k] = value;
            st[0] = value;
        }
    }
}

/* Deobfuscate band endpoints and multiply gains in order; skip the first offset and never move backward on descending boundaries. */
void llac_apply_sns_gains(float *x, float *scf, int *bands_offset, int fdns_npts) {
    int position = 0;
    for (int band = 0; band < fdns_npts; ++band) {
        uint32_t raw = (uint32_t)bands_offset[band + 1] ^ LLAC_BAND_OFFSET_KEY;
        int end;
        memcpy(&end, &raw, 4);
        while (position < end) {
            x[position] = scf[band] * x[position];
            ++position;
        }
    }
}

/* Interpolate 16 log gains to 64, compress to the band count, and interpolate tables to reconstruct linear gains. */
void llac_interpolate_sns_gains(float *gains, int bands_number, float *gains_int, float *tmp) {
    memset(tmp, 0, 80 * sizeof *tmp);
    gains_int[0] = gains_int[1] = gains[0];
    for (int i = 0; i < 15; ++i) {
        float delta = gains[i + 1] - gains[i];
        gains_int[i * 4 + 2] = audio_math_fma_f32(delta, 0.125f, gains[i]);
        gains_int[i * 4 + 3] = audio_math_fma_f32(delta * 3.0f, 0.125f, gains[i]);
        gains_int[i * 4 + 4] = audio_math_fma_f32(delta * 5.0f, 0.125f, gains[i]);
        gains_int[i * 4 + 5] = audio_math_fma_f32(delta * 7.0f, 0.125f, gains[i]);
    }
    float delta = gains[15] - gains[14];
    gains_int[62] = audio_math_fma_f32(delta, 0.125f, gains[15]);
    gains_int[63] = audio_math_fma_f32(delta * 3.0f, 0.125f, gains[15]);
    if (bands_number < 64) {
        int merge = 64 - bands_number;
        assert(merge <= 31);
        for (int i = 0; i < merge; ++i)
            tmp[i] = (gains_int[i * 2] + gains_int[i * 2 + 1]) * 0.5f;
        for (int i = 1; i < merge; ++i)
            gains_int[i] = gains_int[i * 2];
        for (int i = merge * 2; i < 64; ++i)
            gains_int[i - merge] = gains_int[i];
        memcpy(gains_int, tmp, (size_t)merge * 4);
    }
    for (int i = 0; i < bands_number; ++i) {
        float value = gains_int[i];
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        int integer = audio_math_truncate_i32_f32(value);
#else
        int integer = (int)value;
#endif
        if (value >= 0.0f) {
            float fractional_position = (value - (float)integer) * 100.0f;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            int part = audio_math_truncate_i32_f32(fractional_position);
#else
            int part = (int)fractional_position;
#endif
            float fraction = fractional_position - (float)part;
            float linear = audio_math_fma_f32(
                lhdc_decoder_v4_llac_signal_power_fraction[part], 1.0f - fraction,
                fraction * lhdc_decoder_v4_llac_signal_power_fraction[part + 1]);
            gains_int[i] = power_integer[integer] * linear;
        } else {
            float fractional_position = (float)(-integer) + value;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            /* The negative-path product does not round back to float; binary64 represents float * 100 exactly.
             * Rounding to float first changes integer table indices at some hundredth boundaries. */
            int part = audio_math_truncate_i32_f64((double)fractional_position * 100.0);
#else
            int part = (int)(fractional_position * 100.0f);
#endif
            float fraction = audio_math_fma_f32(-fractional_position, 100.0f, (float)part);
            float linear = audio_math_fma_f32(
                lhdc_decoder_v4_llac_signal_power_fraction[-part], 1.0f - fraction,
                fraction * lhdc_decoder_v4_llac_signal_power_fraction[1 - part]);
            gains_int[i] = 1.0f / (linear * power_integer[-integer]);
        }
    }
}

/* Read-only initialization data with a single definition referenced through the matching header. */
#include "spectral_tables.h"

const float lhdc_decoder_v4_llac_signal_power_fraction[101] = {

    1.0f,       1.0069555f, 1.0139595f, 1.0210121f, 1.0281138f, 1.035265f,  1.0424658f, 1.0497167f,
    1.057018f,  1.0643702f, 1.0717734f, 1.0792283f, 1.0867349f, 1.0942937f, 1.1019051f, 1.1095694f,
    1.1172872f, 1.1250585f, 1.1328839f, 1.1407638f, 1.1486983f, 1.1566882f, 1.1647335f, 1.172835f,
    1.1809926f, 1.1892071f, 1.1974787f, 1.2058078f, 1.2141949f, 1.2226403f, 1.2311444f, 1.2397077f,
    1.2483306f, 1.2570133f, 1.2657566f, 1.2745606f, 1.2834259f, 1.2923528f, 1.3013419f, 1.3103935f,
    1.319508f,  1.3286858f, 1.3379276f, 1.3472335f, 1.3566043f, 1.3660402f, 1.3755418f, 1.3851094f,
    1.3947437f, 1.4044449f, 1.4142135f, 1.4240502f, 1.4339552f, 1.4439292f, 1.4539725f, 1.4640857f,
    1.4742693f, 1.4845235f, 1.4948492f, 1.5052468f, 1.5157166f, 1.5262592f, 1.5368751f, 1.547565f,
    1.5583291f, 1.5691682f, 1.5800827f, 1.5910729f, 1.6021397f, 1.6132835f, 1.6245048f, 1.6358042f,
    1.647182f,  1.6586391f, 1.6701758f, 1.6817929f, 1.6934906f, 1.7052698f, 1.7171309f, 1.7290745f,
    1.7411011f, 1.7532115f, 1.765406f,  1.7776854f, 1.7900501f, 1.802501f,  1.8150383f, 1.827663f,
    1.8403753f, 1.8531761f, 1.866066f,  1.8790455f, 1.8921152f, 1.905276f,  1.9185282f, 1.9318726f,
    1.9453099f, 1.9588406f, 1.9724654f, 1.986185f,  2.0f};
