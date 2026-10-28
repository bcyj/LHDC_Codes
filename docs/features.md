# Features

English | [Simplified Chinese](zh-CN/features.md) | [Home](../README.md)

This document describes the distributed sources. Public headers define the interfaces. Codec versions, instance handles, and workspaces are not interchangeable.

## Components

| Component | Capabilities | CMake target | Public headers |
|---|---|---|---|
| Encoder | V4 LHDC/LLAC and V5 PCM encoding, framing, bitrate, and MTU configuration | `lhdc::encoder` | `<lhdc/encoder/lhdc_bt.h>` |
| Decoder | V4 LHDC/LLAC and V5 lossy parsing, spectral reconstruction, synthesis, and PCM output | `lhdc::decoder` | `<lhdc/decoder/decoder.h>` |
| Generic PCM interface | Format description, workspace binding, backend dispatch, parameter forwarding | `audio_ar::ar` | `<ar/pcm.h>`, `<ar/backend.h>` |
| LHDC AR | Upmix, distance gain, reverb, HRIR convolution, binaural positioning | `lhdc::ar` | `<ar/lhdc_pcm.h>`, `<lhdc/ar/pcm_backend.h>` |
| Shared mathematics | Floating FFT, FMA, classification, conversion, rounding, scaling, software math | `audio_common::math` | `<audio_math/...>` |
| Shared core | Allocation-free ring buffer | `audio_common::core` | `<audio_common/ring_buffer.h>` |
| LHDC infrastructure | Instance memory, compatibility ring-buffer adapter, fixed FFT, DCT-IV, protocol tables | `lhdc::common`, `lhdc::dsp` | Dependencies propagate through component targets |

Encoding, decoding, and AR can be combined as needed. AR processes PCM before encoding, after decoding, or in a standalone effects chain.

## Encoders

### V4 and LLAC

V4 includes LHDC and low-latency LLAC, with creation, initialization, encoding, bitrate adjustment, and destruction interfaces.

- BT wrappers accept 44.1/48/96 kHz; the selected core validates specific combinations.
- Select a version through `lhdcBT_get_handle()` and query frame input samples with `lhdcBT_get_block_Size()`.
- `lhdcBT_encodeV3()` reports output bytes/frames. `lhdcBT_encode()` is a single-frame LHDC call and cannot encode LLAC.
- Quality levels, maximum bitrate, queue feedback, MTU, and scheduling intervals are supported.
- Query/modify ordinary LHDC quality presets per instance through `<lhdc/encoder/v4/quality_presets.h>`.

### V5

V5 BT wrappers accept 44.1/48/96/192 kHz and 16/24/32-bit PCM and initialize 5 ms frames. A 32-bit input container does not imply 32-bit effective audio precision.

- `lhdcv5BT_get_handle()` creates an instance; `lhdcv5BT_init_encoder()` configures format, quality, MTU, and interval.
- `lhdcv5BT_encode()` takes input/output capacities and reports bytes/frames.
- Fixed quality, bitrate bounds, queue-feedback ABR, and META/AR extension parameters are available.
- `LHDC_V5_ENCODER_FIXED_FFT=ON` enables an optional fixed FFT within MDCT; default is off. Window functions, pre/post rotations, and the remaining encoder still use floating point. This is not an entirely fixed-point encoder.
- `LHDC_BT_USE_AOSP_REFERENCE` retains alternate BT policies; regular release builds set it to `0`. Complete lossless VBR flow control cannot be inferred from that alternate branch as a default capability.

Both BT wrappers are cross-platform encoding adapters, not OS Bluetooth transmit services. The host sends encoded data and supplies queue feedback.

### PCM and frame input

Encoding uses interleaved stereo PCM. Follow each interface's byte-container, input-block, and output-capacity requirements. Query frame samples from the current instance instead of assuming one size for all rates. Metadata/extensions use header-defined packet layouts.

## Decoders

Public decoder interfaces are instance-based, without global singletons.

| Operation | Main entries |
|---|---|
| V4 memory query/checked initialization | `lhdc_util_dec_get_mem_req()`, `lhdc_util_init_decoder_in_memory()` |
| V4 frame information/decoding/destruction | `lhdcFetchFrameInfo()`, `lhdcDecodeProcess()`, `lhdcDestroy()` |
| V5 memory query/checked initialization | `lhdcv5_util_dec_get_mem_req()`, `lhdcv5_util_init_decoder_in_memory()` |
| V5 frame information/decoding/destruction | `lhdcv5_util_dec_fetch_frame_info()`, `lhdcv5_util_dec_process()`, `lhdcv5_util_dec_destroy()` |

V4 initialization recognizes versions 200/300/400; 500 selects LLAC. V5 uses version 550 and duration `50` for 5 ms. Memory queries and initialization must use matching parameters.

Current V5 synthesis is lossy. `lossless_enable` does not activate a complete separate lossless decoder. Parameter/function names alone do not establish lossless support.

### Output formats

