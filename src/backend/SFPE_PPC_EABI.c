#define CERROR_FILE "SFPE_PPC_EABI.c"
#include "compiler/common.h"
#include "version.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CParser.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"

struct Object *data_00587584;
struct Object *data_005875ac;
struct Object *data_005875bc;
struct Object *data_005875f4;
#if VERSION > VERSION_GC_1_0
short return_gpr_first;
SInt16 sfpe_right_operand_reg;
short returnRegHi;
SInt16 sfpe_right_operand_reg_hi;
#endif

/* r3, where a runtime routine takes its argument and returns its result */
enum { RESULT_GPR = 3 };

typedef void (*ExpressionGenerator)(ENode *, short, short, Operand *);

typedef enum ConversionTypeKind {
    ConversionTypeKind1 = TYPEINT,
    ConversionTypeKind2 = TYPEFLOAT,
    ConversionTypeKind3 = TYPEENUM,
    ConversionTypeKind11 = TYPEPOINTER
} ConversionTypeKind;

#if VERSION == VERSION_GC_1_0
/* Moves srcA into dstA and srcB into dstB, first copying srcB elsewhere when the move into dstA would overwrite it;
 * returns where srcB ends up. */
static SInt16 move_register_pair(SInt16 srcA, SInt16 dstA, SInt16 srcB, SInt16 dstB)
{
    SInt16 temporary;

    if (srcA != dstA) {
        if (srcB == dstA) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, srcB);
            srcB = temporary;
        }
        PCodeUtilities_EmitInstruction(PC_MR, dstA, srcA);
    }
    if (srcB != dstB)
        PCodeUtilities_EmitInstruction(PC_MR, dstB, srcB);
    return srcB;
}

/* Puts the left pair in r3:r4 and the right pair in r5:r6, the arguments of a double runtime routine. */
static void load_argument_pairs(Operand *left, Operand *right)
{
    SInt16 temporary;

    if (left->regHi != 3) {
        if (left->reg == 3) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, left->reg);
            left->reg = temporary;
        } else if (right->reg == 3) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, right->reg);
            right->reg = temporary;
        } else if (right->regHi == 3) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, right->regHi);
            right->regHi = temporary;
        }
        PCodeUtilities_EmitInstruction(PC_MR, 3, left->regHi);
    }
    if (left->reg != 4) {
        if (right->reg == 4) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, right->reg);
            right->reg = temporary;
        } else if (right->regHi == 4) {
            temporary = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, temporary, right->regHi);
            right->regHi = temporary;
        }
        PCodeUtilities_EmitInstruction(PC_MR, 4, left->reg);
    }
    right->reg = move_register_pair(right->regHi, 5, right->reg, 6);
}
#endif

static inline void lift_result(Operand *op, int kind)
{
    op->kind = kind;
    op->reg = gUsedVirtualRegistersGPR;
    ++gUsedVirtualRegistersGPR;
    if (kind == 0) {
        PCodeUtilities_EmitInstruction(PC_MR, op->reg, 3);
    } else {
        op->regHi = gUsedVirtualRegistersGPR;
        ++gUsedVirtualRegistersGPR;
        PCodeUtilities_EmitInstruction(PC_MR, (int)op->reg, return_gpr_first);
        PCodeUtilities_EmitInstruction(PC_MR, (int)op->regHi, returnRegHi);
    }
}

static PCodeInstruction *first_instr(struct PCodeLabel *q)
{
    PCodeBlock *n;
    for (n = q->target.block; n->instruction_count == 0; n = n->next)
        ;
    return n->instructions;
}

enum { Register3 = 3, Register4 = 4 };

static inline void LoadFloatArg(int reg)
{
    union {
        float f;
        SInt32 l;
    } u;

    u.f = float_one.data.value;
    PCodeUtilities_EmitLoadImmediate(reg, u.l);
    if (reg != 4)
        PCodeUtilities_EmitInstruction(PC_MR, 4, reg);
}

static inline void LoadFloatArgGPR(int reg)
{
    LoadFloatArg(reg);
}

static inline void LoadDoubleArg(int reg1, int reg2)
{
    union {
        Float f;
        CInt64 i;
    } v;

    v.f = float_one;
    PCodeUtilities_EmitLoadImmediate(reg2, v.i.lo);
    PCodeUtilities_EmitLoadImmediate(reg1, v.i.hi);
    if (reg2 != sfpe_right_operand_reg_hi)
        PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg_hi, reg2);
    if (reg1 != sfpe_right_operand_reg)
        PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg, reg1);
}

#if VERSION == VERSION_GC_1_0
/* (1.0 loads the constant 1.0 straight into r5:r6, in the target's byte order) */
static inline void LoadDoubleArgPair(void)
{
    union {
        Float f;
        CInt64 i;
    } v;

    v.f = float_one;
    if (copts.littleendian) {
        PCodeUtilities_EmitLoadImmediate(5, v.i.hi);
        PCodeUtilities_EmitLoadImmediate(6, v.i.lo);
    } else {
        PCodeUtilities_EmitLoadImmediate(5, v.i.lo);
        PCodeUtilities_EmitLoadImmediate(6, v.i.hi);
    }
}
#endif

