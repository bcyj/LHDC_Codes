#include "lhdc_memory.h"
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_ENCODER_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_ENCODER_H
#include "../types/lhdc_base_types.h"
#include "../../../../include/lhdc/encoder/v4/quality_presets.h"

typedef struct FFT_block_s FFT_block_s;

typedef struct FFT_block_s FFT_BLOCK;

typedef enum lhdc_ext_func_t {
    EXT_FUNC_AR = 0,
    EXT_FUNC_LARC = 1,
    EXT_FUNC_EXT_FLG = 2,
    EXT_FUNC_JAS = 3,
    EXT_FUNC_META = 4
} lhdc_ext_func_t;

FFT_BLOCK *v4f_LossyEncoderNew_with_allocator(int32_t version, const LhdcAllocator *allocator);

int32_t v4f_LossyEncoderDelete(FFT_BLOCK *fb);

void v4f_LossyEncoderInit(FFT_BLOCK *fb, int32_t sample_rate, int32_t bits_per_sample,
                          int32_t channels, int32_t block_size, int32_t sink_buf_len,
                          int32_t target_byte_rate, int32_t fast_mode,
                          int32_t one_frame_per_channel, int32_t need_min_byte_rate,
                          int32_t process_size, int32_t target_mtu_byte,
                          int32_t no_btr_limit_frame_cnt, int32_t min_bits_reserved,
                          int32_t lhdc_ver);

int32_t v4f_LossyEncoderProcessWav(FFT_BLOCK *fb, uint8_t *wav, int32_t ns, int32_t final,
                                   uint8_t *out, int32_t out_len);

void v4f_LossyEncoderSetTargetByteRate(FFT_BLOCK *fb, int32_t target_byte_rate);

void v4f_LossyEncoderUpdateFrameSize(FFT_BLOCK *fb, int32_t target_mtu_byte,
                                     int32_t target_byte_rate);

void v4f_LhdcExtFuncArEnable(FFT_BLOCK *fb, int32_t enable_ar);

void v4f_LhdcExtFuncJasEnable(FFT_BLOCK *fb, int32_t enable_Jas);

void v4f_LhdcExtFuncMetaEnable(FFT_BLOCK *fb, int32_t enable_meta, uint8_t *pmeta_data,
                               int32_t meta_data_len, int32_t conti_frame_cnt);

bool v4f_LhdcGetExtFuncState(FFT_BLOCK *fb, lhdc_ext_func_t ext_func);

#endif /* LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_ENCODER_H */
