#define CERROR_FILE "ObjGen_PPC_EABI.c"
#include "compiler/common.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/DumpIR.h"
#include "compiler/ELF_Endian.h"
#include "compiler/Exceptions.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeListing.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "compiler/TOC.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include <ctype.h>
#include <stdio.h>
#include "compiler/Objects.h"

#include <string.h>

#pragma options align = mac68k
static Boolean data_00580d80;
static struct ObjGenSection *default_code_section;
static struct ObjGenSection *defaultDataSection;
static struct ObjGenSection *default_f32_section;
static struct ObjGenSection *savedDefaultDataSection;
static struct ObjGenSection *default_data_section;
static UInt8 section_table_dirty;
static SInt32 uid_counter;
static SInt32 object_storage_size;
static SInt32 object_data_size;
static SInt32 data_00580da4;
static SInt32 data_00580da8;
static int DAT_00580dac;
static struct ObjGenSection *DAT_00580db0;
static SInt32 *data_00580db4;
static struct BufferUpdate *pending_buffer_updates;
static struct BufferUpdate *pending_data_tail;
#pragma options align = reset

void ObjGen_PPC_EABI_EmitSerializedFormat(Object *key, int index)
{
    struct ObjGenSection *output;
    GList *buffer;
    struct SerializedFormat *format;
    struct SerializedValueList *values;
    struct SerializedValueList *short_values;
    ObjGenSection *lookup;
    struct SerializedWideHeader *wide_header;
    struct SerializedShortHeader *short_header;
    struct ObjGenRelocation *pending;
    struct ObjGenSection *records;
    int position;
    int header_offset;
    struct SerializedLocation location;
    lookup = select_object_section(key, index, 1, 1, 0);
    output = lookup->output;
    format = lookup->output->sectionData.serialized->format;
    if (lookup->output->sectionData.serialized->format->kind == 0) {
        buffer = &output->buffer;
        BE_elf_AlignRecord(buffer, 4);
        position = output->buffer.size;
        if (format->count >= 256 || index > 32767) {
            wide_header = (struct SerializedWideHeader *)galloc(12);
            memset(wide_header, 0, 12);
            wide_header->kind = 3;
            wide_header->flags = 0;
            wide_header->count = CTool_EndianConvertWord16(format->count);
            wide_header->index = CTool_EndianConvertWord32(index);
            wide_header->reserved = 0;
            header_offset = 8;
            CompilerTools_AppendGListData(&output->buffer, wide_header, 12);
            values = format->values;
            if (values != NULL) {
                do {
                    AppendGListTargetEndianLong(&output->buffer, values->value);
                    values = values->next;
                } while (values != NULL);
            }
        } else {
            short_header = (struct SerializedShortHeader *)galloc(8);
            memset(short_header, 0, 8);
            short_header->kind = 2;
            short_header->count = (char)format->count;
            short_header->index = CTool_EndianConvertWord16(index);
            short_header->reserved = 0;
            header_offset = 4;
            CompilerTools_AppendGListData(&output->buffer, short_header, 8);
            short_values = (SerializedValueList *)format->values;
            if (short_values != NULL) {
                do {
                    AppendGListTargetEndianWord(&output->buffer, (short)short_values->value);
                    short_values = short_values->next;
                } while (short_values != NULL);
            }
        }
        records = output->relocations;
        records->relocationCount += 1;
        pending = (struct ObjGenRelocation *)galloc(24);
        pending->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(key);
        pending->kind = 1;
        pending->section = records;
        pending->index = records->relocationCount - 1;
        location.offset = position + header_offset;
        location.reserved = 0;
        position = location.offset;
        location.kind = 1;
        CompilerTools_AppendGListData(&records->buffer, &location, 12);
        pending->next = relocation_list;
        relocation_list = pending;
    }
    return;
}

void ObjGen_PPC_EABI_AppendOutputEntry(ObjGenSection *context, unsigned int value)
{
    SerializedFormat *entries = context->output->sectionData.serialized->format;
    if (!entries->kind) {
        if (!entries->last) {
            entries->last = (SerializedValueList *)galloc(8U);
            entries->values = entries->last;
        } else {
            entries->last->next = (SerializedValueList *)galloc(8U);
            entries->last = entries->last->next;
        }
        memset(entries->last, 0, 8U);
        entries->last->value = value;
        if (entries->count > 32767)
            CError_FATAL(2466);
        entries->count += 1;
    }
}

void ObjGen_PPC_EABI_AddSectionAttribute(Object *object, UInt8 flags)
{
    ObjGenSection *info;
    ObjGenSection *output;
    CGList *list;
    SectionAttributeNode *node;
    SectionRec *section;
    char *name;

    info = select_object_section(object, 0, 1, 1, 0);
    if (info->output == NULL) {
        name = (char *)galloc(strlen(info->name) + 8);
        strcpy(name, ".mwcats");
        strcat(name, info->name);
        section = BE_elf_CreateSectionRec(name, NULL, 0, 0, 0);
        output = BE_elf_CreateSectionWithRelocations(section, 9, 4, 5000, 1000, 0, 0, 0);
        info->output = output;
        list = (CGList *)galloc(sizeof(CGList));
        output->sectionData.attributes = list;
        memset(list, 0, sizeof(CGList));
        list->sourceSection = info;
    }
    list = info->output->sectionData.attributes;
    if (list->tail == NULL) {
        list->tail = (SectionAttributeNode *)galloc(sizeof(SectionAttributeNode));
        list->head = list->tail;
    } else {
        if (list->tail->object == object) {
            list->tail->flags |= flags;
            return;
        }
        node = (SectionAttributeNode *)galloc(sizeof(SectionAttributeNode));
        list->tail->next = node;
        list->tail = list->tail->next;
    }
    memset(list->tail, 0, sizeof(SectionAttributeNode));
    list->tail->object = object;
    list->tail->flags = flags;
}

void create_main_file_object(void)
{
    SInt32 size;
    Boolean savedFileSymInfo;
    UInt8 sourceName[256];
    char objectName[256];
    Object *obj;
    char *cursor;
    CPrepCU *unit = (CPrepCU *)cprep_cu;

    savedFileSymInfo = copts.filesyminfo;
    CompilerTools_GetPFileFields(&unit->mainFile, NULL, NULL, sourceName);
    sprintf(objectName, "__%.*s", sourceName[0], sourceName + 1);

    cursor = &objectName[sourceName[0] + 1];
    while (*cursor) {
        if (*cursor == '.') {
            cursor[0] = '_';
            cursor[1] = 'o';
            cursor[2] = 0;
            break;
        }
        cursor--;
    }
    for (cursor = objectName + 1; *cursor; cursor++) {
        if (*cursor == '.')
            *cursor = '_';
    }

    obj = (Object *)galloc(sizeof(Object));
    memclrw(obj, sizeof(Object));
    obj->otype = OT_OBJECT;
    obj->type = (Type *)&stchar;
    obj->name = GetHashNameNode(objectName);
    obj->sclass = TK_EXTERN;
    obj->datatype = DDATA;
    ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, 0);
    BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
    copts.filesyminfo = 0;
    size = obj->type->size;
    if (copts.filesyminfo && obj->name->name[0] != '@') {
        if (!DAT_0058849e) {
            DWARF_SetSectionAndState(dwarf_info_section, dwarf_line_section);
            DWARF_SetupSectionDebugState(BE_elf_FindSection(".text", 0));
            DAT_0058849e = 1;
        }
        DWARF_CreateObjectDebugEntry(obj);
    }
    allocate_object_storage(obj, size, 0);
    copts.filesyminfo = savedFileSymInfo;
}

InterruptGenerationRecord *ObjGen_PPC_EABI_GetInterruptInfo(Object *obj)
{
    BE_SymNode *node;
    InterruptGenerationRecord *info = (node = BE_symbol_GetOrCreateFunctionObjectSymbol(obj))->interruptInfo;
    CError_ASSERT(2333, info != NULL);
    return info;
}

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

