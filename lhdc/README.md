# LHDC Components

English | [Simplified Chinese](README.zh-CN.md)

`encoder/`, `decoder/`, and `ar/` provide encoding, decoding, and PCM effects. `common/` manages shared instance-memory infrastructure, buffers, and transforms; `platform/` provides host adaptation.

Shared mathematics and the allocation-free ring buffer come from the sibling `../../audio_codec_common/` repository; the generic PCM backend contract is in `../ar/`. Version-specific instances/state remain separate, while read-only shared tables have a single definition.

See [features](../docs/features.md) and [build/integration](../docs/build-and-integration.md).
