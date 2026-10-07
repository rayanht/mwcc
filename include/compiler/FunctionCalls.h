#ifndef COMPILER_FUNCTIONCALLS_H
#define COMPILER_FUNCTIONCALLS_H

#include "compiler/common.h"
#include "compiler/InstrSelection.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct ArgumentContext {
    struct ArgumentContext *next;
    struct ENode *node;
    Operand operand;
    int stack_offset;
    short gpr;
    short second_gpr;
    short fpr;
    short vectorRegister;
    short evaluated;
    short flags;
};
#pragma pack(pop)
extern ArgumentContext *assign_argument_locations(ENode *thisArg, ENodeList *args, FuncArg *type, UInt32 *gprMask,
                                                  UInt32 *fprMask, Boolean *hasFloatArgs, UInt32 *vectorMask,
                                                  Boolean *hasVectorArgs, Boolean hasSpecialArgument);
extern void FunctionCalls_PushObjectReferenceEntry(TemplStack *entry, TypeClass *tmclass, Object *object);
extern void evaluate_hascall_arguments_reverse(ArgumentContext *x);
extern void store_argument_on_stack(ArgumentContext *arg);
extern void FunctionCalls_GenerateCall(ENode *item, Operand *result);
extern void load_argument_registers(ArgumentContext *argument);

#ifdef __cplusplus
}
#endif

#endif
