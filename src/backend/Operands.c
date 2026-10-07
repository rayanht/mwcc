#define CERROR_FILE "Operands.c"
#include "compiler/common.h"
#include "compiler/Operands.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "compiler/TOC.h"
#include "compiler/ENode.h"
#include "compiler/Types.h"

enum { kRegOne = 1 };

#define Operands_NewReg(reg, avoid) (((reg) != 0 && (reg) != (avoid)) ? (reg) : gUsedVirtualRegistersGPR++)
#define Operands_NewReg0(reg) ((reg) != 0 ? (reg) : gUsedVirtualRegistersGPR++)

int Operands_IsLogicalExpression(ENode *enode)
{
    UInt8 type;

    while ((type = enode->type) == EFORCELOAD || type == ETYPCON || type == ECOMMA) {
        if (enode->rtype->type == TYPEINT || enode->rtype->type == TYPEENUM || enode->rtype->type == TYPEPOINTER ||
            (enode->rtype->type == TYPEMEMBERPOINTER && enode->rtype->size == 4)) {
            if (type == ECOMMA)
                enode = enode->data.diadic.right;
            else
                enode = enode->data.monadic;
        } else
            break;
    }
    if (type == ELOGNOT || type == ELAND || type == ELOR)
        return 1;
    return 0;
}

int fn_0049f630(ENode *expr)
{
    while (expr->type == EFORCELOAD || expr->type == ETYPCON || expr->type == ECOMMA) {
        if (!(expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM || expr->rtype->type == TYPEPOINTER ||
              (expr->rtype->type == TYPEMEMBERPOINTER && expr->rtype->size == 4)))
            break;
        if (expr->type == ECOMMA)
            expr = expr->data.diadic.right;
        else
            expr = expr->data.monadic;
    }
    if (expr->type >= 0x13 && expr->type <= 0x18)
        return 1;
    return 0;
}

void Operands_EmitAddress(SInt16 reg, Operand *operand)
{
    Operands_Normalize(operand);
    if (operand->kind == OpndType_IndirectGPR_ImmOffset) {
        if (operand->displacement == 0 && operand->object == NULL) {
            if (operand->reg != reg)
                PCodeUtilities_EmitInstruction(PC_MR, reg, operand->reg);
        } else {
            PCodeUtilities_EmitAddress(reg, operand->reg, operand->object, operand->displacement);
        }
    } else if (operand->kind == OpndType_IndirectGPR_Indexed) {
        PCodeUtilities_EmitInstruction(PC_ADD, reg, operand->reg, operand->secondary_reg);
    } else {
        CError_FATAL(1369);
    }
}

/* Register-bearing prefix of an operand record. */

unsigned int Operands_InsertBitField(unsigned short reg, Operand *operand, TypeBitfield *record)
{
    TypeBitfield adjusted;
    long shift;
    int end;
    int width;

    shift = 32;
    shift -= record->bitfieldtype->size << 3;
    shift += record->offset;
    width = record->bitlength;
    if (copts.nativeByteOrder != 0) {
        adjusted = *record;
        CABI_ReverseBitField(&adjusted);
        shift = 32 - (adjusted.bitfieldtype->size << 3) + adjusted.offset;
    }
    end = shift + width;
    return (unsigned int)PCodeUtilities_EmitInstruction(PC_RLWIMI, operand->reg, (short)reg, 32 - end, shift, end - 1);
}

/* Register operand prefix used by this instruction emitter. */

void Operands_ExtractBitfield(Operand *operand, TypeBitfield *tbitfield, SInt16 reg, Operand *result)
{
    SInt32 shift;
    SInt32 bitlength;
    SInt32 regno;
    TypeBitfield reversedType;
    SInt16 resultReg;

    shift = 0x20 - tbitfield->bitfieldtype->size * 8 + tbitfield->offset;
    bitlength = tbitfield->bitlength;
    if (reg != 0) {
        resultReg = reg;
    } else {
        resultReg = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR++;
    }
    regno = resultReg;
    if (copts.nativeByteOrder != 0) {
        reversedType = *tbitfield;
        tbitfield = &reversedType;
        CABI_ReverseBitField(&reversedType);
        shift = 0x20 - reversedType.bitfieldtype->size * 8 + reversedType.offset;
    }
    if (Type_IsUnsigned(tbitfield->bitfieldtype)) {
        PCodeUtilities_EmitInstruction(PC_RLWINM, regno, operand->reg, shift + bitlength, 0x20 - bitlength, 0x1f);
    } else {
        if (shift == 0) {
            PCodeUtilities_EmitInstruction(PC_SRAWI, regno, operand->reg, 0x20 - bitlength);
        } else {
            PCodeUtilities_EmitInstruction(PC_RLWINM, regno, operand->reg, shift, 0, bitlength);
            PCodeUtilities_EmitInstruction(PC_SRAWI, regno, regno, 0x20 - bitlength);
        }
    }
    result->kind = OpndType_GPR;
    result->reg = regno;
}

/* 0x58846e, word access */

void Operands_MoveToNewGPR(Operand *op, short outputReg)
{
    if (op->reg != kRegOne)
        PCodeUtilities_EmitInstruction(PC_FMR, 1, op->reg);
    PCodeUtilities_EmitObjectInstructionWithPayload(data_0058758c, 0, 0, 2, 0);
    op->kind = OpndType_GPR;
    op->reg = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_MR, op->reg, 3);
}

