#ifndef COMPILER_OBJGEN_PPC_EABI_H
#define COMPILER_OBJGEN_PPC_EABI_H

#include "compiler/common.h"
#include "compiler/CompilerTools.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SecKind { SK_NONE = 0, SK_1 = 1, SK_5 = 5, SK_SDATA = 6, SK_SDATA2 = 7, SK_SDATA0 = 8 } SecKind;
struct EABIRelocationRecord {
    SInt32 offset;
    SInt32 kind;
    SInt32 value;
};
struct RelocationRecord {
    SInt32 offset;
    SInt32 kind;
    SInt32 addend;
};
struct BufferUpdate {
    struct BufferUpdate *next;
    struct BE_SymNode *target;
    Boolean active;
    Boolean suppressed;
};
#pragma options align = mac68k
struct CGList {
    struct SectionAttributeNode
        *head; /* 0x00: ObjGen_PPC_EABI_AddSectionAttribute initializes the attribute list head */
    struct SectionAttributeNode
        *tail; /* 0x04: ObjGen_PPC_EABI_AddSectionAttribute appends nodes and merges the last object's flags */
    struct ObjGenSection *
        sourceSection; /* 0x08: ObjGen_PPC_EABI_AddSectionAttribute stores the object's section; ElfLinkSection reads its ELF index */
};
#pragma options align = reset
struct CNameNode {
    struct CNameNode *next;
    HashNameNode *key;
    SInt32 uid;
    SInt32 flags;
};
#pragma options align = mac68k
struct InterruptGenerationRecord {
    struct InterruptGenerationRecord *next;
    UInt16 sectionIndex;
    UInt16 id;
    UInt8 enable;
    UInt8 SRR;
    UInt8 DAR;
    UInt8 DSISR;
    UInt8 unused[2];
};
#pragma options align = reset
struct ObjGenRelocation {
    struct ObjGenRelocation *next;
    SInt32 kind;
    struct ObjGenSection *section;
    SInt32 source;
    SInt32 index;
    struct BE_SymNode *symbol;
};
#pragma options align = mac68k
struct ObjGenSection {
    struct ObjGenSection *next;
    struct ObjGenSection *nextInGroup;
    UInt8 kind;
    UInt8 flags;
    SInt16 index;
    SInt32 relocationCount;
    struct ObjGenSymbolLink *symbolLink;
    struct SectionSymbolAttributes *header;
    char *name;
    SInt32 initialSize;
    GList buffer;
    SInt32 nameoff;
    SInt32 type;
    UInt32 alignment;
    SInt32 address;
    SInt32 offset;
    SInt32 size;
    SInt32 link;
    SInt32 info;
    UInt32 maximumSize;
    SInt32 entrySize;
    struct ObjGenSection *relocations; /* 0x58: BE_elf_AddRelocation writes relocation entries to this section */
    struct DwarfFunctionState *debugState;
    struct ObjGenSection *context; /* 0x60: BE_elf_0049c540 stores the section context; findInitialSection tests it */
    struct BE_SymNode *sym;
    struct ObjGenSection *output;
    union {
        struct CGList *
            attributes; /* 0x6c: kind == 9 in ElfLinkSection; ObjGen_PPC_EABI_AddSectionAttribute creates the attribute list */
        struct SerializedFormatLink
            *serialized; /* 0x6c: ObjGen_PPC_EABI_EmitSerializedFormat reads the serialized-format output section */
    } sectionData;
};
#pragma options align = reset
#pragma pack(push, 1)
struct ObjGenSymbolLink {
    struct Object *
        object; /* 0x00: BE_symbol_GetFunctionSymbolLinkData returns this Object; TOC_HasObjectReferenceWithoutExpression reads it */
    struct BE_SymNode *symbol; /* 0x04: ObjGen_PPC_EABI.c uses this as the section symbol for relocations */
    UInt8
        value; /* 0x08: BE_symbol.c tests and sets this flag; ObjGen_PPC_EABI_ClearSectionSymbolLinkValues clears it */
};
#pragma pack(pop)
struct OutputBufferState {
    UInt8 unknown_00[0x10];
    SInt32 length1;
    SInt32 length2;
    SInt32 length3;
    UInt8 unknown_1c[0x18];
    SInt32 length4;
};

#pragma pack(push, 1)
struct SectionAttributeNode {
    struct SectionAttributeNode *next;
    struct Object *object;
    SInt32 f08;
    SInt32 f0c;
    SInt32 f10;
    UInt8 flags;
    UInt8 f15;
};
#pragma pack(pop)
struct SectionSymbolAttributes {
    struct SectionSymbolAttributes *next;
    struct ObjGenSection *target;
    unsigned short index;
    SecKind kind;
    SecKind alignment;
    struct ObjGenSection *linkedSymbol;
    struct ObjGenSection *auxiliary;
};
struct SerializedFormat {
    char reserved0[8];
    struct SerializedValueList *
        values; /* 0x08: ObjGen_PPC_EABI_EmitSerializedFormat traverses values for wide output; short output reads them as SerializedValueList. */
    struct SerializedValueList
        *last; /* 0x0c: ObjGen_PPC_EABI_AppendOutputEntry appends after the last value and updates the tail. */
    int count; /* 0x10: ObjGen_PPC_EABI_EmitSerializedFormat writes the serialized entry count. */
    char kind; /* 0x14: ObjGen_PPC_EABI_EmitSerializedFormat emits only kind == 0. */
};
struct SerializedFormatLink {
    char reserved0[4];
    struct SerializedFormat *
        format; /* 0x04: ObjGen_PPC_EABI_EmitSerializedFormat reads the format; ObjGen_PPC_EABI_AppendOutputEntry appends its values. */
};
struct SerializedLocation {
    int offset;
    int kind;
    int reserved;
};
struct SerializedShortHeader {
    char kind;
    char count;
    short index;
    int reserved;
};

