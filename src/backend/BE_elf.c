#define CERROR_FILE "BE_elf.c"
#include "compiler/common.h"
#include "compiler/BE_elf.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Switch.h"
#include "driver/CLIO.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include "driver/TargetPanels-eabi-ppc.h"
#include "driver/TextUtils.h"
#include "driver/libimp-eabi-ppc.h"
#include <string.h>

#pragma options align = mac68k
static UInt8 elfBigEndian;
static SInt32 symbol_order_count;
static unsigned char data_00580df6[128];
static char file_name[256];
static struct ObjGenSection **ordered_section_index;
#pragma options align = reset

/* zeros ElfPad copies: the buffer grows when a padding is longer */
static void *data_0055e528 = data_00580df6;
static int max_padding_size = 128;
void BE_elf_SetRelocationValue(ObjGenRelocation *selection, unsigned int value)
{
    IndexedEntry *entry;
    struct ObjGenSection *owner;
    ObjGenRelocation *current;
    current = selection;
    owner = current->section;
    entry = (IndexedEntry *)*owner->buffer.data;
    entry += current->index;
    entry->value = value;
}

ObjGenRelocation *BE_elf_AddRelocation(ObjGenSection *context, int offset, Object *object, void *source, int value,
                                       int adjustment)
{
    struct ObjGenSection *writer;
    ObjGenRelocation *entry;
    int kind;
    entry = NULL;
    writer = context->relocations;
    kind = 1;
    if (copts.fd5 != 0) {
        struct EABIRelocationRecord payload;
        writer->relocationCount += 1;
        entry = (ObjGenRelocation *)galloc(sizeof(*entry));
        entry->symbol = NULL;
        entry->source = 0;
        if (object != NULL) {
            entry->symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
        } else {
            entry->source = (SInt32)source;
        }
        offset = offset + adjustment;
        if (((offset + 3) & -4) - offset != 0) {
            kind = 24;
        }
        entry->kind = kind;
        entry->section = writer;
        entry->index = writer->relocationCount - 1;
        payload.offset = offset;
        payload.kind = kind & 255;
        payload.value = value;
        CompilerTools_AppendGListData(&writer->buffer, &payload, sizeof(payload));
        entry->next = relocation_list;
        relocation_list = entry;
    }
    return entry;
}

#define ELF_SECTION(p) ((ObjGenSection *)(void *)(p))

static void ElfAllocZero(SInt32 size)
{
    data_0055e528 = galloc(size);
    memset(data_0055e528, 0, size);
}

static inline SInt32 ElfPadSize(SInt32 alignment, int size)
{
    SInt32 n = size;

    n = (int)(n + alignment - 1);
    alignment--;
    n &= ~alignment;
    return n - data_00583ae8.buffer.size;
}

/* Pads the object file to ALIGNMENT and returns its new size. */
static SInt32 ElfPad(SInt32 alignment)
{
    SInt32 n;

    if ((n = ElfPadSize(alignment, data_00583ae8.buffer.size)) > max_padding_size) {
        ElfAllocZero(n);
        max_padding_size = n;
    }
    CompilerTools_AppendGListData(&data_00583ae8.buffer, data_0055e528, n);
    return data_00583ae8.buffer.size;
}

static inline void ElfPlaceSection(ObjGenSection *section)
{
    section->offset = ElfPad(section->maximumSize);
    section->size = section->buffer.size;
}

static inline void ElfAppend(GList *list)
{
    GList *src = list;
    SInt32 capacity;

    if (data_00583ae8.buffer.size + src->size > data_00583ae8.buffer.hndlsize) {
        data_00583ae8.buffer.hndlsize += src->size + data_00583ae8.buffer.growsize;
        capacity = data_00583ae8.buffer.hndlsize;
        if (!COS_ResizeHandle((struct StorageHandle *)data_00583ae8.buffer.data, capacity) && data_00587708)
            data_00587708();
    }
    memcpy(*data_00583ae8.buffer.data + data_00583ae8.buffer.size, *src->data, src->size);
    data_00583ae8.buffer.size += src->size;
}

static void ElfSwapSection(ObjGenSection *section, int words)
{
    if (words)
        swap_conversion_blocks(*section->buffer.data, section->buffer.size, CError_Internal, 1);
    else
        swap_tagged_records((void *)*section->buffer.data, section->buffer.size, CError_Internal, 1);
}

