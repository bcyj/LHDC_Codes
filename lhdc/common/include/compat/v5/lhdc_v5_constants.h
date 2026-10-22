/* Semantic constants for V5 codecs and AR.
 * Frame duration uses 0.1 ms; AR lengths use samples, float elements, or bytes as specified. */
#ifndef LHDC_V5_CONSTANTS_H
#define LHDC_V5_CONSTANTS_H
#include <stdint.h>
#include <audio_math/constants.h>

/* PCM sample rates in Hz and saturated output ranges. */
#define LHDC_V5_SAMPLE_RATE_44100 44100
#define LHDC_V5_SAMPLE_RATE_48000 48000
#define LHDC_V5_SAMPLE_RATE_88200 88200
#define LHDC_V5_SAMPLE_RATE_96000 96000
#define LHDC_V5_SAMPLE_RATE_192000 192000
#define LHDC_V5_PCM_S16_MAX 32767
#define LHDC_V5_PCM_S16_NEGATIVE_MAGNITUDE 32768
#define LHDC_V5_PCM_S24_MAX 8388607
#define LHDC_V5_PCM_S24_NEGATIVE_MAGNITUDE 8388608

/* Encoding frame duration, bitrate, and gain tables. The internal 2.5 ms branch does not imply public API support. */
#define LHDC_V5_FRAME_DURATION_2P5MS 25
#define LHDC_V5_FRAME_DURATION_5MS 50
#define LHDC_V5_FRAME_DURATION_10MS 100
#define LHDC_V5_FRAME_DURATION_1S 10000
#define LHDC_V5_MAX_BITRATE_BPS 1000000
#define LHDC_V5_GAIN_TABLE_SIZE 512
#define LHDC_V5_GAIN_INDEX_MAX 511
#define LHDC_V5_GAIN_LINEAR_LAST_INDEX 503
#define LHDC_V5_GAIN_EXTRAPOLATION_INDEX 504
#define LHDC_V5_GAIN_MAX_EXPONENT 30.0f
#define LHDC_V5_QUANTIZED_SPECTRUM_LIMIT 1.0737418e9f //(float)(1 << 30)
#define LHDC_V5_ADSQ_TABLE_SIZE 64
#define LHDC_V5_ADSQ_INDEX_MAX 63
#define LHDC_V5_ADSQ_INDEX_SATURATION_THRESHOLD 62
#define LHDC_V5_SNS_MAX_BANDS 32
#define LHDC_V5_SNS_MAX_DIFFERENCES 31

/* Workspace capacities, in elements or bytes as specified by each constant. */
#define LHDC_V5_WORKSPACE_SLOT_COUNT 12
#define LHDC_V5_WORKSPACE_MAX_CHANNELS 8
#define LHDC_V5_MAX_FRAME_SAMPLES 1920
#define LHDC_V5_MDCT_OVERLAP_SAMPLES 384
#define LHDC_V5_CHANNEL_ENCODED_BYTES 688
#define LHDC_V5_TRANSFORM_BUFFER_BYTES 3840
#define LHDC_V5_MDCT_WINDOW_BYTES 1536
#define LHDC_V5_BPC_OUTPUT_BYTES 3840
#define LHDC_V5_BPC_LAST_TRIPLE_START 0xefc
#define LHDC_V5_BPC_LAST_OUTPUT_INDEX 0xeff

/* Frame-header bit fields and extension data. The first two ext_data bytes are not META payload. */
#define LHDC_V5_HEADER_VERSION_WIDTH 2
#define LHDC_V5_HEADER_VERSION_SHIFT 10
#define LHDC_V5_HEADER_JAS_SHIFT 12
#define LHDC_V5_HEADER_AR_SHIFT 13
#define LHDC_V5_HEADER_LARC_SHIFT 14
#define LHDC_V5_HEADER_META_SHIFT 15
#define LHDC_V5_HEADER_LARC_MASK 0x4000
#define LHDC_V5_HEADER_META_MASK 0x8000
#define LHDC_V5_HEADER_WITHOUT_META_MASK 0x7fff
#define LHDC_V5_HEADER_CONTROL_MASK 0xfc00
#define LHDC_V5_HEADER_BASE_BYTES 2
#define LHDC_V5_HEADER_LARC_BYTES 4
#define LHDC_V5_META_OFFSET 2
#define LHDC_V5_META_BYTES 8
#define LHDC_V5_HEADER_EXT_BYTES 10

