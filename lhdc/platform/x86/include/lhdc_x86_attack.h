#ifndef LHDC_X86_ATTACK_H
#define LHDC_X86_ATTACK_H

#if defined(_MSC_VER) &&                                                                          \
    (defined(_M_X64) || (!defined(_KERNEL_MODE) && defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define LHDC_X86_ATTACK_HIGHPASS_BLOCK 1

/* Compute 40 high-pass samples from small snapshots; input needs 42 samples, output 40.
 * Disjoint input/output lets MSVC process independent elements with SSE2; retain three float roundings,
 * first-element positive-zero addition, and double operation order without FMA/AVX. */
static inline void lhdc_x86_attack_highpass_block(const float *samples, float *output) {
    for (int i = 0; i < 40; ++i) {
        float sum = (float)((double)samples[i + 2] * 0.375 + 0.0);
        sum = (float)((double)sum + (double)samples[i + 1] * -0.5);
        output[i] = (float)((double)samples[i] * 0.125 + (double)sum);
    }
}
#endif

#endif
