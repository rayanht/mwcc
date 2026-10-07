#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE(ty) ((Type *)(ty))
#define TYPE_INTEGRAL(ty) ((TypeIntegral *)(ty))
#define TYPE_ENUM(ty) ((TypeEnum *)(ty))
#define TYPE_STRUCT(ty) ((TypeStruct *)(ty))
#define TYPE_CLASS(ty) ((TypeClass *)(ty))
#define TYPE_FUNC(ty) ((TypeFunc *)(ty))
#define TYPE_METHOD(ty) ((TypeMemberFunc *)(ty))
#define TYPE_BITFIELD(ty) ((TypeBitfield *)(ty))
#define TYPE_MEMBER_POINTER(ty) ((TypeMemberPointer *)(ty))
#define TYPE_POINTER(ty) ((TypePointer *)(ty))
#define TPTR_TARGET(ty) (TYPE_POINTER(ty)->target)
/* Single-byte enumeration in this build: TYPEPOINTER is 11, verified by
 * the argument check in CABI_ThisArg. */
enum {
    TYPEVOID = 0,
    TYPEINT = 1,
    TYPEFLOAT = 2,
    TYPEENUM = 3,
    TYPESTRUCT = 4,
    TYPECLASS = 5,
    TYPEFUNC = 6,
    TYPEBITFIELD = 7,
    TYPELABEL = 8,
    TYPETEMPLATE = 9,
    TYPEMEMBERPOINTER = 10,
    TYPEPOINTER = 11,
    TYPEARRAY = 12,
    TYPEOBJCID = 13,
    TYPETEMPLDEPEXPR = 14
};
#define IS_TYPE_VOID(ty) ((ty)->type == TYPEVOID)
#define IS_TYPE_INT(ty) ((ty)->type == TYPEINT)
#define IS_TYPE_ENUM(ty) ((ty)->type == TYPEENUM)
#define IS_TYPE_FLOAT(ty) ((ty)->type == TYPEFLOAT)
#define IS_TYPE_STRUCT(ty) ((ty)->type == TYPESTRUCT)
#define IS_TYPE_CLASS(ty) ((ty)->type == TYPECLASS)
#define IS_TYPE_FUNC(ty) ((ty)->type == TYPEFUNC)
enum {
    FUNC_PASCAL = 1,
    FUNC_DEFINED = 2,
    FUNC_PURE = 8,
    FUNC_METHOD = 0x10,
    FUNC_CONVERSION = 0x40,
    FUNC_AUTO_GENERATED = 0x100,
    FUNC_INTRINSIC = 0x200,
    FUNC_IS_CTOR = 0x1000,
    FUNC_IS_DTOR = 0x2000
};
#define IS_TYPEFUNC_METHOD(ty) ((ty)->flags & FUNC_METHOD)
#define IS_TYPE_POINTER(ty) ((ty)->type == TYPEPOINTER || (ty)->type == TYPEARRAY)
#define IS_TYPE_POINTER_ONLY(ty) ((ty)->type == TYPEPOINTER)
#define IS_TYPE_ARRAY(ty) ((ty)->type == TYPEARRAY)
#define IS_TYPE_BITFIELD(ty) ((ty)->type == TYPEBITFIELD)
#define IS_TYPE_MEMBERPOINTER(ty) ((ty)->type == TYPEMEMBERPOINTER)
enum {
    IT_BOOL,
    IT_CHAR,
    IT_SCHAR,
    IT_UCHAR,
    IT_WCHAR_T,
    IT_SHORT,
    IT_USHORT,
    IT_INT,
    IT_UINT,
    IT_LONG,
    IT_ULONG,
    IT_LONGLONG,
    IT_ULONGLONG,
    IT_FLOAT,
    IT_SHORTDOUBLE,
    IT_DOUBLE,
    IT_LONGDOUBLE
};
enum {
    CLASS_HANDLEOBJECT = 1,
    CLASS_COMPLETED = 2,
    CLASS_ABSTRACT = 8,
    CLASS_SINGLE_OBJECT = 0x10,
    CLASS_HAS_VBASES = 0x20,
    CLASS_IS_CONVERTIBLE = 0x40,
    CLASS_SOM_INIT = 0x8000
};
enum { CLASS_EFLAGS_INTERNAL = 1, CLASS_EFLAGS_IMPORT = 2, CLASS_EFLAGS_EXPORT = 4, CLASS_EFLAGS_F0 = 0xF0 };
#pragma options align = mac68k
struct Type {
    UInt8 type;  /* 0x00: TypedefDeclInfo tests TYPEPOINTER and TYPEARRAY; type-kind discriminator */
    SInt32 size; /* 0x02: scandeclarator sets total array byte size; emit_array_type reads it */
    struct {
        union {
            struct {                    /* TYPEENUM: TypeEnum payload, not an array payload */
                NameSpace *nspace;      /* 0x06: CDecl.c sets the TYPEENUM namespace */
                ObjEnumConst *enumlist; /* 0x0a: emit_enum_type iterates TYPEENUM constants */
                Type *enumtype;         /* 0x0e: DWARF.c reads the TYPEENUM underlying integral type */
                HashNameNode *enumname; /* 0x12: emit_enum_type emits the TYPEENUM name */
            };
            UInt8 integral;    /* 0x06: DWARF.c reads the TYPEINT or TYPEFLOAT integral code */
            struct {           /* TYPEARRAY: scandeclarator constructs; TypedefDeclInfo copies as TypePointer */
                Type *element; /* 0x06: scandeclarator sets elementType; emit_array_type reads element size */
                UInt32
                    qual; /* 0x0a: scandeclarator clears; TypedefDeclInfo copies the TYPEARRAY payload as TypePointer, including qual */
            };
        };
    } array[0]; /* 0x06: kind-selected extension; common Type prefix remains six bytes */
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeIntegral {
    UInt8 type;     /* 0x00: CMachine.c tests TYPEFLOAT for stshortdouble */
    SInt32 size;    /* 0x02: SignedIntType selects the integral type by byte size */
    UInt8 integral; /* 0x06: get_integral_type_code and DumpIR.c decode the IT_* integral or floating-point code */
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeEnum {
    UInt8 type;
    SInt32 size;
    NameSpace *nspace;
    ObjEnumConst *enumlist;
    Type *enumtype;
    HashNameNode *enumname;
};
#pragma options align = reset
#pragma options align = mac68k
struct StructMember {
    StructMember *next;
    Type *type;
    HashNameNode *name;
    SInt32 offset;
    UInt32 qual;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeStruct {
    UInt8 type;            /* 0x00: CDecl_NewStructType sets TYPESTRUCT */
    SInt32 size;           /* 0x02: CDecl_NewStructType sets byte size */
    HashNameNode *name;    /* 0x06: DWARF_004b0a80 emits name */
    StructMember *members; /* 0x0a: DWARF_004b0a80 iterates members */
    SInt8 stype;           /* 0x0e: DWARF_004b0a80 selects structure or union tag */
    UInt8
        alignmentPadding; /* 0x0f: CDecl_NewStructType clears the 18-byte record; unused byte between stype and the two-byte-aligned align member. */
    SInt16 align;         /* 0x10: CDecl_NewStructType sets alignment */
};
#pragma options align = reset
#pragma options align = mac68k
struct ClassList {
    ClassList *next;
    TypeClass *base;
    SInt32 offset;
    SInt32 voffset;
    UInt8 access;
    Boolean is_virtual;
};
#pragma options align = reset
#pragma options align = mac68k
struct VClassList {
    VClassList *next;
    TypeClass *base;
    SInt32 offset;
    SInt32 voffset;
    Boolean has_override;
};
#pragma options align = reset
#pragma options align = mac68k
struct BClassList {
    BClassList *next;
    Type *type;
};
#pragma options align = reset
/* A class's virtual function table: its object, the class that owns it, the offset of the table pointer in the
 * class and the table's size (CABI lays it out). */
#pragma options align = mac68k
struct VTable {
    Object *object;
    TypeClass *owner;
    SInt32 offset;
    int size;
};
#pragma options align = reset
/* Class records come in three sizes: write_typeclass copies 0x32 bytes for an ordinary class, 0x4a when
 * flags & 0x800 and 0x5a when flags & 0x100 (TypeClassExt800 / TypeClassTemplate below begin at 0x32). The
 * 0x2c..0x31 bytes are split by CClass (state at 0x2d), StackFrameEABI (align at 0x2e) and the CObjC views. */
/* A class template (its record is a TypeClassTemplate: write_typeclass copies 0x5a bytes) and a class instantiated from one
 * (a TypeClassExt800: 0x4a bytes). */
#define CLASS_IS_TEMPL 0x100
#define CLASS_IS_TEMPL_INST 0x800

#pragma options align = mac68k
struct TypeClass {
    UInt8 type;
    SInt32 size;
    NameSpace *nspace;
    HashNameNode *classname;
    ClassList *bases;
    VClassList *vbases;
    ObjMemberVar *ivars;
    struct CFriend *friends; /* 0x1a: CClass_CheckStaticAccess traverses the friend list and tests is_class */
    VTable *vtable;
    SOMInfo *sominfo;
    ObjCInfo *objcinfo;
    UInt16 flags;
    UInt8 mode;
    UInt8 state;
    SInt16 align;
    UInt8 eflags;
    UInt8
        alignmentPadding; /* 0x31: CDecl_DefineClass clears sizeof(*type); unused trailing byte rounds the class header to its two-byte alignment. */
};
#pragma options align = reset
/* flags & 0x800: 0x4a bytes (CTemplateNew reads flags and the byte at 0x47 of these). Members at 0x36 and 0x3a
 * are written through the type writer, 0x3e and 0x42 through the list writer serialize_ct_state_elems. */
#pragma options align = mac68k
struct TypeClassExt800 {
    TypeClass base; /* 0x00: create_class_template_instance initializes the instantiated class header */
    TypeClassExt800 *
        next; /* 0x32: create_class_template_instance links definition->instances; CTemplateClass_GetInstance walks instances */
    Type *relatedClass;  /* 0x36: create_class_template_instance copies definition->relatedClass */
    Type *classTemplate; /* 0x3a: create_class_template_instance stores the defining class template */
    struct CTStateElem
        *targs; /* 0x3e: CTemplateClass_GetInstance compares template arguments with CTemplTool_EqualArgs */
    struct CTStateElem *
        templateArgumentOverride; /* 0x42: CTemplateClass_GetInstance selects alternate arguments for CTemplTool_EqualArgs */
    UInt8 instantiating;          /* 0x46: CDecl marks an instance being instantiated */
    UInt8
        suppressImplicitInstantiation; /* 0x47: CBrowse_GenerateClassRecord tests suppression before following classTemplate */
    UInt8 memberInstantiationState
        [2]; /* 0x48: instantiate_members tests member instantiation state and marks data members processed */
};
#pragma options align = reset
/* flags & 0x100: 0x5a bytes; every pointer from 0x36 on is relocated by write_typeclass. */
#pragma options align = mac68k
struct ExceptSpecList {
    ExceptSpecList *next;
    Type *type;
    UInt32 qual;
};
#pragma options align = reset
#pragma options align = mac68k
struct FuncArg {
    FuncArg *next;
    HashNameNode *name;
    ENode *dexpr;
    Type *type;
    UInt32 qual;
    SInt16 sclass;
    Boolean is_array;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeFunc {
    UInt8 type;
    SInt32 size;
    FuncArg *args;
    ExceptSpecList *exspecs;
    Type *functype;
    UInt32 qual;
    UInt32 flags;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeMemberFunc {
    UInt8 type;
    SInt32 size;
    FuncArg *args;
    ExceptSpecList *exspecs;
    Type *functype;
    UInt32 qual;
    UInt32 flags;
    TypeClass *theclass;
    SInt32 vtbl_index;
    SInt32 funcid;
    Boolean is_static;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeBitfield {
    UInt8 type;             /* 0x00: CDecl_ScanStructDeclarator sets bitfield kind 7 */
    SInt32 size;            /* 0x02: CDecl_ScanStructDeclarator copies underlying type size */
    Type *bitfieldtype;     /* 0x06: CDecl_ScanStructDeclarator sets underlying declarator type */
    char offset;            /* 0x0a: CDecl_ScanStructDeclarator initially clears with memclrw */
    char bitlength;         /* 0x0b: CDecl_ScanStructDeclarator sets constant bit width */
    char suppressAlignment; /* 0x0c: CDecl_ScanStructDeclarator sets for unnamed bitfields */
    char
        alignmentPadding; /* 0x0d: CDecl_ScanStructDeclarator and make_bitfield_type clear the whole 0x0e-byte record; no member access, trailing byte after suppressAlignment at 0x0c. */
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeMemberPointer {
    UInt8 type;
    SInt32 size;
    Type *memberType;
    union {
        Type *type;
        TypeClass *classType;
    } owner;
    UInt32 qual;
};
#pragma options align = reset
#pragma options align = mac68k
struct TemplParamID {
    UInt16 index;
    UInt8 nindex;
    Boolean type;
};
#pragma options align = reset
/* A template-dependent type (TYPETEMPLATE); dtype selects the union member: 0 argument, 1 qualified name, 2 template,
 * 3 array, 4 qualified template, 5 bitfield. */
#pragma options align = mac68k
struct TypeTemplDep {
    UInt8 type;
    SInt32 size;
    UInt8 kind;
    union {
        TemplParamID pid;
        struct {
            struct TypeTemplDep *type;
            HashNameNode *name;
        } qual;
        struct {
            struct TypeClassTemplate
                *templ; /* 0x08: CTemplTool_IsTemplate, kind == 2, reads templateParameters and specializations */
            struct CTStateElem *args; /* 0x0c: write_templdep, kind == 2, serialize_ct_state_elems */
        } templ;
        struct {
            Type *type;
            ENode *index;
        } array;
        struct {
            struct TypeTemplDep *type;
            struct CTStateElem *args;
        } qualtempl;
        struct {
            Type *type;
            ENode *size;
        } bitfield;
    } u;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypePointer {
    UInt8 type;
    SInt32 size;
    Type *target;
    UInt32 qual;
    struct ObjectList *protocols[0];
};
#pragma options align = reset
extern TypeIntegral stbool;
extern TypeIntegral stchar;
extern TypeIntegral stsignedchar;
extern TypeIntegral stunsignedchar;
extern TypeIntegral stwchar;
extern TypeIntegral stsignedshort;
extern TypeIntegral stunsignedshort;
extern TypeIntegral stsignedint;
extern TypeIntegral stunsignedint;
extern TypeIntegral stsignedlong;
extern TypeIntegral stunsignedlong;
extern TypeIntegral stsignedlonglong;
extern TypeIntegral stunsignedlonglong;
extern TypeIntegral stfloat;
extern TypeIntegral stshortdouble;
extern TypeIntegral stdouble;
extern TypeIntegral stlongdouble;
extern TypeStruct stvectorunsignedchar;
extern TypeStruct stvectorsignedchar;
extern TypeStruct stvectorboolchar;
extern TypeStruct stvectorunsignedshort;
extern TypeStruct stvectorsignedshort;
extern TypeStruct stvectorboolshort;
extern TypeStruct stvectorunsignedlong;
extern TypeStruct stvectorsignedlong;
extern TypeStruct stvectorboollong;
extern TypeStruct stvectorfloat;
extern TypeStruct stvectorpixel;
extern TypeStruct stvector;
extern Type stvoid;
extern TypePointer void_ptr;

#ifdef __cplusplus
}
#endif

#endif