static SInt32 ElfSectionIndex(ObjGenSection *section)
{
    return section->index;
}

static SInt32 ElfIndex(ObjGenSection *section)
{
    return ElfSectionIndex(section);
}

static void ElfLinkSymbolTable(ObjGenSection *section)
{
    SInt32 index = ElfIndex(symbol_string_table_section);

    section->link = index;
    section->info = symbol_order_count + 1;
}

static void ElfLinkSection(ObjGenSection *section)
{
    SInt32 link, info;

    if (section->kind < 10 && section->relocations) {
        link = ElfIndex(ELF_SECTION(data_005884ce));
        section->relocations->link = link;
        info = ElfIndex(section);
        section->relocations->info = info;
    }
    if (section->kind == 9)
        section->link = ElfIndex(section->sectionData.attributes->sourceSection);
}

static void ElfLockSection(ObjGenSection *section)
{
    COS_LockHandle(section->buffer.data);
}

static int ElfBigEndian(void)
{
    return !copts.littleendian;
}

/* Writes the ELF object file: the header, the relocations' symbol indices, the section links, the section-name table,
   each section's contents at its alignment and the section header table, in the target's byte order. */
void fn_0049b920(void)
{
    ObjGenSection *s;
    Elf32Rela *rela;
    ObjGenRelocation *r;
    int i;
    SInt32 offset;
    SInt32 len;

    elfBigEndian = ElfBigEndian() != 0;
    if (data_0058849e) {
        DWARF_WriteDebugInfo();
        if (elfBigEndian) {
            ElfLockSection(ELF_SECTION(dwarf_line_section));
            ElfSwapSection(ELF_SECTION(dwarf_line_section), 1);
            COS_UnlockHandle(ELF_SECTION(dwarf_line_section)->buffer.data);
            ElfLockSection(ELF_SECTION(dwarf_info_section));
            ElfSwapSection(ELF_SECTION(dwarf_info_section), 0);
            COS_UnlockHandle(ELF_SECTION(dwarf_info_section)->buffer.data);
        }
    }
    build_ordered_section_index();
    if (!copts.f27)
        write_codewarrior_version_record();
    elf_header.ident[0] = 0x7f;
    elf_header.ident[1] = 'E';
    elf_header.ident[2] = 'L';
    elf_header.ident[3] = 'F';
    elf_header.ident[4] = 1;
    elf_header.ident[5] = ElfBigEndian() ? 2 : 1;
    elf_header.ident[6] = 1;
    elf_header.type = CTool_EndianConvertWord16(1);
    elf_header.machine = CTool_EndianConvertWord16(20);
    elf_header.version = CTool_EndianConvertWord32(1);
    elf_header.entry = 0;
    elf_header.phoff = 0;
    elf_header.shoff = 0;
    elf_header.flags = CTool_EndianConvertWord32(0x80000000);
    elf_header.ehsize = CTool_EndianConvertWord16(52);
    elf_header.phentsize = 0;
    elf_header.phnum = 0;
    elf_header.shentsize = CTool_EndianConvertWord16(40);
    elf_header.shnum = 0;
    elf_header.shstrndx = CTool_EndianConvertWord16(shstrtab_section->index);
    CompilerTools_AppendGListData(&data_00583ae8.buffer, &elf_header, 52);
    assign_symbol_order();
    if (relocation_list) {
        for (r = relocation_list; r; r = r->next) {
            rela = (Elf32Rela *)*r->section->buffer.data, rela += r->index;
            if (!r->symbol)
                r->symbol = BE_symbol_GetSectionSym((ObjGenSection *)r->source);
            {
                SInt32 info = (r->symbol->order << 8) + (r->kind & 255);
                rela->info = info;
            }
            ELF_Endian_ConvertThreeWords((unsigned int *)rela);
        }
    }
    s = ELF_SECTION(data_005884ce);
    ElfLinkSymbolTable(s);
    s = section_list->next;
    while (s) {
        ElfLinkSection(s);
        s = s->next;
    }
    build_symbol_string_table();
    offset = 0;
    AppendGListByte(&shstrtab_section->buffer, offset);
    offset++;
    for (i = 1, s = ordered_section_index[1]; s != NULL; s = ordered_section_index[++i]) {
        s->nameoff = offset;
        {
            char *name;
            GList *list;

            list = &shstrtab_section->buffer, name = s->name;
            CompilerTools_AppendGListData(list, name, len = strlen(name) + 1);
        }
        offset += len;
    }
    write_sym_nodes();
    {
        SInt32 index;
        ObjGenSection *section;

        for (index = 1, section = ordered_section_index[1]; section; section = ordered_section_index[++index]) {
            ElfPlaceSection(section);
            if (section->type != 8)
                ElfAppend(&section->buffer);
        }
    }
    elf_header.shoff = ElfPad(8), ELF_Endian_SwapElf32Section((Elf32Section *)&section_list->nameoff);
    CompilerTools_AppendGListData(&data_00583ae8.buffer, &section_list->nameoff, 40);
    elf_header.shnum++;
    {
        int index;

        for (index = 1, s = ordered_section_index[1]; s; s = ordered_section_index[++index]) {
            ELF_Endian_SwapElf32Section((Elf32Section *)&s->nameoff);
            CompilerTools_AppendGListData(&data_00583ae8.buffer, &s->nameoff, 40);
            elf_header.shnum++;
        }
    }
    elf_header.shoff = CTool_EndianConvertWord32(elf_header.shoff);
    ((ElfHeader *)*data_00583ae8.buffer.data)->shoff = elf_header.shoff;
    elf_header.shnum = CTool_EndianConvertWord16(elf_header.shnum),
    ((ElfHeader *)*data_00583ae8.buffer.data)->shnum = elf_header.shnum;
}

