#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/ELF_Endian.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CompilerTools.h"
#include "driver/libimp-eabi-ppc.h"

static int (*data_005805e0)(const char *, int);
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

void ELF_Endian_ConvertThreeWords(unsigned int *words)
{
    words[0] = CTool_EndianConvertWord32(words[0]);
    words[1] = CTool_EndianConvertWord32(words[1]);
    words[2] = CTool_EndianConvertWord32(words[2]);
}

void swap_conversion_blocks(char *data, int remainingSize, int (*conversionMode)(const char *, int),
                            char readBeforeConversion)
{
    unsigned int blockSize;
    unsigned int offset;
    unsigned int *valueAddress;
    short *shortAddress;

    data_005805e0 = conversionMode;
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

static unsigned short swap16(void *address, char readBeforeSwap)
{
    unsigned short value;

    if (readBeforeSwap != 0) {
        value = *(unsigned short *)address;
        CTool_EndianConvertInPlaceWord16Ptr(address);
    } else {
        CTool_EndianConvertInPlaceWord16Ptr(address);
        value = *(unsigned short *)address;
    }
    return value;
}

static unsigned int swap32(void *address, char readBeforeSwap)
{
    unsigned int value;

    if (readBeforeSwap != 0) {
        value = *(unsigned int *)address;
        CTool_EndianConvertInPlaceWord32Ptr(address);
    } else {
        CTool_EndianConvertInPlaceWord32Ptr(address);
        value = *(unsigned int *)address;
    }
    return value;
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

int swap_tagged_record_data(void *data, DwarfAttribute attribute, char readBeforeSwap)
{
    char *bytes = data;
    int offset = 0;
    short remaining;
    unsigned char op;
    int size;
    unsigned char inSubscript = 0;

    remaining = swap16(bytes, readBeforeSwap);
    offset += 2;
    if (remaining > 0) {
        do {
            op = *(unsigned char *)(bytes + offset);
            offset++;
            remaining--;
            if (attribute == AT_location || attribute == AT_return_addr || attribute == AT_MW_location ||
                attribute == AT_pure_virtual || attribute == AT_virtual) {
                switch (op) {
                    case 1:
                    case 2:
                    case 4:
                    case 0x80:
                    case 0x81:
                        (void)swap32(bytes + offset, readBeforeSwap);
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 3:
                        (void)swap32(bytes + offset, readBeforeSwap);
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 5:
                    case 6:
                    case 7:
                        break;
                    case 0xe0:
                        (void)swap32(bytes + offset, readBeforeSwap);
                        offset += 4;
                        remaining -= 4;
                        break;
                }
            } else if (attribute == AT_mod_u_d_type) {
                if (remaining < 4) {
                    (void)swap32(bytes + offset - 1, readBeforeSwap);
                    offset += 3;
                    remaining -= 3;
                }
            } else if (attribute == AT_mod_fund_type) {
                if (remaining < 2) {
                    (void)swap16(bytes + offset - 1, readBeforeSwap);
                    offset += 1;
                    remaining -= 1;
                }
            } else if (attribute == AT_subscr_data) {
                if (!inSubscript)
                    inSubscript = 1;
                switch (op) {
                    case 8:
                        attribute = swap16(bytes + offset, readBeforeSwap);
                        offset += 2;
                        remaining -= 2;
                        if (attribute == AT_fund_type) {
                            (void)swap16(bytes + offset, readBeforeSwap);
                            offset += 2;
                            remaining -= 2;
                        } else if (attribute == AT_mod_fund_type || attribute == AT_mod_u_d_type) {
                            size = swap_tagged_record_data(bytes + offset, attribute, readBeforeSwap);
                            offset += size;
                            remaining -= size;
                        } else if (attribute == AT_user_def_type) {
                            (void)swap32(bytes + offset, readBeforeSwap);
                            offset += 4;
                            remaining -= 4;
                        }
                        inSubscript = 0;
                        break;
                    case 0:
                        swap16(bytes + offset, readBeforeSwap);
                        size = swap32(offset + 2 + bytes, readBeforeSwap);
                        size = swap32(offset + 6 + bytes, readBeforeSwap);
                        offset += 10;
                        remaining -= 10;
                        break;
                    case 1:
                        swap16(bytes + offset, readBeforeSwap);
                        size = swap32(offset + 2 + bytes, readBeforeSwap);
                        offset += 6;
                        remaining -= 6;
                        size = swap_tagged_record_data(bytes + offset, AT_location, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        break;
                    case 2:
                        swap16(bytes + offset, readBeforeSwap);
                        offset += 2;
                        remaining -= 2;
                        size = swap_tagged_record_data(bytes + offset, AT_location, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        swap32(bytes + offset, readBeforeSwap);
                        offset += 4;
                        remaining -= 4;
                        break;
                    case 3:
                        swap16(bytes + offset, readBeforeSwap);
                        offset += 2;
                        remaining -= 2;
                        size = swap_tagged_record_data(bytes + offset, AT_location, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        size = swap_tagged_record_data(bytes + offset, AT_location, readBeforeSwap);
                        offset += size;
                        remaining -= size;
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        inSubscript = 0;
                        attribute = 0;
                        break;
                }
            }
        } while (remaining > 0);
    }
    return offset;
}

void swap_tagged_records(unsigned char *data, unsigned int size, int (*errorHandler)(const char *, int),
                         char swapBeforeRead)
{
    int offset = 0;
    struct RecordBounds record;
    int length;
    DwarfTag tag;
    DwarfAttribute attribute;
    unsigned char character;

    data_005805e0 = errorHandler;
    while (offset < size) {
        record.endOffset = offset;
        record.length = swap32(data + offset, swapBeforeRead);
        if ((int)record.length < 8) {
            if ((int)record.length <= 0)
                (*data_005805e0)("ELF_Endian.c", 0x183);
            offset += record.length;
        } else {
            tag = swap16((offset + 4) + data, swapBeforeRead);
            offset += 6;
            if (tag == TAG_padding) {
                record.length -= 6;
                offset += record.length;
            } else {
                goto recordSetup;
            }
        }
        continue;
    recordTest:
        while (offset < record.endOffset) {
            attribute = swap16(data + offset, swapBeforeRead);
            offset += 2;
            /* the attribute's form */
            switch (attribute & 0xf) {
                case 1:
                    (void)swap32(data + offset, swapBeforeRead);
                    offset += 4;
                    break;
                case 2:
                    (void)swap32(data + offset, swapBeforeRead);
                    offset += 4;
                    break;
                case 3:
                    offset += swap_tagged_record_data(data + offset, attribute, swapBeforeRead);
                    break;
                case 4:
                    length = swap32(data + offset, swapBeforeRead);
                    offset += 4;
                    if (attribute == AT_element_list) {
                        while (length != 0) {
                            (void)swap32(data + offset, swapBeforeRead);
                            offset += 4;
                            length -= 4;
                            while (data[offset] != '\0') {
                                offset++;
                                length--;
                            }
                            offset++;
                            length--;
                        }
                    }
                    break;
                case 5:
                    swap16(data + offset, swapBeforeRead);
                    offset += 2;
                    break;
                case 6:
                    swap32(data + offset, swapBeforeRead);
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
    return;
recordSetup:
    record.endOffset += record.length;
    goto recordTest;
}
