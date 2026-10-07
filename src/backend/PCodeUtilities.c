#define CERROR_FILE "PCodeUtilities.c"
#include "compiler/common.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/Exceptions.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Registers.h"
enum { Register2 = 2, Register13 = 13 };

static inline SInt16 lowHalf(SInt32 value)
{
    return value;
}

static inline UInt8 exception_scopes_enabled(void)
{
    return copts.exceptions;
}

static inline UInt8 PCodeUtilities_ExceptionScopesEnabled(void)
{
    return copts.exceptions;
}

static inline UInt8 PCodeUtilities_ShouldSplitObjectInstructionBlock(void)
{
    return copts.instructionSchedulingMode;
}

/* Object cv-qualifier: for pointer types the pointee qualifier lives in the
 * TypePointer record, otherwise the object's own qual field is used. */
#define PCode_GetQual(obj)                                                                                             \
    ((SInt32)(((obj)->type->type == TYPEPOINTER) ? TYPE_POINTER((obj)->type)->qual : (obj)->qual))

#define PCARG(type) (*(type *)((args += 4) - 4))

PCodeInstruction *create_pcode_instruction(SInt16 opcode, char *args)
{
    PCodeOpcodeDescriptor *desc;
    PCodeInstruction *inst;
    SInt32 extra;
    const char *fmt;
    PCodeOperand *opnd;
    SInt32 mode;
    SInt32 count;
    SInt32 variableCount;

    desc = &gPCodeOpcodeDescriptors[opcode];
    variableCount = 0;
    fmt = desc->operand_format;
    count = desc->operand_count;
    extra = 0;
    if (*fmt == '#') {
        variableCount += PCARG(SInt32);
        count += variableCount;
        fmt++;
    }
    if (desc->flags & 0x200)
        extra++;
    inst = (PCodeInstruction *)CompilerTools_AllocatePool(sizeof(PCodeInstruction) +
                                                          (count + extra) * sizeof(PCodeOperand));
    inst->opcode = opcode;
    inst->operand_count = count;
    inst->flags = desc->flags;
    opnd = inst->operandData.operands;

    while (*fmt != 0) {
        if (*fmt == ',')
            fmt++;
        mode = 1;
        if (*fmt == '=') {
            mode++;
            fmt++;
        } else if (*fmt == '+') {
            mode = 3;
            fmt++;
        }
        switch (*fmt) {
            case 'b': {
                SInt32 value = PCARG(SInt32);
                if (value == 0)
                    mode = 0;
                opnd->kind = PCOp_GPR;
                opnd->value.reg = value;
                opnd->flags = mode;
                break;
            }
            case 'c':
                opnd->kind = PCOp_CRFIELD;
                opnd->value.reg = PCARG(SInt16);
                opnd->flags = mode;
                break;
            case 'f':
                opnd->kind = PCOp_FPR;
                opnd->value.reg = PCARG(SInt16);
                opnd->flags = mode;
                break;
            case 'v':
                opnd->kind = PCOp_VR;
                opnd->value.reg = PCARG(SInt16);
                opnd->flags = mode;
                break;
            case 'i':
                opnd->kind = PCOp_IMMEDIATE;
                opnd->value.signed_value = PCARG(SInt32);
                opnd->object = NULL;
                break;
            case 'l': {
                SInt32 label = PCARG(SInt32);
                if (label != 0) {
                    opnd->kind = PCOp_LABEL;
                    opnd->value.signed_value = label;
                } else {
                    Object *object;
                    opnd->kind = PCOp_MEMORY;
                    object = PCARG(Object *);
                    CError_ASSERT(151, object->otype == OT_OBJECT);
                    opnd->object = object;
                    opnd->value.signed_value = 0;
                    opnd->flags = 4;
                }
                break;
            }
            case 'm': {
                Object *object = PCARG(Object *);
                if (object != NULL) {
                    CError_ASSERT(162, object->otype == OT_OBJECT);
                    if (object->datatype == DABSOLUTE) {
                        opnd->kind = PCOp_IMMEDIATE;
                        opnd->object = object;
                        opnd->value.signed_value = PCARG(SInt32);
                    } else {
                        SInt32 quals;
                        opnd->kind = PCOp_MEMORY;
                        opnd->object = object;
                        opnd->value.signed_value = PCARG(SInt32);
                        if (inst->flags & (fIsRead | fIsWrite)) {
                            quals = PCode_GetQual(object) & 2;
                            if (quals != 0)
                                inst->flags |= fIsVolatile;
                            quals = PCode_GetQual(object) & 1;
                            if (quals != 0)
                                inst->flags |= fIsConst;
                        }
                        if (inst->flags & 0x24)
                            opnd->flags = 4;
                        else if (object->datatype == DLOCAL)
                            opnd->flags = 1;
                        else if (PCodeUtilities_Require(object))
                            opnd->flags = 2;
                        else
                            opnd->flags = 6;
                    }
                } else {
                    opnd->kind = PCOp_IMMEDIATE;
                    opnd->value.signed_value = PCARG(SInt32);
                    opnd->object = NULL;
                    if (inst->flags & (fIsRead | fIsWrite))
                        inst->flags |= fIsPtrOp;
                }
                break;
            }
            case 'M': {
                Object *object = PCARG(Object *);
                if (object != NULL) {
                    CError_ASSERT(212, object->otype == OT_OBJECT);
                    if (object->datatype == DABSOLUTE) {
                        opnd->kind = PCOp_IMMEDIATE;
                        opnd->object = object;
                        opnd->value.signed_value = PCARG(SInt32);
                    } else {
                        SInt32 quals;
                        opnd->kind = PCOp_MEMORY;
                        opnd->object = object;
                        opnd->value.signed_value = PCARG(SInt32);
                        if (inst->flags & (fIsRead | fIsWrite)) {
                            quals = PCode_GetQual(object) & 2;
                            if (quals != 0)
                                inst->flags |= fIsVolatile;
                            quals = PCode_GetQual(object) & 1;
                            if (quals != 0)
                                inst->flags |= fIsConst;
                        }
                        if (object->datatype == DLOCAL) {
                            CError_FATAL(234);
                        } else {
                            opnd->flags = 8;
                        }
                    }
                } else {
                    opnd->kind = PCOp_IMMEDIATE;
                    opnd->value.signed_value = PCARG(SInt32);
                    opnd->object = NULL;
                    if (inst->flags & (fIsRead | fIsWrite))
                        inst->flags |= fIsPtrOp;
                }
                break;
            }
            case 'r':
                opnd->kind = PCOp_GPR;
                opnd->value.reg = PCARG(SInt16);
                opnd->flags = mode;
                break;
            case 'C':
                opnd->kind = PCOp_SPR;
                opnd->value.reg = 1;
                opnd->flags = mode;
                break;
            case 'L':
                opnd->kind = PCOp_SPR;
                opnd->value.reg = 2;
                opnd->flags = mode;
                break;
            case 'V': {
                SInt32 index = variableCount;
                while (index--) {
                    opnd->kind = PCOp_GPR;
                    opnd->value.reg = 0x1f - index;
                    opnd->flags = mode;
                    opnd++;
                }
                opnd--;
                break;
            }
            case 'X':
                opnd->kind = PCOp_SPR;
                opnd->value.reg = 0;
                opnd->flags = mode;
                break;
            case 'Y': {
                SInt32 index = 0;
                do {
                    opnd->kind = PCOp_CRFIELD;
                    opnd->value.reg = index;
                    opnd->flags = mode;
                    opnd++;
                } while (++index <= 7);
                opnd--;
                break;
            }
            case 'Z':
                opnd->kind = PCOp_CRFIELD;
                opnd->value.reg = 0;
                opnd->flags = mode;
                break;
            case 'S':
                opnd->kind = PCOp_SPR;
                opnd->value.reg = PCARG(SInt16);
                opnd->flags = mode;
                break;
            case 'p':
                opnd->kind = TYPEMEMBERPOINTER;
                break;
            default:
            case '?':
                CError_FATAL(313);
                break;
        }
        fmt++;
        opnd++;
    }
    return inst;
}

