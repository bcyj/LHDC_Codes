#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LHDC_DEC_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LHDC_DEC_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "../../../../common/include/lhdc_cirbuf.h"
#define cirbuf_init lhdc_cirbuf_init
#define cirbuf_reset lhdc_cirbuf_reset
#define cirbuf_len lhdc_cirbuf_len
#define cirbuf_empty_len lhdc_cirbuf_empty_len
#define cirbuf_get lhdc_cirbuf_get
#define cirbuf_put lhdc_cirbuf_put
#define cirbuf_get_no_copy lhdc_cirbuf_get_no_copy
#define cirbuf_put_no_copy lhdc_cirbuf_put_no_copy
#ifdef __cplusplus
}
#endif
#endif
