#ifndef COMPILER_COBJCMODERN_H
#define COMPILER_COBJCMODERN_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HashEntry {
    struct HashEntry *next;
    struct Object *obj;
    HashNameNode *name;
    struct SelectorMethod *methods;
};
#pragma options align = mac68k
struct NamedObjectCacheEntry {
    struct NamedObjectCacheEntry *next;
    struct Object *object;
    UInt8 *name;
    short kind;
};
#pragma options align = reset
struct ObjCSymbolTable {
    int word0;
    int word4;
    unsigned short count1;
    unsigned short count2;
    int definitions[1];
};
#pragma options align = mac68k
struct ObjcModule {
    int version;
    int size;
    int name;
    int symtab;
};
#pragma options align = reset
extern ENode *CObjCModern_0050a000(TypeClass *context, ENode *receiver, HashNameNode *name, char option);
extern ENode *CObjCModern_CreateAllocMessage(TypeClass *object);
extern ENode *CObjCModern_TryParseMethodCall(TypeClass *a, ENode *b);
extern void CObjCModern_GenerateSymbolTableAndModule(void);
extern Object *CObjCModern_GetOrCreateFunctionObject(char *identifier, char *identifier2);
extern Object *CObjCModern_GetSelectorReference(HashEntry *p);
extern HashEntry *CObjCModern_RegisterMethodSelector(MethRec *link);
extern Boolean CObjCModern_CompareMethRecs(struct MethRec *left, struct MethRec *right);
extern Object *fn_00509c40(char *name, short kind);
extern char *CObjCModern_ConcatStrings(char *first, char *second, char *third);
extern void fn_00509df0(void);
extern void CObjCModern_ResetGlobals(void);
extern ENode *CObjCModern_MakeDeallocMessage(TypeClass *type, ENode *object);
extern HashEntry *CObjCModern_FindMessageArgumentHashEntry(struct MessageArgument *p);
extern struct HashEntry **selector_hash;
extern struct PrecTypeEntry *class_type_entries;
extern SInt32 selector_reference_count;

#ifdef __cplusplus
}
#endif

#endif