InterruptGenerationRecord *fn_00488750(Object *object, BE_SymNode *linkage)
{
    InterruptGenerationRecord *resolved;
    SectionSymbolAttributes *lookupHead;
    short index;
    int lookupIndex;
    int count;
    ObjGenSection *entry;
    SectionSymbolAttributes *lookup;
    ObjGenSection *entryHead;
    short tableSize;
    InterruptGenerationRecord *previous;

    resolved = CodeGen_FindInterruptGenerationRecord(object->section);
    CE_ASSERT(resolved->sectionIndex == 0, CError_FATAL(2299));
    if ((previous = linkage->interruptInfo) == NULL) {
        linkage->interruptInfo = resolved;
        object->section = resolved->sectionIndex;
        return resolved;
    }
    if (previous->sectionIndex == 0) {
        object->section = previous->sectionIndex = resolved->sectionIndex;
    }
    if (((short *)previous)[2] != (short)resolved->sectionIndex) {
        if ((index = previous->sectionIndex) < 0) {
            index = CodeGen_FindInterruptGenerationRecord(index)->sectionIndex;
        }
        lookupIndex = index;
        if (section_table_dirty != 0 && section_table_dirty != 0) {
            section_table_dirty = 0;
            copts.sectionHeaderTableSize = (tableSize = sectionHeaderCount),
            data_00580db4 = copts.sectionHeaderTable = (SInt32 *)galloc(tableSize << 2);
            memset(copts.sectionHeaderTable, 0, sectionHeaderCount << 2);
            count = 0;
            entry = entryHead = section_list;
            if (entryHead != NULL) {
                do {
                    copts.sectionHeaderTable[count] = (SInt32)entry->header;
                    count = count + 1;
                    entry = entry->next;
                } while (entry != NULL);
            }
        }
        lookup = lookupHead = section_symbol_attributes;
        if (lookupHead != NULL) {
            do {
                if ((short)lookup->index == lookupIndex) {
                    break;
                }
                lookup = lookup->next;
            } while (lookup != NULL);
        }
        CE_ASSERT(lookup == 0, CError_FATAL(2320));
        CE_ASSERT(lookup->target == 0, CError_FATAL(2321));
        PPCError_ReportError(129, linkage->nameData.hashName->name, lookup->target->name);
    }
    object->section = previous->sectionIndex;
    return previous;
}

UInt16 ObjGen_PPC_EABI_GetSectionIndex(short reg)
{
    InterruptGenerationRecord *object;

    if (reg < 0) {
        object = CodeGen_FindInterruptGenerationRecord(reg);
        return object->sectionIndex;
    }
    return reg;
}

void ObjGen_PPC_EABI_EmitObjectRelocation(Object *object)
{
    unsigned int header[3];
    unsigned int value;
    ObjGenSection *output;
    ObjGenSection *records;
    ObjGenRelocation *record;
    unsigned int position;
    value = CTool_EndianConvertWord32(0);
    output = (ObjGenSection *)*(int *)&data_005884ae;
    if (output == NULL) {
        CError_FATAL(2219);
    }
    position = output->buffer.size;
    CompilerTools_AppendGListData(&output->buffer, &value, 4);
    records = output->relocations;
    records->relocationCount += 1;
    record = (ObjGenRelocation *)galloc(24);
    record->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    record->kind = 1;
    record->section = records;
    record->index = records->relocationCount - 1;
    header[0] = position;
    header[1] = record->kind & 0xff;
    header[2] = 0;
    CompilerTools_AppendGListData(&records->buffer, header, 12);
    record->next = relocation_list;
    relocation_list = record;
}

/* State passed through to the PPC object emitter. */
/* State passed through to the PPC object emitter. */

void fn_004889b0(ObjGenSection *context, int section, Object *object, UInt8 flags, unsigned int options)
{
    emit_relocation(flags, section, object, context, options);
}

GList *ObjGen_PPC_EABI_GetSectionBuffer(ObjGenSection *section)
{
    return &section->buffer;
}

void ObjGen_PPC_EABI_EmitDescriptorWithRelocations(Object *obj, SInt32 value, void *data, UInt32 size,
                                                   ObjGenRelocationRequest *list)
{
    Object *object;
    ObjGenSection *section;
    ObjGenSection *recordSection;
    ObjGenSection *relocations;
    ObjGenRelocation *relocation;
    BE_SymNode *symbol;
    Object *target;
    SInt32 base;
    BE_SymNode *dataSymbol;
    SInt32 dataAddend;
    SInt32 objectAddend;
    SInt32 recordOffset;
    ObjGenSymbolLink *link;
    RelocationRecord record;
    RelocationRecord descriptor;
    SInt32 zero = 0;
    const SInt32 *words = data;

    if (size != 4) {
        object = galloc(sizeof(*object));
        memset(object, 0, sizeof(*object));
        object->sclass = TK_STATIC;
        object->name = CParser_GetUniqueName();
        COptimizer_GetFunctionObject(object);
        object->datatype = DDATA;
        object->section = data_005884b6->header->index;
        section = data_005884b6;
        dataSymbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
        dataSymbol->symbolKind = 1;
        dataSymbol->size = size;
        dataSymbol->attributes = 0;
        dataSymbol->sectionData.section = section;
        dataSymbol->alignment = 4;
        dataSymbol->flags = 2;
        BE_elf_AlignRecord(&section->buffer, dataSymbol->alignment);
        dataSymbol->offset = base = section->buffer.size;
        CompilerTools_AppendGListData(&section->buffer, data, size);
        if (size & 3)
            CompilerTools_AppendGListData(&section->buffer, &zero, ((size + 3) & ~3u) - size);
        relocations = section->relocations;
        for (; list != NULL; list = list->next) {
            target = list->object;
            relocations->relocationCount++;
            relocation = galloc(sizeof(*relocation));
            relocation->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(target);
            relocation->kind = 1;
            relocation->section = relocations;
            relocation->index = relocations->relocationCount - 1;
            record.offset = base + list->offset;
            record.addend = 0;
            record.kind = 1;
            CompilerTools_AppendGListData(&relocations->buffer, &record, sizeof(record));
            relocation->next = relocation_list;
            relocation_list = relocation;
        }
        object_data_size += size;
    }

    object = galloc(sizeof(*object));
    memset(object, 0, sizeof(*object));
    object->sclass = TK_STATIC;
    object->name = CParser_GetUniqueName();
    COptimizer_GetFunctionObject(object);
    object->datatype = DDATA;
    object->section = data_005884ba->header->index;
    recordSection = data_005884ba;
    symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    symbol->symbolKind = 1;
    symbol->size = sizeof(descriptor);
    symbol->attributes = 0;
    symbol->sectionData.section = recordSection;
    symbol->alignment = 4;
    symbol->flags = 2;
    BE_elf_AlignRecord(&recordSection->buffer, symbol->alignment);
    recordOffset = recordSection->buffer.size;
    symbol->offset = recordOffset;
    descriptor.offset = 0;
    descriptor.kind = CTool_EndianConvertWord32((value & 0x7fffffff) | ((size == 4) << 31));
    descriptor.addend = (size == 4) ? *words : 0;
    CompilerTools_AppendGListData(&recordSection->buffer, &descriptor, sizeof(descriptor));
    relocations = recordSection->relocations;
    relocations->relocationCount++;
    relocation = galloc(sizeof(*relocation));
    relocation->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
    objectAddend = 0;
    if (copts.usedatapool && (link = relocation->symbol->sectionData.section->symbolLink) != NULL &&
        link->symbol != NULL && !(obj->qual & Q_IMPLICIT_WEAK) && !(obj->qual & Q_WEAK) && obj->datatype != DFUNC &&
        (UInt8)(obj->datatype - DVFUNC) > DINLINEFUNC - DVFUNC && !PCodeUtilities_Require(obj)) {
        relocation->symbol->flags |= 0x80;
        objectAddend = relocation->symbol->offset;
        relocation->symbol = relocation->symbol->sectionData.section->symbolLink->symbol;
        relocation->symbol->flags &= 0xbf;
    }
    relocation->kind = 1;
    relocation->section = relocations;
    relocation->index = relocations->relocationCount - 1;
    record.offset = recordOffset;
    record.addend = objectAddend;
    record.kind = 1;
    CompilerTools_AppendGListData(&relocations->buffer, &record, sizeof(record));
    relocation->next = relocation_list;
    relocation_list = relocation;

    if (size != 4) {
        relocations->relocationCount++;
        relocation = galloc(sizeof(*relocation));
        relocation->symbol = dataSymbol;
        dataAddend = 0;
        if (copts.usedatapool && (link = relocation->symbol->sectionData.section->symbolLink) != NULL &&
            link->symbol != NULL && !(object->qual & Q_IMPLICIT_WEAK) && !(object->qual & Q_WEAK) &&
            object->datatype != DFUNC && (UInt8)(object->datatype - DVFUNC) > DINLINEFUNC - DVFUNC &&
            !PCodeUtilities_Require(object)) {
            relocation->symbol->flags |= 0x80;
            dataAddend = relocation->symbol->offset;
            relocation->symbol = relocation->symbol->sectionData.section->symbolLink->symbol;
            relocation->symbol->flags &= 0xbf;
        }
        relocation->kind = 1;
        relocation->section = relocations;
        relocation->index = relocations->relocationCount - 1;
        recordOffset += 8;
        record.offset = recordOffset;
        record.kind = 1;
        record.addend = dataAddend;
        CompilerTools_AppendGListData(&relocations->buffer, &record, sizeof(record));
        relocation->next = relocation_list;
        relocation_list = relocation;
    }
    object_data_size += sizeof(descriptor);
}

void ObjGen_PPC_EABI_RestoreFunctionState(void)

{
    emit_dwarf_arguments_and_locals();
    DWARF_RestoreFunctionState();
    return;
}

