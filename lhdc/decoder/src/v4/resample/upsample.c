#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include <audio_math/bits.h>
#include <audio_math/fma_native.h>
#include <audio_math/float_convert.h>
#include "../include/lhdc_upsample.h"
#include "../include/upsample_history_view.h"
#include <math.h>
#include <string.h>

static const float feedback_a[7] = {

    0.58305f, 1.15013f, 0.87121f, 0.47391f, 0.7082f, 1.03872f, 0.50504f};
static const float feedback_b[7] = {

    0.80582f, 0.35153f, 0.56523f, 0.96556f, 0.69577f, 0.43612f, 0.8931f};
static const float feed_b1[7] = {

    0.025052529f, 0.43113288f, 0.6367769f, 0.0027519623f, 0.06258026f, 0.9142323f, 0.022693053f};
static const float feed_b02[7] = {

    0.11254f, 0.2239f, 0.70629f, 0.34747f, 0.12888f, 0.62836f, 0.30596f};

/* Reset all filter history for the specified channel. */
void ResetDecodeUps2xLPF(lhdc_Decoder_t *state, int nCh) {
    memset(&state->filter_history[nCh], 0, sizeof state->filter_history[nCh]);
}

/* Apply multiplication and fused multiply-add/subtract in order for seven-stage 2x interpolation and update history. */
void DecodeUps2xLPF(lhdc_Decoder_t *state, int nCh, int *X, int *pcm, int N, int bits_per_sample) {
    unsigned times = (unsigned)N * 2;
    if ((int32_t)times <= 0)
        return;
    unsigned shift = (24u - (unsigned)bits_per_sample) & 255;
    uint32_t scale_bits = shift < 32 ? 1u << shift : 0;
    int32_t scale;
    memcpy(&scale, &scale_bits, 4);
    LHDCUpsampleHistory *history = &state->filter_history[nCh];
    for (unsigned i = 0; i < times; ++i) {
        float fX = (i & 1) ? 0.0f : (float)X[i / 2] * (float)scale;
        float old1 = history->stages[0].previous_1, old2 = history->stages[0].previous_2;
        float current = audio_math_fma_f32(-old2, feedback_b[0], audio_math_fma_f32(old1, feedback_a[0], fX));
        history->stages[0].previous_1 = current;
        history->stages[0].previous_2 = old1;
        for (unsigned stage = 1; stage < 7; ++stage) {
            LHDCUpsampleStage *filter = &history->stages[stage];
            float p1 = filter->previous_1, p2 = filter->previous_2;
            float value = old1 * feed_b1[stage - 1];
            value = audio_math_fma_f32(current, feed_b02[stage - 1], value);
            value = audio_math_fma_f32(old2, feed_b02[stage - 1], value);
            value = audio_math_fma_f32(p1, feedback_a[stage], value);
            value = audio_math_fma_f32(-p2, feedback_b[stage], value);
            filter->previous_1 = value;
            filter->previous_2 = p1;
            current = value;
            old1 = p1;
            old2 = p2;
        }
        float value = old1 * feed_b1[6];
        value = audio_math_fma_f32(current, feed_b02[6], value);
        value = audio_math_fma_f32(old2, feed_b02[6], value);
        /* Only finite values inside the clipping interval enter bounded conversion; NaN retains the upper-limit branch. */
        int pcm_sample = !(value < LHDC_V4_UPSAMPLE_MAX_F32)       ? 4194303
                         : value > -LHDC_V4_UPSAMPLE_MAGNITUDE_F32
                             ? audio_math_truncate_i32_f32(value)
                             : -4194304;
        uint32_t result = audio_math_shift_arithmetic_low8_sign32((uint32_t)pcm_sample, shift) << 1;
        memcpy(pcm + i, &result, 4);
    }
}

/* Apply 2x interpolation and maintain per-channel delay buffers; the first block leaves the first 48 output entries untouched. */
void fast_upsampling_2x(lhdc_Decoder_t *state, int *input, int *output, int ns, int dec_ch,
                        int nFirstUpsChunk, int bits_per_sample) {
    int *cache = state->upsample_cache[dec_ch];
    if (!nFirstUpsChunk)
        memcpy(output, cache, sizeof state->upsample_cache[0]);
    else {
        memset(cache, 0, sizeof state->upsample_cache[0]);
        ResetDecodeUps2xLPF(state, dec_ch);
    }
    DecodeUps2xLPF(state, dec_ch, input, output + 48, 8, bits_per_sample);
    if (ns == 32)
        DecodeUps2xLPF(state, dec_ch, input + 8, cache, 24, bits_per_sample);
}

