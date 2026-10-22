/* Semantic constants for V4 codecs and AR.
 * Frame duration uses 0.1 ms; AR lengths use samples, float elements, or bytes as specified. */
#ifndef LHDC_V4_CONSTANTS_H
#define LHDC_V4_CONSTANTS_H
#include <stdint.h>
#include <audio_math/constants.h>

/* PCM sample rates in Hz and saturated output ranges. */
#define LHDC_V4_SAMPLE_RATE_44100 44100
#define LHDC_V4_SAMPLE_RATE_48000 48000
#define LHDC_V4_SAMPLE_RATE_88200 88200
#define LHDC_V4_SAMPLE_RATE_96000 96000
#define LHDC_V4_PCM_S16_MAX 32767
#define LHDC_V4_PCM_S16_NEGATIVE_MAGNITUDE 32768
#define LHDC_V4_PCM_S24_MAX 8388607
#define LHDC_V4_PCM_S24_NEGATIVE_MAGNITUDE 8388608

/* AR fixed blocks and convolution. FFT length and half-spectrum float count both equal 512 but have different meanings. */
#define LHDC_V4_AR_BLOCK_SAMPLES 256
#define LHDC_V4_AR_BLOCK_BYTES 1024
#define LHDC_V4_AR_FFT_POINTS 512
#define LHDC_V4_AR_HALF_SPECTRUM_FLOATS 512
#define LHDC_V4_AR_MIRROR_IMAG_START 513
#define LHDC_V4_AR_SPECTRUM_FLOATS 1024
#define LHDC_V4_AR_SPECTRUM_BYTES 4096
#define LHDC_V4_AR_CONV_PARTITIONS 32
#define LHDC_V4_AR_PARTITION_DATA_FLOATS 513
#define LHDC_V4_AR_UPCONV_SLOT_COUNT 16
#define LHDC_V4_AR_HRIR_SLOT_COUNT 8
#define LHDC_V4_AR_HRIR_DATA_BYTES 0x4c00
#define LHDC_V4_AR_ABIR_COEFFICIENT_FLOATS 32768
#define LHDC_V4_AR_HRIR_COEFFICIENT_FLOATS 276480
#define LHDC_V4_AR_PARAMETER_BYTES 456

/* Angle reduction: gyroscope angles use 0.1 degrees; trigonometric functions use single-precision degrees. */
#define LHDC_V4_AR_GYRO_FULL_TURN 3600
#define LHDC_V4_DEGREES_FULL_TURN 360.0f
#define LHDC_V4_DEGREES_HALF_TURN 180.0f
#define LHDC_V4_DEGREES_QUARTER_TURN 90.0f
#define LHDC_V4_DEGREES_OCTANT 45.0f
#define LHDC_V4_DEGREES_THREE_OCTANTS 135.0f
#define LHDC_V4_DEGREES_TO_RADIANS AUDIO_MATH_RADIANS_PER_DEGREE_F32

/* No bytes were read from or written to the ring buffer. */
#define LHDC_V4_CIRBUF_NO_TRANSFER_BYTES 0

/* 16-bit PCM. */
#define LHDC_V4_PCM_BITS_S16 16

/* 24-bit PCM. */
#define LHDC_V4_PCM_BITS_S24 24

/* 32-bit PCM container. */
#define LHDC_V4_PCM_BITS_S32 32

/* Operation succeeded; for status-returning interfaces only. */
#define LHDC_V4_STATUS_OK 0

/* Generic interface failure; the function's checks identify the cause. */
#define LHDC_V4_STATUS_ERROR (-1)

/* No encoded bytes produced: input may be pending, or the interface may signal failure. */
#define LHDC_V4_NO_OUTPUT_BYTES 0

/* No valid sample count obtained. */
#define LHDC_V4_NO_SAMPLES 0

/* No valid bitrate obtained. */
#define LHDC_V4_NO_BITRATE 0

#define LHDC_V4_SUPPORTED 1

/* Unsupported configuration. */
#define LHDC_V4_UNSUPPORTED 0

/* Invalid AR parameters or workspace. */
#define LHDC_V4_AR_INVALID_ARGUMENT (-1)

