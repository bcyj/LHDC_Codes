#include <lhdc/decoder/decoder.h>
#include <stdlib.h>
#include "../../common/include/lhdc_log.h"
static unsigned log_calls;
static unsigned other_log_calls;
/* Count another decoder instance's log callbacks. */
static void other_log(char *message) {
    (void)message;
    ++other_log_calls;
}
/* Count the current decoder instance's log callbacks. */
static void capture_log(char *message) {
    (void)message;
    ++log_calls;
}
/* Verify memory queries, initialization, and lifetime of the combined two-version library through public headers. */
int main(void) {
    if (lhdc_util_dec_get_mem_req(400, NULL) != (uint32_t)LHDC_DEC_INVALID_ARGUMENT)
        return 20;
    if (lhdcv5_util_dec_get_mem_req(550, 48000, 0, 0, NULL) != LHDC_DEC_INVALID_ARGUMENT)
        return 21;
    if (lhdcv5_util_init_decoder(NULL, 16, 48000, 0, 50, 0, 550) != LHDC_DEC_INVALID_ARGUMENT)
        return 22;
    if (lhdcv5_util_dec_get_sample_size(NULL) != LHDC_DEC_NOT_INITIALIZED)
        return 23;
    if (lhdcv5_util_dec_channel_selsect(NULL, 0) != LHDC_DEC_NOT_INITIALIZED)
        return 24;
    if (lhdcv5_util_dec_destroy(NULL) != LHDC_DEC_OK)
        return 25;
    uint32_t bytes4 = 0, bytes5 = 0;
    if (lhdc_util_dec_get_mem_req(400, &bytes4) || !bytes4)
        return 1;
    if (lhdcv5_util_dec_get_mem_req(550, 48000, 0, 0, &bytes5) || !bytes5)
        return 2;
    uint32_t *v4 = calloc(1, bytes4);
    void *v5 = calloc(1, bytes5);
    if (!v4 || !v5)
        return 3;
    lhdcInit(v4, 16, 48000, 400000, 400);
    if (lhdcv5_util_init_decoder(v5, 16, 48000, 0, 50, 0, 550))
        return 4;
    lhdc_register_log_cb(v4, capture_log);
    lhdcv5_util_dec_register_log_cb(v5, capture_log);
    lhdcChannelSelsect(v4, 0);
    lhdcv5_util_dec_channel_selsect(v5, 0);
    uint32_t *other4 = calloc(1, bytes4);
    void *other5 = calloc(1, bytes5);
    if (!other4 || !other5)
        return 8;
    lhdcInit(other4, 16, 48000, 400000, 400);
    if (lhdcv5_util_init_decoder(other5, 16, 48000, 0, 50, 0, 550))
        return 9;
    lhdc_register_log_cb(other4, other_log);
    lhdcv5_util_dec_register_log_cb(other5, other_log);
    log_calls = other_log_calls = 0;
    lhdcChannelSelsect(v4, 1);
    lhdcv5_util_dec_channel_selsect(v5, 1);
#if LHDC_ENABLE_LOGGING
    if (log_calls != 2 || other_log_calls)
        return 10;
#endif
    lhdc_register_log_cb(v4, NULL);
    lhdcv5_util_dec_register_log_cb(v5, NULL);
    lhdcChannelSelsect(other4, 1);
    lhdcv5_util_dec_channel_selsect(other5, 1);
#if LHDC_ENABLE_LOGGING
    if (log_calls != 2 || other_log_calls != 2)
        return 11;
#else
    if (log_calls || other_log_calls)
        return 12;
#endif
    lhdcDestroy(other4);
    lhdcv5_util_dec_destroy(other5);
    free(other4);
    free(other5);
    if (!lhdcGetSampleSize(v4) || lhdcv5_util_dec_get_sample_size(v5) <= 0)
        return 5;
    lhdcDestroy(v4);
    int result = lhdcv5_util_dec_destroy(v5);
    lhdc_register_log_cb(v4, NULL);
    lhdcv5_util_dec_register_log_cb(v5, NULL);
    free(v4);
    free(v5);
#if LHDC_ENABLE_LOGGING
    if (!log_calls)
        return 6;
#else
    if (log_calls)
        return 7;
#endif
    return result != 0;
}
