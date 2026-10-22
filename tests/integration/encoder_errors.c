#include "../../lhdc/encoder/src/v4/include/encoder_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "encoder error contract line %d: %s\n", __LINE__, #condition);         \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

/* Verify public encoder handling of invalid handles, parameters, and output capacity. */
int main(void) {
    LLAC_UINT8 *memory = v4f_llac_enc_mem_alloc();
    REQUIRE(memory);
    LLAC_INT bytes = 99, bitrate = 99;
    uint32_t samples = 99;
    REQUIRE(v4f_llac_enc_initial(12345, 2, 16, 16, 5, 192000, &bytes, &samples, &bitrate, memory) ==
            LLAC_SAMPLERATE_ERROR);
    REQUIRE(bytes == 0 && samples == 0 && bitrate == 0);
    REQUIRE(v4f_llac_enc_initial(48000, 3, 16, 16, 5, 192000, &bytes, &samples, &bitrate, memory) ==
            LLAC_CHANNELS_ERROR);
    REQUIRE(v4f_llac_enc_initial(48000, 2, 16, 16, NAN, 192000, &bytes, &samples, &bitrate,
                                 memory) == LLAC_FRAMEMS_ERROR);
    REQUIRE(v4f_llac_enc_initial(48000, 2, 16, 16, 5, 0, &bytes, &samples, &bitrate, memory) ==
            LLAC_BITRATE_ERROR);
    REQUIRE(v4f_llac_enc_initial(48000, 2, 16, 16, 5, 192000, &bytes, &samples, &bitrate, memory) ==
            LLAC_OK);
    REQUIRE(samples == 240 && bytes > 0);
    REQUIRE(v4f_llac_enc_set_bitrate(0, &bytes, &bitrate, memory) == LLAC_BITRATE_ERROR);
    REQUIRE(bytes == 0 && bitrate == 0);
    REQUIRE(v4f_llac_enc_set_bitrate(192000, &bytes, &bitrate, memory) == LLAC_OK);
    int32_t pcm[480] = {0};
    uint8_t output[4096];
    REQUIRE(v4f_llac_enc_process(NULL, output, memory) == -LLAC_NULL_ERROR);
    REQUIRE(v4f_llac_enc_process(pcm, output, memory) == bytes);
    LHDCV4_LLACWorkspaceDirectory *directory = (void *)memory;
    *directory->bps_in = 12;
    REQUIRE(v4f_llac_enc_process(pcm, output, memory) == -LLAC_ERROR);
    v4f_llac_enc_mem_free(memory);

    size_t required = 99;
    REQUIRE(!v4f_kiss_fftr_alloc(0, 0, NULL, &required) && required == 0);
    REQUIRE(!v4f_kiss_fftr_alloc(-2, 0, NULL, &required) && required == 0);
    REQUIRE(!v4f_kiss_fftr_alloc(5, 0, NULL, &required) && required == 0);
    kiss_fftr_cfg forward = v4f_kiss_fftr_alloc(8, 0, NULL, NULL);
    kiss_fftr_cfg inverse = v4f_kiss_fftr_alloc(8, 1, NULL, NULL);
    REQUIRE(forward && inverse);
    float real[8] = {0}, saved[8];
    kiss_fft_cpx spectrum[5], saved_spectrum[5];
    memset(real, 0xa5, sizeof real);
    memset(spectrum, 0xa5, sizeof spectrum);
    memcpy(saved, real, sizeof real);
    memcpy(saved_spectrum, spectrum, sizeof spectrum);
    REQUIRE(v4f_kiss_fftr(inverse, real, spectrum) == LHDC_V4_STATUS_ERROR);
    REQUIRE(v4f_kiss_fftri(forward, spectrum, real) == LHDC_V4_STATUS_ERROR);
    REQUIRE(memcmp(saved, real, sizeof real) == 0);
    REQUIRE(memcmp(saved_spectrum, spectrum, sizeof spectrum) == 0);
    REQUIRE(v4f_kiss_fftr(NULL, real, spectrum) == LHDC_V4_STATUS_ERROR);
    memset(real, 0, sizeof real);
    REQUIRE(v4f_kiss_fftr(forward, real, spectrum) == LHDC_V4_STATUS_OK);
    REQUIRE(v4f_kiss_fftri(inverse, spectrum, real) == LHDC_V4_STATUS_OK);
    lhdc_memory_free(forward);
    lhdc_memory_free(inverse);
    puts("PASS encoder recoverable errors and recovery");
    return 0;
}
