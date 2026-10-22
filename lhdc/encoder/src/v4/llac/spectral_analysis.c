#include "../../../../platform/windows/include/lhdc_windows_exp_log.h"
#include <audio_math/float_properties.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#include <audio_math/float_exp.h>
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/math.h>
#include <audio_math/float_sqrt.h>
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "../../../../common/include/compat/v4/lhdc_v4_math.h"
#include <audio_math/float_rounding.h>
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../../../../platform/x86/include/lhdc_x86_smoothing.h"
#include "../../../../platform/x86/include/lhdc_x86_attack.h"
#include "../include/codec/codec_internal.h"
#include "fft_adapter.h"
#include "spectral_analysis_tables.h"
#include <string.h>

#if defined(LHDC_X86_ATTACK_HIGHPASS_BLOCK)
/* Downsample in 40-sample blocks with branches outside loops to preserve MSVC vectorization.
 * first is 0/40/80/120; the final two blocks of 240-sample frames retain zero-padding rules. */
static void attack_downsample_block(const LLAC_FLOAT *input, LLAC_INT frame_size, LLAC_INT first,
                                    LLAC_FLOAT *output) {
    switch (frame_size) {
    case 240:
        if (first >= 80) {
            memset(output, 0, 40 * sizeof *output);
            break;
        }
        for (int i = 0; i < 40; ++i) {
            int index = (first + i) * 3;
            output[i] = (LLAC_FLOAT)((double)input[index] +
                                     (double)(input[index + 1] + input[index + 2]) * 0.5);
        }
        break;
    case 320:
        for (int i = 0; i < 40; ++i) {
            int index = (first + i) * 2;
            output[i] = input[index] + input[index + 1];
        }
        break;
    case 480:
        for (int i = 0; i < 40; ++i) {
            int index = (first + i) * 3;
            output[i] = input[index] + input[index + 1] + input[index + 2];
        }
        break;
    case LHDC_V4_LLAC_MAX_FRAME_SAMPLES:
        for (int i = 0; i < 40; ++i) {
            int index = (first + i) * 6;
            output[i] = input[index] + input[index + 1] + input[index + 2] + input[index + 3] +
                        input[index + 4] + input[index + 5];
        }
        break;
    default:
        memset(output, 0, 40 * sizeof *output);
        break;
    }
}
#else
/* Obtain one downsampled transient-detector sample; zero-pad the second half of 240-point frames and unknown sizes.
 * The three-point special path retains inner float addition, outer double arithmetic, and final float rounding. */
static LLAC_FLOAT attack_downsample_sample(const LLAC_FLOAT *input, LLAC_INT frame_size,
                                           LLAC_INT index) {
    switch (frame_size) {
    case 240:
        if (index < 80)
            return (LLAC_FLOAT)((double)input[index * 3] +
                                (double)(input[index * 3 + 1] + input[index * 3 + 2]) * 0.5);
        return 0.0f;
    case 320:
        return input[index * 2] + input[index * 2 + 1];
    case 480:
        return input[index * 3] + input[index * 3 + 1] + input[index * 3 + 2];
    case LHDC_V4_LLAC_MAX_FRAME_SAMPLES:
        return input[index * 6] + input[index * 6 + 1] + input[index * 6 + 2] +
               input[index * 6 + 3] + input[index * 6 + 4] + input[index * 6 + 5];
    default:
        return 0.0f;
    }
}
#endif

/* Stream downsampling/high-pass filtering, accumulate four energies, detect transients, and retain frame state.
 * MSVC SSE2 processes 40-sample blocks; other hosts process per sample. Neither stores a full frame.
 * Write history only after consuming all input to preserve input/history aliasing order. */
void v4f_llac_detect_attack(EncSetup *state, LLAC_INT frame_size) {
    if (!state->attack_handling)
        return;
    const LLAC_FLOAT *in = state->s_in_scaled;
    LLAC_INT *lastAttackPosition = &state->attdec_position, *attackFlag = &state->attdec_detected;
    LLAC_FLOAT *accNrg = &state->attdec_acc_energy, *attdec_filter_mem = state->attdec_filter_mem;
    LLAC_FLOAT previous2 = attdec_filter_mem[0], previous1 = attdec_filter_mem[1];
    LLAC_FLOAT block_nrg[4] = {0};
#if defined(LHDC_X86_ATTACK_HIGHPASS_BLOCK)
    LLAC_FLOAT samples[42], filtered[40];
    for (LLAC_INT j = 0; j < 4; ++j) {
        samples[0] = previous2;
        samples[1] = previous1;
        attack_downsample_block(in, frame_size, j * 40, samples + 2);
        lhdc_x86_attack_highpass_block(samples, filtered);
        for (LLAC_INT i = 0; i < 40; ++i)
            block_nrg[j] += filtered[i] * filtered[i];
        previous2 = samples[40];
        previous1 = samples[41];
    }
#else
    for (LLAC_INT j = 0; j < 4; ++j) {
        for (LLAC_INT i = 0; i < 40; ++i) {
            LLAC_FLOAT current = attack_downsample_sample(in, frame_size, j * 40 + i);
            LLAC_FLOAT sum = (LLAC_FLOAT)((double)current * 0.375 + 0.0);
            sum = (LLAC_FLOAT)((double)sum + (double)previous1 * -0.5);
            LLAC_FLOAT filtered = (LLAC_FLOAT)((double)previous2 * 0.125 + (double)sum);
            block_nrg[j] += filtered * filtered;
            previous2 = previous1;
            previous1 = current;
        }
    }
#endif
    attdec_filter_mem[0] = previous2;
    attdec_filter_mem[1] = previous1;
    LLAC_FLOAT floor_energy =
        frame_size == LHDC_V4_LLAC_MAX_FRAME_SAMPLES ? LHDC_V4_LLAC_ATTACK_ENERGY_FLOOR : 0.0f;
    LLAC_FLOAT accumulated = *accNrg;
    LLAC_INT attackPosition = -1;
    *attackFlag = 0;
    for (LLAC_INT j = 0; j < 4; ++j) {
        LLAC_FLOAT mval = accumulated > floor_energy ? accumulated : floor_energy;
        if (mval < block_nrg[j] / 8.5f) {
            attackPosition = j + 1;
            *attackFlag = 1;
        }
        double decayed = (double)accumulated * 0.25;
        accumulated = (double)block_nrg[j] > decayed ? block_nrg[j] : (LLAC_FLOAT)decayed;
    }
    *accNrg = accumulated;
    if (*lastAttackPosition > 2)
        *attackFlag = 1;
    *lastAttackPosition = attackPosition;
}

