/* Adapt MSVC pointer widths, C keywords, and GCC builtins. */
#ifndef LHDC_MSVC_INTRINSICS_H
#define LHDC_MSVC_INTRINSICS_H
#ifdef _MSC_VER
#include <intrin.h>
#include <malloc.h>
#define restrict __restrict
#endif
#endif
