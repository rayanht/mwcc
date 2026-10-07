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
#define TYPE_TEMPLATE(ty) ((TypeTemplDep *)(ty))
#define TPTR_TARGET(ty) (TYPE_POINTER(ty)->target)
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
#define IS_TYPESTRUCT_VECTOR(ty) ((ty)->stype >= STRUCT_VECTOR_UCHAR && (ty)->stype <= STRUCT_VECTOR_PIXEL)
#define IS_TYPE_VECTOR(ty) ((ty)->type == TYPESTRUCT && IS_TYPESTRUCT_VECTOR(TYPE_STRUCT(ty)))
#define IS_TYPE_NONVECTOR_STRUCT(ty) ((ty)->type == TYPESTRUCT && !IS_TYPESTRUCT_VECTOR(TYPE_STRUCT(ty)))
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
    UInt8 type;
    SInt32 size;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeIntegral {
    UInt8 type;
    SInt32 size;
    UInt8 integral;
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
enum {
    STRUCT_TYPE_STRUCT = 0,
    STRUCT_TYPE_UNION = 1,
    STRUCT_TYPE_CLASS = 2,
    STRUCT_TYPE_MAX = 3,
    STRUCT_VECTOR_UCHAR = 4,
    STRUCT_VECTOR_SCHAR = 5,
    STRUCT_VECTOR_BCHAR = 6,
    STRUCT_VECTOR_USHORT = 7,
    STRUCT_VECTOR_SSHORT = 8,
    STRUCT_VECTOR_BSHORT = 9,
    STRUCT_VECTOR_UINT = 10,
    STRUCT_VECTOR_SINT = 11,
    STRUCT_VECTOR_BINT = 12,
    STRUCT_VECTOR_FLOAT = 13,
    STRUCT_VECTOR_PIXEL = 14
};
#pragma options align = mac68k
struct TypeStruct {
    UInt8 type;
    SInt32 size;
    HashNameNode *name;
    StructMember *members;
    SInt8 stype;
    SInt16 align;
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
/* TypeClass flags: a TemplClass, a TemplClassInst */
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
    struct ClassFriend *friends;
    VTable *vtable;
    SOMInfo *sominfo;
    ObjCInfo *objcinfo;
    UInt16 flags;
    UInt8 mode;
    UInt8 action;
    SInt16 align;
    UInt8 eflags;
};
#pragma options align = reset
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
    UInt8 type;
    SInt32 size;
    Type *bitfieldtype;
    char offset;
    char bitlength;
    char suppressAlignment;
};
#pragma options align = reset
#pragma options align = mac68k
struct TypeMemberPointer {
    UInt8 type;
    SInt32 size;
    Type *ty1;
    Type *ty2;
    UInt32 qual;
};
#pragma options align = reset
/* TypeTemplDep dtype: which arm of u */
enum { TEMPLDEP_ARGUMENT, TEMPLDEP_QUALNAME, TEMPLDEP_TEMPLATE, TEMPLDEP_ARRAY, TEMPLDEP_QUALTEMPL, TEMPLDEP_BITFIELD };
#pragma options align = mac68k
struct TypeTemplDep {
    UInt8 type;
    SInt32 size;
    UInt8 dtype;
    union {
        TemplParamID pid;
        struct {
            struct TypeTemplDep *type;
            HashNameNode *name;
        } qual;
        struct {
            struct TemplClass *templ;
            struct TemplArg *args;
        } templ;
        struct {
            Type *type;
            ENode *index;
        } array;
        struct {
            struct TypeTemplDep *type;
            struct TemplArg *args;
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
