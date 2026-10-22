#include "../include/llac_bit_budget.h"
#include "../../../../common/include/types/llac_scalar_types.h"
#include "../../../../common/include/compat/v4/lhdc_v4_constants.h"
#include "../include/llac_spectral.h"
#include "../include/llac_semantic_records.h"
#include "tables/llac_decode_keys.h"
/* Pair model frequency rows with encrypted cumulative boundaries; preserve every word's bit pattern. */
#include "../../../../../math/include/audio_math/integer.h"
#include <stdbool.h>
#include <string.h>
#include "spectral_tables.h"

/* Locate a symbol in monotonic cumulative probabilities using signed comparisons; equality selects the right interval.
 * The read-only model decodes boundaries with one XOR key; zero is the first symbol and the last includes values beyond its final boundary.
 */
static int find_arithmetic_symbol(uint32_t value, const uint32_t *cdf, uint32_t key,
                                  unsigned symbol_count) {
    unsigned first = 0, last = symbol_count;
    while (first + 1 < last) {
        unsigned middle = first + (last - first) / 2;
        if (audio_math_int32_from_bits(value) < audio_math_int32_from_bits(cdf[middle] ^ key))
            last = middle;
        else
            first = middle;
    }
    return (int)first;
}

/* Weight absolute magnitudes by spectral position and return the sign-extended low 16 bits as the noise seed.
 * Unsigned absolute values cover INT32_MIN; multiply-add wraps modulo 2^32 without assuming HR intermediate-sum bounds. */
static int32_t spectral_noise_seed(const int32_t *spectrum, int length) {
    uint32_t weighted_magnitude = 0;
    int i = 0;
#if defined(__arm__) &&                                                                            \
    (defined(__OPTIMIZE_SIZE__) || (!defined(__ARM_NEON) && !defined(__ARM_NEON__)))
    /* ARM32 scalar/size-optimized builds reduce branches and split multiply-add dependencies without requiring a newer ISA.
     * NEON speed builds retain the vectorizable loop below. Partial sums combine modulo 2^32. */
    uint32_t weighted_groups[4] = {0};
    for (; length - i >= 4; i += 4) {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
        for (unsigned lane = 0; lane < 4; ++lane) {
            int32_t value = spectrum[i + lane];
            uint32_t sign = audio_math_sign_mask_i32(value);
            uint32_t magnitude = ((uint32_t)value ^ sign) - sign;
            weighted_groups[lane] += ((uint32_t)i + lane) * magnitude;
        }
    }
    weighted_magnitude =
        weighted_groups[0] + weighted_groups[1] + weighted_groups[2] + weighted_groups[3];
#endif
    for (; i < length; ++i) {
        uint32_t magnitude = (uint32_t)spectrum[i];
        if (spectrum[i] < 0)
            magnitude = 0u - magnitude;
        weighted_magnitude += (uint32_t)i * magnitude;
    }
    return audio_math_int32_from_bits(
        audio_math_shift_arithmetic_low8_sign32(weighted_magnitude << 16, 16));
}

/* Read one backward side-information bit and advance the cursor. */
static inline unsigned read_side_bit(LLACSideReader *side) {
    unsigned bit = (side->bytes[side->byte_position] & side->mask) != 0;
    if (side->mask == 0x80) {
        --side->byte_position;
        side->mask = 1;
    } else {
        side->mask <<= 1;
    }
    return bit;
}

/* Decode a symbol from cumulative probabilities and refill bytes to normalize the arithmetic range. */
static unsigned decode_arithmetic_symbol(LLACArithmeticReader *reader, const uint32_t *frequency,
                                         const uint32_t *cdf, uint32_t key, unsigned count) {
    uint32_t unit = reader->range >> 10;
    reader->exhausted |= (reader->range & 0xfffffc00u) <= reader->low;
    unsigned symbol = (unsigned)find_arithmetic_symbol(reader->low / unit, cdf, key, count);
    reader->low -= unit * (cdf[symbol] ^ key);
    reader->range = frequency[symbol] * unit;
    while (reader->range < 0x10000) {
        reader->range <<= 8;
        reader->low = (reader->low & 0xffffu) * 0x100u + reader->bytes[reader->byte_position];
        ++reader->byte_position;
    }
    return symbol;
}

