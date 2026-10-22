# AOSP、KISS FFT 与 LZH 的引用及许可说明

[English](THIRD_PARTY_NOTICES.md) | 简体中文

核对日期：2026-10-22。本说明区分 Android/LHDC 接口参考、实际 FFT 实现中的相关参考，以及 LZH 评估阶段的算法参考；不替代工程整体许可证，也不将其他编解码代码统一声明为某个上游项目的许可。

| 本项目范围 | 上游项目 | 引用性质 | 上游许可 |
|---|---|---|---|
| V5 Rust 编码流程及定点变换参考 | Google AOSP Bluetooth 的 `system/audio/codecs/lhdcv5` | 官方模块与本地 Rust 参考树哈希一致；用于分析及 C 改写参考 | 模块默认 Apache-2.0；`kiss_fft.rs` 自身另附 BSD 三条款 |
| 公共浮点 FFT、公共定点 FFT 的相关蝶形及混合基数组织 | KISS FFT，Mark Borgerding 等 | 实现参考及改写，随包保留上游版权与许可 | BSD-3-Clause |
| V4 LZH 字典及匹配组织的评估 | LHa for UNIX | 算法结构参考，未作为产品源码组件引入 | 上游自定义许可 |
| V4 LZH 解码边界及重叠回溯规则的评估 | libarchive 的 LHA reader | 算法结构参考，未作为产品源码组件引入 | 相关文件为 BSD-2-Clause |

## AOSP / Android LHDC 参考

Google 官方 [V5 编码模块](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/tags/android-vts-17.0_r1/system/audio/codecs/lhdcv5) 的 `Android.bp` 明确指定 Apache-2.0；BT 包装列明 `Copyright (C) 2025 The Android Open Source Project`。本地 Rust 参考的整个源码树在换行统一后与该官方标签一致。随包保留 [Apache-2.0 全文](LICENSES/Apache-2.0.txt) 及 [Rust FFT 的独立 BSD 通知](LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt)。

## KISS FFT