/* AR initialization channel count does not meet module requirements. */
#define LHDC_V4_AR_INIT_INVALID_CHANNELS (-2)

/* Unsupported AR initialization bit depth. */
#define LHDC_V4_AR_INIT_INVALID_BIT_DEPTH (-3)

/* Unsupported AR initialization sample rate. */
#define LHDC_V4_AR_INIT_INVALID_SAMPLE_RATE (-4)

/* Upmix input/output sample count is not positive. */
#define LHDC_V4_AR_INVALID_SAMPLE_COUNT (-5)

/* Spatial positioning module is not initialized. */
#define LHDC_V4_AR_NOT_INITIALIZED (-6)

/* Upmix library workspace query, initialization, or processing failed. */
#define LHDC_V4_AR_UPMIX_LIBRARY_ERROR (-6)

/* AR channel-position index out of range. */
#define LHDC_V4_AR_INVALID_CHANNEL_INDEX (-2)

/* AR azimuth index out of range. */
#define LHDC_V4_AR_INVALID_AZIMUTH_INDEX (-4)

/* AR elevation index out of range. */
#define LHDC_V4_AR_INVALID_ELEVATION_INDEX (-5)

/* Null AR API workspace. */
#define LHDC_V4_AR_API_NULL_WORKSPACE (-1)

/* Insufficient AR API workspace bytes. */
#define LHDC_V4_AR_API_WORKSPACE_TOO_SMALL (-2)

/* Null AR API parameter pointer. */
#define LHDC_V4_AR_API_NULL_PARAMS (-3)

/* Insufficient AR parameter-block bytes. */
#define LHDC_V4_AR_API_PARAMS_TOO_SMALL (-4)

/* AR API resources are not bound. */
#define LHDC_V4_AR_API_RESOURCES_NOT_ACQUIRED (-5)

/* AR API parameter application failed. */
#define LHDC_V4_AR_API_PARAMS_APPLY_FAILED (-6)

/* AR API upmix initialization failed. */
#define LHDC_V4_AR_API_INIT_UPMIX_FAILED (-4)

/* AR API distance-effect initialization failed. */
#define LHDC_V4_AR_API_INIT_DISTANCE_FAILED (-5)

/* AR API positioning initialization or default-parameter setup failed. */
#define LHDC_V4_AR_API_INIT_LOCALIZATION_FAILED (-6)

/* AR API reverb initialization failed. */
#define LHDC_V4_AR_API_INIT_REVERB_FAILED (-7)

/* AR API overall parameter binding failed. */
#define LHDC_V4_AR_API_INIT_PARAMS_FAILED (-8)

/* AR API positioning gyroscope update failed. */
#define LHDC_V4_AR_API_GYRO_LOCALIZATION_FAILED (-3)

/* AR API reverb gyroscope update failed. */
#define LHDC_V4_AR_API_GYRO_REVERB_FAILED (-4)

/* Null top-level AR workspace. */
#define LHDC_V4_AR_WRAPPER_NULL_WORKSPACE (-1)

/* Null top-level AR parameter pointer. */
#define LHDC_V4_AR_WRAPPER_NULL_PARAMS (-2)

/* Top-level AR resource acquisition failed. */
#define LHDC_V4_AR_WRAPPER_RESOURCE_ERROR (-2)

/* Top-level AR processing, initialization, or parameter setup failed. */
#define LHDC_V4_AR_WRAPPER_OPERATION_FAILED (-3)

/* Top-level AR gyroscope update failed. */
#define LHDC_V4_AR_WRAPPER_GYRO_FAILED (-2)

/* Null upmix library workspace. */
#define LHDC_V4_UPMIX_NULL_WORKSPACE (-3)

#define LHDC_V4_UPMIX_INVALID_WORKSPACE_SIZE (-1)

/* Null upmix library input channel. */
#define LHDC_V4_UPMIX_NULL_INPUT (-5)

/* Null required upmix library output channel. */
#define LHDC_V4_UPMIX_NULL_OUTPUT (-6)

/* Upmix library shelving-filter coefficient query failed. */
#define LHDC_V4_UPMIX_SHELVING_FILTER_ERROR (-7)

