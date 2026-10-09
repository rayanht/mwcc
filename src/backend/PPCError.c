#define CERROR_FILE "PPCError.c"
#include "compiler/common.h"
#include "version.h"
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

static void PPCError_GetErrorString(char *buffer, SInt16 code)
{
    if ((int)code < 100 || (int)code >= (VERSION > VERSION_GC_1_0 ? 178 : 177))
        CError_FATAL(40);
    COS_GetString(buffer, 10001, (int)code - 99);
}

static void PPCError_Message(SInt32 code, va_list args, int fatal, int warning)
{
    char buffer[256];

    PPCError_GetErrorString(buffer, code);
    CError_FormatAndReportDiagnostic(code + 10001, buffer, args, fatal, warning);
}

void PPCError_ReportError(SInt32 error, ...)
{
    va_list args;

    if (trychain != NULL)
        longjmp(trychain->jmpbuf, 1);
    args = (va_list)&error + (((va_list)(&error + 1) - (va_list)&error + 3) / 4 * 4);
    PPCError_Message(error, args, 0, 0);
    if (data_005884fd != 0)
        InlineAsm_LongJump();
}

void PPCError_ReportDiagnostic(SInt32 diagnosticId, ...)
{
    va_list args;

    if (trychain != NULL)
        return;
    args = (va_list)&diagnosticId + (((va_list)(&diagnosticId + 1) - (va_list)&diagnosticId) + 3) / 4 * 4;
    PPCError_Message(diagnosticId, args, 0, 1);
}

void PPCError_FatalError(short diagnostic, ...)
{
    va_list args;

    if (trychain != NULL)
        longjmp(trychain->jmpbuf, 1);
    args = (va_list)&diagnostic + (((va_list)(&diagnostic + 1) - (va_list)&diagnostic + 3) / 4 * 4);
    PPCError_Message(diagnostic, args, 1, 0);
    if (data_005884fd != 0)
        InlineAsm_LongJump();
    longjmp(error_jmp_buf, 1);
}
