#include "../src/v4/include/llac_side_info.h"
#include "../../common/include/compat/v4/lhdc_v4_constants.h"
#include <stdio.h>
#include <string.h>

/* Write protocol fields backward from the packet tail to generate side-information fixtures independently of decoder code. */
static void put_field(uint8_t *bytes, unsigned *position, unsigned value, unsigned width) {
    for (unsigned bit = 0; bit < width; ++bit, ++*position)
        bytes[31 - *position / 8] |= ((value >> bit) & 1u) << (*position % 8);
}

/* Verify all four SNS modes and partial-write behavior when invalid fields cause early exit. */
int main(void) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        uint8_t bytes[32] = {0};
        unsigned position = 0;
        put_field(bytes, &position, 0, 3); /* lastnz=2, N=16. */
        put_field(bytes, &position, 0, 1);
        put_field(bytes, &position, 117, 8);
        put_field(bytes, &position, 0, 1); /* One TNS filter with order zero. */
        put_field(bytes, &position, 1, 1);
        put_field(bytes, &position, 3, 5);
        put_field(bytes, &position, 9, 5);
        put_field(bytes, &position, mode < 2 || mode == 4 ? 0 : 1, 1);
        put_field(bytes, &position, 1, mode < 2 || mode == 4 ? 1 : 2);
        put_field(bytes, &position, 1, 1);
        unsigned combined = mode == 0   ? 2u * LHDC_V4_LLAC_SNS_MPVQ_RADIX + 123u
                            : mode == 1 ? LHDC_V4_LLAC_SNS_MPVQ_RADIX + 123u
                            : mode == 2 ? 123u
                            : mode == 3 ? 0x00e74c00u + 247u
                                        : 0x01ffffffu;
        put_field(bytes, &position, combined, mode < 2 || mode == 4 ? 25 : 24);
        put_field(bytes, &position, 1, 1);
        put_field(bytes, &position, 231, 9);
        put_field(bytes, &position, 6, 3);
        LLACSnsIndices sns = {77, 77, 77, 77, 77, 77, 77};
        LLACLtpfIndices ltpf = {77, 77, 77};
        int mask = 0, cursor = 0, bfi = 0, gain = 0, noise = 77;
        LLACTnsData tns = {0, {77, 77}, {0}};
        int bandwidth = 0, lastnz = 0, lsb = 0;
        llac_read_side_information(bytes, 32, &mask, &cursor, 16, 0, 0, &bfi, &gain, &sns, &noise,
                                   &tns, &ltpf, &bandwidth, &lastnz, &lsb, 25);
        if (sns.low_codebook != 3 || sns.high_codebook != 9 || sns.leading_sign != 1 ||
            gain != 117 || tns.filter_count != 1 || lastnz != 2 || tns.order[0] != 0 ||
            tns.order[1] != 77)
            return 1;
        if (mode == 4) {
            if (bfi != 1 || sns.submode != 0 || sns.gain != 1 || sns.main_mpvq != 77 ||
                sns.secondary_mpvq != 77 || ltpf.present != 77 || ltpf.active != 77 ||
                ltpf.pitch != 77 || noise != 77)
                return 2;
        } else if (bfi != 0 || sns.submode != (int)mode || sns.main_mpvq != 123 ||
                   sns.gain != (mode == 1 || mode == 3 ? 3 : 1) ||
                   sns.secondary_mpvq != (mode == 0   ? 0
                                          : mode == 2 ? -1
                                                      : -2) ||
                   ltpf.present != 1 || ltpf.active != 1 || ltpf.pitch != 231 || noise != 6 ||
                   cursor != 31 - (int)(position / 8) || mask != (1 << (position % 8))) {
            return 3;
        }
    }
    puts("PASS LLAC typed side information: four modes and partial-write error contract");
    return 0;
}
