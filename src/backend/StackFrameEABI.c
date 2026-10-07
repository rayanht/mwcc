#define CERROR_FILE "StackFrameEABI.c"
#include "compiler/common.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>
#include <stdio.h>

static Object *data_00580fa8;

/* Declarations gathered from the merged files. */

enum { STRUCT_KIND_4 = 4, STRUCT_KIND_14 = 14 };
typedef enum { kMergeTag = 0x1f } MergeTag;

void fn_004a9c70(void)
{
    return;
}

static inline void StackFrame_EmitAltivecStackPointerSave(Boolean restore, SInt16 *savedStackReg)
{
    if (data_00588521 == 0 || copts.altivecVrsave == 0)
        return;

    if (restore) {
        PCodeUtilities_EmitInstruction(PC_STW, 1, *savedStackReg, 0, 0);
        Registers_BindGPR(NULL, *savedStackReg);
    } else {
        if (fn_004c15f0() == 0)
            *savedStackReg = 12;
        else
            *savedStackReg = Coloring_ClaimGPRColor();
        PCodeUtilities_EmitInstruction(PC_LWZ, *savedStackReg, 1, 0, 0);
    }
}

void StackFrame_CheckAltivec(void)
{
    PCodeBlock *savedBlock;
    int frameReg;
    SInt16 savedStackReg;
    Boolean argumentInParamArea;

    if (copts.altivecModel == 0)
        return;
    if (Registers_AreNonvolatileVRUsed()) {
        gHasAltivecFrame = 1;
        data_005884ff = 1;
    }
    {
        ObjectList *localEntry;
        Object *object;
        for (localEntry = locals; localEntry != NULL; localEntry = localEntry->next) {
            object = localEntry->object.value;
            if (StackFrameEABI_GetTypeAlignment(object->type) >= 16) {
                gHasAltivecFrame = 1;
                if ((Registers_GetInfo(object) ? Registers_GetInfo(object)->reg : 0) == 0) {
                    data_005884ff = 1;
                    data_00588521 = 0;
                } else if (Registers_GetInfo(object)->is_vector > 0) {
                    if ((Registers_GetInfo(object) ? Registers_GetInfo(object)->reg : 0) < 32 &&
                        (Registers_GetInfo(object) ? Registers_GetInfo(object)->reg : 0) >= 20) {
                        data_005884ff = 1;
                        data_00588521 = 0;
                    }
                }
                break;
            }
        }
    }
    argumentInParamArea = 0;
    {
        Object *object;
        ObjectList *argument;
        for (argument = arguments; argument != NULL; argument = argument->next) {
            object = argument->object.value;
            if (StackFrameEABI_GetTypeAlignment(object->type) >= 16) {
                gHasAltivecFrame = 1;
                if (object->u.var.info->reg == 0) {
                    if (Registers_GetInfo(object)->in_param_area != 0) {
                        argumentInParamArea = 1;
                    } else {
                        data_005884ff = 1;
                        data_00588521 = 0;
                    }
                    break;
                } else if (Registers_GetInfo(object)->is_vector != 0) {
                    if ((Registers_GetInfo(object) ? Registers_GetInfo(object)->reg : 0) < 32 &&
                        (Registers_GetInfo(object) ? Registers_GetInfo(object)->reg : 0) >= 20) {
                        data_00588521 = 0;
                    }
                }
                break;
            }
        }
    }
    if (argumentInParamArea != 0)
        data_005884ff = 0;
    if (data_005884ff != 0)
        data_00588521 = 0;
    if (gHasAltivecFrame) {
        if (data_00588521 != 0) {
            frameReg = 1;
        } else {
            frameReg = Coloring_ClaimGPRColor();
            if (frameReg == -1) {
                frameReg = 11;
                PPCError_ReportError(0x74);
            }
            Registers_BindGPR(NULL, frameReg);
        }
        savedBlock = gCurrentBlock;
        gCurrentBlock = prologueBlock;
        if (data_00588521 == 0)
            PCodeUtilities_EmitInstruction(PC_LWZ, frameReg, 1, 0, 0);
        StackFrame_EmitAltivecStackPointerSave(0, &savedStackReg);
        gCurrentBlock = gReturnBlock;
        if (data_00588521 == 0)
            PCodeUtilities_EmitInstruction(PC_STW, 1, frameReg, 0, 0);
        StackFrame_EmitAltivecStackPointerSave(1, &savedStackReg);
        gCurrentBlock = savedBlock;
    }
}

int fn_004a9f70(int value)
{
    if (value != 0) {
        return 8;
    }
    return stack_frame_size + 8;
}

unsigned int fn_004a9f90(void)
{
    return data_005880cc + outgoing_argument_size;
}

void emit_restore_special_registers(SInt16 frameRegister)
{
    int offset;

    special_register_save_offset -= 4;
    if (data_005882c0.record->enable) {
        PCodeUtilities_EmitInstruction(PC_MFMSR, 0);
        PCodeUtilities_EmitInstruction(PC_ANDI, 0, 0, 0x8002);
        PCodeUtilities_EmitInstruction(PC_MTMSR, 0);
    }
    if (data_005882c0.record->DSISR) {
        emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
        PCodeUtilities_EmitInstruction(PC_MTSPR, 0x12, 0);
        special_register_save_offset -= 4;
    }
    if (data_005882c0.record->DAR) {
        emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
        PCodeUtilities_EmitInstruction(PC_MTSPR, 0x13, 0);
        special_register_save_offset -= 4;
    }
    if (data_005882c0.record->SRR) {
        emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
        PCodeUtilities_EmitInstruction(PC_MTSPR, 0x1b, 0);
        special_register_save_offset -= 4;
        emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
        PCodeUtilities_EmitInstruction(PC_MTSPR, 0x1a, 0);
        special_register_save_offset -= 4;
    }
    offset = emit_lwz_register_restores(frameRegister);
    emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
    PCodeUtilities_EmitInstruction(PC_MTLR, 0);
    special_register_save_offset -= 4;
    emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
    PCodeUtilities_EmitInstruction(PC_MTCRF, 0xff, 0);
    special_register_save_offset -= 4;
    emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
    PCodeUtilities_EmitInstruction(PC_MTXER, 0);
    special_register_save_offset -= 4;
    emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
    PCodeUtilities_EmitInstruction(PC_MTCTR, 0);
    special_register_save_offset -= 4;
    emit_opcode_with_base_offset(PC_LWZ, 0, frameRegister, NULL, special_register_save_offset);
    if (data_0058852d) {
        if (offset == 0 || (SInt16)frameRegister != 0xc)
            CError_FATAL(2276);
        emit_opcode_with_base_offset(PC_LWZ, frameRegister, frameRegister, NULL, offset);
    }
}

/* Stack-frame flags following two words of frame metadata. */

static inline void SaveSpecialRegister(void)
{
    PCodeUtilities_EmitInstruction(49U, 0U, 1U, 0U, special_register_save_offset);
    special_register_save_offset += 4U;
}

