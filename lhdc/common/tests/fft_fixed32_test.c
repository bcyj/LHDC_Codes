/* Independent formula, read-only input/plan, API boundary, and buffer-sentinel tests for shared fixed-point FFT. */
#include "dsp/fft_fixed32.h"
#include <audio_math/constants.h>
#include "dsp/fft_float32.h" /* Floating- and fixed-point headers must coexist without confusing complex types. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static LhdcFftFixed32Complex input[484], output[484], twiddles[480], saved[480];
static uint32_t random_state = 0x74fd5321;
/* Generate bounded samples to avoid comparing intentional modular overflow with an independent real formula. */
static int32_t sample(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return (int32_t)(random_state & 0x1fffff) - 0x100000;
}
/* Report assertion location and abort the test on failure. */
static void check_at(int ok, int line) {
    if (!ok) {
        fprintf(stderr, "Fixed FFT API failure at line %d\n", line);
        exit(2);
    }
}
#define check(value) check_at((value), __LINE__)
/* Check 1/N-scaled DFT on small signals and verify invalid arguments leave output unchanged. */
int main(void) {
    const int lengths[] = {60, 120, 240, 480};
    double worst = 0.0;
    unsigned cases = 0;
    check(lhdc_fft_fixed32_execute(NULL, input, output) == LHDC_FFT_FIXED32_INVALID_ARGUMENT);
    for (unsigned l = 0; l < 4; ++l)
        for (int inverse = 0; inverse < 2; ++inverse) {
            int n = lengths[l];
            for (int k = 0; k < n; ++k) {
                double phase = (inverse ? 2.0 : -2.0) * AUDIO_MATH_PI_F64 * k / n;
                twiddles[k] = (LhdcFftFixed32Complex){(int32_t)floor(INT32_MAX * cos(phase) + .5),
                                                      (int32_t)floor(INT32_MAX * sin(phase) + .5)};
            }
            const LhdcFftFixed32Plan plan = {n, inverse, twiddles};
            for (unsigned test = 0; test < 8; ++test) {
                memset(input, 0xa5, sizeof input);
                memset(output, 0xa5, sizeof output);
                for (int k = 0; k < n; ++k) {
                    input[2 + k] = (LhdcFftFixed32Complex){sample(), sample()};
                    if (test == 0)
                        input[2 + k] = (LhdcFftFixed32Complex){k == 0 ? 100000 : 0, 0};
                    if (test == 1)
                        input[2 + k] = (LhdcFftFixed32Complex){100000, -200000};
                    if (test == 2)
                        input[2 + k] = (LhdcFftFixed32Complex){k & 1 ? -100000 : 100000, 0};
                    if (test == 3)
                        input[2 + k] = (LhdcFftFixed32Complex){k & 1 ? -1 : 1, k & 2 ? -1 : 1};
                }
                memcpy(saved, input + 2, n * sizeof *saved);
                check(lhdc_fft_fixed32_execute(&plan, input + 2, output + 2) == 0);
                check(!memcmp(saved, input + 2, n * sizeof *saved));
                for (int k = 0; k < n; ++k) {
                    double real = 0.0, imag = 0.0;
                    for (int i = 0; i < n; ++i) {
                        double phase = (inverse ? 2.0 : -2.0) * AUDIO_MATH_PI_F64 * k * i / n;
                        real += input[2 + i].r * cos(phase) - input[2 + i].i * sin(phase);
                        imag += input[2 + i].r * sin(phase) + input[2 + i].i * cos(phase);
                    }
                    double er = fabs(output[2 + k].r - real / n),
                           ei = fabs(output[2 + k].i - imag / n);
                    if (er > worst)
                        worst = er;
                    if (ei > worst)
                        worst = ei;
                    check(er < 16.0 && ei < 16.0);
                }
                for (int k = 0; k < 484; ++k)
                    if (k < 2 || k >= n + 2) {
                        uint32_t bits[2];
                        memcpy(bits, output + k, 8);
                        check(bits[0] == 0xa5a5a5a5 && bits[1] == 0xa5a5a5a5);
                    }
                ++cases;
            }
            memset(output, 0xa5, sizeof output);
            check(lhdc_fft_fixed32_execute(&plan, NULL, output) ==
                  LHDC_FFT_FIXED32_INVALID_ARGUMENT);
            check(lhdc_fft_fixed32_execute(&plan, input, input) ==
                  LHDC_FFT_FIXED32_INVALID_ARGUMENT);
            check(lhdc_fft_fixed32_execute(&plan, input, input + 1) ==
                  LHDC_FFT_FIXED32_INVALID_ARGUMENT);
            check(lhdc_fft_fixed32_execute(&plan, input, twiddles) ==
                  LHDC_FFT_FIXED32_INVALID_ARGUMENT);
            LhdcFftFixed32Plan bad = plan;
            bad.length = 119;
            check(lhdc_fft_fixed32_execute(&bad, input, output) ==
                  LHDC_FFT_FIXED32_UNSUPPORTED_LENGTH);
            for (unsigned k = 0; k < sizeof output; ++k)
                check(((unsigned char *)output)[k] == 0xa5);
        }
    printf(
        "{\"cases\":%u,\"lengths\":4,\"directions\":2,\"max_error_vs_dft_integer_units\":%.12g,\"bounds\":\"passed\"}\n",
        cases, worst);
    return 0;
}
