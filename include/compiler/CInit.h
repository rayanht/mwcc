#ifndef COMPILER_CINIT_H
#define COMPILER_CINIT_H

#include "compiler/common.h"
#include "compiler/enode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CInit {
    struct ENode *expr;
    ENode exprbuf;
    UInt8 state;
    UInt8 usesConstructorSyntax;
    UInt8 parenthesized;
};
/* The initializer state CInit saves and restores around a nested initializer (cinit_state is the current one):
   the object being initialized, its node list, the output buffer and its use, and the emit callbacks. */
struct InitInfo {
    struct Object *obj;
    struct OLinkList *list;
    UInt32 unusedSlot;
    SInt32 expr_offset;
    void (*expr_cb)(struct Type *, struct ENode *, Boolean);
    Boolean expr_cb_called;
    Boolean hasRuntimeInitialization;
    Boolean useEmitCallback;
    void (*init_expr_register_cb)(struct ENode *);
    struct Object *emitObject;
    void (*insert_expr_cb)(struct ENode *);
    struct ENode *(*register_object_cb)(struct Type *, struct Object *, SInt32, SInt32);
    UInt8 *buffer;
    SInt32 bufferSize;
    SInt32 bufferUsed;
    struct InitInfo *next;
};
struct CInit_ArgumentList {
    struct CInit_ArgumentList *next;
    struct ENode *expression;
};
#pragma options align = mac68k
struct PooledString {
    struct PooledString *next;
    struct Object *obj;
    SInt32 offset;
    char *data;
    UInt32 size;
    UInt8 ispascal;
    UInt8 iswide;
};
#pragma options align = reset
#pragma options align = mac68k
struct FloatNode {
    UInt8 type;
    UInt8 cost;
    UInt16 flags;
    UInt8 ignored;
    UInt8 hascall;
    struct Type *rtype;
    Float floatval;
};
#pragma options align = reset
#pragma options align = mac68k
struct InitListItem {
    struct InitListItem *next;
    struct Object *object;
};
#pragma options align = reset
struct InitializerData {
    struct InitializerData *next;
    struct InitializerData *owner;
    unsigned char *buffer;
    SInt32 offset;
    SInt32 size;
    SInt32 capacity;
    SInt32 size_adjustment;
    struct InitializerEntry *entries;
    struct OLinkList *relocations;
    char unknown9;
};
struct InitializerEntry {
    struct InitializerEntry *next;
    Type *type;
    ENode *expression;
    SInt32 offset;
};
#pragma pack(push, 1)
struct NameEntry {
    struct NameEntry *next;
    Object *object;
    SInt32 offset;
    char *bytes;
    SInt32 length;
    char unsignedChar;
};
#pragma pack(pop)
extern void CInit_RewriteString(ENode *node, SInt32 flag_arg);
extern NameEntry *CInit_DeclarePooledWString(char *string, UInt32 length);
extern Object *CInit_DeclareString(const char *data, UInt32 length, UInt8 kind1, UInt8 kind2);
extern void CInit_InitializeStaticData(Object *initObject, void (*output)(ENode *));
extern ENode *CInit_AutoObject(Object *obj, Type *type, UInt32 qual);
extern void CInit_InitializeAutoData(Object *obj, void (*emitInitializer)(ENode *),
                                     void (*registerDestructor)(Type *, Object *, SInt32, SInt32));
extern Boolean initialize_from_assignment(Object *obj, Boolean flag);
extern void find_dtor_temp_arg(ENode *e);
extern void emit_indirect_assignment(Type *type, ENode *expr, Boolean flag);
extern ENode *create_temp_object_expr(Type *type, Boolean reportError);
extern ENode *create_scopebegin_node(Type *type);
extern void fn_004cfc50(Object *object);
extern void initialize_class_array(Object *obj, Type *type, Boolean flag);
extern void CInit_ExportConst(Object *obj);
extern ENode *CInit_004d0ae0(Object *obj, Type *type, UInt32 qual, void (*contextOffset)(Type *, ENode *, Boolean),
                             Boolean flag);
extern void CInit_004d1170(Type *type, ENode *expr, Boolean flag);
extern void initialize_object_at_offset(Type *arg1, ENode *arg2, Boolean arg3);
extern void initialize_data_by_type(Type *node, UInt32 mode, Boolean flag);
extern void initialize_array_data_by_type(TypePointer *tptr, UInt32 mode, Boolean flag);
extern void initialize_class_data(Type *t, Boolean flag);
extern void initialize_struct(Type *pt, Boolean brace);
extern void CInit_004d1d90(Type *type, ENode *node);
extern void init_int(Type *type, ENode *node);
extern void init_int_or_relocation(Type *arg0, ENode *arg1);
extern Boolean CInit_004d20e0(Type *type, ENode *initializer, SInt32 offset, char parseArguments);
extern Boolean initialize_class_object(Object *obj, Type *initObject, ENode *expr, SInt32 offset, Boolean parse);
extern ENode *create_destructor_registration_call(Type *p1, Object *p2, ENode *p3);
extern ENode *build_init_assignment(ENode *previous, ENode *base, SInt32 offset, Type *type, ENode *value);
extern void CInit_004d2700(InitializerData *data, Type *type, UInt32 qual, Boolean flag);
extern int initialize_typed_data(InitializerData *p1, CInit *p2, Type *ty, UInt32 fl, int p5);
extern void initialize_struct_data(InitializerData *ctx, CInit *ci, Type *type, UInt32 qual, Boolean allowIncomplete);
extern void CInit_004d3620(TypeBitfield *bf, unsigned char *ptr, CInt64 value);
extern void initialize_int(InitializerData *stage, ENode *expr, Type *type, UInt32 qual);
extern void initialize_pointer_or_intconst(InitializerData *ctx, ENode *node, Type *ns, UInt32 qual);
extern Boolean CInit_RelocInitCheck(ENode *node, Object **pobj, CInt64 *pval);
extern Boolean CInit_004d3b20(Type *type);
extern Boolean CInit_004d3ba0(Type *type);
extern UInt8 advance_initializer_state(CInit *ci);
extern void initialize_array_data(InitializerData *pool, CInit *iter, TypePointer *arg, UInt32 arg4, Boolean flag);
extern void append_initializer_entry(InitializerData *ctx, Type *type, ENode *expr);
extern void initialize_class_initializer_data(InitializerData *dst, CInit *op, Type *type, UInt32 qual, int flag);
extern NameEntry *CInit_DeclarePooledString(const char *name, SInt32 len, SInt8 flag);
extern void CInit_InitializeData(Object *obj);
extern void CInit_DeclareReadOnlyData(Object *object, void *buffer, void *args, int size);
extern void CInit_DeclareData(Object *object, void *buffer, void *args, SInt32 size);
extern void emit_object(Object *object, const void *buffer, struct OLinkList *args, unsigned int options,
                        Boolean useAlternate);
extern void CInit_DeclarePooledStrings(void);
extern ENode *parse_initializer_expression(ENode *node);
extern void write_buffer_at_offset(void *src, SInt32 offset, SInt32 size);
extern void CInit_Init(void);
extern void CInit_DefineTentativeData(void);
extern struct Object *destructor_registration_func;
extern struct InitInfo *cinit_state;

#ifdef __cplusplus
}
#endif

#endif