#include <math.h>
#include <string.h>

/* Interpolate sixteen log gains to the actual band count and convert to encoding/decoding linear gains. */

void v4f_llac_interpolate_sns_gains(LLAC_FLOAT *gains, LLAC_INT encoder_side, LLAC_INT bands_number,
                                    LLAC_FLOAT *gains_int) {
    gains_int[0] = gains[0];
    gains_int[1] = gains[0];
    for (LLAC_INT i = 0; i < 15; ++i) {
        gains_int[4 * i + 2] = (LLAC_FLOAT)((double)(gains[i + 1] - gains[i]) * 0.125 + gains[i]);
        gains_int[4 * i + 3] =
            (LLAC_FLOAT)((double)(gains[i + 1] - gains[i]) * 3.0 * 0.125 + gains[i]);
        gains_int[4 * i + 4] =
            (LLAC_FLOAT)((double)(gains[i + 1] - gains[i]) * 5.0 * 0.125 + gains[i]);
        gains_int[4 * i + 5] =
            (LLAC_FLOAT)((double)(gains[i + 1] - gains[i]) * 7.0 * 0.125 + gains[i]);
    }
    gains_int[62] = (LLAC_FLOAT)((double)(gains[15] - gains[14]) * 0.125 + gains[15]);
    gains_int[63] = (LLAC_FLOAT)((double)(gains[15] - gains[14]) * 3.0 * 0.125 + gains[15]);
    if (bands_number < 64) {
        if (bands_number <= 32) {
            LLAC_FLOAT ratio = (LLAC_FLOAT)bands_number;
            if (audio_math_ceil_f64(64.0 / ratio) == 4.0) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
                LLAC_INT n4 = audio_math_truncate_i32_f32(audio_math_round_f32(
                    audio_math_abs_f32((LLAC_FLOAT)(-32.0 / ratio + 1.0)) * ratio));
#else
                LLAC_INT n4 = (LLAC_INT)audio_math_round_f32(
                    audio_math_abs_f32((LLAC_FLOAT)(-32.0 / ratio + 1.0)) * ratio);
#endif
                /* Read each entire group before writing ahead of its first element, preserving unread input. */
                for (LLAC_INT i = 0; i < n4; ++i)
                    gains_int[i] = ((gains_int[4 * i] + gains_int[4 * i + 1]) +
                                    gains_int[4 * i + 2] + gains_int[4 * i + 3]) *
                                   0.25f;
                for (LLAC_INT i = n4; i < bands_number; ++i) {
                    LLAC_INT n = 4 * n4 + 2 * (i - n4);
                    gains_int[i] = (gains_int[n] + gains_int[n + 1]) * 0.5f;
                }
            }
        } else {
            LLAC_INT d = 64 - bands_number;
            /* Averages occupy only [0, d); the unread tail begins at 2*d. */
            for (LLAC_INT i = 0; i < d; ++i)
                gains_int[i] = (gains_int[2 * i] + gains_int[2 * i + 1]) * 0.5f;

            for (LLAC_INT i = 2 * d; i < 64; ++i)
                gains_int[i - d] = gains_int[i];
        }
    }
    if (encoder_side == 1)
        for (LLAC_INT i = 0; i < bands_number; ++i)
            gains_int[i] = -gains_int[i];
    for (LLAC_INT i = 0; i < bands_number; ++i)
        gains_int[i] = audio_math_exp2f(gains_int[i]);
}

/* Project input to an integer pulse vector and greedily allocate remaining pulses by correlation/energy ratio. */