PCodeInstruction *PCodeUtilities_CreateInstruction(UInt16 op, ...)
{
    va_list arguments;
    PCodeInstruction *instruction;
    arguments = (va_list)&op + (((va_list)(&op + 1) - (va_list)&op + 3) / 4) * 4;
    instruction = create_pcode_instruction(op, arguments);
    return instruction;
}

PCodeInstruction *PCodeUtilities_EmitInstruction(short opcode, ...)
{
    va_list arguments = (va_list)&opcode + (((va_list)(&opcode + 1) - (va_list)&opcode + 3) / 4) * 4;
    return PCode_AppendInstruction(gCurrentBlock, create_pcode_instruction(opcode, arguments));
}

void PCodeUtilities_MakeRecordForm(PCodeInstruction *o)
{
    int num = o->operand_count;

    if (o->opcode == PC_ANDI || o->opcode == PC_ANDIS) {
        o->flags |= fRecordBit;
        return;
    }

    if ((unsigned short)(o->opcode - 0x3f) <= 1) {
        PCodeOperand *p;

        o->flags |= fSetsCarry;
        o->flags |= fRecordBit;
        o->operand_count = 5;
        o->opcode = PC_ADDICR;

        p = &o->operandData.operands[3];
        p->kind = PCOp_SPR;
        p->value.reg = 0;
        p->flags = 2;

        p = &o->operandData.operands[4];
        p->kind = PCOp_CRFIELD;
        p->value.reg = 0;
        p->flags = 2;
        return;
    }

    {
        int idx = num;
        PCodeOperand *it = &o->operandData.operands[idx];

        it->kind = PCOp_CRFIELD;
        it->flags = 2;
        o->operand_count = num + 1;

        if ((o->flags & 3) == 2) {
            it->value.reg = 1;
        } else if ((o->flags & 3) == 3) {
            it->value.reg = 6;
        } else {
            it->value.reg = 0;
        }

        if (o->opcode != PC_ADDICR) {
            o->flags |= fRecordBit;
        }
    }
}

