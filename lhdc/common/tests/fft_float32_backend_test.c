/* Verify bit-identical twiddles and complex outputs for the optional shared backend and default recursive backend. */
#include "../include/dsp/fft_float32.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#endif

void lhdc_reference_fft_work(kiss_fft_cpx *, const kiss_fft_cpx *, size_t, int32_t, const int32_t *,
                             const LhdcKissFftPlan *);
void lhdc_reference_fft_twiddles(int32_t, int32_t, kiss_fft_cpx *);
void lhdc_reference_fft_512(float *, int, LhdcKissFft512Work *);

/* Generate deterministic zero, impulse, wide-noise, and small-noise inputs including negative zero. */
static void fill_input(kiss_fft_cpx *input, int count, int pattern) {
    uint32_t random = UINT32_C(0x764813ef);
    for (int i = 0; i < count; ++i) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        float real = (float)((int32_t)(random & 65535) - 32768);
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        float imag = (float)((int32_t)(random & 65535) - 32768);
        switch (pattern) {
        case 0:
            real = 0.0f;
            imag = -0.0f;
            break;
        case 1:
            real = i == 0 ? 1.0f : 0.0f;
            imag = 0.0f;
            break;
        case 2:
            break;
        case 3:
            real *= 0x1p-40f;
            imag *= 0x1p-40f;
            break;
        case 4:
            real = i & 1 ? -1.0f : 1.0f;
            imag = real;
            break;
        default:
            real *= 0x1p40f;
            imag *= 0x1p40f;
            break;
        }
        input[i] = (kiss_fft_cpx){real, imag};
    }
}

/* Place the shortest valid factor array before a guard page to detect reads beyond its terminating pair. */
static int check_short_factor_bounds(void) {
    size_t page;
    unsigned char *mapping;
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    page = info.dwPageSize;
    mapping = VirtualAlloc(NULL, 2 * page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD old_protection;
    if (!mapping || !VirtualProtect(mapping + page, page, PAGE_NOACCESS, &old_protection))
        return 0;
#else
    page = (size_t)sysconf(_SC_PAGESIZE);
    mapping = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED || mprotect(mapping + page, page, PROT_NONE))
        return 0;
#endif
    int32_t *factors = (int32_t *)(mapping + page - 2 * sizeof(int32_t));
    factors[0] = 60;
    factors[1] = 1;
    kiss_fft_cpx *memory = calloc(300, sizeof *memory);
    if (!memory)
        return 0;
    kiss_fft_cpx *input = memory, *output = memory + 60, *reference = memory + 120,
                 *twiddles = memory + 180;
    fill_input(input, 60, 2);
    lhdc_kiss_fft_twiddles(60, 0, twiddles);
    const LhdcKissFftPlan plan = {60, 0, twiddles, 1, memory + 240};
    lhdc_kiss_fft_work(output, input, 1, 1, factors, &plan);
    lhdc_reference_fft_work(reference, input, 1, 1, factors, &plan);
    int equal = memcmp(output, reference, 60 * sizeof *output) == 0;
    free(memory);
#ifdef _WIN32
    VirtualFree(mapping, 0, MEM_RELEASE);
#else
    munmap(mapping, 2 * page);
#endif
    return equal;
}

/* Compare the default 512-point implementation byte-for-byte with the SSE2-disabled scalar reference. */
static int check_fft_512_byte_identity(void) {
    float actual[1024];
    float reference[1024];
    LhdcKissFft512Work actual_work;
    LhdcKissFft512Work reference_work;

    for (int inverse = 0; inverse < 2; ++inverse) {
        for (uint32_t sample = 0; sample < 128; ++sample) {
            uint32_t random = UINT32_C(0x639a2026) ^ (sample * UINT32_C(0x9e3779b9));
            for (int i = 0; i < 1024; ++i) {
                random ^= random << 13;
                random ^= random >> 17;
                random ^= random << 5;
                int32_t value = (int32_t)(random & UINT32_C(0x1ffff)) - 65536;
                float scale = sample & 1 ? 0x1p-16f : 1.0f;
                actual[i] = (float)value * scale;
            }
            if (sample == 0)
                memset(actual, 0, sizeof actual);
            else if (sample == 1) {
                memset(actual, 0, sizeof actual);
                actual[0] = 1.0f;
                actual[1] = -0.0f;
            }
            memcpy(reference, actual, sizeof actual);
            lhdc_kiss_fft_512(actual, inverse, &actual_work);
            lhdc_reference_fft_512(reference, inverse, &reference_work);
            if (memcmp(actual, reference, sizeof actual)) {
                fprintf(stderr, "512-point FFT mismatch: inverse=%d sample=%u\n", inverse,
                        sample);
                return 0;
            }
        }
    }
    return 1;
}

