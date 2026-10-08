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
