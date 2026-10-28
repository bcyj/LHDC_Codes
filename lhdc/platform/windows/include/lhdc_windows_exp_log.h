/* Copyright (c) 2017-2025, Arm Limited. SPDX-License-Identifier: MIT
 * Windows adaptation: evaluate exp2f/log2f in the specified fused multiply-add order.
 * See audio_codec_common/math/platform/windows/NOTICE.math.txt for the complete license.
 */
#ifndef LHDC_WINDOWS_CODEC_MATH_H
#define LHDC_WINDOWS_CODEC_MATH_H
#include <audio_math/platform/windows/trig.h>
#include <audio_math/platform/windows/exp_log.h>

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* SSE2 expf return model; AR final weights are verified, but this does not replace arbitrary host expf policies. */
/* Select the default CPU/OS model once at the complete caller; computation helpers no longer probe CPU. */
unsigned lhdc_windows_log10_policy(void);
float lhdc_windows_log10f_soft(float input, unsigned fused);
/* Only for SNS base ten and bounded integer TNS powers; the complete caller selects fused. */
float lhdc_windows_positive_powf_soft(float base, float exponent, unsigned fused);
/* Round sequential double TNS powers to float; base is [0.85f,1], exponent 2..8. */
float lhdc_windows_tns_round_power_soft(double power, float base, unsigned exponent);
#endif
#endif