void SFPE_PPC_EABI_GenerateDiadicArithmetic(ENode *node, SInt16 requestedReg, SInt16 requestedRegHi, Operand *result)
{
    ENode *left;
    Operand leftOperand;
    Operand rightOperand;
    ENode *right;

    left = node->data.diadic.left;
    right = node->data.diadic.right;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (node->rtype->size == 4) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 129 : 187, rightOperand.kind == OpndType_GPR);

        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 136 : 194, leftOperand.kind == OpndType_GPR);

#if VERSION == VERSION_GC_1_0
        rightOperand.reg = move_register_pair(leftOperand.reg, 3, rightOperand.reg, 4);
#else
        {
            SInt32 reg = leftOperand.reg;
            if (reg != 3)
                PCodeUtilities_EmitInstruction(PC_MR, 3, reg);
        }
        {
            SInt32 reg = rightOperand.reg;
            if (reg != 4)
                PCodeUtilities_EmitInstruction(PC_MR, 4, reg);
        }
#endif

        if (node->type == EADD) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_005875bc, 0, 0x18, 0, 0);
        } else if (node->type == ESUB) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_005875ac, 0, 0x18, 0, 0);
        } else if (node->type == EMUL) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_005875f0, 0, 0x18, 0, 0);
        } else if (node->type == EDIV) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_005875e8, 0, 0x18, 0, 0);
        } else {
            CError_FATAL(VERSION > VERSION_GC_1_0 ? 158 : 213);
        }

        result->kind = OpndType_GPR;
        result->reg = gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_MR, result->reg, 3);
    } else {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 174 : 229, rightOperand.kind == OpndType_GPRPair);

        data_00560648[left->type](left, 0, 0, &leftOperand);
#if VERSION == VERSION_GC_1_0
        Operands_ForceGPRPair(&leftOperand, left->rtype, 4, 3);
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 181 : 236, leftOperand.kind == OpndType_GPRPair);
        load_argument_pairs(&leftOperand, &rightOperand);
#else
        Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 181 : 236, leftOperand.kind == OpndType_GPRPair);

        if (leftOperand.regHi != returnRegHi)
            PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, leftOperand.regHi);
        if (leftOperand.reg != return_gpr_first)
            PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, leftOperand.reg);
        if (rightOperand.regHi != sfpe_right_operand_reg_hi)
            PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg_hi, rightOperand.regHi);
        if (rightOperand.reg != sfpe_right_operand_reg)
            PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg, rightOperand.reg);
#endif

        if (node->type == EADD) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_005875f4, 0, 0x78, 0, 0);
        } else if (node->type == ESUB) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_00587584, 0, 0x78, 0, 0);
        } else if (node->type == EMUL) {
            PCodeUtilities_EmitObjectInstructionWithPayload(data_0058762c, 0, 0x78, 0, 0);
        } else if (node->type == EDIV) {
            if (((left->rtype->type == TYPEINT || left->rtype->type == TYPEENUM) && left->rtype->size == 8 &&
                 is_unsigned(left->rtype)) ||
                ((right->rtype->type == TYPEINT || right->rtype->type == TYPEENUM) && right->rtype->size == 8 &&
                 is_unsigned(right->rtype))) {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 206 : 254);
                PCodeUtilities_EmitObjectInstructionWithPayload(data_0058761c, 0, 0x78, 0, 0);
            } else {
                PCodeUtilities_EmitObjectInstructionWithPayload(data_0058761c, 0, 0x78, 0, 0);
            }
        } else {
            CError_FATAL(VERSION > VERSION_GC_1_0 ? 214 : 262);
        }

        result->kind = OpndType_GPRPair;
        result->reg = gUsedVirtualRegistersGPR++;
        result->regHi = gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_MR, result->reg, return_gpr_first);
        PCodeUtilities_EmitInstruction(PC_MR, result->regHi, returnRegHi);
    }
}

#if VERSION == VERSION_GC_1_0
void SFPE_PPC_EABI_MakeIntvalOperand(ENode *node, SInt16 reg1, SInt16 reg2, Operand *op)
{
    CError_FATAL(286);
    reg1 = reg1 ? reg1 : gUsedVirtualRegistersGPR++;
    reg2 = reg2 ? reg2 : gUsedVirtualRegistersGPR++;
    PCodeUtilities_LoadImmediate(reg1, node->data.intval.lo);
    PCodeUtilities_LoadImmediate(reg2, node->data.intval.hi);
    op->kind = OpndType_GPRPair;
    op->reg = reg1;
    op->regHi = reg2;
}
#endif

