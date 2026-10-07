#ifndef COMPILER_IROJUMP_H
#define COMPILER_IROJUMP_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ERange {
    UInt8 type;
    CInt64 upper;
    CInt64 lower;
};
#pragma options align = reset
extern SInt32 IRO_DoJumpChaining(void);
extern SInt32 chain_label(CLabel **label);
extern SInt32 IRO_RangePropagateInFNode(void);
extern int initialize_linear_range(IROLinear *nd);
extern int IRO_RemoveLabels(void);
extern void IroJump_ConvertCInt64ToType(CInt64 *value, Type *type);
extern int IRO_RemoveRedundantJumps(void);
extern SInt32 IRO_RemoveUnreachable(void);
extern void IroJump_MarkReachable(IRONode *param);
extern struct ERangeVar *range_vars;

#ifdef __cplusplus
}
#endif

#endif
