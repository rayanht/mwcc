#ifndef COMPILER_CEXCEPTION_H
#define COMPILER_CEXCEPTION_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

union InlineOperand {
    SInt32 value;
    struct Object *object;
    struct InlineIndexReference *reference;
    UInt8 byte;
};
#pragma options align = mac68k
struct ExceptionAction {
    struct ExceptionAction *next;
    union {
        struct {
            struct Object *object;
            struct Object *dtor;
            SInt32 offset;
        } local;
        struct {
            struct Object *object;
            struct Object *cond;
            struct Object *dtor;
        } local_cond;
        struct {
            struct Object *pointer;
            struct Object *dtor;
        } local_pointer;
        struct {
            struct Object *context;
            struct Object *dtor;
            struct Object *value1;
            void *value2;
        } call;
        struct {
            void *type[4];
        } types;
        struct {
            struct Object *objectptr;
            struct Object *dtor;
            SInt32 offset;
        } member;
        struct {
            struct Object *objectptr;
            struct Object *cond;
            struct Object *dtor;
            SInt32 offset;
        } member_cond;
        struct {
            struct Object *objectptr;
            struct Object *dtor;
            SInt32 offset;
            SInt32 count;
            SInt32 size;
        } member_array;
        struct {
            struct Object *pointer;
            struct Object *deletefunc;
        } delete_pointer;
        struct {
            struct Object *first;
            struct Object *second;
        } pair;
        struct {
            struct Object *pointer;
            struct Object *deletefunc;
            struct Object *cond;
        } delete_pointer_cond;
        struct {
            struct Object *object;
            struct Object *info;
            struct CLabel *label;
            struct Object *typeInfo;
            struct Type *exceptionType;
            UInt32 declarationData;
        } catch_block;
        struct {
            struct Object *info;
            Boolean call_dtor;
        } active_catch;
        struct {
            SInt32 count;
            struct Object **ids;
            struct CLabel *label;
            struct Object *info;
        } specification;
        void *slots[6];
        InlineOperand operands[6];
        UInt8 bytes[24];
    } data;
    UInt8 kind;
    UInt8 pad_1d;
};
#pragma options align = reset
struct ClassNode {
    struct ClassNode *next;
    struct TypeClass *cls;
    SInt32 offset;
    Boolean flagc;
    Boolean flagd;
    Boolean flage;
};
#pragma options align = mac68k
struct ECacheNode {
    struct ECacheNode *next;
    struct Object *obj;
    SInt32 key;
};
#pragma options align = reset
struct ExceptionHandlerRecord {
    struct ExceptionHandlerRecord *previous;
    struct Object *catchObject;
    struct Object *exceptionObject;
    struct Statement *handlerEntry;
    struct Statement *handlerEnd;
    struct Type *exceptionType;
    UInt32 declarationData;
};
struct InlineIndexReference {
    SInt32 value;
    SInt32 index;
};
#pragma options align = mac68k
struct TemporaryObject {
    struct TemporaryObject *next;
    struct Object *object;
    struct Object *classObject;
    struct Object *initializationFlag;
};
#pragma options align = reset
extern void fn_004e0b20(ENode *expr);
extern void setup_exception_specification(struct Statement *statements, struct ExceptSpecList *handlers);
extern void insert_temporary_object_destruction(Statement *statement, char flag1, char flag2);
extern Statement *generate_temporary_object_destruction(Statement *arg);
extern ENode *rewrite_funccall_temporaries(ENode *node, Boolean reverse);
extern ENode *fn_004e1940(ENode *node);
extern Object *CException_GetTempObject(ENode *obj);
extern void CExcept_ScanTryBlock(void *context, char flag);
extern ENode *create_catch_object_init(DeclInfo *info, ExceptionHandlerRecord *args);
extern void fn_004e1fb0(Statement *firstScope, Statement *insertionScope, Statement *lastScope,
                        ExceptionHandlerRecord *entries);
extern ENode *CExcept_ScanThrowExpression(void);
extern ENode *create_call_with_arg_and_default_args(Object *func, TypeClass *cls, ENode *which, ENode *arg);
extern void CExcept_ScanExceptionSpecification(TypeFunc *func);
extern ENode *create_type_stringconst(Type *type, UInt32 qualifiers, Boolean flag);
extern ClassNode *add_class_and_bases(ClassNode *list, TypeClass *ctx, TypeClass *cls, SInt32 offset, Boolean a,
                                      Boolean b);
extern void mark_class_and_bases(ClassNode *list, TypeClass *cls);
extern void append_namespace_names(NameSpace *p);
extern void CExcept_RegisterMember(Statement *p1, Object *p2, SInt32 p3, Object *p4, Object *p5, Boolean p6);
extern void CExcept_ArrayInit(void);
extern void CExcept_Magic(void);
extern void CExcept_Terminate(void);
extern ENode *fn_004e1050(ENode *expression);
extern ENode *rewrite_expr_temporaries(ENode *expr);
extern void insert_exception_action(Statement *stmt, ExceptionAction *action);
extern void CExcept_RegisterDeleteObject(Statement *expr, Object *first, Object *second);
extern void CExcept_RegisterLocalArray(Statement *unused, Object *context, Object *destructor, SInt32 value1,
                                       SInt32 value2);
extern ENode *CExcept_RegisterDestructorObject(Object *obj, SInt32 value, Object *dtorobj, int flag);
extern unsigned char CExcept_ActionNeedsDestruction(ExceptionAction *entry);
extern void emit_flagged_class_offsets(TypeClass *type);
extern void fn_004e2940(TypeClass *exceptionData);
extern Boolean CExcept_ActionCompare(ExceptionAction *a, ExceptionAction *b);
extern void update_statement_dobjstacks(Statement *node);
extern void lower_newexception(ENode *node, Boolean useExpression);
extern void CExcept_CompareSpecifications(ExceptSpecList *a, ExceptSpecList *b);
extern void CExcept_CheckStackRefs(ExceptionAction *node);
extern void CExcept_RegisterMemberArray(Statement *stmt, Object *object, SInt32 offset, Object *dtor, SInt32 count,
                                        SInt32 size);
extern void CExcept_ExceptionTansform(Statement *stmt);
extern Statement *CExcept_ActionCleanup(ExceptionAction *input, Statement *statement);
extern unsigned char fn_004e0ab0(Statement *node);
extern void CExcept_Setup(void);
extern struct Object *throw_func;
extern struct ECacheNode *cached_objects;
extern SInt8 cexcept_magic;
extern UInt8 exception_cleanup_registered;
extern TypeIntegral stchar;
extern Type exception_temp_object_type;

#ifdef __cplusplus
}
#endif

#endif
