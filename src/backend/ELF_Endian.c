#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/ELF_Endian.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/Coloring.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/libimp-eabi-ppc.h"
#include <string.h>
#include <setjmp.h>

static int (*DAT_005805e0)(const char *, int);
/* inferred floating type; verify ABI */

static inline void readhdr(volatile int *out, unsigned int *p, char flag)
{
    if (flag) {
        *out = *p;
        CTool_EndianConvertInPlaceWord32Ptr(p);
    } else {
        CTool_EndianConvertInPlaceWord32Ptr(p);
        *out = *p;
    }
}
static inline void readheader(volatile int *out, unsigned int *p, char flag)
{
    readhdr(out, p, flag);
}
/* Bounds of a serialized record in the input buffer. */

void swap_tagged_records(unsigned char *data, unsigned int size, int (*errorHandler)(const char *, int),
                         char swapBeforeRead)
{
    int offset = 0;
    volatile struct RecordBounds record;
    int length;
    short count;
    unsigned short tag;
    unsigned int *valueAddress;
    int value;
    int scalar;
    unsigned char character;
    unsigned int *wordAddress;
    short *shortAddress;

    DAT_005805e0 = errorHandler;
    while (offset < size) {
        record.endOffset = offset;
        readheader(&record.length, (unsigned int *)(data + offset), swapBeforeRead);
        if (record.length < 8) {
            if (record.length <= 0)
                (*DAT_005805e0)("ELF_Endian.c", 0x183);
            offset += record.length;
        } else {
            if (swapBeforeRead) {
                count = *(short *)((offset + 4) + data);
                CTool_EndianConvertInPlaceWord16Ptr((short *)((offset + 4) + data));
            } else {
                shortAddress = (short *)((offset + 4) + data);
                CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
                count = *shortAddress;
            }
            offset += 6;
            if (count == 0) {
                record.length = record.length - 6;
                offset += record.length;
            } else {
                goto recordSetup;
            }
        }
        continue;
    recordTest:
        while (offset < record.endOffset) {
            if (swapBeforeRead) {
                shortAddress = (short *)(data + offset);
                tag = *shortAddress;
                CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
            } else {
                shortAddress = (short *)(data + offset);
                CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
                tag = *shortAddress;
            }
            offset += 2;
            switch (tag & 0xf) {
                case 1:
                    if (swapBeforeRead) {
                        value = *(int *)(data + offset);
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                    } else {
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        value = *valueAddress;
                    }
                    offset += 4;
                    break;
                case 2:
                    if (swapBeforeRead) {
                        value = *(int *)(data + offset);
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                    } else {
                        wordAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(wordAddress);
                        value = *wordAddress;
                    }
                    offset += 4;
                    break;
                case 3:
                    offset += swap_tagged_record_data(data + offset, tag, swapBeforeRead);
                    break;
                case 4:
                    if (swapBeforeRead) {
                        length = *(int *)(data + offset);
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                    } else {
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        length = *(int *)(data + offset);
                    }
                    offset += 4;
                    if (tag == 0xf4) {
                        while (length != 0) {
                            if (swapBeforeRead) {
                                value = *(int *)(data + offset);
                                valueAddress = (unsigned int *)(data + offset);
                                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                            } else {
                                valueAddress = (unsigned int *)(data + offset);
                                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                                value = *(int *)(data + offset);
                            }
                            offset += 4;
                            length -= 4;
                            while (data[offset] != '\0') {
                                offset++;
                                length--;
                            }
                            length--;
                            offset++;
                        }
                    }
                    break;
                case 5:
                    if (swapBeforeRead) {
                        shortAddress = (short *)(data + offset);
                        scalar = *shortAddress;
                        CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
                    } else {
                        shortAddress = (short *)(data + offset);
                        CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
                        scalar = *shortAddress;
                    }
                    offset += 2;
                    break;
                case 6:
                    if (swapBeforeRead) {
                        scalar = *(int *)(data + offset);
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                    } else {
                        valueAddress = (unsigned int *)(data + offset);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        scalar = *(int *)(data + offset);
                    }
                    offset += 4;
                    break;
                case 7:
                    read_double(data + offset, swapBeforeRead);
                    offset += 8;
                    break;
                case 8:
                    while ((character = data[offset]) != '\0')
                        offset++;
                    offset++;
                    break;
            }
        }
    }
    (void)value;
    return;
recordSetup:
    record.endOffset = record.endOffset + record.length;
    goto recordTest;
}

