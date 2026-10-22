# LHDC AR Effects Backend

English | [Simplified Chinese](README.zh-CN.md)

Implements upmix, distance gain, reverb, convolution, and binaural positioning with per-instance audio history. The chain processes PCM independently of codecs.

Use `<ar/pcm.h>` for the generic interface, `<lhdc/ar/pcm_backend.h>` to select LHDC, and `<ar/lhdc_pcm.h>` for direct versioned LHDC parameters. Encoder-specific adaptation is in `src/compat/`.

CMake target: `lhdc::ar`.

See [features](../../docs/features.md) and [build/integration](../../docs/build-and-integration.md).