/* Upmix library crossover filtering failed. */
#define LHDC_V4_UPMIX_FILTER_BANK_ERROR (-9)

/* Upmix workspace parameter slots are not bound. */
#define LHDC_V4_UPMIX_NULL_PARAMETERS (-10)

/* Compensation-filter coefficient query failed. */
#define LHDC_V4_UPMIX_COEFFICIENT_ERROR (-11)

/* Upmix workspace guard region was overwritten. */
#define LHDC_V4_UPMIX_GUARD_CORRUPTED (-100)

/* Filter does not support this sample rate. */
#define LHDC_V4_FILTER_UNSUPPORTED_SAMPLE_RATE (-1)

/* Null filter output-coefficient pointer. */
#define LHDC_V4_FILTER_NULL_COEFFICIENT_OUTPUT (-2)

/* Invalid filter band index or sample rate. */
#define LHDC_V4_FILTER_INVALID_SELECTION (-3)

/* LLAC sample rate in Hz. */
#define LHDC_V4_SAMPLE_RATE_8000 8000

/* LLAC sample rate in Hz. */
#define LHDC_V4_SAMPLE_RATE_16000 16000

/* LLAC sample rate in Hz. */
#define LHDC_V4_SAMPLE_RATE_24000 24000

/* LLAC sample rate in Hz. */
#define LHDC_V4_SAMPLE_RATE_32000 32000

/* Maximum samples in the LHDC encoding block/prediction workspace. */
#define LHDC_V4_MAX_BLOCK_SAMPLES 512

/* Maximum input samples for LHDC spectral analysis. */
#define LHDC_V4_ANALYSIS_MAX_SAMPLES 256

/* Initial LHDC real-FFT length. */
#define LHDC_V4_ANALYSIS_FFT_MAX_POINTS 512

/* Encoding block sample count for V3/V4 wrappers. */
#define LHDC_V4_FRAME_SAMPLES_V3_V4 256
#define LHDC_V4_LPC_TUKEY_TAPER_SAMPLES 64
#define LHDC_V4_PCM_FRAME_MAX_BYTES (LHDC_V4_FRAME_SAMPLES_V3_V4 * 2 * 3)

/* Encoding block sample count for the compatibility frame format. */
#define LHDC_V4_FRAME_SAMPLES_LEGACY 512

/* PCM input ring-buffer capacity in bytes. */
#define LHDC_V4_INPUT_FIFO_BYTES 32768

/* Wrapper's stack encoding scratch-buffer size in bytes. */
#define LHDC_V4_PACKET_SCRATCH_BYTES 1024
#define LHDC_V4_BT_MAX_MTU_BYTES 4095

/* Default receiver-buffer bytes for flow-control initialization. */
#define LHDC_V4_DEFAULT_SINK_BUFFER_BYTES 10240

#define LHDC_V4_STREAM_VERSION_CODE_200 200

#define LHDC_V4_STREAM_VERSION_CODE_300 300

#define LHDC_V4_STREAM_VERSION_CODE_400 400

/* Maximum LLAC frame/spectrum workspace elements. */
#define LHDC_V4_LLAC_MAX_FRAME_SAMPLES 960
#define LHDC_V4_LLAC_TNS_MAX_ORDER 8

/* Capacity of the two-frame LLAC MDCT scratch array. */
#define LHDC_V4_LLAC_MDCT_TEMP_SAMPLES 1920

/* Internal LLAC frame duration in 0.1 ms units. */
#define LHDC_V4_FRAME_DURATION_2P5MS 25

/* Internal LLAC frame duration in 0.1 ms units. */
#define LHDC_V4_FRAME_DURATION_5MS 50

/* Internal LLAC frame duration in 0.1 ms units. */
#define LHDC_V4_FRAME_DURATION_10MS 100

/* Cost sentinel when no valid encoding scheme is found. */
#define LHDC_V4_ENCODING_COST_UNAVAILABLE 0x7fffffff

/* LPC coefficient quantization succeeded. */
#define LHDC_V4_LPC_QUANT_OK 0

/* LPC quantization shift is below -16. */
#define LHDC_V4_LPC_QUANT_SHIFT_TOO_SMALL 1