void v4f_llac_pvq_search(LLAC_FLOAT *x_in, LLAC_INT dim, LLAC_INT pulses, LLAC_INT *y,
                         LLAC_FLOAT *y_en1_norm) {
    /* Pulse squares use int32; invalid ranges do not access caller storage. */
    if (!x_in || !y || !y_en1_norm || dim < 1 || dim > LHDC_V4_LLAC_PVQ_MAX_DIMENSIONS ||
        pulses < 1 || pulses > LHDC_V4_LLAC_PVQ_MAX_PULSES)
        return;
    LLAC_FLOAT xabs[LHDC_V4_LLAC_PVQ_MAX_DIMENSIONS];
    /* Dimension is at most 16; save all magnitudes/comparison signs before output to support input/normalized-output overlap.
     * NaN makes >= false; sign selection must use this comparison rather than only testing < 0. */
    uint32_t negative_mask = 0;
    LLAC_FLOAT xsum = 0.0f;
    for (LLAC_INT i = 0; i < dim; ++i) {
        LLAC_FLOAT value = x_in[i];
        xabs[i] = audio_math_abs_f32(value);
        negative_mask |= (uint32_t) !(value >= 0.0f) << i;
        xsum += xabs[i];
    }
    if (!(xsum > AUDIO_MATH_POW2_NEG_24_F32) || !audio_math_is_finite_f32(xsum)) {
        /* With no valid direction, select the canonical rank-zero vector. Keep all pulses within the valid dimension,
         * independently of existing array contents, matching encoder normalization and decoder pulse reconstruction. */
#if defined(_MSC_VER) && !defined(__clang__) &&                                                    \
    (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
        /* Fixed four-entry blocks improve MSVC clearing; write up to three remaining entries individually. */
        LLAC_INT i = 1;
        for (; i + 3 < dim; i += 4) {
            memset(y + i, 0, 4 * sizeof *y);
            memset(y_en1_norm + i, 0, 4 * sizeof *y_en1_norm);
        }
        for (; i < dim; ++i) {
            y[i] = 0;
            y_en1_norm[i] = 0.0f;
        }
#else
        for (LLAC_INT i = 1; i < dim; ++i) {
            y[i] = 0;
            y_en1_norm[i] = 0.0f;
        }
#endif
        LLAC_FLOAT energy = (LLAC_FLOAT)(pulses * pulses);
        LLAC_FLOAT gain_fac = 1.0f / audio_math_sqrt_f32(energy);
        LLAC_INT first_pulse = negative_mask & UINT32_C(1) ? -pulses : pulses;
        y[0] = first_pulse;
        y_en1_norm[0] = gain_fac * (LLAC_FLOAT)first_pulse;
        return;
    }
    LLAC_INT pulse_tot = 0;
    LLAC_FLOAT xy = 0.0f;
    LLAC_FLOAT yy = 0.0f;
    LLAC_FLOAT proj_fac = (LLAC_FLOAT)(pulses - 1) / xsum;
    for (LLAC_INT i = 0; i < dim; ++i) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* floor already returns a binary32 integer; projected pulse counts fit int32. */
        y[i] = audio_math_truncate_i32_f32(audio_math_floor_f32(proj_fac * xabs[i]));
#else
        y[i] = (LLAC_INT)audio_math_floor_f32(proj_fac * xabs[i]);
#endif
        pulse_tot += y[i];
        yy += (LLAC_FLOAT)(y[i] * y[i]);
        xy += xabs[i] * (LLAC_FLOAT)y[i];
    }
    yy *= 0.5f;
    while (pulse_tot < pulses) {
        LLAC_INT imax = 0;
        LLAC_FLOAT cmax_num = -32768.0f, cmax_den = 0.0f;
        LLAC_FLOAT yy_tmp_base = yy + 0.5f;
        for (LLAC_INT i = 0; i < dim; ++i) {
            LLAC_FLOAT xy2_tmp = xy + xabs[i];
            xy2_tmp *= xy2_tmp;
            LLAC_FLOAT yy_tmp = yy_tmp_base + (LLAC_FLOAT)y[i];
            if (cmax_den * xy2_tmp > cmax_num * yy_tmp) {
                imax = i;
                cmax_num = xy2_tmp;
                cmax_den = yy_tmp;
            }
        }
        xy += xabs[imax];
        yy = yy_tmp_base + (LLAC_FLOAT)y[imax];
        ++y[imax];
        ++pulse_tot;
    }
    yy += yy;
    LLAC_FLOAT gain_fac = 1.0f / audio_math_sqrt_f32(yy);
    for (LLAC_INT i = 0; i < dim; ++i) {
        LLAC_INT sign = negative_mask & (UINT32_C(1) << i) ? -1 : 1;
        y[i] *= sign;
        y_en1_norm[i] = gain_fac * (LLAC_FLOAT)y[i];
    }
}

/* Apply spectral-envelope gains by band boundaries, always starting at zero. */
void v4f_llac_apply_sns_gains(LLAC_FLOAT *x, LLAC_FLOAT *scf, const LLAC_INT *bands_offset,
                              LLAC_INT fdns_npts) {
    LLAC_INT j = 0;
    for (LLAC_INT i = 0; i < fdns_npts; ++i)
        for (; j < bands_offset[i + 1]; ++j)
            x[j] = x[j] * scf[i];
}

