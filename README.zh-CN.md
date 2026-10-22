# LHDC 代码库

[English](README.md) | 简体中文

本页保留原中文说明。本轮发布版已将源码注释翻译为英文，运算代码保持不变；最新翻译与验证范围见[源码翻译与验证](docs/zh-CN/source-language.md)。

本项目包含 V4/V5 编码器、V4/V5 解码器、AR PCM 效果器，以及可单独使用的公共数学模块。支持 Windows Win32/x64 和 Android ARM32/ARM64 构建，三个音频组件可独立选择或组合使用。

此库本质上是我为我另一个开源A2DP驱动项目所制造的，大量参考了AOSP中相关的蓝牙编码算法以及WillyBilly06的LHDCv5库，在多个耳机中测试通过，由于找不到LHDC v1的耳机，所以不支持v1, 以后会补上。过程中大量使用Gemini和ChatGPT生成代码，文档、注释全部用Gemini和ChatGPT生成和翻译，等我有时间再手动校对。

- [功能说明](docs/zh-CN/features.md)：组件能力、公开接口、格式及使用限制。
- [模块体积统计](docs/zh-CN/module-sizes.md)：V4/V5 编码、解码、AR 和数学库的四 ABI 源码及编译体积。
- [开源引用与许可](THIRD_PARTY_NOTICES.zh-CN.md)：AOSP、KISS FFT 与 LZH 相关实现及参考项目的适用范围、版权和条款。
- [构建与接入](docs/zh-CN/build-and-integration.md)：CMake、MSBuild、Android NDK 和驱动内存适配。
- [发布清单](release_manifest.json)：文件哈希、构建调整及本次验证摘要。

```text
lhdc_codes_release/
├─ CMakeLists.txt             组件组合与测试入口
├─ lhdc_windows.sln           Windows 编码器及 AR 的 MSBuild 入口
├─ lhdc/
│  ├─ encoder/                V4/V5 编码器及跨平台 BT 适配
│  ├─ decoder/                V4/V5 解码器
│  ├─ ar/                     LHDC AR 效果算法及配置接口
│  ├─ common/                 共享内存、缓冲区、变换及专用数值实现
│  └─ platform/               Android、Windows、ARM、x86 适配
├─ math/                      独立公共 C 数学库
├─ ar/                        通用 PCM 效果后端接口
├─ cmake/                     构建和资源选项
├─ tests/integration/         自包含的组件及实例测试
├─ tools/                     Android 构建及源码清单检查工具
└─ docs/                      当前功能与构建说明
```

在本目录打开终端，可先构建 Windows x64：

```powershell
cmake -S . -B build/x64 -G "Visual Studio 16 2019" -A x64 -DLHDC_BUILD_TESTS=ON
cmake --build build/x64 --config Release --parallel 4
cmake -E chdir build/x64 ctest -C Release --output-on-failure
```

使用 Visual Studio 2022 时改用其对应的生成器。默认启用三个 LHDC 组件；算法默认选项与维护版相同。发布目录不携带封存库、原始音频、历史分析报告、开发者环境设置或编译缓存。

库源码和头文件保持维护版内容；发布时仅调整构建路径、Android NDK 查找方式和说明文档。现有版权及第三方说明随相关实现保留。

## 商标与专利声明 / Trademarks & Patent Disclaimer

1. **商标声明 (Trademark Notice)**：
   - “LHDC”、“LLAC”及相关图形标识为 Savitech Corp.（盛微先进科技股份有限公司）的注册商标或商标。
   - 本项目提及上述商标仅用于客观描述所实现或兼容的音频编解码技术规格与协议标准，并不代表本项目由 Savitech 官方赞助、背书、授权或存在任何关联关系。

2. **专利与知识产权免责声明 (Patent & IP Disclaimer)**：
   - 本代码库是开发者依据公开参考资料、开源实现及协议规范独立改写的实现，供学习、研究和兼容性驱动开发参考使用。
   - 本项目的开源代码许可仅适用于本仓库中具有原创性的代码版权，**不包含、不默示且不转让任何涵盖 LHDC / LLAC 相关技术的专利许可（Patent Grant）或商业运营授权**。
   - 在商业产品（如耳机/音箱固件、移动操作系统、商业音频硬件或软件）中部署、使用或分发 LHDC / LLAC 编解码技术，可能需要从 Savitech Corp. 或相关专利持有方处获得正式的商业许可与认证。
   - 使用方须自行承担相关专利与知识产权合规责任，本项目作者不对任何因使用、修改或分发本代码而产生的知识产权侵权纠纷承担法律责任。

---

*“LHDC” and “LLAC” are registered trademarks or trademarks of Savitech Corp. All product and company names mentioned herein are trademarks™ or registered® trademarks of their respective holders. Use of them does not imply any affiliation with or endorsement by them. This repository provides an independent implementation for research, compatibility, and educational purposes. No patent license or commercial operating authorization for LHDC/LLAC technology is granted or implied by this repository's copyright license. Users deploying this code in commercial applications are solely responsible for obtaining all necessary patent licenses and certifications from Savitech Corp. or respective patent holders.*

