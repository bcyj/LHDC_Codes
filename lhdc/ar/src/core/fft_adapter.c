#include "lhdc_version.h"
#include "include/ar_internal.h"
#include <audio_math/fft_float32.h>
#include <string.h>

/* Interleave previous and current frames into complex FFT input and advance frame history. */
void LHDC_FN(fft_buffer_n_to_4n)(const float *new_frame, float *prev_frame, float *new_data,
                                 uint32_t frame_size, uint32_t partition_size) {
    lhdc_ulong_t new_frame_offset;
    size_t frame_bytes;
    int32_t sample_offset;
    lhdc_ulong_t i;
    const float *new_sample;
    float *previous_sample;

    if (prev_frame != (float *)0x0) {
        if (frame_size == 0) {
            frame_bytes = 0;
        } else {
            i = 0;
            new_sample = new_frame;
            previous_sample = prev_frame;
            do {
                new_frame_offset = (partition_size >> 1) + i;
                new_data[i & 0xffffffff] = *previous_sample;
                new_data[(int32_t)i + 1] = 0.0;
                i = i + 2;
                new_data[new_frame_offset & 0xffffffff] = *new_sample;
                new_data[(int32_t)new_frame_offset + 1] = 0.0;
                new_sample = new_sample + 1;
                previous_sample = previous_sample + 1;
            } while ((lhdc_ulong_t)frame_size * 2 - i != 0);
            frame_bytes = (lhdc_ulong_t)frame_size << 2;
        }
        memcpy(prev_frame, new_frame, frame_bytes);
        return;
    }
    if (frame_size == 0) {
        return;
    }
    i = 0;
    do {
        sample_offset = (int32_t)i;
        new_frame_offset = (partition_size >> 1) + i;
        new_data[i & 0xffffffff] = *new_frame;
        i = i + 2;
        new_data[sample_offset + 1] = 0.0;
        new_data[new_frame_offset & 0xffffffff] = 0.0;
        new_data[(int32_t)new_frame_offset + 1] = 0.0;
        new_frame = new_frame + 1;
    } while ((lhdc_ulong_t)frame_size * 2 - i != 0);
    return;
}
