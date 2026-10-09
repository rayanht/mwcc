#define CERROR_FILE "PPCError.c"
#include "compiler/common.h"
#include "compiler/PPCError.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CPrec.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/StackFrameEABI.h"
#include "driver/COSToolsCLT.h"

typedef void (*PCodeGenFn)(ENode *node, SInt32 a, SInt32 b, Operand *dst);

typedef char *va_list;

static inline void PPCError_CheckDiagnosticCode(SInt16 diagnosticId)
{
    if (diagnosticId < 100 || diagnosticId >= 178) {
        CError_FATAL(40);
    }
}

void PPCError_ReportError(SInt32 error, ...)
{
    char buffer[256];
    char *args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (trychain != NULL)
        longjmp(trychain->jmpbuf, 1);

    args = (char *)&error + (((char *)(&error + 1) - (char *)&error + 3) / 4 * 4);
    diagnosticCode = error;
    CError_ASSERT(40, (SInt16)diagnosticCode >= 100 && (SInt16)diagnosticCode < 178);

    errorCode = diagnosticCode;
    COS_GetString(buffer, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buffer, args, 0, 0);

    if (data_005884fd != 0)
        InlineAsm_LongJump();
}

void PPCError_ReportDiagnostic(SInt32 diagnosticId, ...)
{
    char buf[256];
    va_list args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (trychain != NULL) {
        return;
    }
    args = (va_list)&diagnosticId + (((va_list)(&diagnosticId + 1) - (va_list)&diagnosticId) + 3) / 4 * 4;
    diagnosticCode = diagnosticId;
    PPCError_CheckDiagnosticCode(diagnosticCode);
    errorCode = diagnosticCode;
    COS_GetString(buf, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buf, args, 0, 1);
}

void PPCError_FatalError(short diagnostic, ...)
{
    char buffer[256];
    va_list args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (trychain != NULL)
        longjmp(trychain->jmpbuf, 1);

    args = (va_list)&diagnostic + (((va_list)(&diagnostic + 1) - (va_list)&diagnostic + 3) / 4 * 4);

    diagnosticCode = diagnostic;
    CError_ASSERT(40, (SInt16)diagnosticCode >= 100 && (SInt16)diagnosticCode < 178);

    errorCode = diagnosticCode;

    COS_GetString(buffer, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buffer, args, 1, 0);

    if (data_005884fd != 0)
        InlineAsm_LongJump();
    longjmp(error_jmp_buf, 1);
}