SInt16 ObjGen_PPC_EABI_SetupFunctionSection(Object *param)
{
    ObjGenSection *buffer;
    SInt16 index;
    UInt8 alignment;

    buffer = select_object_section(param, 0, 1, 1, 0);
    DAT_00580db0 = (ObjGenSection *)buffer;
    if (copts.filesyminfo != 0)
        DWARF_SetupFunctionState(buffer);
    if ((buffer->flags & 4) == 0)
        report_section_permission_conflict(param, buffer, "RX", 4);
    if ((alignment = copts.codeAlignment) > buffer->maximumSize)
        buffer->maximumSize = alignment;
    BE_elf_AlignRecord(&buffer->buffer, copts.codeAlignment);
    DAT_00580dac = buffer->buffer.size;
    if ((index = param->section) < 0)
        index = CodeGen_FindInterruptGenerationRecord(index)->sectionIndex;
    return index;
}

void ObjGen_PPC_EABI_00488ee0(SInt32 entry, SInt32 value)
{
    if ((entry == data_00587ff8->flags && value == data_00580da8) || entry == 0)
        return;
    if (value <= data_00580da8) {
        if (value == data_00580da8)
            DWARF_ReplaceTrailingLongWordLong(data_00587ff8->flags = entry, value + DAT_00580dac);
        return;
    }
    DWARF_AppendLongWordLong(data_00587ff8->flags = entry, value + DAT_00580dac);
    data_00580da8 = value;
}

/* Serialized relocation record. */
/* Relocation output block and its owning context. */

void emit_relocation(SInt32 op, SInt32 offset, Object *obj, ObjGenSection *ctx, SInt32 value)
{
    EABIRelocationRecord record;
    ObjGenSection *context;
    ObjGenSection *block;
    ObjGenRelocation *node;
    ObjGenSymbolLink *entry;

    context = (ObjGenSection *)(void *)ctx;
    block = context->relocations;
    block->relocationCount++;
    node = galloc(sizeof(ObjGenRelocation));
    node->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
    if (copts.usedatapool && (entry = node->symbol->sectionData.section->symbolLink) != NULL && entry->symbol != NULL &&
        !(obj->qual & Q_IMPLICIT_WEAK) && !(obj->qual & Q_WEAK) && obj->datatype != DFUNC &&
        (UInt8)(obj->datatype - DVFUNC) > 1 && PCodeUtilities_Require(obj) == 0 && op != 0xe && op != 0xf &&
        TOC_HasObjectReferenceWithoutExpression(node->symbol->sectionData.section->symbolLink->object) != 0) {
        node->symbol = (BE_SymNode *)node->symbol->sectionData.section->symbolLink->symbol;
    }

    offset += DAT_00580dac;
    node->kind = -1;
    record.value = value;
    if (op == 3) {
        if (ObjGen_PPC_EABI_0048ac10(obj)) {
            node->kind = 0x6d;
            offset += copts.littleendian ? 0 : copts.rel109_offset;
        } else {
            node->kind = 3;
            offset += copts.littleendian ? 0 : 2;
        }
    } else if (op == 2) {
        node->kind = 10;
    } else if (op == 10) {
        node->kind = 2;
    } else if (op == 8) {
        node->kind = 11;
    } else if (op == 13) {
        node->kind = 7;
    } else if (op == 4) {
        node->kind = 4;
        offset += copts.littleendian ? 0 : 2;
    } else if (op == 9) {
        node->kind = -3;
        CError_FATAL(1824);
    } else if (op == 11) {
        node->kind = -4;
        CError_FATAL(1827);
    } else if (op == 5) {
        node->kind = 4;
        offset += copts.littleendian ? 0 : 2;
    } else if (op == 6) {
        node->kind = 5;
        offset += copts.littleendian ? 0 : 2;
    } else if (op == 7) {
        node->kind = 6;
        offset += copts.littleendian ? 0 : 2;
    } else if (op == 15) {
        node->kind = 6;
        offset += copts.littleendian ? 0 : 2;
    } else if (op == 14) {
        node->kind = 4;
        offset += copts.littleendian ? 0 : 2;
    } else {
        CError_FATAL(1844);
    }
    node->section = block;
    node->index = block->relocationCount - 1;
    record.offset = offset;
    record.kind = node->kind & 0xff;
    CompilerTools_AppendGListData(&block->buffer, &record, sizeof(record));
    node->next = relocation_list;
    relocation_list = node;
}

void ObjGen_PPC_EABI_SetSymbolOffset(Object *object, int offset)
{
    BE_SymNode *record;

    record = BE_symbol_004918f0(object, DAT_00580db0);
    record->offset = DAT_00580dac + offset;
}

ObjGenSection *fn_004892a0(Object *object, int size)
{
    BE_SymNode *info;
    ObjGenSection *section;

    section = select_object_section(object, size, 1, 1, 0);
    DAT_00580db0 = section;
    if ((section->flags & 4) == 0) {
        report_section_permission_conflict(object, section, "RX", 4);
    }
    if ((data_005882c0.record != NULL) && (0x100 < size)) {
        PPCError_ReportDiagnostic(0xa3, object, size);
    }
    info = BE_symbol_SetupObjectSymbol(object, size, section);
    if (info->alignment > section->maximumSize) {
        section->maximumSize = info->alignment;
    }
    BE_elf_AlignRecord(&section->buffer, info->alignment);
    info->offset = section->buffer.size;
    if (copts.catssupport != '\0') {
        ObjGen_PPC_EABI_AddSectionAttribute(object, 0);
    }
    output_buffer_length = output_buffer_length + size;
    DAT_00580dac = section->buffer.size;
    return section;
}

void fn_00489360(Object *object, int size, void *data)
{
    CError_FATAL(1686);
}

void ObjGen_PPC_EABI_EmitSwitchTable(Object *gl, Object *func)
{
    UInt32 remaining;
    ObjGenRelocation *node;
    SInt32 index;
    EABIRelocationRecord relocation;
    SInt32 offset;
    ObjGenSection *list;
    ObjGenSection *record;
    BE_SymNode *info;
    BE_SymNode *reference;

    record = select_object_section(gl, gl->u.data.u.switchtable.size << 2, 1, 0, 0);
    info = BE_symbol_DefineObjectSymbol(gl, gl->u.data.u.switchtable.size << 2, record);
    reference = BE_symbol_GetOrCreateFunctionObjectSymbol(func);
    BE_elf_AlignRecord(&record->buffer, info->alignment);
    info->offset = offset = record->buffer.size;
    index = 0;
    list = record->relocations;
    remaining = gl->u.data.u.switchtable.size;
    for (; remaining != 0; remaining--) {
        list->relocationCount++;
        node = (ObjGenRelocation *)galloc(sizeof(ObjGenRelocation));
        node->symbol = reference;
        if (copts.usedatapool != 0 && node->symbol->sectionData.section->symbolLink != NULL &&
            node->symbol->sectionData.section->symbolLink->symbol != NULL && (func->qual & Q_IMPLICIT_WEAK) == 0 &&
            (func->qual & Q_WEAK) == 0 && func->datatype != DFUNC && (UInt8)(func->datatype - DVFUNC) > 1 &&
            !PCodeUtilities_Require(func)) {
            node->symbol = node->symbol->sectionData.section->symbolLink->symbol;
        }
        node->kind = 1;
        node->section = list;
        node->index = list->relocationCount - 1;
        relocation.offset = offset + index;
        relocation.value = *(SInt32 *)((char *)gl->u.data.u.switchtable.data + index);
        relocation.value = CTool_EndianConvertWord32(relocation.value);
        *(void **)((char *)gl->u.data.u.switchtable.data + index) = NULL;
        relocation.kind = 1;
        CompilerTools_AppendGListData(&list->buffer, &relocation, 0xc);
        node->next = relocation_list;
        relocation_list = node;
        index += 4;
    }
    CompilerTools_AppendGListData(&record->buffer, gl->u.data.u.switchtable.data, gl->u.data.u.switchtable.size << 2);
    object_data_size += info->size;
}

void ObjGen_PPC_EABI_EmitFloatObject(Object *node)
{
    ObjGenSection *output;
    BE_SymNode *record;
    Object *object;

    object = node;
    output = select_object_section(object, object->type->size, 1, 0, 1);
    record = BE_symbol_DefineObjectSymbol(object, object->type->size, output);

    if (record->alignment > output->maximumSize)
        output->maximumSize = record->alignment;

    BE_elf_AlignRecord(&output->buffer, record->alignment);
    record->offset = output->buffer.size;

    if (object->type->size == 4) {
        float value = (float)*(double *)object->u.data.u.string;
        CTool_EndianConvertMem(&value, 4);
        CompilerTools_AppendGListData(&output->buffer, &value, 4);
    } else if (object->type->size == 8) {
        double value = *(double *)object->u.data.u.string;
        CTool_EndianConvertMem(&value, 8);
        CompilerTools_AppendGListData(&output->buffer, &value, 8);
    } else {
        CError_FATAL(1527);
    }

    object_data_size += object->type->size;
}

