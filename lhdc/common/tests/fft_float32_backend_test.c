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
#include <unistd.h>
#endif

void lhdc_reference_fft_work(kiss_fft_cpx *, const kiss_fft_cpx *, size_t, int32_t, const int32_t *,
                             const LhdcKissFftPlan *);
void lhdc_reference_fft_twiddles(int32_t, int32_t, kiss_fft_cpx *);

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

/* Cover static and unlisted lengths, both directions, input stride, and alternate mixed-radix factorizations. */
int main(void) {
    static const int lengths[] = {1,   2,   3,   4,   5,   7,   8,   9,   10,  11,  13, 15,  16,
                                  17,  20,  30,  32,  33,  40,  55,  60,  64,  65,  80, 110, 120,
                                  128, 160, 220, 240, 256, 257, 320, 440, 480, 512, 960};
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
    printf(
        "{\"cases\":%u,\"twiddle_tables\":%u,\"short_factor_guard\":\"passed\",\"byte_identity\":\"passed\"}\n",
        cases, tables);
    return 0;
}