void write_codewarrior_version_record(void)
{
    struct {
        char name[11];
        UInt8 majorVersion;
        UInt8 minorVersion;
        UInt8 patchVersion;
        UInt8 reservedVersion;
        UInt8 versionFormat;
        UInt8 abiOption;
        UInt8 returnConvention;
        UInt8 languageVersion[2];
        UInt8 recordSize;
        UInt8 options;
        UInt8 reservedBytes[2];
        UInt32 reservedWords[5];
    } record;
    U16Bytes version;
    UInt8 returnConvention;
    UInt8 flags = 0;

    fn_0042c0c0();

    record.name[0] = 'C';
    record.name[1] = 'o';
    record.name[2] = 'd';
    record.name[3] = 'e';
    record.name[4] = 'W';
    record.name[5] = 'a';
    record.name[6] = 'r';
    record.name[7] = 'r';
    record.name[8] = 'i';
    record.name[9] = 'o';
    record.name[10] = 'r';
    record.majorVersion = 8;
    record.minorVersion = 2;
    record.patchVersion = 3;
    record.reservedVersion = 0;
    record.versionFormat = 1;

    record.abiOption = copts.usedatapool ? 1 : 0;
    if (copts.debugEnabled) {
        if (copts.operandsDebug)
            returnConvention = 1;
        else
            returnConvention = 2;
    } else {
        returnConvention = 0;
    }
    record.returnConvention = returnConvention;

    version.w = CTool_EndianConvertWord16(copts.processor);
    record.languageVersion[0] = version.b[0];
    record.languageVersion[1] = version.b[1];

    record.recordSize = sizeof(record);

    if (copts.incompatible_return_small_structs != 0)
        flags++;
    if (copts.incompatible_sfpe_double_params != 0)
        flags += 2;
    record.options = flags;

    record.reservedBytes[0] = 0;
    record.reservedBytes[1] = 0;
    record.reservedWords[0] = 0;
    record.reservedWords[1] = 0;
    record.reservedWords[2] = 0;
    record.reservedWords[3] = 0;
    record.reservedWords[4] = 0;

    CompilerTools_AppendGListData(&data_005884da->buffer, &record, sizeof(record));
}

void BE_elf_AppendGList(GList *dst, GList *src)
{
    if (dst->size + src->size > dst->hndlsize) {
        dst->hndlsize += src->size + dst->growsize;
        if (!COS_ResizeHandle((struct StorageHandle *)dst->data, dst->hndlsize) && data_00587708)
            data_00587708();
    }
    memcpy(*dst->data + dst->size, *src->data, src->size);
    dst->size += src->size;
}

