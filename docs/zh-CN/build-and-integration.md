# 构建与接入

[English](../build-and-integration.md) | 简体中文

以下命令均在发布目录根执行。构建输出写入 `build/`，不依赖开发机的盘符或外部封存目录。最低 CMake 版本为 3.19；Python 工具使用 Python 3.9 或更新版本。

## Windows CMake

需要支持 C11 的 MSVC、Windows SDK 及 CMake。Visual Studio 2019 建议使用更新后的 v142 工具链；也可使用 Visual Studio 2022。

```powershell
cmake -S . -B build/x64 -G "Visual Studio 16 2019" -A x64 -DLHDC_BUILD_TESTS=ON
cmake --build build/x64 --config Release --parallel 4
cmake -E chdir build/x64 ctest -C Release --output-on-failure

cmake -S . -B build/win32 -G "Visual Studio 16 2019" -A Win32 -DLHDC_BUILD_TESTS=ON
cmake --build build/win32 --config Release --parallel 4
cmake -E chdir build/win32 ctest -C Release --output-on-failure
```

两种架构使用不同构建目录。不要额外启用 fast-math 或改变浮点融合策略来构建一致性基线。

### 按组件选择

| CMake 选项 | 默认 | 含义 |
|---|---|---|
| `LHDC_BUILD_ENCODER` | `ON` | V4/V5 编码器 |
| `LHDC_BUILD_DECODER` | `ON` | V4/V5 解码器 |
| `LHDC_BUILD_AR` | `ON` | 通用 PCM 接口及 LHDC 效果后端 |
| `LHDC_ENCODER_WITH_AR` | 跟随 AR | 编码器中的 AR 集成 |
| `LHDC_BUILD_BUNDLE` | `ON` | 满足组件依赖时生成编码器与 AR 共享库 |
| `LHDC_BUILD_TESTS` | `OFF` | 自包含功能和数值测试 |
| `LHDC_ENABLE_LOGGING` | `OFF` | 调试打印及参数求值 |
| `LHDC_V5_ENCODER_FIXED_FFT` | `OFF` | V5 编码 MDCT 的可选定点 FFT |
| `LHDC_KISS_FFT_ITERATIVE_STATIC` | `OFF` | 非递归静态浮点 FFT |
| `LHDC_EXTERNAL_ALLOCATOR_ONLY` | `OFF` | 禁用默认 CRT 堆，要求显式工作区或分配器 |
| `LHDC_DRIVER_RESOURCES` | `OFF` | 选择非递归 FFT、显式分配及关闭日志的组合 |
| `LHDC_V4_SHIFT_COUNT_BITS` | `AUTO` | V4 位移计数模型；也可显式选择 `5` 或 `8` |
| `AR_DEFAULT_BACKEND` | `LHDC` | 通用 PCM 接口默认后端；可选择 `NONE` |

资源组合要求 Release／`NDEBUG`；不能同时显式选择递归 FFT、默认分配器或开启日志。

仅构建编码器：

```powershell
cmake -S . -B build/encoder -A x64 -DLHDC_BUILD_DECODER=OFF -DLHDC_BUILD_AR=OFF -DLHDC_ENCODER_WITH_AR=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/encoder --config Release
```

仅构建解码器：

```powershell
cmake -S . -B build/decoder -A x64 -DLHDC_BUILD_ENCODER=OFF -DLHDC_BUILD_AR=OFF -DLHDC_ENCODER_WITH_AR=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/decoder --config Release
```

仅构建 AR：

```powershell
cmake -S . -B build/ar -A x64 -DLHDC_BUILD_ENCODER=OFF -DLHDC_BUILD_DECODER=OFF -DLHDC_BUILD_BUNDLE=OFF
cmake --build build/ar --config Release
```

### 接入上层 CMake 工程

```cmake
add_subdirectory(path/to/lhdc_codes_release lhdc_build)
target_link_libraries(my_app PRIVATE lhdc::encoder lhdc::decoder audio_ar::ar)
```

按实际使用的组件删除不需要的链接目标。依赖库和头文件路径由目标传递。根构建的 `lhdc_all` 共享库只包含编码器及 AR，解码器使用 `lhdc::decoder` 静态目标；不要据共享库名称推断其包含全部组件。

## 原生 MSBuild

项目支持 Win32/x64、Debug/Release，默认工具集为 v142。使用相应 Visual Studio 开发者终端：

```powershell
msbuild lhdc_windows.sln /m /p:Configuration=Release /p:Platform=x64
msbuild lhdc_windows.sln /m /p:Configuration=Release /p:Platform=Win32
msbuild lhdc/platform/windows/lhdc_decoders.sln /m /p:Configuration=Release /p:Platform=x64
msbuild lhdc/platform/windows/lhdc_decoders.sln /m /p:Configuration=Release /p:Platform=Win32
```