void PCodeUtilities_ResolveLabel(PCodeLabel *data)
{
    if (gCurrentBlock->instruction_count != 0) {
        PCode_AddSuccessor(gCurrentBlock, data);
        PCode_CreateBlock();
    }
    PCode_ResolveLabel((gCurrentBlock), data);
}

void PCodeUtilities_EmitConditionBranch(SInt16 operand, SInt16 condition, SInt16 branchIfTrue, PCodeLabel *targetBlock)
{
    PCodeLabel *fallthroughBlock;
    SInt32 kind;

    fallthroughBlock = PCode_NewLabel();
    switch (condition) {
        case 0x18:
            branchIfTrue = !branchIfTrue;
        case 0x17:
            kind = 2;
            break;
        case 0x16:
            branchIfTrue = !branchIfTrue;
        case 0x13:
            kind = 0;
            break;
        case 0x15:
            branchIfTrue = !branchIfTrue;
        case 0x14:
            kind = 1;
            break;
    }
    PCodeUtilities_EmitInstruction(branchIfTrue ? 5 : 8, operand, kind, targetBlock);
    PCode_AddSuccessor(gCurrentBlock, targetBlock);
    PCode_AddSuccessor(gCurrentBlock, fallthroughBlock);
    PCode_CreateBlock();
    PCode_ResolveLabel((gCurrentBlock), (fallthroughBlock));
}

void PCodeUtilities_EmitBranch(PCodeLabel *target)
{
    PCodeUtilities_EmitInstruction(0U, target);
    PCode_AddSuccessor(gCurrentBlock, target);
    PCode_CreateBlock();
}