struct SerializedValueList {
    struct SerializedValueList
        *next; /* 0x00: ObjGen_PPC_EABI_EmitSerializedFormat traverses the value list in both output widths. */
    int value; /* 0x04: ObjGen_PPC_EABI_EmitSerializedFormat emits the value as a long or its low short as a word. */
};
struct SerializedWideHeader {
    char kind;
    char flags;
    short count;
    int index;
    int reserved;
};
struct InterruptGenerationRecord;
extern void ObjGen_PPC_EABI_AddSectionAttribute(Object *a, UInt8 b);
extern void create_main_file_object(void);
extern InterruptGenerationRecord *ObjGen_PPC_EABI_GetInterruptInfo(Object *obj);
extern InterruptGenerationRecord *fn_00488750(Object *object, BE_SymNode *linkage);
extern GList *ObjGen_PPC_EABI_GetSectionBuffer(ObjGenSection *a0);
extern void ObjGen_PPC_EABI_EmitDescriptorWithRelocations(Object *obj, SInt32 value, void *data, UInt32 size,
                                                          ObjGenRelocationRequest *list);
extern void ObjGen_PPC_EABI_RestoreFunctionState(void);
extern SInt16 ObjGen_PPC_EABI_SetupFunctionSection(Object *param);
extern void ObjGen_PPC_EABI_00488ee0(SInt32 entry, SInt32 value);
extern void emit_relocation(SInt32 op, SInt32 offset, Object *obj, ObjGenSection *ctx, SInt32 value);
extern void ObjGen_PPC_EABI_SetSymbolOffset(Object *object, int offset);
extern void fn_00489360(Object *arg1, int arg2, void *arg3);
extern UInt16 ObjGen_PPC_EABI_GetSectionIndex(short reg);
extern void ObjGen_PPC_EABI_AppendOutputEntry(ObjGenSection *context, unsigned int value);
extern void ObjGen_PPC_EABI_EmitObjectRelocation(Object *object);
extern ObjGenSection *fn_004892a0(Object *object, int size);
extern void ObjGen_PPC_EABI_EmitSwitchTable(Object *gl, Object *func);
extern void ObjGen_PPC_EABI_EmitFloatObject(Object *node);
extern void emit_object_data_and_relocations(Object *func, const char *data, RelocationList *list, SInt32 size,
                                             Boolean flag);
extern void allocate_object_storage(Object *obj, SInt32 size, Boolean flag);
extern void report_section_permission_conflict(Object *function, ObjGenSection *qualInfo, void *name, UInt32 flags);
extern SInt16 ObjGen_PPC_EABI_MapObjectSectionCode(Object *obj);
extern void ObjGen_PPC_EABI_SetObjectSection(Object *obj, SInt32 size, Boolean flag);
extern short ObjGen_PPC_EABI_GetHeaderIndex(short kind);
extern void ObjGen_PPC_EABI_InitSections(void);
extern void ObjGen_PPC_EABI_UpdateSectionHeaders(void);
extern void ObjGen_PPC_EABI_BuildSectionHeaderTable(void);
extern Boolean ObjGen_PPC_EABI_IsInvalidAbsName(char *name);
extern Boolean ObjGen_PPC_EABI_0048ac10(Object *obj);
extern Boolean PCodeUtilities_Require(Object *obj);
extern SInt32 ObjGen_PPC_EABI_GetSectionAlignmentOrKind(Object *obj);
extern char *ObjGen_PPC_EABI_GetSectionName(struct ObjGenSection *descriptor);
extern unsigned int ObjGen_PPC_EABI_EmitObject(Object *object, const void *context, RelocationList *value,
                                               unsigned int flags);
extern void ObjGen_PPC_EABI_EmitObjectWithDebugEntry(Object *object, const void *data, RelocationList *attributes,
                                                     unsigned int alignment);
extern void ObjGen_PPC_EABI_ClearSectionSymbolLinkValues(void);
extern void fn_0048b090(HashNameNode *oldid, HashNameNode *newid);
extern void fn_0048b160(HashNameNode *key, int param_2, int param_3);
extern void fn_0048b1e0(HashNameNode *arg1);
extern void ObjGen_PPC_EABI_SetObjectSectionIndex(Object *object);
extern void fn_004889b0(ObjGenSection *context, int section, Object *object, UInt8 flags, unsigned int options);
extern SInt32 output_buffer_length;
extern UInt8 DAT_0058849e;
extern struct CNameNode *data_005870e8;
extern struct ObjGenRelocation *relocation_list;
extern struct CNameNode *data_00587ff8;
extern short sectionHeaderCount;
extern struct ObjGenSection *data_section_linked_symbol;
extern struct SectionSymbolAttributes *section_symbol_attributes;
extern ObjGenSection *select_object_section(Object *obj, SInt32 section, Boolean usePrimary, Boolean force,
                                            Boolean options);
extern void ObjGen_PPC_EABI_SetSectionOptions(SectionRec *section);
extern unsigned char ObjGen_PPC_EABI_IsSpecialSectionName(char *name);
extern void ObjGen_PPC_EABI_EmitSerializedFormat(Object *key, int index);
extern void fn_0048b2c0(void);
extern void fn_0048b2d0(void);
extern void fn_0048b2e0(void);
extern void fn_0048b500(void);
extern void emit_dwarf_arguments_and_locals(void);
extern void ObjGen_PPC_EABI_FinalizeOutputBuffers(void);
extern void fn_0048b3f0(void);
struct ObjGenRelocation;
struct ObjGenRelocation;
struct ObjGenSection;

#ifdef __cplusplus
}
#endif

#endif
