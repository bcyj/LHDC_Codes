#include "../src/v4/include/encoder_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Report failing locations for CTest and standalone binary-record tests. */
static void check_at(int valid, int line) {
    if (!valid) {
        fprintf(stderr, "quality preset test failed at %d\n", line);
        exit(2);
    }
}
#define check(condition) check_at((condition), __LINE__)

/* Construct safe nondefault presets that round-trip through decimal notation. */
static LhdcV4QualityPreset row(unsigned index) {
    LhdcV4QualityPreset preset;
    preset.min_bits_to_keep = (float)(index + 1) / 8.0f;
    preset.upper_freq = 2000 + (int32_t)index * 16;
    preset.avg_power_shift = (float)index / 16.0f;
    preset.noise_floor_shift = (float)(index + 1) / 32.0f;
    preset.noise_floor_dB = -80.0f + (float)index / 4.0f;
    return preset;
}

/* Cover three rates, two depths, and sixteen qualities; verify host presets actually reach instances. */
static void test_derived(FFT_BLOCK *encoder, const LhdcV4QualityPresets *presets) {
    const int rates[] = {44100, 48000, 96000};
    for (unsigned rate = 0; rate < 3; ++rate) {
        for (int bits = 16; bits <= 24; bits += 8) {
            for (int quality = 0; quality < LHDC_V4_QUALITY_PRESET_COUNT; ++quality) {
                encoder->sample_rate = rates[rate];
                encoder->bits_per_sample = bits;
                encoder->fft_len = 512;
                encoder->bin_low = 1;
                memset(encoder->fft_freq_factor, 0, sizeof encoder->fft_freq_factor);
                v4f_SetQuality(encoder, quality);
                const LhdcV4QualityPreset *preset =
                    bits == 16 ? &presets->pcm16[quality] : &presets->pcm24[quality];
                check(encoder->quality == quality);
                check(encoder->upper_freq == preset->upper_freq);
                check(encoder->avg_power_shift == preset->avg_power_shift);
            }
        }
    }
}

/* Verify file-free copying, null arguments, self-copy, isolation, and no implicit quality reapplication. */
static void test_memory_api(FFT_BLOCK *encoder, const LhdcV4QualityPresets *seed) {
    LhdcV4QualityPresets output;
    FFT_BLOCK *other = calloc(1, sizeof *other);
    check(other != NULL);
    v4f_RestoreDefQualityPreset(other);
    LhdcV4QualityPresets defaults = other->quality_presets;
    check(v4f_LossyEncoderSetQualityPresets(NULL, seed) == LHDC_V4_STATUS_ERROR);
    check(v4f_LossyEncoderSetQualityPresets(encoder, NULL) == LHDC_V4_STATUS_ERROR);
    check(v4f_LossyEncoderGetQualityPresets(NULL, &output) == LHDC_V4_STATUS_ERROR);
    check(v4f_LossyEncoderGetQualityPresets(encoder, NULL) == LHDC_V4_STATUS_ERROR);
    encoder->quality = 7;
    check(v4f_LossyEncoderSetQualityPresets(encoder, seed) == LHDC_V4_STATUS_OK);
    check(encoder->quality == 7);
    check(v4f_LossyEncoderGetQualityPresets(encoder, &output) == LHDC_V4_STATUS_OK);
    check(memcmp(&output, seed, sizeof output) == 0);
    check(v4f_LossyEncoderSetQualityPresets(encoder, &encoder->quality_presets) ==
          LHDC_V4_STATUS_OK);
    check(v4f_LossyEncoderGetQualityPresets(encoder, &encoder->quality_presets) ==
          LHDC_V4_STATUS_OK);
    check(memcmp(&other->quality_presets, &defaults, sizeof defaults) == 0);
    free(other);
}

/* Verify file-free configuration, instance isolation, quality application, and default restoration. */
int main(void) {
    FFT_BLOCK *encoder = calloc(1, sizeof *encoder);
    check(encoder != NULL);
    LhdcV4QualityPresets seed, output, defaults;
    for (unsigned i = 0; i < LHDC_V4_QUALITY_PRESET_COUNT; ++i) {
        seed.pcm16[i] = row(i + 64);
        seed.pcm24[i] = row(i + 80);
    }
    v4f_RestoreDefQualityPreset(encoder);
    check(v4f_LossyEncoderGetQualityPresets(encoder, &defaults) == LHDC_V4_STATUS_OK);
    test_memory_api(encoder, &seed);
    test_derived(encoder, &seed);
    v4f_RestoreDefQualityPreset(encoder);
    check(v4f_LossyEncoderGetQualityPresets(encoder, &output) == LHDC_V4_STATUS_OK);
    check(memcmp(&output, &defaults, sizeof defaults) == 0);
    free(encoder);
    puts("PASS quality presets: memory API, instance isolation, default restore, derived=96");
    return 0;
}
