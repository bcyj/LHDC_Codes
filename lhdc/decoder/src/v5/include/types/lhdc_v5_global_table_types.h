
#ifndef LHDC_DECODER_SRC_V5_INCLUDE_TYPES_LHDC_V5_GLOBAL_TABLE_TYPES_H
#define LHDC_DECODER_SRC_V5_INCLUDE_TYPES_LHDC_V5_GLOBAL_TABLE_TYPES_H
typedef unsigned char undefined;

typedef unsigned char byte;
typedef unsigned int dword;
typedef long long longlong;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef unsigned char undefined1;
typedef unsigned int undefined4;
typedef unsigned short ushort;
typedef unsigned short word;

enum segment_setting_table_index_special_case_enum_values {
    SEGMENT_480_HR = 480,
    SEGMENT_480 = 481,
    SEGMENT_480_LB = 482
};

typedef struct segment_cutoff_struct segment_cutoff_struct;

struct segment_cutoff_struct {
    int segment_size;
    int segment_cuteoff;
};

#endif