/* Read the monotonic high-resolution clock used by development FFT paired benchmarks. */
static double benchmark_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return (double)value.tv_sec + (double)value.tv_nsec * 1e-9;
#endif
}

/* Time the default and forced-scalar FFT kernels with equal paired workloads. */
static double benchmark_fft_kernel(int length, int iterations, int reference) {
    kiss_fft_cpx *memory = calloc((size_t)length * 4, sizeof *memory);
    if (!memory)
        return -1.0;
    kiss_fft_cpx *twiddles = memory;
    kiss_fft_cpx *input = memory + length;
    kiss_fft_cpx *output = memory + 2 * length;
    kiss_fft_cpx *scratch = memory + 3 * length;
    int32_t factors[64];
    lhdc_kiss_fft_twiddles(length, 0, twiddles);
    lhdc_kiss_fft_factor(length, (int32_t)sqrt((double)length), factors);
    fill_input(input, length, 2);
    const LhdcKissFftPlan plan = {length, 0, twiddles, 1, scratch};
    for (int warmup = 0; warmup < 32; ++warmup) {
        if (reference)
            lhdc_reference_fft_work(output, input, 1, 1, factors, &plan);
        else
            audio_math_fft_work(output, input, 1, 1, factors, &plan);
    }
    double start = benchmark_seconds();
    for (int iteration = 0; iteration < iterations; ++iteration) {
        if (reference)
            lhdc_reference_fft_work(output, input, 1, 1, factors, &plan);
        else
            audio_math_fft_work(output, input, 1, 1, factors, &plan);
    }
    double elapsed = benchmark_seconds() - start;
    volatile float sink = output[length / 2].r;
    (void)sink;
    free(memory);
    return elapsed * 1e6 / iterations;
}

/* Measure a fixed 512-point forward plus normalized inverse transform separately from generic dispatch. */
static double benchmark_fft_512_kernel(int iterations, int reference) {
    float interleaved[1024];
    LhdcKissFft512Work work;
    fill_input((kiss_fft_cpx *)interleaved, 512, 2);
    for (int warmup = 0; warmup < 16; ++warmup) {
        if (reference) {
            lhdc_reference_fft_512(interleaved, 0, &work);
            lhdc_reference_fft_512(interleaved, 1, &work);
        } else {
            lhdc_kiss_fft_512(interleaved, 0, &work);
            lhdc_kiss_fft_512(interleaved, 1, &work);
        }
    }
    double start = benchmark_seconds();
    for (int iteration = 0; iteration < iterations; ++iteration) {
        if (reference) {
            lhdc_reference_fft_512(interleaved, 0, &work);
            lhdc_reference_fft_512(interleaved, 1, &work);
        } else {
            lhdc_kiss_fft_512(interleaved, 0, &work);
            lhdc_kiss_fft_512(interleaved, 1, &work);
        }
    }
    double elapsed = benchmark_seconds() - start;
    volatile float sink = interleaved[512];
    (void)sink;
    return elapsed * 1e6 / iterations;
}

/* Report scalar/SSE2 median times and speedups for practical and boundary lengths. */
static int benchmark_fft_lengths(void) {
    static const int lengths[] = {32,  60,  64,  110, 120, 128, 220,
                                  240, 256, 440, 480, 512, 960, 1024};
    for (size_t index = 0; index < sizeof lengths / sizeof *lengths; ++index) {
        int length = lengths[index];
        int iterations = 3000000 / length;
        if (iterations < 2000)
            iterations = 2000;
        double scalar[7], current[7];
        for (int round = 0; round < 7; ++round) {
            if (round & 1) {
                current[round] = benchmark_fft_kernel(length, iterations, 0);
                scalar[round] = benchmark_fft_kernel(length, iterations, 1);
            } else {
                scalar[round] = benchmark_fft_kernel(length, iterations, 1);
                current[round] = benchmark_fft_kernel(length, iterations, 0);
            }
            if (scalar[round] < 0.0 || current[round] < 0.0)
                return 2;
        }
        for (int i = 1; i < 7; ++i) {
            double scalar_value = scalar[i], current_value = current[i];
            int j = i;
            while (j && scalar[j - 1] > scalar_value) {
                scalar[j] = scalar[j - 1];
                --j;
            }
            scalar[j] = scalar_value;
            j = i;
            while (j && current[j - 1] > current_value) {
                current[j] = current[j - 1];
                --j;
            }
            current[j] = current_value;
        }
        printf("length=%d scalar_us=%.6f current_us=%.6f speedup=%.4f\n", length, scalar[3],
               current[3], scalar[3] / current[3]);
    }
    double scalar[7], current[7];
    for (int round = 0; round < 7; ++round) {
        if (round & 1) {
            current[round] = benchmark_fft_512_kernel(4000, 0);
            scalar[round] = benchmark_fft_512_kernel(4000, 1);
        } else {
            scalar[round] = benchmark_fft_512_kernel(4000, 1);
            current[round] = benchmark_fft_512_kernel(4000, 0);
        }
    }
    for (int i = 1; i < 7; ++i) {
        double scalar_value = scalar[i], current_value = current[i];
        int j = i;
        while (j && scalar[j - 1] > scalar_value) {
            scalar[j] = scalar[j - 1];
            --j;
        }
        scalar[j] = scalar_value;
        j = i;
        while (j && current[j - 1] > current_value) {
            current[j] = current[j - 1];
            --j;
        }
        current[j] = current_value;
    }
    printf("fixed512_pair scalar_us=%.6f current_us=%.6f speedup=%.4f\n", scalar[3],
           current[3], scalar[3] / current[3]);
    return 0;
}

