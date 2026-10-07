#ifndef COMPILER_CTEMPLATENEW_H
#define COMPILER_CTEMPLATENEW_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CInline.h"
#include "compiler/CPrep.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ObjectReferenceEntry {
    struct ObjectReferenceEntry *
        next; /* 0x00: FunctionCalls_PushObjectReferenceEntry links object_reference_stack; CTemplateTools_PopObjectReferenceEntry pops it */
    Type *
        value; /* 0x04: append_instantiation_stack reads a Type when kind == 0, casts to Object for append_object_name when kind != 0 */
    unsigned char
        kind; /* 0x08: FunctionCalls_PushObjectReferenceEntry sets 0 for Type, 1 for Object; append_instantiation_stack tests it */
};
#pragma options align = reset
#pragma options align = mac68k
struct ParserPosition {
    long position;
};
#pragma options align = reset
#pragma options align = mac68k
struct ObjectTemplated {
    Object object;
    Object *parent;
};
#pragma options align = reset
struct TemplateArgumentOverride {
    struct TemplateArgumentOverride *next;
    FuncArg *templateParameters;
    SInt32 templateArgumentKey;
};
#pragma pack(push, 1)
struct TemplateParameterRecord {
    struct TemplateParameterRecord *next; /* 0x00: src/frontend/CTemplateNew.c */
    struct HashNameNode *name;            /* 0x04: src/frontend/CTemplateNew.c */
    union {
        TemplParamID pid; /* 0x08: src/frontend/CTemplateNew.c */
        struct {
            short index;          /* 0x08: src/frontend/CTemplateNew.c */
            unsigned char depth;  /* 0x0a: src/frontend/CTemplateNew.c */
            char isTypeParameter; /* 0x0b: src/frontend/CTemplateNew.c */
        };
    };
    struct Type *value; /* 0x0c: src/frontend/CTemplateNew.c */
    unsigned int flags; /* 0x10: src/frontend/CTemplateNew.c */
    union {
        unsigned char isTypeDependent; /* 0x14: src/frontend/CTemplateNew.c */
        struct ENode *expression;      /* 0x14: src/frontend/CTemplateNew.c */
    } defaultValue;
};
#pragma pack(pop)
#pragma options align = mac68k
struct TemplateScopeState {
    struct NameSpace *scope;
    struct NameSpace *linkedNamespace;
};
#pragma options align = reset
#pragma options align = mac68k
struct TemplateSourceRecordTyped {
    struct TemplateSourceRecordTyped *next;
    struct FuncArg
        *context; /* 0x04: CTemplateNew_CompileObject passes template parameters to CTemplateTools_InsertTemplateArgs */
    struct Object *object;
    FileOffsetInfo sourceInfo;
    struct TokenStream state;
    struct CPrepFileInfo *sourceFile;
    UInt32 sourceLine;
    struct CPrepFileInfo *
        f26; /* 0x26: serialize_prec_records allocates image storage with prec_position += 0x2a and clears this pointer */
    UInt16 f2a; /* 0x2a: PrecRecord trailing storage outside serialize_prec_records image allocation */
};
#pragma options align = reset
#pragma options align = mac68k
struct TplSpec {
    UInt8 pad0[6];
    SInt32 val;
};
#pragma options align = reset
extern Boolean CTemplateNew_InstantiatePendingTemplates(void);
extern void CTemplateNew_CompileObject(struct TemplClass *templateClass, TemplClassInst *context,
                                       TemplateSourceRecordTyped *source, Object *object, Boolean reset);
extern Boolean CTemplateNew_InstantiateFunction(TemplateFunction *definition, TemplFuncInstance *specialization,
                                                Boolean report);
extern void CTemplateNew_ParseTemplateDeclaration(TypeClass *templateClass);
extern void parse_explicit_template_specialization(void);
extern void parse_explicit_template_instantiation(void);
extern void parse_function_template_declaration(TemplateScopeState *stack, TemplateParameterRecord *params,
                                                TypeClass *tclass, SInt32 *startOffset);
extern void parse_template_member_definition(void *context, TypeClass *template_info, DeclInfo *declaration,
                                             SInt32 *position);
extern unsigned char instantiate_members(struct TemplClass *templ, TemplClassInst *state, char force);
extern void CTemplateNew_ParseFuncDef(Object *object, TemplClassInst *context, TplSpec *specialization);
extern Boolean CTemplateNew_InstantiateInlineTemplateObject(Object *obj);
extern UInt8 CTemplateNew_LinkTemplateScope(DeclInfo *context, TypeTemplDep *request, NameSpace **destination);
extern TemplateParameterRecord *parse_template_parameter_list(NameSpace *parserState, unsigned char mode);
extern TemplateParameterRecord *parse_template_parameter(NameSpace *owner, TemplateParameterRecord *value,
                                                         short memberValue, char memberByte);
extern void fn_004f0000(void);
extern void CTemplateNew_Reset(void);
extern ENode *parse_non_type_template_argument(Type *targetType, unsigned int qualifiers);
extern Type *CTemplTool_GetSelfRefTemplate(struct TemplClass *record);
extern struct CTStateElem *CTemplateNew_ParseTemplateArguments(struct TemplateParameterRecord *arg, char flag);
extern void skip_balanced_angle_tokens(void);
extern SInt32 source_line;
extern char template_recordbrowseinfo;
extern CTStateElem *parse_template_arguments(struct TemplClass **classType, CTStateElem **result);

#ifdef __cplusplus
}
#endif

#endif
