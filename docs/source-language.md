# Source Translation and Validation

English | [Simplified Chinese](zh-CN/source-language.md) | [Home](../README.md)

This release uses English source comments and English documentation by default. The development tree is unchanged. Original Chinese documents are retained as `README.zh-CN.md`, `THIRD_PARTY_NOTICES.zh-CN.md`, and matching pages under `docs/zh-CN/`.

The translation covers 2,216 distinct Chinese comment lines in 342 source/header files. It preserves numeric-domain, rounding, ownership, aliasing, partial-update, and error-return explanations rather than replacing them with generic function descriptions.

All non-comment C/H tokens are compared with the pre-translation sources. Identifiers, function signatures, expressions, numeric literals, preprocessor directives, and string literals are unchanged. Original third-party license texts, including the Japanese LHa reference section, remain byte-for-byte unchanged. English and Chinese explanatory notices do not replace those texts.

Documentation links use English filenames. English feature/build pages link to matching Chinese pages; component READMEs link to their original Chinese counterparts. Module-size measurements retain their original date and values and are not presented as new translation measurements.

## Validation results

| Check | Result |
|---|---|
| Non-comment C/H tokens and source line counts | Identical in all 342 translated files |
| Chinese in source/build files or English documents | None |
| Original Chinese documents | All 14 retained, with updated links/navigation |
| Authoritative license texts | All six checked texts byte-identical |
| Relative documentation links | Valid |
| Windows Win32 / x64 | Builds passed; 47 tests passed per architecture |
| Android ARM32 / ARM64 | NDK builds passed; seven SOs per ABI |
| Android source list | Check passed |

Current file hashes and validation summaries are recorded in [release_manifest.json](../release_manifest.json). Reproducible translation/build intermediates have been removed. Prior codec/AR runtime output regressions are historical evidence, not new device executions in this translation pass.
