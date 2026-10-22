#include "v5_fft_twiddles_reference.h"
#include "../src/v5/codec/fft_adapter_tables.h"
#include <audio_math/numeric_bits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static kiss_fft_cpx reference_twiddles[514], current_twiddles[514];
static kiss_fft_cpx cached_twiddles[514];
static kiss_fft_cpx input[512], reference_output[514], current_output[514];
static kiss_fft_state reference_state, current_state;
static kiss_fft_state cached_state;
static LhdcTestV5FftAllocator volatile allocators[2] = {lhdc_test_v5_fft_alloc_reference,
                                                        lhdc_v5_fft_alloc};
static uint32_t random_state = UINT32_C(0x35210719);
static volatile uint64_t consumed;

/* Preserve an explicit failing check line instead of exiting silently. */
static void require_at(int ok, int line) {
    if (!ok) {
        fprintf(stderr, "V5 FFT twiddle failure at line %d\n", line);
        exit(2);
    }
}
#define require(value) require_at((value), __LINE__)

/* Generate deterministic full-width input including modular-overflow amplitudes. */
static int32_t next_sample(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return audio_math_int32_from_bits(random_state);
}

/* Check both sentinels and every byte following valid output remain untouched. */
static void check_guards(const kiss_fft_cpx *buffer, int length) {
    const unsigned char *bytes = (const unsigned char *)buffer;
    for (size_t i = 0; i < sizeof(kiss_fft_cpx); ++i)
        require(bytes[i] == 0xa5);
    for (size_t i = (size_t)(length + 1) * sizeof *buffer; i < sizeof current_twiddles; ++i)
        require(bytes[i] == 0xa5);
}

/* Compare initialized state, every twiddle, and factorization; retain dynamic handling for unknown lengths. */
static void check_initialization(int length, int inverse) {
    memset(&reference_state, 0xa5, sizeof reference_state);
    memset(&current_state, 0xa5, sizeof current_state);
    memset(reference_twiddles, 0xa5, sizeof reference_twiddles);
    memset(current_twiddles, 0xa5, sizeof current_twiddles);
    size_t before = sizeof reference_state, after = sizeof current_state;
    require(allocators[0](length, inverse, &reference_state, &before, reference_twiddles + 1) ==
            &reference_state);
    require(allocators[1](length, inverse, &current_state, &after, current_twiddles + 1) ==
            &current_state);
    require(before == after && after == sizeof current_state);
    require(reference_state.nfft == current_state.nfft &&
            reference_state.inverse == current_state.inverse);
    require(reference_state.twiddles == reference_twiddles + 1 &&
            current_state.twiddles == current_twiddles + 1);
    require(!memcmp(reference_state.factors, current_state.factors, sizeof current_state.factors));
    require(!memcmp(reference_twiddles, current_twiddles, sizeof current_twiddles));
    check_guards(current_twiddles, length);
    memset(&cached_state, 0xa5, sizeof cached_state);
    memset(cached_twiddles, 0xa5, sizeof cached_twiddles);
    size_t cached_bytes = sizeof cached_state;
    require(lhdc_test_v5_fft_alloc_cached_reference(length, inverse, &cached_state, &cached_bytes,
                                                    cached_twiddles + 1) == &cached_state);
    require(cached_bytes == after && cached_state.nfft == current_state.nfft &&
            cached_state.inverse == current_state.inverse);
    require(cached_state.twiddles == cached_twiddles + 1);
    require(!memcmp(cached_state.factors, current_state.factors, sizeof current_state.factors));
    require(!memcmp(cached_twiddles, current_twiddles, sizeof current_twiddles));
    if (length == 60 || length == 120 || length == 240 || length == 480) {
        for (unsigned i = 0; i < 64; i += 2) {
            require(reference_state.factors[i] > 0 &&
                    reference_state.factors[i] <= LHDC_V5_FIXED_FFT_MAX_RADIX);
            if (reference_state.factors[i + 1] == 1)
                break;
            require(i < 62);
        }
    }
}

/* Compare complete fixed-FFT output bitwise using the same input and two initialized plans. */
static void check_transform(int length, int inverse, unsigned pattern) {
    check_initialization(length, inverse);
    for (int i = 0; i < length; ++i) {
        switch (pattern) {
        case 0:
            input[i] = (kiss_fft_cpx){0, 0};
            break;
        case 1:
            input[i] = (kiss_fft_cpx){i == 0 ? INT32_MAX : 0, 0};
            break;
        case 2:
            input[i] = (kiss_fft_cpx){INT32_MAX, INT32_MIN};
            break;
        case 3:
            input[i] = (kiss_fft_cpx){i & 1 ? INT32_MIN : INT32_MAX, i & 2 ? -1 : 1};
            break;
        default:
            input[i] = (kiss_fft_cpx){next_sample(), next_sample()};
            break;
        }
    }
    memset(reference_output, 0xa5, sizeof reference_output);
    memset(current_output, 0xa5, sizeof current_output);
    lhdc_v5_fft(&reference_state, input, reference_output + 1);
    lhdc_v5_fft(&current_state, input, current_output + 1);
    require(!memcmp(reference_output, current_output, sizeof current_output));
    check_guards(current_output, length);
}