static inline InterruptGenerationRecord *GetStackFrameFlags(void)
{
    return data_005882c0.record;
}

static inline void BeginSpecialRegisterSaves(unsigned int offset)
{
    special_register_save_offset = offset;
    PCodeUtilities_EmitInstruction(49U, 0U, 1U, 0U, offset);
    special_register_save_offset += 4U;
}

void generate_interrupt_register_saves(void)
{
    unsigned int offset;
    unsigned int frameOffset;
    int registerIndex;
    InterruptGenerationRecord *flags;

    offset = data_005880cc;
    offset += outgoing_argument_size;
    BeginSpecialRegisterSaves(offset);
    PCodeUtilities_EmitInstruction(128U, 0U);
    SaveSpecialRegister();
    PCodeUtilities_EmitInstruction(127U, 0U);
    SaveSpecialRegister();
    PCodeUtilities_EmitInstruction(130U, 0U);
    SaveSpecialRegister();
    PCodeUtilities_EmitInstruction(129U, 0U);
    SaveSpecialRegister();

    offset = data_0058764c;
    offset += data_005880d8;
    frameOffset = stack_frame_size;
    offset += data_00588070;
    offset += data_00587638;
    offset += frame_alignment_padding;
    offset += data_00587634;
    offset += stack_frame_adjustment;
    offset += stack_frame_padding;
    offset += data_005876a8;
    frameOffset -= offset;
    if (!data_00588521) {
        for (registerIndex = 0; registerIndex < 10; ++registerIndex) {
            PCodeUtilities_EmitInstruction(49U, registerIndex + 3, 1U, 0U, frameOffset);
            frameOffset += 4U;
        }
    } else {
        for (registerIndex = 0; registerIndex < 10; ++registerIndex) {
            if ((1U << (registerIndex + 3)) & interrupt_register_save_mask)
                PCodeUtilities_EmitInstruction(49U, registerIndex + 3, 1U, 0U, frameOffset);
            frameOffset += 4U;
        }
    }

    flags = GetStackFrameFlags();
    if (flags->SRR) {
        PCodeUtilities_EmitInstruction(126U, 0U, 26U);
        SaveSpecialRegister();
        PCodeUtilities_EmitInstruction(126U, 0U, 27U);
        SaveSpecialRegister();
    }
    flags = GetStackFrameFlags();
    if (flags->DAR) {
        PCodeUtilities_EmitInstruction(126U, 0U, 19U);
        SaveSpecialRegister();
    }
    flags = GetStackFrameFlags();
    if (flags->DSISR) {
        PCodeUtilities_EmitInstruction(126U, 0U, 18U);
        SaveSpecialRegister();
    }
    flags = GetStackFrameFlags();
    if (flags->enable) {
        PCodeUtilities_EmitInstruction(125U, 0U);
        PCodeUtilities_EmitInstruction(88U, 0U, 0U, 0U, 32770U);
        PCodeUtilities_EmitInstruction(123U, 0U);
    }
}

SInt32 StackFrameEABI_GetRecordSize(Object *object)
{
    SInt32 size = 0;
    if (object->qual & Q_INTERRUPT) {
        ObjGen_PPC_EABI_SetObjectSection(object, 0, 0);
        data_005882c0.record = ObjGen_PPC_EABI_GetInterruptInfo(object);
        size += 20;
        if (data_005882c0.record->SRR != 0U) {
            size += 8;
        }
        if (data_005882c0.record->DAR != 0U) {
            size += 4;
        }
        if (data_005882c0.record->DSISR != 0U) {
            size += 4;
        }
    }
    return size;
}

void StackFrameEABI_SaveArgumentRegisters(PCodeBlock *entryBlock, PCodeBlock *fpSaveBlock, PCodeBlock *gpSaveBlock,
                                          PCodeLabel *operand)
{
    PCodeInstruction *instruction;
    short offsetBase;
    short offset;
    short fpRegister;
    short gpRegister;
    if (classTypeUpdates != NULL)
        PPCError_UpdateClassTypeOperands();
    if (fpSaveBlock != NULL) {
        instruction = PCodeUtilities_CreateInstruction(8, 1, 2, operand);
        PCode_AppendInstruction(entryBlock, instruction);
        offsetBase = data_005880cc;
        offsetBase += outgoing_argument_size;
        offset = offsetBase + 32;
        for (fpRegister = 1; fpRegister <= 8; fpRegister++) {
            instruction = PCodeUtilities_CreateInstruction(154, fpRegister, 1, 0, (fpRegister - 1) * 8 + offset);
            PCode_AppendInstruction(fpSaveBlock, instruction);
        }
        fpSaveBlock->flags |= 1;
    }
    for (gpRegister = 3; gpRegister <= 10; gpRegister++) {
        instruction = PCodeUtilities_CreateInstruction(49, gpRegister, 1, 0,
                                                       (gpRegister - 3) * 4 + (data_005880cc + outgoing_argument_size));
        PCode_AppendInstruction(gpSaveBlock, instruction);
    }
    gpSaveBlock->flags |= 1;
}

void fn_004aa580(void)
{
    gStackFrameSize += 96U;
    return;
}

void emit_stack_frame_allocation(int frame_register)
{
    if (gHasAltivecFrame) {
        if (!data_00588521) {
            PCodeUtilities_EmitInstruction(PC_MR, frame_register, 1);
            Operands_AllocateGPR(0x20000);
        }
        if (data_005884ff) {
            PCodeUtilities_EmitInstruction(PC_RLWINM, 11, 1, 0, 28, 31);
            PCodeUtilities_EmitInstruction(PC_SUBFIC, 11, 11,
                                           -(stack_frame_size ? stack_frame_size : eabi_stack_frame_size));
            PCodeUtilities_EmitInstruction(PC_STWUX, 1, 1, 11);
        } else {
            PCodeUtilities_EmitInstruction(PC_STWU, 1, 1, 0, -stack_frame_size);
        }
    } else if (stack_frame_size > 0x7fff) {
        SInt32 displacement = -stack_frame_size;
        SInt16 immediate = (displacement >> 16) + ((displacement >> 15) & 1);
        PCodeUtilities_EmitInstruction(PC_LIS, 11, 0, immediate);
        if ((SInt16)-stack_frame_size)
            PCodeUtilities_EmitInstruction(PC_ADDI, 11, 11, 0, (SInt16)-stack_frame_size);
        PCodeUtilities_EmitInstruction(PC_STWUX, 1, 1, 11);
    } else {
        PCodeUtilities_EmitInstruction(PC_STWU, 1, 1, 0, -stack_frame_size);
    }
}