void SFPE_PPC_EABI_LoadFloatConstant(ENode *node, SInt16 regA, SInt16 regB, Operand *out)
{
    if (node->rtype->size == 4) {
        union {
            float f;
            SInt32 l;
        } u;
        SInt16 reg1;
#if VERSION == VERSION_GC_1_0
        u.f = node->data.floatval.data.value;
#endif
        if (regA != 0)
            reg1 = regA;
        else
            reg1 = gUsedVirtualRegistersGPR++;
#if VERSION > VERSION_GC_1_0
        u.f = node->data.floatval.data.value;
#endif
        PCodeUtilities_EmitLoadImmediate(reg1, u.l);
        out->kind = OpndType_GPR;
        out->reg = reg1;
    } else {
        SInt16 reg1;
        SInt16 reg2;
        CInt64 v;
#if VERSION == VERSION_GC_1_0
        v = node->data.intval;
#endif
        if (regA != 0)
            reg1 = regA;
        else
            reg1 = gUsedVirtualRegistersGPR++;
        if (regB != 0)
            reg2 = regB;
        else
            reg2 = gUsedVirtualRegistersGPR++;
#if VERSION > VERSION_GC_1_0
        v = node->data.intval;
#endif
#if VERSION == VERSION_GC_1_0
        if (copts.littleendian) {
            PCodeUtilities_EmitLoadImmediate(reg2, v.hi);
            PCodeUtilities_EmitLoadImmediate(reg1, v.lo);
        } else
#endif
        {
            PCodeUtilities_EmitLoadImmediate(reg2, v.lo);
            PCodeUtilities_EmitLoadImmediate(reg1, v.hi);
        }
        out->kind = OpndType_GPRPair;
        out->reg = reg1;
        out->regHi = reg2;
    }
}

void SFPE_PPC_EABI_ToggleHighBit(ENode *expr, SInt16 lowReg, SInt16 highReg, Operand *result)
{
    Operand operand;
    ENode *subexpr;

    subexpr = expr->data.monadic;
    memclrw(&operand, sizeof(operand));
    if (expr->rtype->size == 4) {
        SInt32 resultReg;
        data_00560648[subexpr->type](subexpr, 0, 0, &operand);
        if (operand.kind)
            Operands_ForceGPR(&operand, subexpr->rtype, 0);
        resultReg = lowReg ? lowReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_XORIS, resultReg, operand.reg, 0x8000);
        result->kind = OpndType_GPR;
        result->reg = resultReg;
    } else {
        SInt32 resultLowReg, resultHighReg;
        data_00560648[subexpr->type](subexpr, 0, 0, &operand);
        Operands_ForceGPRPair(&operand, subexpr->rtype, 0, 0);
        resultHighReg = highReg ? highReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_XORIS, resultHighReg, operand.regHi, 0x8000);
        resultLowReg = lowReg ? lowReg : gUsedVirtualRegistersGPR++;
        if (resultLowReg != operand.reg)
            PCodeUtilities_EmitInstruction(PC_MR, resultLowReg, operand.reg);
        result->kind = OpndType_GPRPair;
        result->reg = resultLowReg;
        result->regHi = resultHighReg;
    }
}

