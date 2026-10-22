/* LHDC AR backend selection and parameters; the generic interface does not depend on this header. */
#ifndef LHDC_AR_PCM_BACKEND_H
#define LHDC_AR_PCM_BACKEND_H
#include <ar/pcm.h>
#include <ar/lhdc_pcm.h>
#ifdef __cplusplus
extern "C" {
#endif
const ArPcmBackend *lhdc_ar_pcm_backend(LhdcArProfile profile);
const ArPcmBackend *lhdc_ar_default_backend(void);
#ifdef __cplusplus
}
#endif
#endif
