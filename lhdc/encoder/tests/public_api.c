#include <lhdc/encoder/lhdc_bt.h>
#include <stdint.h>
/* Check creation/destruction of both encoders using public targets only. */
int main(void) {
    HANDLE_LHDC_BT v4 = lhdcBT_get_handle(4);
    HANDLE_LHDCV5_BT v5 = 0;
    if (!v4 || lhdcv5BT_get_handle(1, &v5) || !v5)
        return 1;
    lhdcBT_free_handle(v4);
    return lhdcv5BT_free_handle(v5) != 0;
}
