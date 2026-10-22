# Build and Integration

English | [Simplified Chinese](zh-CN/build-and-integration.md) | [Home](../README.md)

Run these commands from the release root. Output goes to `build/` without developer-specific drive letters or external baseline directories. Minimum versions are CMake 3.19 and Python 3.9 for the Python tools.

## Windows CMake

Use a C11-capable MSVC toolchain, Windows SDK, and CMake. An updated VS2019 v142 toolchain or Visual Studio 2022 is recommended.

```powershell
cmake -S . -B build/x64 -G "Visual Studio 16 2019" -A x64 -DLHDC_BUILD_TESTS=ON
cmake --build build/x64 --config Release --parallel 4
cmake -E chdir build/x64 ctest -C Release --output-on-failure

cmake -S . -B build/win32 -G "Visual Studio 16 2019" -A Win32 -DLHDC_BUILD_TESTS=ON
cmake --build build/win32 --config Release --parallel 4
cmake -E chdir build/win32 ctest -C Release --output-on-failure
```

Use separate architecture directories. Do not enable fast-math or change contraction policy for consistency baselines.

### Component selection

| CMake option | Default | Meaning |
|---|---|---|
| `LHDC_BUILD_ENCODER` | `ON` | V4/V5 encoders |
| `LHDC_BUILD_DECODER` | `ON` | V4/V5 decoders |
| `LHDC_BUILD_AR` | `ON` | Generic PCM interface and LHDC effects backend |
| `LHDC_ENCODER_WITH_AR` | Follows AR | Encoder AR integration |
| `LHDC_BUILD_BUNDLE` | `ON` | Encoder/AR shared library when component dependencies permit |
| `LHDC_BUILD_TESTS` | `OFF` | Self-contained functional/numeric tests |
| `LHDC_ENABLE_LOGGING` | `OFF` | Diagnostic output and argument evaluation |
| `LHDC_V5_ENCODER_FIXED_FFT` | `OFF` | Optional fixed FFT in V5 encoding MDCT |
| `LHDC_KISS_FFT_ITERATIVE_STATIC` | `OFF` | Iterative static floating FFT |
| `LHDC_EXTERNAL_ALLOCATOR_ONLY` | `OFF` | Disable default CRT heap; require workspace/allocator |
| `LHDC_DRIVER_RESOURCES` | `OFF` | Combine iterative FFT, explicit allocation, and disabled logs |
| `LHDC_V4_SHIFT_COUNT_BITS` | `AUTO` | V4 shift-count model; explicit `5` or `8` also available |
| `AR_DEFAULT_BACKEND` | `LHDC` | Generic PCM default backend; may be `NONE` |

The driver-resource combination requires Release/`NDEBUG` and rejects conflicting explicit recursive FFT, default allocation, or enabled logging.

Encoder only:

```powershell
cmake -S . -B build/encoder -A x64 -DLHDC_BUILD_DECODER=OFF -DLHDC_BUILD_AR=OFF -DLHDC_ENCODER_WITH_AR=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/encoder --config Release
```

Decoder only:

```powershell
cmake -S . -B build/decoder -A x64 -DLHDC_BUILD_ENCODER=OFF -DLHDC_BUILD_AR=OFF -DLHDC_ENCODER_WITH_AR=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/decoder --config Release
```

AR only:

```powershell
cmake -S . -B build/ar -A x64 -DLHDC_BUILD_ENCODER=OFF -DLHDC_BUILD_DECODER=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/ar --config Release
```

### Parent CMake projects

```cmake
add_subdirectory(path/to/lhdc_codes_release lhdc_build)
target_link_libraries(my_app PRIVATE lhdc::encoder lhdc::decoder audio_ar::ar)
```

Remove unused targets. Targets propagate include paths/dependencies. The root `lhdc_all` shared library contains encoders and AR only; decoders use static `lhdc::decoder`. Do not infer component contents from a shared-library name.

## Native MSBuild

Projects support Win32/x64 and Debug/Release with default v142. Run from a suitable Visual Studio developer terminal:

