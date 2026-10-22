#include "../src/v5/include/encoder_internal.h"
#include "../src/v5/include/codec/codec_api.h"
#include <lhdc/encoder/v5/lhdcv5BT.h>
#include <stdio.h>
#include <string.h>

/* Check sample-rate switching/custom bitrate tables affect only their own instances. */
int main(void) {
    void *a = NULL, *b = NULL;
    if (lhdcv5BT_get_handle(1, &a) || lhdcv5BT_get_handle(1, &b))
        return 1;
    if (lhdcv5BT_init_encoder(a, 44100, 24, 3, 660, 10) ||
        lhdcv5BT_init_encoder(b, 48000, 16, 3, 660, 10))
        return 2;
    lhdcv5_para_t *pa = ((lhdcv5_cb_t *)a)->enc.lhdcv5;
    lhdcv5_para_t *pb = ((lhdcv5_cb_t *)b)->enc.lhdcv5;
    uint32_t value = 0;
    if (v5f_lhdcv5_util_get_bitrate(pa, 3, &value) || value != 240)
        return 3;
    if (v5f_lhdcv5_util_get_bitrate(pb, 3, &value) || value != 256)
        return 4;
    int32_t custom[10], unchanged[10];
    if (v5f_lhdcv5_util_get_bitrate_t(pa, 44100, custom, 10, 0x20220421))
        return 5;
    memcpy(unchanged, custom, sizeof custom);
    custom[3] = 248;
    if (v5f_lhdcv5_util_set_bitrate_t(pa, 44100, custom, 10, 0x20220421))
        return 6;
    int32_t actual[10];
    if (v5f_lhdcv5_util_get_bitrate_t(pb, 44100, actual, 10, 0x20220421) ||
        memcmp(actual, unchanged, sizeof actual))
        return 7;
    if (lhdcv5BT_init_encoder(a, 44100, 24, 3, 660, 10) ||
        v5f_lhdcv5_util_get_bitrate(pa, 3, &value) || value != 248)
        return 8;
    if (v5f_lhdcv5_util_get_bitrate(pb, 3, &value) || value != 256)
        return 9;
    lhdcv5BT_free_handle(a);
    if (lhdcv5BT_set_bitrate(b, 4))
        return 10;
    lhdcv5BT_free_handle(b);
    puts("PASS instance sample-rate/custom bitrate tables");
    return 0;
}
