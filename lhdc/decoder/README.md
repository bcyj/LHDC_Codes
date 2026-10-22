# Decoders

English | [Simplified Chinese](README.zh-CN.md)

Public headers are in `include/lhdc/decoder/`, with umbrella `<lhdc/decoder/decoder.h>`. `src/api/v4/` and `src/api/v5/` manage initialization, decoding, controls, and logging. Algorithms are in `src/v4/` and `src/v5/`.

V4 includes LHDC, LLAC, and upsampling. V5 includes lossy parsing, fixed FFT, and synthesis. Hosts supply workspace sized for the current ABI through public queries.

The combined CMake alias is `lhdc::decoder`. Versioned static targets are `lhdc_decoder_v4` and `lhdc_decoder_v5`; the combined target is `lhdc_decoder`. These names do not restrict target processors.

See [features](../../docs/features.md) and [build/integration](../../docs/build-and-integration.md).
