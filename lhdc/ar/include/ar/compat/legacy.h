/* AR compatibility interface; see ../pcm.h for PCM processing. */
#ifndef LHDC_AR_COMPAT_LEGACY_H
#define LHDC_AR_COMPAT_LEGACY_H
#include "../../../../common/include/types/lhdc_base_types.h"
#ifdef __cplusplus
extern "C" {
#endif
uint32_t v4f_LhdcArMemBytesReq(void);
uint32_t v5f_LhdcArMemBytesReq(void);
int32_t v4f_InitLhdcAR(int32_t, int32_t, int32_t, int32_t, int32_t, uint8_t *, uint32_t);
int32_t v5f_InitLhdcAR(int32_t, int32_t, int32_t, int32_t, int32_t, uint8_t *, uint32_t);
int32_t v4f_ProcessLhdcAR(uint8_t *, uint8_t *, lhdc_ulong_t, uint8_t *, uint32_t);
int32_t v5f_ProcessLhdcAR(uint8_t *, uint8_t *, lhdc_ulong_t, uint8_t *, uint32_t);
int32_t v4f_SetARParams(uint8_t *, uint32_t, uint8_t *, uint32_t);
int32_t v5f_SetARParams(uint8_t *, uint32_t, uint8_t *, uint32_t);
int32_t v4f_GetARParams(uint8_t *, uint32_t, uint8_t *, uint32_t);
int32_t v5f_GetARParams(uint8_t *, uint32_t, uint8_t *, uint32_t);
int32_t v4f_SetGyroPosToLhdcAR(int32_t, int32_t, int32_t, uint8_t *, uint32_t);
int32_t v5f_SetGyroPosToLhdcAR(int32_t, int32_t, int32_t, uint8_t *, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