void SFPE_PPC_EABI_GenerateAssignment(ENode *g, SInt16 requestedRegister, SInt16 requestedRegisterPair, Operand *res)
{
    ENode *destination;
    ENode *value;
    UInt8 typeKind;
    Type *valueType;
    Operand destinationLoc;
    Operand valueLoc;
    Operand unusedLoc;
    SInt32 adjustment;
    VarInfo *info;

    destination = (ENode *)g->data.diadic.left;
    value = (ENode *)g->data.diadic.right;
    valueType = g->rtype;

    memclrw(&destinationLoc, sizeof(destinationLoc));
    memclrw(&valueLoc, sizeof(valueLoc));
    memclrw(&unusedLoc, sizeof(unusedLoc));

    if (destination->type == EOBJREF &&
        ((Registers_GetInfo(destination->data.objref)) ? (Registers_GetInfo(destination->data.objref))->reg : 0) != 0) {
        info = Registers_GetInfo(destination->data.objref);
        data_00560648[value->type](value, info->reg, info->regHi, &valueLoc);
        if (info->is_fpr != 0) {
            if (valueType->size == 4) {
                if (valueLoc.kind != OpndType_GPR)
                    Operands_ForceGPR(&valueLoc, valueType, info->reg);
                if (valueLoc.reg != info->reg)
                    PCodeUtilities_EmitInstruction(PC_MR, info->reg, valueLoc.reg);
                res->kind = OpndType_GPR;
                res->reg = info->reg;
            } else {
                Operands_ForceGPRPair(&valueLoc, valueType, info->reg, info->regHi);
                if (valueLoc.reg != info->reg)
                    PCodeUtilities_EmitInstruction(PC_MR, info->reg, valueLoc.reg);
                if (valueLoc.regHi != info->regHi)
                    PCodeUtilities_EmitInstruction(PC_MR, info->regHi, valueLoc.regHi);
                res->kind = OpndType_GPRPair;
                res->reg = info->reg;
                res->regHi = info->regHi;
            }
        } else {
            PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 375 : 462);
            CError_FATAL(VERSION > VERSION_GC_1_0 ? 376 : 464);
            Operands_ForceGPRPair(&valueLoc, valueType, info->reg, info->regHi);
            if (valueLoc.reg != info->reg)
                PCodeUtilities_EmitInstruction(PC_MR, info->reg, valueLoc.reg);
            if (valueLoc.regHi != info->regHi)
                PCodeUtilities_EmitInstruction(PC_MR, info->regHi, valueLoc.regHi);
            res->kind = OpndType_GPRPair;
            res->reg = info->reg;
            res->regHi = info->regHi;
        }
    } else if ((typeKind = valueType->type) == TYPEFLOAT) {
        data_00560648[value->type](value, 0, 0, &valueLoc);
        if (valueType->size == 4) {
            if (valueLoc.kind != OpndType_GPR)
                Operands_ForceGPR(&valueLoc, value->rtype, 0);
            if (InstrSelection_MatchPostIncDecRegister(destination, &destinationLoc, &adjustment) != 0) {
                Operands_MakeIndirect(&destinationLoc, destination);
                Operands_EmitTypedGPRMemoryInstruction(valueLoc.reg, &destinationLoc, valueType);
                InstrSelection_EmitAddImmediate(destinationLoc.reg, destinationLoc.reg, adjustment);
            } else {
                data_00560648[destination->type](destination, 0, 0, &destinationLoc);
                Operands_MakeIndirect(&destinationLoc, destination);
                Operands_EmitTypedGPRMemoryInstruction(valueLoc.reg, &destinationLoc, valueType);
            }
            res->kind = OpndType_GPR;
            res->reg = valueLoc.reg;
        } else {
            Operands_ForceGPRPair(&valueLoc, value->rtype, 0, 0);
            CError_ASSERT(VERSION > VERSION_GC_1_0 ? 413 : 501, destination->type != EBITFIELD);
            if (InstrSelection_MatchPostIncDecRegister(destination, &destinationLoc, &adjustment) != 0) {
                Operands_MakeIndirect(&destinationLoc, destination);
                Operands_StoreGPRPair(valueLoc.reg, valueLoc.regHi, &destinationLoc, valueType);
                InstrSelection_EmitAddImmediate(destinationLoc.reg, destinationLoc.reg, adjustment);
            } else {
                data_00560648[destination->type](destination, 0, 0, &destinationLoc);
                Operands_MakeIndirect(&destinationLoc, destination);
                Operands_StoreGPRPair(valueLoc.reg, valueLoc.regHi, &destinationLoc, valueType);
            }
            res->kind = OpndType_GPRPair;
            res->reg = valueLoc.reg;
            res->regHi = valueLoc.regHi;
        }
    } else if (typeKind == TYPEINT || typeKind == TYPEENUM || typeKind == TYPEPOINTER ||
               (typeKind == TYPEMEMBERPOINTER && valueType->size == 4)) {
        CError_FATAL(VERSION > VERSION_GC_1_0 ? 437 : 525);
        data_00560648[value->type](value, 0, 0, &valueLoc);
        Operands_ForceGPRPair(&valueLoc, value->rtype, 0, 0);
        if (destination->type == EBITFIELD) {
            CError_FATAL(VERSION > VERSION_GC_1_0 ? 442 : 530);
        } else {
            data_00560648[destination->type](destination, 0, 0, &destinationLoc);
            Operands_MakeIndirect(&destinationLoc, destination);
            Operands_StoreGPRPair(valueLoc.reg, valueLoc.regHi, &destinationLoc, valueType);
        }
        res->kind = OpndType_GPRPair;
        res->reg = valueLoc.reg;
        res->regHi = valueLoc.regHi;
    }
}

void SFPE_PPC_EABI_GenerateFloatPostIncDec(ENode *expr, SInt16 outputReg, SInt16 outputReg2, Operand *result)
{
    ENode *inner;
    Type *type;
    int sourceReg;
    int sourceReg2;
    int argumentReg;
    int argumentReg2;
    Operand operand;
    Operand value;

    inner = expr->data.monadic->data.monadic;
    type = expr->rtype;
    memclrw(&operand, sizeof(operand));
    memclrw(&value, sizeof(value));

    if (type->size == 4) {
        if (inner->type == EOBJREF &&
            (sourceReg = Registers_GetInfo(inner->data.objref) ? Registers_GetInfo(inner->data.objref)->reg : 0) != 0) {
            Registers_GetInfo(inner->data.objref);
            result->kind = OpndType_GPR;
            result->reg = (outputReg != 0 && outputReg != sourceReg) ? outputReg : gUsedVirtualRegistersGPR++;
            CError_ASSERT(488, sourceReg != result->reg);
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, sourceReg);
            if (sourceReg != 3)
                PCodeUtilities_EmitInstruction(PC_MR, 3, sourceReg);
#if VERSION == VERSION_GC_1_0
            LoadFloatArg(4);
#else
            argumentReg = gUsedVirtualRegistersGPR++;
            LoadFloatArg(argumentReg);
#endif
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875bc, 0, 0x18, 0, 0);
            else
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875ac, 0, 0x18, 0, 0);
            PCodeUtilities_EmitInstruction(PC_MR, sourceReg, 3);
        } else {
            if (inner->type == EBITFIELD)
                CError_FATAL(515);
            data_00560648[inner->type](inner, 0, 0, &operand);
            Operands_MakeIndirect(&operand, inner);
            value = operand;
            if (value.kind != OpndType_GPR)
                Operands_ForceGPR(&value, type, VERSION == VERSION_GC_1_0 ? 3 : 0);
            result->kind = OpndType_GPR;
            result->reg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, value.reg);
            if (value.reg != Register3)
                PCodeUtilities_EmitInstruction(PC_MR, 3, value.reg);
