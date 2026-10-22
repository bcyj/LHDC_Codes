#include "lzh_encode.h"
#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

/* Select dictionary slots from high bits of a little-endian four-byte multiplicative hash to reduce low-bit collisions. */
static unsigned hash_prefix(const uint8_t *input) {
    uint32_t word = (uint32_t)input[0] | (uint32_t)input[1] << 8 | (uint32_t)input[2] << 16 |
                    (uint32_t)input[3] << 24;
    return (word * LHDC_V4_LZH_HASH_MULTIPLIER) >> (32u - LHDC_V4_LZH_HASH_BITS);
}

/* Record the current position; the next same-slot lookup considers only the latest candidate. */
static void insert_position(LhdcV4LzhWorkspace *workspace, const uint8_t *input,
                            unsigned position) {
    workspace->positions[hash_prefix(input + position)] = (uint16_t)(position + 1u);
}

/* Find the first differing byte of a nonzero XOR word in memory byte order. */
static unsigned first_different_byte(uint64_t difference) {
#if defined(_MSC_VER)
    unsigned long bit;
#if defined(_WIN64)
    _BitScanForward64(&bit, difference);
    return (unsigned)bit / 8u;
#else
    if ((uint32_t)difference) {
        _BitScanForward(&bit, (uint32_t)difference);
        return (unsigned)bit / 8u;
    }
    _BitScanForward(&bit, (uint32_t)(difference >> 32));
    return 4u + (unsigned)bit / 8u;
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (unsigned)__builtin_ctzll(difference) / 8u;
#else
    return (unsigned)__builtin_clzll(difference) / 8u;
#endif
#else
    uint8_t bytes[sizeof difference];
    memcpy(bytes, &difference, sizeof bytes);
    unsigned index = 0;
    while (!bytes[index])
        ++index;
    return index;
#endif
}

/* Extend a confirmed four-byte prefix with bounded wide comparisons and byte-tail comparisons. */
static unsigned match_length(const uint8_t *input, const uint8_t *reference, unsigned limit) {
    unsigned length = LHDC_V4_LZH_MIN_MATCH;
    while (length + sizeof(uint64_t) <= limit) {
        uint64_t current, previous;
        memcpy(&current, input + length, sizeof current);
        memcpy(&previous, reference + length, sizeof previous);
        if (current != previous)
            return length + first_different_byte(current ^ previous);
        length += sizeof(uint64_t);
    }
    while (length < limit && input[length] == reference[length])
        ++length;
    return length;
}

/* Compute literal-run overhead including empty-reference separators at 255-byte boundaries. */
static unsigned literal_cost(unsigned length) {
    return length ? length + 1u + ((length - 1u) / LHDC_V4_LZH_LITERAL_LIMIT) * 2u : 1u;
}

/* Copy literal segments, inserting zero-length references between long runs to preserve alternating block types. */
static uint8_t *write_literals(uint8_t *output, const uint8_t *input, unsigned length) {
    while (length > LHDC_V4_LZH_LITERAL_LIMIT) {
        *output++ = LHDC_V4_LZH_LITERAL_LIMIT;
        memcpy(output, input, LHDC_V4_LZH_LITERAL_LIMIT);
        output += LHDC_V4_LZH_LITERAL_LIMIT;
        *output++ = 0;
        input += LHDC_V4_LZH_LITERAL_LIMIT;
        length -= LHDC_V4_LZH_LITERAL_LIMIT;
    }
    *output++ = (uint8_t)length;
    memcpy(output, input, length);
    return output + length;
}

/* Share encoding/length estimation with deferred literals, sparse match dictionary updates, and retained final position. */
static uint32_t encode_or_measure(LhdcV4LzhWorkspace *workspace, const uint8_t *input,
                                  uint32_t size, uint8_t *output, uint32_t capacity) {
    if (!workspace || !input || size < LHDC_V4_LZH_MIN_MATCH || size > LHDC_V4_LZH_INPUT_LIMIT)
        return 0;
    memset(workspace, 0, sizeof *workspace);
    unsigned position = 0, anchor = 0, written = 0;
    /* Keep three trailing bytes so every inserted match position has a readable four-byte prefix. */
    while (position + LHDC_V4_LZH_MIN_MATCH + 3u <= size) {
        unsigned slot = hash_prefix(input + position);
        unsigned candidate = workspace->positions[slot];
        workspace->positions[slot] = (uint16_t)(position + 1u);
        unsigned length = 0, distance = 0;
        unsigned limit = size - position - 3u;
        if (limit > LHDC_V4_LZH_MAX_MATCH)
            limit = LHDC_V4_LZH_MAX_MATCH;
        if (candidate) {
            unsigned reference = candidate - 1u;
            if (reference < position && position - reference <= LHDC_V4_LZH_MAX_DISTANCE &&
                memcmp(input + position, input + reference, LHDC_V4_LZH_MIN_MATCH) == 0) {
                length = match_length(input + position, input + reference, limit);
                distance = position - reference;
            }
        }
        if (!length) {
            ++position;
            continue;
        }
        unsigned cost = literal_cost(position - anchor) + (distance < 255u ? 2u : 3u);
        if (cost > capacity - written)
            return 0;
        if (output) {
            uint8_t *cursor = write_literals(output + written, input + anchor, position - anchor);
            *cursor++ = (uint8_t)length;
            if (distance < 255u)
                *cursor++ = (uint8_t)distance;
            else {
                *cursor++ = 255;
                *cursor++ = (uint8_t)(distance - 255u);
            }
        }
        written += cost;
        unsigned end = position + length;
        for (unsigned next = position + LHDC_V4_LZH_INSERT_STRIDE; next < end;
             next += LHDC_V4_LZH_INSERT_STRIDE)
            insert_position(workspace, input, next);
        if ((length - 1u) % LHDC_V4_LZH_INSERT_STRIDE)
            insert_position(workspace, input, end - 1u);
        position = end;
        anchor = position;
    }
    unsigned cost = literal_cost(size - anchor);
    if (cost > capacity - written)
        return 0;
    if (output)
        write_literals(output + written, input + anchor, size - anchor);
    return written + cost;
}

/* Write a short-distance LZ byte stream using caller workspace; reject null output. */
uint32_t v4f_lzh_enc(LhdcV4LzhWorkspace *workspace, const uint8_t *input, uint32_t size,
                     uint8_t *output, uint32_t capacity) {
    return output ? encode_or_measure(workspace, input, size, output, capacity) : 0;
}

/* Compute exact encoded length using the real path, matching successful writes at equal capacity. */
uint32_t v4f_lzh_enc_eval(LhdcV4LzhWorkspace *workspace, const uint8_t *input, uint32_t size,
                          uint32_t capacity) {
    return encode_or_measure(workspace, input, size, NULL, capacity);
}