/* Cover static and unlisted lengths, both directions, input stride, and alternate mixed-radix factorizations. */
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--benchmark") == 0)
        return benchmark_fft_lengths();
    static const int lengths[] = {1,   2,   3,   4,   5,   7,   8,   9,   10,  11,  13, 15,  16,
                                  17,  20,  30,  32,  33,  40,  55,  60,  64,  65,  80, 110, 120,
                                  128, 160, 220, 240, 256, 257, 320, 440, 480, 512, 960, 1024};
    unsigned cases = 0, tables = 0;
    for (size_t index = 0; index < sizeof lengths / sizeof *lengths; ++index) {
        int length = lengths[index];
        kiss_fft_cpx *memory = calloc((size_t)length * 9, sizeof *memory);
        if (!memory)
            return 2;
        kiss_fft_cpx *twiddles = memory, *reference_twiddles = memory + 2 * length;
        kiss_fft_cpx *input = memory + 4 * length;
        kiss_fft_cpx *output = memory + 6 * length, *reference = memory + 7 * length;
        for (int inverse = 0; inverse < 2; ++inverse) {
            for (int subplan = 0; subplan < 2; ++subplan) {
                int plan_length = length * (subplan + 1);
                lhdc_kiss_fft_twiddles(plan_length, inverse, twiddles);
                lhdc_reference_fft_twiddles(plan_length, inverse, reference_twiddles);
                if (memcmp(twiddles, reference_twiddles, (size_t)plan_length * sizeof *twiddles)) {
                    fprintf(stderr, "twiddle mismatch: N=%d inverse=%d\n", plan_length, inverse);
                    return 3;
                }
                ++tables;
                for (int alternate = 0; alternate < 2; ++alternate) {
                    int32_t factors[64];
                    if (alternate && length > 2 && !(length & 1)) {
                        factors[0] = 2;
                        factors[1] = length / 2;
                        lhdc_kiss_fft_factor(length / 2, (int32_t)sqrt((double)(length / 2)),
                                             factors + 2);
                    } else {
                        lhdc_kiss_fft_factor(length, (int32_t)sqrt((double)length), factors);
                    }
                    const LhdcKissFftPlan plan = {plan_length, inverse, twiddles, 1, memory + 8 * length};
                    for (int pattern = 0; pattern < 6; ++pattern) {
                        fill_input(input, 2 * length, pattern);
                        memset(output, 0xa5, (size_t)length * sizeof *output);
                        memset(reference, 0xa5, (size_t)length * sizeof *reference);
                        lhdc_kiss_fft_work(output, input, subplan + 1, 1, factors, &plan);
                        lhdc_reference_fft_work(reference, input, subplan + 1, 1, factors, &plan);
                        if (memcmp(output, reference, (size_t)length * sizeof *output)) {
                            fprintf(
                                stderr,
                                "FFT mismatch: N=%d direction=%d stride=%d alternate=%d pattern=%d\n",
                                length, inverse, subplan + 1, alternate, pattern);
                            return 4;
                        }
                        ++cases;
                    }
                }
            }
        }
        free(memory);
    }
    if (!check_short_factor_bounds())
        return 5;
    ++cases;
    if (!check_fft_512_byte_identity())
        return 6;
    cases += 256;
    printf(
        "{\"cases\":%u,\"twiddle_tables\":%u,\"short_factor_guard\":\"passed\",\"byte_identity\":\"passed\"}\n",
        cases, tables);
    return 0;
}