void save_and_update_vrsave(int a, int argb)
{
    if (gHasAltivecFrame && vrsave_mask && copts.altivecVrsave) {
        SInt32 frame = -(frame_alignment_padding + data_00587638 + stack_frame_adjustment + data_00587634 +
                         data_00588070 + data_0058764c + data_005880d8);
        UInt16 hi = vrsave_mask >> 16;
        UInt16 lo = vrsave_mask;
        int b = argb;
        if (b == -1) {
            PCodeUtilities_EmitInstruction(PC_MFSPR, 0, 0x100);
            PCodeUtilities_EmitInstruction(PC_STW, 0, a, 0, frame);
        } else {
            PCodeUtilities_EmitInstruction(PC_MFSPR, b, 0x100);
        }
        if (hi != 0) {
            if (b == -1) {
                PCodeUtilities_EmitInstruction(PC_ORIS, 0, 0, hi);
            } else {
                PCodeUtilities_EmitInstruction(PC_ORIS, 0, b, hi);
            }
        } else if (b != -1) {
            PCodeUtilities_EmitInstruction(PC_MR, 0, b);
        }
        if (lo != 0) {
            PCodeUtilities_EmitInstruction(PC_ORI, 0, 0, lo);
        }
        PCodeUtilities_EmitInstruction(PC_MTSPR, 0x100, 0);
    }
}

void StackFrameEABI_004aa7d0(void)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;
    SInt32 operandIndex;

    if (data_00588521 == 0) {
        data_00588204 = 10;
        return;
    }
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            for (operandIndex = 0; operandIndex < instruction->operand_count; operandIndex++) {
                if (instruction->operandData.operands[operandIndex].kind == PCOp_GPR &&
                    ((SInt8)instruction->operandData.operands[operandIndex].flags & 2) != 0) {
                    interrupt_register_save_mask |= 1 << instruction->operandData.operands[operandIndex].value.reg;
                }
            }
        }
    }
    if (data_0058852d != 0) {
        interrupt_register_save_mask |= 0x1000;
    }
    operandIndex = 3;
    do {
        if ((interrupt_register_save_mask & (1 << operandIndex)) != 0) {
            data_00588204++;
        }
        operandIndex++;
    } while (operandIndex < 13);
}

void StackFrameEABI_ClearUnusedStackFrame(void)
{
    PCodeBlock *p;
    PCodeInstruction *s;
    int i;

    if (data_005882c0.record != NULL) {
        StackFrameEABI_004aa7d0();
        return;
    }
    if (outgoing_argument_size != 0 || gGPRSaveSpan != 0 || gFPRSaveSpan != 0 || gVRSaveSpan != 0)
        return;
    if (gStackFrameSize == 0)
        return;

    for (p = gPCodeBlocks; p != NULL; p = p->next) {
        for (s = (PCodeInstruction *)p->instructions; s != NULL; s = s->next) {
            for (i = 0; i < s->operand_count; i++) {
                if (s->operandData.operands[i].kind == PCOp_GPR && s->operandData.operands[i].value.reg == 1)
                    return;
            }
        }
    }
    gStackFrameSize = 0;
}

int emit_lwz_register_restores(short base_register)
{
    int register_index;
    int base_register_save_offset;
    int restore_area_offset;
    int interrupt_register_index;

    base_register_save_offset = 0;
    restore_area_offset =
        stack_frame_size - (data_0058764c + data_005880d8 + data_00588070 + data_00587638 + frame_alignment_padding +
                            data_00587634 + stack_frame_adjustment + stack_frame_padding + data_005876a8);
    if (data_00588521 == 0) {
        register_index = 0;
        do {
            if (data_0058852d != 0 && register_index + 3 == base_register) {
                base_register_save_offset = restore_area_offset + register_index * 4;
            } else {
                emit_opcode_with_base_offset(PC_LWZ, register_index + 3, base_register, NULL,
                                             restore_area_offset + register_index * 4);
            }
            register_index = register_index + 1;
        } while (register_index < 10);
    } else {
        interrupt_register_index = 0;
        do {
            if (data_0058852d != 0 && interrupt_register_index + 3 == base_register) {
                base_register_save_offset = restore_area_offset + interrupt_register_index * 4;
            } else if (((1 << (interrupt_register_index + 3)) & interrupt_register_save_mask) != 0) {
                emit_opcode_with_base_offset(PC_LWZ, interrupt_register_index + 3, base_register, NULL,
                                             restore_area_offset + interrupt_register_index * 4);
            }
            interrupt_register_index = interrupt_register_index + 1;
        } while (interrupt_register_index < 10);
    }
    return base_register_save_offset;
}

SInt32 StackFrameEABI_GetTypeAlignment(Type *type)
{
    SInt32 alignment = 1;
    SInt32 aggregateAlignment;
    ClassList *base;
    ObjMemberVar *ivar;
    SInt32 memberAlignment;
    StructMember *member;

    while (1) {
        SInt8 kind = type->type;
        switch (kind) {
            case TYPEENUM:
                type = TYPE_ENUM(type)->enumtype;
                /* fall through */
            case TYPEINT:
                if (((TypeStruct *)type)->size > alignment)
                    alignment = type->size;
                return alignment;

            case TYPEFLOAT:
                return type->size == 4 ? 4 : 8;

            case TYPESTRUCT:
                if (TYPE_STRUCT(type)->stype >= STRUCT_KIND_4 && TYPE_STRUCT(type)->stype <= STRUCT_KIND_14)
                    return 16;
                aggregateAlignment = TYPE_STRUCT(type)->align;
                memberAlignment = aggregateAlignment > 4 ? aggregateAlignment : 4;
                member = TYPE_STRUCT(type)->members;
                if (member == NULL)
                    return aggregateAlignment > memberAlignment ? aggregateAlignment : memberAlignment;
                for (member = TYPE_STRUCT(type)->members; member != NULL; member = member->next) {
                    SInt32 requiredAlignment = StackFrameEABI_GetTypeAlignment(member->type);
                    if (requiredAlignment > memberAlignment)
                        memberAlignment = requiredAlignment;
                }
                return memberAlignment;

            case TYPECLASS:
                aggregateAlignment = TYPE_CLASS(type)->align;
                aggregateAlignment = aggregateAlignment > 4 ? aggregateAlignment : 4;
                for (base = TYPE_CLASS(type)->bases; base != NULL; base = base->next) {
                    SInt32 requiredAlignment = StackFrameEABI_GetTypeAlignment((Type *)base->base);
                    if (requiredAlignment > aggregateAlignment)
                        aggregateAlignment = requiredAlignment;
                }
                for (ivar = TYPE_CLASS(type)->ivars; ivar != NULL; ivar = ivar->next) {
                    SInt32 requiredAlignment = StackFrameEABI_GetTypeAlignment(ivar->type);
                    if (requiredAlignment > aggregateAlignment)
                        aggregateAlignment = requiredAlignment;
                }
                return aggregateAlignment;

            case TYPEBITFIELD:
                type = TYPE_BITFIELD(type)->bitfieldtype;
                break;

            case TYPEPOINTER:
            case TYPEMEMBERPOINTER:
                return 4;

            case TYPEARRAY:
                type = TPTR_TARGET(type);
                if (type->size < 4)
                    alignment = 4;
                break;

            case TYPEVOID:
                return 4;

            default:
                CError_FATAL(1753);
                return 1;
        }
    }
}