/* Expand band energy to 64 bands, smooth/tilt it, and generate mean-removed 16-dimensional SNS log gains. */
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
void v4f_llac_analyze_sns_with_policy(LLAC_FLOAT *x, LLAC_INT tilt, LLAC_INT xLen,
                                      LLAC_FLOAT *gains, LLAC_INT smooth, LLAC_FLOAT sns_damping,
                                      unsigned math_policy) {
#else
/* Smooth grouped energy and produce tilt-compensated, damped SNS gains. */
void v4f_llac_analyze_sns(LLAC_FLOAT *x, LLAC_INT tilt, LLAC_INT xLen, LLAC_FLOAT *gains,
                          LLAC_INT smooth, LLAC_FLOAT sns_damping) {
#endif
    LLAC_FLOAT band_workspace[64] = {0};
    LLAC_FLOAT gain_workspace[16];
    if (xLen < 64) {
        LLAC_INT d = 64 - xLen;
        if (d < xLen) {
            for (LLAC_INT i = 0; i < d; ++i)
                band_workspace[2 * i] = band_workspace[2 * i + 1] = x[i];
            memcpy(band_workspace + 2 * d, x + d, (size_t)(64 - 2 * d) * sizeof(LLAC_FLOAT));
        } else if (audio_math_ceil_f64(64.0 / (double)(LLAC_FLOAT)xLen) == 4.0) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            LLAC_INT n4 = audio_math_truncate_i32_f32(audio_math_round_f32(
                audio_math_abs_f32((LLAC_FLOAT)(-32.0 / (double)(LLAC_FLOAT)xLen + 1.0)) *
                (LLAC_FLOAT)xLen));
            LLAC_INT j = 0;
#else
            LLAC_INT n4 = (LLAC_INT)audio_math_round_f32(
                         audio_math_abs_f32((LLAC_FLOAT)(-32.0 / (double)(LLAC_FLOAT)xLen + 1.0)) *
                         (LLAC_FLOAT)xLen),
                     j = 0;
#endif
            for (LLAC_INT i = 0; i < n4; ++i)
                for (LLAC_INT n = 0; n < 4; ++n)
                    band_workspace[j++] = x[i];
            for (LLAC_INT i = n4; i < xLen; ++i) {
                band_workspace[j++] = x[i];
                band_workspace[j++] = x[i];
            }
        }
        memcpy(x, band_workspace, sizeof band_workspace);
        xLen = 64;
    }
#if defined(LHDC_X86_SMOOTH_121)
    /* After band expansion, the first ten workspace entries can hold block snapshots. */
    lhdc_x86_smooth_121_f32(x, xLen, band_workspace);
#else
    /* Retain the original left neighbor before writes; the right neighbor is unread, and endpoints repeat.
     * Preserve double multiply-add order and final float rounding without copying the full frame. */
    LLAC_FLOAT previous = x[0];
    for (LLAC_INT i = 0; i < xLen; ++i) {
        LLAC_FLOAT current = x[i];
        LLAC_FLOAT next = i + 1 < xLen ? x[i + 1] : current;
        x[i] = (LLAC_FLOAT)((double)current * 0.5 + (double)previous * 0.25 + (double)next * 0.25);
        previous = current;
    }
#endif
    const LLAC_FLOAT *tilt_gains = lhdc_llac_sns_tilt_gains(tilt, xLen);
    if (tilt_gains) {
        for (LLAC_INT i = 0; i < xLen; ++i)
            x[i] *= tilt_gains[i];
    } else {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)

#endif
        for (LLAC_INT i = 0; i < xLen; ++i) {
            LLAC_FLOAT exponent =
                (LLAC_FLOAT)(((double)((LLAC_FLOAT)tilt * (LLAC_FLOAT)i) / ((double)xLen - 1.0)) /
                             10.0);
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            x[i] *= audio_math_pow10_to_f32_soft((double)exponent);
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
            x[i] *= lhdc_windows_positive_powf_soft(10.0f, exponent, math_policy);
#else
        x[i] *= audio_math_powf(10.0f, exponent);
#endif
        }
    }
    LLAC_FLOAT sum = 0.0f;
    for (LLAC_INT i = 0; i < 64; ++i)
        sum += x[i];
    LLAC_FLOAT nf = (sum / (LLAC_FLOAT)xLen) * 0.0001f;
    if (nf <= AUDIO_MATH_POW2_NEG_32_F32)
        nf = AUDIO_MATH_POW2_NEG_32_F32;
    /* Expansion/smoothing are complete; reuse 64-point workspace for log energy. */
    for (LLAC_INT i = 0; i < 64; ++i) {
        if (x[i] < nf)
            x[i] = nf;
        band_workspace[i] = lhdc_v4_log2_compat_f32(x[i]) * 0.5f;
    }
    const LLAC_FLOAT W[6] = {

        1.0f / 12.0f, 1.0f / 6.0f, 0.25f, 0.25f, 1.0f / 6.0f, 1.0f / 12.0f};
    LLAC_INT bands_number = xLen / 4;
    for (LLAC_INT i = 0; i < bands_number; ++i) {
        LLAC_INT start = 4 * i - 1;
        LLAC_FLOAT value = 0.0f;
        for (LLAC_INT j = 0; j < 6; ++j) {
            LLAC_INT idx = start + j;
            if (idx < 0)
                idx = 0;
            if (idx > 63)
                idx = 63;
            if (j == 0)
                value = band_workspace[idx] * W[j] + 0.0f;
            else
                value += band_workspace[idx] * W[j];
        }
        gain_workspace[i] = value;
    }
    sum = 0.0f;
    for (LLAC_INT i = 0; i < bands_number; ++i)
        sum += gain_workspace[i];
    LLAC_FLOAT mean = sum / ((LLAC_FLOAT)xLen * 0.25f);
    for (LLAC_INT i = 0; i < bands_number; ++i)
        gains[i] = (gain_workspace[i] - mean) * sns_damping;
    if (smooth) {
        /* All downsampled values are output; reuse 16-point workspace for smoothed gains. */
        for (LLAC_INT i = 0; i < 16; ++i) {
            LLAC_INT start = i > 1 ? i - 2 : 0, stop = i < 14 ? i + 2 : 15;
            LLAC_FLOAT value = gains[start];
            for (LLAC_INT j = start + 1; j <= stop; ++j)
                value += gains[j];
            gain_workspace[i] = value / (LLAC_FLOAT)(stop - start + 1);
        }
        sum = gain_workspace[0] + 0.0f;
        for (LLAC_INT i = 1; i < 16; ++i)
            sum += gain_workspace[i];
        mean = sum * 0.0625f;
        for (LLAC_INT i = 0; i < 16; ++i)
            gains[i] = (gain_workspace[i] - mean) * 0.5f;
    }
}
#include "../include/llac/llac_const.h"

/* Inverse-transform sixteen SNS residuals using the four-ABI cosine table, retaining multiplication precision and accumulation order.
 * Verified kernel hosts always have fixed tables and need no compiled dynamic-cosine fallback.
 * Other configurations retain fallback policy; do not round double coefficients to float early. */
static void v4i_sns_inverse_dct(const LLAC_FLOAT *input, LLAC_FLOAT *output) {
    const double(*cosines)[16] = lhdc_llac_sns_inverse_cosines();
    for (LLAC_INT i = 0; i < 16; ++i) {
#if !defined(_KERNEL_MODE) || !LHDC_LLAC_SNS_FIXED_TABLES_AVAILABLE
        double row = (double)i + 1.0;
#endif
        LLAC_FLOAT sumInLoop = 0.0f;
        for (LLAC_INT j = 0; j < 16; ++j) {
#if defined(_KERNEL_MODE) && LHDC_LLAC_SNS_FIXED_TABLES_AVAILABLE
            double cosine = cosines[i][j];
#else
            double cosine;
            if (cosines) {
                cosine = cosines[i][j];
            } else {
                double angle = (row + row - 1.0) * AUDIO_MATH_PI_OVER_32_F64 * (double)j;
                cosine = audio_math_cos(angle);
            }
#endif
            LLAC_FLOAT value = (LLAC_FLOAT)(cosine * (double)input[j]);
            if (!j)
                value *= AUDIO_MATH_INV_SQRT_TWO_F32;
            sumInLoop += value;
        }
        output[i] = sumInLoop * AUDIO_MATH_INV_SQRT_EIGHT_F32;
    }
}

/* Accumulate candidate/target squared distances dimensionwise and select candidates with strict less-than. */
static LLAC_FLOAT v4i_sns_codebook_distance(const LLAC_FLOAT *target, const LLAC_FLOAT *candidate,
                                            LLAC_FLOAT gain) {
    LLAC_FLOAT sum = 0.0f;
    for (LLAC_INT i = 0; i < 16; ++i) {
        LLAC_FLOAT delta = target[i] - gain * candidate[i];
        sum += delta * delta;
    }
    return sum;
}

