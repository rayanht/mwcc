#ifndef COMPILER_PPCERROR_H
#define COMPILER_PPCERROR_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

union ClassTypeHeaderBytes {
    struct {
        UInt8 lowByte;
        UInt8 highByte;
    } bytes;
    UInt16 packedWord;
};
struct ClassTypeUpdate {
    struct ClassTypeUpdate *next;
    struct PCodeInstruction *first;
    struct PCodeInstruction *second;
};
extern void PPCError_EmitClassTypeUpdate(SInt16 p1, ENode *node, SInt16 p3, Operand *res);
extern void PPCError_FatalError(short code, ...);
extern void PPCError_ReportDiagnostic(SInt32 code, ...);
extern void PPCError_ReportError(SInt32 code, ...);
extern void PPCError_UpdateClassTypeOperands(void);
extern struct ClassTypeUpdate *classTypeUpdates;

#ifdef __cplusplus
}
#endif

#endif
