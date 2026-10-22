/* Ring-buffer compatibility names share the common implementation. */
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_TYPES_CIRBUF_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_TYPES_CIRBUF_H
#include "../../../../../common/include/lhdc_cirbuf.h"
#define v4f_cirbuf_init lhdc_cirbuf_init_inline
#define v4f_cirbuf_reset lhdc_cirbuf_reset_inline
#define v4f_cirbuf_len lhdc_cirbuf_len_inline
#define v4f_cirbuf_empty_len lhdc_cirbuf_empty_len_inline
#define v4f_cirbuf_get lhdc_cirbuf_get
#define v4f_cirbuf_put lhdc_cirbuf_put
#define v4f_cirbuf_get_no_copy lhdc_cirbuf_get_no_copy
#define v4f_cirbuf_put_no_copy lhdc_cirbuf_put_no_copy
#endif