void build_symbol_string_table(void)
{
    BE_SymNode *entry;
    unsigned int offset;
    unsigned int length;
    const char *name;
    GList *base;

    offset = 0;
    AppendGListByte(&symbol_string_table_section->buffer, offset);
    offset++;
    entry = BE_symbol_GetSymbolOrderTail()->orderNext;
    for (; entry != NULL; entry = entry->orderNext) {
        if ((entry->symbolKind & 15U) == 3U) {
            entry->stringOffset = 0U;
        } else {
            entry->stringOffset = offset;
            base = &symbol_string_table_section->buffer;
            name = (const char *)entry->nameData.hashName + 10U;
            length = strlen(name) + 1U;
            CompilerTools_AppendGListData(base, name, length);
            offset += length;
        }
    }
}

void write_sym_nodes(void)
{
    long padding;
    int position;
    int alignment;
    BE_SymNode *record;
    alignment = data_005884ce->maximumSize;
    position = data_00583ae8.buffer.size;
    padding = position;
    padding = (~(alignment - 1) & padding + alignment - 1) - position;
    if (padding > max_padding_size) {
        data_0055e528 = galloc(padding);
        memset(data_0055e528, 0, padding);
        max_padding_size = padding;
    }
    CompilerTools_AppendGListData(&data_00583ae8.buffer, data_0055e528, padding);
    record = BE_symbol_GetSymbolOrderTail();
    while (record != NULL) {
        if ((record->flags & 64) == 0) {
            record->attributes = record->sectionData.section->index;
            if (copts.f27 != 0) {
                record->flags = 0;
            } else {
                record->flags &= 63;
                record->alignment = CTool_EndianConvertWord32(record->alignment);
                CompilerTools_AppendGListData(&data_005884da->buffer, &record->alignment, 8);
            }
            ELF_Endian_ConvertSymbolRecord(&record->stringOffset);
            CompilerTools_AppendGListData(&data_005884ce->buffer, &record->stringOffset, 16);
        }
        record = record->orderNext;
    }
}

static inline ObjGenSection *fn_0049c140_inline1(void)
{
    ObjGenSection *section;
    if (".text"[0] != 0) {
        section = section_list;
        if (section != NULL) {
            do {
                if (section->context == NULL && memcmp(section->name, ".text", 6) == 0)
                    return section;
                section = section->next;
            } while (section != NULL);
        }
    }
    return NULL;
}

static inline ObjGenSection *findInitialSection(void)
{
    ObjGenSection *cursor;
    ObjGenSection *section;
    if (".text"[0] != 0) {
        section = cursor = section_list;
        if (cursor != NULL) {
            do {
                if (section->context == NULL) {
                    if (memcmp(section->name, ".text", 6) == 0)
                        return section;
                }
                section = section->next;
            } while (section != NULL);
        }
    }
    return NULL;
}
void build_ordered_section_index(void)
{
    int count;
    BE_SymNode *binding;
    ObjGenSection *middleHead;
    BE_SymNode *initialBinding;
    ObjGenSection *lastRecord;
    ObjGenSection *middleRecord;
    ObjGenSection *head;
    ObjGenSection *firstHead;
    ObjGenSection *firstRecord;
    ObjGenSection *lastHead;
    ObjGenSection *record;
    ObjGenSection *initialRecord;

    count = 1;
    head = (ObjGenSection *)(unsigned long)section_list;
    record = head;
    if (head != NULL) {
        do {
            record->index = 0;
            if (record->kind < 12 && record->buffer.size != 0)
                ++count;
            if (record->kind >= 12 && record->kind < 16)
                ++count;
            record = record->next;
        } while (record != NULL);
    }
    ordered_section_index = (ObjGenSection **)galloc((count + 2) * sizeof(int));
    ordered_section_index[0] = data_005884aa;
    count = 1;
    if (data_0058849e != 0) {
        initialRecord = findInitialSection();
        initialBinding = BE_symbol_GetSectionSym(initialRecord);
        if (initialRecord->buffer.size == 0) {
            ++count;
            ordered_section_index[1] = initialRecord;
            initialRecord->index = 1;
            initialBinding->symbolKind = 3;
            initialBinding->sectionData.section = initialRecord;
            initialBinding->alignment = initialRecord->maximumSize;
            initialBinding->kind = 258;
        }
    }
    firstHead = section_list->next;
    firstRecord = firstHead;
    if (firstHead != NULL) {
        do {
            if (firstRecord->kind < 10 && firstRecord->buffer.size != 0) {
                ordered_section_index[count] = firstRecord;
                firstRecord->index = count;
                ++count;
                binding = BE_symbol_GetSectionSym(firstRecord);
                binding->symbolKind = 3;
                binding->sectionData.section = firstRecord;
                binding->alignment = firstRecord->maximumSize;
                binding->kind = 258;
            }
            firstRecord = firstRecord->next;
        } while (firstRecord != NULL);
    }
    middleHead = section_list->next;
    middleRecord = middleHead;
    if (middleHead != NULL) {
        do {
            if (middleRecord->kind >= 10 && middleRecord->kind < 12 && middleRecord->buffer.size != 0) {
                ordered_section_index[count] = middleRecord;
                middleRecord->index = count;
                ++count;
            }
            middleRecord = middleRecord->next;
        } while (middleRecord != NULL);
    }
    lastHead = section_list->next;
    lastRecord = lastHead;
    if (lastHead != NULL) {
        do {
            if (lastRecord->kind >= 12 && lastRecord->kind < 16) {
                ordered_section_index[count] = lastRecord;
                lastRecord->index = count;
                ++count;
            }
            lastRecord = lastRecord->next;
        } while (lastRecord != NULL);
    }
    ordered_section_index[count] = NULL;
    data_005884de->index = -14;
    data_005884e2->index = -15;
    data_005884ae->flags = 2;
    data_005884ae->alignment = 2;
    data_005884b2->flags = 2;
    data_005884b2->alignment = 2;
}