static inline SInt32 StackFrameEABI_VectorTypeKind(TypeStruct *type)
{
    return type->stype;
}

void *StackFrameEABI_004aabb0(UInt32 codeOffset, char *name, SInt32 *outSize, Object *function)
{
    SInt16 nameLength;
    SInt32 size;
    TB *traceback;
    char *cursor;
    FuncArg *argument, *arguments;
    int isVariadic;
    UInt8 vectorArgumentCount;

    nameLength = strlen(name);
    size = (nameLength + 21 + (data_0058852d ? 1 : 0) + (gHasAltivecFrame ? 2 : 0)) & ~3;
    traceback = (TB *)CompilerTools_AllocatePool(size);
    memclrw(traceback, size);
    traceback->version = 0;
    traceback->lang = copts.cplusplus ? 9 : 0;
    traceback->a2 = 1;
    traceback->b1 = 1;
    if (data_0058852d)
        traceback->b2 = 1;
    if (data_005883ee)
        traceback->b6 = 1;
    if (!data_00588521 || gFPRSaveSpan > 3)
        traceback->b7 = 1;
    if (stack_frame_size)
        traceback->c0 = 1;
    traceback->c2 = gFPRSaveSpan;
    traceback->d2 = gGPRSaveSpan;
    traceback->c1 = 0;
    traceback->d0 = 0;
    traceback->d1 = gHasAltivecFrame ? 1 : 0;
    cursor = (char *)traceback + sizeof(TB);
    *(UInt32 *)cursor = CTool_EndianConvertWord32(codeOffset);
    cursor += sizeof(UInt32);
    *(UInt16 *)cursor = CTool_EndianConvertWord16(nameLength);
    cursor += sizeof(UInt16);
    strcpy(cursor, name);
    cursor += nameLength;
    if (data_0058852d) {
        *cursor = 31;
        cursor++;
    }
    if (gHasAltivecFrame) {
        VX *extension = (VX *)cursor;
        vectorArgumentCount = 0;
        argument = TYPE_FUNC(function->type)->args;
        arguments = argument;
        while (argument && argument != &data_00583098)
            argument = argument->next;
        isVariadic = argument == &data_00583098;
        for (argument = arguments; argument; argument = argument->next) {
            TypeStruct *type = TYPE_STRUCT(argument->type);
            if (type && type->type == TYPESTRUCT && StackFrameEABI_VectorTypeKind(type) >= 4 &&
                StackFrameEABI_VectorTypeKind(type) <= 14)
                vectorArgumentCount++;
        }
        extension->regs = gVRSaveSpan;
        extension->opt = copts.altivecVrsave;
        extension->var = isVariadic;
        extension->count = vectorArgumentCount;
        extension->other = vrsave_mask ? 1 : 0;
    }
    *outSize = size;
    return traceback;
}

void StackFrameEABI_EmitFrameAllocation(char allocateFrame, short scratchReg, int frameSize)
{
    short savedStackReg;
    int negativeSize;
    short highSize;
    savedStackReg = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR += 1;
    emit_opcode_with_base_offset(PC_LWZ, savedStackReg, 1, NULL, 0);
    if (allocateFrame != 0) {
        frameSize = (frameSize + 15) & -16;
        if (frameSize < 32768) {
            PCodeUtilities_ResolveLabel(PCode_NewLabel());
            PCodeUtilities_EmitInstruction(PC_STWU, savedStackReg, 1, 0, -frameSize);
            PCodeUtilities_ResolveLabel(PCode_NewLabel());
        } else {
            negativeSize = -frameSize;
            highSize = (negativeSize >> 16) + ((negativeSize >> 15) & 1);
            PCodeUtilities_EmitInstruction(PC_LIS, scratchReg, 0, highSize);
            if ((short)negativeSize != 0) {
                PCodeUtilities_EmitInstruction(PC_ADDI, scratchReg, scratchReg, 0, (short)negativeSize);
            }
            PCodeUtilities_ResolveLabel(PCode_NewLabel());
            PCodeUtilities_EmitInstruction(PC_STWUX, savedStackReg, 1, scratchReg);
            PCodeUtilities_ResolveLabel(PCode_NewLabel());
        }
    } else {
        PCodeUtilities_ResolveLabel(PCode_NewLabel());
        PCodeUtilities_EmitInstruction(PC_STWUX, savedStackReg, 1, scratchReg);
        PCodeUtilities_ResolveLabel(PCode_NewLabel());
    }
    PCodeUtilities_EmitAddress(scratchReg, 1, data_00580fa8, 0);
}

void restore_gprs(PCodeBlock *func, Boolean a, Boolean b, SInt16 c)
{
    SInt32 i;
    Object *node;
    char *base;
    PCodeOperand *p;
    char buf[0x20];
    SInt32 regcount;
    NameSpace *old;

    if (b) {
        regcount = 0;
    } else {
        regcount = stack_frame_size;
    }

    if (a != 0 && (gGPRSaveSpan > 4 || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 1))) {
        PCodeUtilities_EmitInstruction(PC_LMW, gGPRSaveSpan - 1, 0x20 - gGPRSaveSpan, c, 0,
                                       regcount - (data_00587638 + frame_alignment_padding + data_00587634));
        Operands_AllocateGPR(0x20000);
    } else if (a == 0 && (gGPRSaveSpan > 4 || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 2))) {
        if (regcount > 0x7fff) {
            CError_FatalError(ERR_LOCAL_DATA_32K);
        }
        PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, c, 0, regcount - (data_00587638 + frame_alignment_padding));
        Operands_AllocateGPR(0x400);
        sprintf(buf, "_restgpr_%d", 0x20 - gGPRSaveSpan);
        old = currentNameSpace;
        currentNameSpace = registration_context;
        node = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = old;
        node->name = GetHashNameNode(buf);
        BE_symbol_GetOrCreateFunctionObjectSymbol(node);
        base = (char *)PCodeUtilities_CreateInstruction(1, gGPRSaveSpan, node, 0);
        i = 1;
        p = (PCodeOperand *)(base + 0x28);
        for (; i <= gGPRSaveSpan;) {
            p->kind = PCOp_GPR;
            p->value.reg = 0x20 - i++;
            p->flags = 2;
            p++;
        }
        PCode_AppendInstruction(func, (PCodeInstruction *)base);
    } else {
        for (i = 1; i <= gGPRSaveSpan; i++) {
            PCodeUtilities_EmitInstruction(PC_LWZ, 0x20 - i, c, 0,
                                           regcount - (data_00587638 + frame_alignment_padding + 4 * i));
            Operands_AllocateGPR(0x20000);
        }
    }
}

