#ifndef COMPILER_CFUNC_H
#define COMPILER_CFUNC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Statement kinds (Statement.type): labels are defined by kind 2 and jumped to by kind 3, expressions are kind 4,
   conditional jumps 6 and 7, returns 8 and inline assembly 16. */
enum {
    ST_NOP = 1,
    ST_LABEL,
    ST_GOTO,
    ST_EXPRESSION,
    ST_SWITCH,
    ST_IFGOTO,
    ST_IFNGOTO,
    ST_RETURN,
    ST_OVF,
    ST_EXIT,
    ST_ENTRY,
    ST_BEGINCATCH,
    ST_ENDCATCH,
    ST_ENDCATCHDTOR,
    ST_GOTOEXPR,
    ST_ASM,
    ST_BEGINLOOP,
    ST_ENDLOOP
};
#pragma options align = mac68k
struct Statement {
    Statement *next; /* 0x00: CodeGen traverses the statement list */
    UInt8 type;      /* 0x04: CodeGen selects the statement kind */
    UInt8 marked;    /* 0x05: COptimizer marks visited statements */
    UInt8 flags;     /* 0x06: CodeGen tests source-location flags */
    UInt8 unk07;
    SInt16 value; /* 0x08: CodeGen passes this to set_block_line_and_execution_weight */
    union {
        ENode *expression; /* 0x0a: CodeGen kinds 4, 6, 7, 8 and 15 evaluate expressions */
        struct ParsedAsmInstruction
            *asmInstruction; /* 0x0a: CodeGen kind 16; InlineAsmPPC_00462d70 reads opcode and operands */
    } expr;
    union {
        struct CLabel *label; /* 0x0e: CodeGen kinds 2, 3, 6 and 7 use a label */
        struct SwitchInfo *
            switchDescriptor; /* 0x0e: parse_statement creates kind 5; mark_reachable_statements kind 5 reads cases and defaultlabel; build_linear_from_statements kind 5 reads this descriptor */
    } target;
    struct ExceptionAction *dobjstack; /* 0x12: IroFlowgraph_RebuildSuccPred walks active exception scopes */
    SInt32 sourceoffset;               /* 0x16: CodeGen passes source position to set_block_line_and_execution_weight */
};
#pragma options align = reset
/* A function queued for code generation after the translation unit has been parsed (list head DAT_005876e8,
 * emitted by CInline_GeneratePendingFunctionBody and written to precompiled headers by the CPrec list writer). Consecutive entries
 * with the same `cls` are generated together as one __init__ body when that class has flag 0x800. */
struct PendingFunction {
    struct PendingFunction *next;
    Object *func;
    Object *cls;
};
#pragma options align = mac68k
struct CLabel {
    struct CLabel *next;
    union {
        struct Statement
            *stmt; /* 0x04: frontend statement-label variant, used by COptimizer before IRO_BuildflowGraph */
        struct IRONode *
            node; /* 0x04: IRO_BuildflowGraph and IroFlowgraph_RebuildSuccPred select the flowgraph variant for IROLinearLabel; unroll_loop installs basic blocks */
    } target;
    struct HashNameNode *uniquename;
    struct HashNameNode *name;
    struct PCodeLabel *pclabel;
};
#pragma options align = reset
/* Temporary-object cleanup list built by sub_47bca0. */
struct CleanNode {
    struct CleanNode *next;
    Object *object;
    Object *dtor;
};
#pragma options align = mac68k
struct CtorChain {
    CtorChain *next;
    UInt8 what;
    ENode *objexpr;
    union {
        ClassList *base;
        VClassList *vbase;
        ObjMemberVar *membervar;
    } u;
};
#pragma options align = reset
/* Inherited statement parsing context, copied for nested loops and switches. */
struct StatementContext {
    Type *thetype;
    UInt32 qual;
    SwitchInfo *switchinfo;
    CLabel *loopContinue;
    CLabel *loopBreak;
};
#pragma options align = mac68k
struct SwitchInfo {
    struct SwitchCase *cases; /* 0x00: parse_statement initializes cases; Switch_GenerateSwitch traverses case labels */
    struct CLabel
        *defaultlabel; /* 0x04: parse_statement initializes defaultlabel; Switch_GenerateSwitch reads default target */
    Type *sizetype;    /* 0x08: parse_statement stores expression rtype; CFunc converts case values */
};
#pragma options align = reset
extern void CFunc_Gen(Statement *context, Object *object, unsigned int options);
extern void parse_ctor_initializers(void);
extern void fn_00476e60(TypeClass *type);
extern NameSpace *CFunc_FuncGenSetup(Statement *stmt, Object *func);
extern ObjectList *create_arg_object_list(FuncArg *arg);
extern void CFunc_SetupNewFuncArgs(Object *func, FuncArg *args);
extern void CFunc_ParseScopedStatement(struct StatementContext *context);
extern ENode *initialize_argument_object(ENode *initData, Type *type, UInt32 flags);
extern ENode *parse_declarations(char mode, int singleDeclaration, char allowEmpty, char stopAfterDeclaration);
extern void register_destructor_object(Type *a1, Object *a2, long a3, long a4);
extern void append_localstatic_init_expr(ENode *expr);
extern void CFunc_CodeCleanup(Statement *stmt);
extern ENode *sub_47bca0(ENode *node);
extern ENode *isolate_diadic_right_cleanup(ENode *node);
extern void append_or_defer_expression_statement(ENode *node);
extern ENode *fn_0047bff0(ENode *statement);
extern void CFunc_WarnUnused(void);
extern ENode *rewrite_cond_with_cleannodes(ENode *node);
extern ENode *sub_47c050(ENode *node, struct CleanNode *args, Boolean flag);
extern void generate_conditional_jump(ENode *expr, CLabel *dest, CLabel *other, Boolean sense, Boolean flag);
extern void setup_function_arguments(Object *function, DeclInfo *body, Statement *state);
extern void CFunc_00476e70(TypeClass *theclass, struct CtorChain *inits);
extern void create_local_object_copy(Object *func, TypeIntegral *type, Type *type2, Boolean flag);
extern void declare_local_object(DeclInfo *declaration, TStreamElement *proto, char flag3, char flag4);
extern void rewrite_enode_list_nodes(ENodeList *entry);
extern void parse_statement(struct StatementContext *context);
extern void CFunc_ParseFuncDef(Object *func, DeclInfo *definition, TypeClass *scopeObject, Boolean isMember,
                               unsigned char scopeFlag, NameSpace *scope);
