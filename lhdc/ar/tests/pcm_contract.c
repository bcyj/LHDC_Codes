#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int32_t v4f_GetOneSample(int32_t, uint8_t *, int32_t *);
int32_t v5f_GetOneSample(int32_t, uint8_t *, int32_t *);
int32_t v4f_SaveOneSample(int32_t, uint8_t *, int32_t);
int32_t v5f_SaveOneSample(int32_t, uint8_t *, int32_t);

/* Verify unaligned little-endian PCM access, sign extension, clipping, and invalid-width guards. */
int main(void) {
    int32_t (*read_sample[2])(int32_t, uint8_t *, int32_t *) = {v4f_GetOneSample, v5f_GetOneSample};
    int32_t (*write_sample[2])(int32_t, uint8_t *, int32_t) = {v4f_SaveOneSample,
                                                               v5f_SaveOneSample};
    static const int32_t values[] = {INT32_MIN, -8388609, -8388608, -32769,  -32768,  -1,       0,
                                     1,         32767,    32768,    8388607, 8388608, INT32_MAX};
    unsigned checked = 0;
    for (unsigned version = 0; version < 2; ++version) {
        for (int bytes = 2; bytes <= 4; ++bytes) {
            int64_t magnitude = INT64_C(1) << (bytes * 8 - 1);
            for (unsigned i = 0; i < sizeof values / sizeof *values; ++i) {
                int64_t expected = values[i];
                if (expected >= magnitude)
                    expected = magnitude - 1;
                if (expected < -magnitude)
                    expected = -magnitude;
                for (unsigned offset = 1; offset < 8; ++offset) {
                    uint8_t data[16];
                    memset(data, 0x5a, sizeof data);
                    int32_t actual;
                    if (write_sample[version](bytes, data + offset, values[i]) ||
                        read_sample[version](bytes, data + offset, &actual) || actual != expected)
                        return 1;
                    for (int j = 0; j < bytes; ++j)
                        if (data[offset + j] != (uint8_t)((uint32_t)expected >> (j * 8)))
                            return 2;
                    if (data[offset - 1] != 0x5a || data[offset + bytes] != 0x5a)
                        return 3;
                    ++checked;
                }
            }
        }
        static const int invalid[] = {-1, 5, INT_MIN, INT_MAX};
        for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
            uint8_t guard[16], original[16];
            memset(guard, 0x5a, sizeof guard);
            memcpy(original, guard, sizeof guard);
            if (write_sample[version](invalid[i], guard + 1, 123) != -1 ||
                memcmp(guard, original, sizeof guard))
                return 4;
        }
    }
    printf("PASS PCM contracts: %u nonaligned signed/clamped little-endian roundtrips\n", checked);
    return 0;
}