unsigned int PCodeUtilities_EmitConditionalBranch(unsigned int opcode, PCodeLabel *target)
{
    PCodeLabel *fallthrough = PCode_NewLabel();
    PCodeUtilities_EmitInstruction(opcode, target);
    PCode_AddSuccessor(gCurrentBlock, target);
    PCode_AddSuccessor(gCurrentBlock, fallthrough);
    PCode_CreateBlock();
    PCode_ResolveLabel(gCurrentBlock, fallthrough);
}

void PCodeUtilities_EmitInstructionAndCreateBlock(Object *object)
{
    PCodeUtilities_EmitInstruction(18, object, 0);
    PCode_CreateBlock();
}

PCodeOperand *PCodeUtilities_004a2290(PCodeOperand *operand, UInt32 gprMask, UInt32 fprMask, UInt32 vrMask)
{
    SInt32 reg;

    operand->kind = PCOp_GPR;
    operand->value.reg = 0;
    operand->flags = 2;
    if (gprMask & 1) {
        operand->flags |= 1;
    }
    operand++;

    reg = 3;
    do {
        operand->kind = PCOp_GPR;
        operand->value.reg = reg;
        operand->flags = 2;
        if (gprMask & (1L << reg)) {
            operand->flags |= 1;
        }
        reg++;
        operand++;
    } while (reg <= 12);

    reg = 0;
    do {
        operand->kind = PCOp_FPR;
        operand->value.reg = reg;
        operand->flags = 2;
        if (fprMask & (1L << reg)) {
            operand->flags |= 1;
        }
        reg++;
        operand++;
    } while (reg <= 13);

    reg = 0;
    do {
        operand->kind = PCOp_VR;
        operand->value.reg = reg;
        operand->flags = 2;
        if (vrMask & (1L << reg)) {
            operand->flags |= 1;
        }
        reg++;
        operand++;
    } while (reg <= 19);

    operand->kind = PCOp_CRFIELD;
    operand->value.reg = 0;
    operand->flags = 2;
    operand++;
    operand->kind = PCOp_CRFIELD;
    operand->value.reg = 1;
    operand->flags = 3;
    operand++;
    operand->kind = PCOp_CRFIELD;
    operand->value.reg = 6;
    operand->flags = 2;
    operand++;
    operand->kind = PCOp_CRFIELD;
    operand->value.reg = 7;
    operand->flags = 2;
    operand++;
    return operand;
}

void PCodeUtilities_EmitObjectInstructionWithPayload(Object *operand, SInt16 emitInstruction, SInt32 firstRegisterMask,
                                                     SInt32 secondRegisterMask, SInt32 thirdRegisterMask)
{
    SInt32 operandCount = 49;
    PCodeInstruction *instruction;
    PCodeLabel *label;
    PCodeOperand *payload;
    char operandKind;

    if (PCodeUtilities_ExceptionScopesEnabled() && gCurrentStatement)
        operandCount += Exceptions_CountBoundObjectFields(gCurrentStatement->dobjstack);

    operandKind = ObjGen_PPC_EABI_GetSectionAlignmentOrKind(operand);
    if (operandKind == 10) {
        instruction = PCodeUtilities_CreateInstruction(1, operandCount, operand, 0);
    } else if (operandKind == 11) {
        instruction = PCodeUtilities_CreateInstruction(1, operandCount, operand, 0);
        instruction->flags |= fAbsolute;
    } else {
        instruction = PCodeUtilities_CreateInstruction(1, operandCount, operand, 0);
    }

    payload = PCodeUtilities_004a2290(instruction->operandData.operands + 1, firstRegisterMask, secondRegisterMask,
                                      thirdRegisterMask);

    if (PCodeUtilities_ExceptionScopesEnabled() && gCurrentStatement)
        Exceptions_CollectRegisterOperands(gCurrentStatement->dobjstack, payload);

    if (PCodeUtilities_ShouldSplitObjectInstructionBlock()) {
        label = PCode_NewLabel();
        if (gCurrentBlock->instruction_count != 0) {
            PCode_AddSuccessor(gCurrentBlock, label);
            PCode_CreateBlock();
        }
        PCode_ResolveLabel(gCurrentBlock, label);
    }

    PCode_AppendInstruction(gCurrentBlock, instruction);

    if (emitInstruction != 0)
        PCodeUtilities_EmitInstruction(PC_NOP);

    label = PCode_NewLabel();
    if (gCurrentBlock->instruction_count != 0) {
        PCode_AddSuccessor(gCurrentBlock, label);
        PCode_CreateBlock();
    }
    PCode_ResolveLabel(gCurrentBlock, label);

    if (PCodeUtilities_ExceptionScopesEnabled() && gCurrentStatement)
        Exceptions_AppendScopeEntry(instruction, gCurrentStatement->dobjstack);
}

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