```powershell
msbuild lhdc_windows.sln /m /p:Configuration=Release /p:Platform=x64
msbuild lhdc_windows.sln /m /p:Configuration=Release /p:Platform=Win32
msbuild lhdc/platform/windows/lhdc_decoders.sln /m /p:Configuration=Release /p:Platform=x64
msbuild lhdc/platform/windows/lhdc_decoders.sln /m /p:Configuration=Release /p:Platform=Win32
```

VS2022 v143 users may add `/p:PlatformToolset=v143`. Output defaults to `build/windows/`; override with `LhdcOutputRoot`, `DecoderEvidenceRoot`, and `AudioMathOutputRoot`.

Optional properties include `LhdcV5EncoderFixedFft=1`, `LhdcKissFftIterativeStatic=1`, and `LhdcDriverResources=1`. Direct static linking must include common math/component dependencies; prefer public CMake targets for dependency management.

## Android NDK

Install the NDK and supply its path explicitly or through `ANDROID_NDK_HOME`. Default API is 21; ABIs are `armeabi-v7a` and `arm64-v8a`.

```powershell
python tools/generate_android_sources.py --check
python tools/build_android.py --ndk <NDK_PATH> --out build/android --abi both
```

The Python tool invokes the supplied `Android.mk` without external test scripts. Options include `--logging`, `--v5-fixed-fft`, `--iterative-fft`, `--external-allocator-only`, `--driver-resources`, and `--no-bundle`.

Direct ndk-build:

```powershell
& <NDK_PATH>/ndk-build.cmd NDK_PROJECT_PATH=null `
  APP_BUILD_SCRIPT="$PWD/lhdc/platform/android/Android.mk" `
  NDK_APPLICATION_MK="$PWD/lhdc/platform/android/Application.mk" `
  NDK_OUT="$PWD/build/android/obj" NDK_LIBS_OUT="$PWD/build/android/libs"
```

Linux/macOS hosts use the NDK's `ndk-build`.

| SO | Contents |
|---|---|
| `liblhdc.so` | V4 encoding core and AR configuration |
| `liblhdcv5.so` | V5 encoding core and AR configuration |
| `liblhdcBT_enc.so` | V4 BT wrapper, depends on `liblhdc.so` |
| `liblhdcv5BT_enc.so` | V5 BT wrapper, depends on `liblhdcv5.so` |
| `liblhdcdec.so` | Instance-based V4 decoding |
| `liblhdcv5dec.so` | Instance-based V5 decoding |
| `liblhdc_all.so` | Python tool's default additional bundle, including codecs and AR |

Direct ndk-build creates six versioned SOs by default; set `LHDC_ANDROID_BUILD_BUNDLE=1` for the bundle. Names do not establish ABI compatibility with other SDKs. Use the supplied public headers, especially explicit-instance decoder workspace interfaces.

## Standalone shared modules

```powershell
cmake -S math -B build/math -A x64 -DAUDIO_MATH_BUILD_TESTS=ON
cmake --build build/math --config Release
cmake -E chdir build/math ctest -C Release --output-on-failure

cmake -S ar -B build/pcm-interface -A x64 -DAR_DEFAULT_BACKEND=NONE -DAUDIO_AR_BUILD_TESTS=ON
cmake --build build/pcm-interface --config Release
cmake -E chdir build/pcm-interface ctest -C Release --output-on-failure
```

## Windows driver hosts

`lhdc/platform/windows/kernel/windows_memory.c` and `include/lhdc_kernel_memory.h` adapt WDK nonpaged pools to `LhdcAllocator`, using `ExAllocatePool2` and targeting Windows 10 2004 or later.

Hosts may select `LHDC_DRIVER_RESOURCES=ON` and explicitly supply allocation callbacks/workspace. This resource option does not define `_KERNEL_MODE`, automatically link kernel libraries, or generate a complete `.sys`.

Win32 allocator callbacks remain `cdecl`; do not change all codec interfaces through global `/Gz`. Memory adaptation follows its WDK convention. The host driver manages FP/SIMD context, threads/IRQL, stack budgets, kernel CRT choices, and actual loading.

## Release validation

This source distribution contains no precompiled binaries; build for the selected platform/options. English comment translation preserves every non-comment C/H token. Historical runtime/quality results are not counted as fresh translation-test runs. Build caches and validation logs under ignored `build/` are not release payload.
