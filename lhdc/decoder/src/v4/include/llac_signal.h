#ifndef LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SIGNAL_H
#define LHDC_DECODER_SRC_V4_INCLUDE_LLAC_SIGNAL_H
#include <stdint.h>
#include "../../../../common/include/types/llac_side_types.h"
void llac_apply_tns(float *x, const LLACTnsData *tns, int bw_fcbin, int N, int fs, int frame_dms);
void llac_apply_sns_gains(float *x, float *scf, int *bands_offset, int fdns_npts);
void llac_interpolate_sns_gains(float *gains, int bands_number, float *gains_int, float *tmp);
#endif
