#ifndef COMPILER_BE_ELF_H
#define COMPILER_BE_ELF_H

#include "compiler/common.h"
#include "compiler/CompilerTools.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An ELF relocation with addend: BE_elf_AddRelocation appends them to a relocation section; fn_0049b920 sets each one's
   symbol index when the object file is written. */
struct Elf32Rela {
    UInt32 offset;
    UInt32 info;
    SInt32 addend;
};
#pragma options align = mac68k
struct ElfHeader {
    UInt8 ident[16];
    UInt16 type;
    UInt16 machine;
    UInt32 version;
    UInt32 entry;
    UInt32 phoff;
    UInt32 shoff;
    UInt32 flags;
    UInt16 ehsize;
    UInt16 phentsize;
    UInt16 phnum;
    UInt16 shentsize;
    UInt16 shnum;
    UInt16 shstrndx;
};
#pragma options align = reset
#pragma pack(push, 1)
struct IndexedEntry {
    unsigned int reserved[2];
    unsigned int value;
};
#pragma pack(pop)
#pragma options align = mac68k
struct SectionRec {
    UInt8 flags;
    UInt8 typebits;
    UInt8 far_reloc;
    UInt8 near_reloc;
    UInt8 mode;
    UInt8 alignmentPadding;
    char *sectionName;
    char *linkedSectionName;
};
#pragma options align = reset
#pragma pack(push, 1)
union U16Bytes {
    UInt16 w;
    UInt8 b[2];
};
#pragma pack(pop)
extern void BE_elf_SetDeclSection(char *name, DeclInfo *file);
extern void fn_0049c510(char *value1, UInt8 value2, SInt8 value3, SInt32 value4);
extern ObjGenSection *fn_0049c540(ObjGenSection *input, SInt32 context);
extern SectionRec *BE_elf_CreateSectionRec(char *nspace, char *name, unsigned int datatype, unsigned int unk03,
                                           unsigned int access);
extern ObjGenSection *BE_elf_FindSection(const char *name, SInt32 hashval);
extern ObjGenSection *BE_elf_CreateSection(char *name, UInt8 kind, SInt8 attribute, SInt32 initialSize,
                                           SectionRec *flags, UInt8 alignment, struct ObjGenSymbolLink *symbolLink);
extern ObjGenSection *BE_elf_CreateSectionWithRelocations(SectionRec *rawObject, short kind, char flags, int size,
                                                          int relocationSize, unsigned char attributes,
                                                          char allocateData, char createEntry);
extern void write_codewarrior_version_record(void);
extern void BE_elf_AppendGList(GList *dst, GList *src);
extern void build_symbol_string_table(void);
extern void write_sym_nodes(void);
extern void BE_elf_FreeSectionBuffers(void);
extern void BE_elf_InitSectionsAndFileSymbol(void);
extern SectionSymbolAttributes *BE_elf_GetOrCreateSectionSymbolAttributes(ObjGenSection *identifier, UInt8 kind,
                                                                          UInt8 attributes, ObjGenSection *value,
                                                                          ObjGenSection *size);
extern SectionSymbolAttributes *find_section_symbol_attributes(ObjGenSection *key, UInt8 byte10, UInt8 byte11,
                                                               ObjGenSection *key12, ObjGenSection *key16);
extern void assign_symbol_order(void);
extern void BE_elf_AlignRecord(GList *record, SInt32 alignment);
extern void build_ordered_section_index(void);
extern struct ElfHeader elf_header;
extern void fn_0049b920(void);
extern UInt16 data_0058847a;
extern SInt32 data_005884a0;
extern struct ObjGenSection *section_list;
extern struct ObjGenSection *data_005884ae;
extern struct ObjGenSection *data_005884b2;
extern struct ObjGenSection *data_005884ca;
extern struct ObjGenSection *data_005884ce;
extern struct ObjGenSection *symbol_string_table_section;
extern struct ObjGenSection *shstrtab_section;
extern struct ObjGenSection *data_005884da;
extern struct ObjGenSection *data_005884de;
extern struct ObjGenSection *data_005884e2;
extern UInt16 data_005884ee;
extern void BE_elf_SetRelocationValue(ObjGenRelocation *selection, unsigned int value);
extern ObjGenRelocation *BE_elf_AddRelocation(struct ObjGenSection *context, int offset, Object *object, void *source,
                                              int value, int adjustment);
extern void *symbol_order_tail;
extern SInt32 data_005876b4;
extern struct ObjGenSection *data_005884b6;
extern struct ObjGenSection *data_005884ba;
extern struct ObjGenSection *dwarf_info_section;
extern struct ObjGenSection *dwarf_line_section;
extern union {
    GList buffer;
    SInt32 data;
} data_00583ae8;

#ifdef __cplusplus
}
#endif

#endif
