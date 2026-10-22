# Third-Party Notices

English | [Simplified Chinese](THIRD_PARTY_NOTICES.zh-CN.md)

Reviewed on 2026-10-22. These notices distinguish Android/LHDC interface references, references used in FFT implementations, and algorithm references used during LZH evaluation. They do not replace the project's overall license or assign all codec code to an upstream license.

| Scope in this project | Upstream | Use | Upstream license |
|---|---|---|---|
| V5 Rust encoding flow and fixed transforms | Google AOSP Bluetooth `system/audio/codecs/lhdcv5` | Official module matches the local Rust reference tree after line-ending normalization; used for analysis and C rewriting | Module defaults to Apache-2.0; `kiss_fft.rs` carries its own three-clause BSD notice |
| Butterflies and mixed-radix organization in common floating/fixed FFT | KISS FFT, Mark Borgerding and contributors | Implementation reference and adaptation, with upstream copyright/license retained | BSD-3-Clause |
| Evaluation of V4 LZH dictionary/matching organization | LHa for UNIX | Algorithm-structure reference, not included as a product source component | Custom upstream license |
| Evaluation of V4 LZH decoding boundaries and overlapping back-references | libarchive LHA reader | Algorithm-structure reference, not included as a product source component | Referenced file uses BSD-2-Clause |

## AOSP / Android LHDC references

Google's official [V5 encoding module](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5) explicitly selects Apache-2.0 in `Android.bp`; its BT wrapper retains `Copyright (C) 2025 The Android Open Source Project`. The complete local Rust source reference matches that official tag after line-ending normalization. This distribution includes the [Apache-2.0 text](LICENSES/Apache-2.0.txt) and the [Rust FFT's separate BSD notice](LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt).

## KISS FFT