void save_gprs(PCodeBlock *func, Boolean a, Boolean b)
{
    SInt32 i;
    Object *node;
    char *base;
    PCodeOperand *p;
    char buf[0x20];
    SInt32 regcount;
    NameSpace *old;

    if (b) {
        regcount = stack_frame_size;
    } else {
        regcount = 0;
    }

    if (a != 0 && (gGPRSaveSpan > 4 || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 1))) {
        PCodeUtilities_EmitInstruction(PC_STMW, gGPRSaveSpan - 1, 0x20 - gGPRSaveSpan, 1, 0,
                                       regcount - (data_00587638 + frame_alignment_padding + data_00587634));
    } else if (a == 0 && (gGPRSaveSpan > 4 || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 2))) {
        if (regcount > 0x7fff) {
            CError_FatalError(ERR_LOCAL_DATA_32K);
        }
        PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, 1, 0, regcount - (data_00587638 + frame_alignment_padding));
        Operands_AllocateGPR(0x400);
        sprintf(buf, "_savegpr_%d", 0x20 - gGPRSaveSpan);
        old = currentNameSpace;
        currentNameSpace = registration_context;
        node = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = old;
        node->name = GetHashNameNode(buf);
        BE_symbol_GetOrCreateFunctionObjectSymbol(node);
        base = (char *)PCodeUtilities_CreateInstruction(1, gGPRSaveSpan, node, 0);
        i = 1;
        p = (PCodeOperand *)(base + 0x28);
        for (; i <= gGPRSaveSpan;) {
            p->kind = PCOp_GPR;
            p->value.reg = 0x20 - i++;
            p->flags = 1;
            p++;
        }
        PCode_AppendInstruction(func, (PCodeInstruction *)base);
    } else {
        for (i = 1; i <= gGPRSaveSpan; i++) {
            PCodeUtilities_EmitInstruction(PC_STW, 0x20 - i, 1, 0,
                                           regcount - (data_00587638 + frame_alignment_padding + 4 * i));
        }
    }
}

void restore_vrs(PCodeBlock *block)
{
    SInt16 i;
    SInt32 offset;
    char name[32];
    UInt32 size;
    Object *object;
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    NameSpace *savedValue;

    size = eabi_stack_frame_size -
           (data_00587638 + frame_alignment_padding + data_00587634 + stack_frame_adjustment + data_00588070) - 16;
    size = (size + 15) & ~15;
    if (stack_frame_size > 0x7fff)
        CError_FatalError(ERR_LOCAL_DATA_32K);
    if (copts.uniformSpillBlockWeight == 0 || (gVRSaveSpan <= 3 && data_00588521 != 0) || gVRSaveSpan <= 1) {
        i = 1;
        for (; i <= gVRSaveSpan; i++) {
            offset = size - (i - 1) * 16;
            if (size - (i - 1) * 16 != 0) {
                PCodeUtilities_EmitInstruction(PC_LI, 0, offset);
                Operands_AllocateGPR(PCodeInstruction_ObjectFlag2);
                PCodeUtilities_EmitInstruction(PC_LVX, 0x20 - i, stack_base_reg, 0);
                Operands_AllocateGPR(PCodeInstruction_ObjectFlag2);
            } else {
                PCodeUtilities_EmitInstruction(PC_LVX, 0x20 - i, 0, stack_base_reg);
                Operands_AllocateGPR(PCodeInstruction_ObjectFlag2);
            }
        }
    } else {
        PCodeUtilities_EmitInstruction(PC_ADDI, 0, stack_base_reg, 0, size + 16);
        sprintf(name, "_restvr%d", 0x20 - gVRSaveSpan);
        savedValue = currentNameSpace;
        currentNameSpace = registration_context;
        object = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = savedValue;
        object->name = GetHashNameNode(name);
        BE_symbol_GetOrCreateFunctionObjectSymbol(object);
        instruction = PCodeUtilities_CreateInstruction(1, gVRSaveSpan + 1, object, 0);
        operand = instruction->operandData.operands + 1;
        operand[0].kind = PCOp_GPR;
        operand[0].value.reg = 0xc;
        operand[0].flags = PCodeOperand_Use | PCodeOperand_Definition;
        i = 1;
        operand = instruction->operandData.operands + 2;
        for (; i <= gVRSaveSpan; i++) {
            operand->kind = PCOp_VR;
            operand->value.reg = 0x20 - i;
            operand->flags = PCodeOperand_Definition;
            operand++;
        }
        PCode_AppendInstruction((PCodeBlock *)block, (PCodeInstruction *)instruction);
    }
}

void emit_restore_fprs(PCodeBlock *func, Boolean flag)
{
    Object *helper;
    char name[32];
    PCodeOperand *span;
    PCodeInstruction *instruction;
    SInt16 i;
    SInt32 base;
    NameSpace *savedNamespace;

    if (flag)
        base = 0;
    else
        base = stack_frame_size;

    if (copts.uniformSpillBlockWeight == 0 || ((gFPRSaveSpan <= 3 && data_00588521 != 0) || gFPRSaveSpan <= 1)) {
        for (i = 1; i <= gFPRSaveSpan; i++) {
            emit_opcode_with_base_offset(PC_LFD, 0x20 - i, stack_base_reg, NULL, base - i * 8);
            Operands_AllocateGPR(0x20000);
        }
    } else {
        if (base != 0) {
            if (base > 0x7fff)
                CError_FatalError(ERR_LOCAL_DATA_32K);
            PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, stack_base_reg, 0, base);
            Operands_AllocateGPR(0x400);
        }
        sprintf(name, "_restfpr_%d", 0x20 - gFPRSaveSpan);
        savedNamespace = currentNameSpace;
        currentNameSpace = registration_context;
        helper = (Object *)CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = savedNamespace;
        helper->name = GetHashNameNode(name);
        BE_symbol_GetOrCreateFunctionObjectSymbol(helper);
        instruction = PCodeUtilities_CreateInstruction(1, gFPRSaveSpan, helper, 0);
        i = 1;
        span = (PCodeOperand *)&instruction->operandData.operands[1];
        if (gFPRSaveSpan >= 1) {
            do {
                span->kind = 1;
                span->value.reg = 0x20 - i;
                span->flags = 2;
                span++;
            } while (++i <= gFPRSaveSpan);
        }
        PCode_AppendInstruction(func, instruction);
    }
}

