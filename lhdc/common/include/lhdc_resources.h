#ifndef LHDC_RESOURCES_H
#define LHDC_RESOURCES_H

/* Resource configuration constrains allocation, FFT scheduling, and diagnostics only; it does not establish kernel math or FP-state support. */
#ifndef LHDC_DRIVER_RESOURCES
#define LHDC_DRIVER_RESOURCES 0
#endif
#if LHDC_DRIVER_RESOURCES
#ifndef LHDC_KISS_FFT_ITERATIVE_STATIC
#define LHDC_KISS_FFT_ITERATIVE_STATIC 1
#endif
#ifndef LHDC_EXTERNAL_ALLOCATOR_ONLY
#define LHDC_EXTERNAL_ALLOCATOR_ONLY 1
#endif
#ifndef LHDC_ENABLE_LOGGING
#define LHDC_ENABLE_LOGGING 0
#endif
#if !LHDC_KISS_FFT_ITERATIVE_STATIC || !LHDC_EXTERNAL_ALLOCATOR_ONLY || LHDC_ENABLE_LOGGING
#error "Driver resources require iterative FFT, explicit allocation and disabled logging"
#endif
#ifndef NDEBUG
#error "Driver resources currently require NDEBUG; kernel diagnostic policy is not implemented"
#endif
#endif
#endif