#if VERSION == VERSION_GC_1_0
            LoadFloatArg(4);
#else
            argumentReg = gUsedVirtualRegistersGPR++;
            LoadFloatArgGPR(argumentReg);
#endif
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875bc, 0, 0x18, 0, 0);
            else
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875ac, 0, 0x18, 0, 0);
            Operands_EmitTypedGPRMemoryInstruction(3, &operand, type);
        }
    } else {
        if (inner->type == EOBJREF &&
            (sourceReg = Registers_GetInfo(inner->data.objref) ? Registers_GetInfo(inner->data.objref)->reg : 0) != 0) {
            sourceReg2 = Registers_GetInfo(inner->data.objref)->regHi;
            result->kind = OpndType_GPRPair;
            result->regHi = (outputReg2 != 0 && (outputReg2 != sourceReg2 && outputReg2 != sourceReg))
                                ? outputReg2
                                : gUsedVirtualRegistersGPR++;
            result->reg = (outputReg != 0 && (outputReg != sourceReg && outputReg != sourceReg2))
                              ? outputReg
                              : gUsedVirtualRegistersGPR++;
            CError_ASSERT(556, sourceReg != result->reg && sourceReg2 != result->reg);
            CError_ASSERT(557, sourceReg != result->regHi && sourceReg2 != result->regHi);
            PCodeUtilities_EmitInstruction(PC_MR, result->regHi, sourceReg2);
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, sourceReg);
            if (sourceReg2 != returnRegHi)
                PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, sourceReg2);
            if (sourceReg != return_gpr_first)
                PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, sourceReg);
#if VERSION == VERSION_GC_1_0
            LoadDoubleArgPair();
#else
            argumentReg = gUsedVirtualRegistersGPR++;
            argumentReg2 = gUsedVirtualRegistersGPR++;
            LoadDoubleArg(argumentReg, argumentReg2);
#endif
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875f4, 0, 0x78, 0, 0);
            else
                PCodeUtilities_EmitObjectInstructionWithPayload(data_00587584, 0, 0x78, 0, 0);
            PCodeUtilities_EmitInstruction(PC_MR, sourceReg2, returnRegHi);
            PCodeUtilities_EmitInstruction(PC_MR, sourceReg, return_gpr_first);
        } else {
            if (inner->type == EBITFIELD)
                CError_FATAL(591);
            data_00560648[inner->type](inner, 0, 0, &operand);
            Operands_MakeIndirect(&operand, inner);
            value = operand;
#if VERSION == VERSION_GC_1_0
            Operands_ForceGPRPair(&value, type, 4, 3);
#else
            Operands_ForceGPRPair(&value, type, 0, 0);
#endif
            result->kind = OpndType_GPRPair;
            result->reg = gUsedVirtualRegistersGPR++;
            result->regHi = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, value.reg);
            PCodeUtilities_EmitInstruction(PC_MR, result->regHi, value.regHi);
#if VERSION == VERSION_GC_1_0
            value.reg = move_register_pair(value.regHi, 3, value.reg, 4);
#else
            if (value.regHi != returnRegHi)
                PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, value.regHi);
            if (value.reg != return_gpr_first)
                PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, value.reg);
#endif
#if VERSION == VERSION_GC_1_0
            LoadDoubleArgPair();
#else
            argumentReg = gUsedVirtualRegistersGPR++;
            argumentReg2 = gUsedVirtualRegistersGPR++;
            LoadDoubleArg(argumentReg, argumentReg2);
#endif
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875f4, 0, 0x78, 0, 0);
            else
                PCodeUtilities_EmitObjectInstructionWithPayload(data_00587584, 0, 0x78, 0, 0);
            Operands_StoreGPRPair(return_gpr_first, returnRegHi, &operand, type);
        }
    }
}

void SFPE_PPC_EABI_GenerateMonadicOperand(ENode *input, short firstRegister, short secondRegister, Operand *result)
{
    ENode *node = input;
    Type *type = node->rtype;
    ENode *expression = (ENode *)node->data.monadic;
    VarInfo *registerInfo;
    unsigned char kind;
    Operand temporary;
    SInt32 value;

    memclrw(&temporary, 22);
    if (expression->type == EOBJREF &&
        (Registers_GetInfo(expression->data.objref) ? (Registers_GetInfo(expression->data.objref))->reg : 0) != 0) {
        registerInfo = Registers_GetInfo(expression->data.objref);
        if (registerInfo->is_fpr) {
            if (type->size == 4) {
                result->kind = OpndType_GPR;
                result->reg = registerInfo->reg;
                result->object = NULL;
            } else {
                result->kind = OpndType_GPRPair;
                result->reg = registerInfo->reg;
                result->regHi = registerInfo->regHi;
                result->object = NULL;
            }
        } else {
            CError_FATAL(VERSION > VERSION_GC_1_0 ? 662 : 778);
            result->kind = OpndType_GPRPair;
            result->reg = registerInfo->reg;
            result->regHi = registerInfo->regHi;
            result->object = NULL;
        }
    } else if (expression->type == EBITFIELD) {
        CError_FATAL(VERSION > VERSION_GC_1_0 ? 674 : 790);
    } else if (InstrSelection_MatchPostIncDecRegister(expression, &temporary, &value) &&
               (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                (type->type == TYPEMEMBERPOINTER && type->size == 4) || type->type == TYPEFLOAT)) {
        Operands_MakeIndirect(&temporary, expression);
        *result = temporary;
        if (type->size == 8) {
            Operands_ForceGPRPair(result, (Type *)type, firstRegister, secondRegister);
            InstrSelection_EmitAddImmediate(temporary.reg, temporary.reg, value);
        } else {
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, type, firstRegister);
            InstrSelection_EmitAddImmediate(temporary.reg, temporary.reg, value);
        }
    } else {
        kind = expression->type;
        data_00560648[kind]((ENode *)expression, 0, 0, result);
        Operands_MakeIndirect(result, expression);
    }
}