如使用 VS2022 的 v143 工具集，可附加 `/p:PlatformToolset=v143`。输出默认位于本目录的 `build/windows/`，可通过 `LhdcOutputRoot`、`DecoderEvidenceRoot` 和 `AudioMathOutputRoot` 属性改写。

MSBuild 可选属性：`LhdcV5EncoderFixedFft=1`、`LhdcKissFftIterativeStatic=1`、`LhdcDriverResources=1`。直接链接静态库时须一并链接其公共数学和组件依赖；优先使用 CMake 的公开目标管理依赖。

## Android NDK

安装 NDK，使用显式路径或环境变量 `ANDROID_NDK_HOME`。默认 API 为 21，默认 ABI 为 `armeabi-v7a` 和 `arm64-v8a`。

```powershell
python tools/generate_android_sources.py --check
python tools/build_android.py --ndk <NDK目录> --out build/android --abi both
```

Python 工具调用随附 `Android.mk`，不依赖外部测试脚本。可选参数包括 `--logging`、`--v5-fixed-fft`、`--iterative-fft`、`--external-allocator-only`、`--driver-resources` 和 `--no-bundle`。

也可直接调用 ndk-build：

```powershell
& <NDK目录>/ndk-build.cmd NDK_PROJECT_PATH=null `
  APP_BUILD_SCRIPT="$PWD/lhdc/platform/android/Android.mk" `
  NDK_APPLICATION_MK="$PWD/lhdc/platform/android/Application.mk" `
  NDK_OUT="$PWD/build/android/obj" NDK_LIBS_OUT="$PWD/build/android/libs"
```

Linux/macOS 宿主使用 NDK 中的 `ndk-build`。输出库如下：

| SO | 内容 |
|---|---|
| `liblhdc.so` | V4 编码核心与对应 AR 配置 |
| `liblhdcv5.so` | V5 编码核心与对应 AR 配置 |
| `liblhdcBT_enc.so` | V4 BT 包装，依赖 `liblhdc.so` |
| `liblhdcv5BT_enc.so` | V5 BT 包装，依赖 `liblhdcv5.so` |
| `liblhdcdec.so` | V4 实例式解码 |
| `liblhdcv5dec.so` | V5 实例式解码 |
| `liblhdc_all.so` | Python 工具默认额外生成的组合库，包含编解码与 AR |

直接 ndk-build 默认只生成六个分版 SO；设置 `LHDC_ANDROID_BUILD_BUNDLE=1` 可生成组合库。SO 名称不代表与其他 SDK 具有相同 ABI；调用方必须使用本目录公开头文件，尤其解码接口需要显式实例工作区。

## 独立公共模块

```powershell
cmake -S math -B build/math -A x64 -DAUDIO_MATH_BUILD_TESTS=ON
cmake --build build/math --config Release
cmake -E chdir build/math ctest -C Release --output-on-failure

cmake -S ar -B build/pcm-interface -A x64 -DAR_DEFAULT_BACKEND=NONE -DAUDIO_AR_BUILD_TESTS=ON
cmake --build build/pcm-interface --config Release
cmake -E chdir build/pcm-interface ctest -C Release --output-on-failure
```

## Windows 驱动宿主接入

`lhdc/platform/windows/kernel/windows_memory.c` 与其 `include/lhdc_kernel_memory.h` 提供 WDK nonpaged pool 到 `LhdcAllocator` 的适配，使用 `ExAllocatePool2`，要求 Windows 10 2004 或更新目标。

宿主可选择 `LHDC_DRIVER_RESOURCES=ON`，并显式提供分配回调或工作区。该选项是资源配置，不自动定义 `_KERNEL_MODE`，不自动链接内核库，也不生成完整 `.sys`。

Win32 allocator 回调保持 `cdecl`；不要用全局 `/Gz` 改变所有 codec 接口。内存适配源使用其 WDK 约定。浮点/SIMD 上下文保存、线程及 IRQL、栈预算、内核 CRT 选择和实际驱动加载由宿主驱动工程负责。

## 发布验证范围

发布清单记录与维护版逐文件一致的 C/H 源码，以及发布目录专用的构建路径调整。本次构建和自包含测试摘要见 `release_manifest.json`；它不把历史音质评估或历史实机回归重复计为本次运行。

源码快照不含预编译二进制；按目标平台和选项构建。验证过程的对象文件、可执行程序和日志位于发布目录之外，不属于发布内容。
