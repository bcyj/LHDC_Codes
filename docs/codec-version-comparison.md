# LHDC V2–V5 and LLAC Comparison

English | [Simplified Chinese](zh-CN/codec-version-comparison.md) | [Documentation](README.md)

Reviewed against the release sources on 2026-10-22. This compares the implementations and public interfaces in this library, not every capability advertised for products using these codec names. The default BT implementation is selected with `LHDC_BT_USE_AOSP_REFERENCE=0`.

## Overview

V2, V3, and V4 share a predictive PCM/residual-coding family. V3 adds adaptive LPC and shorter blocks; V4 retains that core and adds extension signaling. V5 uses a separate MDCT-based spectral codec. LLAC is also a transform codec, but has its own shaping, side information, entropy coding, and decoder; it is not ordinary V4 with a different quality setting or an interchangeable V5 stream.

The source directory `lhdc/encoder/src/v4/` includes **V2, V3, V4, and LLAC**. Its name does not mean all contained streams use the V4 format.

## Version numbers and entry points

| Stream | Encoder creation | Decoder initialization version | Implementation family |
|---|---|---:|---|
| LHDC V2 | `lhdcBT_get_handle(1)` | `200` | V4 component, predictive core |
| LHDC V3 | `lhdcBT_get_handle(2)` | `300` | V4 component, predictive core |
| LHDC V4 | `lhdcBT_get_handle(3)` | `400` | V4 component, predictive core |
| LLAC | `lhdcBT_get_handle(4)` | `500` | V4 component, separate LLAC core |
| LHDC V5 | `lhdcv5BT_get_handle(1, &handle)` | `550` | Separate V5 component |

Encoder creation versions and decoder protocol versions are different numbering domains. Old BT parameter `4` selects LLAC, not ordinary LHDC V4. Decoder version `500` selects LLAC, not V5; V5 uses a separate API. The V5 creation value `1` identifies a revision of the V5 API/core, not LHDC V1.

## Algorithm comparison

| Property | V2 | V3 | V4 | LLAC | V5 |
|---|---|---|---|---|---|
| Main representation | Quantized PCM and prediction residuals | Same family | Same family | MDCT spectrum | MDCT spectrum |
| Prediction/shaping | Fixed-order differences and stereo difference selection | Adds adaptive quantized LPC | Retains V3 prediction | SNS and TNS | SNS/ADSQ scale-factor organization |
| Main payload coding | Rice or LHDC-specific LZ, with raw/constant alternatives | Same family, plus LPC parameters | Same family, plus extensions | Context arithmetic coding of spectral pairs, escape/sign/residual bits | Predictive bit planes, FAC arithmetic coding, signs/remainders |
| FFT role in encoding | Spectral analysis for quantization/quality control | Same | Same | Part of the MDCT/DCT transform | Part of the MDCT/DCT transform |
| Core reconstruction | Integer prediction and residual reconstruction | Adds LPC reconstruction | Same core with extension parsing | Spectrum reconstruction, noise fill, inverse TNS/SNS, IMDCT | Spectral reconstruction/scaling, noise fill, fixed FFT and synthesis |

In V2–V4, finding an FFT implementation does **not** make the encoded representation an MDCT spectrum. Frequency analysis chooses PCM quantization strength, after which the encoder predicts and compresses samples/residuals. V5 and LLAC instead quantize and encode transform coefficients.

The LZ stream in V2–V4 alternates literal and length/distance blocks; it is not an LHA archive or a standard LH4–LH7 Huffman stream. 

## PCM formats and frame sizes

| Property | V2 | V3 | V4 | LLAC | V5 |
|---|---|---|---|---|---|
| BT encoder sample rates | 44.1/48/96 kHz | Same | Same | Wrapper accepts the same rates; core/mode validation still applies | 44.1/48/96/192 kHz |
| BT encoder input depth | 16/24 bits | Same | Same | 16/24 bits through BT | 16/24/32-bit containers |
| Encoding block | 512 samples/channel | 256 samples/channel | 256 samples/channel | BT initializes nominal 5 ms frames | BT initializes 5 ms frames |
| Input duration at 48 kHz | About 10.67 ms | About 5.33 ms | About 5.33 ms | 5 ms, 240 samples | 5 ms, 240 samples |
| Public checked decoder rate limitation | V4 API accepts 44.1/48/96 kHz | Same | Same | Checked binding excludes 96 kHz; use 44.1/48 kHz | Separate V5 initialization, including high-rate configurations |

Important distinctions:

- LLAC's internal encoding APIs include 32-bit containers and 2.5/5/10 ms configuration paths. The BT wrapper restricts input to 16/24 bits and selects 5 ms. Internal branches do not expand the BT contract automatically.
- LLAC encoding can initialize a 96 kHz high-resolution mode, while this library's checked public LLAC decoder rejects 96 kHz. Do not describe that combination as supported end-to-end decoding.
- V5 32-bit encoding input uses a container from which effective 24-bit data is extracted. It does not establish 32-bit lossless fidelity.
- Query frame sample counts and memory from the active instance. Do not derive every sample count simply by multiplying a nominal duration by the input rate, especially at 44.1 kHz.
- Frame input duration is not Bluetooth end-to-end latency. Packet grouping, scheduling, host queues, radio transport, and receiver buffering are additional factors.