void emit_object_data_and_relocations(Object *func, const char *data, OLinkList *list, SInt32 size, Boolean flag)
{
    ObjGenSection *buffer;
    Object *object;
    BE_SymNode *info;
    ObjGenRelocation *node;
    ObjGenSection *queue;
    BufferUpdate *notification;
    BE_SymNode *target;
    SInt32 addend;
    RelocationRecord record;
    SInt32 offset;

    buffer = select_object_section(func, size, 1, 0, flag);
    if (flag == 0 && (buffer->flags & 1) == 0)
        report_section_permission_conflict(func, buffer, "RW", 1);
    info = BE_symbol_DefineObjectSymbol(func, size, buffer);
    if (info->alignment > buffer->maximumSize)
        buffer->maximumSize = info->alignment;
    BE_elf_AlignRecord(&buffer->buffer, info->alignment);
    info->offset = offset = buffer->buffer.size;
    CompilerTools_AppendGListData(&buffer->buffer, data, info->size);
    queue = buffer->relocations;
    while (list != NULL) {
        object = list->obj;
        queue->relocationCount++;
        node = galloc(0x18);
        node->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
        addend = 0;
        if (copts.usedatapool != 0) {
            if (node->symbol->sectionData.section->symbolLink != NULL &&
                node->symbol->sectionData.section->symbolLink->symbol != NULL &&
                (object->qual & Q_IMPLICIT_WEAK) == 0 && (object->qual & Q_WEAK) == 0 && object->datatype != DFUNC &&
                (UInt8)(object->datatype - DVFUNC) > 1 && !PCodeUtilities_Require(object)) {
                node->symbol->flags |= 0x80;
                addend = node->symbol->offset;
                target = node->symbol->sectionData.section->symbolLink->symbol;
                node->symbol = target;
                node->symbol->flags &= 0xbf;
            }
        }
        node->kind = 1;
        node->section = queue;
        node->index = queue->relocationCount - 1;
        record.kind = 1;
        record.offset = offset + list->offset;
        record.addend = addend + list->addend;
        CompilerTools_AppendGListData(&queue->buffer, &record, 0xc);
        node->next = relocation_list;
        relocation_list = node;
        list = list->next;
    }
    notification = pending_buffer_updates;
    while (notification != NULL) {
        if (notification->target == info) {
            if ((UInt8)notification->active != 0 && (info->flags & 0x80) != 0)
                PPCError_ReportError(0x80);
            notification->active = 0;
            break;
        }
        notification = notification->next;
    }
    object_data_size += size;
}

void allocate_object_storage(Object *obj, SInt32 size, Boolean flag)
{
    ObjGenSection *sec;
    BE_SymNode *info;

    BE_SymNode *node = BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
    if (node->size != 0)
        return;

    sec = select_object_section(obj, size, 0, 0, flag);

    if (!flag && !(sec->flags & 1))
        report_section_permission_conflict(obj, sec, "RW", 1);

    info = BE_symbol_DefineObjectSymbol(obj, size, sec);

    if (info->alignment > sec->maximumSize)
        sec->maximumSize = info->alignment;

    if (sec->kind == 0x10) {
        info->offset = info->alignment;
    } else if (sec->kind != 6) {
        BE_elf_AlignRecord(&sec->buffer, info->alignment);
        info->offset = sec->buffer.size;
        AppendGListNoData(&sec->buffer, info->size);
        memset(*sec->buffer.data + info->offset, 0, info->size);
    } else {
        BufferUpdate *pendingData = galloc(10);
        if (pending_buffer_updates == NULL) {
            pending_buffer_updates = pendingData;
        } else {
            pending_data_tail->next = pendingData;
        }
        pendingData->next = NULL;
        pendingData->target = (BE_SymNode *)info;
        pendingData->active = 1;
        pendingData->suppressed = 0;
        pending_data_tail = pendingData;
        if (copts.usedatapool > 0 && sec->symbolLink != NULL && sec->symbolLink->symbol != NULL &&
            (obj->qual & Q_IMPLICIT_WEAK) == 0 && (obj->qual & Q_WEAK) == 0 && obj->datatype != DFUNC &&
            (UInt8)(obj->datatype - DVFUNC) > 1 && !PCodeUtilities_Require(obj)) {
            BE_elf_AlignRecord(&sec->buffer, info->alignment);
            info->offset = sec->buffer.size;
            AppendGListNoData(&sec->buffer, info->size);
            pendingData->suppressed = 1;
        }
    }

    object_storage_size += size;
}

void report_section_permission_conflict(Object *function, ObjGenSection *qualInfo, void *name, UInt32 flags)
{
    UInt32 combinedFlags;
    char *cursor;
    char currentPermissions[4];
    char requestedPermissions[4];

    cursor = currentPermissions;
    if (qualInfo->flags & 2)
        *cursor++ = 'R';
    if (qualInfo->flags & 1)
        *cursor++ = 'W';
    if (qualInfo->flags & 4)
        *cursor++ = 'X';
    *cursor = 0;

    cursor = requestedPermissions;
    if ((combinedFlags = qualInfo->alignment | flags) & 2)
        *cursor++ = 'R';
    if (combinedFlags & 1)
        *cursor++ = 'W';
    if (combinedFlags & 4)
        *cursor++ = 'X';
    *cursor = 0;

    PPCError_ReportDiagnostic(0x9d, name, COptimizer_GetFunctionObject(function)->name, qualInfo->name,
                              currentPermissions, requestedPermissions);
}

/* The object header as this translation unit sees it: the signed 16-bit
 * identifier field sits at +4 (the shared header types it unsigned). */

static inline void fill_section_table(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *lp;
    int index;

    if (section_table_dirty) {
        section_table_dirty = 0;
        count = sectionHeaderCount;
        copts.sectionHeaderTableSize = count;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        index = 0;
        lp = head = section_list;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)lp->header;
                lp = lp->next;
                index++;
            } while (lp != NULL);
        }
    }
}

static inline SectionSymbolAttributes *ObjGen_Lookup(Object *obj)
{
    SectionSymbolAttributes *p;
    SectionSymbolAttributes *p_s;
    SInt16 s;
    SInt32 id;

    /* (stand-in: the original calls it through an int-returning declaration) */ (
        void)((int (*)(Object *, SInt32, Boolean))ObjGen_PPC_EABI_SetObjectSection)(obj, obj->type->size, 0);
    s = obj->section;
    if (s < 0)
        s = CodeGen_FindInterruptGenerationRecord(s)->sectionIndex;
    id = s;
    if (section_table_dirty)
        fill_section_table();
    p = p_s = section_symbol_attributes;
    if (p_s) {
        do {
            if ((SInt16)p->index == id)
                break;
            p = p->next;
        } while (p);
    }
    return p;
}

static inline UInt16 section_code(Object *obj)
{
    SectionSymbolAttributes *p;
    UInt16 code;

    if (obj->datatype == DLOCAL) {
        code = 9;
    } else {
        SectionSymbolAttributes *p_s;
        SInt16 s;
        SInt32 id;
        /* (stand-in: the original calls it through an int-returning declaration) */ (
            void)((int (*)(Object *, SInt32, Boolean))ObjGen_PPC_EABI_SetObjectSection)(obj, obj->type->size, 0);
        s = obj->section;
        if (s < 0)
            s = CodeGen_FindInterruptGenerationRecord(s)->sectionIndex;
        id = s;
        if (section_table_dirty)
            fill_section_table();
        p = p_s = section_symbol_attributes;
        if (p_s) {
            do {
                if ((SInt16)p->index == id)
                    break;
                p = p->next;
            } while (p);
        }
        if (obj->datatype == DFUNC || (UInt8)(obj->datatype - 4) <= 1)
            code = p->alignment;
        else
            code = p->kind;
    }

    return code;
}

static inline void initialize_section_table(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *lp;
    int index;

    if (section_table_dirty) {
        section_table_dirty = 0;
        copts.sectionHeaderTableSize = count = sectionHeaderCount;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        index = 0;
        lp = head = section_list;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)lp->header;
                lp = lp->next;
                index++;
            } while (lp != NULL);
        }
    }
}

ObjGenSection *select_object_section(Object *obj, SInt32 section, Boolean usePrimary, Boolean force, Boolean options)
{
    ObjGenSection *record;
    SectionSymbolAttributes *entries;
    int id;
    SectionSymbolAttributes *entry;
    ObjGenSection *alternate;
    ObjGenSymbolLink *definition;
    Boolean hasAlternate = 0;

    ObjGen_PPC_EABI_SetObjectSection(obj, section, options);
    id = (SInt16)obj->section < 0 ? (SInt16)CodeGen_FindInterruptGenerationRecord((SInt16)obj->section)->sectionIndex
                                  : (SInt16)obj->section;

    if (section_table_dirty)
        initialize_section_table();

    entry = entries = (SectionSymbolAttributes *)section_symbol_attributes;
    if (entries) {
        do {
            if ((SInt16)entry->index == id)
                break;
            entry = entry->next;
        } while (entry);
    }
    CError_ASSERT(1266, entry);

    record = entry->target;
    CError_ASSERT(1268, record);

    if (!usePrimary) {
        if (!(alternate = entry->linkedSymbol)) {
            PPCError_ReportError(0x92, record->name, obj->name->name);
            return (ObjGenSection *)record;
        }
        hasAlternate = entry->auxiliary != NULL;
        record = alternate;
    }

    if (!PCodeUtilities_Require(obj) && !force && hasAlternate && !CParser_HasInternalLinkage(obj))
        return data_005884de;

    if ((definition = record->symbolLink) && record->buffer.size == 0 && definition->object == NULL &&
        !(obj->qual & Q_IMPLICIT_WEAK) && !(obj->qual & Q_WEAK)) {
        definition->object = obj;
        record->symbolLink->symbol = (struct BE_SymNode *)BE_symbol_CreateSectionSymbol((ObjGenSection *)record);
    }
    return (ObjGenSection *)record;
}