void emit_vr_saves(PCodeBlock *block)
{
    SInt16 index;
    SInt32 offset;
    char name[32];
    UInt32 saveOffset;
    Object *helper;
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    NameSpace *savedState;

    saveOffset = eabi_stack_frame_size -
                 (data_00587638 + frame_alignment_padding + data_00587634 + stack_frame_adjustment + data_00588070) -
                 16;
    saveOffset = (saveOffset + 15) & ~15;
    if (copts.uniformSpillBlockWeight == 0 || (gVRSaveSpan <= 3 && data_00588521 != 0) || gVRSaveSpan <= 1) {
        index = 1;
        for (; index <= gVRSaveSpan; index++) {
            offset = saveOffset - (index - 1) * 16;
            if (saveOffset - (index - 1) * 16 != 0) {
                PCodeUtilities_EmitInstruction(PC_LI, 0, offset);
                PCodeUtilities_EmitInstruction(PC_STVX, 0x20 - index, 1, 0);
            } else {
                PCodeUtilities_EmitInstruction(PC_STVX, 0x20 - index, 0, 1);
            }
        }
    } else {
        PCodeUtilities_EmitInstruction(PC_ADDI, 0, 1, 0, saveOffset + 16);
        sprintf(name, "_savevr%d", 0x20 - gVRSaveSpan);
        savedState = currentNameSpace;
        currentNameSpace = registration_context;
        helper = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = savedState;
        helper->name = GetHashNameNode(name);
        BE_symbol_GetOrCreateFunctionObjectSymbol(helper);
        instruction = PCodeUtilities_CreateInstruction(1, gVRSaveSpan + 2, helper, 0);
        operand = instruction->operandData.operands + 1;
        operand[0].kind = PCOp_GPR;
        operand[0].value.reg = 0xc;
        operand[0].flags = 3;
        operand[1].kind = PCOp_GPR;
        operand[1].value.reg = 0;
        operand[1].flags = 1;
        operand += 2;
        index = 1;
        for (; index <= gVRSaveSpan; index++) {
            operand->kind = PCOp_VR;
            operand->value.reg = 0x20 - index;
            operand->flags = 1;
            operand++;
        }
        PCode_AppendInstruction((PCodeBlock *)block, (PCodeInstruction *)instruction);
    }
}

void emit_save_fprs(PCodeBlock *block, Boolean savefpr)
{
    char buf[32];
    SInt16 i;
    int base;
    Object *obj;
    PCodeInstruction *result;
    PCodeOperand *slot;
    NameSpace *save;

    base = savefpr ? stack_frame_size : 0;

    if (copts.uniformSpillBlockWeight == 0 || (gFPRSaveSpan <= 3 && data_00588521 != 0) || gFPRSaveSpan <= 1) {
        for (i = 1; i <= gFPRSaveSpan; i++) {
            PCodeUtilities_EmitInstruction(PC_STFD, 0x20 - i, 1, 0, base - 8 * i);
        }
    } else {
        if (base != 0) {
            if (base > 0x7fff)
                CError_FatalError(ERR_LOCAL_DATA_32K);
            PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, 1, 0, base);
            Operands_AllocateGPR(0x400);
        }
        sprintf(buf, "_savefpr_%d", 0x20 - gFPRSaveSpan);
        save = currentNameSpace;
        currentNameSpace = registration_context;
        obj = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
        currentNameSpace = save;
        obj->name = GetHashNameNode(buf);
        BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
        result = PCodeUtilities_CreateInstruction(1, gFPRSaveSpan, obj, 0);
        i = 1;
        slot = result->operandData.operands + 1;
        if (gFPRSaveSpan >= 1) {
            do {
                slot->kind = PCOp_FPR;
                slot->value.reg = 0x20 - i;
                slot->flags = 1;
                slot++;
            } while (++i <= gFPRSaveSpan);
        }
        PCode_AppendInstruction(block, (PCodeInstruction *)result);
    }
}

/* Referenced symbols. */

static inline UInt8 StackFrameEABI_LoadMultipleEnabled(void)
{
    return copts.useRegisterSaveHelpers;
}

static inline UInt8 StackFrameEABI_VRSAVEEnabled(void)
{
    return copts.altivecVrsave;
}

void StackFrameEABI_MergePrologueEpilogue(PCodeBlock *block, char emitReturn)
{
    PCodeBlock *savedBlock;
    PCodeInstruction *instruction;
    SInt32 frameReg;
    Boolean restoreLR;
    SInt32 largeFrame;
    SInt32 savedFrameReg;
    SInt16 restoreBaseReg;

    savedBlock = gCurrentBlock;
    largeFrame = (0x7fff < stack_frame_size);
    frameReg = -1;
    savedFrameReg = -1;
    restoreLR = !data_00588521 || (copts.uniformSpillBlockWeight != 0 && (gFPRSaveSpan > 3 || gVRSaveSpan > 3));
    if (!restoreLR) {
        if (StackFrameEABI_LoadMultipleEnabled() == 0 || copts.nativeByteOrder != 0) {
            restoreLR = (gGPRSaveSpan > 4) || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 2);
        }
    }
    restoreBaseReg = stack_base_reg;
    gCurrentBlock = block;
    if (data_005882c0.record != NULL) {
        restoreLR = 0;
    }
    if (gHasAltivecFrame != 0) {
        instruction = block->instructions;
        if (data_00588521 != 0) {
            frameReg = 1;
            if (StackFrameEABI_VRSAVEEnabled() != 0) {
                if (block->instructions != NULL && block->instructions->opcode == PC_STW) {
                    savedFrameReg = block->instructions->operandData.operands[1].value.reg;
                    PCode_UnlinkInstruction(block->instructions);
                    if (savedFrameReg == -1) {
                        CError_FATAL(910);
                    }
                }
            }
        } else {
            if (instruction != NULL && instruction->opcode == PC_STW) {
                frameReg = instruction->operandData.operands[1].value.reg;
                PCode_UnlinkInstruction(instruction);
                instruction = block->instructions;
                if (StackFrameEABI_VRSAVEEnabled() != 0 && instruction != NULL && instruction->opcode == PC_STW) {
                    CError_FATAL(922);
                    savedFrameReg = instruction->operandData.operands[1].value.reg;
                    PCode_UnlinkInstruction(instruction);
                }
            } else {
                CError_FATAL(928);
            }
        }
        if (vrsave_mask != 0 && StackFrameEABI_VRSAVEEnabled() != 0) {
            if (data_00588521 != 0 && savedFrameReg != -1) {
                PCodeUtilities_EmitInstruction(PC_MTSPR, 0x100, savedFrameReg);
            } else {
                PCodeUtilities_EmitInstruction(PC_LWZ, 0xb, frameReg, 0,
                                               -(frame_alignment_padding + data_00587638 + stack_frame_adjustment +
                                                 data_00587634 + data_00588070 + data_0058764c + data_005880d8));
            }
        }
        if (vrsave_mask != 0 && gHasAltivecFrame != 0 && StackFrameEABI_VRSAVEEnabled() != 0 && savedFrameReg == -1) {
            PCodeUtilities_EmitInstruction(PC_MTSPR, 0x100, 0xb);
        }
    }
    if (gVRSaveSpan != 0) {
        restore_vrs(block);
    }
    if (!largeFrame && gHasAltivecFrame == 0 && restoreLR) {
        emit_opcode_with_base_offset(PC_LWZ, 0, stack_base_reg, NULL, stack_frame_size + 4);
    }
    if (data_005883ee != 0 && data_005882c0.record == NULL) {
        emit_opcode_with_base_offset(PC_LWZ, 0xc, stack_base_reg, NULL, r12_save_offset);
        PCodeUtilities_EmitInstruction(PC_MTCRF, 0xff, 0xc);
    }
    if (gFPRSaveSpan != 0) {
        emit_restore_fprs(block, 0);
    }
    if (restoreBaseReg == kMergeTag && gGPRSaveSpan != 0) {
        PCodeUtilities_EmitInstruction(PC_MR, 0xc, restoreBaseReg);
        restoreBaseReg = 0xc;
    }
    if (gGPRSaveSpan != 0) {
        restore_gprs(block, (StackFrameEABI_LoadMultipleEnabled() != 0) && !copts.nativeByteOrder, 0, restoreBaseReg);
    }
    if (data_005882c0.record == NULL && stack_frame_size != 0) {
        if (data_0058852d != 0 || largeFrame || gHasAltivecFrame != 0) {
            emit_opcode_with_base_offset(PC_LWZ, 1, 1, NULL, 0);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, 1, 1, 0, stack_frame_size);
        }
    }
    if ((largeFrame || gHasAltivecFrame != 0) && restoreLR) {
        emit_opcode_with_base_offset(PC_LWZ, 0, 1, NULL, 4);
    }
    if (restoreLR) {
        PCodeUtilities_EmitInstruction(PC_MTLR, 0);
    }
    if (data_005882c0.record != NULL) {
        emit_restore_special_registers(restoreBaseReg);
        if (stack_frame_size != 0) {
            if (data_0058852d != 0 || largeFrame || gHasAltivecFrame != 0) {
                emit_opcode_with_base_offset(PC_LWZ, 1, 1, NULL, 0);
            } else {
                PCodeUtilities_EmitInstruction(PC_ADDI, 1, 1, 0, stack_frame_size);
            }
        }
    }
    if (emitReturn != 0) {
        if (data_005882c0.record != NULL) {
            PCodeUtilities_EmitInstruction(PC_RFI);
        } else {
            PCodeUtilities_EmitInstruction(PC_BLR);
        }
        Operands_AllocateGPR(0x800000);
    }
    block->flags |= 2;
    gCurrentBlock = savedBlock;
}

