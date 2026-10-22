# AOSP / Android LHDC References

English | [Simplified Chinese](aosp-reference.zh-CN.md)

This project references Google Android Open Source Project's V5 Rust encoder and acknowledges its authors/maintainers. Reference groups are recorded separately; older vendor files are not collectively attributed to Google or AOSP.

## Google AOSP V5 encoder

The following were verified on Google's official source server:

- Repository: [platform/packages/modules/Bluetooth](https://android.googlesource.com/platform/packages/modules/Bluetooth/).
- Tag: [android-vts-17.0_r1](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1), pointing to commit `c77db469de80f86660bc053bec4dee0c5d4b947c`.
- Module: [system/audio/codecs/lhdcv5](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5).
- [Android.bp](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5/Android.bp) explicitly declares `default_applicable_licenses: ["Android-Apache-2.0"]`.
- [src/lhdcv5BT_enc.c](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5/src/lhdcv5BT_enc.c) retains `Copyright (C) 2025 The Android Open Source Project` and the Apache-2.0 header.

| Reference scope | Related implementation | Use/modifications |
|---|---|---|
| Rust V5 flow, numeric types, and fixed transforms | V5 encoding code including `lhdc/encoder/src/v5/codec/mdct.c` | Flow analysis and optional fixed FFT reference; the main encoder retains floating operations and is not a literal translation of the complete Rust encoder |
| Rust `src/kiss_fft.rs` | `lhdc/common/src/dsp/fft_fixed32.c` and headers | Fixed butterflies, stage scaling, and numeric semantics, rewritten in C with iterative scheduling and instance-workspace adaptation |

The full [Apache-2.0 license](Apache-2.0.txt) is bundled. Related references/adaptations retain applicable copyrights/notices; the C interfaces, layouts, workspaces, and organization have changed. Distribution of covered derivative portions must supply the license, retain applicable notices, and identify changes in modified files. This explanation does not replace license terms.

`src/kiss_fft.rs` separately retains Mark Borgerding's three-clause BSD header, reproduced in [AOSP-Rust-KISS-FFT-BSD-3-Clause.txt](AOSP-Rust-KISS-FFT-BSD-3-Clause.txt). The module's default Apache-2.0 declaration does not replace that third-party notice. See also the [KISS FFT notice](../THIRD_PARTY_NOTICES.md#kiss-fft).

This document and third-party notices accompany the source release. They record references/licenses without changing interfaces or algorithms.