static inline void fill_section_table_for_code(void)
{
    SInt16 count;
    ObjGenSection *entry;
    ObjGenSection *first;
    int index;

    if (section_table_dirty) {
        section_table_dirty = 0, copts.sectionHeaderTableSize = count = sectionHeaderCount;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        entry = first = section_list, index = 0;
        if (first)
            do {
                copts.sectionHeaderTable[index] = (SInt32)entry->header;
                entry = entry->next, index++;
            } while (entry);
    }
}

static inline SectionSymbolAttributes *object_section_lookup(Object *obj)
{
    SectionSymbolAttributes *section;
    SectionSymbolAttributes *head;
    SInt16 sectionIndex;
    SInt32 id;

    (void)((int (*)(Object *, SInt32, Boolean))ObjGen_PPC_EABI_SetObjectSection)(obj, obj->type->size, 0);
    if ((sectionIndex = obj->section) < 0)
        sectionIndex = CodeGen_FindInterruptGenerationRecord(sectionIndex)->sectionIndex;
    id = sectionIndex;
    if (section_table_dirty)
        fill_section_table_for_code();
    section = head = section_symbol_attributes;
    if (head) {
        do {
            if ((SInt16)section->index == id)
                break;
            section = section->next;
        } while (section);
    }
    return section;
}

static inline SInt16 object_section_code(Object *obj)
{
    UInt16 code;
    SectionSymbolAttributes *section;

    if (obj->datatype == DLOCAL) {
        code = 9;
    } else {
        SectionSymbolAttributes *head;
        SInt16 sectionIndex;
        SInt32 id;

        (void)((int (*)(Object *, SInt32, Boolean))ObjGen_PPC_EABI_SetObjectSection)(obj, obj->type->size, 0);
        if ((sectionIndex = obj->section) < 0)
            sectionIndex = CodeGen_FindInterruptGenerationRecord(sectionIndex)->sectionIndex;
        id = sectionIndex;
        if (section_table_dirty)
            fill_section_table_for_code();
        section = head = section_symbol_attributes;
        if (head) {
            do {
                if ((SInt16)section->index == id)
                    break;
                section = section->next;
            } while (section);
        }
        if (obj->datatype == DFUNC || (UInt8)(obj->datatype - 4) <= 1)
            code = section->alignment;
        else
            code = section->kind;
    }
    return code;
}

SInt16 ObjGen_PPC_EABI_MapObjectSectionCode(Object *obj)
{
    UInt16 sectionCode;
    switch ((UInt8)(sectionCode = object_section_code(obj))) {
        case 1:
        case 2:
        case 8:
        case 10:
        case 11:
        case 12:
            return 0;
        case 6:
            return 13;
        case 3:
        case 4:
        case 7:
            return 2;
        case 9:
            return stack_base_reg;
    }

    if (obj->datatype != DLOCAL)
        object_section_lookup(obj);
    CError_FATAL(1252);
    return 2;
}

static inline SInt16 ObjGen_PPC_EABI_DefaultBSSSectionIndex(void)
{
    return copts.bssSection->header->index;
}

static inline SInt16 ObjGen_PPC_EABI_DefaultDataSectionIndex(void)
{
    return copts.dataSection->header->index;
}

void ObjGen_PPC_EABI_SetObjectSection(Object *object, SInt32 size, Boolean isUninitialized)
{
    SInt16 section;
    InterruptGenerationRecord *interruptInfo;
    SInt32 smallDataLimit;

    interruptInfo = NULL;
    if ((section = object->section) > 0)
        return;
    if (section < 0) {
        interruptInfo = fn_00488750(object, BE_symbol_GetOrCreateFunctionObjectSymbol(object));
        if (interruptInfo->sectionIndex != 0)
            return;
        section = 0;
    }
    if (object->datatype == DDATA) {
        if (object->type != NULL && object->type->type == TYPEFLOAT && copts.debugEnabled == 0) {
            PPCError_ReportError(0x83, COptimizer_GetFunctionObject(object)->name);
        }
        if (object->name != NULL && object->name->name[1] == '_' && strncmp(object->name->name, "__vt__", 6) == 0) {
            size = object->type->size;
        }
        if (isUninitialized) {
            if (size == 0) {
                section = ObjGen_PPC_EABI_DefaultBSSSectionIndex();
            } else {
                smallDataLimit = copts.constsmalldatathreshold;
                if (smallDataLimit != 0 && size <= smallDataLimit) {
                    section = copts.smallBSSSection->header->index;
                } else {
                    section = ObjGen_PPC_EABI_DefaultBSSSectionIndex();
                }
            }
        } else {
            if (size == 0) {
                section = ObjGen_PPC_EABI_DefaultDataSectionIndex();
            } else {
                smallDataLimit = copts.nonconstsmalldatathreshold;
                if (smallDataLimit != 0 && size <= smallDataLimit) {
                    section = copts.smallDataSection->header->index;
                } else {
                    section = ObjGen_PPC_EABI_DefaultDataSectionIndex();
                }
            }
        }
    } else if ((UInt8)(object->datatype - 3) <= 2) {
        section = copts.textSection->header->index;
    }
    if (interruptInfo != NULL)
        interruptInfo->sectionIndex = section;
    object->section = section;
}

short ObjGen_PPC_EABI_GetHeaderIndex(short kind)
{
    unsigned short value;
    switch (kind) {
        case 1:
            value = copts.textSection->header->index;
            break;
        case 2:
            value = copts.dataSection->header->index;
            break;
        case 0x20:
            value = copts.bssSection->header->index;
            break;
        case 0x22:
            value = copts.smallBSSSection->header->index;
            break;
        case 0x21:
            value = copts.smallDataSection->header->index;
            break;
        default:
            CError_FATAL(1103);
    }
    return value;
}

static inline void fill_section_header_table(void)
{
    SInt16 count;
    ObjGenSection *lp;
    SInt32 index;
    ObjGenSection *head;

    section_table_dirty = 0;
    count = sectionHeaderCount;
    copts.sectionHeaderTableSize = count;
    data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
    memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
    lp = head = section_list;
    index = 0;
    if (head != NULL) {
        do {
            copts.sectionHeaderTable[index] = (SInt32)lp->header;
            index++;
            lp = lp->next;
        } while (lp != NULL);
    }
}

static inline void build_section_table(UInt16 sectionCount)
{
    SInt16 count = sectionCount;
    ObjGenSection *record;
    ObjGenSection *head;
    SInt32 index;

    copts.sectionHeaderTableSize = count;
    data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
    memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
    record = head = section_list;
    index = 0;
    if (head != NULL) {
        do {
            copts.sectionHeaderTable[index] = (SInt32)record->header;
            index++;
            record = record->next;
        } while (record != NULL);
    }
}

