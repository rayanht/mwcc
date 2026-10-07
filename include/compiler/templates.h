#ifndef COMPILER_TEMPLATES_H
#define COMPILER_TEMPLATES_H

#include "compiler/common.h"
#include "compiler/tokens.h"
#include "compiler/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEMPL_CLASS(ty) ((TemplClass *)(ty))
#define TEMPL_CLASS_INST(ty) ((TemplClassInst *)(ty))

#pragma options align = mac68k
struct PackedDeclInfo {
    Type *thetype;
    UInt32 qual;
    NameSpace *nspace;
    HashNameNode *name;
    TemplArg *expltargs;
    SInt16 storageclass;
    SInt16 section;
    UInt8 exportflags;
    Boolean has_expltargs;
};

struct TemplateFriend {
    PackedDeclInfo decl;
    FileOffsetInfo fileoffset;
    TokenStream stream;
};

struct TemplateMember {
    TemplateMember *next;
    TemplParam *params;
    Object *object;
    FileOffsetInfo fileoffset;
    TokenStream stream;
    CPrepFileInfo *srcfile;
    SInt32 startoffset;
    SInt32 endoffset;
};

/* pid.type is 1 for a type parameter, 0 for a non-type one */
struct TemplParam {
    TemplParam *next;
    HashNameNode *name;
    TemplParamID pid;
    union {
        struct {
            Type *type;
            UInt32 qual;
            Boolean isTypeDependent;
        } typeparam;
        struct {
            Type *type;
            UInt32 qual;
            ENode *defaultarg;
        } paramdecl;
    } data;
};

struct TemplArg {
    TemplArg *next;
    TemplParamID pid;
    union {
        struct {
            Type *type;
            UInt32 qual;
        } typeparam;
        struct {
            ENode *expr;
            Boolean is_ref;
        } paramdecl;
    } data;
    Boolean is_deduced;
};

struct DeduceInfo {
    TemplArg *args;
    TemplArg argBuffer[16];
    SInt32 maxCount;
    SInt32 count;
    UInt8 depth;
};

struct DefAction {
    DefAction *next;
    TemplateAction *action;
    ObjBase *refobj;
    UInt8 unused[4];
    TypeEnum *enumtype;
};

struct TypeDeduce {
    TemplClass *tmclass;
    TemplClassInst *inst;
    TemplParam *params;
    TemplArg *args;
    DefAction *defActions;
    Boolean processingClassTypes;
    Boolean processingArgument;
    Boolean hasNewVBases;
    UInt8 nindex;
};

struct TemplPartialSpec {
    TemplPartialSpec *next;
    TemplClass *templ;
    TemplArg *args;
};

struct TemplStack {
    TemplStack *next;
    union {
        Object *func;
        TypeClass *theclass;
    } u;
    Boolean is_func;
};

struct TemplClass {
    TypeClass theclass;
    TemplClass *next;
    TemplClass *templ_parent;
    TemplClassInst *inst_parent;
    TemplParam *templ__params;
    TemplateMember *members;
    TemplClassInst *instances;
    TemplClass *pspec_owner;
    TemplPartialSpec *pspecs;
    TemplateAction *actions;
    UInt16 lex_order_count;
    SInt8 align;
    UInt8 flags;
};

struct TemplClassInst {
    TypeClass theclass;
    TemplClassInst *next;
    TemplClassInst *parent;
    TemplClass *templ;
    TemplArg *inst_args;
    TemplArg *oargs;
    Boolean is_instantiated;
    Boolean is_specialized;
    Boolean is_extern;
    Boolean static_instantiated;
};

struct TemplateFunction {
    TemplateFunction *next;
    TemplateFunction *original;
    HashNameNode *name;
    TemplParam *params;
    TokenStream stream;
    TStreamElement deftoken;
    Object *tfunc;
    TemplFuncInstance *instances;
    CPrepFileInfo *srcfile;
    SInt32 startoffset;
    SInt32 endoffset;
};

struct TemplFuncInstance {
    TemplFuncInstance *next;
    Object *object;
    TemplArg *args;
    Boolean is_instantiated;
    Boolean is_specialized;
    Boolean is_extern;
};

typedef enum TemplateActionType {
    TAT_NESTEDCLASS,
    TAT_ENUMTYPE,
    TAT_FRIEND,
    TAT_ENUMERATOR,
    TAT_BASE,
    TAT_OBJECTINIT,
    TAT_USINGDECL,
    TAT_OBJECTDEF,
    TAT_ILLEGAL
} TemplateActionType;

struct TemplateAction {
    TemplateAction *next;
    TStreamElement source_ref;
    union {
        TemplClass *tclasstype;
        TypeEnum *enumtype;
        TemplateFriend *tfriend;
        struct {
            ObjEnumConst *objenumconst;
            ENode *initexpr;
        } enumerator;
        struct {
            Type *type;
            ClassList *insert_after;
            UInt8 access;
            Boolean is_virtual;
        } base;
        struct {
            Object *object;
            ENode *initexpr;
        } objectinit;
        struct {
            TypeTemplDep *type;
            UInt8 access;
        } usingdecl;
        ObjBase *refobj;
    } u;
    UInt8 type;
};
#pragma options align = reset

#ifdef __cplusplus
}
#endif

#endif