void BE_elf_FreeSectionBuffers(void)
{
    ObjGenSection *section;

    for (section = (ObjGenSection *)*(int *)&section_list; section != NULL; section = section->next) {
        if (section->initialSize != 0)
            FreeGList(&section->buffer);
    }
}

/* Record returned by BE_symbol_CreateSymNode; unobserved regions remain opaque. */
void BE_elf_InitSectionsAndFileSymbol(void)
{
    BE_SymNode *record;

    data_0058847a = 1;
    memset(&data_0058849e, 0, 82);
    ObjGen_PPC_EABI_InitSections();
    COS_FileGetFSSpecInfo(&((CPrepCU *)cprep_cu)->mainFile, NULL, NULL, file_name);
    CLIO_ConvertPascalToCString(file_name);

    record = BE_symbol_CreateSymNode(GetHashNameNodeExport(file_name));
    record->symbolKind = 4;
    record->sectionData.section = data_005884e2;
    record->alignment = 1;
    record->kind = 258;
    data_005884a0 = (long)record;

    relocation_list = NULL;
    data_0055e528 = data_00580df6;
    max_padding_size = 128;
}

void BE_elf_SetDeclSection(char *name, DeclInfo *declaration)
{
    ObjGenSection *section;
    ObjGenSection *head;
    InterruptGenerationRecord *record = NULL;
    UInt16 targetIndex;

    if (declaration->section != 0) {
        CError_FATAL(489);
    } else {
        record = galloc(sizeof(*record));
        memclrw(record, sizeof(*record));
        record->next = interrupt_generation_records;
        interrupt_generation_records = record;
        record->id = declaration->section = --data_005876b4;
    }

    head = section_list;
    for (section = head->next; section != NULL; section = section->next) {
        if (section->kind == 6 || section->kind == 0x10 || (UInt8)(section->kind - 1) <= 2) {
            if (strcmp(name, section->name) == 0) {
                if (section->kind == 6 || section->kind == 0x10) {
                    PPCError_ReportError(0x8f, name);
                    return;
                }
                targetIndex = ObjGen_PPC_EABI_GetSectionIndex(declaration->section);
                if (targetIndex != 0 && targetIndex != section->header->index) {
                    PPCError_ReportError(0x81, declaration->name->name, section->name);
                    return;
                }
                record->sectionIndex = section->header->index;
                return;
            }
        }
    }
    PPCError_ReportError(0x93, name);
}

void fn_0049c510(char *value1, UInt8 value2, SInt8 value3, SInt32 value4)
{
    BE_elf_CreateSection(value1, value2, value3, value4, NULL, 0, NULL);
}

