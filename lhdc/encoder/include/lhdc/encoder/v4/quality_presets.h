#ifndef LHDC_ENCODER_V4_QUALITY_PRESETS_H
#define LHDC_ENCODER_V4_QUALITY_PRESETS_H
#include <stdint.h>

#define LHDC_V4_QUALITY_PRESET_COUNT 16

typedef struct LhdcV4QualityPreset {
    float min_bits_to_keep;
    int32_t upper_freq;
    float avg_power_shift;
    float noise_floor_shift;
    float noise_floor_dB;
} LhdcV4QualityPreset;

typedef struct LhdcV4QualityPresets {
    LhdcV4QualityPreset pcm16[LHDC_V4_QUALITY_PRESET_COUNT];
    LhdcV4QualityPreset pcm24[LHDC_V4_QUALITY_PRESET_COUNT];
} LhdcV4QualityPresets;

struct FFT_block_s;
#ifdef __cplusplus
extern "C" {
#endif
/* Accept a complete host-validated preset. Copy parameters without recalculating current quality; serialize instance access.
 * Return 0 on success or -1 for NULL. The parameter table may alias the instance's complete table.
 */
int32_t v4f_LossyEncoderSetQualityPresets(struct FFT_block_s *encoder,
                                          const LhdcV4QualityPresets *presets);
/* Copy the instance's complete presets; return 0 on success or -1 for NULL. */
int32_t v4f_LossyEncoderGetQualityPresets(const struct FFT_block_s *encoder,
                                          LhdcV4QualityPresets *presets);
#ifdef __cplusplus
}
#endif
#endif
