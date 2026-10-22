/* Generic DCT-IV C tests: formula, self-inverse, in-place output, odd radices, and capacity boundaries. */
#include "../include/dsp/dct4_float32.h"
#include <audio_math/constants.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static kiss_fft_cpx rotations[1441], packed[481], spectrum[481], scratch[481];
static float input[960], output[960], original[960];
static uint32_t rng = 0x18fc3151;
/* Generate bounded floating-point samples from a fixed pseudorandom sequence. */
static float random_value(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float)(int32_t)rng / 2147483648.0f;
}
/* Report the line number and abort the test if an assertion fails. */
static void require_at(int ok, int line) {
    if (!ok) {
        fprintf(stderr, "DCT API failure at line %d\n", line);
        exit(2);
    }
}
#define require(expression) require_at((expression), __LINE__)
/* Verify DCT-IV formula accuracy, workspace binding, scaling, and in-place operation. */
int main(void) {
    const int lengths[] = {2, 4, 6, 8, 10, 14, 22, 32, 106, 220, 240, 480, 960};
    LhdcDct4F32Plan plan = {0};
    require(!lhdc_dct4_f32_rotation_floats(0) && !lhdc_dct4_f32_rotation_floats(3) &&
            !lhdc_dct4_f32_rotation_floats(-2));
    require(lhdc_dct4_f32_init(NULL, 220, rotations, 660) == LHDC_DCT4_INVALID_ARGUMENT);
    require(lhdc_dct4_f32_init(&plan, 220, rotations, 659) == LHDC_DCT4_BUFFER_TOO_SMALL);
    double worst = 0, worst_inverse = 0;
    unsigned cases = 0;
    for (unsigned index = 0; index < sizeof lengths / sizeof lengths[0]; ++index) {
        int n = lengths[index];
        for (unsigned i = 0; i < 1441; ++i)
            rotations[i] = (kiss_fft_cpx){-12345.0f, -12345.0f};
        for (unsigned i = 0; i < 481; ++i)
            packed[i] = spectrum[i] = scratch[i] = (kiss_fft_cpx){-12345.0f, -12345.0f};
        require(lhdc_dct4_f32_init(&plan, n, rotations, 3 * (size_t)n) == 0);
        size_t rs = lhdc_dct4_f32_radix_scratch_floats(&plan);
        require(rs > 0 && rs <= 960);
        LhdcDct4F32Work work = {packed, (size_t)n, spectrum, (size_t)n, scratch, rs};
        require(lhdc_dct4_f32_transform(NULL, input, output, &work) == LHDC_DCT4_INVALID_ARGUMENT);
        --work.packed_input_floats;
        require(lhdc_dct4_f32_transform(&plan, input, output, &work) == LHDC_DCT4_BUFFER_TOO_SMALL);
        ++work.packed_input_floats;
        --work.radix_scratch_floats;
        require(lhdc_dct4_f32_transform(&plan, input, output, &work) == LHDC_DCT4_BUFFER_TOO_SMALL);
        ++work.radix_scratch_floats;
        for (unsigned test = 0; test < 3; ++test) {
            for (int i = 0; i < n; ++i)
                input[i] = original[i] = test == 0   ? (i == 0 ? 1.0f : 0.0f)
                                         : test == 1 ? (i & 1 ? 1.0f : -1.0f)
                                                     : random_value();
            require(!lhdc_dct4_f32_transform(&plan, input, output, &work));
            /* Borrowed rotation tables must match owned-plan execution without copying or reinitializing coefficients. */
            LhdcDct4F32Kernel kernel = {n,
                                        plan.factors,
                                        plan.pre_rotation,
                                        plan.post_rotation,
                                        plan.fft_twiddles,
                                        1,
                                        LHDC_DCT4_SCALE_AFTER_ROTATION,
                                        plan.normalization};
            float borrowed_output[960];
            require(!lhdc_dct4_f32_execute(&kernel, input, borrowed_output, &work));
            require(memcmp(output, borrowed_output, (size_t)n * sizeof(float)) == 0);
            kernel.scaling = (LhdcDct4F32Scaling)99;
            require(lhdc_dct4_f32_execute(&kernel, input, borrowed_output, &work) ==
                    LHDC_DCT4_INVALID_ARGUMENT);
            kernel.scaling = LHDC_DCT4_SCALE_IN_ROTATION_TABLE;
            require(!lhdc_dct4_f32_execute(&kernel, input, borrowed_output, &work));
            double power = 0, error = 0, unscaled_power = 0, unscaled_error = 0;
            for (int k = 0; k < n; ++k) {
                double expected = 0;
                for (int i = 0; i < n; ++i)
                    expected += (double)input[i] * cos(AUDIO_MATH_PI_F64 / n * (i + .5) * (k + .5));
                double unscaled = expected;
                double unscaled_delta = (double)borrowed_output[k] - unscaled;
                unscaled_power += unscaled * unscaled;
                unscaled_error += unscaled_delta * unscaled_delta;
                expected *= sqrt(2.0 / n);
                double d = output[k] - expected;
                power += expected * expected;
                error += d * d;
            }
            double relative = sqrt(error / power);
            if (relative > worst)
                worst = relative;
            require(sqrt(unscaled_error / unscaled_power) < 1e-5);
            require(!lhdc_dct4_f32_transform(&plan, output, output, &work));
            power = error = 0;
            for (int i = 0; i < n; ++i) {
                double d = output[i] - original[i];
                power += (double)original[i] * original[i];
                error += d * d;
            }
            relative = sqrt(error / power);
            if (relative > worst_inverse)
                worst_inverse = relative;
            ++cases;
        }
        require(rotations[3 * n / 2].r == -12345.0f && packed[n / 2].r == -12345.0f &&
                spectrum[n / 2].r == -12345.0f && scratch[rs / 2].r == -12345.0f);
    }
    require(worst < 1e-5 && worst_inverse < 1e-5);
    printf(
        "{\"lengths\":13,\"cases\":%u,\"relative_l2_vs_formula\":%.12g,\"relative_l2_self_inverse\":%.12g,\"bounds\":\"passed\"}\n",
        cases, worst, worst_inverse);
    return 0;
}
