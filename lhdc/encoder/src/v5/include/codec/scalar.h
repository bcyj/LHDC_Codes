/* Scalar research reconstruction, Android ARM32 ILP32 / AArch64 LP64 layouts.
 * These independent names are not a drop-in codec ABI or a complete encoder.
 */
#ifndef LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_SCALAR_H
#define LHDC_ENCODER_SRC_V5_INCLUDE_CODEC_SCALAR_H
#include <stdint.h>
#include <stddef.h>
#include "../../../../../common/include/compat/v5/lhdc_v5_constants.h"
#include "../types/lhdc_bit_writer_types.h"

typedef struct {
    uint16_t info;
    uint8_t ext_data[LHDC_V5_HEADER_EXT_BYTES];
    int32_t enc_frm_len_provided;
    uint8_t enc_frm_len_need_update;
    int32_t meta_data_loop_count;
} v5_frame_header;

/* Supplemental directory view using LHDCV5_Workspace content-index order. */
typedef struct {
    void *header;
    void *encoder;
    void *mdct;
    void *channels;
    void *arithmetic;
    void *bit_writer;
    void *encoded_ch0;
    void *encoded_ch1;
    void *mdct_twid1;
    void *mdct_twid2;
    void *window;
    void *fft_twid;
} v5_workspace_directory;
_Static_assert(offsetof(v5_workspace_directory, header) == 0 * sizeof(void *),
               "V5 directory slot 0");
_Static_assert(offsetof(v5_workspace_directory, encoder) == 1 * sizeof(void *),
               "V5 directory slot 1");
_Static_assert(offsetof(v5_workspace_directory, mdct) == 2 * sizeof(void *), "V5 directory slot 2");
_Static_assert(offsetof(v5_workspace_directory, channels) == 3 * sizeof(void *),
               "V5 directory slot 3");
_Static_assert(offsetof(v5_workspace_directory, arithmetic) == 4 * sizeof(void *),
               "V5 directory slot 4");
_Static_assert(offsetof(v5_workspace_directory, bit_writer) == 5 * sizeof(void *),
               "V5 directory slot 5");
_Static_assert(offsetof(v5_workspace_directory, encoded_ch0) == 6 * sizeof(void *),
               "V5 directory slot 6");
_Static_assert(offsetof(v5_workspace_directory, encoded_ch1) == 7 * sizeof(void *),
               "V5 directory slot 7");
_Static_assert(offsetof(v5_workspace_directory, mdct_twid1) == 8 * sizeof(void *),
               "V5 directory slot 8");
_Static_assert(offsetof(v5_workspace_directory, mdct_twid2) == 9 * sizeof(void *),
               "V5 directory slot 9");
_Static_assert(offsetof(v5_workspace_directory, window) == 10 * sizeof(void *),
               "V5 directory slot 10");
_Static_assert(offsetof(v5_workspace_directory, fft_twid) == 11 * sizeof(void *),
               "V5 directory slot 11");

void v5_bw_init(v5_bit_writer *bw, uint32_t *buf, uint32_t bytes);
int32_t v5_bw_write_uint(v5_bit_writer *bw, uint32_t val, uint32_t bits);
int32_t v5_bw_write_bytes(v5_bit_writer *bw, const uint8_t *bytes, int32_t len);
/* Emits at most four staged bytes; returns total length, leaves state unchanged. */
int32_t v5_bw_flush(v5_bit_writer *bw);
int32_t v5_process_header(v5_frame_header *hdr, int32_t ch, int32_t *usable, uint8_t *out);
void *v5_workspace_get(v5_workspace_directory *ws, uint32_t index);
int32_t v5_workspace_get_size(int32_t channels, int32_t *size);
int32_t v5_workspace_init(int32_t channels, void *workspace);
int32_t v5_set_version(int32_t version, v5_workspace_directory *workspace);
int32_t v5_set_jas_flag(int32_t flag, v5_workspace_directory *workspace);
int32_t v5_get_jas_flag(int32_t *flag, v5_workspace_directory *workspace);
int32_t v5_set_ar_flag(int32_t flag, v5_workspace_directory *workspace);
int32_t v5_get_ar_flag(int32_t *flag, v5_workspace_directory *workspace);
int32_t v5_set_larc_flag(int32_t flag, v5_workspace_directory *workspace);
int32_t v5_get_larc_flag(int32_t *flag, v5_workspace_directory *workspace);
int32_t v5_set_meta(int32_t flag, const uint8_t *data, int32_t loops,
                    v5_workspace_directory *workspace);
int32_t v5_get_meta(int32_t *flag, uint8_t *data, v5_workspace_directory *workspace);
int32_t v5_get_encoded_frame_size(int32_t *size, v5_workspace_directory *workspace);
#endif
