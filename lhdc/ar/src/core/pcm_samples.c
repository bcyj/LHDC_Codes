#include <audio_math/float_properties.h>
#include "lhdc_log.h"
#include "../../../../math/include/audio_math/byte_order.h"
#include "../../../../math/include/audio_math/integer.h"
#include "include/ar_internal.h"
#include <string.h>
#include <math.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
#include <audio_math/float_convert.h>
#endif

/* Read a signed PCM sample using the input byte width. */
int32_t LHDC_FN(GetOneSample)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, int32_t *pnSample) {
    if (!pbSamplePtr || !pnSample)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    int32_t value = 0;
    if (nBytesPerSample >= 2 && nBytesPerSample <= 4) {
        uint32_t raw = nBytesPerSample == 2   ? audio_math_read_le16(pbSamplePtr)
                       : nBytesPerSample == 3 ? audio_math_read_le24(pbSamplePtr)
                                              : audio_math_read_le32(pbSamplePtr);
        unsigned shift = 32u - (unsigned)nBytesPerSample * 8u;
        value = audio_math_shift_right_i32(audio_math_int32_from_bits(raw << shift), shift);
    }
    *pnSample = value;
    return LHDC_CONST(STATUS_OK);
}

/* Read a PCM sample and convert it to normalized floating point. */
int32_t LHDC_FN(GetOneSample_f)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, float *pfSample) {
    if (!pbSamplePtr || !pfSample)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    int32_t nSample;
    LHDC_FN(GetOneSample)(nBytesPerSample, pbSamplePtr, &nSample);
    if (nBytesPerSample < 2 || nBytesPerSample > 4)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    /* Unsigned shifts produce a positive scale; S32 uses exactly positive 2^31. */
    *pfSample = (float)nSample / (float)(UINT32_C(1) << (nBytesPerSample * 8 - 1));
#if defined(_WIN32) && !defined(_WIN64)
    if (audio_math_is_nan_f32(*pfSample)) {
        uint32_t bits = UINT32_C(0x7fc00000);
        memcpy(pfSample, &bits, sizeof bits);
    }
#endif
    return LHDC_CONST(STATUS_OK);
}

/* Write an integer PCM sample using the output byte width. */
int32_t LHDC_FN(SaveOneSample)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, int32_t nSample) {
    if (!pbSamplePtr || nBytesPerSample < 0 || nBytesPerSample > 4)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    int32_t data = 0;
    if (nBytesPerSample == 2) {
        if (nSample >= LHDC_CONST(PCM_S16_MAX))
            nSample = LHDC_CONST(PCM_S16_MAX);
        else if (nSample <= -LHDC_CONST(PCM_S16_NEGATIVE_MAGNITUDE))
            nSample = -LHDC_CONST(PCM_S16_NEGATIVE_MAGNITUDE);
        data = nSample;
    } else if (nBytesPerSample == 3) {
        if (nSample >= LHDC_CONST(PCM_S24_MAX))
            nSample = LHDC_CONST(PCM_S24_MAX);
        else if (nSample <= -LHDC_CONST(PCM_S24_NEGATIVE_MAGNITUDE))
            nSample = -LHDC_CONST(PCM_S24_NEGATIVE_MAGNITUDE);
        data = nSample;
    } else if (nBytesPerSample == 4)
        data = nSample;
    audio_math_write_le_bytes(pbSamplePtr, (uint32_t)data, (unsigned)nBytesPerSample);
    return LHDC_CONST(STATUS_OK);
}

/* Clamp floating-point PCM and convert to an integer sample of the specified width. */
int32_t LHDC_FN(SaveOneSample_f)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, float fSample) {
    if (!pbSamplePtr)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    if (nBytesPerSample < 2 || nBytesPerSample > 4)
        return LHDC_CONST(AR_INVALID_ARGUMENT);
    uint32_t magnitude = UINT32_C(1) << (nBytesPerSample * 8 - 1);
    float base = (float)magnitude, scaled = base * fSample;
    int32_t sample;
    /* Saturate before float-to-integer conversion; float(2^31)-1 cannot represent the S32 maximum. */
    if (audio_math_is_nan_f32(scaled))
        sample = 0;
    else if (scaled >= base)
        sample = (int32_t)(magnitude - 1u);
    else if (scaled <= -base)
        sample = nBytesPerSample == 4 ? INT32_MIN : -(int32_t)magnitude;
    else {
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && defined(_M_IX86)
        /* NaN and out-of-range values have been excluded; power-of-two scaling adds no rounding within the valid range. */
        sample = audio_math_truncate_i32_f32(scaled);
#else
        sample = (int32_t)scaled;
#endif
    }
    return LHDC_FN(SaveOneSample)(nBytesPerSample, pbSamplePtr, sample);
}
