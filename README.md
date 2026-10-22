# LHDC Codec Library

English | [Simplified Chinese](README.zh-CN.md)

This library contains V4/V5 encoders, V4/V5 decoders, an AR PCM effects processor, and a standalone shared mathematics module. It supports Windows Win32/x64 and Android ARM32/ARM64. The three audio components can be built individually or combined.

I developed this library for my other open-source A2DP driver project, drawing extensively on AOSP Bluetooth codec implementations and WillyBilly06's LHDCv5 library. It has been tested with multiple headphones. LHDC v1 is currently unsupported because I could not find a compatible headset; support may be added later. Gemini and ChatGPT were used extensively to generate code. All documentation and comments were generated and translated with Gemini and ChatGPT; I plan to proofread them manually when time permits.

- [Features](docs/features.md): capabilities, public interfaces, formats, and limitations.
- [V2–V5 and LLAC comparison](docs/codec-version-comparison.md): version mappings, algorithms, formats, bitrate control, and compatibility.
- [Module sizes](docs/module-sizes.md): source and compiled sizes for V4/V5 encoding, decoding, AR, and mathematics across four ABIs.
- [Third-party notices](THIRD_PARTY_NOTICES.md): reference scope, copyrights, and terms for AOSP, KISS FFT, and LZH-related projects.
- [Build and integration](docs/build-and-integration.md): CMake, MSBuild, Android NDK, and driver memory integration.
- [Chinese documentation](docs/zh-CN/README.md).
- [Translation and validation](docs/source-language.md): translation scope and release verification.

```text
lhdc_codes_release/
├─ CMakeLists.txt             Component selection and tests
├─ lhdc_windows.sln           Windows encoder and AR MSBuild entry
├─ lhdc/
│  ├─ encoder/                V4/V5 encoders and cross-platform BT wrappers
│  ├─ decoder/                V4/V5 decoders
│  ├─ ar/                     LHDC AR algorithms and configuration
│  ├─ common/                 Memory, buffers, transforms, and codec-specific numerics
│  └─ platform/               Android, Windows, ARM, and x86 adaptation
├─ math/                      Standalone shared C mathematics
├─ ar/                        Generic PCM effects backend interface
├─ cmake/                     Build and resource options
├─ tests/integration/         Self-contained component and instance tests
├─ tools/                     Android build and source-list tools
└─ docs/                      English documentation and zh-CN translations
```

To build Windows x64, open a terminal in this directory:

```powershell
cmake -S . -B build/x64 -G "Visual Studio 16 2019" -A x64 -DLHDC_BUILD_TESTS=ON
cmake --build build/x64 --config Release --parallel 4
cmake -E chdir build/x64 ctest -C Release --output-on-failure
```

Select the corresponding generator when using Visual Studio 2022. All three LHDC components are enabled by default, with the same algorithm defaults as the development tree. This distribution excludes archived baselines, raw audio, historical analysis reports, developer environment settings, and build caches.

Release-specific adjustments cover portable build paths, NDK discovery, documentation, and English source comments. Source translation preserves all non-comment C/H tokens. Existing copyrights and third-party notices remain with the relevant implementations.

## Trademarks and patents

1. **Trademark notice**
   - “LHDC”, “LLAC”, and related logos are registered trademarks or trademarks of Savitech Corp.
   - These names identify implemented or compatible audio coding specifications and protocols. Their use does not imply sponsorship, endorsement, authorization, or affiliation with Savitech.
2. **Patent and intellectual-property disclaimer**
   - This is an independently rewritten implementation based on public references, open-source implementations, and protocol specifications, intended for learning, research, and compatibility-driver development.
   - The repository's open-source copyright license applies only to original code covered by that license. It does **not grant, imply, or transfer patent licenses or commercial operating authorization** for LHDC/LLAC technology.
   - Commercial deployment, use, or distribution in headphone/speaker firmware, operating systems, or audio hardware/software may require formal licenses and certification from Savitech or other patent holders.
   - Users are responsible for intellectual-property compliance. The authors disclaim liability for intellectual-property disputes arising from use, modification, or distribution of this code.

LHDC and LLAC are registered trademarks or trademarks of Savitech Corp. Other product and company names belong to their respective holders. No affiliation or endorsement is implied. Users deploying this code commercially are responsible for obtaining applicable patent licenses and certifications.
