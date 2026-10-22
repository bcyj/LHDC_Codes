#ifndef LHDC_WINDOWS_FLOAT_MATH_CONSTANTS_H
#define LHDC_WINDOWS_FLOAT_MATH_CONSTANTS_H
#include <audio_math/platform/windows/constants.h>

/* Original platform-polynomial fitted coefficients; do not replace with exact mathematical constants even when close.
 * Cn denotes the x^n term; literals and fused evaluation order jointly determine rounding.
 */

/* EXP2_C1 approximates ln(2), LOG2_C1 approximates log2(e); both are fitted values. */

/* Ordinary double/fused evaluation differs by less than 2^-42, with extra endpoint-rounding margin. */

/* exp2 polynomial rounding guard interval before multiplying the common positive scale. */

/* Ordinary double sine/cosine polynomial rounding intervals; reduce angles before polynomial evaluation. */

/* Exact bounded-float reduction split: sums to the specified double pi/2, excluding the true-pi tail. */

/* Exact numeric constants; preserve literal types and values and express casts explicitly at use sites. */

/* SSE2 natural exponent's 64-segment reduction/cubic model; do not substitute a higher-order approximation. */

/* Single-precision decimal-log table policy/split constants; preserve every float rounding. */
#define LHDC_WIN_LOG10_HIGH_ZERO_BITS 14
#define LHDC_WIN_LOG10_SERIES_C5_F32 0.012500000186264515f
#define LHDC_WIN_LOG10_SERIES_C3_F32 0.0833333358168602f
#define LHDC_WIN_LOG10_E_LOW_F32 0.0007007318781688809f
#define LHDC_WIN_LOG10_E_HIGH_F32 0.43359375f
#define LHDC_WIN_LOG10_CUBIC_F32 0.3333333432674408f
#define LHDC_WIN_LOG10_E_F32 0.4342944920063019f
#define LHDC_WIN_LOG10_TWO_LOW_F32 0.0002487456367816776f
#define LHDC_WIN_LOG10_TWO_HIGH_F32 0.30078125f

/* Reduction/polynomial constants for positive-base SNS/TNS powers. */
#define LHDC_WIN_POW_NEAR_ONE_F64 0.0625
#define LHDC_WIN_POW_LOG_C5_F64 0.012500000003771751
#define LHDC_WIN_POW_LOG_C3_F64 0.08333333333333179
#define LHDC_WIN_POW_LOG_C9_F64 0.0004348877777076146
#define LHDC_WIN_POW_LOG_C7_F64 0.0022321399879194482
#define LHDC_WIN_POW_THIRD_F64 0.3333333333333333
#define LHDC_WIN_POW_LN2_F64 0.6931471805599453
#define LHDC_WIN_POW_OVERFLOW_F64 88.72283935546875
#define LHDC_WIN_POW_UNDERFLOW_MAG_F64 103.2789306640625
#define LHDC_WIN_POW_REDUCTION_F64 92.33248261689366
#define LHDC_WIN_POW_LN2_OVER_64_F64 0.010830424696249145
#define LHDC_WIN_POW_SIXTH_F64 0.16666666666666666

/* Rounded base-ten reduction and conservative relative rounding interval for exponent synthesis. */
#define LHDC_WIN_POW_LN10_POLICY_F64 2.3025850929940455
#define LHDC_WIN_POW_ROUND_GUARD_F64 2.2737367544323206e-13

/* Original log-table indices used by the non-near-one TNS branch over [0.85f,1]. */
#define LHDC_WIN_POW_LOG_FIRST_INDEX 179u
#define LHDC_WIN_POW_LOG_TABLE_ROWS 46

/* Relative rounding guard 2^-32 for TNS bases [0.85f,1] and powers 2..8. */
#define LHDC_WIN_TNS_POWER_ROUND_GUARD_F64 2.3283064365386963e-10

#endif