/* Enumerate PVQ pulses/signs from highest dimension downward to produce the combined index and final sign bit. */
static LLAC_INT v4i_sns_rank_pulses(const LLAC_INT *pulses, LLAC_INT dimension, LLAC_INT *sign) {
    LLAC_INT pos = 0, k = 0;
    *sign = -1;
    for (LLAC_INT i = dimension - 1, row = 0; i >= 0; --i, ++row) {
        if (pulses[i]) {
            if (*sign >= 0)
                pos = pos * 2 + *sign;
            *sign = pulses[i] < 0;
        }
        pos += alg_signal_t3[row][k];
        k += pulses[i] < 0 ? -pulses[i] : pulses[i];
    }
    return pos;
}

/* Search low/high eight-dimensional codebooks separately and return the last selected index. */
static LLAC_INT select_sns_codebooks(const LLAC_FLOAT *env, LLACSnsIndices *sns,
                                     LLAC_FLOAT *st1_vector) {
    LLAC_INT idx = 0;
    for (LLAC_INT sec = 0; sec < 2; ++sec) {
        LLAC_FLOAT bestdist = LHDC_V4_LLAC_SNS_INITIAL_DISTANCE;
        for (LLAC_INT c = 0; c < 32; ++c) {
            LLAC_FLOAT sum = 0.0f;
            for (LLAC_INT i = 0; i < 8; ++i) {
                LLAC_FLOAT codebook_error =
                    env[sec * 8 + i] - (sec ? alg_signal_t2[i][c] : alg_signal_t1[i][c]);
                sum += codebook_error * codebook_error;
            }
            if (sum < bestdist) {
                bestdist = sum;
                idx = c;
            }
        }
        if (sec == 0)
            sns->low_codebook = idx;
        else
            sns->high_codebook = idx;
        for (LLAC_INT i = 0; i < 8; ++i)
            st1_vector[sec * 8 + i] = sec ? alg_signal_t2[i][idx] : alg_signal_t1[i][idx];
    }
    return idx;
}

/* Select instance SNS codebooks, quantize residuals, and save indices/reconstructed gains. */
void v4f_llac_quantize_sns(EncSetup *state) {
    const LLAC_FLOAT *env = state->scf;
    LLACSnsIndices *sns = &state->sns_indices;
    LLAC_FLOAT *envq = state->scf_q;
    Dct2 dct2structSNS = state->dct2StructSNS;
    LLAC_FLOAT st1_vector[16] = {0}, target[16], pvq_target[16] = {0};
    LLAC_INT idx = select_sns_codebooks(env, sns, st1_vector);
    for (LLAC_INT i = 0; i < 16; ++i)
        target[i] = env[i] - st1_vector[i];
    v4f_llac_dct2_transform(&dct2structSNS, target, pvq_target);
    /* PVQ fallback and ordinary projection write only active dimensions; pulse arrays need no extra sentinel. */
    LLAC_INT pulses_submodeC1[10] = {0}, pulses_submodeC2[6] = {0}, pulses[16] = {0};
    LLAC_INT chosen_pulses[16] = {0};
    LLAC_FLOAT normalized_submodeC1[16] = {0}, norm[16] = {0};
    /* Keep joint-pulse projections for the first two groups; generate other gain candidates into norm one group at a time. */
    LLAC_FLOAT candidate_workspace[2][16];
    v4f_llac_pvq_search(pvq_target, 10, 10, pulses_submodeC1, normalized_submodeC1);
    /* Six-dimensional normalized results are not read later; candidate selection completely overwrites norm. */
    v4f_llac_pvq_search(pvq_target + 10, 6, 1, pulses_submodeC2, norm);
    memcpy(pulses, pulses_submodeC1, 10 * sizeof(LLAC_INT));
    memcpy(pulses + 10, pulses_submodeC2, 6 * sizeof(LLAC_INT));
    LLAC_FLOAT sum = (LLAC_FLOAT)(pulses[0] * pulses[0]);
    for (LLAC_INT i = 1; i < 16; ++i)
        sum += (LLAC_FLOAT)(pulses[i] * pulses[i]);
    LLAC_FLOAT factor = 1.0f / audio_math_sqrt_f32(sum);
    for (LLAC_INT i = 0; i < 16; ++i) {
        LLAC_FLOAT combined = factor * (LLAC_FLOAT)pulses[i];
        for (LLAC_INT c = 0; c < 2; ++c)
            candidate_workspace[c][i] = combined * alg_signal_t14[c];
    }
    LLAC_FLOAT bestdist = 32768.0f;
    LLAC_INT candidate = idx;
    for (LLAC_INT c = 0; c < 6; ++c) {
        const LLAC_FLOAT *projection;
        if (c < 2) {
            projection = candidate_workspace[c];
        } else {
            /* The final six dimensions of the ten-dimensional submode retain initialized zeros. */
            for (LLAC_INT i = 0; i < 16; ++i)
                norm[i] = normalized_submodeC1[i] * alg_signal_t12[c - 2];
            projection = norm;
        }
        LLAC_FLOAT distance = v4i_sns_codebook_distance(pvq_target, projection, 1.0f);
        if (distance < bestdist) {
            bestdist = distance;
            candidate = c;
            /* The first two groups are compared; row zero stores a new candidate only when it wins. */
            if (c >= 2)
                memcpy(candidate_workspace[0], norm, sizeof candidate_workspace[0]);
        }
    }
    LLAC_FLOAT gain_fac = alg_signal_t13[candidate];
    const LLAC_FLOAT *selected_projection = candidate_workspace[candidate == 1 ? 1 : 0];
    for (LLAC_INT i = 0; i < 16; ++i)
        norm[i] = selected_projection[i] / gain_fac;
    /* After normalization neither projection row is read; reuse them for inverse transform and final reconstruction. */
    LLAC_FLOAT *reconstruction = candidate_workspace[0];
    LLAC_FLOAT *stage2 = candidate_workspace[1];
    memset(stage2, 0, sizeof candidate_workspace[1]);
    v4i_sns_inverse_dct(norm, reconstruction);
    LLAC_FLOAT min_err = v4i_sns_codebook_distance(target, reconstruction, gain_fac);
    if (!(min_err < 32768.0f))
        min_err = 32768.0f;
    else {
        sns->submode = candidate > 1;
        sns->gain = candidate > 1 ? candidate - 2 : candidate;
        memcpy(chosen_pulses, pulses, 16 * sizeof(LLAC_INT));
        if (candidate > 1) {
            memcpy(chosen_pulses, pulses_submodeC1, 10 * sizeof(LLAC_INT));
            memset(chosen_pulses + 10, 0, 6 * sizeof(LLAC_INT));
        }
        for (LLAC_INT i = 0; i < 16; ++i)
            stage2[i] = gain_fac * reconstruction[i];
    }
    for (LLAC_INT mode = 2; mode < 4; ++mode) {
        v4f_llac_pvq_search(pvq_target, 16, mode == 2 ? 8 : 6, pulses, norm);
        v4i_sns_inverse_dct(norm, reconstruction);
        LLAC_FLOAT best = 32768.0f, selected_gain = gain_fac;
        const LLAC_FLOAT *gains = mode == 2 ? alg_signal_t11 : alg_signal_t10;
        LLAC_INT selected = candidate;
        for (LLAC_INT c = 0; c < (mode == 2 ? 4 : 8); ++c) {
            LLAC_FLOAT error = v4i_sns_codebook_distance(target, reconstruction, gains[c]);
            if (error < best) {
                best = error;
                selected = c;
                selected_gain = gains[c];
            }
        }
        if (best < min_err) {
            min_err = best;
            sns->submode = mode;
            sns->gain = selected;
            memcpy(chosen_pulses, pulses, 16 * sizeof(LLAC_INT));
            for (LLAC_INT i = 0; i < 16; ++i)
                stage2[i] = selected_gain * reconstruction[i];
        }
        candidate = selected;
        gain_fac = selected_gain;
    }
    LLAC_INT sign;
    sns->main_mpvq = v4i_sns_rank_pulses(chosen_pulses, sns->submode < 2 ? 10 : 16, &sign);
    sns->leading_sign = sign;
    if (sns->submode == 0) {
        LLAC_INT second_sign;
        LLAC_INT position = v4i_sns_rank_pulses(chosen_pulses + 10, 6, &second_sign);
        sns->secondary_mpvq = position * 2 + second_sign;
    } else
        sns->secondary_mpvq = sns->submode == 2 ? -1 : -2;
    for (LLAC_INT i = 0; i < 16; ++i)
        envq[i] = st1_vector[i] + stage2[i];
}
#include "../include/llac/llac_const.h"

