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

#pragma pack(push, 1)
struct AsmOperandPattern {
    char *name;
    unsigned int opcode;
    unsigned char operands[6];
    unsigned int processorMask;
    unsigned int instruction;
};
#pragma pack(pop)
struct NameLookupLink {
    struct NameLookupLink *next;
    struct AsmOperandPattern *record;
};
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
struct RegisterBinding {
    struct RegisterBinding *next;
    unsigned int key;
    unsigned short attribute1;
    unsigned short registerNumber;
    struct Object *object;
};
struct RegistrationEntry {
    const char *name;
    unsigned char kind;
    unsigned short value;
};
struct RegistrationHashEntry {
    struct RegistrationHashEntry *next;
    int id;
    const char *name;
    short kind;
    short value;
    int extra;
};
struct RegistrationTableEntry {
    const char *name;       /* 0x00: CTemplateNew_InitRegistrationHashTables reads the register name */
    short value;            /* 0x04: CTemplateNew_InitRegistrationHashTables copies the register value */
    short alignmentPadding; /* 0x06: registration_table, unused space aligning id to 0x08 */
    int id;                 /* 0x08: CTemplateNew_InitRegistrationHashTables copies the register id */
};
struct SecondaryRegistrationEntry {
    int id;
    const char *name;
    short value;
};
#pragma options align = mac68k
struct TemplateObjectInstance {
    Object base;
    Object *templateObject;
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
    FOI sourceInfo;
    struct PrepTokenBuffer state;
    struct PFile *sourceFile;
    UInt32 sourceLine;
    struct PFile *
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
extern void CTemplateNew_CompileObject(struct TypeClassTemplate *templateClass, TypeClassExt800 *context,
                                       TemplateSourceRecordTyped *source, Object *object, Boolean reset);
extern Boolean CTemplateNew_InstantiateFunction(TemplateFunction *definition,
                                                TemplateSpecializationData *specialization, Boolean report);
extern void CTemplateNew_ParseTemplateDeclaration(TypeClass *templateClass);
extern void parse_explicit_template_specialization(void);
extern void parse_explicit_template_instantiation(void);
extern void parse_function_template_declaration(TemplateScopeState *stack, TemplateParameterRecord *params,
                                                TypeClass *tclass, SInt32 *startOffset);
extern void parse_template_member_definition(void *context, TypeClass *template_info, DeclInfo *declaration,
                                             SInt32 *position);
extern unsigned char instantiate_members(Type *unused, TypeClassExt800 *state, char force);
extern void CTemplateNew_ParseFuncDef(Object *object, TypeClassExt800 *context, TplSpec *specialization);
extern Boolean CTemplateNew_InstantiateInlineTemplateObject(Object *obj);
extern UInt8 CTemplateNew_LinkTemplateScope(DeclInfo *context, TypeTemplDep *request, NameSpace **destination);
extern TemplateParameterRecord *parse_template_parameter_list(NameSpace *parserState, unsigned char mode);
extern TemplateParameterRecord *parse_template_parameter(NameSpace *owner, TemplateParameterRecord *value,
                                                         short memberValue, char memberByte);
extern void fn_004f0000(void);
extern void CTemplateNew_Reset(void);
extern unsigned int fn_004f0040(IROLinear *node);
extern AsmOperandPattern *CTemplateNew_FindAsmOperandPattern(char *name);
extern void CTemplateNew_InitAsmOperandPatternLookup(void);
extern void *find_register_binding_key(unsigned int *key);
extern void CTemplateNew_InsertRegisterBinding(const char *key, unsigned int attribute1, short registerNumber,
                                               Object *object);
extern ENode *parse_non_type_template_argument(Type *targetType, unsigned int qualifiers);
extern Type *CTemplTool_GetSelfRefTemplate(struct TypeClassTemplate *record);
extern struct CTStateElem *CTemplateNew_ParseTemplateArguments(struct TemplateParameterRecord *arg, char flag);
extern void skip_balanced_angle_tokens(void);
extern struct InlineAsmRegisterEntry *fn_004f06d0(char *name);
extern struct InlineAsmRegisterEntry *CTemplateNew_LookupInlineAsmRegister(char *name);
extern struct RegisterBinding *register_binding_hash[64];
extern struct NameLookupLink *asmOperandPatternLookup[256];
extern void *DAT_00584cb4[];
extern void *DAT_00584cb8[];
extern struct NameLookupLink *DAT_00584cbc[256];
extern struct NameLookupLink *DAT_00584cc0[];
extern struct NameLookupLink *DAT_00584cc4[];
extern struct NameLookupLink *DAT_00584cc8[256];
extern struct NameLookupLink *DAT_00584ccc[];
extern char data_00565458[];
extern unsigned int inline_asm_register_masks[];
extern jmp_buf template_declaration_jmpbuf;
extern Boolean data_00582108;
extern struct InlineAsmRegisterEntry inlineAsmRegisterEntry;
extern unsigned short data_0058242c;
extern unsigned short data_0058242e;
extern unsigned int data_00582430;
extern struct InlineAsmRegisterEntry {
    const char *name;      /* 0x00: fn_004f06d0 clears the numeric register name */
    short kind;            /* 0x04: fn_004f06d0 sets DCR kind 4; CTemplateNew_LookupInlineAsmRegister sets SPR kind 2 */
    short number;          /* 0x06: InlineAsmPPC.c reads the DCR register number */
    struct Object *object; /* 0x08: CTemplateNew_LookupInlineAsmRegister clears object for numeric registers */
} data_00582434;
extern SInt16 data_00582438;
extern SInt16 data_0058243a;
extern SInt32 data_0058243c;
extern SInt32 source_line;
extern char template_recordbrowseinfo;
extern CTStateElem *parse_template_arguments(struct TypeClassTemplate **classType, CTStateElem **result);
extern void CTemplateNew_InitRegistrationHashTables(void);
extern struct SecondaryRegistrationEntry secondary_registration_table[];
extern char data_00572397[];
extern struct RegistrationHashEntry *inlineAsmRegisterHashTable[64];
extern struct RegistrationHashEntry *secondary_registration_hash[64];
extern InlineAsmRegisterEntry *CTemplateNew_GetInlineAsmRegisterEntry(HashNameNode *name);
extern void CTemplateNew_ClearGlobalArray(void);
extern AsmOperandPattern asm_operand_patterns[];
extern struct RegistrationEntry registration_entries[];
extern struct RegistrationTableEntry registration_table[];

#ifdef __cplusplus
}
#endif

#endif
