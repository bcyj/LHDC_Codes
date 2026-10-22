#include "lhdc_memory.h"
#ifndef LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_API_H
#define LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_API_H
#include "../types/lhdc_native_types.h"
#include "../llac/spectral_coding.h"
#include "../../../../../ar/src/core/include/ar_interfaces.h"

LLAC_INT v4f_llac_encode_channels(LLAC_Enc *encoder, void **input, LLAC_UINT8 *output,
                                  LLAC_INT bps);

void v4f_llac_encode_residual_bits(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gain, LLAC_INT L_spec,
                                   LLAC_INT targetBits, LLAC_INT nBits, LLAC_UINT8 *resBits,
                                   LLAC_INT *numResBits, LLAC_INT hrmode);

void v4f_llac_write_side_information(LLAC_UINT8 *bytes, LLAC_INT *bp_side, LLAC_INT *mask_side,
                                     LLAC_INT numbytes, LLAC_INT bw_cutoff_bits,
                                     LLAC_INT bw_cutoff_idx, LLAC_INT lastnz, LLAC_INT N,
                                     LLAC_INT lsbMode, LLAC_INT gg_idx, const LLACTnsData *tns,
                                     const LLACLtpfIndices *ltpf, const LLACSnsIndices *sns,
                                     LLAC_INT fac_ns_idx);

void v4f_llac_dct4_transform(const Dct4 *dct, const LLAC_FLOAT *input, LLAC_FLOAT *output);

void v4f_llac_mdct_analyze_overlap(const LLAC_FLOAT *input, LLAC_FLOAT *output, Mdct *mdct);

void v4f_llac_mdct_analyze(const LLAC_FLOAT *in, LLAC_FLOAT *out, Mdct *mdctStruct);

void v4f_llac_band_mean_square(LLAC_INT bands_number, const LLAC_INT *acc_coeff_per_band,
                               LLAC_FLOAT *d2, LLAC_FLOAT *d);

void v4f_llac_estimate_bandwidth(LLAC_FLOAT *d2, LLAC_INT fs_idx, LLAC_INT frame_dms,
                                 LLAC_INT *bw_idx);

#if !(defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                         \
      (defined(_M_IX86) || defined(_M_X64)))
/* Provide at least one quarter of the maximum frame length for energy storage, disjoint from input/output state. */
void v4f_llac_estimate_quantization_gain_with_workspace(
    LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ, LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
    LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff, LLAC_FLOAT *targetBitsOff,
    LLAC_INT *old_targetBits, LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
    LLAC_FLOAT frame_ms, LLAC_FLOAT *energy_workspace);
int32_t v4f_kiss_fftri(kiss_fftr_cfg st, const kiss_fft_cpx *freqdata, float *timedata);
#endif

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
void v4f_llac_estimate_quantization_gain(LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ,
                                         LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
                                         LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff,
                                         LLAC_FLOAT *targetBitsOff, LLAC_INT *old_targetBits,
                                         LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
                                         LLAC_FLOAT frame_ms, LLAC_FLOAT *energy_workspace);
#else
void v4f_llac_estimate_quantization_gain(LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ,
                                         LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
                                         LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff,
                                         LLAC_FLOAT *targetBitsOff, LLAC_INT *old_targetBits,
                                         LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
                                         LLAC_FLOAT frame_ms);
int32_t v4f_kiss_fftri(kiss_fftr_cfg st, const kiss_fft_cpx *freqdata, float *timedata);
#endif

#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
/* Bind numeric policy during initialization; these internal entries neither query CPU nor change policy. */
void v4f_llac_estimate_bandwidth_with_policy(LLAC_FLOAT *d2, LLAC_INT fs_idx, LLAC_INT frame_dms,
                                             LLAC_INT *bw_idx, unsigned math_policy);
void v4f_llac_estimate_quantization_gain_with_policy(
    LLAC_FLOAT *x, LLAC_INT lg, LLAC_INT nbitsSQ, LLAC_FLOAT *gain, LLAC_INT *quantizedGain,
    LLAC_INT *quantizedGainMin, LLAC_INT quantizedGainOff, LLAC_FLOAT *targetBitsOff,
    LLAC_INT *old_targetBits, LLAC_INT old_specBits, LLAC_INT hrmode, LLAC_INT regBits,
    LLAC_FLOAT frame_ms, unsigned math_policy, LLAC_FLOAT *energy_workspace);