/* node->type is held in an enum-typed local: the compiler spills an enum
 * with a 4-byte store where a UInt8 local would use a byte store. */

int SFPE_PPC_EABI_GenerateComparison(ENode *node, Operand *result, int branch)
{
    int isWide;
    ENode *left;
    ENode *right;
    Operand leftOperand, rightOperand, tempOperand;
    SInt32 size;
    SInt32 condition;
    SInt32 flags;
    SInt16 reg;
    int registerMask;
    ENodeType comparison;

    left = node->data.diadic.left;
    right = node->data.diadic.right;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));
    memclrw(&tempOperand, sizeof(tempOperand));

    size = right->rtype->size;
    if (left->rtype->size > size)
        size = left->rtype->size;
    if (size < 4 || size > 8)
        CError_FATAL(VERSION > VERSION_GC_1_0 ? 729 : 919);

    if (right->hascall != 0) {
        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (right->rtype->size < 4) {
            PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 737 : 927);
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        }
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        if (right->rtype->size < size) {
            reg = gUsedVirtualRegistersGPR++;
            if (is_unsigned(right->rtype)) {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 744 : 934);
                PCodeUtilities_LoadImmediate(reg, 0);
            } else {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 747 : 937);
                PCodeUtilities_EmitInstruction(PC_SRAWI, reg, rightOperand.reg, 0x1f);
            }
            rightOperand.kind = OpndType_GPRPair;
            rightOperand.regHi = reg;
        }

        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (left->rtype->size < 4) {
            PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 755 : 945);
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        }
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < size) {
            reg = gUsedVirtualRegistersGPR++;
            if (is_unsigned(right->rtype)) {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 762 : 952);
                PCodeUtilities_LoadImmediate(reg, 0);
            } else {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 765 : 955);
                PCodeUtilities_EmitInstruction(PC_SRAWI, reg, leftOperand.reg, 0x1f);
            }
            leftOperand.kind = OpndType_GPRPair;
            leftOperand.regHi = reg;
        }
    } else {
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < 4) {
            PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 777 : 967);
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        }
        if (left->rtype->size < size) {
            reg = gUsedVirtualRegistersGPR++;
            if (is_unsigned(right->rtype)) {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 783 : 973);
                PCodeUtilities_LoadImmediate(reg, 0);
            } else {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 786 : 976);
                PCodeUtilities_EmitInstruction(PC_SRAWI, reg, leftOperand.reg, 0x1f);
            }
            leftOperand.kind = OpndType_GPRPair;
            leftOperand.regHi = reg;
        }

        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (right->rtype->size < 4) {
            PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 794 : 984);
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        }
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        if (right->rtype->size < size) {
            reg = gUsedVirtualRegistersGPR++;
            if (is_unsigned(right->rtype)) {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 801 : 991);
                PCodeUtilities_LoadImmediate(reg, 0);
            } else {
                PPCError_ReportError(0x82, CERROR_FILE, VERSION > VERSION_GC_1_0 ? 804 : 994);
                PCodeUtilities_EmitInstruction(PC_SRAWI, reg, rightOperand.reg, 0x1f);
            }
            rightOperand.kind = OpndType_GPRPair;
            rightOperand.regHi = reg;
        }
    }

    if (size == 8)
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 813 : 1003,
                      leftOperand.kind == OpndType_GPRPair && rightOperand.kind == OpndType_GPRPair);
    if (size == 4)
        CError_ASSERT(VERSION > VERSION_GC_1_0 ? 815 : 1005,
                      leftOperand.kind == OpndType_GPR && rightOperand.kind == OpndType_GPR);

    if (size == 8) {
        registerMask = 0x78;
#if VERSION == VERSION_GC_1_0
        load_argument_pairs(&leftOperand, &rightOperand);
#else
        if (leftOperand.regHi != returnRegHi)
            PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, leftOperand.regHi);
        if (leftOperand.reg != return_gpr_first)
            PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, leftOperand.reg);
        if (rightOperand.regHi != sfpe_right_operand_reg_hi)
            PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg_hi, rightOperand.regHi);
        if (rightOperand.reg != sfpe_right_operand_reg)
            PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg, rightOperand.reg);
