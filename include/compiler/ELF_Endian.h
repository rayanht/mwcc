#ifndef COMPILER_ELF_ENDIAN_H
#define COMPILER_ELF_ENDIAN_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct ConversionBlockEntry {
    unsigned int value;
    UInt16 shortValue;
    unsigned int trailingValue;
};
#pragma pack(pop)
struct ConversionBlockHeader {
    unsigned int size;
    unsigned int value;
};
struct ElfSymbolRecord {
    unsigned int name;
    unsigned int value;
    unsigned int size;
    unsigned char info;
    unsigned char other;
    unsigned short sectionIndex;
};
struct RecordBounds {
    unsigned int length;
    int endOffset;
};
extern void swap_tagged_records(unsigned char *data, unsigned int size, int (*errorHandler)(const char *, int),
                                char swapBeforeRead);
extern long double read_double(UInt8 *bytes, char flag);
extern void swap_conversion_blocks(char *data, int remainingSize, int (*conversionMode)(const char *, int),
                                   char readBeforeConversion);
extern void ELF_Endian_ConvertThreeWords(unsigned int *words);
extern unsigned short ELF_Endian_ConvertSymbolRecord(void *data);
extern void ELF_Endian_SwapElf32Section(Elf32Section *values);
extern void ELF_Endian_SwapElf32Header(struct Elf32Header *header);
/* DWARF 1 debugging entries: a tag, then attributes whose low four bits are their form */
typedef enum DwarfTag { TAG_padding = 0x0000, TAG_lo_user = 0x4080, TAG_hi_user = 0xffff } DwarfTag;
typedef enum DwarfAttribute {
    AT_location = 0x0023,
    AT_fund_type = 0x0055,
    AT_mod_fund_type = 0x0063,
    AT_user_def_type = 0x0072,
    AT_mod_u_d_type = 0x0083,
    AT_subscr_data = 0x00a3,
    AT_element_list = 0x00f4,
    /* (the block forms MWCC gives a virtual function's vtable location) */
    AT_pure_virtual = 0x0293,
    AT_return_addr = 0x02a3,
    AT_virtual = 0x0303,
    AT_MW_location = 0x2013
} DwarfAttribute;
extern int swap_tagged_record_data(void *data, DwarfAttribute kind, char readBeforeSwap);

#ifdef __cplusplus
}
#endif

#endif