## Bitrates and rate control

| Item | V2–V4 LHDC | LLAC | V5 |
|---|---|---|---|
| Regular highest fixed BT level | 900 kbps | The wrapper exposes levels up to 900 kbps, subject to core rate/frame/mode checks | 1000 kbps |
| AUTO selector | Index `8` | Index `8` | Index `9` |
| Default AUTO starting target | 400 kbps | 400 kbps | 400 kbps |
| Control | Queue statistics, quality limits, frame/MTU budgets | Separate LLAC queue/budget path | Separate queue control with sample-rate-specific tables and bitrate bounds |

The V4-family shared table contains `64,128,192,256,320,400,500,900,1000` kbps. Index eight is AUTO, so its `1000` entry is not a regular fixed 1 Mbps level. Ordinary LHDC also applies a 128 kbps lower clamp, or 320 kbps when its minimum-limit flag is active; raw table entries alone do not describe effective output settings.

LLAC also retains stateful initialization behavior: on alternating initialization phases, targets of 900/600 kbps can update extension-mode state, while 64 kbps can update its automatic-bitrate flag according to input depth. These values are not solely bitrate selectors in that path. Repeated configuration should follow a deliberate instance lifecycle rather than assuming initialization is stateless.

V5's default table has fixed indices 0..8 and AUTO at nine. At 44.1 kHz, the intermediate 256/500 kbps entries become 240/480 kbps. A `2000` table entry at the AUTO index is not proof of an available fixed 2 Mbps mode. Instance bitrate limits and custom tables further affect the selected target. Packet byte rounding can make actual bitrate differ from the requested value.

The alternate AOSP-reference BT branch contains VBR/lossless-control logic. That branch does not establish a complete lossless codec in the default release. The current V5 decoder is a lossy implementation; a lossless-related argument is not evidence of an independent working lossless chain.


## Changes between versions

### V2 to V3

V3 halves the input block size and adds adaptive LPC as a candidate alongside fixed difference prediction. The encoder compares residual costs before selecting a method. It also changes packet/frame budgeting and has version-specific stereo-difference behavior. This is more than a header-number change, but remains the same predictive codec family.

### V3 to V4

The main prediction, quantization, and residual coding remain largely shared. V4 adds explicit extension-header behavior, AR/JAS signaling, and META payloads. Header writing exits before this extension logic for version codes at or below 399. The default wrapper creates optional AR state for ordinary V4 and LLAC, not for V2/V3.

AR is a PCM effects processor rather than the residual codec itself. V2/V3 can still be preceded by the separate PCM effects interface if the host explicitly processes audio, but this is not their built-in BT AR integration.

### V4 to V5

V5 changes the payload representation to transform-domain coding and has independent headers, band tables, scale factors, entropy state, and decoder synthesis. It adds 192 kHz input and a regular 1 Mbps level. This is not an in-place upgrade obtained by changing a V4 version field.

### LLAC versus V4 and V5

LLAC is a separate low-delay transform path hosted by the V4 component. Unlike ordinary V4 it encodes MDCT coefficients with SNS/TNS and spectral-pair arithmetic contexts. Unlike V5 it has different side information, codebooks, gain/residual organization, and framing. Shared FFT/DCT infrastructure does not make their streams interchangeable.

## Numeric paths, memory, and performance

- V2–V4 use floating spectral/LPC analysis alongside integer quantization and residual reconstruction.
- LLAC uses floating MDCT/SNS/TNS with integer quantization/arithmetic coding; its decoder uses the common floating DCT-IV path.
- V5 encoding remains primarily floating point. `LHDC_V5_ENCODER_FIXED_FFT=ON` replaces the FFT portion only; windows and rotations remain floating point. V5 decoding uses the common fixed-point FFT within its synthesis chain.
- V3's shorter blocks do not prove lower CPU cost than V2, because LPC adds analysis work. Similarly, V4 extensions or a V5 label alone do not determine speed or quality.
- Instance capacities depend on ABI, sample rate, frame length, AR inclusion, and FFT backend. Use public workspace queries. The [module-size report]groups V2–V4/LLAC into a V4 component; it is not a separate per-protocol size benchmark.

## Compatibility and integration

1. Keep stream negotiation separate from API version numbers. Use the matching codec API and decoder initialization code.
2. V2–V4 share a decoder implementation but not identical prediction/header semantics. LLAC and V5 have separate bitstreams and cannot be decoded by exchanging version identifiers.
3. Distinguish PCM bit depth from its byte container. Encoder packed PCM and decoder 32-bit output slots may need explicit conversion;
4. AR is independent PCM processing. Apply it explicitly where needed, without treating an extension flag as proof of equivalent processing on every version.
5. Shared math, FFT, memory, and ring-buffer implementations do not imply shared mutable codec state. Use separate handles/workspaces, and serialize access to each instance.
