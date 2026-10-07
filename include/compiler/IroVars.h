#ifndef COMPILER_IROVARS_H
#define COMPILER_IROVARS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct AsmOp {
    UInt8 type;
    UInt8 alignmentPadding;
    struct Object *object;
    SInt32 offset;
    SInt32 size;
};
#pragma options align = reset
#pragma options align = mac68k
struct AsmOut {
    UInt8 optimizationBarrier;
    UInt8 writesMemory;
    UInt8 readsMemory;
    UInt8 unmodeledControlFlow;
    UInt8 branchWithLink;
    UInt8 noFallthrough;
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
    UInt16 index;
    struct Object *object;
    SInt32 x6;
    UInt8 inductionState;
    UInt8 noregister;
    UInt8 usedAtCall;
    UInt8 xD;
    struct VarRecord *next;
    struct IRODef *defs;
    struct IROUse *uses;
    struct VarRecordAux *x1a;
    struct IROLinear *bitFieldReplacement;
};
#pragma options align = reset
#pragma options align = mac68k
struct IROVarPart {
    struct IROLinear *source;
    UInt8 flags;
    UInt8 alignmentPadding;
    struct VarRecord *var;
    int offset;
    int size;
    UInt8 x12[8];
    Type *type;
    struct IROVarPart *next;
    struct IROElmList *uses;
    UInt8 x26[4];
};
#pragma options align = reset
extern void IRO_ScalarizeClassDataMembers(void);
extern void RewriteBitFieldTemps(void);
extern void IroVars_CheckVariablesInitializedBeforeUse(void);
extern void fn_0044b2d0(IROLinear *p);
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
