#ifndef COMPILER_CINLINE_H
#define COMPILER_CINLINE_H

#include "compiler/common.h"
#include "compiler/tokens.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How the inliner is placing the values of the function being inlined (evalMode), and whether its temporaries
   are global (alloc_state). */
typedef enum AllocState { AS_NONE = 0, AS_GLOBAL = 1 } AllocState;
typedef enum EvalMode { EM_NONE = 0, EM_REG = 2, EM_ARG = 3, EM_LOCAL = 4 } EvalMode;
#pragma pack(push, 1)
struct CIBEntry {
    SInt16 statementIndex;
    CInt64 caseValue;
};
#pragma pack(pop)
#pragma options align = mac68k
struct CInlineInfo {
    SInt16 nargs;
    struct CInlineVar *arginfo;
    SInt16 nlocals;
    struct CInlineVar *localinfo;
    UInt16 nstmts;
    struct IStmtRec *stmtinfo;
    FileOffsetInfo fileinfo;
    UInt32 f1c;
    UInt32 tokenoffset;
    UInt32 tokenline;
    UInt8 kind;
};
#pragma options align = reset
#pragma options align = mac68k
struct CInlineVar {
    struct HashNameNode *name;
    struct Type *type;
    UInt32 qual;
    UInt8 storageFlags;
    UInt8 used;
    UInt8 dirty;
    UInt8 alignmentByte;
};
#pragma options align = reset
#pragma options align = mac68k
struct ChainRec {
    struct ChainRec *next;
    struct Statement *node;
    struct IStmtRec *ent;
};
#pragma options align = reset
#pragma options align = mac68k
/* ExcBase is opaque: CInline.h only retains EMemberInfo::base; no code uses its members. */
struct ExcBase;
#pragma options align = reset
#pragma options align = mac68k
struct IFixup {
    struct IFixup *next;
    struct CLabel **destination;
    SInt16 labelIndex;
};
#pragma options align = reset
#pragma options align = mac68k
struct IStmtRec {
    UInt8 type;
    UInt8 flags;
    UInt16 value;
    UInt32 sourceoffset;
    struct ExceptionAction *exceptionActions;
    union {
        struct ParsedAsmInstruction *assembly;
        struct ENode *operand;
        struct InlineSwitchData *switchInfo;
        SInt16 targetIndex;
    } data;
    union {
        UInt32 assemblyData;
        SInt16 targetIndex;
    } secondaryOperand;
};
#pragma options align = reset
#pragma options align = mac68k
/* InlNode is opaque: CInline.h retains only IStmtRec::list; no code uses its members. */
struct InlNode;
#pragma options align = reset
#pragma pack(push, 1)
struct InlineMemberPointerTarget {
    UInt8 metadata[6];
    UInt8 flags;
};
#pragma pack(pop)
#pragma options align = mac68k
struct InlineNode {
    struct InlineNode *next;
    struct Object *func;
    struct CInlineInfo *body;
    Boolean flag;
};
#pragma options align = reset
struct InlineObjectEntry {
    struct InlineObjectEntry *next;
    struct Object *object;
};
#pragma options align = mac68k
struct InlineSlot {
    struct Object *var;
    struct ENode *expr;
    struct ENode *arg;
};
#pragma options align = reset
#pragma pack(push, 1)
struct InlineSwitchData {
    struct ENode *expression;
    struct Type *valueType;
    SInt16 defaultStatementIndex;
    SInt16 caseCount;
    CIBEntry entries[1];
};
#pragma pack(pop)
#pragma options align = mac68k
struct MemoNode {
    struct MemoNode *next;
    ENode *key;
    SInt32 val;
};
#pragma options align = reset
extern Boolean anyerrors;
extern Boolean anyerrors;
extern ObjectList *locals;
extern Boolean CInline_DispatchNextDeferredNode(void);
extern void make_auto_generated_method(Object *func);
extern void fn_0050ee60(Statement *stmt, Object *func, Boolean flag);
extern Boolean check_statement_count_and_locals_size(Object *func, Statement *stmt);
extern Boolean fn_0050f120(struct InlineObjectEntry *list);
extern void collect_undefined_function_objects(struct CInlineInfo *c);
extern void forward_statement_objrefs(Statement *stmt);
extern void CInline_AddSpecialization(Object *func, void *a, void *b);
extern void CInline_AddFunctionPrecNode(Object *func, TypeClass *value, FileOffsetInfo *key, TokenStream *pair,
                                        Boolean flag);
extern ExceptionAction *fn_005102f0(Statement *indexMap, Statement *info);
extern void inline_statement_list(Statement *list);
extern void fn_005114e0(ENode *node);
extern Statement *inline_statement(Statement *statement);
extern Statement *expand_inline_calls(Statement *stmt);
extern Statement *generate_inline_statements(Object *function, Statement *tail, CInlineInfo *args, ENode *result,
                                             CLabel *returnLabel, Object *returnObject, UInt8 appendStatement);
extern ExceptionAction *copy_exception_actions(IStmtRec *parent, char copyExpressions);
extern void reconstruct_switch_info(Statement *arg1, IStmtRec *arg2, CLabel **table);
extern ENode *inline_expression(ENode *node);
extern Boolean can_inline(ENode *node);
extern ENode *inline_call_expression(ENode *expr);
extern void fn_005129f0(ENode *expr);
extern void forward_objref(ENode *expr);
extern ENode *copy_result_reference(ENode *e);
extern ENode *fn_00513040(ENode *expr, UInt8 mode);
extern unsigned char fn_0050ebc0(void);
extern void add_undefined_exception_function_objects(ExceptionAction *entry);
extern void add_undefined_function_object(Object *object);
extern void generate_inline_code(Object *object, CInlineInfo *input, char mode);
extern unsigned char fn_00511180(Object *function, Statement *statement);
extern SInt16 CInline_GetStatementIndex(Statement *link, Statement *target);
extern Object *CInline_GetObjectByIndex(UInt32 index, char useTable);
extern void set_object_sclass(Object *object, UInt8 kind);
extern void fn_005130b0(ENode *node, Boolean flag);
extern ENode *fn_00513240(ENode *e);
extern ENodeList *copy_enode_list(ENodeList *values);
extern Boolean fn_00513910(ENode *expr);
extern unsigned int CInline_GetObjectIndex(void *object);
extern ENode *fold_constants(ENode *node);
extern ENode *setup_inline_locals_and_arguments(Object *function, CInlineInfo *inlineInfo, ENodeList *arguments);
extern void CInline_ReconstructFunction(Object *unused, CInlineInfo *rec, Statement *out);
extern Statement *try_inline_statement(Statement *obj, char *flag);
extern void fn_0050f240(Object *object);
extern void CInline_SaveInfo(CInlineInfo *out, Statement *list, Object *function);
struct CPrecNode;
extern void parse_inline_definition(struct CPrecNode *inlineInfo);
extern Object *create_local_object(Type *type, unsigned int qual, unsigned int storageClassFlags);
extern void *create_inline_switch_data(Statement *base, Statement *classInfo);
extern SInt16 CInline_ReturnZero(Type *type);
extern void CInline_GeneratePendingFunctionBody(void);
extern PendingFunction *generate_guarded_initializers(PendingFunction *pending);
extern UInt32 function_token_line;
extern struct CPrecNode *pendingInlineWork;
extern void fn_00514220(void);
extern UInt32 function_tokenoffset;
extern SInt32 data_00587184;

#ifdef __cplusplus
}
#endif

#endif
