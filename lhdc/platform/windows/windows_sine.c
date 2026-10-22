#include "include/lhdc_windows_sine.h"
#include <audio_math/float_sine.h>

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/numeric_bits.h>

/* Native Win32 x87 PC53/RNE large-phase policy; x64 uses common software reduction.
 * No host DLL access or CRT math entry is required. */
double lhdc_windows_sine_v4_phase_f64(double input) {
#ifdef _M_IX86
    uint64_t magnitude = audio_math_float64_bits(input) & AUDIO_MATH_FLOAT64_MAGNITUDE_BITS;
    if (magnitude >= UINT64_C(0x40f6000000000000) && magnitude < AUDIO_MATH_FLOAT64_INFINITY_BITS) {
        /* x87's 80-bit pi*2^62 approximate divisor, not mathematical 2*pi. */
        static const unsigned char period[10] = {0x35, 0xc2, 0x68, 0x21, 0xa2,
                                                 0xda, 0x0f, 0xc9, 0x3e, 0x40};
        double result;
        __asm {
            fld input
            fsin
            fnstsw ax
            test ah, 4
            jz finished
            fld tbyte ptr period
            fxch st(1)
        reduce_again:
            fprem1
            fnstsw ax
            test ah, 4
            jnz reduce_again
            fstp st(1)
            fsin
        finished:
            fstp result
        }
        return result;
    }
#endif
    return audio_math_sine_wide_f64_soft(input);
}
#endif