/* Decode TNS orders/reflection coefficients in filter order, retaining written coefficients on failure. */
static bool decode_tns_filters(LLACArithmeticReader *reader, int weighting, LLACTnsData *tns) {
    const LLACOrderModel *model = &lhdc_decoder_v4_llac_spectral_tns_order_models[weighting];
    for (int filter = 0; filter < tns->filter_count; ++filter) {
        if (tns->order[filter] <= 0)
            continue;
        tns->order[filter] =
            (int)decode_arithmetic_symbol(reader, model->frequency, model->cumulative.by_symbol,
                                          decode_keys.tns_order_cdf, 8) +
            1;
        for (int coefficient = 0; coefficient < tns->order[filter]; ++coefficient) {
            const LLACReflectionModel *reflection =
                &lhdc_decoder_v4_llac_spectral_tns_reflection_models[coefficient];
            tns->reflection_indices[filter * 8 + coefficient] = (int)decode_arithmetic_symbol(
                reader, reflection->frequency, reflection->cumulative.by_symbol,
                decode_keys.tns_reflection_cdf, 17);
        }
    }
    return !reader->exhausted;
}

/* Decode a spectral pair through escape levels, read signs, and return the next pair's context increment.
 * The first four levels switch models; later levels reuse the fourth. LSB mode defers only level-zero's two bits. */
static bool decode_spectral_pair(LLACArithmeticReader *reader, LLACSideReader *side,
                                 unsigned context, int lsb_mode, unsigned maximum_level, int *pair,
                                 int *saved_level, unsigned *next_context) {
    uint32_t magnitude_low = 0, magnitude_high = 0;
    unsigned level = 0;
    for (;;) {
        unsigned model_level = level < 3 ? level : 3;
        unsigned model_index =
            lhdc_decoder_v4_llac_spectral_spectral_context_models[context + model_level * 0x400] ^
            decode_keys.spectral_context;
        const LLACPairModel *model =
            &lhdc_decoder_v4_llac_spectral_spectral_pair_models[model_index];
        unsigned symbol = decode_arithmetic_symbol(
            reader, model->frequency, model->cumulative.by_symbol, decode_keys.spectral_cdf, 17);
        if (symbol != 16) {
            magnitude_low += (symbol & 3u) << level;
            magnitude_high += (symbol >> 2) << level;
            *saved_level = (int)level;
            *next_context =
                level < 2 ? (level + 1) * ((symbol & 3u) + (symbol >> 2)) + 1 : model_level + 12;
            break;
        }
        if (level >= maximum_level)
            return false;
        if (level != 0 || lsb_mode == 0) {
            magnitude_low += read_side_bit(side) << level;
            magnitude_high += read_side_bit(side) << level;
        }
        ++level;
    }
    if (magnitude_low != 0 && read_side_bit(side))
        magnitude_low = 0u - magnitude_low;
    if (magnitude_high != 0 && read_side_bit(side))
        magnitude_high = 0u - magnitude_high;
    pair[0] = audio_math_int32_from_bits(magnitude_low);
    pair[1] = audio_math_int32_from_bits(magnitude_high);
    return reader->byte_position - side->byte_position <= 3 && !reader->exhausted;
}

/* Pack normal-mode residual bits, limited by nonzero spectral lines and the HR per-line budget. */
static void read_spectral_residual_bits(LLACSideReader *side, const int *spectrum, int length,
                                        int hrmode, int *remaining_bits, uint8_t *residual_bits) {
    int nonzero = 0;
    for (int i = 0; i < length; ++i)
        nonzero += spectrum[i] != 0;
    int limit = hrmode ? nonzero * 20 : nonzero;
    if (limit < *remaining_bits)
        *remaining_bits = limit;
    memset(residual_bits, 0, 625 * sizeof *residual_bits);
    for (int bit = 0; bit < *remaining_bits; ++bit)
        residual_bits[bit >> 3] |= (uint8_t)(read_side_bit(side) << (bit & 7));
}

