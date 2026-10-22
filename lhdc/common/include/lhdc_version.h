/* Symbol naming for shared sources; the build selects a version per translation unit. */
#ifndef LHDC_CODES_VERSION_H
#define LHDC_CODES_VERSION_H
#define LHDC_JOIN_IMPL(a, b) a##b
#define LHDC_JOIN(a, b) LHDC_JOIN_IMPL(a, b)
#if LHDC_CODE_VERSION == 4
#define LHDC_FN(name) LHDC_JOIN(v4f_, name)
#define LHDC_PRIVATE(name) LHDC_JOIN(v4i_, name)
#define LHDC_HELPER(name) LHDC_JOIN(v4_, name)
#define LHDC_CONST(name) LHDC_JOIN(LHDC_V4_, name)
#elif LHDC_CODE_VERSION == 5
#define LHDC_FN(name) LHDC_JOIN(v5f_, name)
#define LHDC_PRIVATE(name) LHDC_JOIN(v5i_, name)
#define LHDC_HELPER(name) LHDC_JOIN(v5_, name)
#define LHDC_CONST(name) LHDC_JOIN(LHDC_V5_, name)
#else
#error LHDC_CODE_VERSION must be 4 or 5
#endif
#endif
