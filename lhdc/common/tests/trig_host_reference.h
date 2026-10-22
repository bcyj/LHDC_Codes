#ifndef LHDC_TEST_TRIG_HOST_REFERENCE_H
#define LHDC_TEST_TRIG_HOST_REFERENCE_H
#include <stdint.h>
typedef uint64_t (*LhdcTrigHostFunction)(uint64_t, uint64_t);
enum { LHDC_TEST_TRIG_HOST_OPERATIONS = 9 };
extern LhdcTrigHostFunction const lhdc_test_trig_host_reference[LHDC_TEST_TRIG_HOST_OPERATIONS];
#endif
