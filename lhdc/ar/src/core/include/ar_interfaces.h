/* AR/DSP declarations shared by V4/V5; version macros select the linked symbols.
 * One declaration set describes the common interface without merging version-specific global state. */
#ifndef LHDC_AR_SRC_CORE_INCLUDE_AR_INTERFACES_H
#define LHDC_AR_SRC_CORE_INCLUDE_AR_INTERFACES_H
#include "lhdc_version.h"
#include "../../../include/ar/types.h"
#include "ar_workspace_views.h"

void LHDC_FN(kiss_fft_cleanup)(void);

int32_t LHDC_FN(kiss_fft_next_fast_size)(int32_t n);

uint32_t LHDC_FN(lhdc_ar_api_mem_bytes)(void);

int32_t LHDC_FN(lhdc_ar_api_res_acquire)(uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_api_init)(int32_t nArInChNum, int32_t nArInBitsPerSample,
                                  int32_t nArInSampleRate, int32_t nArOutChNum,
                                  int32_t nArOutBitsPerSample, uint8_t *pucLhdcArMem,
                                  uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_api_ready_to_process)(uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_api_process_pcm)(uint8_t *pucDst, uint8_t *pucSrc, lhdc_ulong_t ulNumBytes,
                                         uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(GetOneSample_f)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, float *pfSample);

int32_t LHDC_FN(SaveOneSample_f)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, float fSample);

int32_t LHDC_FN(lhdc_ar_api_set_gyro_pos)(int32_t nGyroX, int32_t nGyroY, int32_t nGyroZ,
                                          uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_api_set_ar_params)(uint8_t *pucArParams, uint32_t uiArParamsBufBytes,
                                           uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_api_get_ar_params)(uint8_t *pucArParams, uint32_t uiArParamsBufBytes,
                                           uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(GetOneSample)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, int32_t *pnSample);

int32_t LHDC_FN(SaveOneSample)(int32_t nBytesPerSample, uint8_t *pbSamplePtr, int32_t nSample);

uint32_t LHDC_FN(LhdcArMemBytesReq)(void);