- V4 outputs interleaved stereo: two bytes per 16-bit sample, otherwise four-byte slots in public output paths.
- V5 `audio_fmt=0` is interleaved stereo, `2` mono planar, `3` stereo planar.
- V5 32-bit output left-aligns 24 effective bits.
- AR packed 24-bit PCM differs from decoder 32-bit slots; the host explicitly converts containers.
- Supply complete frames plus required readable prefetch tails. Frame-info parsing does not replace all capacity checks. Size output by current samples, bit depth, and channel layout.

Register log callbacks per instance after initialization and again after reinitialization. Disabled logging does no formatting or callbacks.

## AR PCM effects

Generic `ArPcm` defines only format, workspace, and backend callbacks. The default uses LHDC AR's V5 parameters. Select V4/V5 through `<lhdc/ar/pcm_backend.h>` or provide a custom `ArPcmBackend`.

LHDC backend conventions:

- Little-endian interleaved stereo, 16/24/32 bits; 24-bit samples occupy three consecutive bytes.
- 44.1/48/88.2/96 kHz, at most 256 sample frames per call.
- Caller-owned, 16-byte-aligned workspace sized by query.
- In-place/separate output; successful output bytes equal input bytes.
- Read/write effects parameters and gyroscope orientation in tenths of a degree.
- Initialization/processing do not take workspace ownership.

Build the generic interface alone with `AR_DEFAULT_BACKEND=NONE`; instances explicitly supply backends without linking LHDC algorithms.

## Instance memory and concurrency

Encoders support default user-mode allocation, host callbacks via `*_get_handle_with_allocator()`, or caller storage via `*_get_handle_in_memory()`. Query current capacity with `*_workspace_bytes()`.

Decoders/AR use explicit workspaces. Capacity varies with ABI, options, and implementation; use public queries instead of internal sizes or old constants. Encoder instances and checked decoder initialization require 16-byte base alignment.

`LhdcAllocator` is declared in `<lhdc_memory.h>`. Allocation copies callbacks; `context` and host resources must outlive associated instances. `LhdcMemoryArena` partitions initialization storage sequentially without individual-block reclamation.

Different instances isolate audio state. The host serializes configuration, processing, callback changes, and destruction for each instance. Math tables are read-only/shared; histories/scratch belong to instances.

## Mathematics and transforms

The sibling `audio_codec_common/math/` module is independent of LHDC types, allocators, and logs and can serve other codecs. Floating FFT allocates no memory; callers provide input, output, twiddles, and generic-radix scratch. Recursive and optional iterative static schedules are supported. Exact SSE2 butterflies are enabled by default on supported x86/x64 targets; an optional non-bit-exact split-radix SSE backend remains separate for AR-oriented performance testing.

The sibling `audio_codec_common/core/` module provides the allocation-free public ring buffer. LHDC retains only its compatibility API and delegates storage and index operations to the shared implementation.

Shared LHDC DSP adds DCT-IV/fixed FFT. Numeric helpers follow header-defined domains, rounding, saturation, and modular arithmetic. They are not complete C99 `libm` replacements and do not generally reproduce CRT `errno`, exceptions, or arbitrary rounding modes.

See [Performance Optimizations](performance-optimizations.md) for backend policy, affected hot paths,
and validation requirements.

## Platforms

- Windows: MSVC/MSBuild Win32/x64, SSE2 by default on Win32, strict numeric FP settings.
- Android: NDK ARM32/ARM64, versioned cores/BT wrappers/decoders as SOs, Android logging adaptation.
- Windows drivers: explicit callbacks and WDK nonpaged-pool adaptation. Resource options do not constitute a complete `.sys`; hosts manage FP state, context, stack, and linking.

No Bluetooth stack, audio-file parser, player, CSV loader, or complete driver project is included.

## Module sizes

Historical Release static-object **code plus read-only data**, KiB, before final link trimming. Shared dependencies are separate; instance workspace, stack, and other loader metadata are excluded. V4 includes LLAC.

| Module | Win32 | x64 | ARM32 | ARM64 |
|---|---:|---:|---:|---:|
| V4 encoder | 159.40 | 182.20 | 142.98 | 160.51 |
| V5 encoder | 64.82 | 67.11 | 57.48 | 66.36 |
| V4 decoder | 74.96 | 81.48 | 66.58 | 76.00 |
| V5 decoder | 36.77 | 39.01 | 33.28 | 40.34 |
| AR effects | 1900.95 | 1916.91 | 1886.16 | 1907.11 |
| Mathematics | 82.11 | 84.95 | 77.00 | 76.20 |
| LHDC DSP | 17.65 | 20.52 | 15.48 | 16.94 |
| LHDC memory/buffers | 1.62 | 1.76 | 1.40 | 1.84 |
| LHDC platform | 0.006 | 0.011 | No separate object | No separate object |

Generic PCM dispatch is inline and counted in callers. See [module sizes](module-sizes.md) for measurement scope/date, source/archive sizes, section breakdowns, and Android SO sizes.

## Trademarks and intellectual property

LHDC/LLAC are registered trademarks or trademarks of Savitech Corp. This library supports compatibility-driver development, interoperability testing, and research. Open source distribution does not grant or transfer LHDC/LLAC patents. Commercial deployment must follow applicable rights-holder licensing/certification requirements.