int swap_tagged_record_data(void *data, short kind, char readBeforeSwap)
{
    char *bytes = data;
    short *lengthAddress = data;
    int offset = 0;
    unsigned int *valueAddress;
    short remaining;
    unsigned char tag;
    short length;
    int size;
    int registerValue;
    int extendedValue;
    int constantValue;
    int typeReference;
    short shortValue;
    short extraValue;
    unsigned char flag = 0;

    if (readBeforeSwap != 0) {
        length = *lengthAddress;
        CTool_EndianConvertInPlaceWord16Ptr(lengthAddress);
    } else {
        CTool_EndianConvertInPlaceWord16Ptr(lengthAddress);
        length = *lengthAddress;
    }
    remaining = length;
    offset += 2;
    if (length > 0) {
        do {
            tag = *(unsigned char *)(bytes + offset);
            offset++;
            remaining--;
            if (kind == 0x23 || kind == 0x2a3 || kind == 0x2013 || kind == 0x293 || kind == 0x303) {
                switch (tag) {
                    case 1:
                    case 2:
                    case 4:
                    case 0x80:
                    case 0x81:
                        if (readBeforeSwap != 0) {
                            (void)(size = *(int *)(bytes + offset));
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        } else {
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                            (void)(size = *valueAddress);
                        }
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 3:
                        if (readBeforeSwap != 0) {
                            valueAddress = (unsigned int *)(bytes + offset);
                            (void)(registerValue = *valueAddress);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        } else {
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                            (void)(registerValue = *valueAddress);
                        }
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 5:
                    case 6:
                    case 7:
                        break;
                    case 0xe0:
                        if (readBeforeSwap != 0) {
                            valueAddress = (unsigned int *)(bytes + offset);
                            (void)(extendedValue = *valueAddress);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        } else {
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                            (void)(extendedValue = *valueAddress);
                        }
                        offset += 4;
                        remaining -= 4;
                        break;
                }
            } else if (kind == 0x83) {
                if (remaining < 4) {
                    if (readBeforeSwap != 0) {
                        valueAddress = (unsigned int *)(bytes + offset - 1);
                        (void)(constantValue = *valueAddress);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                    } else {
                        valueAddress = (unsigned int *)(bytes + offset - 1);
                        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        (void)(constantValue = *valueAddress);
                    }
                    offset += 3;
                    remaining -= 3;
                }
            } else if (kind == 0x63) {
                if (remaining < 2) {
                    if (readBeforeSwap != 0) {
                        (void)(shortValue = *(short *)(bytes + offset - 1));
                        CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset - 1));
                    } else {
                        CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset - 1));
                        (void)(shortValue = *(short *)(bytes + offset - 1));
                    }
                    offset += 1;
                    remaining -= 1;
                }
            } else if (kind == 0xa3) {
                if (!flag)
                    flag = 1;
                switch (tag) {
                    case 8:
                        if (readBeforeSwap != 0) {
                            kind = *(short *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        } else {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                            kind = *(short *)(bytes + offset);
                        }
                        offset += 2;
                        remaining -= 2;
                        if (kind == 0x55) {
                            if (readBeforeSwap != 0) {
                                (void)(extraValue = *(short *)(bytes + offset));
                                CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                            } else {
                                CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                                (void)(extraValue = *(short *)(bytes + offset));
                            }
                            offset += 2;
                            remaining -= 2;
                        } else if (kind == 0x63 || kind == 0x83) {
                            size = swap_tagged_record_data(bytes + offset, kind, readBeforeSwap);
                            offset += size;
                            remaining -= size;
                        } else if (kind == 0x72) {
                            if (readBeforeSwap != 0) {
                                valueAddress = (unsigned int *)(bytes + offset);
                                (void)(typeReference = *valueAddress);
                                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                            } else {
                                valueAddress = (unsigned int *)(bytes + offset);
                                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                                (void)(typeReference = *valueAddress);
                            }
                            offset += 4;
                            remaining -= 4;
                        }
                        flag = 0;
                        break;
                    case 0:
                        if (readBeforeSwap != 0) {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        } else {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        }
                        if (readBeforeSwap != 0) {
                            (void)(offset + 2 + bytes);
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 2 + bytes));
                        } else {
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 2 + bytes));
                        }
                        if (readBeforeSwap != 0) {
                            (void)(offset + 6 + bytes);
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 6 + bytes));
                        } else {
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 6 + bytes));
                        }
                        offset += 10;
                        remaining -= 10;
                        break;
                    case 1:
                        if (readBeforeSwap != 0) {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        } else {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        }
                        if (readBeforeSwap != 0) {
                            (void)(offset + 2 + bytes);
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 2 + bytes));
                        } else {
                            CTool_EndianConvertInPlaceWord32Ptr((unsigned int *)(offset + 2 + bytes));
                        }
                        offset += 6;
                        remaining -= 6;
                        size = swap_tagged_record_data(bytes + offset, 0x23, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        break;
                    case 2:
                        if (readBeforeSwap != 0) {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        } else {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        }
                        offset += 2;
                        remaining -= 2;
                        size = swap_tagged_record_data(bytes + offset, 0x23, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        if (readBeforeSwap != 0) {
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        } else {
                            valueAddress = (unsigned int *)(bytes + offset);
                            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
                        }
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 3:
                        if (readBeforeSwap != 0) {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        } else {
                            CTool_EndianConvertInPlaceWord16Ptr((short *)(bytes + offset));
                        }
                        offset += 2;
                        remaining -= 2;
                        size = swap_tagged_record_data(bytes + offset, 0x23, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        size = swap_tagged_record_data(bytes + offset, 0x23, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        flag = 0;
                        kind = 0;
                        break;
                }
            }
        } while (remaining > 0);
    }
    return offset;
}

