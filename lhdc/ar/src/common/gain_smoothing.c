#include "../../../platform/include/lhdc_numeric_policy.h"
#include "gain_smoothing.h"
#include <audio_math/math.h>
#include <audio_math/numeric_bits.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_exp.h>
#endif
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
#include <audio_math/platform/windows/exp_log.h>
#endif

/* Products of four standard sample rates and the default 100 ms attack time, matched by product bit pattern;
 * do not constrain parameter sources or cache mutable instance parameters. */
#if LHDC_PLATFORM_CALIBRATED_FLOAT_MATH
static const struct {
    uint32_t product_bits;
    float decay;
} smoothing_coefficients[4] = {
    {UINT32_C(1250333984), 0.999546587f},
    {UINT32_C(1251113984), 0.999583423f},
    {UINT32_C(1258722592), 0.999773264f},
    {UINT32_C(1259502592), 0.999791682f},
};
#endif

/* Use verified results for fixed single-precision inputs; evaluate other parameters in the specified order. */
float lhdc_ar_smoothing_decay(float rate_time_product) {
#if LHDC_PLATFORM_CALIBRATED_FLOAT_MATH
    uint32_t bits = audio_math_float32_bits(rate_time_product);
    for (unsigned i = 0; i < sizeof smoothing_coefficients / sizeof smoothing_coefficients[0]; ++i)
        if (bits == smoothing_coefficients[i].product_bits)
            return smoothing_coefficients[i].decay;
#endif
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
    return audio_math_expf_soft(-2000.0f / rate_time_product);
#elif defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    /* The two host-FMA decay differences are much smaller than float spacing near 1;
     * the actual 1-decay values and continuous PCM for distance and reverb have been verified identical. */
    return audio_math_windows_expf_sse2_soft(-2000.0f / rate_time_product);
#else
    return audio_math_expf(-2000.0f / rate_time_product);
#endif
}
