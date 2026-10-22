/* Check disabled logging has no argument side effects and enabled logging evaluates arguments once. */
#include "../include/lhdc_log.h"
#include <stdio.h>
static int counter;
#if LHDC_ENABLE_LOGGING
/* Increment a test counter to track logging-argument evaluation. */
static int next_value(void) {
    return ++counter;
}
#else
/* No implementation is supplied with logging disabled; residual argument evaluation must fail to link. */
int next_value(void);
#endif
/* Verify disabled logs skip argument evaluation and enabled logs evaluate once. */
int main(void) {
    int result = lhdc_log_print(LHDC_LOG_INFO, "LHDC-log-test", "value=%d", next_value());
#if LHDC_ENABLE_LOGGING
    if (counter != 1)
        return 2;
    (void)result; /* Android's native result also depends on logd filtering and acceptance. */
#else
    if (counter != 0 || result != 1)
        return 3;
    /* Linking must fail if an undefined function remains in the object file. */
    extern int deliberately_unlinked_log_argument(void);
    result =
        lhdc_log_print(LHDC_LOG_DEBUG, "LHDC-log-test", "%d", deliberately_unlinked_log_argument());
    if (result != 1)
        return 4;
#endif
    puts("logging argument evaluation passed");
    return 0;
}
