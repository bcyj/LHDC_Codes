#ifndef LHDC_DECODER_STATUS_H
#define LHDC_DECODER_STATUS_H

/* Generic status interface: zero is success; these errors belong to the public wrapper.
 * Count-returning interfaces use NO_OUTPUT for zero, not successful decoding.
 * Core, V4 frame scanning, LLAC, and FFT plans have distinct status domains; do not interchange codes.
 */
#define LHDC_DEC_OK 0
#define LHDC_DEC_NO_OUTPUT 0
#define LHDC_DEC_NOT_INITIALIZED (-1)
#define LHDC_DEC_INVALID_ARGUMENT (-2)
#define LHDC_DEC_CODEC_ERROR (-3)
#define LHDC_DEC_WORKSPACE_TOO_SMALL (-4)
#define LHDC_DEC_UNSUPPORTED_CODEC (-10)

/* V5 core: -1 for invalid parameters or initialization, -2 for a truncated bitstream.
 * packet_decode maps internal failures to CORE_ERROR; equal numeric values retain distinct semantic names.
 */
#define LHDC_V5_DEC_INVALID_ARGUMENT (-1)
#define LHDC_V5_DEC_CORE_ERROR (-1)
#define LHDC_V5_DEC_INPUT_EXHAUSTED (-2)
#define LHDC_V5_DEC_INVALID_FRAME (-4)
#define LHDC_V5_DEC_INVALID_SYMBOL (-6)
#define LHDC_V5_DEC_ARITH_TAIL_BITS_SHORT 16
#define LHDC_V5_DEC_ARITH_TAIL_BITS_LONG 24

/* V4 frame-scanning status domain. HEADER_REJECTED means continue scanning, not abort the packet.
 * Positive channel-decoding results only indicate that channel could not complete due to amplitude or residual-read failure.
 */
#define LHDC_V4_DEC_HEADER_NEED_INPUT 0
#define LHDC_V4_DEC_HEADER_ACCEPTED 1
#define LHDC_V4_DEC_HEADER_REJECTED 2
#define LHDC_V4_DEC_DISCONTINUITY (-2)
#define LHDC_V4_DEC_CHANNEL_REJECTED 1
#define LHDC_V4_DEC_NONZERO_PADDING (-3)
#define LHDC_V4_DEC_FRAME_INCOMPLETE (-1)
#define LHDC_V4_DEC_FRAME_NOT_FOUND (-2)

/* LLAC initialization/decoding status domain; positive values are also errors and cannot be tested with < 0. */
#define LHDC_LLAC_DEC_BAD_FRAME 2
#define LHDC_LLAC_DEC_INVALID_POINTER 3
#define LHDC_LLAC_DEC_UNSUPPORTED_RATE 4
#define LHDC_LLAC_DEC_UNSUPPORTED_CHANNELS 5
#define LHDC_LLAC_DEC_INVALID_BITRATE 6
#define LHDC_LLAC_DEC_INVALID_BYTE_BUDGET 7
#define LHDC_LLAC_DEC_UNSUPPORTED_DURATION 9
#define LHDC_LLAC_DEC_UNSUPPORTED_HR_MODE 11
#define LHDC_LLAC_DEC_UNKNOWN_EXTENSION 255

/* LLAC FFT-plan binding status domain, separate from LLAC decoding status. */
#define LHDC_LLAC_FFT_INVALID_DIRECTION 1
#define LHDC_LLAC_FFT_INVALID_LENGTH 2
#define LHDC_LLAC_FFT_MISSING_WORKSPACE 3

#endif
