#ifndef COMPILER_IROLOOP_H
#define COMPILER_IROLOOP_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An address expression split into its terms (IroVars_CreateAddrRecord creates one, IroVars_CollectAddrRecordElements collects the operands of
   its EADD tree): the object references, the integer constants and everything else. */
#pragma options align = mac68k
struct IROAddrRecord {
    struct IROLinear *linear;
    UInt8 x4;
    UInt8 alignmentPadding;
    SInt16 numObjRefs;
    IROElmList *objRefs;
    SInt16 numMisc;
    IROElmList *misc;
    SInt16 numInts;
    IROElmList *ints;
    SInt32 x18;
};
#pragma options align = reset
/* The kinds of IR node (IROLinear.type), as the optimizer's dump prints them (IroDump: "Operand", "Goto", "If %d %s",
 * "Funccall %d(", "BeginCatch %d", ...) and as the statements they are built from give them (ST_ENTRY 11, ST_EXIT 12,
 * ST_ASM 16); 17 and 18 stay unnamed. */
enum {
    IROLinearNop,
    IROLinearOperand,
    IROLinearOp1Arg,
    IROLinearOp2Arg,
    IROLinearGoto,
    IROLinearIf,
    IROLinearIfNot,
    IROLinearReturn,
    IROLinearLabel,
    IROLinearSwitch,
    IROLinearFunccall,
    IROLinearEntry,
    IROLinearExit,
    IROLinearBeginCatch,
    IROLinearEndCatch,
    IROLinearEndCatchDtor,
    IROLinearAsm,
    IROLinearEnd = 19
};
/* An IR node's flags (IROLinear.flags), as the optimizer's dump prints them (<reffed>, <assigned>, <used>, ...); its other
 * bits are not named. */
#define IROLF_Reffed 0x2
#define IROLF_Assigned 0x4
#define IROLF_Used 0x10
#define IROLF_Ind 0x20
#define IROLF_Subs 0x40
#define IROLF_LoopInvariant 0x100
#define IROLF_BeginLoop 0x200
#define IROLF_EndLoop 0x400
#define IROLF_Ris 0x800
#define IROLF_Immind 0x1000
#define IROLF_VecOp 0x2000
#define IROLF_VecOpBase 0x10000
#define IROLF_CounterLoop 0x40000

#pragma options align = mac68k
struct IROLinear {
    UInt8 type;
    UInt8 nodetype;
    UInt32 flags;
    UInt16 nodeflags;
    UInt16 index;
    struct Statement *stmt;
    Type *rtype;
    struct IROExpr *expr;
    struct ERange *range;
    union {
        struct IROLinear *monadic;
        ENode *node;
        struct CLabel *label;
        struct Statement *asm_stmt;
        struct {
            struct CLabel *label;
            struct IROLinear *cond;
        } branch;
        struct {
            struct SwitchInfo *info;
            struct IROLinear *cond;
        } swtch;
        struct {
            struct IROLinear *left;
            struct IROLinear *right;
        } diadic;
        struct {
            struct IROLinear *a;
            struct IROLinear *b;
            struct IROLinear *c;
        } args3;
        struct {
            char ispure;
            SInt16 argCount;
            struct IROLinear **args;
            struct IROLinear *callee;
            struct TypeFunc *functype;
        } funccall;
    } u;
    struct IROLinear *next;
};
#pragma options align = reset
#pragma pack(push, 2)
struct IROList {
    struct IROLinear *head;
    struct IROLinear *tail;
};
#pragma pack(pop)
#pragma options align = mac68k
struct IROLoopInd {
    UInt32 flags;
    struct VarRecord *var;
    struct IRONode *node;
    struct IROLinear *nd;
    SInt32 addConst;
    struct IROLinear *step;
    struct IROLoopInd *next;
};
#pragma options align = reset
/* A loop's analysis (fn_0045faa0, 0x3c bytes): its blocks, its test and the induction variable the test uses. */
#pragma options align = mac68k
struct IROLoop {
    UInt32 flags;
    struct IRONode *fnode;
    UInt32 x8;
    struct IRONode *preheader;
    struct IRONode *body;
    struct IROLinear *init;
    struct IROLinear *cond;
    IROLoopInd *induction;
    SInt32 lo;
    SInt32 hi;
    CInt64 x28;
    CInt64 x30;
    SInt32 count;
};
#pragma options align = reset
#pragma options align = mac68k
struct IRONode {
    UInt16 index;
    UInt16 numsucc;
    UInt16 *succ;
    UInt16 numpred;
    UInt16 *pred;
    struct IROLinear *first;
    struct IROLinear *last;
    struct BitVector *in;
    struct BitVector *out;
    struct BitVector *gen;
    struct BitVector *kill;
    UInt32 x26;
    struct BitVector *copyOut;
    struct BitVector *dom;
    struct IRONode *nextnode;
    Boolean reachable;
    Boolean visited;
    Boolean mustreach;
    Boolean referenced;
    UInt16 loopdepth;
};
#pragma options align = reset
#pragma options align = mac68k
struct LoopCandidate {
    unsigned char flags;
    unsigned char reserved01;
    struct BitVector *blocks;
    struct LoopCandidate *next;
    struct IRONode *header;
    unsigned int value0e;
};
#pragma options align = reset
extern int combine_nonoverlapping_shifts(CInt64 a, CInt64 b, CInt64 *out);
extern unsigned int IRO_FindLoops_Unroll(void);
extern void IRO_LoopUnroller(void);
extern void find_induction_variables(void);
extern void mark_nonintersecting_linears(void);
extern void IRO_FindLoops(void);
extern void split_last_linear_into_new_node(void);
extern void IroLoop_ComputeLoopDepth(void);
extern void fn_00461dc0(IROLinear *type, unsigned int enabled);
extern void forward_expr_if_check_object(IROLinear *entry, int checkObject);
extern void fn_00460f70(void);
extern void unroll_loop(int factor, struct IRONode *header);
extern void IRO_CollectLoopBlocks_004614f0(IRONode *lp);
extern void IroLoop_0045c520(IROLoop *p1, CInt64 *p2, int *p3, int *p4, int *p5, int *p6);
extern int compute_loop_count(IROLoop *loop, CInt64 *count);
extern int is_loop_unrollable(IROLoop *loop);
extern int compute_positive_addr_record_difference(IROAddrRecord *first, IROAddrRecord *second, int context,
                                                   CInt64 *difference);
