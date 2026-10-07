#ifndef COMPILER_CTEMPLATECLASS_H
#define COMPILER_CTEMPLATECLASS_H

#include "compiler/common.h"
#include "compiler/objects.h"
#include "compiler/enode.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateNew.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 2)
struct TypeClassTemplate {
    TypeClass base;          /* 0x00: CTemplateClass_ParseClassDeclaration initializes class header with flags 0x100 */
    TypeClassTemplate *next; /* 0x32: CTemplateClass_ParseClassDeclaration links class_template_list */
    TypeClassTemplate *enclosingTemplate; /* 0x36: CTemplateClass_ParseClassDeclaration sets enclosing template owner */
    Type *relatedClass;                   /* 0x3a: write_typeclass serializes related class */
    struct TemplateParameterRecord *
        templateParameters; /* 0x3e: CTemplateClass_ParseClassDeclaration stores parameters; ParsePartialSpecialization walks parameter->next */
    struct KeyedEntry *
        templateArgumentOverrides; /* 0x42: CTemplateClass_AddTemplateArgumentOverride links keyed member body overrides */
    TypeClassExt800 *
        instances; /* 0x46: CTemplateClass_GetInstance walks instantiated classes; create_class_template_instance links instance */
    Type *replacementTemplate; /* 0x4a: CTemplateClass_ParsePartialSpecialization stores specialized template */
    struct ClassTemplateSpecialization
        *specializations; /* 0x4e: CTemplateClass_ParsePartialSpecialization links specialization->next */
    struct TemplateClassDeclaration
        *declarations; /* 0x52: serialize_template_class_declarations dispatches kind-tagged member declarations */
    UInt16 virtualSlotCount;  /* 0x56: CTemplateClass_CompleteClassLayout stores ClassLayoutInput count */
    UInt8 structAlignment;    /* 0x58: CTemplateClass_ParseClassDeclaration saves copts.structalignment */
    UInt8 hasVirtualFunction; /* 0x59: CTemplateClass_CompleteClassLayout stores ClassLayoutInput hasVirtualFunction */
};
struct ClassChainEntry {
    struct ClassChainEntry *next;
    TStreamElement sourcePosition;
    UInt32 value;
    UInt8 kind;
    UInt8 unknown21[5];
    UInt8 type;
    UInt8 unknown27;
};
#pragma pack(pop)
#pragma options align = mac68k
struct ClassTemplateSpecialization {
    struct ClassTemplateSpecialization
        *next; /* 0x00: CTemplateClass_ParsePartialSpecialization links templateClass->specializations */
    struct TypeClassTemplate *type; /* 0x04: CTemplateClass_ParsePartialSpecialization stores specialization template */
    struct CTStateElem *
        arguments; /* 0x08: CTemplateClass_ParsePartialSpecialization copies specialization arguments; serialize_reference_entries serializes CTStateElem list */
};
#pragma options align = reset
#pragma options align = mac68k
struct KeyedEntry {
    struct KeyedEntry *next; /* 0x00: CTemplateClass_AddTemplateArgumentOverride links owner's overrides */
    struct TemplateParameterRecord
        *templateParameters; /* 0x04: parse_template_member_definition stores context after CTemplTool_EqualParams */
    Object *key;             /* 0x08: CTemplateClass_AddTemplateArgumentOverride checks object redefinition */
    FileOffsetInfo name;     /* 0x0c: CTemplateClass_AddTemplateArgumentOverride copies source file information */
    TokenStream payload;     /* 0x16: CTemplateClass_AddTemplateArgumentOverride saves member body tokens */
};
#pragma options align = reset
struct MemberVarAlias {
    ObjMemberVar member;
    struct BClassList *bases;
};
#pragma options align = mac68k
struct NewFunc {
    struct Type *dtype;                  /* 0x00: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.dtype */
    UInt32 qual;                         /* 0x04: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.qual */
    struct NameSpace *nspace;            /* 0x08: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.nspace */
    HashNameNode *name;                  /* 0x0c: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.name */
    struct CTStateElem *parsedData;      /* 0x10: CDecl_CopyDeclInfoToNewFunc copies the CTStateElem list */
    unsigned short storage;              /* 0x14: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.storage */
    unsigned short extraQualifiers;      /* 0x16: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.extraQualifiers */
    unsigned char declarationAttributes; /* 0x18: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.declarationAttributes */
    signed char hasTemplateArguments;    /* 0x19: CDecl_CopyDeclInfoToNewFunc copies DeclInfo.hasTemplateArguments */
    FileOffsetInfo ot;      /* 0x1a: CTemplateClass_AddDeferredFunctionDeclaration saves function_fileinfo */
    TokenStream bodyTokens; /* 0x24: CTemplateClass_AddDeferredFunctionDeclaration saves function body tokens */
};
#pragma options align = reset
#pragma pack(push, 2)
struct PendingTemplateInstantiation {
    struct PendingTemplateInstantiation *next; /* 0x00: CTemplateClass_AppendEnumDeclaration appends to declarations */
    TStreamElement context; /* 0x04: CTemplateClass_AppendEnumDeclaration saves CPrep_GetLastBufferedToken */
    TypeEnum *enumType;     /* 0x1c: CTemplateClass_AppendEnumDeclaration stores its TypeEnum argument */
    unsigned char reserved[6];
    unsigned char kind; /* 0x26: CTemplateClass_AppendEnumDeclaration sets declaration kind 1 */
};
#pragma pack(pop)
#pragma options align = mac68k
struct TemplateClassDeclaration {
    struct TemplateClassDeclaration *next; /* 0x00: serialize_template_class_declarations walks next */
    CPrecWrittenEntry source;              /* 0x04: create_class_template_instance saves written entry */
    unsigned char sourceTail[12];          /* 0x10: serialize_template_class_declarations clears source tail */
    union {
        Type *type; /* 0x1c: serialize_template_class_declarations kinds 0, 1, 4, 6 write types */
        struct TemplateDeclarationData
            *friendDeclaration; /* 0x1c: create_class_template_instance kind 2 instantiates friend declaration */
        ObjBase *
            object; /* 0x1c: initialize_enum_constants kind 3 reads ObjEnumConst; instantiate_ivars kind 7 and instantiate_template_object kinds 5, 7 match ObjBase objects */
    } target;
    union {
        struct ClassList *bases;   /* 0x20: serialize_template_class_declarations kind 4 writes base list */
        struct ENode *initializer; /* 0x20: serialize_template_class_declarations kinds 3, 5 write expressions */
        unsigned char access;      /* 0x20: create_class_template_instance kind 6 reads using declaration access */
    } value;
    unsigned char access;     /* 0x24: instantiate_bases kind 4 copies base access */
    unsigned char is_virtual; /* 0x25: instantiate_bases kind 4 copies virtual flag */
    unsigned char kind;       /* 0x26: serialize_template_class_declarations dispatches declaration variant */
};
#pragma options align = reset
#pragma pack(push, 2)
struct TemplateClassMatch {
    struct TemplateClassMatch *next;
    struct ClassTemplateSpecialization *candidate;
};
#pragma pack(pop)
struct TemplateContext {
    struct TypeClass *templateClass;
    struct TypeClass *instance;
    struct TemplateParameterRecord *
        templateArgs; /* 0x08: CTemplateClass.c initializes from resolvedTemplate->templateParameters; CTemplateFunc.c uses function template params */
    struct CTStateElem *
        instanceArgs; /* 0x0c: CTemplateFunc.c initializes from state->slots and bindingList; CTemplateClass.c uses classInstance->targs */
    struct TemplateInstantiationMapping *mappings;
    union {
        short reserved;
        struct {
            unsigned char processingClassTypes;
            unsigned char processingArgument;
        } modes;
    };
    char hasNewVBases;
    unsigned char parameterNIndex;
};
#pragma pack(push, 2)
struct TemplateExpressionRecord {
    struct TemplateExpressionRecord *next; /* 0x00: CTemplateClass_AppendExpressionRecord links declarations */
    TStreamElement parserState; /* 0x04: CTemplateClass_AddDeferredFunctionDeclaration saves buffered token */
    union {
        Object *object; /* 0x1c: CTemplateClass_AppendExpressionRecord kind 5; AppendEnumConstDeclaration kind 3 */
        struct NewFunc *functionDeclaration; /* 0x1c: CTemplateClass_AddDeferredFunctionDeclaration kind 2 */
    } declaration; /* 0x1c: CTemplateClass_AddDeferredFunctionDeclaration stores kind 2 function; AppendExpressionRecord stores kind 5 object */
    ENode *expression;         /* 0x20: CTemplateClass_AppendExpressionRecord stores initializer */
    unsigned char reserved[2]; /* 0x24: CTemplateClass_AppendExpressionRecord clears storage */
    unsigned char
        kind; /* 0x26: CTemplateClass_AddDeferredFunctionDeclaration sets 2; AppendEnumConstDeclaration sets 3; AppendExpressionRecord sets 5 */
    unsigned char reserved_end; /* 0x27: CTemplateClass_AppendExpressionRecord clears storage */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct TemplateInstantiationMapping {
    struct TemplateInstantiationMapping *next; /* 0x00: instantiate_enum links context mappings */
    struct TemplateClassDeclaration *
        declaration; /* 0x04: instantiate_enum stores entry; create_class_template_instance matches member declarations */
    ObjBase *object; /* 0x08: instantiate_object_type tests OT_MEMBERVAR, OT_TYPE and OT_OBJECT */
    UInt8 unused[4]; /* 0x0c: instantiate_enum allocates 20 bytes; CTemplateClass.c never reads or writes this slot */
    TypeEnum *replacement; /* 0x10: instantiate_enum stores replacement; initialize_enum_constants reads its enumlist */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct TemplateListRecord {
    struct TemplateListRecord *next;
    struct TStreamElement state;
    struct TypeClassTemplate
        *object; /* 0x1c: CTemplateClass_CreateClassTemplateDeclaration stores nested class template */
    unsigned char reserved_20[6];
    unsigned char value_26;
};
#pragma pack(pop)
#pragma options align = mac68k
struct TemplateMemberData {
    struct TemplateMemberData *next;
    struct TStreamElement sourcePosition;
    struct TypeClassTemplate *record; /* 0x1c: CTemplateClass_ParseClassDeclaration stores nested class template */
    char reserved20[6];
    char kind;
};
#pragma options align = reset
#pragma pack(push, 2)
struct TemplateObjectDeclaration {
    struct TemplateObjectDeclaration *next;
    TStreamElement data;
    Object *argument;
    unsigned char reserved[6];
    unsigned char kind;
    unsigned char trailing;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TemplateRecordEntry {
    struct TemplateRecordEntry *next;
    TStreamElement sourcePosition;
    Type *value;
    struct ClassList *lastBase;
    unsigned char
        access; /* 0x24: CTemplateClass_PrependTemplateRecordEntry stores base access; instantiate_bases reads kind == 4 declaration access */
    unsigned char
        is_virtual; /* 0x25: CTemplateClass_PrependTemplateRecordEntry stores virtual-base flag; instantiate_bases reads kind == 4 declaration is_virtual */
    unsigned char tag;
    unsigned char reserved;
};
#pragma pack(pop)
extern unsigned char CTemplateClass_InstantiateClass(TypeClass *theclass);
extern void instantiate_namespace_objects(TemplateContext *map, TypeClass *unused, TypeClass *obj);
extern void CTemplateClass_0051c680(TemplateContext *ctx, Object *obj);
extern void instantiate_objtype(TemplateContext *context, ObjType *type, HashNameNode *name);
extern void initialize_enum_constants(TemplateContext *context, struct TemplateClassDeclaration *object,
                                      TypeEnum *scope);
extern void instantiate_enum(TemplateContext *context, struct TemplateClassDeclaration *entry);
extern struct TemplateClassMatch *remove_less_specialized_matches(struct TemplateClassMatch *list);
extern unsigned char match_template_arguments(ClassTemplateSpecialization *arguments,
                                              ClassTemplateSpecialization *pattern);
extern CTStateElem *match_specialization_arguments(ClassTemplateSpecialization *arguments, CTStateElem *actual,
                                                   char instantiate);
extern void CTemplateClass_ParseClassDeclaration(TemplateScopeState *scope, TemplateParameterRecord *parameters,
                                                 short access, SInt32 *state);
extern unsigned char CTemplateClass_CompleteClassLayout(struct TypeClassTemplate *state, ClassLayoutInput *values);
extern struct TypeClassTemplate *CTemplateClass_CreateClassTemplateDeclaration(TypeClass *owner, HashNameNode *arg1,
                                                                               short arg2);
extern char CTemplateClass_SelectSpecialization(CTStateElem *context, struct TypeClassTemplate **classType,
                                                CTStateElem **result);
extern TypeClassExt800 *create_class_template_instance(struct TypeClassTemplate *definition, void *argument,
                                                       void *alternate_argument);
extern void CTemplateClass_AppendExpressionRecord(struct TypeClassTemplate *type, Object *object, ENode *expression);
extern void CTemplateClass_AppendEnumConstDeclaration(struct TypeClassTemplate *self, ObjEnumConst *a, ENode *str);
extern void CTemplateClass_AppendEnumDeclaration(struct TypeClassTemplate *type, TypeEnum *value);
extern void CTemplateClass_AppendFuncDeclaration(struct TypeClassTemplate *owner, TypeTemplDep *value,
                                                 unsigned char kind);
extern struct TypeClassTemplate *CTemplateClass_ResolveRelatedClass(struct TypeClassTemplate *record);
extern unsigned int CTemplateClass_PrependTemplateRecordEntry(struct TypeClassTemplate *list, Type *value,
                                                              unsigned char value24, unsigned char value25);
extern void instantiate_friend_declaration(TemplateContext *ctx, struct TemplateDeclarationData *declaration);
extern void instantiate_object_type(TemplateContext *context, TemplateClassDeclaration *function, ObjBase *object);
extern void instantiate_template_object(TemplateContext *ctx, Object *templ);
extern void CTemplateClass_0051cec0(TemplateContext *context, struct TypeClassTemplate *templateClass);
extern void CTemplateClass_ParsePartialSpecialization(TemplateScopeState *scope,
                                                      struct TemplateParameterRecord *parameters, short access,
                                                      SInt32 *position);
extern TypeClassExt800 *CTemplateClass_GetInstance(struct TypeClassTemplate *cls, CTStateElem *key, CTStateElem *flag);
extern struct KeyedEntry *CTemplateClass_AddTemplateArgumentOverride(struct TypeClassTemplate *owner, Object *key,
                                                                     FileOffsetInfo *name, struct TokenStream *payload);
extern void CTemplateClass_AppendObjectDeclaration(struct TypeClassTemplate *ctx, Object *obj);
extern void instantiate_bases(TemplateContext *arg1, TypeClass *arg2, struct TypeClassTemplate *arg3);
extern void instantiate_ivars(TemplateContext *ctx, TypeClass *dst, struct TypeClassTemplate *src);
extern void CTemplateClass_AddDeferredFunctionDeclaration(struct TypeClassTemplate *p1, DeclInfo *p2);
extern void fn_0051b800(void);
extern void fn_0051b810(void);
extern struct TypeClassTemplate *class_template_list;
extern char *CTemplateClass_ParseDouble(char *value, double *result, char *error);
struct TypeClassTemplate;

#ifdef __cplusplus
}
#endif

#endif
