# Module Sizes

English | [Simplified Chinese](zh-CN/module-sizes.md) | [Home](../README.md)

Measurement date: **2026-10-22**. These figures are preserved from the previously validated Release artifacts. The measurement pass read file/section information without recompiling or changing algorithms. Translation does not constitute a new size measurement. Values use uncompressed KiB (1 KiB = 1024 bytes).

## Measurement scope

- Report V4 encoding, V5 encoding, V4 decoding, V5 decoding, AR, and mathematics separately. V4 includes LLAC; encoding includes the respective BT wrapper.
- Shared mathematics, DSP, memory/buffers, and platform support are separate, without repeated attribution to codecs. Static archives exclude external dependencies.
- Static sections precede selective extraction, COMDAT folding, and dead-code elimination; they are not final EXE/DLL/SYS size increments.
- Writable data includes initialized data/BSS, excluding instance workspace, heap, stack, loader-page alignment, and system CRT. Query instance storage through public interfaces.
- PDBs are excluded. NDK archives may include debug data, which section code/data totals exclude.

## C/H source size

Only source/header contents are counted, excluding build files, documents, manifests, and filesystem cluster slack. Tests are separate. Inline headers belong to their source module; compiled inline code belongs to its caller. These are historical pre-translation counts: later header additions and English comments change source bytes without implying algorithm growth.

| Module | C files | H files | Source KiB |
|---|---:|---:|---:|
| V4 encoder | 23 | 29 | 714.08 |
| V5 encoder | 20 | 25 | 444.87 |
| Shared encoder declarations | 0 | 2 | 5.81 |
| V4 decoder | 23 | 25 | 428.68 |
| V5 decoder | 15 | 17 | 190.63 |
| Shared decoder implementation/declarations | 0 | 4 | 3.58 |
| AR effects | 28 | 20 | 8198.38 |
| Mathematics | 8 | 40 | 615.21 |
| LHDC DSP/memory/buffers | 8 | 26 | 122.49 |
| LHDC platform adaptation | 6 | 20 | 39.58 |
| Generic PCM interface | 0 | 2 | 7.22 |
| Self-contained tests | 49 | 3 | 276.29 |
| **Total** | **180** | **213** | **11046.82** |

## Static archive file sizes

Windows uses CMake Release objects/archives. V4/V5 encoder rows use internal per-version archives; public `lhdc::encoder` combines both versions and no separate public per-version targets were added. Android encoding combines its core/BT archives. AR combines both effects variants, shared data, and PCM adaptation.

| Module | Win32 KiB | x64 KiB | ARM32 KiB | ARM64 KiB |
|---|---:|---:|---:|---:|
| V4 encoder | 266.08 | 407.89 | 723.46 | 1055.03 |
| V5 encoder | 122.44 | 173.29 | 446.05 | 637.34 |
| V4 decoder | 123.57 | 170.48 | 376.35 | 561.48 |
| V5 decoder | 72.34 | 108.18 | 233.20 | 340.90 |
| AR effects | 2005.74 | 2107.17 | 2416.22 | 2740.33 |
| Mathematics | 112.07 | 137.85 | 156.53 | 186.98 |
| LHDC DSP | 24.95 | 37.06 | 75.29 | 102.56 |
| LHDC memory/buffers | 8.54 | 12.38 | 31.82 | 42.49 |
| LHDC platform adaptation | 3.06 | 2.95 | No separate archive | No separate archive |

Generic `ar/` is header-only dispatch, counted in callers. Shared decoder helpers are included in decoder archives. Android bindings use headers/system-link settings; no separate archive does not mean no system dependency.

## Static code, read-only, and writable sections

Each cell is **code / read-only data / writable data+BSS**, in KiB. Executable sections count as code. Read-only includes constants and relocated read-only `.data.rel.ro`. Other loader metadata, including unwind tables and dynamic symbols/relocations, is excluded from these three values; discardable COFF debug/link directives are excluded as well.

| Module | Win32 | x64 | ARM32 | ARM64 |
|---|---:|---:|---:|---:|
| V4 encoder | 68.41 / 90.99 / 0.004 | 90.84 / 91.36 / 0.008 | 53.12 / 89.86 / 0 | 69.80 / 90.71 / 0 |
| V5 encoder | 34.00 / 30.82 / 0 | 36.24 / 30.87 / 0 | 26.96 / 30.53 / 0 | 35.62 / 30.73 / 0 |
| V4 decoder | 30.90 / 44.06 / 0.008 | 37.30 / 44.18 / 0.008 | 22.89 / 43.70 / 0 | 31.97 / 44.04 / 0 |
| V5 decoder | 17.16 / 19.61 / 0.004 | 19.23 / 19.79 / 0.004 | 13.87 / 19.41 / 0 | 20.75 / 19.58 / 0 |
| AR effects | 49.68 / 1851.27 / 0.008 | 65.43 / 1851.49 / 0.008 | 35.38 / 1850.78 / 0 | 55.28 / 1851.83 / 0 |
| Mathematics | 11.78 / 70.33 / 0.012 | 14.62 / 70.33 / 0.023 | 7.93 / 69.07 / 0 | 6.63 / 69.57 / 0 |
| LHDC DSP | 6.78 / 10.87 / 0 | 9.65 / 10.88 / 0 | 4.64 / 10.84 / 0 | 6.04 / 10.90 / 0 |
| LHDC memory/buffers | 1.36 / 0.262 / 0 | 1.49 / 0.273 / 0 | 1.13 / 0.262 / 0 | 1.57 / 0.273 / 0 |
| LHDC platform adaptation | 0.006 / 0 / 0 | 0.011 / 0 / 0 | — | — |

## Final Android SO sizes

These NDK release SOs have debug symbols stripped. File size includes dynamic-link metadata, section tables, and alignment, so it differs from summed object sections.

| Artifact | ARM32 KiB | ARM64 KiB | Contents/direct dependencies |
|---|---:|---:|---|
| `liblhdc.so` | 2128.83 | 2173.19 | V4 core, V4 AR, selected common code |
| `liblhdcBT_enc.so` | 10.13 | 12.16 | V4 BT wrapper; also needs liblhdc.so |
| `liblhdcv5.so` | 2035.53 | 2066.58 | V5 core, V5 AR, selected common code |
| `liblhdcv5BT_enc.so` | 8.07 | 10.08 | V5 BT wrapper; also needs liblhdcv5.so |
| `liblhdcdec.so` | 166.07 | 181.27 | V4 decoder and selected common code |
| `liblhdcv5dec.so` | 44.33 | 52.97 | V5 decoder and selected common code |
| `liblhdc_all.so` | 2339.06 | 2431.56 | Combined codecs, AR, BT, common code |

Combined/versioned SOs are alternative deployments; do not sum all of them as one installation. Separate SOs may each contain math code/tables without cross-SO static deduplication. Calculate actual deployment from selected libraries/dependencies.

## Interpreting the figures

Individual modules also require their common dependencies. Encoding-only or decoding-only applications should select component targets/options; final size depends on used entries and link trimming. AR's convolution/HRIR coefficients account for substantial read-only storage, so instruction size alone understates effects-module size.

Measurements use ordinary SDK/Android defaults with fixed encoding FFT, iterative floating FFT, and logging disabled. They do not describe zero-CRT or SYS builds.
