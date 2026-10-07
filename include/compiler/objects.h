#ifndef COMPILER_OBJECTS_H
#define COMPILER_OBJECTS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJ_BASE(obj) ((ObjBase *)(obj))
#define OBJ_ENUM_CONST(obj) ((ObjEnumConst *)(obj))
#define OBJ_TYPE(obj) ((ObjType *)(obj))
#define OBJ_MEMBER_VAR(obj) ((ObjMemberVar *)(obj))
#define OBJECT(obj) ((Object *)(obj))
enum { OT_ENUMCONST, OT_TYPE, OT_TYPETAG, OT_NAMESPACE, OT_MEMBERVAR, OT_OBJECT, OT_ILLEGAL };
/* Single-byte enumeration; DALIAS is 6 in this build (CMangler_GetLinkName
 * follows the alias chain on 6) and DFUNC/DVFUNC are 3/4. */
enum { DDATA, DLOCAL, DABSOLUTE, DFUNC, DVFUNC, DINLINEFUNC, DALIAS, DEXPR, DUNUSED };
enum {
    OBJECT_USED = 1,
    OBJECT_FLAGS_2 = 2,
    OBJECT_DEFINED = 4,
    OBJECT_LAZY = 8,
    OBJECT_INTERNAL = 0x10,
    OBJECT_IMPORT = 0x20,
    OBJECT_EXPORT = 0x40
};
#pragma options align = mac68k
struct ObjectList {
    ObjectList *next;
    Object *object;
};
#pragma options align = reset
#pragma options align = mac68k
/* Common prefix: otype selects the concrete object payload. */
struct ObjBase {
    UInt8 otype;  /* 0x00: InlineAsm_ResolveOperandName switches on the concrete object kind. */
    UInt8 access; /* 0x01: CClass_CheckEnumAccess checks accessibility. */
};
#pragma options align = reset
#pragma options align = mac68k
struct ObjEnumConst {
    UInt8 otype;
    UInt8 access;
    ObjEnumConst *next;
    HashNameNode *name;
    Type *type;
    CInt64 val;
};
#pragma options align = reset
/* OT_NAMESPACE object: an object that names a namespace (CPrec, CScope and four other views agree). */
#pragma options align = mac68k
struct ObjNameSpace {
    UInt8 otype;
    UInt8 access;
    NameSpace *nspace;
};
#pragma options align = reset
#pragma options align = mac68k
struct ObjType {
    UInt8 otype;
    UInt8 access;
    Type *type;
    UInt32 qual;
};
#pragma options align = reset
#pragma options align = mac68k
struct ObjMemberVar {
    UInt8 otype;
    UInt8 access;
    Boolean anonunion;
    Boolean has_path;
    ObjMemberVar *next;
    HashNameNode *name;
    Type *type;
    UInt32 qual;
    UInt32 offset;
};
/* An ObjMemberVar with has_path set: one reached through base classes */
struct ObjMemberVarPath {
    UInt8 otype;
    UInt8 access;
    Boolean anonunion;
    Boolean has_path;
    ObjMemberVar *next;
    HashNameNode *name;
    Type *type;
    UInt32 qual;
    UInt32 offset;
    BClassList *path;
};
#pragma options align = reset
/* An inline function's cross-references to one object: numxrefs (offset, varoffset) pairs. */
#pragma options align = mac68k
struct InlineXRef {
    struct InlineXRef *next;
    Object *object;
    UInt16 xrefoffset;
    UInt16 numxrefs;
    struct {
        UInt32 offset;
        SInt32 varoffset;
    } xref[1];
};
#pragma options align = reset
#pragma options align = mac68k
struct Object {
    UInt8 otype;
    UInt8 access;
    UInt8 datatype;
    UInt16 section;
    NameSpace *nspace;
    HashNameNode *name;
    Type *type;
    UInt32 qual;
    UInt16 sclass;
    UInt8 flags;
    UInt8 unk19;
    union {
        struct Object *toc;
        struct DwarfNode *pendingEntry;
    } dwarfLinks;
    union {
        struct DwarfSymbol *symbol;
        struct DwarfNode *entry;
    } debugInfo;
    union {
        UInt8 flag;
        struct VarRecord *varRecord;
    } aliasOrVarRecord;
    union {
        struct {
            union {
                CInt64 intconst;
                char *string;
                struct {
                    SInt32 *data;
                    SInt32 size;
                } switchtable;
            } u;
            union {
                VarInfo *info;
                struct InterruptGenerationRecord *interruptInfo;
            };
            HashNameNode *linkname;
        } data;
        struct {
            struct CInlineInfo *
                u; /* 0x26: write_object selects inline body when TypeFunc flags & 0x400 is clear and Q_INLINE is set; CInline_0050ee60 stores CInline_SaveInfo output. */
            struct DefArgCtorInfo *
                defargdata; /* 0x2a: make_defarg_function stores the constructor and default expression; make_auto_generated_method tests it before CABI_MakeDefaultArgConstructor. */
            HashNameNode *linkname;
        } func;
        struct {
            VarInfo *info;
            SInt32 uid;
        } var;
        struct {
            UInt8 *
                data; /* 0x26: write_object selects DINLINEFUNC (datatype 5) and copies size bytes from this inline machine-code buffer through unsigned char *data. */
            SInt32 size;
            InlineXRef *xrefs;
        } ifunc;
        struct {
            Object *object;
            BClassList *member;
            SInt32 offset;
        } alias;
        struct TemplateFunction *
            templateFunction; /* 0x26: write_object and instantiate_object_type select this arm when TypeFunc flags & 0x400; parse_function_template_declaration sets that flag and stores templ. */
        SInt16 intrinsic;
        ENode *expr;
    } u;
};
/* An object instantiated from a class template's member (Q_IS_TEMPLATED): the member it came from */
struct ObjectTemplated {
    Object object;
    Object *parent;
};
#pragma options align = reset

#ifdef __cplusplus
}
#endif

#endif