/* FAC range coding: retain UINT32_C and unsigned suffixes to preserve integer promotion. */
#define LHDC_V5_FAC_PROBABILITY_SCALE UINT32_C(0x80000000)
#define LHDC_V5_FAC_NORMALIZE_THRESHOLD UINT32_C(0x1000000)
#define LHDC_V5_FAC_FLUSH_THRESHOLD UINT32_C(0x2000000)
#define LHDC_V5_FAC_FLUSH_SMALL_INCREMENT UINT32_C(0x800000)
#define LHDC_V5_FAC_FLUSH_LARGE_INCREMENT UINT32_C(0x1000000)
#define LHDC_V5_FAC_FLUSH_SMALL_RANGE UINT32_C(0x8000)
#define LHDC_V5_FAC_FLUSH_LARGE_RANGE UINT32_C(0x800000)
#define LHDC_V5_FAC_PROBABILITY_BITS 15
#define LHDC_V5_FAC_EVAL_SCALE 65536u
#define LHDC_V5_FAC_EVAL_INVERSE_SCALE 0x1p-16f
#define LHDC_V5_FAC_MAX_SYMBOLS 5
#define LHDC_V5_FAC_CUMULATIVE_ENTRIES 6
#define LHDC_V5_FAC_HISTORY_SIZE 128

/* AR fixed blocks and convolution. FFT length and half-spectrum float count both equal 512 but have different meanings. */
#define LHDC_V5_AR_BLOCK_SAMPLES 256
#define LHDC_V5_AR_BLOCK_BYTES 1024
#define LHDC_V5_AR_FFT_POINTS 512
#define LHDC_V5_AR_HALF_SPECTRUM_FLOATS 512
#define LHDC_V5_AR_MIRROR_IMAG_START 513
#define LHDC_V5_AR_SPECTRUM_FLOATS 1024
#define LHDC_V5_AR_SPECTRUM_BYTES 4096
#define LHDC_V5_AR_CONV_PARTITIONS 32
#define LHDC_V5_AR_PARTITION_DATA_FLOATS 513
#define LHDC_V5_AR_UPCONV_SLOT_COUNT 16
#define LHDC_V5_AR_HRIR_SLOT_COUNT 8
#define LHDC_V5_AR_HRIR_DATA_BYTES 0x4c00
#define LHDC_V5_AR_ABIR_COEFFICIENT_FLOATS 32768
#define LHDC_V5_AR_HRIR_COEFFICIENT_FLOATS 276480
#define LHDC_V5_AR_PARAMETER_BYTES 456

/* Angle reduction: gyroscope angles use 0.1 degrees; trigonometric functions use single-precision degrees. */
#define LHDC_V5_AR_GYRO_FULL_TURN 3600
#define LHDC_V5_DEGREES_FULL_TURN 360.0f
#define LHDC_V5_DEGREES_HALF_TURN 180.0f
#define LHDC_V5_DEGREES_QUARTER_TURN 90.0f
#define LHDC_V5_DEGREES_OCTANT 45.0f
#define LHDC_V5_DEGREES_THREE_OCTANTS 135.0f
#define LHDC_V5_DEGREES_TO_RADIANS AUDIO_MATH_RADIANS_PER_DEGREE_F32

/* No bytes were read from or written to the ring buffer. */
#define LHDC_V5_CIRBUF_NO_TRANSFER_BYTES 0

/* 16-bit PCM. */
#define LHDC_V5_PCM_BITS_S16 16

/* 24-bit PCM. */
#define LHDC_V5_PCM_BITS_S24 24

/* 32-bit PCM container. */
#define LHDC_V5_PCM_BITS_S32 32

/* Operation succeeded; for status-returning interfaces only. */
#define LHDC_V5_STATUS_OK 0

/* Generic interface failure; the function's checks identify the cause. */
#define LHDC_V5_STATUS_ERROR (-1)

/* No encoded bytes produced: input may be pending, or the interface may signal failure. */
#define LHDC_V5_NO_OUTPUT_BYTES 0

/* Invalid AR parameters or workspace. */
#define LHDC_V5_AR_INVALID_ARGUMENT (-1)

/* AR initialization channel count does not meet module requirements. */
#define LHDC_V5_AR_INIT_INVALID_CHANNELS (-2)

/* Unsupported AR initialization bit depth. */
#define LHDC_V5_AR_INIT_INVALID_BIT_DEPTH (-3)

/* Unsupported AR initialization sample rate. */
#define LHDC_V5_AR_INIT_INVALID_SAMPLE_RATE (-4)

/* Upmix input/output sample count is not positive. */
#define LHDC_V5_AR_INVALID_SAMPLE_COUNT (-5)

/* Spatial positioning module is not initialized. */
#define LHDC_V5_AR_NOT_INITIALIZED (-6)

/* Upmix library workspace query, initialization, or processing failed. */
#define LHDC_V5_AR_UPMIX_LIBRARY_ERROR (-6)

/* AR channel-position index out of range. */
#define LHDC_V5_AR_INVALID_CHANNEL_INDEX (-2)

/* AR azimuth index out of range. */
#define LHDC_V5_AR_INVALID_AZIMUTH_INDEX (-4)

/* AR elevation index out of range. */
#define LHDC_V5_AR_INVALID_ELEVATION_INDEX (-5)

/* Null AR API workspace. */
#define LHDC_V5_AR_API_NULL_WORKSPACE (-1)