static SectionRec *MakeRecA(void *f6, UInt8 f2, UInt8 f3)
{
    SectionRec *a;
    a = (SectionRec *)galloc(0xe);
    memset(a, 0, 0xe);
    a->sectionName = (char *)f6;
    a->linkedSectionName = NULL;
    a->far_reloc = f2;
    a->near_reloc = f3;
    a->typebits = 0;
    return a;
}

ObjGenSection *BE_elf_0049c540(ObjGenSection *input, SInt32 context)
{
    ObjGenSection *relatedRecord = NULL;
    SectionRec *object;
    ObjGenSection *record;
    struct ObjGenSymbolLink *symbolLink = NULL;

    object = MakeRecA(input->name, input->header->kind, input->header->alignment);
    if (input->symbolLink != NULL) {
        symbolLink = galloc((sizeof(*symbolLink) + 1) & ~1);
        memset(symbolLink, 0, (sizeof(*symbolLink) + 1) & ~1);
    }
    record = BE_elf_CreateSection(object->sectionName, input->kind, 4, 0xc8, object, input->flags, symbolLink);
    record->context = (ObjGenSection *)context;
    if (input->relocations != NULL) {
        relatedRecord = BE_elf_CreateSection(input->relocations->name, 0xb, 4, 0x64, NULL, 0, NULL);
        relatedRecord->relocations = record;
    }
    record->relocations = relatedRecord;
    record->header = BE_elf_GetOrCreateSectionSymbolAttributes(record, input->header->kind, input->header->alignment,
                                                               input->header->linkedSymbol, input->header->auxiliary);
    return record;
}

SectionRec *BE_elf_CreateSectionRec(char *nspace, char *name, unsigned int far_reloc, unsigned int near_reloc,
                                    unsigned int typebits)
{
    SectionRec *section;
    section = (SectionRec *)galloc(sizeof(SectionRec));
    memset(section, 0, sizeof(SectionRec));
    section->sectionName = nspace;
    section->linkedSectionName = name;
    section->far_reloc = far_reloc;
    section->near_reloc = near_reloc;
    section->typebits = typebits;
    return section;
}

static inline ObjGenSection *BE_elf_0049c670_inline1(char *v4)
{
    ObjGenSection *v5;
    ObjGenSection *v5s;
    if (v4 == NULL || *v4 == 0) {
        return NULL;
    }
    v5 = v5s = section_list;
    if (v5s != NULL) {
        do {
            if (((SInt32 *)v5)[24] == 0 && strcmp((char *)v5->name, (char *)v4) == 0) {
                return v5;
            }
            v5 = v5->next;
        } while ((int)v5 != 0);
    }
    return NULL;
}

ObjGenSection *BE_elf_CreateSectionWithRelocations(SectionRec *rawObject, short kind, char flags, int size,
                                                   int relocationSize, unsigned char attributes, char allocateData,
                                                   char createEntry)
{
    ObjGenSection *previousSection = NULL;
    ObjGenSection *section;
    ObjGenSection *currentSection;
    ObjGenSection *relocationSection;
    ObjGenSection *namedSection = NULL;
    struct ObjGenSymbolLink *sectionData = NULL;
    char *relocationName;

    relocationName = galloc(strlen(rawObject->sectionName) + strlen(".rela") + 1);
    strcpy(relocationName, ".rela");
    strcat(relocationName, rawObject->sectionName);
    if (allocateData) {
        sectionData = galloc(10);
        memset(sectionData, 0, 10);
    }
    section = BE_elf_CreateSection(rawObject->sectionName, kind, flags, size, rawObject, attributes, sectionData);
    relocationSection = BE_elf_CreateSection(relocationName, 11, 4, relocationSize, NULL, 0, NULL);
    section->relocations = relocationSection;
    relocationSection->relocations = section;
    if (rawObject->linkedSectionName) {
        namedSection = BE_elf_0049c670_inline1(rawObject->linkedSectionName);
        if (!namedSection) {
            if (allocateData) {
                sectionData = galloc(10);
                memset(sectionData, 0, 10);
            }
            namedSection = BE_elf_CreateSection(rawObject->linkedSectionName, 6, flags, size, rawObject,
                                                attributes & ~4, sectionData);
        }
        currentSection = data_005884de;
        if (namedSection == currentSection) {
            namedSection = data_section_linked_symbol;
            previousSection = currentSection;
        }
    }
    if (createEntry) {
        section->header = BE_elf_GetOrCreateSectionSymbolAttributes(
            section, rawObject->far_reloc, rawObject->near_reloc, namedSection, previousSection);
    }
    return section;
}