void Operands_ConvertFloatToInteger(Operand *operand, SInt16 reg)
{
    int resultReg;
    Object *temporary;
    int floatReg;

    Type *type = (Type *)&stdouble;
    temporary = CodeGen_AllocateTemporaryObject(type);
    floatReg = gUsedVirtualRegistersFPR++;
    PCodeUtilities_EmitInstruction(PC_FCTIWZ, floatReg, operand->reg);
    emit_opcode_with_base_offset(PC_STFD, floatReg, stack_base_reg, temporary, 0);
    resultReg = reg ? reg : gUsedVirtualRegistersGPR++;
    emit_opcode_with_base_offset(PC_LWZ, resultReg, stack_base_reg, temporary, low_word_offset);
    operand->kind = OpndType_GPR;
    operand->reg = resultReg;
}

/* Address operand representation used by instruction emission. */

static inline int fpr(Object *x)
{
    int v;
    emit_opcode_with_base_offset(PC_LFD, v = gUsedVirtualRegistersFPR++, stack_base_reg, x, 0);
    return v;
}
static inline SInt16 gpr(void)
{
    return gUsedVirtualRegistersGPR++;
}

static inline Object *start(Float *blk)
{
    Type *type = (Type *)&stdouble;
    Object *x = CodeGen_AllocateTemporaryObject(type);
    blk->data.words[0] = data_0055e9f0[0];
    blk->data.words[1] = data_0055e9f0[1];
    return x;
}

void Operands_ConvertIntegerToFloat(struct Operand *result, Boolean useOpcodeA5, SInt16 requestedRegister)
{
    Object *storage;
    Float block;
    SInt16 baseRegister;
    SInt32 integerRegister;
    SInt32 loadedRegister;
    PCodeInstruction *firstInstruction;
    Object *operand;
    PCodeInstruction *secondInstruction;
    SInt32 constantRegister;
    SInt32 memoryRegister;
    SInt32 resultRegister;
    storage = start(&block);
    operand = TOC_GetFloatObject(TYPE(&stdouble), &block);
    memoryRegister = constantRegister = gUsedVirtualRegistersFPR++;
    baseRegister = 2;
    if (operand->datatype == DDATA && PCodeUtilities_Require(operand) == 0) {
        baseRegister = gUsedVirtualRegistersGPR++;
        firstInstruction = PCodeUtilities_MakeInstructionWithObject(baseRegister, 0, operand, 0, 1);
        secondInstruction = PCodeUtilities_CreateInstructionWithObject(baseRegister, baseRegister, operand, 0, 1);
        if (copts.reuseSectionSymbols != 0) {
            firstInstruction->operandData.operands[1].flags = 0xb;
            secondInstruction->operandData.operands[2].flags = 0xa;
        }
        operand = NULL;
    }
    emit_opcode_with_base_offset(data_0055f622 == 4 ? 0x8e : 0x92, memoryRegister, baseRegister, operand, 0);
    emit_opcode_with_base_offset(PC_STW, result->reg, stack_base_reg, storage, low_word_offset);
    integerRegister = gpr();
    PCodeUtilities_EmitInstruction(PC_LIS, integerRegister, 0, 0x4330);
    emit_opcode_with_base_offset(PC_STW, integerRegister, stack_base_reg, storage, high_word_offset);
    loadedRegister = fpr(storage);
    resultRegister = requestedRegister != 0 ? requestedRegister : gUsedVirtualRegistersFPR++;
    PCodeUtilities_EmitInstruction(useOpcodeA5 ? 0xa5 : 0xa4, resultRegister, loadedRegister, constantRegister);
    result->kind = OpndType_FPR;
    result->reg = resultRegister;
}

static inline Object *prelude(Float *p)
{
    Type *type = (Type *)&stdouble;
    Object *r = CodeGen_AllocateTemporaryObject(type);
    p->data.words[0] = data_0055e9e8[0];
    p->data.words[1] = data_0055e9ec;
    return r;
}

/* Byte 53 of the instruction record returned by PCodeUtilities_CreateInstructionWithObject. */

void Operands_ConvertSignedIntegerToFloat(struct Operand *operand, char subtract, short resultReg)
{
    long immediateReg;
    short selectedReg;
    int outputReg;
    int allocatedFPR;
    short constantReg;
    int constantFPR;
    int loadFPR;
    int valueFPR;
    Float conversionConstant;
    short baseReg;
    Object *stackBase;
    PCodeInstruction *firstInfo;
    PCodeInstruction *secondInfo;
    int loadResult;
    Object *object;
    int integerReg;

    stackBase = prelude(&conversionConstant);
    object = TOC_GetFloatObject(TYPE(&stdouble), &conversionConstant);
    allocatedFPR = loadFPR = constantFPR = gUsedVirtualRegistersFPR++;
    baseReg = 2;
    if (object->datatype == DDATA && PCodeUtilities_Require(object) == 0) {
        baseReg = gUsedVirtualRegistersGPR++;
        firstInfo = PCodeUtilities_MakeInstructionWithObject(baseReg, 0, object, 0, 1);
        secondInfo = PCodeUtilities_CreateInstructionWithObject(baseReg, baseReg, object, 0, 1);
        if (copts.reuseSectionSymbols != 0) {
            firstInfo->operandData.operands[1].flags = 11;
            secondInfo->operandData.operands[2].flags = 10;
        }
        object = NULL;
    }
    emit_opcode_with_base_offset(data_0055f622 == 4 ? 142 : 146, loadFPR, baseReg, object, 0);
    integerReg = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_XORIS, integerReg, operand->reg, 32768);
    emit_opcode_with_base_offset(PC_STW, integerReg, stack_base_reg, stackBase, low_word_offset);
    constantReg = gUsedVirtualRegistersGPR++;
    immediateReg = constantReg;
    PCodeUtilities_EmitInstruction(PC_LIS, immediateReg, 0, 17200);
    emit_opcode_with_base_offset(PC_STW, immediateReg, stack_base_reg, stackBase, high_word_offset);
    emit_opcode_with_base_offset(PC_LFD, loadResult = valueFPR = gUsedVirtualRegistersFPR++, stack_base_reg, stackBase,
                                 0);
    if (resultReg)
        selectedReg = resultReg;
    else
        selectedReg = gUsedVirtualRegistersFPR++;
    outputReg = selectedReg;
    PCodeUtilities_EmitInstruction(subtract != 0 ? 165 : 164, outputReg, valueFPR, constantFPR);
    operand->kind = OpndType_FPR;
    operand->reg = outputReg;
}