/* Consume an LSB correction and, if needed, a zero-line sign bit; preserve the original value when budget expires. */
static bool refine_spectral_coefficient(LLACSideReader *side, int *coefficient, int *remaining) {
    if (*remaining == 0)
        return false;
    unsigned present = read_side_bit(side);
    --*remaining;
    if (!present)
        return true;
    if (*coefficient == 0) {
        if (*remaining == 0)
            return false;
        *coefficient = read_side_bit(side) ? -1 : 1;
        --*remaining;
    } else {
        *coefficient += *coefficient < 0 ? -1 : 1;
    }
    return true;
}

/* Restore low bits only for escaped spectral pairs, consuming remaining budget in line order. */
static void refine_spectral_pairs(LLACSideReader *side, int lastnz, const int *saved_levels,
                                  int *spectrum, int *remaining) {
    for (int i = 0; i < lastnz; i += 2) {
        if (saved_levels[i] <= 0)
            continue;
        if (!refine_spectral_coefficient(side, spectrum + i, remaining) ||
            !refine_spectral_coefficient(side, spectrum + i + 1, remaining))
            break;
    }
}

/* Decode TNS, spectrum, and residuals in order, then generate the noise seed and zero-frame flag.
 * Stages share explicit cursors; errors retain consumed input and partial spectral writes. */
void llac_decode_spectral_symbols(LLAC_UINT8 *bytes, LLAC_INT bp_side, LLAC_INT mask_side,
                                  LLAC_INT L_spec, LLAC_INT fs_idx, LLAC_INT enable_lpc_weighting,
                                  LLACTnsData *tns, LLAC_INT lsbMode, LLAC_INT lastnz,
                                  LLAC_INT *bfi, LLAC_INT fac_ns_idx, LLAC_INT gg_idx,
                                  uint8_t *resBits, LLAC_INT *x, LLAC_INT *nf_seed,
                                  LLAC_INT *zero_frame, LLAC_INT numbytes, LLAC_INT *nbits_residual,
                                  LLAC_INT *residualPresent, LLAC_INT *save_lev, LLAC_INT hrmode) {
    const int frame_bits = numbytes * 8;
    unsigned rate_context = fs_idx == 5 || frame_bits <= (fs_idx + 1) * 160 ? 0 : 0x200;
    LLACArithmeticReader reader = {
        bytes, 0xffffff,
        (uint32_t)bytes[2] + ((uint32_t)bytes[1] + (uint32_t)bytes[0] * 0x100) * 0x100, 3, 0};
    LLACSideReader side = {bytes, bp_side, (uint32_t)mask_side};
    memset(save_lev, 0, 240 * sizeof *save_lev);
    if (!decode_tns_filters(&reader, enable_lpc_weighting, tns)) {
        *bfi = 1;
        return;
    }
    unsigned previous_context = 0;
    unsigned maximum_level =
        hrmode ? LHDC_V4_LLAC_MAX_ESCAPE_LEVEL_HR : LHDC_V4_LLAC_MAX_ESCAPE_LEVEL_NORMAL;
    for (int i = 0; i < lastnz; i += 2) {
        unsigned context = rate_context + previous_context + (L_spec >> 1 < i ? 0x100u : 0u);
        unsigned next_context;
        int level = (int)maximum_level + 1;
        bool valid = decode_spectral_pair(&reader, &side, context, lsbMode, maximum_level, x + i,
                                          &level, &next_context);
        /* A top-level escape writes neither spectrum nor save_lev; other arithmetic errors retain decoded results. */
        if (level <= (int)maximum_level && lsbMode == 1)
            save_lev[i] = level;
        if (!valid) {
            *bfi = 1;
            return;
        }
        previous_context = (previous_context & 15u) * 16u + next_context;
    }
    *nbits_residual = llac_spectral_residual_budget(&reader, &side, frame_bits);
    if (*nbits_residual < 0) {
        *bfi = 1;
        return;
    }
    if (lsbMode == 0) {
        *residualPresent = 1;
        read_spectral_residual_bits(&side, x, L_spec, hrmode, nbits_residual, resBits);
    } else {
        refine_spectral_pairs(&side, lastnz, save_lev, x, nbits_residual);
    }
    *nf_seed = spectral_noise_seed(x, L_spec);
    *zero_frame = lastnz == 2 && x[0] == 0 && x[1] == 0 && gg_idx == 0 && fac_ns_idx == 7;
}
