#ifndef LHDC_AR_COMMON_VBAP_ANGLE_H
#define LHDC_AR_COMMON_VBAP_ANGLE_H

/* Convert a finite sine value in [-0.5, 0.5] to the integer angle used by VBAP. */
int lhdc_ar_vbap_angle(float sine);

#endif