/* Read the monotonic benchmark clock. */
static double now_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER count, frequency;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&frequency);
    return (double)count.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec count;
    clock_gettime(CLOCK_MONOTONIC, &count);
    return count.tv_sec + count.tv_nsec * 1e-9;
#endif
}

/* Alternate full-initialization timing, including coefficient writes and factorization rather than lookup alone. */
static int benchmark(void) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    const int lengths[] = {60, 120, 240, 480};
    for (unsigned index = 0; index < 4; ++index)
        for (int inverse = 0; inverse < 2; ++inverse)
            for (unsigned round = 0; round < 7; ++round) {
                double elapsed[2];
                uint64_t checksums[2];
                for (unsigned visit = 0; visit < 2; ++visit) {
                    unsigned side = (visit + round) & 1;
                    LhdcTestV5FftAllocator allocator = allocators[side];
                    uint64_t checksum = 0;
                    double start = now_seconds();
                    for (unsigned iteration = 0; iteration < 2048; ++iteration) {
                        size_t bytes = sizeof current_state;
                        require(allocator(lengths[index], inverse, &current_state, &bytes,
                                          current_twiddles) == &current_state);
                        unsigned k = iteration % (unsigned)lengths[index];
                        checksum += (uint32_t)current_twiddles[k].r;
                        checksum += (uint32_t)current_twiddles[k].i;
                        checksum += (unsigned)current_state.factors[0];
                    }
                    elapsed[side] = now_seconds() - start;
                    checksums[side] = checksum;
                    consumed = checksum;
                }
                require(checksums[0] == checksums[1]);
                printf("{\"length\":%d,\"inverse\":%d,\"round\":%u,\"operations\":2048,"
                       "\"before\":%.12g,\"after\":%.12g,\"checksum\":\"%016" PRIx64 "\"}\n",
                       lengths[index], inverse, round, elapsed[0], elapsed[1], checksums[0]);
            }
    return 0;
}

/* Export initialized coefficients for four lengths and both directions for offline master-table verification. */
static int dump_tables(void) {
    const int lengths[] = {60, 120, 240, 480};
    for (unsigned index = 0; index < 4; ++index)
        for (int inverse = 0; inverse < 2; ++inverse) {
            check_initialization(lengths[index], inverse);
            printf("{\"length\":%d,\"inverse\":%d,\"twiddles\":[", lengths[index], inverse);
            for (int i = 0; i < lengths[index]; ++i)
                printf("%s[%" PRId32 ",%" PRId32 "]", i ? "," : "", current_state.twiddles[i].r,
                       current_state.twiddles[i].i);
            printf("],\"factors\":[");
            for (unsigned i = 0; i < 64; i += 2) {
                require(current_state.factors[i] > 0);
                printf("%s%d,%d", i ? "," : "", current_state.factors[i],
                       current_state.factors[i + 1]);
                if (current_state.factors[i + 1] == 1)
                    break;
                require(i < 62);
            }
            puts("]}");
        }
    return 0;
}

/* Cover dynamic lengths 1..512, all direction conventions, size/capacity boundaries, and complete FFT output. */
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--dump"))
        return dump_tables();
    if (argc == 2 && !strcmp(argv[1], "--bench"))
        return benchmark();
    const int directions[] = {0, 1, -1, 2};
    unsigned initializations = 0, transforms = 0;
    for (int length = 1; length <= 512; ++length)
        for (unsigned direction = 0; direction < 4; ++direction) {
            check_initialization(length, directions[direction]);
            ++initializations;
        }
    const int lengths[] = {60, 120, 240, 480};
    for (unsigned index = 0; index < 4; ++index)
        for (unsigned direction = 0; direction < 4; ++direction)
            for (unsigned pattern = 0; pattern < 12; ++pattern) {
                check_transform(lengths[index], directions[direction], pattern);
                ++transforms;
            }
    for (unsigned side = 0; side < 2; ++side) {
        size_t size = 0;
        require(!allocators[side](480, 0, NULL, &size, NULL));
        require(size == sizeof(kiss_fft_state));
        size = sizeof(kiss_fft_state) - 1;
        memset(&current_state, 0xa5, sizeof current_state);
        memset(current_twiddles, 0xa5, sizeof current_twiddles);
        require(!allocators[side](480, 0, &current_state, &size, current_twiddles));
        require(size == sizeof(kiss_fft_state));
        const unsigned char *bytes = (const unsigned char *)&current_state;
        for (size_t i = 0; i < sizeof current_state; ++i)
            require(bytes[i] == 0xa5);
        require(!allocators[side](480, 0, &current_state, NULL, current_twiddles));
        for (size_t i = 0; i < sizeof current_twiddles; ++i)
            require(((const unsigned char *)current_twiddles)[i] == 0xa5);
    }
    printf("{\"initializations\":%u,\"transforms\":%u,\"lengths\":512,\"directions\":4,"
           "\"bounds\":\"passed\"}\n",
           initializations, transforms);
    return 0;
}