void PCodeUtilities_EmitAddress(short resultReg, short baseReg, struct Object *object, short offset)
{
    short addressReg;
    PCodeBlock *block;
    PCodeInstruction *instruction;

    addressReg = baseReg;
    if (object != NULL && ((int)baseReg == 2 || (int)baseReg == 13)) {
        addressReg = 0;
    }
    if (object == NULL && offset == 0) {
        PCodeUtilities_EmitInstruction(PC_MR, resultReg, addressReg);
        return;
    }
    PCodeUtilities_EmitInstruction(PC_ADDI, resultReg, addressReg, object, offset);
    if (object != NULL) {
        if ((int)addressReg == 0) {
            block = gCurrentBlock;
            instruction = block->reverse_instructions;
            instruction->flags = ~gPCodeOpcodeDescriptors[instruction->opcode].flags & instruction->flags |
                                 gPCodeOpcodeDescriptors[PC_LI].flags;
            instruction->opcode = PC_LI;
            instruction->operand_count = 2;
            instruction->operandData.operands[1] = instruction->operandData.operands[2];
        }
    } else
        CE_ASSERT(baseReg == 0, CError_FATAL(848));
}

PCodeInstruction *PCodeUtilities_CreateInstructionWithObject(short operand1, short operand2, Object *operand3,
                                                             short operand4, unsigned char recordInstruction)
{
    PCodeInstruction *instruction;
    if (operand3 == NULL) {
        CError_FATAL(869);
    }
    {
        short first = operand1;
        short second = operand2;
        short fourth = operand4;
        instruction = PCodeUtilities_CreateInstruction(63, first, second, operand3, fourth);
    }
    {
        unsigned char record = recordInstruction;
        if (record) {
            PCode_AppendInstruction(gCurrentBlock, instruction);
        }
    }
    return instruction;
}

PCodeInstruction *PCodeUtilities_MakeInstructionWithObject(short opcode, short operand, Object *object, short flags,
                                                           char appendToBlock)
{
    PCodeInstruction *result;

    if (copts.f25 != 0) {
        if ((int)operand == 0) {
            CError_FATAL(889);
        }
        result = PCodeUtilities_CreateInstruction(0x42, opcode, operand, object, flags);
    } else {
        if (operand != 0) {
            CError_FATAL(894);
        }
        result = PCodeUtilities_CreateInstruction(0x8a, opcode, object, flags);
    }
    if (appendToBlock != 0) {
        PCode_AppendInstruction(gCurrentBlock, result);
    }
    return result;
}

void emit_opcode_with_base_offset(short opcode, short dest_reg, short base_reg, Object *obj, SInt32 offset)
{
    short address_reg;
    short offset_reg;
    short temp_reg;

    address_reg = base_reg;
    if (obj && obj->datatype != DLOCAL && PCodeUtilities_Require(obj) &&
        (base_reg == Register2 || base_reg == Register13))
        address_reg = 0;

    if (offset != (short)offset) {
        short high_offset;
        if (opcode == 0x22 && dest_reg == 12)
            offset_reg = 12;
        else if (opcode == 0x22 && dest_reg == 11)
            offset_reg = 11;
        else
            offset_reg = gUsedVirtualRegistersGPR++;
        high_offset = (offset >> 16) + ((offset >> 15) & 1);
        PCodeUtilities_EmitInstruction(PC_ADDIS, offset_reg, address_reg, 0, high_offset);
        offset = (short)offset;
        address_reg = offset_reg;
    }

    if (opcode == 0xfc || opcode == 0xf7) {
        offset_reg = 0;
        if (obj) {
            temp_reg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_ADDI, temp_reg, address_reg, obj, offset);
            address_reg = temp_reg;
        } else if (offset) {
            offset_reg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_LI, offset_reg, offset);
        }
        if (!offset_reg)
            PCodeUtilities_EmitInstruction(opcode, dest_reg, 0, address_reg);
        else
            PCodeUtilities_EmitInstruction(opcode, dest_reg, address_reg, offset_reg);
    } else {
        PCodeUtilities_EmitInstruction(opcode, dest_reg, address_reg, obj, offset);
    }
}