/* 0x5601e0 (file name) */

static inline int NeedGSave(void)
{
    return gGPRSaveSpan > 4 || (copts.uniformSpillBlockWeight != 0 && gGPRSaveSpan > 2);
}

void StackFrameEABI_GeneratePrologueEpilogue(PCodeBlock *block, int prologueFlags, int epilogueFlags)
{
    PCodeBlock *savedBlock;
    Boolean frameAllocated;
    SInt32 vrsaveRegister;
    Boolean saveLinkRegister;
    Boolean saveGPRs;
    SInt32 parameterBaseRegister;
    PCodeBlock *currentBlock;
    PCodeInstruction *instruction;
    SInt32 operandIndex;

    savedBlock = gCurrentBlock;
    saveLinkRegister = frameAllocated = 0;
    vrsaveRegister = !data_00588521;
    saveLinkRegister = vrsaveRegister || (copts.uniformSpillBlockWeight != 0 && (gFPRSaveSpan > 3 || gVRSaveSpan > 3));
    if (!saveLinkRegister && (copts.useRegisterSaveHelpers == 0 || copts.nativeByteOrder != 0)) {
        saveLinkRegister = saveGPRs = NeedGSave();
    }
    parameterBaseRegister = -1;
    vrsaveRegister = -1;
    gCurrentBlock = block;
    if (gHasAltivecFrame != 0) {
        PCodeInstruction *load;
        load = block->instructions;
        if (data_00588521 != 0) {
            parameterBaseRegister = 1;
            if (copts.altivecVrsave != 0 && block->instructions != NULL && block->instructions->opcode == PC_LWZ) {
                vrsaveRegister = block->instructions->operandData.operands[0].value.reg;
                PCode_UnlinkInstruction(block->instructions);
                if (vrsaveRegister == -1)
                    CError_FATAL(676);
            }
        } else {
            if (load != NULL && load->opcode == PC_LWZ) {
                parameterBaseRegister = load->operandData.operands[0].value.reg;
                PCode_UnlinkInstruction(load);
                load = block->instructions;
                if (copts.altivecVrsave != 0 && load != NULL && load->opcode == PC_LWZ) {
                    CError_FATAL(686);
                    vrsaveRegister = load->operandData.operands[0].value.reg;
                    PCode_UnlinkInstruction(load);
                }
            } else {
                CError_FATAL(691);
            }
        }

        if (eabi_stack_frame_size != 0) {
            for (currentBlock = gPCodeBlocks; currentBlock != NULL; currentBlock = currentBlock->next) {
                for (instruction = currentBlock->instructions; instruction != NULL; instruction = instruction->next) {
                    for (operandIndex = 0; operandIndex < instruction->operand_count; operandIndex++) {
                        if (instruction->operandData.operands[operandIndex].kind == PCOp_MEMORY &&
                            instruction->operandData.operands[operandIndex].flags == 1 &&
                            Registers_GetInfo(instruction->operandData.operands[operandIndex].object)->in_param_area !=
                                0) {
                            switch (instruction->opcode) {
                                case PC_LBZ:
                                case PC_LBZU:
                                case PC_LHZ:
                                case PC_LHZU:
                                case PC_LHA:
                                case PC_LHAU:
                                case PC_LWZ:
                                case PC_LWZU:
                                case PC_LMW:
                                case PC_STB:
                                case PC_STBU:
                                case PC_STH:
                                case PC_STHU:
                                case PC_STW:
                                case PC_STWU:
                                case PC_STMW:
                                case PC_ADDI:
                                case PC_ADDIC:
                                case PC_ADDICR:
                                case PC_LFS:
                                case PC_LFSU:
                                case PC_LFD:
                                case PC_LFDU:
                                case PC_STFS:
                                case PC_STFSU:
                                case PC_STFD:
                                case PC_STFDU:
                                    instruction->operandData.operands[1].value.reg = parameterBaseRegister;
                                    instruction->flags |= fCallerSPRelative;
                                    break;
                            }
                        }
                    }
                }
            }
        } else {
            parameterBaseRegister = -1;
        }
    }
    if (data_005882c0.record != NULL) {
        saveLinkRegister = 0;
        emit_stack_frame_allocation(0xc);
        frameAllocated = 1;
        generate_interrupt_register_saves();
    }
    if (saveLinkRegister) {
        PCodeUtilities_EmitInstruction(PC_MFLR, 0);
        PCodeUtilities_EmitInstruction(PC_STW, 0, 1, 0, 4);
    }
    if ((stack_frame_size != 0 || data_005884ff != 0) && !frameAllocated) {
        emit_stack_frame_allocation(0xc);
        frameAllocated = 1;
    }
    if (gFPRSaveSpan != 0)
        emit_save_fprs(block, frameAllocated);
    if (gGPRSaveSpan != 0) {
        Boolean useSaveHelper = copts.useRegisterSaveHelpers != 0 && !copts.nativeByteOrder;
        save_gprs(block, useSaveHelper, frameAllocated);
    }
    if (stack_frame_size != 0 || data_005884ff != 0)
        save_and_update_vrsave(0xc, vrsaveRegister);
    if (data_00588521 == 0 && parameterBaseRegister != -1)
        PCodeUtilities_EmitInstruction(PC_MR, parameterBaseRegister, 0xc);
    if (gVRSaveSpan != 0)
        emit_vr_saves(block);
    if (data_005883ee != 0 && data_005882c0.record == NULL) {
        PCodeUtilities_EmitInstruction(PC_MFCR, 0xc);
        PCodeUtilities_EmitInstruction(PC_STW, 0xc, 1, 0, r12_save_offset);
    }
    if (data_0058852d != 0)
        PCodeUtilities_EmitInstruction(PC_MR, 0x1f, 1);
    block->flags |= 1;
    gCurrentBlock = savedBlock;
}

