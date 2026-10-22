#ifndef LHDC_WINDOWS_NUMERIC_POLICY_H
#define LHDC_WINDOWS_NUMERIC_POLICY_H
#if defined(_M_IX86) || defined(_M_X64)
#define LHDC_PLATFORM_FIXED_DSP_TABLES 1
#else
#define LHDC_PLATFORM_FIXED_DSP_TABLES 0
#endif
#if defined(_MSC_VER) && !defined(__clang__) && (defined(_M_IX86) || defined(_M_X64))
#define LHDC_PLATFORM_CALIBRATED_FLOAT_MATH 1
#else
#define LHDC_PLATFORM_CALIBRATED_FLOAT_MATH 0
#endif
#if defined(_MSC_VER) && defined(_M_IX86)
#define LHDC_PLATFORM_LLAC_GAIN_COMMON 1
#else
#define LHDC_PLATFORM_LLAC_GAIN_COMMON 0
#endif
#if defined(_MSC_VER) && defined(_M_X64)
#define LHDC_PLATFORM_LLAC_GAIN_UCRT64 1
#else
#define LHDC_PLATFORM_LLAC_GAIN_UCRT64 0
#endif
#define LHDC_PLATFORM_VBAP_15_5_THRESHOLD_F32 0.267238408f
#endif