/* Normalize subband autocorrelation and apply a lag window, preserving term accumulation order. */
static void analyze_tns_autocorrelation(const LLAC_FLOAT *x, LLAC_INT first_frequency,
                                        LLAC_INT stop_frequency, LLAC_FLOAT nSubdivisions,
                                        LLAC_INT maxOrder, LLAC_FLOAT *r) {
    LLAC_FLOAT subdiv_len =
        (((LLAC_FLOAT)stop_frequency + 1.0f) - (LLAC_FLOAT)first_frequency) / nSubdivisions;
    for (LLAC_INT sub = 1; (LLAC_FLOAT)sub <= nSubdivisions; ++sub) {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* subdiv_len has rounded to float; convert products at extended precision without another float store.
         * Retain inner/outer truncation toward zero; valid frame-bin/segment indices fit int32. */
        LLAC_INT first_offset =
            audio_math_truncate_i32_f64((double)subdiv_len * (double)(LLAC_FLOAT)(sub - 1));
        LLAC_INT last_offset =
            audio_math_truncate_i32_f64((double)(LLAC_FLOAT)sub * (double)subdiv_len);
        LLAC_INT first = audio_math_truncate_i32_f64((double)(LLAC_FLOAT)first_frequency +
                                                     (double)(LLAC_FLOAT)first_offset - 1.0);
        LLAC_INT last = audio_math_truncate_i32_f64((double)(LLAC_FLOAT)first_frequency +
                                                    (double)(LLAC_FLOAT)last_offset - 1.0);
#else
        LLAC_INT first =
            (LLAC_INT)((LLAC_FLOAT)first_frequency +
                       (LLAC_FLOAT)(LLAC_INT)(subdiv_len * (LLAC_FLOAT)(sub - 1)) - 1.0f);
        LLAC_INT last = (LLAC_INT)((LLAC_FLOAT)first_frequency +
                                   (LLAC_FLOAT)(LLAC_INT)((LLAC_FLOAT)sub * subdiv_len) - 1.0f);
#endif
        LLAC_FLOAT sum = 0.0f;
        for (LLAC_INT i = first; i < last; ++i)
            sum += x[i] * x[i];
        if (last <= first || sum == 0.0f) {
            /* r is a caller correlation array: clear active-order elements, not sizeof the pointer. */
            for (LLAC_INT lag = 0; lag <= maxOrder; ++lag)
                r[lag] = 0.0f;
            r[0] = 1.0f;
            break;
        }
        for (LLAC_INT lag = 0; lag <= maxOrder; ++lag) {
            LLAC_FLOAT corr = 0.0f;

            for (LLAC_INT i = first; i < last; ++i)
                corr += x[i] * (i - first < lag ? 0.0f : x[i - lag]);
            r[lag] += corr / sum;
        }
    }
    for (LLAC_INT i = 0; i <= maxOrder; ++i)
        r[i] *= alg_signal_t7[i];
}