#pragma opt_lifetimes off

void StackFrameEABI_FinalizeLayout(struct PCodeBlock *function)
{
    UInt32 size;
    int combinedSize;
    unsigned int adjustment;
    unsigned int extraSize;
    unsigned int alignmentMask;
    int padding;
    SInt16 vrSaveSpan;

    if (gHasAltivecFrame != 0)
        size = 16U;
    else
        size = 8U;
    adjustment = 0U;
    data_00587e40 = size;
    if (data_005883ee != 0) {
        size = 0U;
        if (data_005882c0.record == NULL)
            size += 1U;
        if (size != 0U)
            adjustment = 1U;
    }
    vrSaveSpan = gVRSaveSpan;
    size = vrSaveSpan;
    combinedSize = gStackFrameSize;
    combinedSize += outgoing_argument_size;
    combinedSize += size;
    extraSize = adjustment * 14U;
    adjustment = gGPRSaveSpan;
    combinedSize = combinedSize * 14U;
    size = gFPRSaveSpan;
    adjustment = adjustment * 14U;
    size = size * 14U;
    combinedSize += data_005882c0.value;
    extraSize += combinedSize;
    adjustment += extraSize;
    size += adjustment;
    if (size != 0U)
        data_005880cc = 8U;
    if (gHasAltivecFrame != 0) {
        combinedSize = gStackFrameSize;
        combinedSize += 15U;
        combinedSize &= 4294967280U;
        gStackFrameSize = combinedSize;
        size = outgoing_argument_size;
        size += data_005880cc;
        size += 15U;
        size &= 4294967280U;
        size -= data_005880cc;
    } else {
        adjustment = gStackFrameSize;
        adjustment += 7U;
        adjustment &= 4294967288U;
        gStackFrameSize = adjustment;
        size = outgoing_argument_size;
        size += 7U;
        size &= 4294967288U;
    }
    outgoing_argument_size = size;
    size = data_005880cc;
    size += outgoing_argument_size;
    size += gStackFrameSize;
    stack_frame_size = size;
    if (gHasAltivecFrame != 0) {
        size = Registers_GetOperandRegMask(function);
        vrsave_mask = size;
        data_005880d8 = 16U;
        vrSaveSpan = gVRSaveSpan;
        size = vrSaveSpan;
        size = size << 4;
        data_0058764c = size;
        if (gVRSaveSpan != 0)
            data_005884ff = 1U;
        stack_frame_size += data_0058764c + 16U;
    }
    if (data_005882c0.record != NULL) {
        size = data_00588204;
        size = size << 2;
        data_005876a8 = size;
        padding = size;
        padding += data_00587e40;
        size = data_00587e40;
        size -= 1U;
        padding -= 1U;
        size = ~size;
        padding &= size;
        padding -= data_005876a8;
        size = padding;
        stack_frame_padding = padding;
        size += data_005876a8;
        stack_frame_size += size;
    }
    if (data_005883ee != 0 && data_005882c0.record == NULL) {
        size = stack_frame_size;
        r12_save_offset = size;
        data_00588070 = 4U;
        stack_frame_size += 4U;
    }
    size = gGPRSaveSpan;
    size = size << 2;
    data_00587634 = size;
    size = stack_frame_size;
    size += data_00587634;
    adjustment = data_00587e40;
    adjustment += size;
    alignmentMask = ~(data_00587e40 - 1U);
    adjustment -= 1U;
    adjustment &= alignmentMask;
    adjustment -= size;
    stack_frame_adjustment = adjustment;
    size = gFPRSaveSpan;
    size = size << 3;
    padding = size;
    data_00587638 = size;
    frame_alignment_padding = padding = ((padding + data_00587e40 - 1U) & alignmentMask) - size;
    size = stack_frame_adjustment;
    size += data_00587634;
    size += padding;
    size += data_00587638;
    stack_frame_size += size;
    if (stack_frame_size > 32767)
        CError_FatalError(ERR_LOCAL_DATA_32K);
    size = stack_frame_size;
    eabi_stack_frame_size = size;
}

#pragma opt_lifetimes reset

void StackFrameEABI_AllocateObjectSlot(Object *object)
{
    SInt16 alignment;
    alignment = StackFrameEABI_GetTypeAlignment(object->type);
    object->u.var.uid = gStackFrameSize = (gStackFrameSize + alignment - 1) & ~(alignment - 1);
    gStackFrameSize += object->type->size;
}

void StackFrameEABI_Initialize(void)
{
    int optionEnabled;
    int settingEnabled;
    int useDefault;
    int eligible;
    data_0058764c = 0;
    data_005880d8 = 0U;
    useDefault = 0;
    eligible = 0;
    if (data_00588521 != 0) {
        optionEnabled = 0;
        if (data_0058852d == 0)
            optionEnabled = 1;
        if (optionEnabled != 0)
            eligible = 1;
    }
    if (eligible != 0) {
        settingEnabled = 0;
        if (data_005882c0.record == NULL)
            settingEnabled = 1;
        if (settingEnabled != 0)
            useDefault = 1;
    }
    data_005880cc = useDefault != 0 ? 0 : 8;
    outgoing_argument_size = data_00588521 != 0 ? 0 : 0;
    interrupt_register_save_mask = 0;
    data_00588204 = 0;
    if (data_0058852d != 0) {
        Object *object;
        data_00580fa8 = galloc(54);
        memclrw(data_00580fa8, 54);
        object = data_00580fa8;
        object->type = &stvoid;
        object = data_00580fa8;
        object->otype = OT_OBJECT;
        data_00580fa8->name = GetHashNameNodeExport("<dummy>");
        data_00580fa8->datatype = DLOCAL;
        data_00580fa8->u.var.info = CPrep_AllocateVarInfo();
    }
}

static inline unsigned int ReadFrameWord(const unsigned char *storage)
{
    return *(const unsigned int *)storage;
}