int32_t LHDC_FN(InitLhdcAR)(int32_t nLhdcArInChNum, int32_t nLhdcArInBitsPerSample,
                            int32_t nLhdcArInSampleRate, int32_t nLhdcArOutChNum,
                            int32_t nLhdcArOutBitsPerSample, uint8_t *pucLhdcArMem,
                            uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(ProcessLhdcAR)(uint8_t *pucDst, uint8_t *pucSrc, lhdc_ulong_t ulNumBytes,
                               uint8_t *pucLhdcArMem, uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(SetGyroPosToLhdcAR)(int32_t nX, int32_t nY, int32_t nZ, uint8_t *pucLhdcArMem,
                                    uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(SetARParams)(uint8_t *pucARParams, uint32_t uiARParamsBytes, uint8_t *pucLhdcArMem,
                             uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(GetARParams)(uint8_t *pucARParams, uint32_t uiARParamsBytes, uint8_t *pucLhdcArMem,
                             uint32_t uiLhdcArMemBytes);

int32_t LHDC_FN(lhdc_ar_set_ch_pos)(int32_t nChId, int32_t nAzimuthIdx, int32_t nElevationIdx,
                                    int32_t nUpdatePosChange, PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(hrir_update_directions)(int32_t *pnHrirCoefUpdate, PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(hrir_load_default_directions)(PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(lhdc_ar_one_blk)(int32_t nBlkSampleNum, PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(lhdc_ar_one_frame)(int32_t nFrameSampleOffset, int32_t nFrameSampleNum,
                                   PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(lhdc_ar_select_coef)(int32_t nCoefIdx, PST_LHDC_AR pstLhdcAr);

int32_t LHDC_FN(lhdc_reverb_one_blk)(int32_t nBlkSampleNum, PST_LHDC_REVERB pstLhdcReverb);

int32_t LHDC_FN(lhdc_dist_effect_one_blk)(int32_t nBlkSampleNum,
                                          PST_LHDC_DIST_EFFECT pstLhdcDistEffect);

uint32_t LHDC_FN(lhdc_ar_mem_req_bytes)(void);

int32_t LHDC_FN(lhdc_ar_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                              int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                              int32_t nSampleRate, uint32_t uiWorkingHoldSamples,
                              uint8_t *pucMemLhdcAr);

int32_t LHDC_FN(lhdc_ar_set_gyro_pos)(int32_t x_cur, int32_t y_cur, int32_t z_cur,
                                      uint8_t *pucMemLhdcAr);

int32_t LHDC_FN(lhdc_ar_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                     float *pfPcmIn3, float *pfPcmOut0, float *pfPcmOut1,
                                     int32_t nSampleNum, uint8_t *pucMemLhdcAr);

int32_t LHDC_FN(lhdc_ar_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcAr);

uint32_t LHDC_FN(lhdc_upmix_mem_req_bytes)(void);

int32_t LHDC_FN(lhdc_upmix_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                 int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                 int32_t nSampleRate, uint8_t *pucMemLhdcUpmix);

int32_t LHDC_FN(lhdc_upmix_process_f)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmOut0,
                                      float *pfPcmOut1, float *pfPcmOut2, float *pfPcmOut3,
                                      int32_t nSampleNum, uint8_t *pucMemLhdcUpmix);

int32_t LHDC_FN(lhdc_upmix_output_f)(int32_t nSampleNum, uint8_t *pucMemLhdcUpmix);

uint32_t LHDC_FN(lhdc_dist_effect_mem_req_bytes)(void);

int32_t LHDC_FN(lhdc_dist_effect_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                       int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                       int32_t nSampleRate, uint8_t *pucMemLhdcDistEffect);

int32_t LHDC_FN(lhdc_dist_effect_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                              float *pfPcmIn3, float *pfPcmOut0, float *pfPcmOut1,
                                              float *pfPcmOut2, float *pfPcmOut3,
                                              int32_t nSampleNum, uint8_t *pucMemLhdcDistEffect);

int32_t LHDC_FN(lhdc_dist_effect_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcDistEffect);

uint32_t LHDC_FN(lhdc_reverb_mem_req_bytes)(void);

int32_t LHDC_FN(lhdc_reverb_init)(int32_t nChNumInput, int32_t nBitsPerSampleInput,
                                  int32_t nChNumOutput, int32_t nBitsPerSampleOutput,
                                  int32_t nSampleRate, uint8_t *pucMemLhdcReverb);

int32_t LHDC_FN(lhdc_reverb_set_gyro_pos)(int32_t x_cur, int32_t y_cur, int32_t z_cur,
                                          uint8_t *pucMemLhdcReverb);

int32_t LHDC_FN(lhdc_reverb_process_f_o)(float *pfPcmIn0, float *pfPcmIn1, float *pfPcmIn2,
                                         float *pfPcmOut0, float *pfPcmOut1, int32_t nSampleNum,
                                         uint8_t *pucMemLhdcReverb);

int32_t LHDC_FN(lhdc_reverb_output_f_o)(int32_t nSampleNum, uint8_t *pucMemLhdcReverb);

int32_t LHDC_FN(lhdc_upmix_get_lib_workspace_size)(uint32_t *puiLibWorkspaceSize);

int32_t LHDC_FN(lhdc_upmix_init_lib)(PST_LHDC_UPMIX pstLhdcUpmix);

int32_t LHDC_FN(lhdc_upmix_one_blk)(int32_t nBlkSampleNum, PST_LHDC_UPMIX pstLhdcUpmix);

uint32_t LHDC_FN(lhdc_ar_params_mem_req_bytes)(void);

int32_t LHDC_FN(lhdc_ar_params_default)(uint8_t *pucMemLhdcArParams);

int32_t LHDC_FN(lhdc_ar_params_set)(PLHDC_AR_API_BLK state);

int32_t LHDC_FN(lhdc_ar_params_get)(uint8_t *parameters, PLHDC_AR_API_BLK state);

void LHDC_FN(dist_effect_reduce)(PST_LHDC_DIST_EFFECT state, int32_t samples);

void LHDC_FN(sincos_degrees)(float angle, float *sine, float *cosine);

void LHDC_FN(reverb_effect_reduce)(PST_LHDC_REVERB state, int32_t samples);

uint32_t LHDC_FN(reverb_workspace_bytes)(void);

void LHDC_FN(init_reverb_memory)(uint8_t *ptr_reverb_mem, uint32_t size);

float LHDC_FN(remainder_with_integer_tolerance)(float u0, float u1);

int32_t LHDC_FN(comple_filter_bank_get_coefficient_ex)(int32_t bank_index, int32_t sample_rate,
                                                       LHDCIirPlan *plan);

int32_t LHDC_FN(shelving_filter_get_coefficient_ex)(int32_t sample_rate, int32_t front_0_rear_1,
                                                    LHDCIirPlan *plan);

int32_t LHDC_FN(check_protection_area)(LHDCUpmixDirectory *workspace);

int32_t LHDC_FN(upmix_lib_get_request_workspace_size)(uint32_t *rtn_size);

int32_t LHDC_FN(upmix_lib_init)(uint8_t *addr_mem, uint32_t mem_size);

int32_t LHDC_FN(upmix_lib_reset_cache_buffer)(uint8_t *addr_mem);

int32_t LHDC_FN(upmix_lib_reset_setting)(uint8_t *addr_mem);

int32_t LHDC_FN(get_vbap_gain_result)(float *ptr_power_l_bank, float *ptr_power_r_bank,
                                      int32_t frame_length, float *ptr_vbap_gain_buf);

int32_t LHDC_FN(calculate_comp_filter_bank)(int32_t bank_index, int32_t sample_rate,
                                            int32_t sample_length, int32_t left_0_right_1,
                                            float *ptr_out_bank_data,
                                            LHDCUpmixDirectory *workspace);

int32_t LHDC_FN(upmix_lib_process)(PST_LHDC_UPMIX state, int32_t frame_length);

int32_t LHDC_FN(upmix_lib_process_ex)(const float *ptr_data_in_l, const float *ptr_data_in_r,
                                      float phase_l, float phase_r, float power_alpha,
                                      float gain_filter_bank, int32_t frame_length,
                                      int32_t upmix_type, int32_t sample_rate,
                                      float *ptr_data_out_fl, float *ptr_data_out_fr,
                                      float *ptr_data_out_c, float *ptr_data_out_lfe,
                                      float *ptr_data_out_rl, float *ptr_data_out_rr,
                                      LHDCUpmixDirectory *workspace);

void LHDC_FN(process_iir_filter)(const double *input, const LHDCIirPlan *plan, int32_t frame_length,
                                 double *zi, double *zo, double *output);

int32_t LHDC_FN(vbap_gain_get_table_ex)(const float **rtn_ptr_table);

void LHDC_FN(cc_buf_list_init)(cc_buf_list *list, cc_buf *list_mem, float *data_mem, uint32_t size,
                               uint32_t count);

void LHDC_FN(cc_buf_list_update)(cc_buf_list *list, float *new_data);

void LHDC_FN(upconv_alloc)(uint8_t *allocated_mem_addr, uint32_t *mem_size);

void LHDC_FN(upconv_init)(LHDCConvolutionDirectory *workspace);

void LHDC_FN(upconv_process)(float *w_in_frame, float *x_in_frame, float *y_in_frame,
                             float *l_out_frame, float *r_out_frame,
                             LHDCConvolutionDirectory *workspace);

void LHDC_FN(fft_buffer_n_to_4n)(const float *new_frame, float *prev_frame, float *new_data,
                                 uint32_t frame_size, uint32_t partition_size);

void LHDC_FN(hrir_upconv_alloc)(uint8_t *allocated_mem_addr, uint32_t *mem_size);

void LHDC_FN(hrir_upconv_process)(PST_LHDC_AR state, int32_t sample_offset);

int32_t LHDC_FN(comple_filter_bank_get_coefficient)(int32_t bank_index, int32_t sample_rate,
                                                    double *rtn_coef_a, double *rtn_coef_b);
int32_t LHDC_FN(shelving_filter_get_coefficient)(int32_t sample_rate, int32_t front_0_rear_1,
                                                 double *rtn_coef_a, double *rtn_coef_b);
int32_t LHDC_FN(vbap_gain_get_table)(float *rtn_ptr_table);
int32_t LHDC_FN(vbap_gain_table_element_count)(int32_t *rtn_element_cnt);
#endif