上游项目：[mborgerding/kissfft](https://github.com/mborgerding/kissfft)。上游 [COPYING](https://github.com/mborgerding/kissfft/blob/master/COPYING) 记录 Mark Borgerding 的版权及 BSD-3-Clause 标识，具体条款见其 [LICENSES/BSD-3-Clause](https://github.com/mborgerding/kissfft/blob/master/LICENSES/BSD-3-Clause)。

```text
Copyright (c) 2003-2010 Mark Borgerding. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause
```

本项目相关实现位置：

- `math/src/fft_float32.c`、`math/src/fft_float32.h`：单精度复数 FFT 蝶形、分解和调度。
- `math/include/audio_math/fft_float32.h`：公共浮点 FFT 计划及接口。
- `lhdc/common/src/dsp/fft_fixed32.c`、`fft_fixed32.h`：定点蝶形、逐级缩放和整数舍入辅助。
- `lhdc/common/include/dsp/fft_fixed32.h`：公共定点 FFT 类型与接口。

本项目在相关实现中使用调用方或实例工作区、共享只读旋转表、可选非递归调度，以及指定的定点缩放、舍入和模运算。这些实现并非未经修改的上游文件。引用记录同时覆盖浮点和定点相关部分，不因数值类型、重命名或重构而省略上游许可。

完整版权与许可随包保存于 [LICENSES/KISS-FFT-BSD-3-Clause.txt](LICENSES/KISS-FFT-BSD-3-Clause.txt)。其中版权信息取自上游 COPYING，条款取自上游 BSD-3-Clause 文本，并将其版权占位符填为上述实际版权信息。

对于受该上游许可覆盖的部分，源码分发应保留版权、条件及免责声明；二进制分发应在随附文档或材料中重现这些内容；不得未经许可使用上游作者或贡献者名称作背书。中文说明仅便于阅读，以随附英文条款为准。

这里列出的是相关实现及引用范围，不表示整份混合来源文件、生成的所有表项或整个 LHDC 编解码器均由 KISS FFT 的 BSD 许可授权。

## LZH：LHDC 字节流与上游算法参考

本项目的 `lhdc/encoder/src/v4/codec/lzh_encode.c` 输出字面量块与长度/距离回溯块交替的 LHDC 字节流；对应读取逻辑位于 `lhdc/decoder/src/v4/codec/lzh_decode.c`。它不实现标准 LHA 文件头或 LH4～LH7 的 Huffman 码流。

当前编码实现使用实例位置表、乘法哈希、批量字面量写出、有界宽字匹配，以及匹配区间的稀疏字典更新。现有实现和评估记录将下列项目列为算法组织及边界处理的参考，没有将这些上游文件复制为产品编译单元。因此，这些条目是参考致谢与上游许可记录，不把整个 LZH 模块标为 LHa 或 libarchive 的源码衍生组件。

### LHa for UNIX

参考项目：[jca02266/lha](https://github.com/jca02266/lha)。参考文件：

- [src/slide.c](https://github.com/jca02266/lha/blob/master/src/slide.c)：滑动字典、哈希链搜索、匹配决策与字典更新。
- [src/huf.c](https://github.com/jca02266/lha/blob/master/src/huf.c)：用于核对标准 LHA 的 Huffman 编码组织及其与当前流格式的区别。

上游文件列出的相关贡献者包括 Nobutaka Watazaki、T. Okamoto；完整项目贡献记录见上游。该项目使用自定义许可，其权威文本位于 [man/lha.n 的 DISTRIBUTION 节](https://github.com/jca02266/lha/blob/master/man/lha.n)，不能将它标记为 BSD、MIT 或不受限制的公共领域代码。该节包含发布材料、二进制单独分发和商用等条件。

本包保留该许可节的原文参考副本：[LICENSES/LHa-for-UNIX-reference.txt](LICENSES/LHa-for-UNIX-reference.txt)。该副本只说明被参考项目的条款，不是对当前 LHDC LZ 实现的重新授权。

### libarchive LHA reader

参考文件：[archive_read_support_format_lha.c](https://github.com/libarchive/libarchive/blob/master/libarchive/archive_read_support_format_lha.c)。相关参考内容为解码状态、输入/输出边界和重叠回溯复制规则。

该文件头列明 `Copyright (c) 2008-2014 Michihiro NAKAJIMA`，采用两条款 BSD 许可。随附版权及条款副本为 [LICENSES/libarchive-LHA-BSD-2-Clause.txt](LICENSES/libarchive-LHA-BSD-2-Clause.txt)。这里只记录该 LHA reader 文件，不将 libarchive 全仓库的所有文件概括为同一许可。

本项目没有随包引入 libarchive 的库、LHA reader 编译单元或 Huffman 表。将来若实际复制上游实现，应按引入文件更新适用范围及通知。

## 随包文件及核验

- [LICENSES/AOSP-reference.md](LICENSES/aosp-reference.zh-CN.md)：Android/LHDC 参考范围与已确认的许可状态。
- [LICENSES/Apache-2.0.txt](LICENSES/Apache-2.0.txt)：Google AOSP V5 编码参考所适用的 Apache-2.0 全文。
- [LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt](LICENSES/AOSP-Rust-KISS-FFT-BSD-3-Clause.txt)：已与官方源码树核对一致的 Rust FFT 文件自身的 BSD 通知。
- [LICENSES/KISS-FFT-BSD-3-Clause.txt](LICENSES/KISS-FFT-BSD-3-Clause.txt)：KISS FFT 版权与 BSD 三条款全文。
- [LICENSES/LHa-for-UNIX-reference.txt](LICENSES/LHa-for-UNIX-reference.txt)：LHa 自定义许可节，仅作为算法参考项目的说明。
- [LICENSES/libarchive-LHA-BSD-2-Clause.txt](LICENSES/libarchive-LHA-BSD-2-Clause.txt)：所参考 LHA reader 的版权与 BSD 两条款全文。


其他现有数学代码的版权与许可说明仍见 [math/platform/windows/NOTICE.math.txt](math/platform/windows/NOTICE.math.txt)。本次只增加引用与许可文档，不修改算法、编译选项或函数前的功能说明。

## Savitech 商标与专利声明 (Trademarks & Patents)

- **商标权利**：“LHDC”与“LLAC”是 Savitech Corp.（盛微先进科技股份有限公司）的注册商标或商标。本项目引用上述名称仅用于客观标识兼容的音频编解码格式及标准规范，不代表本项目获得 Savitech 的官方背书、赞助或存在附属关系。
- **专利与授权免责**：本项目开源发布仅适用于本仓库代码著作权，**不包含、不默示授予任何涵盖 LHDC / LLAC 核心编解码技术的专利许可（Patent Grant）或商业运营授权**。商业产品集成及硬件部署须自行向 Savitech Corp. 或相关权利人取得正式的商业授权与认证。