/* No positive maximum coefficient magnitude found, including the current NaN-ignoring behavior. */
#define LHDC_V4_LPC_QUANT_NO_NONZERO_COEFFICIENT 2

/* Estimated bit count returned on LPC quantization failure; does not repair uninitialized residual totals. */
#define LHDC_V4_LPC_EVALUATION_FAILED_BITS 0

/* Fixed completion marker for Rice partition estimation. */
#define LHDC_V4_RICE_PARTITION_EVALUATED 1

/* Bitrate selection is constrained by mean-square signal energy. */
#define LHDC_V4_BTR_LIMITED_BY_RMS 1

/* PCM quantization verification mismatch. */
#define LHDC_V4_VERIFY_MISMATCH 0

/* PCM quantization verification match. */
#define LHDC_V4_VERIFY_MATCH 1

/* LLAC FFT dispatch does not support this length. */
#define LHDC_V4_LLAC_FFT_UNSUPPORTED_LENGTH 0

/* LLAC FFT dispatch performed the transform. */
#define LHDC_V4_LLAC_FFT_TRANSFORMED 1

/* Internal encoder handle is not ready. */
#define LHDC_V4_ENCODER_NOT_READY (-3)

/* Internal handle is not ready for the LLAC frame-length query. */
#define LHDC_V4_FRAME_QUERY_ENCODER_NOT_READY (-2)

/* No LZ match distance is available. */
#define LHDC_V4_LZ_NO_MATCH 0

/* Residual selector detected a constant signal; not a byte count or success code. */
#define LHDC_V4_RESIDUAL_CONSTANT_SIGNAL 0

/* Invalid negative LPC error maps to an unselectable cost sentinel. */
#define LHDC_V4_LPC_INVALID_ERROR_COST 1e32f

/* Leading-zero count of a zero 32-bit value. */
#define LHDC_V4_CLZ_ZERO_RESULT 32u

/* First threshold bit pattern used when ABR lookup is out of range. */
#define LHDC_V4_ABR_FALLBACK_INDEX 0

#define LHDC_V4_LLAC_FFT_WORKSPACE_POINTS 1024

/* Radix of the combined SNS codebook index, shared by encoding and decoding quotient/remainder operations. */
#define LHDC_V4_LLAC_SNS_MPVQ_RADIX 2390004u

/* Highest escape level required for normal-mode negative full scale and the HR amplitude limit. */
#define LHDC_V4_LLAC_MAX_ESCAPE_LEVEL_NORMAL 14u
#define LHDC_V4_LLAC_MAX_ESCAPE_LEVEL_HR 21u

/* Exact numeric constants; preserve literal types and values and express casts explicitly at use sites. */
#define LHDC_V4_LLAC_QUANT_ENERGY_FLOOR 1e-5f
#define LHDC_V4_LLAC_ATTACK_ENERGY_FLOOR 1.0e-5f
#define LHDC_V4_LLAC_QUANT_LIMIT_HR 8388606.0f
#define LHDC_V4_LLAC_QUANT_LIMIT_NORMAL 32767.625f
#define LHDC_V4_LLAC_QUANT_BIT_BUDGET_SCALE 0x1.f5c28f5c28f5bp+0
#define LHDC_V4_LLAC_SNS_INITIAL_DISTANCE 0x1p100f
#define LHDC_V4_STATISTICS_COUNT_BIAS 1e-6
#define LHDC_V4_RESAMPLE_OUTPUT_GAIN 1.1614486f
#define LHDC_V4_RESAMPLE_CUTOFF_48K 0.49f
#define LHDC_V4_RESAMPLE_CUTOFF_24K 0.245f
#define LHDC_V4_UPSAMPLE_MAX_F32 4194303.0f
#define LHDC_V4_UPSAMPLE_MAGNITUDE_F32 4194304.0f

/* Maximum PVQ vector dimension and safe pulse count for squaring an int32 element. */
#define LHDC_V4_LLAC_PVQ_MAX_DIMENSIONS 16
#define LHDC_V4_LLAC_PVQ_MAX_PULSES 32767

#endif