#endif
    } else {
        registerMask = 0x18;
#if VERSION == VERSION_GC_1_0
        rightOperand.reg = move_register_pair(leftOperand.reg, 3, rightOperand.reg, 4);
#else
        if ((SInt32)leftOperand.reg != 3)
            PCodeUtilities_EmitInstruction(PC_MR, 3, leftOperand.reg);
        if ((SInt32)rightOperand.reg != 4)
            PCodeUtilities_EmitInstruction(PC_MR, 4, rightOperand.reg);
#endif
    }

    comparison = node->type;
    switch (comparison) {
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
            if (comparison == EEQU) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_0058760c : data_005875d8, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else if (comparison == ENOTEQU) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_00587604 : data_005875d4, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else if (comparison == EGREATER) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_00587610 : data_005875dc, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else if (comparison == ELESS) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_00587618 : data_005875e4, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else if (comparison == ELESSEQU) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_005875fc : data_005875c8, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else if (comparison == EGREATEREQU) {
                PCodeUtilities_EmitObjectInstructionWithPayload(size == 8 ? data_00587628 : data_005875ec, 0,
                                                                registerMask, 0, 0);
                condition = 0x17;
                flags = 1;
            } else {
                CError_FATAL(VERSION > VERSION_GC_1_0 ? 881 : 1063);
            }

            if (branch != 0) {
                result->kind = OpndType_GPR;
                result->reg = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_MR, result->reg, 3);
            } else {
                PCodeUtilities_EmitInstruction(PC_CMPI, 0, 3, flags);
                result->kind = OpndType_CRField;
                result->reg = 0;
                result->secondary_reg = condition;
            }

            isWide = size == 8;
            break;
        default:
            isWide = CError_FATAL(VERSION > VERSION_GC_1_0 ? 904 : 1086);
            break;
    }
    return isWide;
}

void SFPE_PPC_EABI_GenerateConversion(ENode *expression, short requested_register, short requested_pair_register,
                                      Operand *result)
{
    Type *type;
    ENode *node;
    Type *target_type;

    node = expression->data.monadic;
    type = node->rtype;
    target_type = expression->rtype;
    if (expression->rtype->type == TYPEVOID) {
        PPCError_ReportError(0x82, "SFPE_PPC_EABI.c", 0x39f);
        data_00560648[node->type](node, 0, 0, result);
        result->kind = OpndType_Immediate;
        result->immediate = 0;
    } else {
        ConversionTypeKind kind;
        kind = type->type;
        if ((kind == TYPEINT) || (kind == TYPEENUM)) {
            if ((copts.operandsDebug != 0) && (target_type->type == TYPEFLOAT) &&
                ((kind == TYPEINT || kind == TYPEENUM) && type->size == 8)) {
                data_00560648[node->type](node, 0, 0, result);
#if VERSION == VERSION_GC_1_0
                Operands_ForceGPRPair(result, (Type *)type, 4, 3);
                result->reg = move_register_pair(result->regHi, 3, result->reg, 4);
#else
                Operands_ForceGPRPair(result, (Type *)type, 0, 0);
                if (result->kind != OpndType_GPRPair) {
                    CError_FATAL(946);
                }
                if (result->regHi != returnRegHi) {
                    PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, result->regHi);
                }
                if (result->reg != return_gpr_first) {
                    PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, result->reg);
                }
#endif
                PCodeUtilities_EmitObjectInstructionWithPayload(
                    is_unsigned(type) ? (target_type->size == 4 ? data_00587f9c : data_00587f50)
                                      : (target_type->size == 4 ? data_00588210 : data_00588250),
                    0, 0x18, 0, 0);
                if (target_type->size == 4) {
                    lift_result(result, 0);
                } else {
                    lift_result(result, 3);
                }
            } else if ((copts.operandsDebug != 0) && (target_type->type == TYPEFLOAT)) {
                data_00560648[node->type](node, 0, 0, result);
                if (type->size < 4) {
                    Operands_ExtendGPR(result, type, 0);
                }
                if (result->kind != OpndType_GPR) {
                    Operands_ForceGPR(result, type, 0);
                }
                if (result->reg != RESULT_GPR) {
                    PCodeUtilities_EmitInstruction(PC_MR, 3, result->reg);
                }
                PCodeUtilities_EmitObjectInstructionWithPayload(
                    is_unsigned(type) ? (target_type->size == 4 ? data_00587e34 : data_00587e5c)
                                      : (target_type->size == 4 ? data_00587ec0 : data_00587e90),
                    0, 8, 0, 0);
                if (target_type->size == 4) {
                    lift_result(result, 0);
                } else {
                    lift_result(result, 3);
                }
            } else {
                CError_FATAL(1021);
            }
        } else if (kind == TYPEPOINTER) {
            CError_FATAL(1030);
        } else if ((copts.operandsDebug != 0) && (target_type->type == TYPEFLOAT)) {
            if ((target_type->size == 4) && (type->size != 4)) {
                data_00560648[node->type](node, 0, 0, result);
#if VERSION == VERSION_GC_1_0
                Operands_ForceGPRPair(result, (Type *)type, 4, 3);
                result->reg = move_register_pair(result->regHi, 3, result->reg, 4);
#else
                Operands_ForceGPRPair(result, (Type *)type, 0, 0);
                if (result->kind != OpndType_GPRPair) {
                    CError_FATAL(1049);
                }
                if (result->regHi != returnRegHi) {
                    PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, result->regHi);
                }
                if (result->reg != return_gpr_first) {
                    PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, result->reg);
                }
