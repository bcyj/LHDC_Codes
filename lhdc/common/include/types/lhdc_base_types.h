/* Scalar aliases shared by V4/V5; lhdc_platform.h defines platform widths. */
#ifndef LHDC_COMMON_BASE_TYPES_H
#define LHDC_COMMON_BASE_TYPES_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "../../../platform/include/lhdc_platform.h"
#ifndef false
#define false 0
#define true 1
#endif

typedef unsigned short unsigned_short;
typedef unsigned char undefined;
#ifndef bool
typedef unsigned char bool;
#endif
typedef unsigned char byte;
typedef unsigned char dwfenc;
typedef unsigned int dword;
typedef float float4;
typedef double float8;
typedef long long longlong;
typedef uint64_t qword;
typedef int sdword;
typedef int64_t sqword;
typedef short sword;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef lhdc_ulong_t ulong;
typedef unsigned long long ulonglong;
typedef unsigned char undefined1;
typedef unsigned int undefined4;
typedef uint64_t undefined8;
typedef unsigned short ushort;
typedef unsigned short word;
typedef float8 real64_T;
typedef ulong __ARRAY_SIZE_TYPE__;
typedef dword uint32_T;
typedef uchar boolean_T;
typedef uchar uint8_T;
typedef word uint16_T;
typedef qword __uint64;
typedef float4 real32_T;
typedef real64_T real_T;

#endif