void ObjGen_PPC_EABI_InitSections(void)
{
    ObjGenSection *dataSection;
    SectionRec *section;
    ObjGenSection *associatedRecord;

    section_table_dirty = 1;
    fn_0049c510("", 0, 0, 0);

    section = BE_elf_CreateSectionRec(".text", ".text", 1, 10, 1);
    BE_elf_CreateSectionWithRelocations(section, 1, 4, 20000, 1000, 6, 1, 1);

    default_code_section = copts.textSection;
    section = BE_elf_CreateSectionRec(".init", ".init", 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 1, 4, 100, 100, 6, 1, 1);

    section = BE_elf_CreateSectionRec(".fini", ".fini", 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 1, 4, 100, 100, 6, 1, 1);

    section = BE_elf_CreateSectionRec(".ctors", NULL, 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 2, 4, 4, 4, 3, 0, 1);

    section = BE_elf_CreateSectionRec(".dtors", NULL, 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 3, 4, 4, 4, 3, 0, 1);

    section = BE_elf_CreateSectionRec("extab", NULL, 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 4, 4, 0x40, 0x40, 2, 0, 1);

    section = BE_elf_CreateSectionRec("extabindex", NULL, 1, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 5, 4, 0x40, 0x40, 2, 0, 1);

    section = BE_elf_CreateSectionRec(".rodata", ".rodata", 1, 10, 4);
    BE_elf_CreateSectionWithRelocations(section, 1, 8, 1000, 1000, 2, 1, 1);

    default_f32_section = copts.bssSection;
    section = BE_elf_CreateSectionRec(".data", ".bss", 1, 10, 2);
    dataSection = BE_elf_CreateSectionWithRelocations(section, 1, 8, 4000, 1000, 3, 1, 1);

    defaultDataSection = copts.dataSection;
    associatedRecord = BE_elf_CreateSection("COMM", 0x10, 1, 0, NULL, 3, NULL);
    if (copts.commonblocks)
        dataSection->header->auxiliary = associatedRecord;
    data_section_linked_symbol = dataSection->header->linkedSymbol;

    section = BE_elf_CreateSectionRec(".line", NULL, 0, 0, 0);
    BE_elf_CreateSectionWithRelocations(section, 8, 1, 1000, 1000, 0, 0, 0);

    section = BE_elf_CreateSectionRec(".debug", NULL, 0, 0, 0);
    BE_elf_CreateSectionWithRelocations(section, 7, 4, 5000, 1000, 0, 0, 0);

    section = BE_elf_CreateSectionRec(".sdata", ".sbss", 6, 10, 8);
    BE_elf_CreateSectionWithRelocations(section, 1, 8, 1000, 1000, 3, 0, 1);

    savedDefaultDataSection = copts.smallDataSection;
    section = BE_elf_CreateSectionRec(".sdata2", ".sbss2", 7, 10, 0x10);
    BE_elf_CreateSectionWithRelocations(section, 1, 8, 1000, 1000, 3, 0, 1);

    default_data_section = copts.smallBSSSection;
    section = BE_elf_CreateSectionRec(".PPC.EMB.sdata0", ".PPC.EMB.sbss0", 8, 10, 0);
    BE_elf_CreateSectionWithRelocations(section, 1, 8, 1000, 1000, 3, 0, 1);

    fn_0049c510(".symtab", 12, 4, 1000);
    fn_0049c510(".strtab", 13, 1, 1000);
    fn_0049c510(".shstrtab", 14, 1, 300);
    fn_0049c510(".comment", 15, 1, 0x40);
    fn_0049c510("", 17, 1, 0);

    if (section_table_dirty) {
        section_table_dirty = 0;
        build_section_table(sectionHeaderCount);
    }
}

static inline void *galloc_clear(long size)
{
    void *p = galloc(size);
    memset(p, 0, size);
    return p;
}

static inline void SetDataSectionOptions(SectionRec *section, ObjGenSection *symbol, Boolean useDefaults)
{
    if (section->typebits & 2)
        copts.dataSection = useDefaults ? defaultDataSection : symbol;
    if (section->typebits & 4)
        copts.bssSection = useDefaults ? default_f32_section : symbol;
    if (section->typebits & 8)
        copts.smallDataSection = useDefaults ? savedDefaultDataSection : symbol;
    if (section->typebits & 0x10)
        copts.smallBSSSection = useDefaults ? default_data_section : symbol;
}

static inline void SetCodeSectionOption(ObjGenSection *symbol, Boolean useDefaults)
{
    copts.textSection = useDefaults ? default_code_section : symbol;
}

void ObjGen_PPC_EABI_SetSectionOptions(SectionRec *section)
{
    ObjGenSection *symbol;
    ObjGenSection *objectSymbol;
    ObjGenSection *linkedSymbol;
    struct ObjGenSymbolLink *symbolLink;
    SecKind sizeFlags;
    SecKind kind;
    ObjGenSection *auxiliary;
    SecKind alignment;

    if (section->sectionName == NULL) {
        if (section->typebits & 1)
            SetCodeSectionOption(NULL, 1);
        SetDataSectionOptions(section, NULL, 1);
        return;
    }
    symbol = BE_elf_FindSection(section->sectionName, 0);
    objectSymbol = BE_elf_FindSection(section->linkedSectionName, 0);
    if (symbol == NULL) {
        SecKind initialFlags;
        if (section->far_reloc == SK_5) {
            PPCError_ReportError(0x8e);
            return;
        }
        initialFlags = section->mode;
        if (initialFlags == 0)
            initialFlags = 2;
        if (section->far_reloc == SK_NONE)
            section->far_reloc = SK_1;
        if (section->near_reloc == 0)
            section->near_reloc = 0xa;
        if (section->typebits != 0) {
            if (section->typebits & 0x1a) {
                if (section->linkedSectionName == NULL) {
                    PPCError_ReportError(0x91);
                    return;
                }
                initialFlags |= 3;
            }
            if (section->typebits & 1)
                initialFlags |= 6;
        }
        BE_elf_CreateSectionWithRelocations(section, 1, 4, 1000, 100, initialFlags, 1, 1);
        section_table_dirty = 1;
        return;
    }
    if (symbol->kind == 6 || symbol->kind == 0x10) {
        PPCError_ReportError(0x8f, section->sectionName);
        return;
    }
    kind = symbol->header->kind;
    alignment = symbol->header->alignment;
    linkedSymbol = symbol->header->linkedSymbol;
    auxiliary = symbol->header->auxiliary;
    if (objectSymbol != NULL && objectSymbol->kind != 0x10) {
        section_table_dirty = 1;
        auxiliary = NULL;
    }
    if (section->linkedSectionName != NULL && linkedSymbol != NULL && linkedSymbol != objectSymbol) {
        section_table_dirty = 1;
        if (objectSymbol->kind == 0x10) {
            auxiliary = data_005884de;
            objectSymbol = linkedSymbol;
        }
        linkedSymbol = objectSymbol;
    }
    if (linkedSymbol == NULL && objectSymbol != NULL) {
        section_table_dirty = 1;
        if (objectSymbol->kind == 0x10) {
            auxiliary = data_005884de;
            objectSymbol = data_section_linked_symbol;
        }
        linkedSymbol = objectSymbol;
    }
    sizeFlags = section->mode;
    if (sizeFlags == 0)
        sizeFlags = 2;
    if (section->linkedSectionName != NULL && linkedSymbol == NULL) {
        section_table_dirty = 1;
        symbolLink = galloc_clear(10);
        linkedSymbol =
            BE_elf_CreateSection(section->linkedSectionName, 6, 4, 1000, section, sizeFlags & ~4, symbolLink);
        if (symbol->symbolLink == NULL) {
            symbol->symbolLink = galloc_clear(10);
        }
    }
    if (section->far_reloc != SK_NONE) {
        if (section->far_reloc == SK_5) {
            if (memcmp(section->sectionName, ".sdata", 7) == 0)
                section->far_reloc = SK_SDATA;
            else if (memcmp(section->sectionName, ".sdata2", 8) == 0)
                section->far_reloc = SK_SDATA2;
            else if (memcmp(section->sectionName, ".PPC.EMB.sdata0", 0x10) == 0)
                section->far_reloc = SK_SDATA0;
            else {
                PPCError_ReportError(0x8e);
                return;
            }
        }
        section_table_dirty = 1;
        kind = section->far_reloc;
    }
    if (section->near_reloc != 0) {
        section_table_dirty = 1;
        alignment = section->near_reloc;
    }
    if (section->typebits != 0) {
        if (section->typebits & 0x1a) {
            if (linkedSymbol == NULL) {
                PPCError_ReportError(0x91);
                return;
            }
            sizeFlags |= 3;
        }
        if (section->typebits & 1) {
            sizeFlags |= 6;
            SetCodeSectionOption(symbol, 0);
        }
        SetDataSectionOptions(section, symbol, 0);
    }
    symbol->alignment |= sizeFlags;
    symbol->flags |= sizeFlags;
    if (linkedSymbol != NULL) {
        linkedSymbol->alignment |= (sizeFlags & ~4);
        linkedSymbol->flags |= (sizeFlags & ~4);
    }
    symbol->header = BE_elf_GetOrCreateSectionSymbolAttributes(symbol, kind, alignment, linkedSymbol, auxiliary);
}

/* 0x58425e, dword global */
/* 0x5884a4, word global */

void ObjGen_PPC_EABI_UpdateSectionHeaders(void)
{
    ObjGenSection *section;
    SInt32 index;
    SInt32 tableSize;
    ObjGenSection *head;

    if ((tableSize = copts.sectionHeaderTableSize) < sectionHeaderCount) {
        memcpy(data_00580db4, copts.sectionHeaderTable, tableSize * sizeof(*copts.sectionHeaderTable));
        copts.sectionHeaderTable = data_00580db4;
        copts.sectionHeaderTableSize = sectionHeaderCount;
    }
    section = head = section_list;
    index = 0;
    if (head != NULL) {
        do {
            section->header = (struct SectionSymbolAttributes *)copts.sectionHeaderTable[index];
            index++;
            section = section->next;
        } while (section != NULL);
    }
}