/* Insufficient AR API workspace bytes. */
#define LHDC_V5_AR_API_WORKSPACE_TOO_SMALL (-2)

/* Null AR API parameter pointer. */
#define LHDC_V5_AR_API_NULL_PARAMS (-3)

/* Insufficient AR parameter-block bytes. */
#define LHDC_V5_AR_API_PARAMS_TOO_SMALL (-4)

/* AR API resources are not bound. */
#define LHDC_V5_AR_API_RESOURCES_NOT_ACQUIRED (-5)

/* AR API parameter application failed. */
#define LHDC_V5_AR_API_PARAMS_APPLY_FAILED (-6)

/* AR API upmix initialization failed. */
#define LHDC_V5_AR_API_INIT_UPMIX_FAILED (-4)

/* AR API distance-effect initialization failed. */
#define LHDC_V5_AR_API_INIT_DISTANCE_FAILED (-5)

/* AR API positioning initialization or default-parameter setup failed. */
#define LHDC_V5_AR_API_INIT_LOCALIZATION_FAILED (-6)

/* AR API reverb initialization failed. */
#define LHDC_V5_AR_API_INIT_REVERB_FAILED (-7)

/* AR API overall parameter binding failed. */
#define LHDC_V5_AR_API_INIT_PARAMS_FAILED (-8)

/* AR API positioning gyroscope update failed. */
#define LHDC_V5_AR_API_GYRO_LOCALIZATION_FAILED (-3)

/* AR API reverb gyroscope update failed. */
#define LHDC_V5_AR_API_GYRO_REVERB_FAILED (-4)

/* Null top-level AR workspace. */
#define LHDC_V5_AR_WRAPPER_NULL_WORKSPACE (-1)

/* Null top-level AR parameter pointer. */
#define LHDC_V5_AR_WRAPPER_NULL_PARAMS (-2)

/* Top-level AR resource acquisition failed. */
#define LHDC_V5_AR_WRAPPER_RESOURCE_ERROR (-2)

/* Top-level AR processing, initialization, or parameter setup failed. */
#define LHDC_V5_AR_WRAPPER_OPERATION_FAILED (-3)

/* Top-level AR gyroscope update failed. */
#define LHDC_V5_AR_WRAPPER_GYRO_FAILED (-2)

/* Null upmix library workspace. */
#define LHDC_V5_UPMIX_NULL_WORKSPACE (-3)

#define LHDC_V5_UPMIX_INVALID_WORKSPACE_SIZE (-1)

/* Null upmix library input channel. */
#define LHDC_V5_UPMIX_NULL_INPUT (-5)

/* Null required upmix library output channel. */
#define LHDC_V5_UPMIX_NULL_OUTPUT (-6)

/* Upmix library shelving-filter coefficient query failed. */
#define LHDC_V5_UPMIX_SHELVING_FILTER_ERROR (-7)

/* Upmix library crossover filtering failed. */
#define LHDC_V5_UPMIX_FILTER_BANK_ERROR (-9)

/* Upmix workspace parameter slots are not bound. */
#define LHDC_V5_UPMIX_NULL_PARAMETERS (-10)

/* Compensation-filter coefficient query failed. */
#define LHDC_V5_UPMIX_COEFFICIENT_ERROR (-11)

/* Upmix workspace guard region was overwritten. */
#define LHDC_V5_UPMIX_GUARD_CORRUPTED (-100)

/* Filter does not support this sample rate. */
#define LHDC_V5_FILTER_UNSUPPORTED_SAMPLE_RATE (-1)

/* Null filter output-coefficient pointer. */
#define LHDC_V5_FILTER_NULL_COEFFICIENT_OUTPUT (-2)

/* Invalid filter band index or sample rate. */
#define LHDC_V5_FILTER_INVALID_SELECTION (-3)

/* Bitstream write succeeded. */
#define LHDC_V5_BIT_WRITER_OK 0

/* Bitstream field width exceeds 32 bits. */
#define LHDC_V5_BIT_WRITER_INVALID_WIDTH (-1)

/* Bitstream output is full. */
#define LHDC_V5_BIT_WRITER_BUFFER_FULL (-2)

/* MDCT workspace initialized; the null-workspace query still returns required bytes. */
#define LHDC_V5_MDCT_INITIALIZED 0

/* FAC range-coding operation succeeded. */
#define LHDC_V5_FAC_OK 0

/* FAC output exhausted. */
#define LHDC_V5_FAC_BUFFER_FULL (-1)

/* Exact numeric constants; preserve literal types and values and express casts explicitly at use sites. */
#define LHDC_V5_GAIN_SLOPE_24BIT_48K 0.01704348f
#define LHDC_V5_GAIN_SLOPE_24BIT_96K 0.008608697f
#define LHDC_V5_GAIN_SLOPE_24BIT_192K 0.0022260882f
#define LHDC_V5_GAIN_SLOPE_16BIT_48K 0.017147828f
#define LHDC_V5_GAIN_SLOPE_16BIT_44K 0.016921738f

#endif