extern void find_induction_init(struct IROLoop *state, struct IRONode *list);
extern void reduce_strength_and_move_loop_invariants(IRONode *func);
extern IROExpr *IroLoop_00461860(IROExpr *root, IROLinear *initial, IROLinear *step, IROLoopInd *context, SInt32 mode);
extern IRONode *insert_loop_preheader(IRONode *p1, IRONode *p2);
extern IROLoop *fn_0045faa0(IRONode *loop);
extern int match_induction_expression(IROLinear *node, IROLinear **factor, IROLinear **expression,
                                      VarRecord **variable);
extern struct IROLinear *create_loop_iteration_count(struct IROList *context, struct IROLoop *statement);
extern IROLinear *reorder_sequence_by_flags_and_type50(IROLinear *tree, IROList *context);
extern void rewrite_selected_monadic_references(void);
extern void append_scaled_induction_update(IROLoop *owner, SInt32 count, IROList *arg);
extern void add_const_to_induction_var_references(IROLinear *first, IROLinear *last, SInt32 addConst, IROLoop *loop);
extern void create_source_constant_branch(IROList *list, IROLinear *source, SInt32 value, struct CLabel *destination);
extern IROLinear *create_induction_offset_temporary(IROLinear *object, SInt32 count, IROList *instructions,
                                                    IROLoop *loop);
extern IROLinear *create_induction_difference_temporary(IROLinear *object, SInt32 unused, IROList *list, IROLoop *loop);
extern IROLinear *create_loop_ind_temporary(IROLoopInd *loop, long value, IROList *chain);
extern IROLinear *create_bound_offset_temporary(IROLinear *object, int multiplier, IROList *context, IROLoop *info);
extern IROLinear *create_cond_left_add_assignment(IROList *list, IROLoop *holder);
extern int is_value_preserving_integral_conversion(IROLinear *op);
extern void compute_mustreach(void);
extern void flatten_linear_to_elm_list(IROLinear *n);
extern struct BitVector *IRO_LoopScratchVector_005880dc;
extern struct IROElmList *iro_elm_list_head;
extern struct LoopCandidate *loop_candidates;
extern int linear_index_counter;
extern struct BitVector *data_005876bc;
extern struct IROElmList *elm_list_tail;
extern UInt16 iro_node_count;
extern UInt8 data_0058851c;
extern struct IRONode *data_00587c68;
extern struct IRONode *iro_flowgraph_head;
extern struct IRONode **iroNodesByIndex;
extern struct IRONode *data_00587fac;

#ifdef __cplusplus
}
#endif

#endif