void v4f_llac_analyze_sns_with_policy(LLAC_FLOAT *x, LLAC_INT tilt, LLAC_INT xLen,
                                      LLAC_FLOAT *gains, LLAC_INT smooth, LLAC_FLOAT sns_damping,
                                      unsigned math_policy);
int32_t v4f_kiss_fftri(kiss_fftr_cfg st, const kiss_fft_cpx *freqdata, float *timedata);
#endif

void v4f_llac_adjust_quantization_gain(LLAC_INT *gg_idx, LLAC_INT gg_idx_min, LLAC_INT gg_idx_off,
                                       LLAC_FLOAT *gain, LLAC_INT target, LLAC_INT nBits,
                                       LLAC_INT *gainChange, LLAC_INT fs_idx);

void v4f_llac_estimate_noise_factor(LLAC_FLOAT *x, LLAC_INT *xq, LLAC_FLOAT gg,
                                    LLAC_INT freq_upper_bound, LLAC_INT frame_dms, LLAC_INT *nf_q);

void v4f_llac_detect_attack(EncSetup *state, LLAC_INT frame_size);

void v4f_llac_analyze_sns(LLAC_FLOAT *x, LLAC_INT tilt, LLAC_INT xLen, LLAC_FLOAT *gains,
                          LLAC_INT smooth, LLAC_FLOAT sns_damping);

void v4f_llac_dct2_transform(Dct2 *dct, const LLAC_FLOAT *input, LLAC_FLOAT *output);

void v4f_llac_quantize_sns(EncSetup *state);

void v4f_llac_interpolate_sns_gains(LLAC_FLOAT *gains, LLAC_INT encoder_side, LLAC_INT bands_number,
                                    LLAC_FLOAT *gains_int);

void v4f_llac_apply_sns_gains(LLAC_FLOAT *x, LLAC_FLOAT *scf, const LLAC_INT *bands_offset,
                              LLAC_INT fdns_npts);

/* Workspace must not overlap input spectrum/output indices; filter state resets on each call. */
void v4f_llac_analyze_and_apply_tns_with_workspace(LLAC_FLOAT *x, LLAC_INT bw_cutoff_idx,
                                                   LLAC_INT bw_fcbin, LLAC_INT fs, LLAC_INT N,
                                                   LLAC_INT frame_dms, LLAC_INT nBits,
                                                   LLACTnsData *tns, LLAC_INT *bits_out,
                                                   LhdcV4LlacTnsWorkspace *workspace);

