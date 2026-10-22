#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_API_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_API_H
#include "../types/lhdc_native_types.h"
#include "../../../../../ar/src/core/include/ar_interfaces.h"

int32_t v5f_lhdcv5_encoder_free(lhdcv5_para_t *lhdcv5);

int32_t v5f_lhdcv5_encoder_get_mem_req(uint32_t version, uint32_t *mem_req_bytes);

int32_t v5f_lhdcv5_encoder_new(uint32_t version, lhdcv5_para_t *handle, uint32_t mem_size);

int32_t v5f_lhdcv5_encoder_set_target_bitrate_inx(lhdcv5_para_t *lhdcv5, uint32_t bitrate_inx);

int32_t v5f_lhdcv5_encoder_get_extra_func(lhdcv5_para_t *lhdcv5, uint32_t func_inx,
                                          bool *func_enable);

int32_t v5f_lhdcv5_encoder_set_extra_func(lhdcv5_para_t *lhdcv5, uint32_t func_inx,
                                          bool func_enable, uint8_t *func_data, uint32_t data_len,
                                          uint32_t loop_cnt);

int32_t v5f_lhdcv5_encoder_init(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                uint32_t bits_per_sample, uint32_t bitrate_inx,
                                uint32_t frame_duration, uint32_t mtu, uint32_t interval);

int32_t v5f_lhdcv5_encoder_get_frame_len(lhdcv5_para_t *lhdcv5, uint32_t *samples_per_frame);

int32_t v5f_lhdcv5_encoder_encode(lhdcv5_para_t *lhdcv5, uint8_t *in, uint32_t in_buf_bytes,
                                  uint8_t *out, uint32_t out_buf_bytes, uint32_t *written_bytes,
                                  uint32_t *out_frames);

int32_t v5f_lhdcv5_util_get_bitrate_t(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                      int32_t *bitrate_table, uint32_t num_bitrate_items,
                                      uint32_t get_tbl_reserved);

int32_t v5f_lhdcv5_util_set_bitrate_t(lhdcv5_para_t *lhdcv5, uint32_t sampling_freq,
                                      int32_t *bitrate_table, uint32_t num_bitrate_items,
                                      uint32_t get_tbl_reserved);

int32_t v5f_LhdcEncInit(LHDCV5_Workspace *workspace, int32_t wsize, int32_t ch,
                        int32_t bits_per_sample, int32_t sample_rate, int32_t frm_duration,
                        int32_t enc_frm_len);

int32_t v5f_LhdcEncSetEncFrameLen(int32_t enc_frm_len, LHDCV5_Workspace *workspace);

int32_t v5f_LhdcEncFrameSize(LHDCV5_Workspace *workspace);

void v5f_LhdcEncode(int32_t *pcm, uint32_t *out0, int32_t *out_len0, uint32_t *out1,
                    int32_t *out_len1, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_init(int32_t ch, int32_t bits_per_sample, int32_t sample_rate,
                             int32_t frm_duration, int32_t bitrate, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_get_samples_per_frame(int32_t *sample_per_frame,
                                              LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_bitrate(int32_t bitrate, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_encode(int32_t *pcm, uint8_t *encoded_frame, int32_t *encoded_frame_size,
                               LHDCV5_Workspace *workspace);

int32_t v5f_enc_process_header(hdr_s *hdr, int32_t ch, int32_t *enc_frm_len_usable,
                               uint8_t *encoded_frame);

int32_t v5f_lhdc_v5_enc_get_encoded_frame_size(int32_t *encoded_frame_size,
                                               LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_version(int32_t version, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_jas_flag(int32_t jas_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_ar_flag(int32_t ar_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_larc_flag(int32_t larc_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_set_meta_flag_and_data(int32_t meta_flag, uint8_t *meta_data,
                                               int32_t loop_count, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_get_jas_flag(int32_t *jas_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_get_ar_flag(int32_t *ar_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_get_larc_flag(int32_t *larc_flag, LHDCV5_Workspace *workspace);

int32_t v5f_lhdc_v5_enc_get_meta_flag_and_data(int32_t *meta_flag, uint8_t *meta_data,
                                               LHDCV5_Workspace *workspace);

int32_t v5f_mdct_get_size(void);

int32_t v5f_mdct_init(void *workspace, int32_t mdct_len, int32_t overlap, int32_t shift_bits,
                      struct mdct_buf_s *buf, kiss_fft_cpx *ptr_mdct_twid1,
                      kiss_fft_cpx *ptr_mdct_twid2, float *ptr_mdct_win,
                      kiss_fft_cpx *ptr_fft_twid);

void v5f_do_mdct(void *ws, int32_t c, const float *in, float *out);

void *v5f_lhdc_v5_get_mem_content_addr(LHDCV5_Workspace *workspace,
                                       enum lhdc_v5_enc_workspace_content_index index);

int32_t v5f_lhdc_v5_enc_workspace_get_size(int32_t ch, int32_t *size);

int32_t v5f_lhdc_v5_enc_workspace_init(int32_t ch, LHDCV5_Workspace *workspace);

void v5f_adsq_enc(int32_t *din, int32_t *dout, int32_t dn, uint8_t *code, int32_t *fi,
                  int32_t max_fi);

int32_t v5f_BitWriteBitsArrayU8(BitWriter *ptr_bw, uint8_t *ba, int32_t bits, int32_t len,
                                int32_t pad_zero);

void v5f_GenerateGainTable(LhdcEncContext *state, int32_t enc_len, int32_t frame_duration);

float v5f_EncGainByTable(int32_t gi, float *gtab, int32_t base, float gstep);

int32_t v5f_RiceQuotientEncode(uint32_t *q, int32_t qn, int32_t min_p0, uint8_t *qexp);

int32_t v5f_BitsPredictCompressOutput(LhdcEncContext *ecb, fac_enc *ptr_fenc_s, BitWriter *ptr_bw);

int32_t v5f_bpc_ac_eval_mv(const LhdcEncContext *ecb, int32_t qn, int32_t epn);

int32_t v5f_BitsPredictCompressEval(LhdcEncContext *ecb);

void v5f_FacEncInit(fac_enc *ptr_fenc_s, uint8_t *out, int32_t out_size);

void v5f_FacEncSetMA(fac_enc *ptr_fenc_s, const uint32_t *cnt, uint32_t sn, int32_t mn);

int32_t v5f_FacEncodeSymbolMA(fac_enc *ptr_fenc_s, uint32_t s);

int32_t v5f_FacEncFlush(fac_enc *ptr_fenc_s, BitWriter *ptr_bw);

int32_t v5f_FacEncEvalMA(uint8_t *dat, int32_t n, const uint32_t *cnt_tbl, uint32_t symbols,
                         int32_t avg_n);

void v5f_shift_scf(const BandCfg_t *band, int32_t *scf);

void v5f_moving_average_scf(const BandCfg_t *band, int32_t *in, int32_t *out);

void v5f_sns_smoothing(int32_t n, int32_t *in, int32_t *out);

void v5f_sns_apply(const BandCfg_t *band, int32_t *scf, float *cf, int32_t cfn);

int32_t v5f_sns_encode(LhdcEncContext *state);

kiss_fft_cfg_v5 v5f_lhdc_v5_fft_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem,
                                      kiss_fft_cpx *ptr_fft_twid);

void v5f_cmplx_multi(cc_buf_list *list, const float *coef, float *sum);

int32_t v5f_cal_frame_size_and_frames_in_packet(lhdcv5_para_t *p);

#endif /* LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_CODEC_API_H */
