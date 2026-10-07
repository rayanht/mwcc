#define CERROR_FILE "FuncLevelAsmPPC.c"
#include "compiler/common.h"
#include "compiler/FuncLevelAsmPPC.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CMangler.h"
#include "compiler/CParser.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "compiler/Coloring.h"
#include "compiler/DumpIR.h"
#include "compiler/InlineAsmRegisters.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeListing.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/StackFrameEABI.h"
#include "driver/cc-eabi-ppc.h"

static struct PCodeAssemblyEntry *data_00581c58;
static struct PCodeAssemblyEntry **assembly_list_tail;

static inline void append_assembly(ParsedAsmInstruction *q, PCodeBlock *block)
{
    PCodeAssemblyEntry *n = (PCodeAssemblyEntry *)lalloc(12);
    memclrw(n, 12);
    n->object = q->data.directive.object;
    n->block = block;
    *assembly_list_tail = n;
    assembly_list_tail = &n->next;
    block->flags |= 0x20;
}

void FuncLevelAsmPPC_AllocateLocals(void)
{
    Object *obj;
    Type *type;
    ObjectList *local;
    SInt32 reg;
    VarInfo *info;

    for (local = locals; local != NULL; local = local->next) {
        info = CPrep_AllocateVarInfo();
        local->object->u.var.info = info;
        local->object->flags |= 1;
        info->used = 1;
    }

    for (local = locals; local != NULL; local = local->next) {
        obj = local->object;
        type = obj->type;
        if (obj->sclass != TK_REGISTER)
            continue;
        {
            UInt8 use_gpr;
            UInt8 typecode;
            SInt32 subtype;
            if (data_00588521 && !data_005884f4)
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);

            if ((((typecode = type->type) == TYPEINT || typecode == TYPEENUM) && type->size == 8) ||
                ((use_gpr = copts.operandsDebug) && typecode == TYPEFLOAT && type->size != 4)) {
                if (gAvailableSavedGPRs < 2)
                    CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);
                Registers_AllocateGPRPair(obj);
                if (Registers_GetInfo(obj))
                    reg = Registers_GetInfo(obj)->reg;
                else
                    reg = 0;
                if (Registers_GetInfo(obj))
                    Registers_GetInfo(obj);
                CTemplateNew_InsertRegisterBinding(obj->name->name, 0, reg, obj);
            } else if (typecode == TYPEINT || typecode == TYPEENUM || typecode == TYPEPOINTER ||
                       (typecode == TYPEMEMBERPOINTER && type->size == 4) ||
                       (use_gpr && typecode == TYPEFLOAT && type->size == 4)) {
                if (gAvailableSavedGPRs == 0)
                    CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);
                Registers_AllocateGPR(obj);
                if (Registers_GetInfo(obj))
                    reg = Registers_GetInfo(obj)->reg;
                else
                    reg = 0;
                CTemplateNew_InsertRegisterBinding(obj->name->name, 0, reg, obj);
            } else if (typecode == TYPEFLOAT) {
                if (gAvailableSavedFPRs == 0)
                    CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);
                Registers_AllocateFPR(obj);
                if (Registers_GetInfo(obj))
                    reg = Registers_GetInfo(obj)->reg;
                else
                    reg = 0;
                CTemplateNew_InsertRegisterBinding(obj->name->name, 1, reg, obj);
            } else if (typecode == TYPESTRUCT && (subtype = TYPE_STRUCT(type)->stype) >= STRUCT_VECTOR_UCHAR &&
                       subtype <= STRUCT_VECTOR_PIXEL) {
                if (gAvailableSavedVRs == 0)
                    CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);
                Registers_AllocateVR(obj);
                if (Registers_GetInfo(obj))
                    reg = Registers_GetInfo(obj)->reg;
                else
                    reg = 0;
                CTemplateNew_InsertRegisterBinding(obj->name->name, 9, reg, obj);
            } else {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, obj->name->name);
            }
        }
    }

    for (local = locals; local != NULL; local = local->next) {
        obj = local->object;
        if (Registers_GetInfo(obj))
            reg = Registers_GetInfo(obj)->reg;
        else
            reg = 0;
        if (reg == 0)
            StackFrameEABI_AllocateObjectSlot(obj);
    }
}

