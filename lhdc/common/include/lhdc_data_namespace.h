/* Enabled only for a combined SO: validate version selection; read-only data already uses common names. */
#ifndef LHDC_CODES_DATA_NAMESPACE_H
#define LHDC_CODES_DATA_NAMESPACE_H
#if defined(LHDC_COMBINED_NAMESPACE) && LHDC_COMBINED_NAMESPACE
#if LHDC_CODE_VERSION == 4
#elif LHDC_CODE_VERSION == 5
#else
#error LHDC_CODE_VERSION must be 4 or 5
#endif
#endif
#endif
