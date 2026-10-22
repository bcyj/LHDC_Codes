# LHDC Components

English | [Simplified Chinese](README.zh-CN.md)

`encoder/`, `decoder/`, and `ar/` provide encoding, decoding, and PCM effects. `common/` manages shared instance-memory infrastructure, buffers, and transforms; `platform/` provides host adaptation.

The standalone mathematics module is in `../math/`; the generic PCM backend contract is in `../ar/`. Version-specific instances/state remain separate, while read-only shared tables have a single definition.

See [features](../docs/features.md) and [build/integration](../docs/build-and-integration.md).
