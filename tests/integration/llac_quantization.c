/* LLAC quantized integers must survive entropy coding exactly; cover normal and HR escape-level limits. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../lhdc/decoder/src/v4/include/llac_spectral.h"
#include "../../lhdc/encoder/src/v4/include/llac/spectral_coding.h"

enum { SAMPLES = 240, FRAME_BYTES = 100, CAPACITY = 2048 };

/* Independently compute full levels by integer division, retaining the special zero-pair marker. */
static int expected_level(int first, int second) {
    unsigned a = first < 0 ? 0u - (unsigned)first : (unsigned)first;
    unsigned b = second < 0 ? 0u - (unsigned)second : (unsigned)second;
    unsigned magnitude = a > b ? a : b;
    if (!magnitude)
        return -1;
    int level = 0;
    while (magnitude >= 4) {
        magnitude /= 2;
        ++level;
    }
    return level;
}

/* Encode quantized integers and decode immediately with enough budget to restore every LSB. */
static int roundtrip(int *q, const LlacSpectralPairCode *coding, int lastnz, int lsb, int hr,
                     int frame_bytes, int *decoded) {
    uint8_t stream[CAPACITY] = {0}, residual[CAPACITY] = {0};
    LLACTnsData tns = {0};
    int levels[SAMPLES] = {0};
    int seed = 0, zero = 0, residual_bits = 0, present = 0, bfi = 0;
    v4f_llac_encode_spectral_symbols(stream, frame_bytes - 1, 1, q, &tns, lastnz, coding, residual,
                                     0, lsb, frame_bytes * 8, 0);
    memset(decoded, 0, SAMPLES * sizeof *decoded);
    llac_decode_spectral_symbols(stream, frame_bytes - 1, 1, SAMPLES, 4, 0, &tns, lsb, lastnz, &bfi,
                                 0, 0, residual, decoded, &seed, &zero, frame_bytes, &residual_bits,
                                 &present, levels, hr);
    return bfi;
}

/* Check the full normal integer domain, quantization boundaries/neighbors, both low-bit forms, and oversized-escape rejection. */
int main(int argc, char **argv) {
    FILE *records = argc == 2 ? fopen(argv[1], "wb") : NULL;
    if (argc > 2 || (argc == 2 && !records))
        return 2;
    for (int value = -32768; value <= 32767; ++value) {
        float input[2] = {(float)value, 0};
        int q[2] = {0}, bits = 0, bits2 = 0, nz = 0, lsb = 0;
        LlacSpectralPairCode code[1] = {0};
        v4f_llac_quantize_spectrum(input, 1.0f, q, 2, 800, &bits, &bits2, 48000, &nz, code, &lsb,
                                   -1, 800, 0);
        if (code[0].escape_count != expected_level(q[0], q[1])) {
            fprintf(stderr, "invalid escape count for %d: %d\n", value, code[0].escape_count);
            return 3;
        }
    }
    const int magnitudes[] = {0,       1,       3,       4,       7,       8,       8190,
                              8191,    8192,    8193,    8194,    32766,   32767,   32768,
                              32769,   2097150, 2097151, 2097152, 4194301, 4194302, 4194303,
                              4194304, 8388601, 8388602, 8388603, 8388604, 8388605, 8388606};
    unsigned checked = 0;
    for (unsigned i = 0; i < sizeof magnitudes / sizeof magnitudes[0]; ++i)
        for (int hr = 0; hr <= 1; ++hr)
            for (int sign = -1; sign <= 1; sign += 2)
                for (int partner = 0; partner < 3; ++partner)
                    for (int mode = -1; mode <= 1; ++mode) {
                        if (!hr && magnitudes[i] > 32769)
                            continue;
                        float input[SAMPLES] = {0};
                        int q[SAMPLES] = {0};
                        LlacSpectralPairCode code[SAMPLES / 2] = {0};
                        int bits = 0, bits2 = 0, nz = 0, lsb = 0;
                        input[0] = (float)(sign * magnitudes[i]);
                        input[1] = partner == 0 ? 0.0f : partner == 1 ? -1.0f : 3.0f;
                        v4f_llac_quantize_spectrum(input, 1.0f, q, SAMPLES, 800, &bits, &bits2,
                                                   48000, &nz, code, &lsb, mode, 800, hr);
                        if (code[0].escape_count != expected_level(q[0], q[1]))
                            return 4;
                        for (int write_lsb = 0; write_lsb <= 1; ++write_lsb) {
                            int decoded[SAMPLES];
                            if (roundtrip(q, code, nz, write_lsb, hr, FRAME_BYTES, decoded) ||
                                memcmp(q, decoded, sizeof q)) {
                                fprintf(stderr, "roundtrip failed: %d sign=%d hr=%d lsb=%d\n",
                                        magnitudes[i], sign, hr, write_lsb);
                                return 5;
                            }
                            int32_t row[] = {magnitudes[i],
                                             sign,
                                             partner,
                                             mode,
                                             write_lsb,
                                             hr,
                                             q[0],
                                             q[1],
                                             code[0].context,
                                             code[0].escape_count,
                                             code[0].symbol,
                                             bits,
                                             bits2,
                                             decoded[0],
                                             decoded[1]};
                            if (records && fwrite(row, sizeof row, 1, records) != 1)
                                return 6;
                            ++checked;
                        }
                    }
    for (int hr = 0; hr <= 1; ++hr) {
        int level = hr ? 22 : 15;
        int q[SAMPLES] = {0}, decoded[SAMPLES];
        LlacSpectralPairCode code[SAMPLES / 2] = {0};
        q[0] = 1 << (level + 1);
        code[0].escape_count = level;
        code[0].symbol = 2;
        if (roundtrip(q, code, 2, 0, hr, FRAME_BYTES, decoded) != 1)
            return 7;
    }
    /* Extreme pairs cross the half-spectrum context switch to verify synchronization after the new terminal level. */
    for (int hr = 0; hr <= 1; ++hr)
        for (int write_lsb = 0; write_lsb <= 1; ++write_lsb) {
            float input[SAMPLES] = {0};
            int q[SAMPLES] = {0}, decoded[SAMPLES];
            LlacSpectralPairCode code[SAMPLES / 2] = {0};
            int bits = 0, bits2 = 0, nz = 0, lsb = 0;
            input[0] = 8192.0f;
            input[1] = -3.0f;
            input[2] = hr ? -8388606.0f : -32768.0f;
            input[3] = hr ? 2097151.0f : 8192.0f;
            input[128] = -8192.0f;
            input[129] = hr ? 8388606.0f : -32768.0f;
            input[238] = hr ? 4194303.0f : 32767.0f;
            input[239] = -1.0f;
            v4f_llac_quantize_spectrum(input, 1.0f, q, SAMPLES, 5000, &bits, &bits2, 48000, &nz,
                                       code, &lsb, -1, 5000, hr);
            if (roundtrip(q, code, nz, write_lsb, hr, 625, decoded) || memcmp(q, decoded, sizeof q))
                return 9;
            if (records && fwrite(decoded, sizeof decoded, 1, records) != 1)
                return 10;
        }
    if (records && fclose(records))
        return 8;
    printf(
        "LLAC quantization: 65536 normal integers, %u exact roundtrips, 4 context chains, 2 overescape rejections passed\n",
        checked);
    return 0;
}