long double read_double(UInt8 *bytes, char flag)
{
    union {
        double value;
        unsigned char bytes[8];
    } representation;

    if (flag != '\0') {
        representation.bytes[0] = bytes[0];
        representation.bytes[1] = bytes[1];
        representation.bytes[2] = bytes[2];
        representation.bytes[3] = bytes[3];
        representation.bytes[4] = bytes[4];
        representation.bytes[5] = bytes[5];
        representation.bytes[6] = bytes[6];
        representation.bytes[7] = bytes[7];
        CTool_EndianConvertMem(&bytes, 8);
    } else {
        CTool_EndianConvertMem(&bytes, 8);
        representation.bytes[0] = bytes[0];
        representation.bytes[1] = bytes[1];
        representation.bytes[2] = bytes[2];
        representation.bytes[3] = bytes[3];
        representation.bytes[4] = bytes[4];
        representation.bytes[5] = bytes[5];
        representation.bytes[6] = bytes[6];
        representation.bytes[7] = bytes[7];
    }
    return (long double)representation.value;
}

void swap_conversion_blocks(char *data, int remainingSize, int (*conversionMode)(const char *, int),
                            char readBeforeConversion)
{
    unsigned int blockSize;
    unsigned int offset;
    unsigned int *valueAddress;
    short *shortAddress;

    DAT_005805e0 = conversionMode;
    offset = 0;
    while (remainingSize != 0) {
        if (readBeforeConversion != '\0') {
            blockSize = ((ConversionBlockHeader *)(offset + data))->size;
            valueAddress = &((ConversionBlockHeader *)(offset + data))->size;
            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
        } else {
            valueAddress = &((ConversionBlockHeader *)(offset + data))->size;
            blockSize = CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
        }
        valueAddress = (unsigned int *)(offset + 4 + data);
        CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
        offset = offset + 8;
        remainingSize -= 8;
        do {
            if (readBeforeConversion != '\0') {
                valueAddress = &((ConversionBlockEntry *)(offset + data))->value;
                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
            } else {
                valueAddress = &((ConversionBlockEntry *)(offset + data))->value;
                CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
            }
            shortAddress = (short *)(offset + 4 + data);
            CTool_EndianConvertInPlaceWord16Ptr(shortAddress);
            valueAddress = (unsigned int *)(offset + 6 + data);
            CTool_EndianConvertInPlaceWord32Ptr(valueAddress);
            offset = offset + 10;
            remainingSize = remainingSize - 10;
        } while (offset < blockSize);
        data = data + offset;
        offset = 0;
    }
}