IIS_FFT_ERROR v4f_llac_fft_plan_init(HANDLE_IIS_FFT *handle, LLAC_INT len, IIS_FFT_DIR sign,
                                     LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_fft_init(Fft *fft, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_fft_reset(Fft *fft);

void v4f_llac_dct2_init(Dct2 *dct, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_dct2_reset(Dct2 *dct);

void v4f_llac_dct4_init(Dct4 *dct, LLAC_INT length, LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_dct4_reset(Dct4 *dct);

void v4f_llac_mdct_init(Mdct *mdct, LLAC_INT length, LLAC_INT frame_dms, LLAC_INT hrmode,
                        LLAC_UINT8 *llac_enc_mem_addr);

void v4f_llac_mdct_reset(Mdct *mdct);

void *v4f_llac_workspace_alloc_aligned4(void *base, size_t *base_size, size_t size);

LLAC_INT v4f_llac_channel_workspace_init(LLAC_Enc *encoder, LLAC_INT channels);

void v4f_llac_configure_frame(LLAC_Enc *encoder, LLAC_UINT8 *llac_enc_mem_addr);

LLAC_Error v4f_llac_init_state(LLAC_Enc *encoder, LLAC_INT samplerate, LLAC_INT channels,
                               LLAC_UINT8 *llac_enc_mem_addr);

LLAC_Error v4f_llac_set_channel_budget(LLAC_Enc *encoder, int32_t bitrate);

void v4f_llac_enc_mem_alloc_fft_set(LLAC_INT ch, enum dct_type dct, LLAC_UINT8 *llac_enc_mem_addr);

LLAC_Error v4f_llac_init_checked(LLAC_Enc *encoder, int32_t samplerate, int32_t channels,
                                 uint8_t *llac_enc_mem_addr);

int32_t v4f_llac_target_bytes(const LLAC_Enc *encoder);

int32_t v4f_llac_target_bitrate(const LLAC_Enc *encoder);

LLAC_Error v4f_llac_set_bitrate_checked(LLAC_Enc *encoder, int32_t bitrate);

LLAC_Error v4f_llac_set_frame_duration(LLAC_Enc *encoder, float frame_ms,
                                       uint8_t *llac_enc_mem_addr);

LLAC_Error v4f_llac_set_high_resolution(LLAC_Enc *encoder, int32_t hrmode,
                                        uint8_t *llac_enc_mem_addr);

LLAC_Error v4f_llac_encode_checked(LLAC_Enc *encoder, void **input_samples, int32_t bitdepth,
                                   void *output_bytes, int32_t *num_bytes);

LLAC_Error v4f_llac_encode_s16(LLAC_Enc *encoder, int16_t **input_samples, void *output_bytes,
                               int32_t *num_bytes);

void v4f_ResetEncodeLPF(LhdcV4ResampleState *state);

void v4f_EncodeLPF(LhdcV4ResampleState *state, int32_t nCh, float *X, int32_t *pcm, int32_t N,
                   int32_t bits_per_sample);

void v4f_win_sinc_delay(float *X, int32_t *pcm, int32_t N);

void v4f_win_sinc_init(LhdcV4ResampleState *state, int32_t bits_per_sample);

int32_t v4f_lhdc_InitEncoder(struct lhdc_Encoder_s *encb, int32_t bits_per_sample,
                             int32_t block_size, int32_t one_frame_per_channle,
                             int32_t lhdc_ver_code);

void v4f_lpcV_Wintu(float *window, const int32_t L, const float p);

int32_t v4f_out_frame_hdr(struct lhdc_Encoder_s *encb, int32_t down_step, int32_t ch, uint8_t *out);

void v4f_byte_write_int32(int32_t dat, int32_t nb, uint8_t *out);

int32_t v4f_lz_make_data(uint32_t *rd2, int32_t ns, int32_t sh, int32_t nb, uint8_t *uc);

int32_t v4f_encode_residual_block(uint32_t *rd2, int32_t ns, uint32_t nb, uint8_t *out,
                                  int32_t out_len);

int32_t v4f_CalcResidual(struct lhdc_Encoder_s *encb, int32_t *dat, int32_t (*rd)[512],
                         long long unsigned int *min_total, int32_t *num_unencoded,
                         LhdcV4LpcResult *lpc);

uint32_t v4f_lpcV_Cares(struct lhdc_Encoder_s *encb, int32_t *dat, int32_t *rd,
                        long long unsigned int *min_total, int32_t *num_unencoded,
                        LhdcV4LpcResult *lpc);

int32_t v4f_eval_rice_encode_len(uint32_t *rd2, int32_t ns, uint32_t nb);

int32_t v4f_block_encode(struct lhdc_Encoder_s *encb, int32_t *num_unencoded, int32_t **rd,
                         int32_t ch0, int32_t ch1, uint8_t *out, int32_t out_len, int32_t do_eval);

void v4f_applyQuantization(struct lhdc_Encoder_s *encb, int32_t *src, int32_t *dst, int32_t div,
                           int32_t down_step);

int32_t v4f_lhdc_Encode(struct lhdc_Encoder_s *encb, int32_t **samples, int32_t *btr,
                        int32_t *btr_ext, int32_t down_step, uint8_t *out, int32_t out_len,
                        int32_t do_eval, int32_t min_len);

struct lhdc_Encoder_s *v4f_lhdc_NewEncoder_with_allocator(const LhdcAllocator *allocator);
size_t v4f_llac_enc_workspace_bytes(void);
LLAC_UINT8 *v4f_llac_enc_mem_alloc_with_allocator(const LhdcAllocator *allocator);

void v4f_lhdc_FreeEncoder(struct lhdc_Encoder_s *encb);

void v4f_lpcV_window_rectangle(float *window, const int32_t L);

void v4f_lpcV_window_hann(float *window, const int32_t L);

void v4f_lpcV_Calpcor(const float *autoc, uint32_t *max_order, LhdcV4LpcWorkspace *workspace);

float v4f_lpcV_CabRer(float lpc_error, float error_scale);

uint32_t v4f_lpcV_Calpco(const float *lpc_error, uint32_t max_order, uint32_t total_samples,
                         uint32_t overhead_bits_per_order);

int32_t v4f_lround(float x);

int32_t v4f_lpcV_Quco(const float *lp_coeff, uint32_t order, uint32_t precision, int32_t *qlp_coeff,
                      int32_t *shift);

void v4f_lpcV_Carcow(const int32_t *data, uint32_t data_len, const int32_t *qlp_coeff,
                     uint32_t order, int32_t lp_quantization, int32_t *residual);

bool v4f_lhdc_GetExtFuncState(struct lhdc_Encoder_s *encb, lhdc_ext_func_t ext_func);

void v4f_lhdc_ExtFuncArEnable(struct lhdc_Encoder_s *encb, int32_t enable_ar);

void v4f_lhdc_ExtFuncJasEnable(struct lhdc_Encoder_s *encb, int32_t enable_Jas);

void v4f_lhdc_ExtFuncMetaEnable(struct lhdc_Encoder_s *encb, int32_t enable_meta,
                                uint8_t *pmeta_data, int32_t meta_data_len,
                                int32_t conti_frame_cnt);

void v4f_lhdc_DecMetaFrameCnt(struct lhdc_Encoder_s *encb);

kiss_fft_cfg_v4 v4f_kiss_fft_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem);

void v4f_kiss_fft_stride(kiss_fft_cfg_v4 st, const kiss_fft_cpx *fin, kiss_fft_cpx *fout,
                         int32_t in_stride);

void v4f_kiss_fft(kiss_fft_cfg_v4 cfg, const kiss_fft_cpx *fin, kiss_fft_cpx *fout);

kiss_fftr_cfg v4f_kiss_fftr_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem);

int32_t v4f_kiss_fftr(kiss_fftr_cfg st, const float *timedata, kiss_fft_cpx *freqdata);

void v4f_RestoreDefQualityPreset(FFT_BLOCK *fb);

void v4f_SetQuality(FFT_BLOCK *fb, int32_t quality);

int32_t v4f_Get2ChWavData16(FFT_BLOCK *fb, uint8_t *wav, int32_t ns);

int32_t v4f_Get2ChWavData24(FFT_BLOCK *fb, uint8_t *wav, int32_t ns);

int32_t v4f_EncodeBlocks(FFT_BLOCK *fb, int32_t ns, int32_t final, int32_t *btr, int32_t *btr_ext,
                         uint8_t *enc_buf, int32_t out_len, int32_t do_eval);

int32_t v4f_BTRtoDIV(int32_t btr, int32_t btr_ext);

void v4f_DIVtoBTR(int32_t div, int32_t *btr, int32_t *btr_ext);

int32_t v4f_DoEncode(FFT_BLOCK *fb, int32_t ns, int32_t final, uint8_t *out, int32_t out_len);

int32_t v4f_CalcBTR(float noise_floor, float ms, int32_t *btr, int32_t *btr_ext, int32_t min_btr);

int32_t v4f_LossyEncoderProcess(FFT_BLOCK *fb, int32_t ns, int32_t final, uint8_t *out,
                                int32_t out_len);

void V4_LOCAL_FLOAT_CC v4f_init_window(float *H, float FC);

int32_t v4f_lhdc_encoder_encodeV3(lhdc_para_t *handle, void *p_pcm, uint8_t *out_put,
                                  uint32_t *written, uint32_t *out_frames);

void v4f_cal_frame_size_and_frames_in_packet(lhdc_para_t *handle);

/* Each output has at least dim entries; dimension 1..16, pulses 1..32767.
 * Invalid ranges/null pointers cause no output access. Degenerate directions use a canonical single pulse within the valid dimension. */
void v4f_llac_pvq_search(LLAC_FLOAT *x_in, LLAC_INT dim, LLAC_INT pulses, LLAC_INT *y,
                         LLAC_FLOAT *y_en1_norm);

void v4f_kf_work(kiss_fft_cpx *Fout, const kiss_fft_cpx *f, const size_t fstride, int32_t in_stride,
                 int32_t *factors, const kiss_fft_cfg_v4 st);

int32_t v4f_kiss_fftri(kiss_fftr_cfg st, const kiss_fft_cpx *freqdata, float *timedata);
#endif /* LHDC_ENCODER_SRC_V4_INCLUDE_CODEC_CODEC_API_H */
