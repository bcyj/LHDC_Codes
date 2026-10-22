#ifndef LHDC_DECODER_SRC_V5_INCLUDE_LHDC_TABLES_H
#define LHDC_DECODER_SRC_V5_INCLUDE_LHDC_TABLES_H
#include "lhdc_dec.h"
#include "types/lhdc_v5_global_table_types.h"
/* Read initial frequencies for index zero or one only. */
unsigned *lhdc_table_init_fq_table(int);
/* Query the 161-element power lookup table. */
int *lhdc_table_power_of_2_table(void);
/* Query the 18-entry segment-cutoff table. */
segment_cutoff_struct *lhdc_table_segment_cutoff(void);
/* Query three scrambled-table classes for six segment indices; return null for unsupported parameters. */
int *lhdc_table_segment_configuration(int, int);
/* Decode segment settings using alternating permutation/XOR, copying float fields bitwise. */
int lhdc_table_segment_read(segment_setting_struct *, int);
/* Rebuild segment, level, initial-probability, and power tables from encoded frame length. */
int lhdc_dec_lossy_frame_length_program(tdata_all_buffer_struct *, int);
/* Smooth segment offsets with four-point moving averages and a final three-point average. */
void lhdc_shift_by_offset_2(segment_setting_struct *, int *, int *);
/* Generate adaptive steps and mean-removed offsets from neighboring directions, clamped to [-2560,1024]. */
void lhdc_dec_freq_shift(segment_setting_struct *, unsigned char *, int, int *);
/* Look up Q20 gains and scale the integer spectrum segment by segment. */
void lhdc_freq_shift_apply_decode(segment_setting_struct *, int *, int *, int *, int);
#endif
