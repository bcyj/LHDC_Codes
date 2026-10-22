# Generic PCM Effects Interface

English | [Simplified Chinese](README.zh-CN.md)

This directory defines PCM formats, instances, workspace, and `ArPcmBackend` callbacks without codec-specific state or effects tables. Public entries are `<ar/pcm.h>` and `<ar/backend.h>`.

The root CMake build links LHDC AR by default through `audio_ar::ar`. Set `AR_DEFAULT_BACKEND=NONE` to build only the header interface and supply another backend explicitly per instance.

Backend descriptors must remain valid and read-only throughout instance lifetime; mutable state belongs in workspace. The backend defines supported sample rates, channels, and maximum block size.

See [features](../docs/features.md) and [build/integration](../docs/build-and-integration.md).