void fn_004e6e30(void)
{
    unsigned char savedFlag;
    int parseResult;
    data_005884fd = 1;
    savedFlag = data_0058850d;
    data_0058850d = 1;
    parseResult = _Setjmp(inlineAsmJmpBuf);
    if (parseResult == 0) {
        while (tk == TK_IDENTIFIER && (parseResult = InlineAsmPPC_ClassifyIdentifier('\x01')) != 0) {
            InlineAsmPPC_ParseDirective(parseResult);
            if ((tk == ';') || (tk == TK_EOL)) {
                CPrep_ResetBufferedTokenPosition();
                tk = CPrepTokenizer_GetNextToken();
            } else {
                InlineAsm_Error(0x71);
            }
            if (parseResult == 2)
                break;
            if (parseResult == 3) {
                data_005884f4 = 1;
                break;
            }
        }
    }
    data_005884fd = 0;
    data_0058850d = savedFlag;
}

void FuncLevelAsmPPC_GenerateFunction(Object *func)
{
    Statement *list;
    Statement *node;
    union {
        ParsedAsmInstruction *assembly;
        PCodeLabel *label;
    } item;
    const char *functionName;
    UInt8 frameFinalized;
    UInt8 savedOptionC2;
    PCodeBlock *frameBlock;
    UInt8 savedOptionC3;

    frameFinalized = 0;

    if (copts.littleendian) {
        high_word_offset = 4;
        low_word_offset = 0;
        returnRegHi = 4;
        return_gpr_first = 3;
        sfpe_right_operand_reg_hi = 6;
        sfpe_right_operand_reg = 5;
    } else {
        high_word_offset = 0;
        low_word_offset = 4;
        returnRegHi = 3;
        return_gpr_first = 4;
        sfpe_right_operand_reg_hi = 5;
        sfpe_right_operand_reg = 6;
    }

    gStackFrameSize = 0;
    data_005882c0.record = NULL;
    data_005884f4 = 0;
    asm_instruction_count = 0;
    data_00581c58 = NULL;
    assembly_list_tail = &data_00581c58;
    list = curstmt;
    has_dlocal_initialization = 0;
    gHasAltivecFrame = 0;
    data_005884ff = 0;

    if (func != NULL && func->name != NULL)
        cc_eabi_ppc_ReportCompilingFunction(func->name->name);

    if (func->qual & Q_INLINE)
        PPCError_ReportDiagnostic(0xad);

    gStackFrameSize += StackFrameEABI_GetRecordSize(func);

    CheckCLabels();
    if (func_errors != 0)
        return;

    if (copts.filesyminfo != 0) {
        fn_0043f1f0(&function_fileinfo);
        ObjGen_PPC_EABI_SetObjectSectionIndex(func);
    }

    function_header_index = ObjGen_PPC_EABI_SetupFunctionSection(func);

    PCode_ResetBlocks();

    PCode_ResolveLabel((prologueBlock = PCode_CreateBlock()), (PCode_NewLabel()));

    PCode_ResolveLabel((frameBlock = PCode_CreateBlock()), (PCode_NewLabel()));

    PCode_AddSuccessor(prologueBlock, frameBlock->labels);
    Operands_ClearTrailingObjectInfo();

    data_00588521 = 1;
    outgoing_argument_size = data_005871a0 = 0;
    InlineAsmPPC_Initialize();
    fn_004e6e30();
    data_0058852d = 0;
    StackFrameEABI_Initialize();
    data_00588224 = 1;
    Registers_InitRegisterState();
    Registers_SetupStackBaseReg();
    CodeGen_AllocateArgumentSlots(func, 0, 0);

    data_0058850d = 1;
    data_005884fd = 1;
    savedOptionC2 = copts.warn_unusedvar;
    savedOptionC3 = copts.warn_unusedarg;
    copts.warn_unusedvar = 0;
    copts.warn_unusedarg = 0;
    InlineAsm_ParseAsmLines(0x7d);

    if (anyerrors == 0 && copts.debug_listing != 0)
        fn_004be830(list, func);

    data_005884fd = 0;
    data_0058850d = 0;
    functionName = COptimizer_GetFunctionObject(func)->name;
    func->flags |= OBJECT_DEFINED;
    if (data_005871a0 != 0)
        outgoing_argument_size = data_005871a0;
    if (data_005884f4 == 0)
        CodeGen_EnumerateArgumentRegisters(emit_dlocal_initialization);
    PCodeUtilities_ResolveLabel(PCode_NewLabel());
    CodeGen_AssignMissingEntryValues(list->next);
    copts.warn_unusedvar = savedOptionC2;
    copts.warn_unusedarg = savedOptionC3;

    for (node = list->next; node != NULL; node = node->next) {
        switch (node->type) {
            case ST_ASM:
                item.assembly = (ParsedAsmInstruction *)node->expr;
                if (item.assembly != NULL) {
                    if ((item.assembly->specialFlags & 1) != 0) {
                        if (item.assembly->opcode == 1) {
                            PCodeUtilities_ResolveLabel(PCode_NewLabel());
                            gCurrentBlock->line = node->sourceoffset;
                            append_assembly((ParsedAsmInstruction *)node->expr, gCurrentBlock);
                        } else if (item.assembly->opcode == 4) {
                            PCodeUtilities_ResolveLabel(PCode_NewLabel());
                            (gReturnBlock = gCurrentBlock)->flags |= 2;
                            CheckCLabels();
                            if (func_errors != 0)
                                return;
                            PCode_BuildPredecessors();
                            if (copts.debug_listing != 0)
                                fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] INITIAL CODE");
                            Coloring_AllocateRegisters(func);
                            if (copts.debug_listing != 0)
                                fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] AFTER REGISTER COLORING");
                            StackFrameEABI_ClearUnusedStackFrame();
                            StackFrameEABI_FinalizeLayout(frameBlock);
                            StackFrameEABI_GeneratePrologueEpilogue(prologueBlock, 0, has_dlocal_initialization);
                            StackFrameEABI_MergePrologueEpilogue(gReturnBlock = gCurrentBlock,
                                                                 data_005882c0.record ? 1 : 0);
                            if (copts.debug_listing != 0)
                                fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] AFTER PROLOGUE/EPILOGUE CREATION");
                            frameFinalized = 1;
                        }
                    } else {
                        PCodeUtilities_ResolveLabel(PCode_NewLabel());
                        gCurrentBlock->line = node->sourceoffset;
                        InlineAsmPPC_GenerateAsmInstruction(node);
                    }
                }
                break;

            case ST_LABEL:
                if ((item.label = node->label->pclabel)->resolved == 0)
                    PCodeUtilities_ResolveLabel(item.label);
                break;

            default:
                CError_FATAL(597);
        }
    }

    if (func_errors != 0)
        return;

    CheckCLabels();
    if (func_errors != 0)
        return;

    if (frameFinalized == 0) {
        PCodeUtilities_ResolveLabel(PCode_NewLabel());
        (gReturnBlock = gCurrentBlock)->flags |= 2;
        PCode_BuildPredecessors();
        if (copts.debug_listing != 0)
            fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] INITIAL CODE");
        if (data_005884f4 == 0) {
            Coloring_AllocateRegisters(func);
            if (func_errors != 0)
                return;
            if (copts.debug_listing != 0)
                fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] AFTER REGISTER COLORING");
        }
        StackFrameEABI_ClearUnusedStackFrame();
        StackFrameEABI_FinalizeLayout(frameBlock);
        if (data_005884f4 != 0)
            stack_frame_size = 0;
        if (func_errors != 0)
            return;
        if (data_005884f4 == 0) {
            StackFrameEABI_GeneratePrologueEpilogue(prologueBlock, 0, has_dlocal_initialization);
            StackFrameEABI_MergePrologueEpilogue(gReturnBlock, 1);
        }
        if (copts.debug_listing != 0)
            fn_004c4bd0(functionName, "[FUNCTION-LEVEL ASM] AFTER PROLOGUE/EPILOGUE CREATION");
    }

    if (func_errors != 0)
        return;

    func->section = function_header_index;
    if (copts.filesyminfo != 0)
        function_token_line = CPrep_UpdateTokenLine(&function_fileinfo);
    copts.peephole = 0;
    PCodeAssembly_EmitFunction(func, data_00581c58);
    if (copts.debug_listing != 0)
        fn_004c4bd0(COptimizer_GetFunctionObject(func)->name, "[FUNCTION-LEVEL ASM] FINAL CODE");
    CFunc_WarnUnused();
}
