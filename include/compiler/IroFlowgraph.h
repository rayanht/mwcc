#ifndef COMPILER_IROFLOWGRAPH_H
#define COMPILER_IROFLOWGRAPH_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void IRO_BuildflowGraph(IROLinear *source);
extern void IroFlowgraph_ComputeDom(void);
extern void IroFlowgraph_RebuildSuccPred(void);
extern void fn_0044a640(IROLinear *value);
extern struct IRONode *IroFlowgraph_NewNode(void);
extern struct IRONode *iroNodeTail;

#ifdef __cplusplus
}
#endif

#endif
