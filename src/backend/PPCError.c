#define CERROR_FILE "PPCError.c"
#include "compiler/common.h"
#include "compiler/PPCError.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include <setjmp.h>

typedef void (*PCodeGenFn)(ENode *node, SInt32 a, SInt32 b, Operand *dst);

/* Declarations gathered from the merged files. */
typedef char *va_list;
void PPCError_FatalError(short diagnostic, ...)
{
    char buffer[256];
    va_list args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);

    args = (va_list)&diagnostic + (((va_list)(&diagnostic + 1) - (va_list)&diagnostic + 3) / 4 * 4);

    diagnosticCode = diagnostic;
    CError_ASSERT(40, (SInt16)diagnosticCode >= 100 && (SInt16)diagnosticCode < 178);

    errorCode = diagnosticCode;

    CompilerTools_GetResourceCString(buffer, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buffer, args, 1, 0);

    if (data_005884fd != 0)
        InlineAsm_LongJump();
    longjmp(error_jmp_buf, 1);
}

static inline void PPCError_CheckDiagnosticCode(SInt16 diagnosticId)
{
    if (diagnosticId < 100 || diagnosticId >= 178) {
        CError_FATAL(40);
    }
}

void PPCError_ReportDiagnostic(SInt32 diagnosticId, ...)
{
    char buf[256];
    va_list args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (data_00588240 != NULL) {
        return;
    }
    args = (va_list)&diagnosticId + (((va_list)(&diagnosticId + 1) - (va_list)&diagnosticId) + 3) / 4 * 4;
    diagnosticCode = diagnosticId;
    PPCError_CheckDiagnosticCode(diagnosticCode);
    errorCode = diagnosticCode;
    CompilerTools_GetResourceCString(buf, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buf, args, 0, 1);
}
void PPCError_ReportError(SInt32 error, ...)
{
    char buffer[256];
    char *args;
    SInt32 diagnosticCode;
    SInt16 errorCode;

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);

    args = (char *)&error + (((char *)(&error + 1) - (char *)&error + 3) / 4 * 4);
    diagnosticCode = error;
    CError_ASSERT(40, (SInt16)diagnosticCode >= 100 && (SInt16)diagnosticCode < 178);

    errorCode = diagnosticCode;
    CompilerTools_GetResourceCString(buffer, 10001, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticCode + 10001, buffer, args, 0, 0);

    if (data_005884fd != 0)
        InlineAsm_LongJump();
}

void PPCError_UpdateClassTypeOperands(void)
{
    ClassTypeUpdate *update;
    update = classTypeUpdates;
    if (classTypeUpdates != NULL) {
        do {
            update->first->operandData.operands[2].value.signed_value = fn_004a9f70(0) + data_00588274;
            update->second->operandData.operands[2].value.signed_value = fn_004a9f90();
            update = update->next;
        } while (update != NULL);
    }
}

void PPCError_EmitClassTypeUpdate(SInt16 requestedReg, ENode *node, SInt16 flags, Operand *result)
{
    SInt32 valueReg, typeReg;
    ClassTypeUpdate *update;
    Operand object;
    ClassTypeHeaderBytes header;

    update = (ClassTypeUpdate *)CompilerTools_AllocatePool(sizeof(*update));
    memclrw(&object, sizeof(object));
    data_00560648[node->type](node, 0, 0, &object);
    if (object.kind)
        Operands_ForceGPR(&object, node->rtype, 0);
    valueReg = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR++;
    header.bytes.lowByte = data_00588476 - 2;
    header.bytes.highByte = data_00588478;
    if (copts.nativeByteOrder != 0)
        PCodeUtilities_EmitInstruction(PC_LI, valueReg, CTool_EndianConvertWord16(header.packedWord));
    else
        PCodeUtilities_EmitInstruction(PC_LIS, valueReg, 0, CTool_EndianConvertWord16(header.packedWord));
    PCodeUtilities_EmitInstruction(PC_STW, valueReg, object.reg, 0, 0);
    valueReg = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitAddress(valueReg, 1, NULL, 0x15);
    update->first = gCurrentBlock->reverse_instructions;
    PCodeUtilities_EmitInstruction(PC_STW, valueReg, object.reg, 0, 4);
    typeReg = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitAddress(typeReg, 1, NULL, 0xb);
    update->second = gCurrentBlock->reverse_instructions;
    PCodeUtilities_EmitInstruction(PC_STW, typeReg, object.reg, 0, 8);
    result->kind = OpndType_GPR;
    result->reg = typeReg;
    update->next = classTypeUpdates;
    classTypeUpdates = update;
}
