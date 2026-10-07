#ifndef COMPILER_BE_SYMBOL_H
#define COMPILER_BE_SYMBOL_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct BE_SymNode {
    struct {
        HashNameNode *hashName;
    } nameData;
    struct {
        struct ObjGenSection *section;
    } sectionData;
    UInt32 stringOffset; /* 0x08: BE_elf.c string-table builder stores offset; write_sym_nodes emits ELF st_name */
    SInt32 offset;
    SInt32 size;
    UInt8 symbolKind;
    UInt8 flags;
    UInt16 attributes;
    struct BE_SymNode *next;
    struct BE_SymNode *orderNext;
    SInt32 order;
    SInt32 alignment;
    char linkageKind;
    char linkageFlags;
    char reserved2a[2];
    SInt16 kind;
    struct InterruptGenerationRecord *interruptInfo;
};
#pragma options align = reset
extern BE_SymNode *BE_symbol_SetupObjectSymbol(Object *object, int size, ObjGenSection *section);
extern Boolean BE_symbol_004913b0(Object *obj);
extern Object *BE_symbol_GetFunctionSymbolLinkData(Object *func);
extern unsigned int BE_symbol_GetOffset(BE_SymNode *a0);
extern unsigned int BE_symbol_CreateSectionSymbol(ObjGenSection *input);
extern BE_SymNode *BE_symbol_GetOrCreateFunctionObjectSymbol(Object *arg);
extern struct BE_SymNode *BE_symbol_GetSymbolOrderTail(void);
extern BE_SymNode *BE_symbol_DefineObjectSymbol(Object *object, int offset, ObjGenSection *value);
extern BE_SymNode *BE_symbol_AdvanceSymbolTail(void);
extern void BE_symbol_Init(void);
extern BE_SymNode *BE_symbol_CreateSymNode(void *value);
extern BE_SymNode *BE_symbol_GetSectionSym(struct ObjGenSection *ctx);
extern BE_SymNode *BE_symbol_004918f0(Object *symbol, struct ObjGenSection *value);
extern struct BE_SymNode *be_symbol_list;
extern struct BE_SymNode *symbol_tail;
extern struct ObjGenSection *data_005884aa;
extern BE_SymNode *BE_symbol_ResetSymbolTail(void);

#ifdef __cplusplus
}
#endif

#endif
