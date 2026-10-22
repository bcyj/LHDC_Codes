#ifndef LHDC_CIRBUF_EXPORTS_H
#define LHDC_CIRBUF_EXPORTS_H
/* GCC/Clang aliases must be declared in the target function's translation unit. */
#define LHDC_CIRBUF_ALIAS(name)                                                                    \
    extern __typeof__(lhdc_##name) name __attribute__((alias("lhdc_" #name)));                     \
    extern __typeof__(lhdc_##name) v4f_##name __attribute__((alias("lhdc_" #name)));               \
    extern __typeof__(lhdc_##name) v5f_##name __attribute__((alias("lhdc_" #name)))
LHDC_CIRBUF_ALIAS(cirbuf_init);
LHDC_CIRBUF_ALIAS(cirbuf_reset);
LHDC_CIRBUF_ALIAS(cirbuf_len);
LHDC_CIRBUF_ALIAS(cirbuf_empty_len);
LHDC_CIRBUF_ALIAS(cirbuf_get);
LHDC_CIRBUF_ALIAS(cirbuf_put);
LHDC_CIRBUF_ALIAS(cirbuf_get_no_copy);
LHDC_CIRBUF_ALIAS(cirbuf_put_no_copy);
#undef LHDC_CIRBUF_ALIAS
#endif
