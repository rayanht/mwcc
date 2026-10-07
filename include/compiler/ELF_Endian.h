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
    unsigned int name;           /* 0x00: ELF_Endian_ConvertSymbolRecord converts symbol name index */
    unsigned int value;          /* 0x04: ELF_Endian_ConvertSymbolRecord converts symbol value */
    unsigned int size;           /* 0x08: ELF_Endian_ConvertSymbolRecord converts symbol size */
    unsigned char info;          /* 0x0c: ELF_Endian_ConvertSymbolRecord leaves ELF symbol info byte unchanged */
    unsigned char other;         /* 0x0d: ELF_Endian_ConvertSymbolRecord leaves ELF symbol other byte unchanged */
    unsigned short sectionIndex; /* 0x0e: ELF_Endian_ConvertSymbolRecord converts and returns section index */
};
struct RecordBounds {
    int length;
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
extern int swap_tagged_record_data(void *data, short kind, char readBeforeSwap);

#ifdef __cplusplus
}
#endif

#endif
