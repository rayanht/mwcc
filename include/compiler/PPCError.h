#ifndef COMPILER_PPCERROR_H
#define COMPILER_PPCERROR_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void PPCError_FatalError(short code, ...);
extern void PPCError_ReportDiagnostic(SInt32 code, ...);
extern void PPCError_ReportError(SInt32 code, ...);

#ifdef __cplusplus
}
#endif

#endif