void fn_004a1cb0(int integerRegisterMask, int floatingRegisterMask, int vectorRegisterMask)
{
    int operandCount;
    PCodeInstruction *instruction;
    PCodeLabel *label;
    PCodeOperand *nextOperand;

    operandCount = 49;
    if (exception_scopes_enabled() && gCurrentStatement)
        operandCount += Exceptions_CountBoundObjectFields(gCurrentStatement->dobjstack);
    instruction = PCodeUtilities_CreateInstruction(PC_BLRL, operandCount, 0);
    nextOperand = PCodeUtilities_004a2290(instruction->operandData.operands + 1, integerRegisterMask,
                                          floatingRegisterMask, vectorRegisterMask);
    if (exception_scopes_enabled() && gCurrentStatement)
        Exceptions_CollectRegisterOperands(gCurrentStatement->dobjstack, nextOperand);
    if (copts.instructionSchedulingMode) {
        label = PCode_NewLabel();
        if (gCurrentBlock->instruction_count != 0) {
            PCode_AddSuccessor(gCurrentBlock, label);
            PCode_CreateBlock();
        }
        PCode_ResolveLabel(gCurrentBlock, label);
    }
    PCodeUtilities_EmitInstruction(PC_MTLR, 12);
    PCode_AppendInstruction(gCurrentBlock, instruction);
    label = PCode_NewLabel();
    if (gCurrentBlock->instruction_count != 0) {
        PCode_AddSuccessor(gCurrentBlock, label);
        PCode_CreateBlock();
    }
    PCode_ResolveLabel(gCurrentBlock, label);
    if (exception_scopes_enabled() && gCurrentStatement)
        Exceptions_AppendScopeEntry(instruction, gCurrentStatement->dobjstack);
}

void PCodeUtilities_LoadImmediate(SInt16 target, SInt32 value)
{
    SInt16 intermediate = target;

    if (value != (SInt16)value) {
        if (copts.deleteDeadInstructions > 1 && (SInt16)value != 0) {
            intermediate = gUsedVirtualRegistersGPR++;
        }
        PCodeUtilities_EmitInstruction(PC_LIS, intermediate, 0, (SInt16)((value >> 16) + (value >> 15 & 1)));
        if ((SInt16)value != 0) {
            PCodeUtilities_EmitInstruction(PC_ADDI, target, intermediate, 0, (SInt16)value);
        }
    } else {
        PCodeUtilities_EmitInstruction(PC_LI, target, value);
    }
}

void PCodeUtilities_EmitLoadImmediate(SInt16 destination, SInt32 value)
{
    SInt16 temporary = destination;
    SInt16 high;

    if (value != lowHalf(value)) {
        if (copts.deleteDeadInstructions > 1 && lowHalf(value) != 0) {
            temporary = gUsedVirtualRegistersGPR++;
        }
        high = value >> 16;
        PCodeUtilities_EmitInstruction(PC_LIS, temporary, 0, high);
        if (lowHalf(value) != 0) {
            PCodeUtilities_EmitInstruction(PC_ORI, destination, temporary, lowHalf(value));
        }
    } else {
        PCodeUtilities_EmitInstruction(PC_LI, destination, value);
    }
}
