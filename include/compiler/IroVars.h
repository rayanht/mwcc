#ifndef COMPILER_IROVARS_H
#define COMPILER_IROVARS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct AsmOp {
    UInt8 type;             /* 0x00: InlineAsmPPC_00462d70 assigns operand access type */
    UInt8 alignmentPadding; /* 0x01: InlineAsmPPC.c: unused alignment byte before the two-byte-aligned object pointer */
    struct Object *object;  /* 0x02: InlineAsmPPC_00462d70 copies operand->target.object */
    SInt32 offset;          /* 0x06: InlineAsmPPC_00462d70 records zero or operand->data.value */
    SInt32 size;            /* 0x0a: InlineAsmPPC_00462d70 records the operand access size */
};
#pragma options align = reset
#pragma options align = mac68k
struct AsmOut {
    UInt8
        optimizationBarrier; /* 0x00: InlineAsmPPC_00462d70 sets for flags 0x400 or no flags & 3; IrOptimizer disables optimization */
    UInt8
        writesMemory; /* 0x01: InlineAsmPPC_00462d70 flags 0x40 & 8; IroVars invalidates noregister variables as for branchWithLink */
    UInt8 readsMemory; /* 0x02: InlineAsmPPC_00462d70 flags 0x40 & 0x10; IROUseDef marks reaching callDefs used */
    UInt8
        unmodeledControlFlow; /* 0x03: InlineAsmPPC_00462d70 non-link indirect branches, label differences, or branches without labels; IrOptimizer disables optimization */
    UInt8 branchWithLink;
    UInt8
        noFallthrough; /* 0x05: InlineAsmPPC_00462d70 sets this for opcode 0; IroFlowgraph_RebuildSuccPred omits the fallthrough successor */
    SInt32 numoperands;
    SInt32 numlabels;
    AsmOp operands[16];
    struct CLabel *labels[16];
};
#pragma options align = reset
#pragma options align = mac68k
struct IROElmList {
    struct IROLinear *element;
    struct IROElmList *next;
};
#pragma options align = reset
#pragma options align = mac68k
struct VarRecord {
    UInt16 index;           /* 0x00: fn_0044ba70 assigns the variable index */
    struct Object *object;  /* 0x02: fn_0044ba70 records the variable object */
    SInt32 x6;              /* 0x06: fn_0044ba70 initializes to zero */
    UInt8 inductionState;   /* 0x0a: IroLoop.c tests induction updates and transitions states 1 and 2 */
    UInt8 noregister;       /* 0x0b: fn_0044ba70 mode 0 also sets VarInfo::noregister; record_var_part_use rejects it */
    UInt8 usedAtCall;       /* 0x0c: mark_var_used_at_call marks an object used by the call visitor */
    UInt8 xD;               /* 0x0d: unused */
    struct VarRecord *next; /* 0x0e: fn_0044ba70 appends to var_records */
    struct IRODef *defs;    /* 0x12: mark_var_used_at_call walks reaching definitions */
    struct IROUse *uses;    /* 0x16: AddUse links variable uses */
    struct VarRecordAux *x1a; /* 0x1a: fn_0044ba70 initializes to NULL; opaque, otherwise unused */
    struct IROLinear
        *bitFieldReplacement; /* 0x1e: RewriteBitFieldTemps clones this expression to replace the bit-field temporary */
};
#pragma options align = reset
#pragma options align = mac68k
struct IROVarPart {
    struct IROLinear *source; /* 0x00: record_var_part_use records the originating indirect access */
    UInt8 flags;              /* 0x04: record_var_part_use sets bit 1 to inhibit IRO_ScalarizeClassDataMembers */
    UInt8 alignmentPadding;   /* 0x05: IroVars.c: alignment byte between flags and the two-byte-aligned var pointer */
    struct VarRecord *var;    /* 0x06: record_var_part_use compares variable indices */
    int offset;               /* 0x0a: record_var_part_use checks overlapping member ranges */
    int size;                 /* 0x0e: record_var_part_use initializes from type->size */
    UInt8 x12[8];             /* 0x12: IroVars.c: no observed accesses */
    Type *type;               /* 0x1a: IRO_ScalarizeClassDataMembers creates a temporary of this type */
    struct IROVarPart *next;  /* 0x1e: record_var_part_use appends to class_data_parts */
    struct IROElmList *uses;  /* 0x22: IRO_ScalarizeClassDataMembers rewrites these member address uses */
    UInt8 x26[4];             /* 0x26: IroVars.c: no observed accesses */
};
#pragma options align = reset
extern void IRO_ScalarizeClassDataMembers(void);
extern void RewriteBitFieldTemps(void);
extern void IroVars_CheckVariablesInitializedBeforeUse(void);
extern void IroVars_0044b2d0(IROLinear *p);
extern VarRecord *IroVars_GetOperandVarRecord(IROLinear *node);
extern void IroVars_CollectAddrRecordElements(IROLinear *tree, IROAddrRecord *collection);
extern void IroVars_BuildNoregisterBitVector(void);
extern void IroVars_ClearObjectVarRecords(void);
extern void IroVars_PrependElmList(IROLinear *node, IROElmList **head);
extern void fn_0044b4e0(IROLinear *node);
extern SInt32 record_monadic_var_part_use(IROLinear *obj);
extern void IroVars_BuildVarRecords(void);
extern int record_var_part_use(IROLinear *source, VarRecord *operand, int offset, Type *type);
extern VarRecord *fn_0044ba70(Object *object, unsigned int create, unsigned int mode);
extern void IroVars_CheckTimedLongjmp(void);
extern IROLinear *IroVars_CreateIntConstant(CInt64 value, Type *type);
extern IROAddrRecord *IroVars_CreateAddrRecord(struct IROLinear *linear);
extern void visit_dobjstack_objects(IROLinear *linear);
extern void IroVars_VisitExceptionOperands(ExceptionAction *node, void (*visitOperand)(Object *));
extern IROLinear *IroVars_NopOutWithSideEffectsChecking(IROLinear *node);
extern IROLinear *fn_0044be00(IROLinear *node);
extern struct VarRecord *var_records;
extern SInt32 data_00587ef4;
extern unsigned int iroVarCount;
extern struct VarRecord *var_records_tail;
extern struct IROVarPart *var_part_use_tail;
extern unsigned int data_00588234;
extern struct IROVarPart *class_data_parts;
extern struct BitVector *noregister_bitvector;
extern UInt8 data_00588510;

#ifdef __cplusplus
}
#endif

#endif