ObjGenSection *BE_elf_FindSection(const char *name, SInt32 hashval)
{
    ObjGenSection *entry;
    ObjGenSection *list;

    if (name == NULL || *name == 0)
        return NULL;
    entry = list = section_list;
    if (list) {
        do {
            if ((SInt32)entry->context == hashval && strcmp(entry->name, name) == 0)
                return entry;
            entry = entry->next;
        } while (entry);
    }
    return NULL;
}

static inline void BE_elf_SetEABISections(ObjGenSection *node, SectionRec *flags)
{
    if (flags->typebits & 1)
        copts.textSection = (ObjGenSection *)node;
    if (flags->typebits & 2)
        copts.dataSection = (ObjGenSection *)node;
    if (flags->typebits & 4)
        copts.bssSection = (ObjGenSection *)node;
    if (flags->typebits & 8)
        copts.smallDataSection = (ObjGenSection *)node;
    if (flags->typebits & 16)
        copts.smallBSSSection = (ObjGenSection *)node;
}

ObjGenSection *BE_elf_CreateSection(char *name, UInt8 kind, SInt8 attribute, SInt32 initialSize, SectionRec *flags,
                                    UInt8 alignment, struct ObjGenSymbolLink *symbolLink)
{
    ObjGenSection *head;
    ObjGenSection **list;
    ObjGenSection *node;
    SInt32 size;

    list = NULL;
    node = galloc(sizeof(ObjGenSection));
    memset(node, 0, sizeof(*node));
    sectionHeaderCount++;
    node->name = name;
    node->kind = kind;
    node->initialSize = initialSize;
    if (flags != NULL && kind != 6) {
        BE_elf_SetEABISections(node, flags);
    }
    node->symbolLink = symbolLink;
    node->type = (kind == 6) ? 8 : 1;
    node->flags = alignment;
    node->alignment = alignment;
    node->maximumSize = attribute;
    if ((size = node->initialSize) != 0 && InitGList(&node->buffer, size) != 0)
        CError_LongJump();

    switch (kind) {
        case 0:
            CError_ASSERT(272, data_005884aa == NULL);
            node->type = 0;
            node->link = 0;
            list = &data_005884aa;
            break;
        case 1:
            break;
        case 2:
            CError_ASSERT(281, data_005884ae == NULL);
            list = &data_005884ae;
            break;
        case 3:
            CError_ASSERT(285, data_005884b2 == NULL);
            list = &data_005884b2;
            break;
        case 4:
            CError_ASSERT(289, data_005884b6 == NULL);
            list = &data_005884b6;
            break;
        case 5:
            CError_ASSERT(293, data_005884ba == NULL);
            list = &data_005884ba;
            break;
        case 6:
            break;
        case 7:
            CError_ASSERT(297, dwarf_info_section == NULL);
            list = &dwarf_info_section;
            node->entrySize = 1;
            break;
        case 8:
            CError_ASSERT(302, dwarf_line_section == NULL);
            list = &dwarf_line_section;
            node->entrySize = 1;
            break;
        case 10:
            node->type = 9;
            node->entrySize = 8;
            list = &data_005884ca;
            break;
        case 11:
            node->type = 4;
            node->entrySize = 0xc;
            list = &data_005884ca;
            break;
        case 12:
            CError_ASSERT(317, data_005884ce == NULL);
            node->type = 2;
            node->entrySize = 0x10;
            list = &data_005884ce;
            break;
        case 13:
            CError_ASSERT(323, symbol_string_table_section == NULL);
            node->type = 3;
            node->entrySize = 1;
            list = &symbol_string_table_section;
            break;
        case 14:
            CError_ASSERT(329, shstrtab_section == NULL);
            node->type = 3;
            node->entrySize = 1;
            list = &shstrtab_section;
            break;
        case 15:
            CError_ASSERT(335, data_005884da == NULL);
            node->entrySize = 1;
            list = &data_005884da;
            break;
        case 16:
            CError_ASSERT(340, data_005884de == NULL);
            list = &data_005884de;
            break;
        case 17:
            CError_ASSERT(344, data_005884e2 == NULL);
            list = &data_005884e2;
            break;
        case 9:
            node->type = 0xca2a82c2;
            node->entrySize = 1;
            break;
        default:
            CError_FATAL(355);
    }

    head = section_list;
    if (head == NULL) {
        section_list = node;
    } else {
        ObjGenSection *tail;
        for (tail = head; tail->next; tail = tail->next)
            ;
        tail->next = node;
    }
    if (list != NULL) {
        ObjGenSection *head;
        if ((head = *list) == NULL) {
            *list = node;
        } else {
            ObjGenSection *tail;
            for (tail = head; tail->nextInGroup; tail = tail->nextInGroup)
                ;
            tail->nextInGroup = node;
        }
    }
    return node;
}