#endif
                PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e9c, 0, 0x18, 0, 0);
                lift_result(result, 0);
            } else if ((type->size == 4) && (target_type->size != 4)) {
                data_00560648[node->type](node, 0, 0, result);
                if (result->kind != OpndType_GPR) {
                    Operands_ForceGPR(result, type, 0);
                }
                if (result->reg != RESULT_GPR) {
                    PCodeUtilities_EmitInstruction(PC_MR, 3, result->reg);
                }
                PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e6c, 0, 8, 0, 0);
                lift_result(result, 3);
            } else {
                if (target_type->size != type->size) {
                    CError_FATAL(1088);
                }
                data_00560648[node->type](node, requested_register, requested_pair_register, result);
                if ((type->size == 4) && (result->kind != OpndType_GPR)) {
                    if (result->kind != OpndType_GPR) {
                        Operands_ForceGPR(result, type, requested_register);
                    }
                    return;
                }
                if ((type->size != 4) && (result->kind != OpndType_GPRPair)) {
                    Operands_ForceGPRPair(result, (Type *)type, requested_register, requested_pair_register);
                }
            }
        } else {
            ConversionTypeKind target_kind;
            target_kind = target_type->type;
            if ((((target_kind == TYPEINT) || (target_kind == TYPEENUM)) && (target_type->size == 8)) &&
                (((copts.operandsDebug != 0) && (kind == TYPEFLOAT)) && (type->size != 4))) {
                data_00560648[node->type](node, 0, 0, result);
#if VERSION == VERSION_GC_1_0
                Operands_ForceGPRPair(result, (Type *)type, 4, 3);
                result->reg = move_register_pair(result->regHi, 3, result->reg, 4);
#else
                Operands_ForceGPRPair(result, (Type *)type, 0, 0);
                if (result->kind != OpndType_GPRPair) {
                    CError_FATAL(1112);
                }
                if (result->regHi != returnRegHi) {
                    PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, result->regHi);
                }
                if (result->reg != return_gpr_first) {
                    PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, result->reg);
                }
#endif
                if (is_unsigned(expression->rtype)) {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00588068, 0, 0x18, 0, 0);
                } else {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_0058823c, 0, 0x18, 0, 0);
                }
                lift_result(result, 3);
            } else if (((target_kind == TYPEINT) || (target_kind == TYPEENUM)) &&
                       ((target_type->size == 8 &&
                         (((copts.operandsDebug != 0) && (kind == TYPEFLOAT)) && (type->size == 4))))) {
                data_00560648[node->type](node, 0, 0, result);
                if (result->kind != OpndType_GPR) {
                    Operands_ForceGPR(result, type, 0);
                }
                if (result->reg != RESULT_GPR) {
                    PCodeUtilities_EmitInstruction(PC_MR, 3, result->reg);
                }
                if (is_unsigned(expression->rtype)) {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00588020, 0, 8, 0, 0);
                } else {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00588214, 0, 8, 0, 0);
                }
                lift_result(result, 3);
            } else if ((((target_kind == TYPEINT) || (target_kind == TYPEENUM)) && (copts.operandsDebug != 0)) &&
                       ((kind == TYPEFLOAT) && (type->size != 4))) {
                data_00560648[node->type](node, 0, 0, result);
#if VERSION == VERSION_GC_1_0
                Operands_ForceGPRPair(result, (Type *)type, 4, 3);
                result->reg = move_register_pair(result->regHi, 3, result->reg, 4);
#else
                Operands_ForceGPRPair(result, (Type *)type, 0, 0);
                if (result->kind != OpndType_GPRPair) {
                    CError_FATAL(1169);
                }
                if (result->regHi != returnRegHi) {
                    PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, result->regHi);
                }
                if (result->reg != return_gpr_first) {
                    PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, result->reg);
                }
#endif
                if (is_unsigned(expression->rtype)) {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e94, 0, 0x18, 0, 0);
                } else {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00587ea4, 0, 0x18, 0, 0);
                }
                lift_result(result, 0);
            } else if ((((target_kind == TYPEINT) || (target_kind == TYPEENUM)) && (copts.operandsDebug != 0)) &&
                       ((kind == TYPEFLOAT) && (type->size == 4))) {
                data_00560648[node->type](node, 0, 0, result);
                if (result->kind != OpndType_GPR) {
                    Operands_ForceGPR(result, type, 0);
                }
                if (result->reg != RESULT_GPR) {
                    PCodeUtilities_EmitInstruction(PC_MR, 3, result->reg);
                }
                if (is_unsigned(expression->rtype)) {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e80, 0, 8, 0, 0);
                } else {
                    PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e7c, 0, 8, 0, 0);
                }
                lift_result(result, 0);
            } else {
                CError_FATAL(1209);
            }
        }
    }
}
