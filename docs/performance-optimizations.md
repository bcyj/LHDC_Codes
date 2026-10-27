# Performance Optimizations

English | [Simplified Chinese](zh-CN/performance-optimizations.md) | [Home](../README.md)

This release incorporates the production-code changes from development commits
`a43e0a24cd4bcb57186307444d9b252f19d7ef1b` through `5d1b90c`. The work targets hot paths in
V4/V5 encoding, decoding, AR processing, and the shared mathematics layer while keeping the
default codec path byte-exact.

## Default bit-exact path

- The floating FFT has a dedicated non-recursive 512-point implementation, build-time twiddle
  tables, radix-two/four SSE2 butterflies on supported x86/x64 targets, and scalar radix-three/five
  butterflies. MSVC Win32 keeps generic mixed-radix work scalar where vector register pressure was
  measured to regress performance; its fixed 512-point kernel still uses the exact SSE2 path.
- The fixed-point FFT uses precomputed leaf permutations and a fused first-stage radix-five
  butterfly. V5 synthesis and optional fixed-FFT encoding share the same integer contracts.
- LLAC/V4 hot paths reduce avoidable copies and loop overhead in MDCT, LPC, LZH/Rice handling,
  bitstream writing, quantization, and spectral analysis.
- V5 hot paths reduce repeated work in frame encoding, MDCT, frequency reconstruction, gain
  scaling, and synthesis.
- AR convolution reuses loaded complex samples, handles packed Nyquist bins outside inner loops,
  and shares an explicit FFT-backend contract.
- Read-only tables remain shared. Temporary arrays reuse existing instance workspaces where
  possible; the changes do not add kilobyte-scale local stack buffers.

The exact SSE2 acceleration is enabled by default through `AUDIO_MATH_FFT_BITEXACT_SSE2=ON`.
Disabling it selects the scalar reference path and is useful for diagnosis, not normally for speed.

## Optional non-bit-exact paths

Two independent options are deliberately disabled by default:

| Option | Scope | Effect |
|---|---|---|
| `LHDC_ENCODER_HIGH_PERFORMANCE_SSE` | Encoder only | Enables faster reciprocal approximations in selected LLAC quantization paths; encoded bytes can differ. |
| `LHDC_AR_HIGH_PERFORMANCE_SSE` | AR only | Enables the split-radix SSE backend for power-of-two AR transforms; PCM can differ by floating-point rounding. |

AR has its own option because its floating-point effect output does not require the codec's encoded
bitstream identity. Enabling either option does not implicitly enable the other. Decoder behavior is
not changed by these high-performance switches.

The underlying standalone math option `AUDIO_MATH_FFT_SPLIT_RADIX_SSE` is also off by default. The
root build enables it when `LHDC_AR_HIGH_PERFORMANCE_SSE=ON`.

## Validation policy

Default-path changes are accepted only after encoded streams, decoded PCM, and AR PCM match the
appropriate frozen baseline for each ABI. Performance measurements use paired workloads and are
treated as directional rather than portable promises. Optional non-bit-exact paths require separate
quality and stability evaluation by the integrating application.

The development campaign covered Win32, x64, Android ARM32, and Android ARM64. After
source/comment synchronization, the release tree passed all 48 CTest cases on both Win32 and x64,
and produced all seven configured shared libraries for both Android ARM32 and ARM64. An additional
x64 build with both optional high-performance switches enabled also passed all 48 CTest cases. See
[Build and Integration](build-and-integration.md) for reproducible commands and the current
release-validation scope.
