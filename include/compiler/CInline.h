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
    SInt16 nargs;                 /* 0x00: CInline_SaveInfo counts arguments */
    struct CInlineVar *arginfo;   /* 0x02: CInline_SaveInfo saves argument variables */
    SInt16 nlocals;               /* 0x06: CInline_SaveInfo counts local variables */
    struct CInlineVar *localinfo; /* 0x08: CInline_SaveInfo saves local variables */
    UInt16 nstmts; /* 0x0c: CInline_SaveInfo counts statements; collect_undefined_function_objects iterates them */
    struct IStmtRec *
        stmtinfo; /* 0x0e: CInline_SaveInfo serializes statements; collect_undefined_function_objects searches their operands */
    FileOffsetInfo fileinfo; /* 0x12: CInline_SaveInfo saves function_fileinfo; serialize_cprec_rec clears it */
    UInt32 f1c;              /* 0x1c: CInline_SaveInfo saves data_00587184; serialize_cprec_rec clears it */
    UInt32 tokenoffset;      /* 0x20: CInline_SaveInfo saves function_tokenoffset */
    UInt32 tokenline; /* 0x24: CInline_SaveInfo saves function_token_line; CInline_ReconstructFunction restores it */
    UInt8 kind;       /* 0x28: CInline_SaveInfo sets fn_00511180 result */
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
    UInt8
        alignmentByte; /* 0x0f: CInline_SaveVars leaves this byte untouched; tail padding for the two-byte-aligned variable record */
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
/* ExcBase is opaque: CInline.h only retains MemberFuncRef::base; no code uses its members. */
struct ExcBase;
#pragma options align = reset
#pragma options align = mac68k
struct IFixup {
    struct IFixup *next; /* 0x00: copy_exception_actions links fixup_list */
    struct CLabel **
        destination; /* 0x04: copy_exception_actions supplies label slots; CInline_ReconstructFunction resolves them; CInline_00513240 supplies an expression slot */
    SInt16
        labelIndex; /* 0x08: copy_exception_actions saves the index; CInline_ReconstructFunction indexes the label table */
};
#pragma options align = reset
#pragma options align = mac68k
struct IStmtRec {
    UInt8 type;          /* 0x00: CInline_SaveInfo saves statement kind; write_prec_recs selects payload */
    UInt8 flags;         /* 0x01: CInline_SaveInfo saves statement flags */
    UInt16 value;        /* 0x02: CInline_SaveInfo saves statement value */
    UInt32 sourceoffset; /* 0x04: CInline_SaveInfo saves statement sourceoffset */
    struct CException *exceptionActions; /* 0x08: CInline_005102f0 saves actions; write_prec_recs serializes them */
    union {
        struct ParsedAsmInstruction *assembly; /* 0x0c: CInline_SaveInfo type 16 copies assembly */
        struct ENode *operand; /* 0x0c: CInline_SaveInfo types 4, 6, 7, 8, 12, 13, 14, 15 save expressions */
        struct InlineSwitchData *switchInfo; /* 0x0c: CInline_SaveInfo type 5 saves switch data */
        SInt16 targetIndex;                  /* 0x0c: CInline_SaveInfo type 3 saves branch index */
    } data;
    union {
        UInt32 assemblyData; /* 0x10: CInline_SaveInfo type 16 saves serialized assembly size */
        SInt16 targetIndex;  /* 0x10: CInline_SaveInfo types 6, 7 save branch index */
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
    struct InlineNode *next; /* 0x00: CInline_0050ee60 links deferredInlineNodes */
    struct Object *func;     /* 0x04: CInline_0050ee60 saves the function for generate_inline_code */
    struct CInlineInfo
        *body;    /* 0x08: CInline_0050ee60 saves CInline_SaveInfo output; generate_inline_code reconstructs it */
    Boolean flag; /* 0x0c: CInline_0050ee60 saves the CodeGen_Generator mode */
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
    struct ENode *
        expression; /* 0x00: create_inline_switch_data saves gen_name output; CInline_ReconstructFunction reconstructs it */
    struct Type *valueType; /* 0x04: create_inline_switch_data saves sizetype; reconstruct_switch_info restores it */
    SInt16
        defaultStatementIndex; /* 0x08: create_inline_switch_data indexes the default label; reconstruct_switch_info resolves it */
    SInt16 caseCount; /* 0x0a: create_inline_switch_data counts cases; reconstruct_switch_info iterates them */
    CIBEntry entries
        [1]; /* 0x0c: create_inline_switch_data saves case values and statement indices; reconstruct_switch_info restores cases */
};
#pragma pack(pop)
#pragma options align = mac68k
struct MemoNode {
    struct MemoNode *next; /* 0x00: CInline_Memo and MemoFirst link memo_list entries */
    ENode *key;            /* 0x04: CInline_Memo and MemoFirst store and compare the ENode key */
    SInt32 val;            /* 0x08: CInline_Memo and MemoFirst cache CParser_GetUniqueID() */
};
#pragma options align = reset
extern Boolean anyerrors;
extern Boolean anyerrors;
extern ObjectList *locals;
extern Boolean CInline_DispatchNextDeferredNode(void);
extern void make_auto_generated_method(Object *func);
extern void CInline_0050ee60(Statement *stmt, Object *func, Boolean flag);
extern Boolean check_statement_count_and_locals_size(Object *func, Statement *stmt);
extern Boolean CInline_0050f120(struct InlineObjectEntry *list);
extern void collect_undefined_function_objects(struct CInlineInfo *c);
extern void forward_statement_objrefs(Statement *stmt);
extern void CInline_AddSpecialization(Object *func, void *a, void *b);
extern void CInline_AddFunctionPrecNode(Object *func, TypeClass *value, FileOffsetInfo *key, TokenStream *pair,
                                        Boolean flag);
extern CException *CInline_005102f0(Statement *indexMap, Statement *info);
extern void inline_statement_list(Statement *list);
extern void CInline_005114e0(ENode *node);
extern Statement *inline_statement(Statement *statement);
extern Statement *expand_inline_calls(Statement *stmt);
extern Statement *generate_inline_statements(Object *function, Statement *tail, CInlineInfo *args, ENode *result,
                                             CLabel *returnLabel, Object *returnObject, UInt8 appendStatement);
extern CException *copy_exception_actions(IStmtRec *parent, char copyExpressions);
extern void reconstruct_switch_info(Statement *arg1, IStmtRec *arg2, CLabel **table);
extern ENode *inline_expression(ENode *node);
extern Boolean can_inline(ENode *node);
extern ENode *inline_call_expression(ENode *expr);
extern void fn_005129f0(ENode *expr);
extern void forward_objref(ENode *expr);
extern ENode *copy_result_reference(ENode *e);
extern ENode *fn_00513040(ENode *expr, UInt8 mode);
extern unsigned char fn_0050ebc0(void);
extern void add_undefined_exception_function_objects(CException *entry);
extern void add_undefined_function_object(Object *object);
extern void generate_inline_code(Object *object, CInlineInfo *input, char mode);
extern unsigned char fn_00511180(Object *function, Statement *statement);
extern SInt16 CInline_GetStatementIndex(Statement *link, Statement *target);
extern Object *CInline_GetObjectByIndex(UInt32 index, char useTable);
extern void set_object_sclass(Object *object, UInt8 kind);
extern void CInline_005130b0(ENode *node, Boolean flag);
extern ENode *CInline_00513240(ENode *e);
extern ENodeList *copy_enode_list(ENodeList *values);
extern Boolean CInline_00513910(ENode *expr);
extern unsigned int CInline_GetObjectIndex(void *object);
extern ENode *fold_constants(ENode *node);
extern ENode *setup_inline_locals_and_arguments(Object *function, CInlineInfo *inlineInfo, ENodeList *arguments);
extern void CInline_ReconstructFunction(Object *unused, CInlineInfo *rec, Statement *out);
extern Statement *try_inline_statement(Statement *obj, char *flag);
extern void CInline_0050f240(Object *object);
extern void CInline_SaveInfo(CInlineInfo *out, Statement *list, Object *a3);
struct CPrecNode;
extern void parse_inline_definition(struct CPrecNode *inlineInfo);
extern Object *create_local_object(Type *type, unsigned int qual, unsigned int a2);
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