/* Code records expose two byte-sized attributes; the remaining bytes are opaque. */

void Operands_EmitOpcodeWithObjectBaseOffset(short dest, Type *type, Object *obj)
{
    short base;
    PCodeInstruction *highCode;
    PCodeInstruction *lowCode;

    base = 2;
    if (obj->datatype == DDATA && !PCodeUtilities_Require(obj)) {
        base = gUsedVirtualRegistersGPR++;
        highCode = PCodeUtilities_MakeInstructionWithObject(base, 0, obj, 0, 1);
        lowCode = PCodeUtilities_CreateInstructionWithObject(base, base, obj, 0, 1);
        if (copts.reuseSectionSymbols) {
            highCode->operandData.operands[1].flags = 0xb;
            lowCode->operandData.operands[2].flags = 0xa;
        }
        obj = NULL;
    }
    emit_opcode_with_base_offset((type->size == 4) ? 0x8e : 0x92, dest, base, obj, 0);
}

void Operands_ExtendGPR(Operand *operand, Type *type, short requestedReg)
{
    int resultReg;
    int alreadyExtended = operand->kind >= 9;
    short byteReg;
    short signedByteReg;
    short halfwordReg;
    if (operand->kind != OpndType_GPR)
        Operands_ForceGPR(operand, type, requestedReg);
    switch (type->size) {
        case 1:
            if (Type_IsUnsigned(type) != 0) {
                if (alreadyExtended)
                    return;
                if (requestedReg != 0)
                    byteReg = requestedReg;
                else {
                    byteReg = gUsedVirtualRegistersGPR;
                    gUsedVirtualRegistersGPR += 1;
                }
                resultReg = byteReg;
                ((unsigned int (*)(unsigned int, ...))PCodeUtilities_EmitInstruction)(103, byteReg, operand->reg, 0, 24,
                                                                                      31);
                break;
            }
            if (requestedReg != 0)
                signedByteReg = requestedReg;
            else {
                signedByteReg = gUsedVirtualRegistersGPR;
                gUsedVirtualRegistersGPR += 1;
            }
            resultReg = signedByteReg;
            ((unsigned int (*)(unsigned int, ...))PCodeUtilities_EmitInstruction)(100, signedByteReg, operand->reg);
            break;
        case 2:
            if (alreadyExtended)
                return;
            if (requestedReg != 0)
                halfwordReg = requestedReg;
            else {
                halfwordReg = gUsedVirtualRegistersGPR;
                gUsedVirtualRegistersGPR += 1;
            }
            resultReg = halfwordReg;
            if (Type_IsUnsigned(type) != 0) {
                ((unsigned int (*)(unsigned int, ...))PCodeUtilities_EmitInstruction)(103, resultReg, operand->reg, 0,
                                                                                      16, 31);
                break;
            }
            ((unsigned int (*)(unsigned int, ...))PCodeUtilities_EmitInstruction)(101, resultReg, operand->reg);
            break;
        default:
            CError_FATAL(1063);
    }
    operand->kind = OpndType_GPR;
    operand->reg = resultReg;
}

