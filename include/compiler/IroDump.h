#ifndef COMPILER_IRODUMP_H
#define COMPILER_IRODUMP_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void IroDump_DumpFunction(char *value, int enabled);
extern Object *IroDump_GetObjRef(IROLinear *linear);
extern void IroDump_Print(const char *message, ...);
extern void IroDump_DumpExpressions(void);
extern void dump_flowgraph(void);
extern void IroDump_PrintBitSet(char *prefix, BitVector *bitset);
extern int INT_005882b8;
extern Object *data_005875b8;
extern void dump_linear_node(IROLinear *node);
extern unsigned int IroDump_IsType1NodeType50(IROLinear *linear);
extern SInt32 IroDump_IsPowerOfTwo(IROLinear *node, SInt32 *bit);
extern int fn_0044d520(IROLinear *node);

#ifdef __cplusplus
}
#endif

#endif