void ELF_Endian_ConvertThreeWords(unsigned int *words)
{
    words[0] = CTool_EndianConvertWord32(words[0]);
    words[1] = CTool_EndianConvertWord32(words[1]);
    words[2] = CTool_EndianConvertWord32(words[2]);
}

unsigned short ELF_Endian_ConvertSymbolRecord(void *data)
{
    unsigned short sectionIndex;
    struct ElfSymbolRecord *symbol;
    unsigned int name = CTool_EndianConvertWord32(((struct ElfSymbolRecord *)data)->name);

    symbol = data;
    symbol->name = name;
    symbol->value = CTool_EndianConvertWord32(symbol->value);
    symbol->size = CTool_EndianConvertWord32(symbol->size);
    sectionIndex = CTool_EndianConvertWord16(symbol->sectionIndex);
    symbol->sectionIndex = sectionIndex;
    return sectionIndex;
}

void ELF_Endian_SwapElf32Section(Elf32Section *values)
{
    values->name = CTool_EndianConvertWord32(values->name);
    values->type = CTool_EndianConvertWord32(values->type);
    values->flags = CTool_EndianConvertWord32(values->flags);
    values->address = CTool_EndianConvertWord32(values->address);
    values->file_offset = CTool_EndianConvertWord32(values->file_offset);
    values->size = CTool_EndianConvertWord32(values->size);
    values->link = CTool_EndianConvertWord32(values->link);
    values->info = CTool_EndianConvertWord32(values->info);
    values->alignment = CTool_EndianConvertWord32(values->alignment);
    values->entry_size = CTool_EndianConvertWord32(values->entry_size);
}

void ELF_Endian_SwapElf32Header(struct Elf32Header *header)
{
    header->type = CTool_EndianConvertWord16(header->type);
    header->machine = CTool_EndianConvertWord16(header->machine);
    header->version = CTool_EndianConvertWord32(header->version);
    header->entry = CTool_EndianConvertWord32(header->entry);
    header->program_header_offset = CTool_EndianConvertWord32(header->program_header_offset);
    header->section_header_offset = CTool_EndianConvertWord32(header->section_header_offset);
    header->flags = CTool_EndianConvertWord32(header->flags);
    header->header_size = CTool_EndianConvertWord16(header->header_size);
    header->program_header_size = CTool_EndianConvertWord16(header->program_header_size);
    header->program_header_count = CTool_EndianConvertWord16(header->program_header_count);
    header->section_header_size = CTool_EndianConvertWord16(header->section_header_size);
    header->section_header_count = CTool_EndianConvertWord16(header->section_header_count);
    header->section_name_index = CTool_EndianConvertWord16(header->section_name_index);
}