void Operands_EmitSTVX(SInt16 reg, Operand *operand, Type *type)
{
    Operands_Normalize(operand);
    switch (operand->kind) {
        case OpndType_IndirectGPR_ImmOffset:
            emit_opcode_with_base_offset(PC_STVX, reg, operand->reg, operand->object, operand->displacement);
            Operands_AllocateGPR(operand->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            PCodeUtilities_EmitInstruction(PC_STVX, reg, operand->reg, operand->secondary_reg);
            Operands_AllocateGPR(operand->flags);
            break;
        default:
            CError_FATAL(1018);
            break;
    }
}

void Operands_EmitGPRMemoryInstruction(SInt16 reg, Operand *node, Type *type)
{
    SInt16 opcode;

    if (copts.operandsDebug && type->type == TYPEFLOAT)
        PPCError_FatalError(0x84);

    Operands_Normalize(node);

    switch (node->kind) {
        case OpndType_IndirectGPR_ImmOffset:
            if (type->size == 4)
                opcode = 0x96;
            else
                opcode = 0x9a;
            emit_opcode_with_base_offset(opcode, reg, node->reg, node->object, node->displacement);
            Operands_AllocateGPR(node->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            if (type->size == 4)
                opcode = 0x98;
            else
                opcode = 0x9c;
            PCodeUtilities_EmitInstruction(opcode, reg, node->reg, node->secondary_reg);
            Operands_AllocateGPR(node->flags);
            break;
        default:
            CError_FATAL(994);
            break;
    }
}

/* low word offset */

void Operands_StoreGPRPair(SInt16 reg, SInt16 regHi, Operand *op, Type *type)
{
    SInt16 tmp;

    if (!(((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
          (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4)))
        CError_FATAL(943);

    Operands_Normalize(op);

    switch (op->kind) {
        case OpndType_IndirectGPR_ImmOffset:
            emit_opcode_with_base_offset(PC_STW, reg, op->reg, op->object, op->displacement + low_word_offset);
            Operands_AllocateGPR(op->flags);
            emit_opcode_with_base_offset(PC_STW, regHi, op->reg, op->object, op->displacement + high_word_offset);
            Operands_AllocateGPR(op->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            tmp = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_ADD, tmp, op->reg, op->secondary_reg);
            emit_opcode_with_base_offset(PC_STW, reg, tmp, NULL, low_word_offset);
            Operands_AllocateGPR(op->flags);
            emit_opcode_with_base_offset(PC_STW, regHi, tmp, NULL, high_word_offset);
            Operands_AllocateGPR(op->flags);
            break;
        default:
            CError_FATAL(963);
            break;
    }
}

void Operands_EmitTypedGPRMemoryInstruction(short reg, Operand *opnd, Type *type)
{
    short op;

    Operands_Normalize(opnd);
    switch (opnd->kind) {
        case OpndType_IndirectGPR_ImmOffset:
            op = 0x31;
            if (type->type == TYPEINT || type->type == TYPEENUM) {
                switch (type->size) {
                    case 1:
                        op = 0x28;
                        break;
                    case 2:
                        op = 0x2c;
                        break;
                }
            } else if (type->type == TYPEPOINTER) {
            } else if (type->type == TYPEMEMBERPOINTER && type->size == 4) {
            } else if (copts.operandsDebug && type->type == TYPEFLOAT && type->size == 4) {
            } else {
                CError_FATAL(906);
            }
            emit_opcode_with_base_offset(op, reg, opnd->reg, opnd->object, opnd->displacement);
            Operands_AllocateGPR(opnd->flags);
            break;

        case OpndType_IndirectGPR_Indexed:
            op = 0x33;
            if (type->type == TYPEINT || type->type == TYPEENUM) {
                switch (type->size) {
                    case 1:
                        op = 0x2a;
                        break;
                    case 2:
                        op = 0x2e;
                        break;
                }
            } else if (type->type == TYPEPOINTER) {
            } else if (type->type == TYPEMEMBERPOINTER && type->size == 4) {
            } else if (copts.operandsDebug && type->type == TYPEFLOAT && type->size == 4) {
            } else {
                CError_FATAL(923);
            }
            PCodeUtilities_EmitInstruction(op, reg, opnd->reg, opnd->secondary_reg);
            Operands_AllocateGPR(opnd->flags);
            break;

        default:
            CError_FATAL(928);
    }
}

void Operands_ForceVR(Operand *operand, Type *type, short targetReg)
{
    short resultReg;
    TypeStruct *operandType;
    Operands_Normalize(operand);
    switch (operand->kind) {
        case OpndType_VR:
            resultReg = operand->reg;
            break;
        case OpndType_IndirectGPR_ImmOffset:
            resultReg = targetReg != 0 ? targetReg : gUsedVirtualRegistersVR++;
            emit_opcode_with_base_offset(PC_LVX, resultReg, operand->reg, operand->object, operand->displacement);
            Operands_AllocateGPR(operand->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            targetReg = targetReg ? targetReg : gUsedVirtualRegistersVR++;
            resultReg = targetReg;
            PCodeUtilities_EmitInstruction(PC_LVX, targetReg, operand->reg, operand->secondary_reg);
            Operands_AllocateGPR(operand->flags);
            break;
        case OpndType_Immediate:
            operandType = (TypeStruct *)type;
            targetReg = targetReg ? targetReg : gUsedVirtualRegistersVR++;
            resultReg = targetReg;
            switch (operandType->stype) {
                case IT_WCHAR_T:
                case IT_SHORT:
                case IT_USHORT:
                    PCodeUtilities_EmitInstruction(PC_VSPLTISB, targetReg, operand->immediate);
                    break;
                case IT_INT:
                case IT_UINT:
                case IT_LONG:
                case IT_SHORTDOUBLE:
                    PCodeUtilities_EmitInstruction(PC_VSPLTISH, targetReg, operand->immediate);
                    break;
                case IT_ULONG:
                case IT_LONGLONG:
                case IT_ULONGLONG:
                case IT_FLOAT:
                    PCodeUtilities_EmitInstruction(PC_VSPLTISW, targetReg, operand->immediate);
                    break;
                default:
                    CError_FATAL(863);
            }
            operand->kind = OpndType_VR;
            operand->reg = targetReg;
            Operands_AllocateGPR(operand->flags);
            break;
        default:
            CError_FATAL(873);
    }
    operand->kind = OpndType_VR;
    operand->reg = resultReg;
}

void Operands_ForceFPR(Operand *operand, Type *type, short requestedReg)
{
    short resultReg;
    short memoryReg;
    int memoryOpcode;
    short indexedReg;
    int indexedOpcode;
    if (copts.operandsDebug != 0 && type->type == TYPEFLOAT) {
        PPCError_FatalError(132);
    }
    Operands_Normalize(operand);
    switch (operand->kind) {
        case OpndType_FPR:
            resultReg = operand->reg;
            break;
        case OpndType_IndirectGPR_ImmOffset:
            memoryReg = requestedReg ? requestedReg : gUsedVirtualRegistersFPR++;
            resultReg = memoryReg;
            memoryOpcode = type->size == 4 ? 142 : 146;
            emit_opcode_with_base_offset(memoryOpcode, memoryReg, operand->reg, operand->object, operand->displacement);
            Operands_AllocateGPR(operand->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            indexedReg = requestedReg ? requestedReg : gUsedVirtualRegistersFPR++;
            resultReg = indexedReg;
            indexedOpcode = type->size == 4 ? 144 : 148;
            ((void (*)(short, ...))PCodeUtilities_EmitInstruction)(indexedOpcode, indexedReg, operand->reg,
                                                                   operand->secondary_reg);
            Operands_AllocateGPR(operand->flags);
            break;
        default:
            CError_FATAL(800);
    }
    operand->kind = OpndType_FPR;
    operand->reg = resultReg;
}

static inline SInt16 Operands_SignedLowHalf(SInt32 value)
{
    return value;
}

void Operands_ForceGPRPair(Operand *op, Type *type, SInt16 first, SInt16 second)
{
    SInt16 secondRegister = -1;
    SInt16 firstRegister;
    SInt32 lowHalf;
    SInt32 value;

    CError_ASSERT(633, ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                           (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4));
    Operands_Normalize(op);
    switch (op->kind) {
        case OpndType_GPRPair: {
            SInt16 targetFirst;
            SInt16 targetSecond;
            if (first != 0 && second == 0)
                second = gUsedVirtualRegistersGPR++;
            if (second != 0 && first == 0)
                first = gUsedVirtualRegistersGPR++;
            if (op->reg != first || op->regHi != second) {
                if (first)
                    targetFirst = first;
                else
                    targetFirst = op->reg;
                if (second)
                    targetSecond = second;
                else
                    targetSecond = op->regHi;
                if (targetFirst != op->reg) {
                    if (targetFirst == op->regHi) {
                        if (targetFirst == targetSecond)
                            CError_FATAL(657);
                        PCodeUtilities_EmitInstruction(PC_MR, targetSecond, op->regHi);
                        PCodeUtilities_EmitInstruction(PC_MR, targetFirst, op->reg);
                    } else {
                        PCodeUtilities_EmitInstruction(PC_MR, targetFirst, op->reg);
                        if (op->regHi != targetSecond)
                            PCodeUtilities_EmitInstruction(PC_MR, targetSecond, op->regHi);
                    }
                } else if (targetSecond != op->regHi) {
                    if (targetSecond == op->reg) {
                        if (targetFirst == targetSecond)
                            CError_FATAL(671);
                        PCodeUtilities_EmitInstruction(PC_MR, targetFirst, op->reg);
                        PCodeUtilities_EmitInstruction(PC_MR, targetSecond, op->regHi);
                    } else {
                        PCodeUtilities_EmitInstruction(PC_MR, targetSecond, op->regHi);
                        if (op->reg != targetFirst)
                            PCodeUtilities_EmitInstruction(PC_MR, targetFirst, op->reg);
                    }
                }
            }
            firstRegister = op->reg;
            secondRegister = op->regHi;
            break;
        }
        case OpndType_GPR:
            CError_FATAL(688);
            break;
        case OpndType_GPR_ImmOffset:
            CError_FATAL(691);
            break;
        case OpndType_GPR_Indexed:
            CError_FATAL(694);
            break;
        case OpndType_Immediate: {
            SInt16 targetRegister;
            SInt16 highHalf;
            if (first != 0)
                targetRegister = first;
            else
                targetRegister = gUsedVirtualRegistersGPR++;
            firstRegister = targetRegister;
            value = op->immediate;
            if (value == (lowHalf = Operands_SignedLowHalf(value))) {
                PCodeUtilities_EmitInstruction(PC_LI, targetRegister, value);
            } else {
                secondRegister = targetRegister;
                if (copts.deleteDeadInstructions > 1 && Operands_SignedLowHalf(value) != 0)
                    secondRegister = gUsedVirtualRegistersGPR++;
                highHalf = (value >> 16) + ((value >> 15) & 1);
                PCodeUtilities_EmitInstruction(PC_LIS, secondRegister, 0, highHalf);
                if (Operands_SignedLowHalf(value) != 0)
                    PCodeUtilities_EmitInstruction(PC_ADDI, targetRegister, secondRegister, 0, lowHalf);
            }
            if (second != 0)
                targetRegister = second;
            else
                targetRegister = gUsedVirtualRegistersGPR++;
            secondRegister = targetRegister;
            if (Type_IsUnsigned(type) || value >= 0)
                PCodeUtilities_EmitInstruction(PC_LI, targetRegister, 0);
            else
                PCodeUtilities_EmitInstruction(PC_LI, targetRegister, -1);
            break;
        }
        case OpndType_IndirectGPR_ImmOffset: {
            SInt16 targetFirst;
            SInt16 targetSecond;
            if (first != 0)
                targetFirst = first;
            else
                targetFirst = gUsedVirtualRegistersGPR++;
            firstRegister = targetFirst;
            if (second != 0)
                targetSecond = second;
            else
                targetSecond = gUsedVirtualRegistersGPR++;
            secondRegister = targetSecond;
            if (op->reg == targetSecond) {
                if (op->reg == targetFirst) {
                    CError_FATAL(726);
                    break;
                }
                emit_opcode_with_base_offset(PC_LWZ, targetFirst, op->reg, op->object,
                                             op->displacement + low_word_offset);
                Operands_AllocateGPR(op->flags);
                emit_opcode_with_base_offset(PC_LWZ, targetSecond, op->reg, op->object,
                                             op->displacement + high_word_offset);
                Operands_AllocateGPR(op->flags);
            } else {
                emit_opcode_with_base_offset(PC_LWZ, targetSecond, op->reg, op->object,
                                             op->displacement + high_word_offset);
                Operands_AllocateGPR(op->flags);
                emit_opcode_with_base_offset(PC_LWZ, targetFirst, op->reg, op->object,
                                             op->displacement + low_word_offset);
                Operands_AllocateGPR(op->flags);
            }
            break;
        }
        case OpndType_IndirectGPR_Indexed: {
            SInt16 targetFirst;
            SInt16 targetSecond;
            if (first != 0)
                targetFirst = first;
            else
                targetFirst = gUsedVirtualRegistersGPR++;
            firstRegister = targetFirst;
            if (second != 0)
                targetSecond = second;
            else
                targetSecond = gUsedVirtualRegistersGPR++;
            secondRegister = targetSecond;
            PCodeUtilities_EmitInstruction(PC_ADD, targetFirst, op->reg, op->secondary_reg);
            emit_opcode_with_base_offset(PC_LWZ, targetSecond, targetFirst, NULL, high_word_offset);
            Operands_AllocateGPR(op->flags);
            emit_opcode_with_base_offset(PC_LWZ, targetFirst, targetFirst, NULL, low_word_offset);
            Operands_AllocateGPR(op->flags);
            break;
        }
        default:
            CError_FATAL(751);
            break;
    }
    if (secondRegister == -1) {
        CError_FATAL(755);
    } else {
        op->kind = OpndType_GPRPair;
        op->reg = firstRegister;
        op->regHi = secondRegister;
    }
}

static __inline SInt32 Operands_GetOpcode(Type *type, SInt32 base, SInt32 line)
{
    SInt32 op = base;

    if (type->type == TYPEINT || type->type == TYPEENUM) {
        switch (type->size) {
            case 1:
                op = base - 0xd;
                break;
            case 2:
                if (Type_IsUnsigned(type))
                    op = base - 9;
                else
                    op = base - 5;
                break;
        }
    } else if (type->type != TYPEPOINTER && (type->type != TYPEMEMBERPOINTER || type->size != 4) &&
               !(copts.operandsDebug && type->type == TYPEFLOAT && type->size == 4)) {
        CError_Internal(CERROR_FILE, line);
    }
    return op;
}

void Operands_ForceGPR(Operand *node, Type *type, SInt16 reg)
{
    SInt16 targetReg;
    SInt16 highReg;
    int value;
    UInt8 kind;

    if ((((kind = type->type) == TYPEINT || kind == TYPEENUM) && type->size == 8) ||
        (copts.operandsDebug && kind == TYPEFLOAT && type->size != 4)) {
        Operands_ForceGPRPair(node, type, reg, 0);
        return;
    }

    Operands_Normalize(node);

    switch (node->kind) {
        case OpndType_GPRPair:
            return;
        case OpndType_GPR:
            return;
        case OpndType_GPR_ImmOffset:
            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitAddress(targetReg, node->reg, node->object, node->displacement);
            break;
        case OpndType_GPR_Indexed:
            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_ADD, targetReg, node->reg, node->secondary_reg);
            break;
        case OpndType_Immediate:
            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            value = node->immediate;
            if (value == (SInt16)value) {
                PCodeUtilities_EmitInstruction(PC_LI, targetReg, value);
            } else {
                highReg = targetReg;
                if (copts.deleteDeadInstructions > 1 && (SInt16)value != 0)
                    highReg = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_LIS, highReg, 0, (SInt16)((value >> 16) + ((value >> 15) & 1)));
                if ((SInt16)value != 0)
                    PCodeUtilities_EmitInstruction(PC_ADDI, targetReg, highReg, 0, (SInt16)value);
            }
            break;
        case OpndType_IndirectGPR_ImmOffset:
            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(Operands_GetOpcode(type, PC_LWZ, 0x224), targetReg, node->reg, node->object,
                                         node->displacement);
            Operands_AllocateGPR(node->flags);
            break;
        case OpndType_IndirectGPR_Indexed:
            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(Operands_GetOpcode(type, PC_LWZX, 0x23a), targetReg, node->reg,
                                           node->secondary_reg);
            Operands_AllocateGPR(node->flags);
            break;
        case OpndType_CRField: {
            SInt16 invert = 0;
            SInt32 bitOffset = 0;
            SInt16 bitIndex;

            targetReg = reg ? reg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MFCR, targetReg);
            switch (node->secondary_reg) {
                case 0x18:
                    invert = 1;
                case 0x17:
                    bitOffset = 2;
                    break;
                case 0x16:
                    invert = 1;
                case 0x13:
                    bitOffset = 0;
                    break;
                case 0x15:
                    invert = 1;
                case 0x14:
                    bitOffset = 1;
                    break;
                default:
                    CError_FATAL(601);
            }
            bitIndex = node->reg;
            bitIndex <<= 2;
            bitIndex += bitOffset;
            PCodeUtilities_EmitInstruction(PC_RLWINM, targetReg, targetReg, bitIndex + 1, 0x1f, 0x1f);
            if (invert)
                PCodeUtilities_EmitInstruction(PC_XORI, targetReg, targetReg, 1);
            break;
        }
        default:
            CError_FATAL(612);
    }

    node->kind = OpndType_GPR;
    node->reg = targetReg;
}

void Operands_Normalize(Operand *op)
{
    SInt16 indirect = 0;
    SInt16 frame;
    Object *object = op->object;

    switch (op->kind) {
        case OpndType_IndirectSymbol:
            indirect = 1;
        case OpndType_Symbol:
            if (object->datatype == DLOCAL) {
                op->kind = OpndType_GPR_ImmOffset;
                op->reg = stack_base_reg;
                op->object = object;
            } else if (object->datatype == DABSOLUTE) {
                UInt32 address = object->u.data.u.intconst.hi;
                if (address == (SInt16)address) {
                    op->reg = 0;
                } else {
                    PCodeUtilities_EmitInstruction(PC_LIS, op->reg = gUsedVirtualRegistersGPR++, 0,
                                                   (SInt16)((address >> 16) + ((address >> 15) & 1)));
                }
                op->displacement = object->u.data.u.intconst.hi;
                op->object = object;
                op->kind = OpndType_GPR_ImmOffset;
            } else {
                frame = ObjGen_PPC_EABI_MapObjectSectionCode(object);
                if (!PCodeUtilities_Require(object)) {
                    SInt16 addressReg = gUsedVirtualRegistersGPR++;
                    SInt16 resultReg = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_MakeInstructionWithObject(addressReg, 0, object, 0, 1);
                    PCodeUtilities_CreateInstructionWithObject(resultReg, addressReg, object, 0, 1);
                    frame = resultReg;
                    op->object = NULL;
                } else if (frame == 0) {
                    frame = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitAddress(frame, 0, object, 0);
                    op->object = NULL;
                }
                op->kind = OpndType_GPR_ImmOffset;
                op->reg = frame;
            }
            if (indirect) {
                if (op->kind == OpndType_GPR_ImmOffset)
                    op->kind = OpndType_IndirectGPR_ImmOffset;
                else
                    CError_FATAL(456);
            }
            break;
        case OpndType_GPR:
        case OpndType_GPR_ImmOffset:
        case OpndType_GPR_Indexed:
        case OpndType_GPRPair:
        case OpndType_Immediate:
        case OpndType_VR:
        case OpndType_CRField:
        case OpndType_IndirectGPR_ImmOffset:
        case OpndType_IndirectGPR_Indexed:
            break;
        default:
            CError_FATAL(474);
    }
}

/* Operand descriptor used by the P-code address generator.  Field offsets are
 * the ones observed in the original: type 0, reg 2, reg2 6, offset 8,
 * disp 0xe, object 0x12. */

void Operands_Add(Operand *left, Operand *right, SInt16 hint, Operand *dest)
{
    Operand *swap;
    SInt32 addressReg;
    UInt8 resultKind;
    SInt16 highImmediate;

    if (left->kind == OpndType_Symbol || left->kind == OpndType_IndirectSymbol)
        Operands_Normalize(left);
    if (right->kind == OpndType_Symbol || right->kind == OpndType_IndirectSymbol)
        Operands_Normalize(right);

    switch (left->kind * 11 + right->kind) {
        case 0: /* (0,0) */
            dest->kind = OpndType_GPR_Indexed;
            dest->reg = left->reg;
            dest->secondary_reg = right->reg;
            break;

        case 12: /* (1,1) */
            if (left->displacement + right->displacement == (SInt16)(left->displacement + right->displacement) &&
                (left->object == NULL || right->object == NULL)) {
                right->displacement += left->displacement;
                if (right->object == NULL)
                    right->object = left->object;
            } else {
                addressReg = Operands_NewReg(hint, right->reg);
                PCodeUtilities_EmitAddress(addressReg, left->reg, left->object, left->displacement);
                left->reg = addressReg;
            }
            /* fall through */
        case 1: /* (0,1) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 11: /* (1,0) */
            if (left->reg == stack_base_reg) {
                dest->kind = OpndType_GPR_Indexed;
                dest->reg = Operands_NewReg(hint, right->reg);
                dest->secondary_reg = right->reg;
                PCodeUtilities_EmitAddress(dest->reg, left->reg, left->object, left->displacement);
            } else if (right->reg == stack_base_reg) {
                dest->kind = OpndType_GPR_Indexed;
                dest->reg = Operands_NewReg(hint, left->reg);
                dest->secondary_reg = left->reg;
                PCodeUtilities_EmitAddress(dest->reg, right->reg, left->object, left->displacement);
            } else if (left->object != NULL) {
                dest->kind = OpndType_GPR_Indexed;
                dest->reg = Operands_NewReg(hint, right->reg);
                dest->secondary_reg = right->reg;
                PCodeUtilities_EmitAddress(dest->reg, left->reg, left->object, left->displacement);
            } else {
                dest->kind = OpndType_GPR_ImmOffset;
                dest->reg = Operands_NewReg0(hint);
                dest->displacement = left->displacement;
                dest->object = left->object;
                PCodeUtilities_EmitInstruction(PC_ADD, dest->reg, left->reg, right->reg);
            }
            break;

        case 2: /* (0,2) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 22: /* (2,0) */
            dest->kind = OpndType_GPR_Indexed;
            dest->reg = left->reg;
            dest->secondary_reg = Operands_NewReg(hint, left->reg);
            PCodeUtilities_EmitInstruction(PC_ADD, dest->secondary_reg, left->secondary_reg, right->reg);
            break;

        case 13: /* (1,2) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 23: /* (2,1) */
            if (right->object != NULL) {
                dest->kind = OpndType_GPR_Indexed;
                dest->reg = Operands_NewReg(hint, right->reg);
                dest->secondary_reg = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADD, dest->reg, left->reg, left->secondary_reg);
                PCodeUtilities_EmitAddress(dest->secondary_reg, right->reg, right->object, right->displacement);
            } else {
                dest->kind = OpndType_GPR_ImmOffset;
                dest->displacement = right->displacement;
                dest->object = right->object;
                dest->reg = Operands_NewReg(hint, right->reg);
                PCodeUtilities_EmitInstruction(PC_ADD, dest->reg, left->reg, left->secondary_reg);
                PCodeUtilities_EmitInstruction(PC_ADD, dest->reg, dest->reg, right->reg);
            }
            break;

        case 24: /* (2,2) */
            dest->kind = OpndType_GPR_Indexed;
            dest->reg = left->reg;
            dest->secondary_reg = Operands_NewReg(hint, left->secondary_reg);
            PCodeUtilities_EmitInstruction(PC_ADD, dest->secondary_reg, right->reg, right->secondary_reg);
            PCodeUtilities_EmitInstruction(PC_ADD, dest->secondary_reg, dest->secondary_reg, left->secondary_reg);
            break;

        case 15: /* (1,4) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 45: /* (4,1) */
            if (right->object == NULL) {
                dest->kind = OpndType_GPR_ImmOffset;
                dest->reg = right->reg;
                dest->displacement = right->displacement;
                dest->object = right->object;
                if (dest->displacement + left->immediate == (SInt16)(dest->displacement + left->immediate)) {
                    dest->displacement += left->immediate;
                } else {
                    dest->reg = Operands_NewReg0(hint);
                    if ((SInt16)((left->immediate >> 16) + ((left->immediate >> 15) & 1)) == 0) {
                        PCodeUtilities_EmitInstruction(PC_ADDI, dest->reg, right->reg, 0, (SInt16)left->immediate);
                    } else {
                        highImmediate = (left->immediate >> 16) + ((left->immediate >> 15) & 1);
                        PCodeUtilities_EmitInstruction(PC_ADDIS, dest->reg, right->reg, 0, highImmediate);
                        if (dest->displacement + (SInt16)left->immediate ==
                            (SInt16)(dest->displacement + (SInt16)left->immediate))
                            dest->displacement += left->immediate;
                        else
                            PCodeUtilities_EmitInstruction(PC_ADDI, dest->reg, dest->reg, 0, (SInt16)left->immediate);
                    }
                }
                break;
            }
            if (right->object->datatype == DLOCAL) {
                dest->kind = OpndType_GPR_ImmOffset;
                dest->object = right->object;
                dest->reg = right->reg;
                dest->displacement = right->displacement + left->immediate;
                break;
            }
            dest->reg = Operands_NewReg0(hint);
            PCodeUtilities_EmitAddress(dest->reg, right->reg, right->object, right->displacement);
            right->kind = OpndType_GPR;
            right->reg = dest->reg;
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 4: /* (0,4) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 44: /* (4,0) */
            if ((SInt16)left->immediate != 0)
                resultKind = OpndType_GPR_ImmOffset;
            else
                resultKind = OpndType_GPR;
            dest->kind = resultKind;
            dest->displacement = left->immediate;
            dest->object = NULL;
            if (left->immediate == (SInt16)left->immediate) {
                dest->reg = right->reg;
            } else {
                dest->reg = Operands_NewReg0(hint);
                highImmediate = (left->immediate >> 16) + ((left->immediate >> 15) & 1);
                PCodeUtilities_EmitInstruction(PC_ADDIS, dest->reg, right->reg, 0, highImmediate);
            }
            break;

        case 26: /* (2,4) */
            swap = left;
            left = right;
            right = swap;
            /* fall through */
        case 46: /* (4,2) */
            dest->kind = OpndType_GPR_Indexed;
            dest->reg = right->reg;
            dest->secondary_reg = Operands_NewReg(hint, right->reg);
            if ((SInt16)((left->immediate >> 16) + ((left->immediate >> 15) & 1)) == 0) {
                PCodeUtilities_EmitInstruction(PC_ADDI, dest->secondary_reg, right->secondary_reg, 0,
                                               (SInt16)left->immediate);
            } else {
                highImmediate = (left->immediate >> 16) + ((left->immediate >> 15) & 1);
                PCodeUtilities_EmitInstruction(PC_ADDIS, dest->secondary_reg, right->secondary_reg, 0, highImmediate);
                if ((SInt16)left->immediate != 0)
                    PCodeUtilities_EmitInstruction(PC_ADDI, dest->secondary_reg, dest->secondary_reg, 0,
                                                   (SInt16)left->immediate);
            }
            break;

        case 48: /* (4,4) */
            dest->kind = OpndType_Immediate;
            dest->immediate = left->immediate + right->immediate;
            break;

        default:
            CError_FATAL(362);
            break;
    }
}

/* Operand record of the PPC code generator: byte kind, word register,
 * word low half of the constant, qual flags, 32-bit value, spare. */

static void Operands_SetKind(Operand *op, UInt8 kind)
{
    op->kind = kind;
    CError_ASSERT(92, op != NULL);
}

static void Operands_SetQual(Operand *op, ENode *e)
{
    if (e != NULL) {
        if (e->type == EINTCONST) {
            op->flags = 0;
            if (e->flags & ENODE_FLAG_VOLATILE)
                op->flags |= fIsVolatile;
            if (e->flags & ENODE_FLAG_CONST)
                op->flags |= fIsConst;
        } else {
            op->flags = CParserIsVolatileExpr(e) ? fIsVolatile : 0;
            op->flags |= CParserIsConstExpr(e) ? fIsConst : 0;
        }
    } else {
        op->flags = 0;
    }
}

void Operands_MakeIndirect(Operand *op, ENode *e)
{
    switch (op->kind) {
        case OpndType_GPRPair:
            CError_FATAL(125);
        case OpndType_CRField:
        case OpndType_IndirectGPR_ImmOffset:
        case OpndType_IndirectGPR_Indexed:
        case OpndType_IndirectSymbol:
            if (op->kind != OpndType_GPR)
                Operands_ForceGPR(op, TYPE(&void_ptr), 0);
        case OpndType_GPR:
            op->displacement = 0;
            op->object = NULL;
        case OpndType_GPR_ImmOffset:
            Operands_SetKind(op, OpndType_IndirectGPR_ImmOffset);
            Operands_SetQual(op, e);
            break;
        case OpndType_GPR_Indexed:
            Operands_SetKind(op, OpndType_IndirectGPR_Indexed);
            Operands_SetQual(op, e);
            break;
        case OpndType_Immediate:
            if (op->immediate == (SInt16)op->immediate) {
                op->reg = 0;
                op->displacement = op->immediate;
            } else {
                PCodeUtilities_EmitInstruction(PC_LIS, op->reg = gUsedVirtualRegistersGPR++, 0,
                                               (SInt16)((op->immediate >> 16) + ((op->immediate >> 15) & 1)));
                op->displacement = op->immediate;
            }
            op->object = NULL;
            Operands_SetKind(op, OpndType_IndirectGPR_ImmOffset);
            Operands_SetQual(op, e);
            break;
        case OpndType_Symbol:
            Operands_SetKind(op, OpndType_IndirectSymbol);
            Operands_SetQual(op, e);
            break;
        default:
            CError_FATAL(162);
    }
}