SectionSymbolAttributes *BE_elf_GetOrCreateSectionSymbolAttributes(ObjGenSection *identifier, UInt8 kind,
                                                                   UInt8 attributes, ObjGenSection *value,
                                                                   ObjGenSection *size)
{
    SectionSymbolAttributes *record;
    SectionSymbolAttributes *tail;

    record = find_section_symbol_attributes(identifier, kind, attributes, value, size);
    if (record != NULL)
        return record;

    record = (SectionSymbolAttributes *)galloc(sizeof(SectionSymbolAttributes));
    memset(record, 0, sizeof(SectionSymbolAttributes));

    record->target = identifier;
    record->kind = kind;
    record->alignment = attributes;
    record->linkedSymbol = value;
    record->auxiliary = size;

    if (data_0058847a == 0x7FFF)
        PPCError_ReportError(0xa4, 0x7fff);

    record->index = data_0058847a;
    data_0058847a++;
    data_005884ee++;

    tail = section_symbol_attributes;
    if (tail == NULL) {
        section_symbol_attributes = record;
    } else {
        SectionSymbolAttributes *last = tail;
        while (last->next != NULL)
            last = last->next;
        last->next = record;
    }
    return record;
}

SectionSymbolAttributes *find_section_symbol_attributes(ObjGenSection *key, UInt8 byte10, UInt8 byte11,
                                                        ObjGenSection *key12, ObjGenSection *key16)
{
    SectionSymbolAttributes *entry;
    SectionSymbolAttributes *head;
    entry = head = section_symbol_attributes;
    if (head) {
        do {
            if (entry->target == key && entry->kind == (unsigned char)byte10 &&
                entry->alignment == (unsigned char)byte11 && entry->linkedSymbol == key12 && entry->auxiliary == key16)
                break;
            entry = entry->next;
        } while (entry);
    }
    return entry;
}

void assign_symbol_order(void)
{
    struct BE_SymNode *tail;
    struct BE_SymNode *record;
    SInt32 count = 0;

    tail = galloc(sizeof(*tail));
    memset(tail, 0, sizeof(*tail));
    tail->attributes = 0;
    tail->sectionData.section = data_005884aa;
    symbol_order_tail = tail;
    for (record = BE_symbol_ResetSymbolTail(); record != NULL; record = BE_symbol_AdvanceSymbolTail()) {
        if ((record->symbolKind >> 4) == 0 && (record->flags & 0x40) == 0) {
            count++;
            tail->orderNext = record;
            tail = record;
            record->order = count;
        }
    }
    symbol_order_count = count;
    for (record = BE_symbol_ResetSymbolTail(); record != NULL; record = BE_symbol_AdvanceSymbolTail()) {
        if ((record->symbolKind >> 4) > 0) {
            count++;
            tail->orderNext = record;
            tail = record;
            record->order = count;
        }
    }
}

void BE_elf_AlignRecord(GList *record, SInt32 alignment)
{
    SInt32 paddingSize;

    paddingSize = record->size;
    paddingSize = (paddingSize + alignment - 1) & ~(alignment - 1);
    paddingSize -= record->size;
    if (paddingSize > max_padding_size) {
        data_0055e528 = galloc(paddingSize);
        memset(data_0055e528, 0, paddingSize);
        max_padding_size = paddingSize;
    }
    CompilerTools_AppendGListData(record, data_0055e528, paddingSize);
}