extern void parse_case_statement(struct StatementContext *context);
extern void check_function_result_automatic_variable(ENode *e);
extern void CFunc_0047b9a0(Statement *statement, Statement *expression);
extern Statement *insert_conditional_goto_cleanup(Statement *statement);
extern void CFunc_DestructorCleanup(Statement *first);
extern void CFunc_GenerateDummyFunction(Object *functionObject);
extern void InitExpr_Register(ENode *expr, Object *cls);
extern ENode *append_cleannode_dtors(ENode *left, struct CleanNode *list);
extern void CFunc_GenerateSingleExprFunc(Object *func, ENode *expr);
extern UInt32 statement_sourceoffset;
extern struct Statement *PTR_00587644;
extern struct CLabel *clabels;
extern struct ExceptionAction *UINT_00587fc4;
extern SInt32 current_statement_number;
extern struct HashNameNode *blank_argument_name;
extern struct CtorChain *ctor_initializers;
extern struct TypeClass *cscope_currentclass;
extern Object *cscope_currentfunc;
extern ENode *create_temp_node2(Type *type);
extern ENode *create_temp_node(Type *type);
extern Object *create_temp_object(Type *type);
extern ENode *(*data_0058757c)(Type *, UInt8);
extern CLabel *findlabel(void);
extern void CheckCLabels(void);
extern Statement *CFunc_InsertAfterStatement(int type, Statement *after);
extern Statement *CFunc_AppendStatement(int kind);
extern CLabel *newlabel(void);
extern FuncArg *parameter_type_list(DeclInfo *state);
extern UInt8 CFunc_ParseFakeArgList(char stop_at_comma);
extern unsigned int parse_func_args(int parameter);
extern ENode *CFunc_DefaultArg(Type *destination, SInt32 flags, FuncArg *value);
extern Boolean check_default_argument_reference(int value, Object *object);
extern void parse_old_style_parameter_names(DeclInfo *scope);
extern void fn_0047ca70(Type **pt);
extern void CFunc_SetupLocalVarInfo(Object *object);
extern unsigned char in_parameter_type_list;
struct CLabel;
struct Statement;
extern FileOffsetInfo function_fileinfo;

/* fn_0047cb60 allocates 0xe bytes with CompilerTools_AllocatePool for this saved-state list node. */
struct DeclBlock {
    struct DeclBlock *next;
    struct ExceptionAction *dobjstack;
    struct NameSpace *parent_nspace;
    UInt16 index;
};
extern void PPCError_RestoreGlobalValues(const struct DeclBlock *values);
extern struct DeclBlock *fn_0047cb60(void);

#ifdef __cplusplus
}
#endif

#endif