/* Compute reflection coefficients/prediction polynomial with Levinson recursion and return residual energy. */
static LLAC_FLOAT predict_tns_reflections(const LLAC_FLOAT *r, LLAC_INT maxOrder,
                                          LLAC_FLOAT *rc_unq, LLAC_FLOAT *out_lev,
                                          LLAC_FLOAT *reversed) {
    LLAC_FLOAT g = r[1] / r[0];
    reversed[0] = g;
    rc_unq[0] = -g;
    LLAC_FLOAT error_lev = (1.0f - g * g) * r[0];
    for (LLAC_INT order = 2; order <= maxOrder; ++order) {
        LLAC_FLOAT sum = 0.0f;
        for (LLAC_INT i = 0; i < order - 1; ++i)
            sum += reversed[i] * r[i + 1];
        g = (r[order] - sum) / error_lev;
        /* Free the first slot, save each coefficient pair, and update both ends together before overwriting unread symmetric terms. */
        for (LLAC_INT i = order - 1; i > 0; --i)
            reversed[i] = reversed[i - 1];
        for (LLAC_INT i = 1; i <= order / 2; ++i) {
            LLAC_INT mirror = order - i;
            LLAC_FLOAT left = reversed[i], right = reversed[mirror];
            reversed[i] = left - g * right;
            if (i != mirror)
                reversed[mirror] = right - g * left;
        }
        reversed[0] = g;
        rc_unq[order - 1] = -g;
        error_lev *= 1.0f - g * g;
    }
    out_lev[0] = 1.0f;
    for (LLAC_INT i = 1; i <= maxOrder; ++i)
        out_lev[i] = -reversed[maxOrder - i];
    return error_lev;
}

/* Analyze predictors and apply TNS in caller workspace; return orders, indices, and bit budgets. */
void v4f_llac_analyze_and_apply_tns_with_workspace(LLAC_FLOAT *x, LLAC_INT bw_cutoff_idx,
                                                   LLAC_INT bw_fcbin, LLAC_INT fs, LLAC_INT N,
                                                   LLAC_INT frame_dms, LLAC_INT nBits,
                                                   LLACTnsData *tns, LLAC_INT *bits_out,
                                                   LhdcV4LlacTnsWorkspace *workspace) {
    double max_length = ((double)frame_dms / 10.0) * 40.0;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    LLAC_INT length = (double)N <= max_length ? N : audio_math_truncate_i32_f64(max_length);
#else
    LLAC_INT length = (double)N <= max_length ? N : (LLAC_INT)max_length;
#endif
    LLAC_INT rate = (double)N <= max_length ? fs : 40000;
    LLAC_INT startfreq[2] = {rate ? length * 1200 / rate + 1 : 1, length / 2 + 1};
    LLAC_INT stopfreq[2] = {bw_fcbin, length};
    LLAC_INT maxOrder =
        frame_dms == LHDC_V4_FRAME_DURATION_10MS
            ? LHDC_V4_LLAC_TNS_MAX_ORDER
            : (frame_dms == LHDC_V4_FRAME_DURATION_2P5MS || frame_dms == LHDC_V4_FRAME_DURATION_5MS
                   ? 4
                   : 0);
    LLAC_FLOAT nSubdivisions =
        frame_dms == LHDC_V4_FRAME_DURATION_10MS
            ? 3.0f
            : (frame_dms == LHDC_V4_FRAME_DURATION_2P5MS || frame_dms == LHDC_V4_FRAME_DURATION_5MS
                   ? 2.0f
                   : 0.0f);
    LLAC_INT weighting = frame_dms != LHDC_V4_FRAME_DURATION_2P5MS &&
                         (double)nBits < ((double)frame_dms / 10.0) * 48.0;
    LLAC_FLOAT minPredictionGain = weighting ? 2.0f : 1.5f;
    const LLAC_INT *order_bits = weighting ? alg_signal_t6 : alg_signal_t5;
    LLAC_INT numfilters = 1, bits = 0;
    if (fs > 31999 && frame_dms > 49 && bw_cutoff_idx > 2) {
        numfilters = 2;
        stopfreq[0] = bw_fcbin / 2;
        stopfreq[1] = bw_fcbin;
        startfreq[1] = bw_fcbin / 2 + 1;
    }
    LLAC_FLOAT *st = workspace->filter_state;
    memset(st, 0, sizeof workspace->filter_state);
    for (LLAC_INT f = 0; f < numfilters; ++f) {
        /* Correlations expire after prediction-gain computation; reuse their storage for polynomial reduction and quantized coefficients. */
        LLAC_FLOAT *stage_workspace = workspace->stage;
        LLAC_FLOAT *rc_unq = workspace->reflections;
        LLAC_FLOAT *out_lev = workspace->polynomial;
        memset(stage_workspace, 0, sizeof workspace->stage);
        analyze_tns_autocorrelation(x, startfreq[f], stopfreq[f], nSubdivisions, maxOrder,
                                    stage_workspace);
        LLAC_FLOAT error_lev = predict_tns_reflections(stage_workspace, maxOrder, rc_unq, out_lev,
                                                       workspace->reversed);
        LLAC_FLOAT predGain = stage_workspace[0] / error_lev;
        ++bits;
        if (!(predGain > 1.5f))
            continue;
        if (predGain < minPredictionGain) {
            LLAC_FLOAT alpha =
                ((minPredictionGain - predGain) * (0.85f - 1.0f)) / (minPredictionGain - 1.5f) +
                1.0f;
            /* Constant term is one and the linear term multiplies alpha directly; other terms retain backend float rounding boundaries. */
            if (maxOrder > 0)
                out_lev[1] *= alpha;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
            for (LLAC_INT i = 2; i <= maxOrder; ++i)
                out_lev[i] *= (LLAC_FLOAT)audio_math_powu_f64((double)alpha, (unsigned)i);
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
            /* Reuse sequential double powers alpha squared through alpha^maxOrder while retaining each order's float boundary.
             * Fallback changes only the current weight; never feed a fallback float into the next double product. */
            double power = (double)alpha;
            for (LLAC_INT i = 2; i <= maxOrder; ++i) {
                power *= (double)alpha;
                out_lev[i] *= lhdc_windows_tns_round_power_soft(power, alpha, (unsigned)i);
            }
#else
            for (LLAC_INT i = 2; i <= maxOrder; ++i)
                out_lev[i] *= audio_math_powf(alpha, (LLAC_FLOAT)i);
#endif
            for (LLAC_INT i = 0; i <= maxOrder; ++i)
                out_lev[i] /= out_lev[0];
            for (LLAC_INT order = maxOrder; order >= 1; --order) {
                LLAC_FLOAT k = out_lev[order];
                LLAC_FLOAT *next = stage_workspace;
                /* Copy only 0..order-1; each active entry is written before reading, so no uncopied tail clearing is needed. */
                next[0] = 1.0f;
                rc_unq[order - 1] = k;
                double denominator = 1.0 - (double)(k * k);
                for (LLAC_INT i = 1; i < order; ++i) {
                    LLAC_FLOAT product = k * out_lev[order - i];
                    next[i] = (LLAC_FLOAT)((double)(out_lev[i] - product) / denominator);
                }
                memcpy(out_lev, next, (size_t)order * sizeof(LLAC_FLOAT));
            }
        }
        LLAC_FLOAT *rc = stage_workspace;
        /* Reduction is complete; reuse correlation/next-polynomial scratch for up to eight quantized reflection coefficients. */
        LLAC_INT *rc_idx_tmp = workspace->reflection_indices;
        memset(rc_idx_tmp, 0, sizeof workspace->reflection_indices);
        LLAC_INT order = 0;
        for (LLAC_INT i = 0; i < maxOrder; ++i) {
            for (LLAC_INT j = 0; j < 17; ++j)
                if (rc_unq[i] <= alg_signal_t9[j + 1] && alg_signal_t9[j] < rc_unq[i])
                    rc_idx_tmp[i] = j;
            rc[i] = alg_signal_t8[rc_idx_tmp[i]];
            if (rc[i] != 0.0f)
                order = i + 1;
        }
        tns->order[f] = order;
        LLAC_INT coded_bits = order_bits[order - 1];
        for (LLAC_INT i = 0; i < order; ++i) {
            coded_bits += alg_signal_t4[i][rc_idx_tmp[i]];
            tns->reflection_indices[f * 8 + i] = rc_idx_tmp[i];
        }
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        bits += audio_math_truncate_i32_f64(
            audio_math_ceil_f64((double)coded_bits * AUDIO_MATH_POW2_NEG_11_F64));
#else
        bits += (LLAC_INT)audio_math_ceil_f64((double)coded_bits * AUDIO_MATH_POW2_NEG_11_F64);
#endif
        for (LLAC_INT i = startfreq[f] - 1; i < stopfreq[f]; ++i) {
            LLAC_FLOAT s = x[i], tmpSave = s;
            for (LLAC_INT j = 0; j < order - 1; ++j) {
                LLAC_FLOAT tmp_fl = st[j];
                st[j] = tmpSave;
                tmpSave = s * rc[j] + tmp_fl;
                s += rc[j] * tmp_fl;
            }
            LLAC_FLOAT previous = st[order - 1];
            st[order - 1] = tmpSave;
            x[i] = s + rc[order - 1] * previous;
        }
    }
    tns->filter_count = numfilters;
    *bits_out = bits;
}