Upstream: [mborgerding/kissfft](https://github.com/mborgerding/kissfft). Its [COPYING](https://github.com/mborgerding/kissfft/blob/master/COPYING) records Mark Borgerding's copyright and BSD-3-Clause identifier; terms appear in [LICENSES/BSD-3-Clause](https://github.com/mborgerding/kissfft/blob/master/LICENSES/BSD-3-Clause).

```text
Copyright (c) 2003-2010 Mark Borgerding. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause
```

Related implementation locations:

- `math/src/fft_float32.c`, `math/src/fft_float32.h`: single-precision complex butterflies, factorization, and scheduling.
- `math/include/audio_math/fft_float32.h`: common floating FFT plans/interfaces.
- `lhdc/common/src/dsp/fft_fixed32.c`, `fft_fixed32.h`: fixed butterflies, stage scaling, and integer rounding helpers.
- `lhdc/common/include/dsp/fft_fixed32.h`: common fixed FFT types/interfaces.

These implementations use caller/instance workspaces, shared read-only twiddles, optional iterative scheduling, and specified fixed-point scaling, rounding, and modular arithmetic. They are modified implementations rather than unchanged upstream files. Attribution covers related floating and fixed portions; numeric-type changes, renaming, or refactoring do not remove upstream notices.

The full copyright/license is included in [LICENSES/KISS-FFT-BSD-3-Clause.txt](LICENSES/KISS-FFT-BSD-3-Clause.txt). Copyright information comes from upstream COPYING; the BSD-3-Clause text uses that actual copyright in place of its placeholder.

For covered portions, source redistribution must retain copyright, conditions, and disclaimer; binary redistribution must reproduce them in accompanying documentation/materials. Upstream author/contributor names may not be used for endorsement without permission. The bundled English license is authoritative; explanatory translations do not replace it.

This identifies related implementations/reference scope. It does not assert that every mixed-source file, generated table entry, or the entire LHDC codec is licensed by KISS FFT's BSD license.

## LZH: LHDC byte stream and algorithm references

`lhdc/encoder/src/v4/codec/lzh_encode.c` produces an LHDC byte stream alternating literal and length/distance back-reference blocks. Its reader is `lhdc/decoder/src/v4/codec/lzh_decode.c`. It does not implement standard LHA file headers or LH4–LH7 Huffman streams.

The current encoder uses instance position tables, multiplicative hashing, batched literals, bounded wide-word matching, and sparse dictionary updates within matches. Implementation/evaluation records identify the following projects as references for organization and boundary handling; their files were not copied as product translation units. These are algorithm-reference acknowledgments/license records, not a designation of the entire LZH module as a source derivative of LHa or libarchive.

### LHa for UNIX

Reference project: [jca02266/lha](https://github.com/jca02266/lha). Referenced files:

- [src/slide.c](https://github.com/jca02266/lha/blob/master/src/slide.c): sliding dictionaries, hash-chain searches, match selection, dictionary updates.
- [src/huf.c](https://github.com/jca02266/lha/blob/master/src/huf.c): standard LHA Huffman organization and its differences from this stream format.

Relevant upstream contributors include Nobutaka Watazaki and T. Okamoto; see upstream for full credits. The custom license's authoritative text is the [DISTRIBUTION section of man/lha.n](https://github.com/jca02266/lha/blob/master/man/lha.n). It must not be labeled BSD, MIT, or unrestricted public domain. That section includes conditions on distribution materials, binary-only distribution, and commercial use.

The unmodified original-language section is retained in [LICENSES/LHa-for-UNIX-reference.txt](LICENSES/LHa-for-UNIX-reference.txt). This reference copy describes upstream terms and does not relicense the current LHDC LZ implementation. Its Japanese text is intentionally unchanged.

### libarchive LHA reader

Referenced file: [archive_read_support_format_lha.c](https://github.com/libarchive/libarchive/blob/master/libarchive/archive_read_support_format_lha.c), for decoder state, input/output bounds, and overlapping back-reference copying.

Its header states `Copyright (c) 2008-2014 Michihiro NAKAJIMA` and uses two-clause BSD. The copyright/terms are included in [LICENSES/libarchive-LHA-BSD-2-Clause.txt](LICENSES/libarchive-LHA-BSD-2-Clause.txt). This records that specific reader, not a uniform license for all libarchive files.

No libarchive library, LHA reader translation unit, or Huffman table is bundled. If upstream implementations are copied later, update scope and notices for those files.

## Included license files

- [LICENSES/aosp-reference.md](LICENSES/aosp-reference.md): Android/LHDC reference scope and verified license status.
- [LICENSES/Apache-2.0.txt](LICENSES/Apache-2.0.txt): full Apache-2.0 text for the Google AOSP V5 encoding reference.
- [LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt](LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt): the Rust FFT's own BSD notice, verified against the official tree.
- [LICENSES/KISS-FFT-BSD-3-Clause.txt](LICENSES/KISS-FFT-BSD-3-Clause.txt): KISS FFT copyright/full three-clause BSD terms.
- [LICENSES/LHa-for-UNIX-reference.txt](LICENSES/LHa-for-UNIX-reference.txt): custom LHa terms, recorded for the algorithm-reference project only.
- [LICENSES/libarchive-LHA-BSD-2-Clause.txt](LICENSES/libarchive-LHA-BSD-2-Clause.txt): referenced LHA reader copyright/two-clause BSD terms.

Other math copyright/license notices remain in [math/platform/windows/NOTICE.math.txt](math/platform/windows/NOTICE.math.txt). These notices document references/licensing without changing algorithms or build options.

## Savitech trademarks and patents

- **Trademarks:** LHDC and LLAC are registered trademarks or trademarks of Savitech Corp. Names identify compatible audio formats/specifications only, without official endorsement, sponsorship, or affiliation.
- **Patents and authorization:** Open-source distribution covers code copyright only and **does not grant or imply patent licenses or commercial operating authorization** for LHDC/LLAC technology. Commercial integration/hardware deployment remains responsible for formal authorization and certification from Savitech or other rights holders.
