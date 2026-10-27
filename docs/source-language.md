# Source Translation and Validation

English | [Simplified Chinese](zh-CN/source-language.md) | [Home](../README.md)

This release uses English source comments and English documentation by default. Original Chinese documents are retained as `README.zh-CN.md`, `THIRD_PARTY_NOTICES.zh-CN.md`, and matching pages under `docs/zh-CN/`.

The original translation covered 2,216 distinct Chinese comment lines in 342 source/header files. The
optimization synchronization through development commit `5d1b90c` translated every new or changed
implementation comment as well. Numeric-domain, rounding, ownership, aliasing, partial-update, and
error-return explanations are preserved rather than replaced with generic descriptions.

For synchronized production files, comment and whitespace removal matches development `HEAD` except
for three intentional release integrations: portable AR include roots, the Android manifest path
description, and retention of the release tree's prior unused-helper cleanup. Original third-party
license texts, including the Japanese LHa reference section, remain unchanged. English and Chinese
explanatory notices do not replace those texts.

Documentation links use English filenames. English feature/build pages link to matching Chinese pages; component READMEs link to their original Chinese counterparts. Module-size measurements retain their original date and values and are not presented as new translation measurements.

## Validation results

| Check | Result |
|---|---|
| Optimized production source range | `a43e0a24cd4bcb57186307444d9b252f19d7ef1b` through `5d1b90c` synchronized |
| Chinese in source/build files or English documents | None |
| Chinese documentation | Matching `zh-CN` optimization/build/feature pages updated |
| Authoritative license texts | All six checked texts byte-identical |
| Relative documentation links | Valid |
| Windows Win32 / x64 | Builds passed; 48 tests passed per architecture |
| Android ARM32 / ARM64 | NDK builds passed; seven SOs per ABI |
| Android source list | Check passed |

Build intermediates are outside the release tree. Prior codec/AR runtime output regressions remain
historical development evidence; this release synchronization independently rebuilt both Windows
architectures and both Android ABIs and reran the self-contained Windows tests.
