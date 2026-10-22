#ifndef LHDC_AR_COMMON_GAIN_SMOOTHING_H
#define LHDC_AR_COMMON_GAIN_SMOOTHING_H

/* Compute decay from the single-precision sample-rate/attack-time product, preserving division and host exponent policy. */
float lhdc_ar_smoothing_decay(float rate_time_product);

#endif
