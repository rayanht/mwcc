#ifndef COMPILER_IROPROPAGATE_H
#define COMPILER_IROPROPAGATE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ReplacementCandidate {
    struct IROLinear *node;
    unsigned short index;
    union {
        short varIndex;
        unsigned short uvarIndex;
    };
    struct IROLinear *value;
    struct BitVector *registers;
    struct Object *object;
    struct VarRecord *var;
    struct ReplacementCandidate *next;
    struct ReplacementCandidate *previous;
    short uses;
    struct IRONode *block;
};
#pragma options align = reset
extern void IroPropagate_PropagateExpressions(void);
extern void IRO_CopyAndConstantPropagation(void);
extern int fn_004592e0(IROLinear *node);
extern SInt32 propagationIndex;
extern struct BitVector *data_00588018;
extern struct BitVector *availableExpressions;
extern struct ReplacementCandidate *replacementCandidateTail;
extern struct ReplacementCandidate *replacementCandidate;
extern unsigned int IroPropagate_IsRegisterEligible(Object *object);

#ifdef __cplusplus
}
#endif

#endif