void ObjGen_PPC_EABI_BuildSectionHeaderTable(void)
{
    ObjGenSection *p;
    SInt16 n;
    ObjGenSection *head;
    SInt32 i;

    if (section_table_dirty) {
        section_table_dirty = 0;
        copts.sectionHeaderTableSize = n = sectionHeaderCount;
        data_00580db4 = copts.sectionHeaderTable = galloc(n * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        p = head = section_list;
        i = 0;
        if (head)
            do {
                copts.sectionHeaderTable[i++] = (SInt32)p->header;
                p = p->next;
            } while (p);
    }
}

unsigned char ObjGen_PPC_EABI_IsSpecialSectionName(char *name)
{
    if (*name == 0) {
        return 1;
    }
    if (memcmp("sinit", name, 6) == 0) {
        return 1;
    }
    if (memcmp("extab", name, 6) == 0) {
        return 1;
    }
    if (memcmp("extabindex", name, 11) == 0) {
        return 1;
    }
    if (strncmp(".rela", name, 5) == 0) {
        return 1;
    }
    if (strncmp(".rel", name, 4) == 0) {
        return 1;
    }
    if (memcmp(".strtab", name, 8) == 0) {
        return 1;
    }
    if (memcmp(".shstrtab", name, 10) == 0) {
        return 1;
    }
    if (memcmp(".symtab", name, 8) == 0) {
        return 1;
    }
    if (strncmp(".debug", name, 6) == 0) {
        return 1;
    }
    if (memcmp(".line", name, 6) == 0) {
        return 1;
    }
    if (memcmp(".ctors", name, 7) == 0) {
        return 1;
    }
    if (memcmp(".dtors", name, 7) == 0) {
        return 1;
    }
    if (strncmp(".mwcats", name, 7) == 0) {
        return 1;
    }
    return 0;
}

/* 0x404e90, strncmp(a,b,n) */
/* 0x405850, tolower(c) */
/* 0x55d5b4, ".abs." */

Boolean ObjGen_PPC_EABI_IsInvalidAbsName(char *name)
{
    char *p;
    char ch;
    SInt32 len;

    if (strncmp(".abs.", name, 5) != 0)
        return 0;
    len = strlen(name + 5);
    if (len == 0 || len > 8)
        return 1;
    p = name + 5;
    while (*p != 0) {
        ch = *p;
        p++;
        switch (tolower(ch)) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                break;
            default:
                return 1;
        }
    }
    return 0;
}

static inline void rebuild_section_header_table(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *lp;
    SInt32 index;

    if (section_table_dirty) {
        section_table_dirty = 0;
        count = sectionHeaderCount;
        copts.sectionHeaderTableSize = count;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        lp = head = section_list;
        index = 0;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)lp->header;
                index++;
                lp = lp->next;
            } while (lp != NULL);
        }
    }
}

static inline void ensure_section_table_ac10(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *entry;
    SInt32 index;

    if (!section_table_dirty)
        return;
    section_table_dirty = 0;
    copts.sectionHeaderTableSize = count = sectionHeaderCount;
    data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
    memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
    entry = head = section_list;
    index = 0;
    if (head) {
        do {
            copts.sectionHeaderTable[index] = (SInt32)entry->header;
            index++;
            entry = entry->next;
        } while (entry);
    }
}

Boolean ObjGen_PPC_EABI_0048ac10(Object *obj)
{
    SInt32 kind;
    SInt16 sectionId;
    SInt32 sectionKey;
    SectionSymbolAttributes *head;
    SectionSymbolAttributes *section;

    if (obj->datatype == DLOCAL) {
        kind = 9;
    } else {
        ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, 0);
        if ((sectionId = obj->section) < 0)
            sectionId = CodeGen_FindInterruptGenerationRecord(sectionId)->sectionIndex;
        sectionKey = sectionId;
        if (section_table_dirty)
            ensure_section_table_ac10();
        section = head = section_symbol_attributes;
        if (head) {
            do {
                SInt16 index = section->index;
                if (index == sectionKey)
                    break;
                section = section->next;
            } while (section);
        }
        if (obj->datatype == DFUNC || (UInt8)(obj->datatype - 4) <= 1)
            kind = section->alignment;
        else
            kind = section->kind;
    }
    if ((UInt8)(kind - SK_SDATA) <= SK_SDATA0 - SK_SDATA)
        return 1;
    return 0;
}

static inline void fill_section_table_0048ad10(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *lp;
    SInt32 index;

    if (section_table_dirty) {
        section_table_dirty = 0;
        copts.sectionHeaderTableSize = count = sectionHeaderCount;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        lp = head = section_list;
        index = 0;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)lp->header;
                index++;
                lp = lp->next;
            } while (lp != NULL);
        }
    }
}

static inline SInt32 SectionKind(Object *obj)
{
    SInt16 id;
    SInt32 key;
    SectionSymbolAttributes *n_s;
    SectionSymbolAttributes *n;

    if (obj->datatype == DLOCAL)
        return 9;
    ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, 0);
    if ((id = ((Object *)obj)->section) < 0)
        id = CodeGen_FindInterruptGenerationRecord(id)->sectionIndex;
    key = id;
    if (section_table_dirty)
        fill_section_table_0048ad10();
    n = n_s = section_symbol_attributes;
    if (n_s) {
        do {
            if ((SInt16)n->index == key)
                break;
            n = n->next;
        } while (n);
    }
    if (obj->datatype == DFUNC || (UInt8)(obj->datatype - 4) <= 1)
        return n->alignment;
    return n->kind;
}

Boolean PCodeUtilities_Require(Object *obj)
{
    SInt32 result;
    UInt8 kind;

    kind = result = SectionKind(obj);
    if ((UInt8)(kind - 6) <= 2 || kind == 2)
        return 1;
    return 0;
}

static inline void build_section_header_table(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *lp;
    SInt32 index;

    if (section_table_dirty) {
        section_table_dirty = 0;
        count = sectionHeaderCount;
        copts.sectionHeaderTableSize = count;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        lp = head = section_list;
        index = 0;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)lp->header;
                index++;
                lp = lp->next;
            } while (lp != NULL);
        }
    }
}

static inline void build_section_index(void)
{
    SInt16 count;
    ObjGenSection *head;
    ObjGenSection *entry;
    SInt32 index;
    if (section_table_dirty) {
        section_table_dirty = 0;
        copts.sectionHeaderTableSize = count = sectionHeaderCount;
        data_00580db4 = copts.sectionHeaderTable = galloc(count * 4);
        memset(copts.sectionHeaderTable, 0, sectionHeaderCount * 4);
        entry = head = section_list;
        index = 0;
        if (head != NULL) {
            do {
                copts.sectionHeaderTable[index] = (SInt32)entry->header;
                index++;
                entry = entry->next;
            } while (entry != NULL);
        }
    }
}

SInt32 ObjGen_PPC_EABI_GetSectionAlignmentOrKind(Object *obj)
{
    SInt16 id;
    SInt32 key;
    SectionSymbolAttributes *head;
    SectionSymbolAttributes *section;

    if (obj->datatype == DLOCAL)
        return 9;

    ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, 0);

    if ((id = obj->section) < 0)
        id = CodeGen_FindInterruptGenerationRecord(id)->sectionIndex;
    key = id;

    if (section_table_dirty)
        build_section_index();

    section = head = section_symbol_attributes;
    if (head) {
        do {
            if ((SInt16)section->index == key)
                break;
            section = section->next;
        } while (section);
    }

    if (obj->datatype == DFUNC || (UInt8)(obj->datatype - 4) <= 1)
        return section->alignment;
    return section->kind;
}

char *ObjGen_PPC_EABI_GetSectionName(struct ObjGenSection *descriptor)
{
    CNameNode *entry;
    HashNameNode *name;
    unsigned int key;
    key = (unsigned int)descriptor->context;
    entry = data_005870e8;
    while (entry != NULL) {
        if (key == entry->uid) {
            name = entry->key;
            return name->name;
        }
        entry = entry->next;
    }
    CError_FATAL(630);
    return NULL;
}

unsigned int ObjGen_PPC_EABI_EmitObject(Object *object, const void *context, OLinkList *value, unsigned int flags)
{
    if (copts.filesyminfo != 0 && object->name->name[0] != '@') {
        if (DAT_0058849e == 0) {
            DWARF_SetSectionAndState(dwarf_info_section, dwarf_line_section);
            DWARF_SetupSectionDebugState(BE_elf_FindSection(".text", 0));
            DAT_0058849e = 1;
        }
        DWARF_CreateObjectDebugEntry(object);
    }
    if (context)
        emit_object_data_and_relocations(object, context, value, flags, 1);
    else
        allocate_object_storage(object, flags, 1);
}

void ObjGen_PPC_EABI_EmitObjectWithDebugEntry(Object *object, const void *data, OLinkList *attributes,
                                              unsigned int alignment)
{
    if (copts.filesyminfo) {
        HashNameNode *name = object->name;
        if (name->name[0] != 64) {
            if (!DAT_0058849e) {
                DWARF_SetSectionAndState(dwarf_info_section, dwarf_line_section);
                DWARF_SetupSectionDebugState(BE_elf_FindSection(".text", 0));
                DAT_0058849e = 1;
            }
            DWARF_CreateObjectDebugEntry(object);
        }
    }

    if (data)
        emit_object_data_and_relocations(object, data, attributes, alignment, 0);
    else
        allocate_object_storage(object, alignment, 0);
}

