

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../include/codec/codec_internal.h"
#include <math.h>

/* File-local data declarations before pointer initializers. */
static const int32_t v5i_band_off_480_HR[33];

static const int32_t v5i_band_scale_480_HR[32];

static const int32_t v5i_band_off_480[33];

static const int32_t v5i_band_scale_480[32];

static const int32_t v5i_band_off_480_LB[25];

static const int32_t v5i_band_scale_480_LB[24];

static const int32_t v5i_band_off_960[33];

static const int32_t v5i_band_scale_960[32];

static const int32_t v5i_band_off_1920[33];

static const int32_t v5i_band_scale_1920[32];

static const int32_t v5i_band_off_480_HR[33] = {

    0,  2,  4,  6,  8,  10,  12,  14,  16,  18,  20,  22,  24,  28,  32,  37, 43,
    50, 58, 68, 79, 92, 107, 124, 144, 168, 195, 226, 263, 306, 355, 413, 480};

static const int32_t v5i_band_scale_480_HR[32] = {
    67108864,  67108864,  67108864,  67108864,  67108864,   67108864,   67108864,   67108864,
    67108864,  67108864,  67108864,  67108864,  47396895,   66949894,   75655391,   89055067,
    107823107, 133266164, 150594768, 193382512, 231135434,  282955815,  352663864,  423428306,
    498423650, 625815217, 769924588, 911188351, 1107494116, 1372820649, 1638256461, 2003249672};

static const int32_t v5i_band_off_480[33] = {

    0,  2,  4,  6,  8,  10, 12,  14,  16,  18,  20,  22,  24,  28,  32,  36, 42,
    48, 56, 64, 74, 85, 98, 112, 130, 149, 172, 198, 228, 262, 302, 348, 400};

static const int32_t v5i_band_scale_480[32] = {

    134217728, 134217728, 134217728, 134217728, 134217728,  134217728,  134217728,  134217728,
    134217728, 134217728, 134217728, 134217728, 89491108,   119338311,  159140196,  141477908,
    188663822, 188690438, 251622743, 268435456, 325422212,  367194803,  454686273,  471593025,
    595780490, 656314565, 774223823, 894784853, 1052836570, 1193383111, 1383827787, 1632437396};

static const int32_t v5i_band_off_480_LB[25] = {

    0,  3,  6,  9,  12,  15,  18,  21,  24,  28,  33,  39, 45,
    54, 63, 74, 87, 103, 121, 142, 167, 196, 231, 272, 320};

static const int32_t v5i_band_scale_480_LB[24] = {

    178956971, 178956971, 178956971, 178956971, 178956971, 178956971, 178956971, 178956971,
    178982217, 190941298, 212186927, 282955815, 251551763, 335449667, 365996716, 412977625,
    447455542, 530392494, 606248364, 679093957, 780677884, 862584857, 981942410, 1118481067};

static const int32_t v5i_band_off_960[33] = {

    0,   4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  56,  64,  74, 86,
    100, 116, 136, 158, 184, 214, 248, 288, 336, 390, 452, 526, 612, 710, 826, 960};

static const int32_t v5i_band_scale_960[32] = {
    67108864,  67108864,  67108864,  67108864,  67108864,   67108864,   67108864,   67108864,
    67108864,  67108864,  67108864,  67108864,  47396895,   66949894,   75655391,   89055067,
    107823107, 133266164, 150594768, 193382512, 231135434,  282955815,  352663864,  423428306,
    498423650, 625815217, 769924588, 911188351, 1107494116, 1372820649, 1638256461, 2003249672};

static const int32_t v5i_band_off_1920[33] = {

    0,   8,   16,  24,  32,  40,  48,  56,  64,  72,  80,  88,   96,   112,  128,  148, 172,
    200, 232, 272, 316, 368, 428, 496, 576, 672, 780, 904, 1052, 1224, 1420, 1652, 1920};

static const int32_t v5i_band_scale_1920[32] = {

    62914560,  62914560,  62914560,  62914560,  62914560,   62914560,   62914560,   62914560,
    62914560,  62914560,  62914560,  62914560,  44434589,   62765525,   70926928,   83489124,
    101084162, 124937028, 141182594, 181296105, 216689469,  265271076,  330622372,  396964037,
    467272171, 586701765, 721804300, 854239078, 1038275733, 1287019358, 1535865432, 1878046567};

const BandStop_t band_stop[18] = {

    {.frm_len = 50, .stop = 120},  {.frm_len = 60, .stop = 120},  {.frm_len = 68, .stop = 120},
    {.frm_len = 72, .stop = 140},  {.frm_len = 76, .stop = 160},  {.frm_len = 80, .stop = 400},
    {.frm_len = 84, .stop = 400},  {.frm_len = 88, .stop = 400},  {.frm_len = 92, .stop = 480},
    {.frm_len = 96, .stop = 480},  {.frm_len = 100, .stop = 480}, {.frm_len = 120, .stop = 480},
    {.frm_len = 140, .stop = 480}, {.frm_len = 160, .stop = 600}, {.frm_len = 180, .stop = 840},
    {.frm_len = 200, .stop = 840}, {.frm_len = 220, .stop = 960}, {.frm_len = 1000, .stop = 1920}};

const BandCfg_t band_480_HR = {

    .n = 32,
    .inv = 67108864,
    .off = (const int32_t *)v5i_band_off_480_HR,
    .scale = (const int32_t *)v5i_band_scale_480_HR,
    .scale_shift_bits = 27,
    .scale_down = 773094113,
    .rice_min_p0 = 80,
    .ac_ma0 = 57,
    .ac_ma1 = 63};

const BandCfg_t band_480 = {

    .n = 32,
    .inv = 67108864,
    .off = (const int32_t *)v5i_band_off_480,
    .scale = (const int32_t *)v5i_band_scale_480,
    .scale_shift_bits = 28,
    .scale_down = 773094113,
    .rice_min_p0 = 80,
    .ac_ma0 = 57,
    .ac_ma1 = 63};

const BandCfg_t band_480_LB = {

    .n = 24,
    .inv = 89478485,
    .off = (const int32_t *)v5i_band_off_480_LB,
    .scale = (const int32_t *)v5i_band_scale_480_LB,
    .scale_shift_bits = 29,
    .scale_down = 773094113,
    .rice_min_p0 = 80,
    .ac_ma0 = 57,
    .ac_ma1 = 63};

const BandCfg_t band_960 = {

    .n = 32,
    .inv = 67108864,
    .off = (const int32_t *)v5i_band_off_960,
    .scale = (const int32_t *)v5i_band_scale_960,
    .scale_shift_bits = 28,
    .scale_down = 966367641,
    .rice_min_p0 = 160,
    .ac_ma0 = 96,
    .ac_ma1 = 64};

const BandCfg_t band_1920 = {

    .n = 32,
    .inv = 67108864,
    .off = (const int32_t *)v5i_band_off_1920,
    .scale = (const int32_t *)v5i_band_scale_1920,
    .scale_shift_bits = 25,
    .scale_down = 966367641,
    .rice_min_p0 = 320,
    .ac_ma0 = 96,
    .ac_ma1 = 64};