/* Pack evens forward/odds backward, FFT, and normalize DCT-II using instance scratch.
 * Real staging, generic FFT scratch, and real output reuse one union by phase without per-frame allocation.
 * Gather all input before final output to support overlap; invalid lengths/plans leave output untouched. */
void v4f_llac_dct2_transform(Dct2 *dct, const LLAC_FLOAT *input, LLAC_FLOAT *output) {
    if (!dct || !input || !output)
        return;
    LLAC_INT len = dct->length;
    HANDLE_IIS_FFT handle = dct->fft.handle;
    if (len < 2 || len > LHDC_V4_LLAC_MAX_FRAME_SAMPLES || (len & 1) || dct->fft.length != len ||
        llac_fft_plan_status(handle) != IIS_FFT_NO_ERROR || handle->len != len)
        return;

    LLAC_FLOAT *staging = handle->scratch->analysis;
    memmove(staging, input, (size_t)len * sizeof *staging);
    for (LLAC_INT i = 0; i < len / 2; ++i) {
        handle->kiss_input[i] = (kiss_fft_cpx){staging[2 * i], 0.0f};
        handle->kiss_input[len - 1 - i] = (kiss_fft_cpx){staging[2 * i + 1], 0.0f};
    }
    llac_fft_run_prepared(handle);

    const kiss_fft_cpx *rotations = lhdc_llac_sns_forward_rotations(len);
    LLAC_FLOAT norm = rotations ? 0.0f : 2.0f / audio_math_sqrt_f32((LLAC_FLOAT)(2 * len));
    for (LLAC_INT i = 0; i < len; ++i) {
        LLAC_FLOAT re, im;
        if (rotations) {
            re = rotations[i].r;
            im = rotations[i].i;
        } else {
            LLAC_FLOAT angle = (LLAC_FLOAT)(((double)i * -AUDIO_MATH_PI_F64) / (double)(2 * len));
            LLAC_FLOAT cosine, sine;
            audio_math_sincosf(angle, &sine, &cosine);
            /* The dynamic path retains complex-rotation zero terms and single-precision rounding. */
            re = norm * cosine - sine * 0.0f;
            im = norm * sine + cosine * 0.0f;
        }
        staging[i] = re * handle->kiss_output[i].r - im * handle->kiss_output[i].i;
    }
    staging[0] /= AUDIO_MATH_SQRT_TWO_F32;
    memmove(output, staging, (size_t)len * sizeof *staging);
}

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Stateless algorithm entry selecting host policy automatically; frame encoding uses the instance-policy entry. */
void v4f_llac_analyze_sns(LLAC_FLOAT *x, LLAC_INT tilt, LLAC_INT xLen, LLAC_FLOAT *gains,
                          LLAC_INT smooth, LLAC_FLOAT sns_damping) {
    v4f_llac_analyze_sns_with_policy(x, tilt, xLen, gains, smooth, sns_damping,
                                     lhdc_windows_log10_policy());
}
#endif