void ObjGen_PPC_EABI_ClearSectionSymbolLinkValues(void)
{
    ObjGenSection *head = (ObjGenSection *)section_list;
    ObjGenSection *node = head->next;

    while (node != NULL) {
        if (node->kind < 10 && node->symbolLink != NULL) {
            node->symbolLink->value = 0;
        }
        node = node->next;
    }
}

void fn_0048b090(HashNameNode *oldid, HashNameNode *newid)
{
    CNameNode *node;
    CNameNode *prev;

    if (oldid == NULL) {
        if (data_00580d80 == 0) {
            data_00580d80 = 1;
            data_00587ff8 = data_005870e8;
            data_005870e8->key = newid;
            return;
        }
        node = data_005870e8;
        while (node != NULL) {
            if (newid == node->key) {
                data_00587ff8 = node;
                return;
            }
            prev = node;
            node = node->next;
        }
    } else {
        node = data_005870e8;
        while (node != NULL) {
            if (oldid == node->key) {
                node->key = newid;
                data_00587ff8 = node;
                return;
            }
            prev = node;
            node = node->next;
        }
    }

    prev->next = (CNameNode *)galloc(16);
    data_00587ff8 = prev->next;
    memset(data_00587ff8, 0, 16);
    data_00587ff8->flags = -1;
    data_00587ff8->key = newid;
    uid_counter = uid_counter + 1;
    data_00587ff8->uid = uid_counter;
}

void fn_0048b160(HashNameNode *key, int nameType, int nameFlags)
{
    CNameNode *node;
    CNameNode *previous;

    if (key == NULL) {
        data_00587ff8 = data_005870e8;
        return;
    }
    node = data_005870e8;
    while (node != NULL) {
        if (key == node->key) {
            data_00587ff8 = node;
            return;
        }
        previous = node;
        node = node->next;
    }
    previous->next = (CNameNode *)galloc(sizeof(CNameNode));
    data_00587ff8 = previous->next;
    memset(data_00587ff8, 0, sizeof(CNameNode));
    data_00587ff8->flags = -1;
    data_00587ff8->key = key;
    ++uid_counter;
    data_00587ff8->uid = uid_counter;
}

void fn_0048b1e0(HashNameNode *name)
{
    return;
}

void ObjGen_PPC_EABI_SetObjectSectionIndex(Object *object)
{
    ObjGenSection *mapping;
    ObjGenSection *result;
    SectionSymbolAttributes *name;

    if (DAT_0058849e == 0) {
        DWARF_SetSectionAndState(dwarf_info_section, dwarf_line_section);
        DWARF_SetupSectionDebugState(BE_elf_FindSection(".text", 0));
        DAT_0058849e = 1;
    }
    data_00580da8 = -1;
    if (data_00587ff8 != data_005870e8) {
        mapping = select_object_section(object, 0, 1, 1, 1);
        result = BE_elf_FindSection(mapping->name, data_00587ff8->uid);
        if (result == NULL) {
            result = BE_elf_0049c540(mapping, data_00587ff8->uid);
        }
        result->header =
            BE_elf_GetOrCreateSectionSymbolAttributes(result, mapping->header->kind, mapping->header->alignment,
                                                      mapping->header->linkedSymbol, mapping->header->auxiliary);
        name = (SectionSymbolAttributes *)result->header;
        object->section = name->index;
    }
}

void fn_0048b2c0(void)
{
    return;
}

void fn_0048b2d0(void)
{
    return;
}

void fn_0048b2e0(void)
{
    if (DAT_0058849e != '\0') {
        fn_004ada90();
    }
    BE_elf_FreeSectionBuffers();
    FreeGList(&data_00583ae8.buffer);
    fn_004be840();
    fn_004c4be0();
}

void ObjGen_PPC_EABI_FinalizeOutputBuffers(void)
{
    BufferUpdate *update;
    GList *record;
    CInit_DeclarePooledStrings();
    TOC_EmitMemberPointerConstants();
    if (copts.create_file_object != 0) {
        create_main_file_object();
    }
    update = pending_buffer_updates;
    while (update != NULL) {
        if (update->active != 0) {
            if (update->target->sectionData.section != data_005884de && update->suppressed == 0) {
                record = &update->target->sectionData.section->buffer;
                BE_elf_AlignRecord(record, update->target->alignment);
                update->target->offset = update->target->sectionData.section->buffer.size;
                AppendGListNoData(&update->target->sectionData.section->buffer, update->target->size);
            }
        }
        update = update->next;
    }
    pending_buffer_updates = NULL;
    fn_0049b920();
    ShrinkGList(&data_00583ae8.buffer);
    ((CPrepCU *)cprep_cu)->objectBuffer = data_00583ae8.data;
    data_00583ae8.data = 0;
    {
        CPrepCU *unit = (CPrepCU *)cprep_cu;
        unit->codeSize = output_buffer_length;
        unit = (CPrepCU *)cprep_cu;
        unit->udataSize = object_storage_size;
        unit = (CPrepCU *)cprep_cu;
        unit->idataSize = object_data_size;
    }
}

void fn_0048b3f0(void)
{
    struct CNameNode *p;
    short result;

    CMachine_ResetMaximumAlignment();
    data_00583ae8.data = 0;
    pending_buffer_updates = NULL;
    data_00587ff8 = data_005870e8 = NULL;
    uid_counter = 0;
    data_005876b4 = 0;
    interrupt_generation_records = NULL;
    result = InitGList(&data_00583ae8.buffer, 40000);
    if (result) {
        CError_LongJump();
    }
    {
        unsigned long *w;
        w = (unsigned long *)&float_one;
        w[0] = 0;
        w[1] = 0x3ff00000;
        w = (unsigned long *)&data_0055e9e8;
        w[0] = 0x80000000;
        w[1] = 0x43300000;
        w = (unsigned long *)&data_0055e9f0;
        w[0] = 0;
        w[1] = 0x43300000;
    }
    p = galloc(16);
    data_005870e8 = p;
    data_00587ff8 = p;
    memset(p, 0, 16);
    data_00587ff8->key = fn_00441850((CPrepFileInfo *)(cprep_cu + 0x92), NULL);
    data_00587ff8->flags = -1;
    data_00580d80 = 0;
    output_buffer_length = object_storage_size = object_data_size = data_00580da4 = 0;
}

void fn_0048b500(void)
{
    BE_symbol_Init();
    BE_elf_InitSectionsAndFileSymbol();
}

void emit_dwarf_arguments_and_locals(void)
{
    ObjectList *entry;
    Object *object;
    Boolean operandsDebug;
    short gpr;
    Object *localObject;
    ObjectList *localEntry;
    ObjectList *reversed;
    Type *type;
    ObjectList *newEntry;
    Boolean onStack;
    short fpr;

    entry = arguments;
    gpr = 3;
    fpr = 1;
    while (entry != NULL) {
        object = entry->object;
        if (object->u.var.info->used != 0 && object->name->name[0] != '@') {
            onStack = 1;
            type = object->type;
            if ((((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8 ||
                  copts.operandsDebug != 0 && type->type == TYPEFLOAT && type->size != 4 &&
                      copts.incompatible_sfpe_double_params == 0) &&
                 gpr % 2 == 0)) {
                ++gpr;
            }
            if (type->type == TYPEFLOAT && (copts.operandsDebug == 0 || type->type != TYPEFLOAT)) {
                if (fpr <= 8) {
                    onStack = 0;
                }
                ++fpr;
            } else {
                if (gpr <= 10) {
                    onStack = 0;
                }
                if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                    ((operandsDebug = copts.operandsDebug) != 0 && type->type == TYPEFLOAT && type->size != 4)) {
                    gpr += 2;
                } else if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                           (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                           (operandsDebug != 0 && type->type == TYPEFLOAT && type->size == 4)) {
                    ++gpr;
                } else {
                    gpr += type->size >> 2;
                    if ((type->size & 3) != 0) {
                        ++gpr;
                    }
                }
            }
            DWARF_AddVar(object, onStack != 0 ? fn_004a9f70(0) : fn_004a9f90());
        }
        entry = entry->next;
    }
    reversed = NULL;
    entry = locals;
    while (entry != NULL) {
        newEntry = (ObjectList *)galloc(sizeof(ObjectList));
        newEntry->object = entry->object;
        newEntry->next = reversed;
        reversed = newEntry;
        entry = entry->next;
    }
    localEntry = reversed;
    while (localEntry != NULL) {
        localObject = localEntry->object;
        if (localObject->u.var.info->used != 0 && localObject->name->name[0] != '@') {
            DWARF_AddLocalVariable(localObject, fn_004a9f90());
        }
        localEntry = localEntry->next;
    }
}
