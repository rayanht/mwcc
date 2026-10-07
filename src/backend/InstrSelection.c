#define CERROR_FILE "InstrSelection.c"
#include "compiler/common.h"
#include "compiler/InstrSelection.h"
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
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/FunctionCalls.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/StructMoves.h"
#include "compiler/Switch.h"
#include "compiler/TOC.h"
#include <stdlib.h>
#include "compiler/ENode.h"

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)
typedef void (*DispatchHandler)(unsigned char *input, int argument2, int argument3, struct DispatchResult *output);
typedef void (*OperandGenFunc)(ENode *node, int a, int b, Operand *out);

typedef void (*SelFn)(ENode *, SInt16, SInt16, Operand *);

typedef void (*GenOperandFunc)(ENode *node, SInt32 a, SInt32 b, void *out);

typedef void (*ENodeGenFn)(ENode *node, SInt32 a, SInt32 b, Operand *res);

typedef void (*CodeGenFn)(ENode *, int, int, void *);
typedef int (*SelectionHandler)(struct S1 *, int, int, void *);
typedef enum TypeTag { TV = 0, TI = 1, TF = 2, TE = 3, TP = 11 } TypeTag;

void emit_vector128_constant(ENode *node, short requestedRegister, short unused, Operand *result)
{
    int index;
    short halfword;
    int patternIndex;
    char byte;
    char bytesEqual;
    char *bytes;
    short *halfwords;
    int word;
    char halfwordsEqual;
    int *words;
    MWVector128 *pattern;
    short targetRegister;
    MWVector128 *value;
    int word2;
    MWVector128 *alternatePattern;
    int temporaryRegister;
    int alternateRegister;
    char wordsEqual;
    int word0;
    int word3;
    int word1;
    int registerNumber;

    targetRegister = requestedRegister != 0 ? requestedRegister : gUsedVirtualRegistersVR++;
    index = 1;
    registerNumber = targetRegister;
    bytes = (char *)(&node->data);
    byte = bytes[0];
    bytesEqual = 1;
    for (; bytesEqual && index < 16; ++index) {
        bytesEqual = byte == bytes[index];
    }
    if (bytesEqual && byte < 16 && byte > -17) {
        PCodeUtilities_EmitInstruction(PC_VSPLTISB, registerNumber, byte);
        result->kind = OpndType_VR;
        result->reg = registerNumber;
        return;
    }
    halfwords = (short *)(&node->data);
    halfword = halfwords[0];
    index = 1;
    halfwordsEqual = 1;
    for (; halfwordsEqual && index < 8; ++index) {
        halfwordsEqual = halfword == halfwords[index];
    }
    if (halfwordsEqual && halfword < 16 && halfword > -17) {
        PCodeUtilities_EmitInstruction(PC_VSPLTISH, registerNumber, halfword);
        result->kind = OpndType_VR;
        result->reg = registerNumber;
        return;
    }
    words = (int *)(&node->data);
    word = words[0];
    index = 1;
    wordsEqual = 1;
    for (; wordsEqual && index < 4; ++index) {
        wordsEqual = word == words[index];
    }
    if (wordsEqual && word < 16 && word > -17) {
        PCodeUtilities_EmitInstruction(PC_VSPLTISW, registerNumber, word);
        result->kind = OpndType_VR;
        result->reg = registerNumber;
        return;
    }
    patternIndex = 0;
    value = &node->data.vector128;
    word0 = value->longElements[0];
    word1 = value->longElements[1];
    word2 = value->longElements[2];
    word3 = value->longElements[3];
    do {
        pattern = (MWVector128 *)(vector128_patterns + patternIndex * sizeof(MWVector128));
        alternatePattern = &alternate_vector_patterns[(unsigned int)patternIndex];
        if (word0 == pattern->longElements[0] && word1 == pattern->longElements[1] &&
            word2 == pattern->longElements[2] && word3 == pattern->longElements[3]) {
            temporaryRegister = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_LI, temporaryRegister, patternIndex);
            PCodeUtilities_EmitInstruction(PC_LVSL, registerNumber, 0, temporaryRegister);
            result->kind = OpndType_VR;
            result->reg = registerNumber;
            return;
        }
        if (word0 == alternatePattern->longElements[0] && word1 == alternatePattern->longElements[1] &&
            word2 == alternatePattern->longElements[2] && word3 == alternatePattern->longElements[3]) {
            alternateRegister = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_LI, alternateRegister, patternIndex);
            PCodeUtilities_EmitInstruction(PC_LVSR, registerNumber, 0, alternateRegister);
            result->kind = OpndType_VR;
            result->reg = registerNumber;
            return;
        }
        ++patternIndex;
    } while (patternIndex < 16);
    CError_Internal(instrSelectionFileName, 5481);
}

/* Instruction-selection operand storage with a register pair. */

enum ETypeCode {
    ETC_VOID = TYPEVOID,
    ETC_INT = TYPEINT,
    ETC_FLOAT = TYPEFLOAT,
    ETC_ENUM = TYPEENUM,
    ETC_POINTER = TYPEPOINTER
};

/* The integer-constant node kind in this build. */
enum { INTCONST_NODE = 49 };

static inline void EmitIntoReg(ENode *l, SInt16 r1, SInt16 r2, Operand *res)
{
    SInt16 r;
    if (r2 != 0)
        r = r2;
    else
        r = gUsedVirtualRegistersGPR++;
    if (r == res->reg) {
        SInt16 rr;
        if (r1 != 0)
            rr = r1;
        else
            rr = gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_MR, rr, res->reg);
        res->reg = rr;
    }
    if (Type_IsUnsigned(l->rtype))
        PCodeUtilities_LoadImmediate(r, 0);
    else
        PCodeUtilities_EmitInstruction(PC_SRAWI, r, res->reg, 0x1f);
    res->kind = OpndType_GPRPair;
    res->regHi = r;
}

void generate_gpr_pair_type_conversion(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result)
{
    ENode *operand;
    Type *resultType;
    Type *operandType;
    Type *conversionType;
    enum ETypeCode resultKind;

    operand = node->data.diadic.left;
    operandType = operand->rtype;
    conversionType = resultType = node->rtype;
    resultKind = resultType->type;

    if (resultKind == TYPEVOID) {
        data_00560648[operand->type](operand, 0, 0, result);
        if (operand->type == EINDIRECT && (result->flags & fIsVolatile) != 0)
            Operands_ForceGPRPair(result, operand->rtype, 0, 0);
        result->kind = OpndType_Immediate;
        result->immediate = 0;
    } else if (operandType->type == TYPEINT || operandType->type == TYPEENUM) {
        if (conversionType->type == TYPEFLOAT) {
            data_00560648[operand->type](operand, 0, 0, result);
            Operands_ForceGPRPair(result, operandType, 0, 0);
            if (result->regHi != returnRegHi)
                PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, result->regHi);
            if (result->reg != return_gpr_first)
                PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, result->reg);
            if (Type_IsUnsigned(operandType))
                PCodeUtilities_EmitObjectInstructionWithPayload(
                    (conversionType->size == 4) ? data_00587eac : data_00587eb4, 0, 0x18, 0, 0);
            else
                PCodeUtilities_EmitObjectInstructionWithPayload(
                    (conversionType->size == 4) ? data_00587edc : data_00587ee4, 0, 0x18, 0, 0);
            result->kind = OpndType_FPR;
            result->reg = gUsedVirtualRegistersFPR++;
            PCodeUtilities_EmitInstruction(PC_FMR, result->reg, 1);
        } else if (operandType->size < conversionType->size &&
                   !(operand->type == EINDIRECT && operand->data.monadic->type == INTCONST_NODE) &&
                   !((operand->type >= EASS && operand->type <= EORASS) &&
                     operand->data.monadic->data.monadic->type == INTCONST_NODE) &&
                   !((operand->type >= EPOSTINC && operand->type <= EPREDEC) &&
                     operand->data.monadic->data.monadic->type == INTCONST_NODE)) {
            CError_ASSERT(5278, (resultKind == TYPEINT || resultKind == TYPEENUM) && resultType->size == 8);
            data_00560648[operand->type](operand, 0, 0, result);
            if (operandType->size < 4)
                Operands_ExtendGPR(result, operandType, reg1);
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, operandType, reg1);
            EmitIntoReg(operand, reg1, reg2, result);
        } else {
            data_00560648[operand->type](operand, reg1, 0, result);
            if (conversionType->size < operandType->size) {
                Operands_ForceGPRPair(result, operandType, reg1, reg2);
                result->kind = OpndType_GPR;
                result->regHi = 0;
            }
        }
    } else if (operandType->type == TYPEPOINTER) {
        data_00560648[operand->type](operand, reg1, 0, result);
        CError_ASSERT(5317, (node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8);
        data_00560648[operand->type](operand, reg1, 0, result);
        EmitIntoReg(operand, reg1, reg2, result);
    } else if (conversionType->type == TYPEFLOAT) {
        CError_FATAL(5339);
    } else {
        data_00560648[operand->type](operand, 0, 0, result);
        if (result->kind != OpndType_FPR)
            Operands_ForceFPR(result, operandType, 0);
        if ((int)result->reg != 1)
            PCodeUtilities_EmitInstruction(PC_FMR, 1, result->reg);
        PCodeUtilities_EmitObjectInstructionWithPayload(data_00587e68, 0, 0, 2, 0);
        result->kind = OpndType_GPRPair;
        result->reg = gUsedVirtualRegistersGPR++;
        result->regHi = gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_MR, result->reg, return_gpr_first);
        PCodeUtilities_EmitInstruction(PC_MR, result->regHi, returnRegHi);
    }
}

void generate_gpr_pair_division_or_modulo(ENode *node, SInt16 requestedReg, SInt16 requestedRegHi, Operand *result)
{
    ENode *left;
    ENode *right;
    Operand leftOperand;
    Operand rightOperand;

    left = node->data.diadic.left;
    right = node->data.diadic.right;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    data_00560648[right->type](right, 0, 0, &rightOperand);
    Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);

    data_00560648[left->type](left, 0, 0, &leftOperand);
    Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);

    if (!(leftOperand.kind == OpndType_GPRPair && rightOperand.kind == OpndType_GPRPair))
        CError_FATAL(5165);

    if (leftOperand.regHi != returnRegHi)
        PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, leftOperand.regHi);
    if (leftOperand.reg != return_gpr_first)
        PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, leftOperand.reg);
    if (rightOperand.regHi != sfpe_right_operand_reg_hi)
        PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg_hi, rightOperand.regHi);
    if (rightOperand.reg != sfpe_right_operand_reg)
        PCodeUtilities_EmitInstruction(PC_MR, sfpe_right_operand_reg, rightOperand.reg);

    if (node->type == EDIV) {
        if (Type_IsUnsigned(left->rtype) || Type_IsUnsigned(right->rtype))
            PCodeUtilities_EmitObjectInstructionWithPayload(data_00588038, 0, 0x78, 0, 0);
        else
            PCodeUtilities_EmitObjectInstructionWithPayload(data_0058803c, 0, 0x78, 0, 0);
    } else if (node->type == EMODULO) {
        if (Type_IsUnsigned(left->rtype) || Type_IsUnsigned(right->rtype))
            PCodeUtilities_EmitObjectInstructionWithPayload(data_00588078, 0, 0x78, 0, 0);
        else
            PCodeUtilities_EmitObjectInstructionWithPayload(data_0058808c, 0, 0x78, 0, 0);
    } else {
        CError_FATAL(5191);
    }

    result->kind = OpndType_GPRPair;
    result->reg = gUsedVirtualRegistersGPR++;
    result->regHi = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_MR, result->reg, return_gpr_first);
    PCodeUtilities_EmitInstruction(PC_MR, result->regHi, returnRegHi);
}

/* Operand storage used by the register-pair shift selector. */

/* Operand storage used by integer-pair instruction selection. */

void generate_gpr_pair_shift(ENode *node, short outputReg, short outputRegHi, Operand *result)
{
    Operand value;
    Operand count;
    ENode *left;
    ENode *right;

    left = node->data.diadic.left;
    right = node->data.diadic.right;

    memclrw(&value, sizeof(value));
    memclrw(&count, sizeof(count));

    (*data_00560648[right->type])(right, 0, 0, &count);
    if (count.kind != OpndType_GPR)
        Operands_ForceGPR(&count, right->rtype, 0);
    if (count.kind == OpndType_GPRPair) {
        count.regHi = 0;
        count.kind = OpndType_GPR;
    }

    (*data_00560648[left->type])(left, 0, 0, &value);
    Operands_ForceGPRPair(&value, left->rtype, 0, 0);

    if (value.kind != OpndType_GPRPair || count.kind != OpndType_GPR)
        CError_FATAL(5097);

    if (value.regHi != returnRegHi)
        PCodeUtilities_EmitInstruction(PC_MR, returnRegHi, value.regHi);
    if (value.reg != return_gpr_first)
        PCodeUtilities_EmitInstruction(PC_MR, return_gpr_first, value.reg);
    if ((SInt32)count.reg != 5)
        PCodeUtilities_EmitInstruction(PC_MR, 5, count.reg);

    if (node->type == ESHR) {
        if (Type_IsUnsigned(left->rtype))
            PCodeUtilities_EmitObjectInstructionWithPayload(data_00587ff0, 0, 0x38, 0, 0);
        else
            PCodeUtilities_EmitObjectInstructionWithPayload(data_00587ff4, 0, 0x38, 0, 0);
    } else if (node->type == ESHL) {
        PCodeUtilities_EmitObjectInstructionWithPayload(data_00587fec, 0, 0x38, 0, 0);
    } else {
        CError_FATAL(5116);
    }

    result->kind = OpndType_GPRPair;
    result->reg = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR++;
    result->regHi = gUsedVirtualRegistersGPR;
    gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_MR, result->reg, return_gpr_first);
    PCodeUtilities_EmitInstruction(PC_MR, result->regHi, returnRegHi);
}

static inline int fn_004b2500_inline1(ENode *v1)
{
    int v2;
    if (Registers_GetInfo(v1->data.objref) != NULL) {
        v2 = (int)Registers_GetInfo(v1->data.objref)->reg;
    } else {
        v2 = 0;
    }
    return v2;
}

#define ENSURE_GPR(op, type, reg)                                                                                      \
    do {                                                                                                               \
        if ((op)->kind != OpndType_GPR)                                                                                \
            Operands_ForceGPR((op), (type), (reg));                                                                    \
    } while (0)

void InstrSelection_GenerateLongLongComparison(ENode *expr, Operand *result, SInt32 produceValue)
{
    ENode *left = expr->data.diadic.left;
    ENode *right = expr->data.diadic.right;
    Operand leftOperand;
    Operand rightOperand;
    Operand swapOperand;
    SInt32 scratchReg, comparisonReg, borrowReg;
    SInt32 leftHighReg, rightHighReg;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));
    memclrw(&swapOperand, sizeof(swapOperand));

    if (right->hascall != 0) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (right->rtype->size < 4)
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        ENSURE_GPR(&rightOperand, right->rtype, 0);
        if (right->rtype->size < 8) {
            SInt16 highReg = gUsedVirtualRegistersGPR++;
            if (Type_IsUnsigned(right->rtype))
                PCodeUtilities_LoadImmediate(highReg, 0);
            else
                PCodeUtilities_EmitInstruction(PC_SRAWI, highReg, rightOperand.reg, 0x1f);
            rightOperand.kind = OpndType_GPRPair;
            rightOperand.regHi = highReg;
        }
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (left->rtype->size < 4)
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        ENSURE_GPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < 8) {
            SInt16 highReg = gUsedVirtualRegistersGPR++;
            if (Type_IsUnsigned(right->rtype))
                PCodeUtilities_LoadImmediate(highReg, 0);
            else
                PCodeUtilities_EmitInstruction(PC_SRAWI, highReg, leftOperand.reg, 0x1f);
            leftOperand.kind = OpndType_GPRPair;
            leftOperand.regHi = highReg;
        }
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        ENSURE_GPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < 4)
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < 8) {
            SInt16 highReg = gUsedVirtualRegistersGPR++;
            if (Type_IsUnsigned(right->rtype))
                PCodeUtilities_LoadImmediate(highReg, 0);
            else
                PCodeUtilities_EmitInstruction(PC_SRAWI, highReg, leftOperand.reg, 0x1f);
            leftOperand.kind = OpndType_GPRPair;
            leftOperand.regHi = highReg;
        }
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (right->rtype->size < 4)
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        ENSURE_GPR(&rightOperand, right->rtype, 0);
        if (right->rtype->size < 8) {
            SInt16 highReg = gUsedVirtualRegistersGPR++;
            if (Type_IsUnsigned(right->rtype))
                PCodeUtilities_LoadImmediate(highReg, 0);
            else
                PCodeUtilities_EmitInstruction(PC_SRAWI, highReg, rightOperand.reg, 0x1f);
            rightOperand.kind = OpndType_GPRPair;
            rightOperand.regHi = highReg;
        }
    }

    if (leftOperand.kind != OpndType_GPRPair || rightOperand.kind != OpndType_GPRPair)
        CError_FATAL(4939);

    switch (expr->type) {
        case EEQU:
        case ENOTEQU:
            scratchReg = gUsedVirtualRegistersGPR++;
            comparisonReg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_XOR, scratchReg, leftOperand.reg, rightOperand.reg);
            PCodeUtilities_EmitInstruction(PC_XOR, comparisonReg, leftOperand.regHi, rightOperand.regHi);
            PCodeUtilities_EmitInstruction(PC_OR, comparisonReg, scratchReg, comparisonReg);
            if (produceValue != 0) {
                if (expr->type == EEQU) {
                    PCodeUtilities_EmitInstruction(PC_CNTLZW, comparisonReg, comparisonReg);
                    PCodeUtilities_EmitInstruction(PC_RLWINM, comparisonReg, comparisonReg, 0x1b, 5, 0x1f);
                } else {
                    PCodeUtilities_EmitInstruction(PC_ADDIC, scratchReg, comparisonReg, -1);
                    PCodeUtilities_EmitInstruction(PC_SUBFE, comparisonReg, scratchReg, comparisonReg);
                }
                result->kind = OpndType_GPR;
                result->reg = comparisonReg;
                result->regHi = 0;
            } else {
                PCodeUtilities_EmitInstruction(PC_CMPI, 0, comparisonReg, 0);
                result->kind = OpndType_CRField;
                result->reg = 0;
                result->secondary_reg = expr->type;
            }
            break;

        case EGREATER:
            swapOperand = leftOperand;
            leftOperand = rightOperand;
            rightOperand = swapOperand;
            /* fall through */
        case ELESS:
            scratchReg = gUsedVirtualRegistersGPR++;
            comparisonReg = gUsedVirtualRegistersGPR++;
            borrowReg = gUsedVirtualRegistersGPR++;
            if (left->rtype != (Type *)&stunsignedlonglong && right->rtype != (Type *)&stunsignedlonglong) {
                PCodeUtilities_EmitInstruction(PC_XORIS, scratchReg, leftOperand.regHi, 0x8000);
                PCodeUtilities_EmitInstruction(PC_XORIS, comparisonReg, rightOperand.regHi, 0x8000);
                leftHighReg = scratchReg;
                rightHighReg = comparisonReg;
            } else {
                leftHighReg = leftOperand.regHi;
                rightHighReg = rightOperand.regHi;
            }
            PCodeUtilities_EmitInstruction(PC_SUBFC, borrowReg, rightOperand.reg, leftOperand.reg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, comparisonReg, rightHighReg, leftHighReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, comparisonReg, scratchReg, scratchReg);
            PCodeUtilities_EmitInstruction(PC_NEG, comparisonReg, comparisonReg);
            if (produceValue != 0) {
                result->kind = OpndType_GPR;
                result->reg = comparisonReg;
                result->regHi = 0;
            } else {
                PCodeUtilities_EmitInstruction(PC_CMPI, 0, comparisonReg, 0);
                result->kind = OpndType_CRField;
                result->reg = 0;
                result->secondary_reg = 0x18;
            }
            break;

        case ELESSEQU:
            swapOperand = leftOperand;
            leftOperand = rightOperand;
            rightOperand = swapOperand;
            /* fall through */
        case EGREATEREQU:
            scratchReg = gUsedVirtualRegistersGPR++;
            comparisonReg = gUsedVirtualRegistersGPR++;
            borrowReg = gUsedVirtualRegistersGPR++;
            if (left->rtype != (Type *)&stunsignedlonglong && right->rtype != (Type *)&stunsignedlonglong) {
                PCodeUtilities_EmitInstruction(PC_XORIS, scratchReg, leftOperand.regHi, 0x8000);
                PCodeUtilities_EmitInstruction(PC_XORIS, comparisonReg, rightOperand.regHi, 0x8000);
                leftHighReg = scratchReg;
                rightHighReg = comparisonReg;
            } else {
                leftHighReg = leftOperand.regHi;
                rightHighReg = rightOperand.regHi;
            }
            PCodeUtilities_EmitInstruction(PC_SUBFC, borrowReg, rightOperand.reg, leftOperand.reg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, comparisonReg, rightHighReg, leftHighReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, comparisonReg, scratchReg, scratchReg);
            PCodeUtilities_EmitInstruction(PC_NEG, comparisonReg, comparisonReg);
            if (produceValue != 0) {
                PCodeUtilities_EmitInstruction(PC_SUBFIC, comparisonReg, comparisonReg, 1);
                result->kind = OpndType_GPR;
                result->reg = comparisonReg;
                result->regHi = 0;
            } else {
                PCodeUtilities_EmitInstruction(PC_CMPI, 0, comparisonReg, 0);
                result->kind = OpndType_CRField;
                result->reg = 0;
                result->secondary_reg = 0x17;
            }
            break;

        default:
            CError_FATAL(5049);
    }
}

void select_monadic_operand(ENode *enode, SInt16 unused2, SInt16 unused3, Operand *out)
{
    Type *type = enode->rtype;
    ENode *child = enode->data.monadic;
    if (child->type == EOBJREF &&
        (Registers_GetInfo(child->data.objref) != NULL ? Registers_GetInfo(child->data.objref)->reg : 0)) {
        VarInfo *info = Registers_GetInfo(child->data.objref);
        if (info->is_vector)
            CError_FATAL(4752);
        out->kind = info->is_fpr ? (UInt8)5 : (UInt8)3;
        out->reg = info->reg;
        out->regHi = info->regHi;
        out->object = NULL;
    } else if (child->type == EBITFIELD) {
        CError_FATAL(4765);
    } else {
        data_00560648[child->type](child, 0, 0, out);
        Operands_MakeIndirect(out, child);
    }
}

void emit_postinc_postdec_gpr_pair(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    int updatedHi;
    ENode *operand;
    int sourceHi;
    int useOutputReg;
    short resultReg;
    int useOutputRegHi;
    short resultRegHi;
    int sourceReg;
    int updatedReg;
    unsigned char kind;
    Type *resultType;
    Operand original;
    Operand value;
    operand = expr->data.monadic->data.monadic;
    resultType = expr->rtype;
    memclrw(&original, sizeof(original));
    memclrw(&value, sizeof(value));
    if (operand->type == EOBJREF && (sourceReg = fn_004b2500_inline1((ENode *)operand)) != 0) {
        sourceHi = (Registers_GetInfo(operand->data.objref))->regHi;
        output->kind = OpndType_GPRPair;
        useOutputReg = outputReg != 0 && (outputReg != sourceReg && outputReg != sourceHi);
        if (useOutputReg != 0) {
            resultReg = outputReg;
        } else {
            resultReg = gUsedVirtualRegistersGPR++;
        }
        output->reg = resultReg;
        useOutputRegHi = outputRegHi != 0 && (outputRegHi != sourceHi && outputRegHi != sourceReg);
        if (useOutputRegHi != 0) {
            resultRegHi = outputRegHi;
        } else {
            resultRegHi = gUsedVirtualRegistersGPR++;
        }
        output->regHi = resultRegHi;
        PCodeUtilities_EmitInstruction(PC_MR, output->reg, sourceReg);
        PCodeUtilities_EmitInstruction(PC_MR, output->regHi, sourceHi);
        if (expr->type == EPOSTINC) {
            PCodeUtilities_EmitInstruction(PC_ADDIC, sourceReg, sourceReg, 1);
            PCodeUtilities_EmitInstruction(PC_ADDZE, sourceHi, sourceHi);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDIC, sourceReg, sourceReg, -1);
            PCodeUtilities_EmitInstruction(PC_ADDME, sourceHi, sourceHi);
        }
    } else {
        CE_ASSERT(operand->type == EBITFIELD, CError_FATAL(4709));
        kind = operand->type;
        data_00560648[kind](operand, 0, 0, &original);
        Operands_MakeIndirect(&original, operand);
        value = original;
        Operands_ForceGPRPair(&value, resultType, 0, 0);
        output->kind = OpndType_GPRPair;
        output->reg = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1;
        output->regHi = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1;
        PCodeUtilities_EmitInstruction(PC_MR, output->reg, value.reg);
        PCodeUtilities_EmitInstruction(PC_MR, output->regHi, value.regHi);
        updatedReg = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1;
        updatedHi = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1;
        if (expr->type == EPOSTINC) {
            PCodeUtilities_EmitInstruction(PC_ADDIC, updatedReg, value.reg, 1);
            PCodeUtilities_EmitInstruction(PC_ADDZE, updatedHi, value.regHi);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDIC, updatedReg, value.reg, -1);
            PCodeUtilities_EmitInstruction(PC_ADDME, updatedHi, value.regHi);
        }
        Operands_StoreGPRPair(updatedReg, updatedHi, &original, resultType);
    }
}

void generate_gpr_pair_assignment(ENode *node, SInt16 requestedReg, SInt16 requestedRegHi, Operand *out)
{
    ENode *left = node->data.diadic.left;
    ENode *right = node->data.diadic.right;
    Type *type = node->rtype;
    Operand leftOperand;
    Operand rightOperand;
    VarInfo *regInfo;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (left->type == EOBJREF &&
        (Registers_GetInfo(left->data.objref) ? Registers_GetInfo(left->data.objref)->reg : 0) != 0) {
        regInfo = Registers_GetInfo(left->data.objref);
        data_00560648[right->type](right, regInfo->reg, regInfo->regHi, &rightOperand);
        if (regInfo->is_fpr != 0 || regInfo->is_vector != 0) {
            CError_FATAL(4636);
        } else {
            Operands_ForceGPRPair(&rightOperand, (Type *)type, regInfo->reg, regInfo->regHi);
            *out = rightOperand;
        }
    } else if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
               (type->type == TYPEMEMBERPOINTER && type->size == 4)) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        Operands_ForceGPRPair(&rightOperand, (Type *)right->rtype, 0, 0);
        if (left->type == EBITFIELD) {
            CError_FATAL(4651);
        } else {
            data_00560648[left->type](left, 0, 0, &leftOperand);
            Operands_MakeIndirect(&leftOperand, left);
            Operands_StoreGPRPair(rightOperand.reg, rightOperand.regHi, &leftOperand, type);
        }
        out->kind = OpndType_GPRPair;
        out->reg = rightOperand.reg;
        out->regHi = rightOperand.regHi;
    }
}

void emit_gpr_pair_multiply(ENode *expr, SInt16 requestedLo, SInt16 requestedHi, Operand *out)
{
    ENode *left = expr->data.diadic.left;
    ENode *right = expr->data.diadic.right;
    Operand leftOperand;
    Operand rightOperand;
    SInt16 crossProduct, highProduct, resultLo, resultHi;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
    } else {
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
    }

    if (!(leftOperand.kind == OpndType_GPRPair && rightOperand.kind == OpndType_GPRPair))
        CError_FATAL(4510);

    crossProduct = gUsedVirtualRegistersGPR++;
    highProduct = gUsedVirtualRegistersGPR++;
    if (requestedLo)
        resultLo = requestedLo;
    else
        resultLo = gUsedVirtualRegistersGPR++;
    if (requestedHi)
        resultHi = requestedHi;
    else
        resultHi = gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_MULLW, crossProduct, leftOperand.regHi, rightOperand.reg);
    PCodeUtilities_EmitInstruction(PC_MULHWU, highProduct, leftOperand.reg, rightOperand.reg);
    PCodeUtilities_EmitInstruction(PC_ADD, crossProduct, crossProduct, highProduct);
    PCodeUtilities_EmitInstruction(PC_MULLW, highProduct, leftOperand.reg, rightOperand.regHi);
    PCodeUtilities_EmitInstruction(PC_MULLW, resultLo, leftOperand.reg, rightOperand.reg);
    PCodeUtilities_EmitInstruction(PC_ADD, resultHi, crossProduct, highProduct);

    out->kind = OpndType_GPRPair;
    out->reg = resultLo;
    out->regHi = resultHi;
}

/* 0x560774, "InstrSelection.c" */

/* Operand / addressing-mode record: byte kind, then register pair. */

void emit_gpr_pair_and(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result)
{
    Operand leftOperand;
    Operand rightOperand;
    ENode *left;
    ENode *right;
    SInt16 resultReg;
    SInt16 resultRegHi;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));
    if (right->hascall) {
        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
    } else {
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
        (*data_00560648[right->type])(right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
    }
    if (leftOperand.kind != OpndType_GPRPair || rightOperand.kind != OpndType_GPRPair)
        CError_FATAL(4453);
    if (reg1)
        resultReg = reg1;
    else
        resultReg = gUsedVirtualRegistersGPR++;
    if (reg2)
        resultRegHi = reg2;
    else
        resultRegHi = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_AND, resultReg, leftOperand.reg, rightOperand.reg);
    PCodeUtilities_EmitInstruction(PC_AND, resultRegHi, leftOperand.regHi, rightOperand.regHi);
    result->kind = OpndType_GPRPair;
    result->reg = resultReg;
    result->regHi = resultRegHi;
}

void emit_gpr_pair_or(ENode *node, SInt16 reg1, SInt16 reg2, Operand *out)
{
    Operand res1;
    Operand res2;
    ENode *left;
    ENode *right;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    memclrw(&res1, sizeof(res1));
    memclrw(&res2, sizeof(res2));

    if (right->hascall) {
        data_00560648[right->type](right, 0, 0, &res2);
        if (res2.kind >= 9)
            Operands_ForceGPRPair(&res2, (Type *)right->rtype, 0, 0);
        data_00560648[left->type](left, 0, 0, &res1);
        if (res1.kind >= 9)
            Operands_ForceGPRPair(&res1, (Type *)left->rtype, 0, 0);
    } else {
        data_00560648[left->type](left, 0, 0, &res1);
        if (res1.kind >= 9)
            Operands_ForceGPRPair(&res1, (Type *)left->rtype, 0, 0);
        data_00560648[right->type](right, 0, 0, &res2);
        if (res2.kind >= 9)
            Operands_ForceGPRPair(&res2, (Type *)right->rtype, 0, 0);
    }

    if (res1.kind != OpndType_GPRPair || res2.kind != OpndType_GPRPair)
        CError_FATAL(4399);

    reg1 = (reg1 != 0) ? reg1 : gUsedVirtualRegistersGPR++;
    reg2 = (reg2 != 0) ? reg2 : gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_OR, reg1, res1.reg, res2.reg);
    PCodeUtilities_EmitInstruction(PC_OR, reg2, res1.regHi, res2.regHi);

    out->kind = OpndType_GPRPair;
    out->reg = reg1;
    out->regHi = reg2;
}

/* 0x560774, filename string */

static void genOperand(ENode *n, Operand *op)
{
    (*(data_00560648[n->type]))(n, 0, 0, op);
    if (op->kind >= 9)
        Operands_ForceGPRPair(op, (Type *)n->rtype, 0, 0);
}

void gen_xor_reg_pair(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result)
{
    ENode *left = node->data.diadic.left;
    ENode *right = node->data.diadic.right;
    Operand opL;
    Operand opR;
    SInt16 rlo;
    SInt16 rhi;

    memclrw(&opL, sizeof(opL));
    memclrw(&opR, sizeof(opR));

    if (right->hascall) {
        genOperand(right, &opR);
        genOperand(left, &opL);
    } else {
        genOperand(left, &opL);
        genOperand(right, &opR);
    }

    if (opL.kind != OpndType_GPRPair || opR.kind != OpndType_GPRPair)
        CError_FATAL(4345);

    if (reg1)
        rlo = reg1;
    else
        rlo = gUsedVirtualRegistersGPR++;
    if (reg2)
        rhi = reg2;
    else
        rhi = gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_XOR, rlo, opL.reg, opR.reg);
    PCodeUtilities_EmitInstruction(PC_XOR, rhi, opL.regHi, opR.regHi);

    result->kind = OpndType_GPRPair;
    result->reg = rlo;
    result->regHi = rhi;
}

/* An operand record: byte kind, then two 16-bit register numbers. */

void emit_gpr_pair_subtraction(ENode *node, SInt16 lowReg, SInt16 highReg, Operand *result)
{
    Operand leftOperand;
    Operand rightOperand;
    ENode *left = node->data.diadic.left;
    ENode *right = node->data.diadic.right;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
    }

    CError_ASSERT(4291, leftOperand.kind == OpndType_GPRPair && rightOperand.kind == OpndType_GPRPair);

    lowReg = lowReg ? lowReg : gUsedVirtualRegistersGPR++;
    highReg = highReg ? highReg : gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_SUBFC, lowReg, rightOperand.reg, leftOperand.reg);
    PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, rightOperand.regHi, leftOperand.regHi);

    result->kind = OpndType_GPRPair;
    result->reg = lowReg;
    result->regHi = highReg;
}

void emit_gpr_pair_add(ENode *node, short targetReg, short targetRegHi, Operand *out)
{
    ENode *left;
    ENode *right;
    Operand leftOperand;
    Operand rightOperand;
    short resultReg;
    short resultRegHi;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind >= 9)
            Operands_ForceGPRPair(&leftOperand, left->rtype, 0, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind >= 9)
            Operands_ForceGPRPair(&rightOperand, right->rtype, 0, 0);
    }

    if (leftOperand.kind != OpndType_GPRPair || rightOperand.kind != OpndType_GPRPair)
        CError_FATAL(4214);

    if (targetReg != 0)
        resultReg = targetReg;
    else
        resultReg = gUsedVirtualRegistersGPR++;

    if (targetRegHi != 0)
        resultRegHi = targetRegHi;
    else
        resultRegHi = gUsedVirtualRegistersGPR++;

    PCodeUtilities_EmitInstruction(PC_ADDC, resultReg, leftOperand.reg, rightOperand.reg);
    PCodeUtilities_EmitInstruction(PC_ADDE, resultRegHi, leftOperand.regHi, rightOperand.regHi);

    out->kind = OpndType_GPRPair;
    out->reg = resultReg;
    out->regHi = resultRegHi;
}

static inline SInt16 low_word(SInt32 value)
{
    return value;
}

static inline int SwapOp(int op)
{
    int r = op;
    switch (op) {
        case ELESS:
            r = EGREATER;
            break;
        case EGREATER:
            r = ELESS;
            break;
        case ELESSEQU:
            r = EGREATEREQU;
            break;
        case EGREATEREQU:
            r = ELESSEQU;
            break;
    }
    return r;
}

static int IS_NonVoid(Type *t)
{
    int r = 0;
    if (t->type != TYPEVOID)
        r = 1;
    return r;
}

static int IS_NotIgnored(ENode *e)
{
    int r = 0;
    if (!e->ignored)
        r = 1;
    return r;
}

static void GenOperand(ENode *node, Operand *res)
{
    data_00560648[node->type](node, 0, 0, res);
    if (res->kind != OpndType_FPR)
        Operands_ForceFPR(res, node->rtype, 0);
}

static inline void InstrSelection_004b5ae0_inline1(ENode *p0, Operand *p1)
{
    unsigned char t1;
    t1 = p0->type;
    data_00560648[t1](p0, 0, 0, p1);
}

void InstrSelection_EmitAddImmediate(SInt16 destReg, SInt16 sourceReg, int immediate)
{
    if (immediate != (SInt16)immediate) {
        SInt16 highImmediate = (immediate >> 16) + ((immediate >> 15) & 1);
        PCodeUtilities_EmitInstruction(PC_ADDIS, destReg, sourceReg, 0, highImmediate);
        if ((SInt16)immediate != 0) {
            SInt16 lowImmediate = immediate;
            PCodeUtilities_EmitInstruction(PC_ADDI, destReg, destReg, 0, lowImmediate);
        }
    } else {
        PCodeUtilities_EmitInstruction(PC_ADDI, destReg, sourceReg, 0, immediate);
    }
}

int InstrSelection_MatchPostIncDecRegister(ENode *node, Operand *info, SInt32 *size)
{
    Type *rtype;
    int reg;

    rtype = node->rtype;
    if (node->type != EPOSTINC && node->type != EPOSTDEC)
        return 0;

    if (node->data.monadic->type != EINDIRECT)
        return 0;

    if (node->data.monadic->data.monadic->type != EOBJREF)
        return 0;

    if (!(reg = Registers_GetInfo(node->data.monadic->data.monadic->data.objref) != NULL
                    ? Registers_GetInfo(node->data.monadic->data.monadic->data.objref)->reg
                    : 0))
        return 0;

    if (rtype->type == TYPEPOINTER) {
        if (node->type == EPOSTINC)
            *size = TYPE_POINTER(rtype)->target->size;
        else
            *size = -TYPE_POINTER(rtype)->target->size;
    } else {
        if (node->type == EPOSTINC)
            *size = 1;
        else
            *size = -1;
    }

    info->kind = OpndType_GPR;
    info->reg = reg;
    return 1;
}

int InstrSelection_GetMaskRange(UInt32 mask, SInt16 *first, SInt16 *last)
{
    SInt16 complementFirst;
    SInt16 complementLast;

    if (is_contiguous_mask(mask, first, last) != 0) {
        return 1;
    }
    if (mask != 0) {
        if (is_contiguous_mask(~mask, &complementFirst, &complementLast) != 0) {
            *first = complementLast + 1;
            *last = complementFirst - 1;
            return 1;
        }
    }
    return 0;
}

int is_contiguous_mask(unsigned int mask, short *firstBit, short *lastBit)
{
    int first;
    int last;
    int bit;

    last = -1;
    bit = 31;
    first = -1;
    do {
        if (mask & 1) {
            if (first != -1) {
                return 0;
            }
            if (last == -1) {
                last = bit;
            }
        } else {
            if ((last != -1) && (first == -1)) {
                first = bit + 1;
            }
        }
        --bit;
        mask = (int)mask >> 1;
    } while (bit >= 0);
    if (last == -1) {
        return 0;
    }
    if (first == -1) {
        first = 0;
    }
    *firstBit = first;
    *lastBit = last;
    return 1;
}

void emit_cmpli_with_addis(SInt16 condition, ENode *node, SInt32 immediate, Operand *result)
{
    int postIncDec;
    Operand operand;
    SInt32 increment;
    SInt32 tempReg;
    SInt16 updateReg;

    memclrw(&operand, sizeof(operand));
    postIncDec = InstrSelection_MatchPostIncDecRegister(node, &operand, &increment);
    if (!postIncDec) {
        data_00560648[node->type](node, 0, 0, &operand);
    } else {
        updateReg = operand.reg;
    }
    if (node->rtype->size < 4)
        Operands_ExtendGPR(&operand, node->rtype, 0);
    if (operand.kind != OpndType_GPR)
        Operands_ForceGPR(&operand, node->rtype, 0);
    tempReg = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_ADDIS, tempReg, operand.reg, 0, low_word(~(immediate >> 16) + 1));
    PCodeUtilities_EmitInstruction(PC_CMPLI, 0, tempReg, low_word(immediate));
    if (postIncDec) {
        SInt16 lowAdjustment = increment;
        SInt32 adjustment = increment;
        if (increment != lowAdjustment) {
            PCodeUtilities_EmitInstruction(PC_ADDIS, updateReg, updateReg, 0,
                                           low_word((adjustment >> 16) + ((adjustment >> 15) & 1)));
            if (low_word(adjustment) != 0)
                PCodeUtilities_EmitInstruction(PC_ADDI, updateReg, updateReg, 0, low_word(adjustment));
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, updateReg, updateReg, 0, adjustment);
        }
    }
    result->kind = OpndType_CRField;
    result->reg = 0;
    result->secondary_reg = condition;
}

void InstrSelection_004b37b0(short comparison, ENode *input, short sense, Operand *result)
{
    short savedRegister;
    int hasImmediate;
    Operand selected;
    SInt32 immediate;
    SInt32 adjustment;
    memclrw(&selected, sizeof(selected));
    hasImmediate = InstrSelection_MatchPostIncDecRegister(input, &selected, &immediate);
    if (!hasImmediate) {
        data_00560648[input->type](input, 0, 0, &selected);
        if (selected.kind != OpndType_CRField) {
            if (input->rtype->size < 4) {
                Operands_ExtendGPR(&selected, input->rtype, 0);
            } else if (selected.kind != OpndType_GPR) {
                Operands_ForceGPR(&selected, input->rtype, 0);
            }
        }
    } else {
        savedRegister = selected.reg;
        if (input->rtype->size < 4) {
            Operands_ExtendGPR(&selected, input->rtype, 0);
        }
        if (selected.kind != OpndType_GPR) {
            Operands_ForceGPR(&selected, input->rtype, 0);
        }
    }
    if (selected.kind == OpndType_CRField) {
        if ((comparison == EEQU && sense == 1) || (comparison == ENOTEQU && sense == 0) ||
            (comparison == EGREATER && sense == 0) || (comparison == EGREATEREQU && sense == 1)) {
            *result = selected;
            return;
        }
        if ((comparison == EEQU && sense == 0) || (comparison == ENOTEQU && sense == 1) ||
            (comparison == ELESS && sense == 1) || (comparison == ELESSEQU && sense == 0)) {
            *result = selected;
            switch (selected.secondary_reg) {
                case EEQU:
                    result->secondary_reg = ENOTEQU;
                    return;
                case ENOTEQU:
                    result->secondary_reg = EEQU;
                    return;
                case ELESS:
                    result->secondary_reg = EGREATEREQU;
                    return;
                case EGREATER:
                    result->secondary_reg = ELESSEQU;
                    return;
                case ELESSEQU:
                    result->secondary_reg = EGREATER;
                    return;
                case EGREATEREQU:
                    result->secondary_reg = ELESS;
                    return;
            }
        }
        if (selected.kind != OpndType_GPR) {
            Operands_ForceGPR(&selected, input->rtype, 0);
        }
    }
    if (copts.peepholeOptimizationEnabled != 0 && sense == 0 && gCurrentBlock->instruction_count > 0 &&
        gCurrentBlock->reverse_instructions->opcode != PC_RLWINM &&
        (gCurrentBlock->reverse_instructions->flags & 3585) == 513 &&
        gCurrentBlock->reverse_instructions->operandData.operands[0].value.reg == selected.reg &&
        (!Type_IsUnsigned(input->rtype) || comparison == EEQU || comparison == ENOTEQU)) {
        PCodeUtilities_MakeRecordForm(gCurrentBlock->reverse_instructions);
    } else {
        PCodeUtilities_EmitInstruction(Type_IsUnsigned(input->rtype) ? PC_CMPLI : PC_CMPI, 0, selected.reg, sense);
    }
    if (hasImmediate) {
        short lowImmediate = immediate;
        adjustment = immediate;
        if (immediate != lowImmediate) {
            short highImmediate = (adjustment >> 16) + ((adjustment >> 15) & 1);
            PCodeUtilities_EmitInstruction(PC_ADDIS, savedRegister, savedRegister, 0, highImmediate);
            if ((short)adjustment != 0) {
                PCodeUtilities_EmitInstruction(PC_ADDI, savedRegister, savedRegister, 0, (short)adjustment);
            }
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, savedRegister, savedRegister, 0, adjustment);
        }
    }
    result->kind = OpndType_CRField;
    result->reg = 0;
    result->secondary_reg = comparison;
}

void emit_gpr_comparison(short secondaryReg, ENode *left, ENode *right, Operand *result)
{
    char isUnsigned;
    unsigned short opcode;
    Operand leftOperand;
    Operand rightOperand;
    memclrw(&leftOperand, sizeof(Operand));
    memclrw(&rightOperand, sizeof(Operand));
    if (right->hascall != 0) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (right->rtype->size < 4)
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (left->rtype->size < 4)
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        if (left->rtype->size < 4)
            Operands_ExtendGPR(&leftOperand, left->rtype, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (right->rtype->size < 4)
            Operands_ExtendGPR(&rightOperand, right->rtype, 0);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
    }
    isUnsigned = Type_IsUnsigned(left->rtype);
    if (isUnsigned != 0)
        opcode = 0x55;
    else
        opcode = 0x53;
    PCodeUtilities_EmitInstruction(opcode, 0, leftOperand.reg, rightOperand.reg);
    result->kind = OpndType_CRField;
    result->reg = 0;
    result->secondary_reg = secondaryReg;
}

void emit_fpr_comparison(SInt16 comparison, ENode *left, ENode *right, Operand *result)
{
    Operand leftOperand;
    Operand rightOperand;
    SInt32 opcode;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&rightOperand, right->rtype, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&leftOperand, left->rtype, 0);
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&leftOperand, left->rtype, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&rightOperand, right->rtype, 0);
    }

    opcode = ((comparison == EEQU) || (comparison == ENOTEQU)) ? 0xb8 : 0xb9;
    PCodeUtilities_EmitInstruction(opcode, 0, leftOperand.reg, rightOperand.reg);

    if (comparison == ELESSEQU) {
        PCodeUtilities_EmitInstruction(PC_CROR, 0, 2, 0, 0, 0, 2);
        comparison = EEQU;
    } else if (comparison == EGREATEREQU) {
        PCodeUtilities_EmitInstruction(PC_CROR, 0, 2, 0, 1, 0, 2);
        comparison = EEQU;
    }

    result->kind = OpndType_CRField;
    result->reg = 0;
    result->secondary_reg = comparison;
}

void generate_comparison_gpr(ENode *expr, Operand *output, short requestedReg)
{
    ENode *left;
    ENode *right;
    int invert;
    int resultReg;
    Boolean isUnsigned;
    int scratchReg1;
    int scratchReg2;
    int scratchReg3;
    short conditionReg;
    Operand comparison;
    Operand leftOperand;
    Operand rightOperand;

    left = expr->data.diadic.left;
    right = expr->data.diadic.right;
    memclrw(&comparison, sizeof(comparison));
    if (copts.operandsDebug != 0 && left->rtype->type == TYPEFLOAT)
        CError_FATAL(3370);
    if (left->rtype->type != TYPEFLOAT) {
        memclrw(&leftOperand, sizeof(leftOperand));
        memclrw(&rightOperand, sizeof(rightOperand));
        if (right->hascall != 0) {
            data_00560648[right->type](right, 0, 0, &rightOperand);
            if (right->rtype->size < 4)
                Operands_ExtendGPR(&rightOperand, right->rtype, 0);
            if (rightOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&rightOperand, right->rtype, 0);
            data_00560648[left->type](left, 0, 0, &leftOperand);
            if (left->rtype->size < 4)
                Operands_ExtendGPR(&leftOperand, left->rtype, 0);
            if (leftOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
        } else {
            data_00560648[left->type](left, 0, 0, &leftOperand);
            if (leftOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
            if (left->rtype->size < 4)
                Operands_ExtendGPR(&leftOperand, left->rtype, 0);
            data_00560648[right->type](right, 0, 0, &rightOperand);
            if (right->rtype->size < 4)
                Operands_ExtendGPR(&rightOperand, right->rtype, 0);
            if (rightOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&rightOperand, right->rtype, 0);
        }
        if (expr->type == EEQU) {
            scratchReg1 = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_SUBF, scratchReg1, leftOperand.reg, rightOperand.reg);
            scratchReg2 = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_CNTLZW, scratchReg2, scratchReg1);
            resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_RLWINM, resultReg, scratchReg2, 0x1b, 5, 0x1f);
            output->kind = OpndType_GPR;
            output->reg = resultReg;
            return;
        }
        if (expr->type == ENOTEQU) {
            scratchReg1 = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_SUBF, scratchReg1, leftOperand.reg, rightOperand.reg);
            scratchReg2 = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_ADDIC, scratchReg2, scratchReg1, -1);
            resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_SUBFE, resultReg, scratchReg2, scratchReg1);
            output->kind = OpndType_GPR;
            output->reg = resultReg;
            return;
        }
        if (expr->type == ELESSEQU) {
            isUnsigned = Type_IsUnsigned(left->rtype);
            if (isUnsigned) {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_LI, scratchReg1, -1);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg2, leftOperand.reg, rightOperand.reg);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFZE, resultReg, scratchReg1);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            } else {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_RLWINM, scratchReg1, leftOperand.reg, 1, 0x1f, 0x1f);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SRAWI, scratchReg2, rightOperand.reg, 0x1f);
                scratchReg3 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg3, leftOperand.reg, rightOperand.reg);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADDE, resultReg, scratchReg2, scratchReg1);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            }
        }
        if (expr->type == EGREATEREQU) {
            isUnsigned = Type_IsUnsigned(left->rtype);
            if (isUnsigned) {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_LI, scratchReg1, -1);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg2, rightOperand.reg, leftOperand.reg);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFZE, resultReg, scratchReg1);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            } else {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_RLWINM, scratchReg1, rightOperand.reg, 1, 0x1f, 0x1f);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SRAWI, scratchReg2, leftOperand.reg, 0x1f);
                scratchReg3 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg3, rightOperand.reg, leftOperand.reg);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADDE, resultReg, scratchReg2, scratchReg1);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            }
        }
        if (expr->type == ELESS) {
            isUnsigned = Type_IsUnsigned(left->rtype);
            if (isUnsigned) {
                if (left->rtype->size <= 2) {
                    scratchReg1 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBF, scratchReg1, rightOperand.reg, leftOperand.reg);
                    PCodeUtilities_EmitInstruction(
                        PC_RLWINM, resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++,
                        scratchReg1, 1, 0x1f, 0x1f);
                } else {
                    scratchReg1 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg1, rightOperand.reg, leftOperand.reg);
                    scratchReg2 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBFE, scratchReg2, scratchReg2, scratchReg2);
                    PCodeUtilities_EmitInstruction(
                        PC_NEG, resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++, scratchReg2);
                }
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            } else {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg1, rightOperand.reg, leftOperand.reg);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_EQV, scratchReg2, rightOperand.reg, leftOperand.reg);
                scratchReg3 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_RLWINM, scratchReg3, scratchReg2, 1, 0x1f, 0x1f);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADDZE, resultReg, scratchReg3);
                PCodeUtilities_EmitInstruction(PC_RLWINM, resultReg, resultReg, 0, 0x1f, 0x1f);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            }
        }
        if (expr->type == EGREATER) {
            isUnsigned = Type_IsUnsigned(left->rtype);
            if (isUnsigned) {
                if (left->rtype->size <= 2) {
                    scratchReg1 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBF, scratchReg1, leftOperand.reg, rightOperand.reg);
                    PCodeUtilities_EmitInstruction(
                        PC_RLWINM, resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++,
                        scratchReg1, 1, 0x1f, 0x1f);
                } else {
                    scratchReg1 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg1, leftOperand.reg, rightOperand.reg);
                    scratchReg2 = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBFE, scratchReg2, scratchReg2, scratchReg2);
                    PCodeUtilities_EmitInstruction(
                        PC_NEG, resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++, scratchReg2);
                }
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            } else {
                scratchReg1 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBFC, scratchReg1, leftOperand.reg, rightOperand.reg);
                scratchReg2 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_EQV, scratchReg2, leftOperand.reg, rightOperand.reg);
                scratchReg3 = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_RLWINM, scratchReg3, scratchReg2, 1, 0x1f, 0x1f);
                resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADDZE, resultReg, scratchReg3);
                PCodeUtilities_EmitInstruction(PC_RLWINM, resultReg, resultReg, 0, 0x1f, 0x1f);
                output->kind = OpndType_GPR;
                output->reg = resultReg;
                return;
            }
        }
    }
    InstrSelection_SelectComparison(expr, &comparison);
    conditionReg = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_MFCR, conditionReg);
    invert = 0;
    scratchReg2 = comparison.reg * 4;
    switch (comparison.secondary_reg) {
        case ENOTEQU:
            invert = 1;
        case EEQU:
            scratchReg2 += 2;
            break;
        case EGREATEREQU:
            invert = 1;
            break;
        case ELESSEQU:
            invert = 1;
        case EGREATER:
            scratchReg2++;
            break;
        case ELESS:
            break;
    }
    resultReg = requestedReg != 0 ? requestedReg : gUsedVirtualRegistersGPR++;
    if (invert != 0) {
        scratchReg2++;
        PCodeUtilities_EmitInstruction(PC_RLWINM, conditionReg, conditionReg, scratchReg2, 0x1f, 0x1f);
        PCodeUtilities_EmitInstruction(PC_XORI, resultReg, conditionReg, 1);
    } else {
        scratchReg2++;
        PCodeUtilities_EmitInstruction(PC_RLWINM, resultReg, conditionReg, scratchReg2, 0x1f, 0x1f);
    }
    output->kind = OpndType_GPR;
    output->reg = resultReg;
}

void InstrSelection_SelectComparison(ENode *node, void *p)
{
    ENode *left = node->data.diadic.left;
    ENode *right = node->data.diadic.right;
    Operand *result = p;
    SInt32 value = 0;
    UInt32 unsignedValue = 0;

    if (copts.operandsDebug && IS_TYPE_FLOAT(left->rtype))
        CError_FATAL(3293);

    if (IS_TYPE_FLOAT(left->rtype)) {
        emit_fpr_comparison(node->type, left, right, result);
    } else {
        if (right->type == EINTCONST) {
            if (Type_IsUnsigned(left->rtype)) {
                UInt16 immediate;
                unsignedValue = right->data.intval.lo;
                immediate = unsignedValue;
                if (unsignedValue == immediate) {
                    InstrSelection_004b37b0(node->type, left, unsignedValue, result);
                    return;
                }
                if (node->type == EEQU || node->type == ENOTEQU) {
                    emit_cmpli_with_addis(node->type, left, unsignedValue, result);
                    return;
                }
            } else {
                SInt16 immediate;
                value = right->data.intval.lo;
                immediate = value;
                if (value == immediate) {
                    InstrSelection_004b37b0(node->type, left, value, result);
                    return;
                }
                if (node->type == EEQU || node->type == ENOTEQU) {
                    emit_cmpli_with_addis(node->type, left, value, result);
                    return;
                }
            }
        } else if (left->type == EINTCONST) {
            if (Type_IsUnsigned(right->rtype)) {
                UInt16 immediate;
                unsignedValue = left->data.intval.lo;
                immediate = unsignedValue;
                if (unsignedValue == immediate) {
                    InstrSelection_004b37b0(SwapOp(node->type), right, unsignedValue, result);
                    return;
                }
                if (node->type == EEQU || node->type == ENOTEQU) {
                    emit_cmpli_with_addis(SwapOp(node->type), right, unsignedValue, result);
                    return;
                }
            } else {
                SInt16 immediate;
                value = left->data.intval.lo;
                immediate = value;
                if (value == immediate) {
                    InstrSelection_004b37b0(SwapOp(node->type), right, value, result);
                    return;
                }
                if (node->type == EEQU || node->type == ENOTEQU) {
                    emit_cmpli_with_addis(SwapOp(node->type), right, value, result);
                    return;
                }
            }
        }
        emit_gpr_comparison(node->type, left, right, result);
    }
}

unsigned int swap_kind_pairs(unsigned int kind)
{
    switch (kind) {
        case 0x13:
            return 0x14;
        case 0x14:
            return 0x13;
        case 0x15:
            return 0x16;
        case 0x16:
            return 0x15;
        default:
            return kind;
    }
}

unsigned char fn_004b4aa0(unsigned char kind)
{
    switch (kind) {
        case 0x13:
            return 0x16;
        case 0x14:
            return 0x15;
        case 0x15:
            return 0x14;
        case 0x16:
            return 0x13;
        default:
            return kind;
    }
}

/* Result produced by comparison instruction selection. */

void generate_condition_branches(ENode *expr, PCodeLabel *trueLabel, PCodeLabel *falseLabel,
                                 PCodeLabel *fallthroughLabel)
{
    Operand result;
    Operand discardedResult;

    memclrw(&result, sizeof(result));

    while (expr->type == EFORCELOAD || expr->type == ETYPCON || expr->type == ECOMMA) {
        if (!(expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM || expr->rtype->type == TYPEPOINTER ||
              (expr->rtype->type == TYPEMEMBERPOINTER && expr->rtype->size == 4)))
            break;
        if (expr->type == ECOMMA) {
            memclrw(&discardedResult, sizeof(discardedResult));
            data_00560648[expr->data.diadic.left->type](expr->data.diadic.left, 0, 0, &discardedResult);
            expr = expr->data.diadic.right;
        } else {
            expr = expr->data.diadic.left;
        }
    }

    switch (expr->type) {
        case ELAND: {
            PCodeLabel *nextLabel = PCode_NewLabel();
            generate_condition_branches(expr->data.diadic.left, nextLabel, falseLabel, nextLabel);
            PCodeUtilities_ResolveLabel(nextLabel);
            generate_condition_branches(expr->data.diadic.right, trueLabel, falseLabel, fallthroughLabel);
            break;
        }
        case ELOR: {
            PCodeLabel *nextLabel = PCode_NewLabel();
            generate_condition_branches(expr->data.diadic.left, trueLabel, nextLabel, nextLabel);
            PCodeUtilities_ResolveLabel(nextLabel);
            generate_condition_branches(expr->data.diadic.right, trueLabel, falseLabel, fallthroughLabel);
            break;
        }
        case ELOGNOT:
            generate_condition_branches(expr->data.diadic.left, falseLabel, trueLabel, fallthroughLabel);
            break;
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU: {
            Boolean useGPR;
            if (((useGPR = copts.operandsDebug) && expr->data.diadic.right->rtype->type == TYPEFLOAT) ||
                (useGPR && expr->data.diadic.left->rtype->type == TYPEFLOAT))
                SFPE_PPC_EABI_GenerateComparison(expr, &result, 0);
            else if (((expr->data.diadic.right->rtype->type == TYPEINT ||
                       expr->data.diadic.right->rtype->type == TYPEENUM) &&
                      expr->data.diadic.right->rtype->size == 8) ||
                     ((expr->data.diadic.left->rtype->type == TYPEINT ||
                       expr->data.diadic.left->rtype->type == TYPEENUM) &&
                      expr->data.diadic.left->rtype->size == 8))
                InstrSelection_GenerateLongLongComparison(expr, &result, 0);
            else
                InstrSelection_SelectComparison(expr, &result);
            if (fallthroughLabel == trueLabel)
                PCodeUtilities_EmitConditionBranch(result.reg, result.secondary_reg, 0, falseLabel);
            else
                PCodeUtilities_EmitConditionBranch(result.reg, result.secondary_reg, 1, trueLabel);
            break;
        }
        default:
            CError_FATAL(3220);
            break;
    }
}

void get_function_type_operand(ENode *lookup, short unused1, short unused2, Operand *result)
{
    struct FunctionCallFrame *entry = function_call_frames;

    while (entry != NULL) {
        if (entry->functionType == (TypeFunc *)lookup->data.longval)
            break;
        entry = entry->next;
    }
    *result = entry->operand;
}

/* Saved operand for a nested function call. */

void emit_conditional_funccall(ENode *expr, SInt32 requestedReg, SInt32 requestedRegHi, struct Operand *out)
{
    Operand functionOperand;
    Operand argumentOperand;
    ENode *functionRef;
    ENode *arguments;
    PCodeLabel *endLabel;
    SInt32 resultReg;
    struct FunctionCallFrame *frame;
    SInt32 needsResult;

    functionRef = expr->data.funccall.funcref;
    arguments = (ENode *)expr->data.funccall.args;

    memclrw(&functionOperand, sizeof(functionOperand));
    memclrw(&argumentOperand, sizeof(argumentOperand));

    needsResult = 0;
    if (IS_NonVoid(expr->rtype) && IS_NotIgnored(expr))
        needsResult = 1;

    data_00560648[functionRef->type](functionRef, 0, 0, &functionOperand);
    if (functionRef->rtype->size < 4)
        Operands_ExtendGPR(&functionOperand, functionRef->rtype, 0);
    if (functionOperand.kind != OpndType_GPR)
        Operands_ForceGPR(&functionOperand, functionRef->rtype, 0);

    frame = (struct FunctionCallFrame *)CompilerTools_AllocatePool(sizeof(frame->next) + sizeof(frame->functionType) +
                                                                   sizeof(frame->operand));
    frame->functionType = expr->data.funccall.functype;
    frame->operand = functionOperand;
    frame->next = function_call_frames;
    function_call_frames = frame;

    PCodeUtilities_EmitInstruction(PC_CMPI, 0, functionOperand.reg, 0);

    if (needsResult) {
        PCodeUtilities_EmitInstruction(PC_MR, resultReg = gUsedVirtualRegistersGPR++, functionOperand.reg);
    }

    endLabel = PCode_NewLabel();
    PCodeUtilities_EmitConditionBranch(0, 0x17, 1, endLabel);
    data_00560648[arguments->type](arguments, 0, 0, &argumentOperand);
    function_call_frames = function_call_frames->next;

    if (needsResult) {
        if (argumentOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&argumentOperand, arguments->rtype, resultReg);
        if (argumentOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_MR, resultReg, argumentOperand.reg);
        out->kind = OpndType_GPR;
        out->reg = (SInt16)resultReg;
    }

    PCodeUtilities_ResolveLabel(endLabel);
}

void generate_boolean_expression(ENode *expression, int unused1, int unused2, Operand *result)
{
    ENode *node;
    int resultReg;
    PCodeLabel *falseLabel;
    int negatedReg;
    ENode *operand;
    int intermediateReg;
    int temporaryReg;
    PCodeLabel *trueLabel;
    int booleanReg;
    UInt8 reversedKind;
    char originalKind, comparisonKind;
    Operand scratch;

    node = expression->data.monadic;
    while (node->type == EFORCELOAD || node->type == ETYPCON || node->type == ECOMMA) {
        if (node->rtype->type != TYPEINT && node->rtype->type != TYPEENUM && node->rtype->type != TYPEPOINTER &&
            (node->rtype->type != TYPEMEMBERPOINTER || node->rtype->size != 4))
            break;
        if (node->type == ECOMMA)
            node = node->data.diadic.right;
        else
            node = node->data.monadic;
    }
    if (expression->type == ELOGNOT && node->type != ELAND && node->type != ELOR) {
        operand = expression->data.monadic;
        originalKind = operand->type;
        while (operand->type == EFORCELOAD || operand->type == ETYPCON || operand->type == ECOMMA) {
            if (operand->rtype->type != TYPEINT && operand->rtype->type != TYPEENUM &&
                operand->rtype->type != TYPEPOINTER &&
                (operand->rtype->type != TYPEMEMBERPOINTER || operand->rtype->size != 4))
                break;
            if (operand->type == ECOMMA) {
                memclrw(&scratch, sizeof(scratch));
                data_00560648[operand->data.diadic.left->type](operand->data.diadic.left, 0, 0, &scratch);
                operand = operand->data.diadic.right;
            } else
                operand = operand->data.monadic;
        }
        if (operand->type == ELOGNOT) {
            switch (operand->data.monadic->type) {
                case ELOGNOT:
                case ELESS:
                case EGREATER:
                case ELESSEQU:
                case EGREATEREQU:
                case EEQU:
                case ENOTEQU:
                case ELAND:
                case ELOR:
                    data_00560648[operand->data.monadic->type](operand->data.monadic, 0, 0, result);
                    if (expression->data.monadic->rtype->size < 4)
                        Operands_ExtendGPR(result, expression->data.monadic->rtype, 0);
                    if (result->kind != OpndType_GPR)
                        Operands_ForceGPR(result, expression->data.monadic->rtype, 0);
                    return;
            }
        }
        if (operand->type == ENOTEQU &&
            ((operand->data.diadic.left->rtype->type != TYPEINT && operand->data.diadic.left->rtype->type != TYPEENUM ||
              operand->data.diadic.left->rtype->size != 8)) &&
            operand->data.diadic.right->type == EINTCONST && operand->data.diadic.right->data.intval.lo == 0) {
            data_00560648[operand->data.diadic.left->type](operand->data.diadic.left, 0, 0, result);
            if (operand->data.diadic.left->rtype->size < 4)
                Operands_ExtendGPR(result, operand->data.diadic.left->rtype, 0);
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, operand->data.diadic.left->rtype, 0);
            resultReg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR += 1;
            intermediateReg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR += 1;
            if (result->kind != OpndType_GPR)
                CError_FATAL(2902);
            PCodeUtilities_EmitInstruction(PC_CNTLZW, intermediateReg, result->reg);
            PCodeUtilities_EmitInstruction(PC_RLWINM, resultReg, intermediateReg, 27, 5, 31);
            result->kind = OpndType_GPR;
            result->reg = resultReg;
        } else {
            switch ((UInt8)originalKind) {
                case ELESS:
                    reversedKind = EGREATEREQU;
                    break;
                case EGREATER:
                    reversedKind = ELESSEQU;
                    break;
                case ELESSEQU:
                    reversedKind = EGREATER;
                    break;
                case EGREATEREQU:
                    reversedKind = ELESS;
                    break;
                default:
                    reversedKind = originalKind;
            }
            if (originalKind != (comparisonKind = reversedKind) &&
                operand->data.diadic.left->rtype->type != TYPEFLOAT) {
                operand->type = reversedKind;
                generate_comparison(operand, 0, 0, result);
                operand->type = reversedKind;
                return;
            }
            data_00560648[operand->type](operand, 0, 0, result);
            if (operand->rtype->size < 4)
                Operands_ExtendGPR(result, operand->rtype, 0);
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, operand->rtype, 0);
            negatedReg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR += 1;
            temporaryReg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR += 1;
            if (result->kind != OpndType_GPR)
                CError_FATAL(2932);
            PCodeUtilities_EmitInstruction(PC_CNTLZW, temporaryReg, result->reg);
            PCodeUtilities_EmitInstruction(PC_RLWINM, negatedReg, temporaryReg, 27, 5, 31);
            result->kind = OpndType_GPR;
            result->reg = negatedReg;
        }
    } else {
        trueLabel = PCode_NewLabel();
        falseLabel = PCode_NewLabel();
        booleanReg = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1;
        PCodeUtilities_EmitInstruction(PC_LI, booleanReg, 0);
        generate_condition_branches(expression, trueLabel, falseLabel, trueLabel);
        PCodeUtilities_ResolveLabel(trueLabel);
        PCodeUtilities_EmitInstruction(PC_LI, booleanReg, 1);
        PCodeUtilities_ResolveLabel(falseLabel);
        result->kind = OpndType_GPR;
        result->reg = booleanReg;
    }
}

void generate_comparison(ENode *expr, SInt32 requestedReg, SInt32 requestedRegHi, Operand *output)
{
    Boolean useSoftwareFloat;
    if (((useSoftwareFloat = copts.operandsDebug) && expr->data.diadic.right->rtype->type == TYPEFLOAT) ||
        (useSoftwareFloat && expr->data.diadic.left->rtype->type == TYPEFLOAT)) {
        SFPE_PPC_EABI_GenerateComparison(expr, output, 1);
        return;
    }
    if (((expr->data.diadic.right->rtype->type == TYPEINT || expr->data.diadic.right->rtype->type == TYPEENUM) &&
         expr->data.diadic.right->rtype->size == 8) ||
        ((expr->data.diadic.left->rtype->type == TYPEINT || expr->data.diadic.left->rtype->type == TYPEENUM) &&
         expr->data.diadic.left->rtype->size == 8)) {
        InstrSelection_GenerateLongLongComparison(expr, output, 1);
        return;
    }
    generate_comparison_gpr(expr, output, requestedReg);
}

void InstrSelection_EmitThreeOperandFPRInstruction(SInt16 opcode, ENode *left, ENode *middle, ENode *right,
                                                   SInt16 targetReg, Operand *result)
{
    Operand leftOperand;
    Operand middleOperand;
    Operand rightOperand;
    SInt16 virtualReg;
    SInt32 resultReg;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&middleOperand, sizeof(middleOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        GenOperand(right, &rightOperand);
        if (middle->hascall) {
            GenOperand(middle, &middleOperand);
            GenOperand(left, &leftOperand);
        } else {
            GenOperand(left, &leftOperand);
            GenOperand(middle, &middleOperand);
        }
    } else {
        if (middle->hascall) {
            GenOperand(middle, &middleOperand);
            GenOperand(left, &leftOperand);
            GenOperand(right, &rightOperand);
        } else {
            GenOperand(left, &leftOperand);
            GenOperand(middle, &middleOperand);
            GenOperand(right, &rightOperand);
        }
    }

    if (targetReg != 0)
        virtualReg = targetReg;
    else
        virtualReg = gUsedVirtualRegistersFPR++;
    resultReg = virtualReg;
    PCodeUtilities_EmitInstruction(opcode, resultReg, leftOperand.reg, middleOperand.reg, rightOperand.reg);
    result->kind = OpndType_FPR;
    result->reg = resultReg;
}

/* sizeof = 22 (0x16) */

void InstrSelection_EmitUnaryFPRInstruction(SInt16 opcode, ENode *node, SInt16 reg, Operand *res)
{
    Operand op;
    SInt32 virtualRegister;
    memclrw(&op, sizeof(op));
    data_00560648[node->type](node, 0, 0, &op);
    if (op.kind != OpndType_FPR)
        Operands_ForceFPR(&op, node->rtype, 0);
    virtualRegister = (SInt16)(reg ? reg : gUsedVirtualRegistersFPR++);
    PCodeUtilities_EmitInstruction(opcode, virtualRegister, op.reg);
    res->kind = OpndType_FPR;
    res->reg = virtualRegister;
}

/* size 24 */

void emit_binary_fpr_instruction(short opcode, ENode *left, ENode *right, SInt16 requestedReg, Operand *result)
{
    Operand leftOperand;
    Operand rightOperand;
    int resultReg;
    Type *type;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->hascall) {
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_FPR) {
            type = right->rtype;
            Operands_ForceFPR(&rightOperand, type, 0);
        }
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_FPR) {
            type = left->rtype;
            Operands_ForceFPR(&leftOperand, type, 0);
        }
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_FPR) {
            type = left->rtype;
            Operands_ForceFPR(&leftOperand, type, 0);
        }
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_FPR) {
            type = right->rtype;
            Operands_ForceFPR(&rightOperand, type, 0);
        }
    }

    resultReg = requestedReg ? requestedReg : gUsedVirtualRegistersFPR++;

    PCodeUtilities_EmitInstruction(opcode, resultReg, leftOperand.reg, rightOperand.reg);
    result->kind = OpndType_FPR;
    result->reg = resultReg;
}

/* sizeof == 0x16 */

void emit_gpr_immediate_operation(SInt16 opcode, ENode *expr, SInt32 value, SInt16 outputReg, Operand *output)
{
    Operand operand;
    SInt32 reg;

    memclrw(&operand, sizeof(operand));
    ((void (*)(ENode *, SInt32, SInt32, Operand *))data_00560648[expr->type])(expr, 0, 0, &operand);
    if (operand.kind != OpndType_GPR)
        Operands_ForceGPR(&operand, expr->rtype, 0);

    reg = outputReg ? outputReg : gUsedVirtualRegistersGPR++;

    if (expr->rtype->size > 2 && value != (UInt16)value) {
        if ((value & 0xffff) != 0) {
            PCodeUtilities_EmitInstruction((opcode == 0x58) ? 0x59 : 0x5b, reg, operand.reg, value >> 16);
            PCodeUtilities_EmitInstruction(opcode, reg, reg, value);
        } else {
            PCodeUtilities_EmitInstruction((opcode == 0x58) ? 0x59 : 0x5b, reg, operand.reg, value >> 16);
        }
    } else {
        PCodeUtilities_EmitInstruction(opcode, reg, operand.reg, value);
    }

    output->kind = OpndType_GPR;
    output->reg = reg;
}

void InstrSelection_EmitUnaryGPRInstruction(SInt16 opcode, ENode *expression, SInt16 targetReg, Operand *result)
{
    Operand operand;
    SInt32 reg;

    memclrw(&operand, sizeof(operand));
    data_00560648[expression->type](expression, 0, 0, &operand);
    if (operand.kind) {
        Operands_ForceGPR(&operand, expression->rtype, 0);
    }
    reg = targetReg ? targetReg : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(opcode, reg, operand.reg);
    result->kind = OpndType_GPR;
    result->reg = reg;
}

void emit_gpr_immediate_instruction(short opcode, ENode *expr, short value, short outputReg, Operand *output)
{
    Operand input;
    short allocatedRegister;
    int destinationRegister;

    memclrw(&input, sizeof(input));
    data_00560648[expr->type](expr, 0, 0, &input);
    if (input.kind != OpndType_GPR) {
        Operands_ForceGPR(&input, expr->rtype, 0);
    }
    if (outputReg != 0) {
        allocatedRegister = outputReg;
    } else {
        allocatedRegister = gUsedVirtualRegistersGPR++;
    }
    destinationRegister = allocatedRegister;
    if ((opcode == 0x49) && (value == 0)) {
        PCodeUtilities_EmitInstruction(PC_LI, destinationRegister, 0);
    } else if ((opcode == 0x49) && (value == 1)) {
        PCodeUtilities_EmitInstruction(PC_MR, destinationRegister, input.reg);
    } else {
        PCodeUtilities_EmitInstruction(opcode, destinationRegister, input.reg, value);
    }
    output->kind = OpndType_GPR;
    output->reg = destinationRegister;
}

void InstrSelection_EmitBinaryGPRInstruction(short opcode, ENode *left, ENode *right, short outputReg, Operand *output)
{
    short reg;
    int emitReg;
    int smallInteger;
    Type *type;
    int reverseOperands;
    int integral;
    ENode *expr;
    int narrow;
    Operand leftOperand;
    Operand rightOperand;
    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));
    if (right->hascall != 0) {
        InstrSelection_004b5ae0_inline1(right, &rightOperand);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        InstrSelection_004b5ae0_inline1(left, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
    } else {
        InstrSelection_004b5ae0_inline1(left, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        InstrSelection_004b5ae0_inline1(right, &rightOperand);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
    }
    if (outputReg != 0)
        reg = outputReg;
    else
        reg = gUsedVirtualRegistersGPR++;
    emitReg = reg;
    do {
        if (opcode == 74) {
            expr = left;
            type = left->rtype;
            if (left->type != ETYPCON) {
                reverseOperands = 0;
            } else {
                do {
                    expr = expr->data.monadic;
                    if (expr->type != ETYPCON)
                        break;
                } while ((type = expr->rtype)->size == 4);
                reverseOperands = 0;
                integral = 1;
                if (type->type != TYPEINT && type->type != TYPEENUM)
                    integral = 0;
                if (integral != 0) {
                    narrow = 1;
                    if (type->size >= 2) {
                        smallInteger = type->size == 2 && !Type_IsUnsigned(type);
                        if (!smallInteger)
                            narrow = 0;
                    }
                    if (narrow)
                        reverseOperands = 1;
                }
            }
            if (reverseOperands) {
                PCodeUtilities_EmitInstruction(opcode, emitReg, rightOperand.reg, leftOperand.reg);
                break;
            }
        }
        PCodeUtilities_EmitInstruction(opcode, emitReg, leftOperand.reg, rightOperand.reg);
    } while (0);
    output->kind = OpndType_GPR;
    output->reg = emitReg;
}

unsigned int fn_004b5ce0(void)
{
    CError_FATAL(2298);
}

void make_objref_operand(ENode *node, unsigned int unused1, unsigned int unused2, Operand *result)
{
    VarInfo *info;
    Object *object = node->data.objref;
    int physical_register;

    result->kind = OpndType_Symbol;
    result->reg = 0;
    result->object = object;
    result->displacement = 0;
    if (object->datatype == DDATA) {
        info = Registers_GetInfo(object);
        if (info)
            physical_register = Registers_GetInfo(object)->reg;
        else
            physical_register = 0;
        if (physical_register) {
            info = Registers_GetInfo(object);
            result->kind = OpndType_GPR;
            result->reg = info->reg;
            result->object = NULL;
        }
    }
}

unsigned int generate_intrinsic_or_function_call(ENode *value, unsigned int operand, unsigned int unused,
                                                 Operand *target)
{
    if (Intrinsics_IsMonadicObjrefTypeFuncFlag200Set(value))
        Intrinsics_GenerateIntrinsicCall(value, operand, target);
    else
        FunctionCalls_GenerateCall(value, target);
}

/* 0x58846c, word */
/* 0x58846e, word */

void generate_conditional_expression(ENode *node, SInt16 outputReg, SInt16 outputRegHi, Operand *output)
{
    ENode *trueExpr;
    ENode *falseExpr;
    Type *type;
    PCodeLabel *trueLabel;
    PCodeLabel *falseLabel;
    PCodeLabel *endLabel;
    Operand trueOperand;
    Operand falseOperand;
    int resultReg;
    int resultRegHi;
    int structKind;

    trueExpr = node->data.cond.expr1;
    falseExpr = node->data.cond.expr2;
    type = node->rtype;
    trueLabel = PCode_NewLabel();
    falseLabel = PCode_NewLabel();
    endLabel = PCode_NewLabel();
    memclrw(&trueOperand, sizeof(trueOperand));
    memclrw(&falseOperand, sizeof(falseOperand));
    generate_condition_branches(node->data.cond.cond, trueLabel, falseLabel, trueLabel);
    PCodeUtilities_ResolveLabel(trueLabel);

    if (type->type == TYPEVOID || node->ignored != 0) {
        data_00560648[trueExpr->type](trueExpr, 0, 0, &trueOperand);
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, 0, 0, &falseOperand);
    } else if (type->type == TYPEFLOAT && !(copts.operandsDebug && type->type == TYPEFLOAT)) {
        if (trueExpr->hascall != 0 || falseExpr->hascall != 0)
            resultReg = gUsedVirtualRegistersFPR++;
        else
            resultReg = outputReg != 0 ? outputReg : gUsedVirtualRegistersFPR++;
        data_00560648[trueExpr->type](trueExpr, resultReg, 0, &trueOperand);
        if (trueOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&trueOperand, trueExpr->rtype, resultReg);
        if (trueOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_FMR, resultReg, trueOperand.reg);
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, resultReg, 0, &falseOperand);
        if (falseOperand.kind != OpndType_FPR)
            Operands_ForceFPR(&falseOperand, falseExpr->rtype, resultReg);
        if (falseOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_FMR, resultReg, falseOperand.reg);
        output->kind = OpndType_FPR;
        output->reg = resultReg;
    } else if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
               (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4)) {
        if (trueExpr->hascall != 0 || falseExpr->hascall != 0) {
            resultReg = gUsedVirtualRegistersGPR++;
            resultRegHi = gUsedVirtualRegistersGPR++;
        } else {
            resultReg = outputReg != 0 ? outputReg : gUsedVirtualRegistersGPR++;
            resultRegHi = outputRegHi != 0 ? outputRegHi : gUsedVirtualRegistersGPR++;
        }
        data_00560648[trueExpr->type](trueExpr, resultReg, resultRegHi, &trueOperand);
        Operands_ForceGPRPair(&trueOperand, trueExpr->rtype, resultReg, resultRegHi);
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, resultReg, resultRegHi, &falseOperand);
        Operands_ForceGPRPair(&falseOperand, falseExpr->rtype, resultReg, resultRegHi);
        output->kind = OpndType_GPRPair;
        output->reg = resultReg;
        output->regHi = resultRegHi;
    } else if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
               (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
               (copts.operandsDebug && type->type == TYPEFLOAT && type->size == 4)) {
        if (trueExpr->hascall != 0 || falseExpr->hascall != 0)
            resultReg = gUsedVirtualRegistersGPR++;
        else
            resultReg = outputReg != 0 ? outputReg : gUsedVirtualRegistersGPR++;
        data_00560648[trueExpr->type](trueExpr, resultReg, 0, &trueOperand);
        if (trueOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&trueOperand, trueExpr->rtype, resultReg);
        if (trueOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_MR, resultReg, trueOperand.reg);
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, resultReg, 0, &falseOperand);
        if (falseOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&falseOperand, falseExpr->rtype, resultReg);
        if (falseOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_MR, resultReg, falseOperand.reg);
        output->kind = OpndType_GPR;
        output->reg = resultReg;
    } else if (type->type == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= 4 && structKind <= 14) {
        if (trueExpr->hascall != 0 || falseExpr->hascall != 0)
            resultReg = gUsedVirtualRegistersVR++;
        else
            resultReg = outputReg != 0 ? outputReg : gUsedVirtualRegistersVR++;
        data_00560648[trueExpr->type](trueExpr, resultReg, 0, &trueOperand);
        if (trueOperand.kind != OpndType_VR)
            Operands_ForceVR(&trueOperand, trueExpr->rtype, resultReg);
        if (trueOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_VMR, resultReg, trueOperand.reg);
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, resultReg, 0, &falseOperand);
        if (falseOperand.kind != OpndType_VR)
            Operands_ForceVR(&falseOperand, falseExpr->rtype, resultReg);
        if (falseOperand.reg != resultReg)
            PCodeUtilities_EmitInstruction(PC_VMR, resultReg, falseOperand.reg);
        output->kind = OpndType_VR;
        output->reg = resultReg;
    } else {
        output->kind = OpndType_IndirectGPR_ImmOffset;
        output->reg = stack_base_reg;
        output->object = CodeGen_AllocateTemporaryObject(type);
        output->displacement = 0;
        data_00560648[trueExpr->type](trueExpr, 0, 0, &trueOperand);
        if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && !Type_RequiresMemoryReturn(type)) {
            if (trueOperand.kind == OpndType_GPRPair) {
                Operands_StoreGPRPair(trueOperand.reg, trueOperand.regHi, output, (Type *)&stunsignedlonglong);
            } else if (trueOperand.kind == OpndType_GPR) {
                Operands_EmitTypedGPRMemoryInstruction(trueOperand.reg, output, (Type *)&stunsignedlong);
            } else
                StructMoves_EmitCopy(output, &trueOperand, type->size, StackFrameEABI_GetTypeAlignment(type));
        } else {
            StructMoves_EmitCopy(output, &trueOperand, type->size, StackFrameEABI_GetTypeAlignment(type));
        }
        PCodeUtilities_EmitBranch(endLabel);
        PCodeUtilities_ResolveLabel(falseLabel);
        data_00560648[falseExpr->type](falseExpr, 0, 0, &falseOperand);
        if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && !Type_RequiresMemoryReturn(type)) {
            if (falseOperand.kind == OpndType_GPRPair) {
                Operands_StoreGPRPair(falseOperand.reg, falseOperand.regHi, output, (Type *)&stunsignedlonglong);
            } else if (falseOperand.kind == OpndType_GPR) {
                Operands_EmitTypedGPRMemoryInstruction(falseOperand.reg, output, (Type *)&stunsignedlong);
            } else
                StructMoves_EmitCopy(output, &falseOperand, type->size, StackFrameEABI_GetTypeAlignment(type));
        } else {
            StructMoves_EmitCopy(output, &falseOperand, type->size, StackFrameEABI_GetTypeAlignment(type));
        }
    }
    PCodeUtilities_ResolveLabel(endLabel);
}

void fn_004b6530(void)
{
    CError_FATAL(1996);
}

void load_float_constant(ENode *node, unsigned int regA, unsigned int regB, Operand *out)
{
    if (copts.operandsDebug) {
        SFPE_PPC_EABI_LoadFloatConstant(node, regA, regB, out);
    } else {
        CError_FATAL(1982);
    }
}

/* 0x560774, "InstrSelection.c" */
/* Register-pair or pointer result of instruction selection. */

void make_intval_operand(ENode *node, SInt16 reg1, SInt16 reg2, Operand *op)
{
    Type *type = node->rtype;
    if (copts.operandsDebug && type->type == TYPEFLOAT) {
        CError_FATAL(1951);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        reg1 = reg1 ? reg1 : gUsedVirtualRegistersGPR++;
        reg2 = reg2 ? reg2 : gUsedVirtualRegistersGPR++;
        PCodeUtilities_LoadImmediate(reg1, node->data.intval.lo);
        PCodeUtilities_LoadImmediate(reg2, node->data.intval.hi);
        op->kind = OpndType_GPRPair;
        op->reg = reg1;
        op->regHi = reg2;
        return;
    }
    op->kind = OpndType_Immediate;
    op->immediate = node->data.intval.lo;
}

void report_fatal_error(void)
{
    CError_FATAL(1933);
}

enum TypeSubtypeBound { TypeSubtype_Min = 4, TypeSubtype_Max = 0xe };

/* Compiler options block at 0x584220. The flag is read as a field: IRO
   CSEs a field access (ADD of a non-zero offset) but not a plain global,
   so the two tests share one byte register. */

void generate_type_conversion(ENode *node, short outputReg, short outputRegHi, Operand *result)
{
    ENode *expr = node->data.monadic;
    Type *exprtype = expr->rtype;
    Type *target = node->rtype;

    if ((copts.operandsDebug && exprtype->type == TYPEFLOAT) || (copts.operandsDebug && target->type == TYPEFLOAT)) {
        SFPE_PPC_EABI_GenerateConversion(node, outputReg, outputRegHi, result);
        return;
    }
    if (((exprtype->type == TYPEINT || exprtype->type == TYPEENUM) && exprtype->size == 8) ||
        ((target->type == TYPEINT || target->type == TYPEENUM) && target->size == 8)) {
        generate_gpr_pair_type_conversion(node, outputReg, outputRegHi, result);
        return;
    }
    if (target->type == TYPEVOID) {
        data_00560648[expr->type](expr, 0, 0, result);
        if (expr->type == EINDIRECT && (result->flags & fIsVolatile) != 0) {
            if (exprtype->type == TYPEINT || exprtype->type == TYPEENUM || exprtype->type == TYPEPOINTER ||
                (exprtype->type == TYPEMEMBERPOINTER && exprtype->size == 4) ||
                (copts.operandsDebug && exprtype->type == TYPEFLOAT && exprtype->size == 4)) {
                if (result->kind != OpndType_GPR)
                    Operands_ForceGPR(result, exprtype, 0);
            } else if (exprtype->type == TYPEFLOAT) {
                if (result->kind != OpndType_FPR)
                    Operands_ForceFPR(result, exprtype, 0);
            }
        }
    } else if (exprtype->type == TYPEINT || exprtype->type == TYPEENUM) {
        if (target->type == TYPEFLOAT) {
            data_00560648[expr->type](expr, 0, 0, result);
            if (exprtype->size < 4)
                Operands_ExtendGPR(result, exprtype, 0);
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, exprtype, 0);
            if (Type_IsUnsigned(exprtype))
                Operands_ConvertIntegerToFloat(result, target->size == 4, outputReg);
            else
                Operands_ConvertSignedIntegerToFloat(result, target->size == 4, outputReg);
        } else if (target->type == TYPESTRUCT && TYPE_STRUCT(target)->stype >= TypeSubtype_Min &&
                   TYPE_STRUCT(target)->stype <= TypeSubtype_Max) {
            data_00560648[expr->type](expr, outputReg, 0, result);
            if (result->kind != OpndType_VR)
                Operands_ForceVR(result, target, outputReg);
        } else if (exprtype->size < target->size &&
                   !((expr->type == EINDIRECT && expr->data.monadic->type == EBITFIELD) ||
                     ((expr->type >= EASS && expr->type <= EORASS) &&
                      expr->data.monadic->data.monadic->type == EBITFIELD) ||
                     ((expr->type >= EPOSTINC && expr->type <= EPREDEC) &&
                      expr->data.monadic->data.monadic->type == EBITFIELD))) {
            data_00560648[expr->type](expr, 0, 0, result);
            Operands_ExtendGPR(result, exprtype, outputReg);
        } else if (target->size < exprtype->size) {
            data_00560648[expr->type](expr, 0, 0, result);
            if (result->kind != OpndType_GPR)
                Operands_ForceGPR(result, exprtype, 0);
            Operands_ExtendGPR(result, target, outputReg);
        } else {
            data_00560648[expr->type](expr, outputReg, 0, result);
        }
    } else if (exprtype->type == TYPEPOINTER) {
        data_00560648[expr->type](expr, outputReg, 0, result);
        if (target->size < exprtype->size && result->kind != OpndType_GPR)
            Operands_ForceGPR(result, exprtype, outputReg);
    } else if (exprtype->type == TYPEFLOAT) {
        if (target->type == TYPEFLOAT) {
            int reg;

            data_00560648[expr->type](expr, outputReg, 0, result);
            if (result->kind != OpndType_FPR)
                Operands_ForceFPR(result, exprtype, outputReg);
            if (target->size == 4 && exprtype->size != 4) {
                reg = outputReg ? outputReg : gUsedVirtualRegistersFPR++;
                PCodeUtilities_EmitInstruction(PC_FRSP, reg, result->reg);
                result->kind = OpndType_FPR;
                result->reg = reg;
            }
        } else if (Type_IsUnsigned(target) && target->size == 4) {
            data_00560648[expr->type](expr, 1, 0, result);
            if (result->kind != OpndType_FPR)
                Operands_ForceFPR(result, exprtype, 1);
            Operands_MoveToNewGPR(result, outputReg);
        } else {
            data_00560648[expr->type](expr, 0, 0, result);
            if (result->kind != OpndType_FPR)
                Operands_ForceFPR(result, exprtype, 0);
            Operands_ConvertFloatToInteger(result, outputReg);
        }
    } else if (exprtype->type == TYPESTRUCT && TYPE_STRUCT(exprtype)->stype >= TypeSubtype_Min &&
               TYPE_STRUCT(exprtype)->stype <= TypeSubtype_Max && target->type == TYPESTRUCT &&
               TYPE_STRUCT(target)->stype >= TypeSubtype_Min && TYPE_STRUCT(target)->stype <= TypeSubtype_Max) {
        data_00560648[expr->type](expr, outputReg, 0, result);
        if (result->kind != OpndType_VR)
            Operands_ForceVR(result, exprtype, outputReg);
    } else {
        CError_FATAL(1919);
    }
}

void select_diadic_left_then_right(ENode *node, SInt32 a, SInt32 b, Operand *ctx)
{
    ENode *left = node->data.diadic.left;
    ENode *right = node->data.diadic.right;

    data_00560648[left->type](left, 0, 0, ctx);

    if (left->type == EINDIRECT && (ctx->flags & fIsVolatile)) {
        UInt8 kind;
        if ((kind = left->rtype->type) == TYPEINT || kind == TYPEENUM || kind == TYPEPOINTER ||
            (kind == TYPEMEMBERPOINTER && left->rtype->size == 4) ||
            (copts.operandsDebug && kind == TYPEFLOAT && left->rtype->size == 4)) {
            if (ctx->kind != OpndType_GPR)
                Operands_ForceGPR(ctx, left->rtype, 0);
        } else if (kind == TYPEFLOAT && ctx->kind != OpndType_FPR) {
            Operands_ForceFPR(ctx, left->rtype, 0);
        }
    }

    data_00560648[right->type](right, a, 0, ctx);
}

/* Register-colouring record returned by Registers_GetInfo. */

/* Struct/class view: signed byte at 0x0e. */

static void EmitAddImm(SInt16 dst, SInt16 src, SInt32 imm)
{
    if (imm != (SInt16)imm) {
        PCodeUtilities_EmitInstruction(PC_ADDIS, dst, src, 0, (SInt16)((imm >> 16) + ((imm >> 15) & 1)));
        if ((SInt16)imm != 0)
            PCodeUtilities_EmitInstruction(PC_ADDI, dst, dst, 0, (SInt16)imm);
    } else {
        PCodeUtilities_EmitInstruction(PC_ADDI, dst, src, 0, imm);
    }
}

void generate_assignment(ENode *node, SInt16 requestedRegister, SInt16 flags, Operand *out)
{
    Operand destination, value, bitfield;
    ENode *left;
    ENode *right;
    Type *type;
    TypeBitfield *bitfieldType;
    SInt32 displacement;
    SInt32 alignment;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    type = node->rtype;

    memclrw(&destination, sizeof(destination));
    memclrw(&value, sizeof(value));
    memclrw(&bitfield, sizeof(bitfield));

    if (copts.operandsDebug != 0 && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateAssignment(node, requestedRegister, flags, out);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        generate_gpr_pair_assignment(node, requestedRegister, flags, out);
        return;
    }
    do {
        if (left->type == EOBJREF) {
            VarInfo *info;
            SInt32 physicalRegister;
            if (Registers_GetInfo(left->data.objref) != NULL)
                physicalRegister = (Registers_GetInfo(left->data.objref))->reg;
            else
                physicalRegister = 0;
            if (physicalRegister != 0) {
                info = Registers_GetInfo(left->data.objref);
                data_00560648[right->type](right, info->reg, 0, &value);
                if (info->is_fpr != 0) {
                    if (value.kind != OpndType_FPR)
                        Operands_ForceFPR(&value, type, info->reg);
                    if (value.reg != info->reg)
                        PCodeUtilities_EmitInstruction(PC_FMR, info->reg, value.reg);
                    out->kind = OpndType_FPR;
                    out->reg = info->reg;
                    break;
                }
                if (info->is_vector != 0) {
                    if (value.kind != OpndType_VR)
                        Operands_ForceVR(&value, type, info->reg);
                    if (value.reg != info->reg)
                        PCodeUtilities_EmitInstruction(PC_VMR, info->reg, value.reg);
                    out->kind = OpndType_VR;
                    out->reg = info->reg;
                    break;
                }
                if (value.kind != OpndType_GPR)
                    Operands_ForceGPR((Operand *)&value, type, info->reg);
                if (value.reg != info->reg)
                    PCodeUtilities_EmitInstruction(PC_MR, info->reg, value.reg);
                out->kind = OpndType_GPR;
                out->reg = info->reg;
                break;
            }
        }
        if (type->type == TYPEFLOAT) {
            data_00560648[right->type](right, 0, 0, &value);
            if (value.kind != OpndType_FPR)
                Operands_ForceFPR(&value, right->rtype, 0);
            if (InstrSelection_MatchPostIncDecRegister(left, &destination, &displacement) != 0) {
                Operands_MakeIndirect(&destination, node);
                Operands_EmitGPRMemoryInstruction(value.reg, &destination, type);
                EmitAddImm(destination.reg, destination.reg, displacement);
            } else {
                data_00560648[left->type](left, 0, 0, &destination);
                Operands_MakeIndirect(&destination, node);
                Operands_EmitGPRMemoryInstruction(value.reg, &destination, type);
            }
            out->kind = OpndType_FPR;
            out->reg = value.reg;
            break;
        }
        if (type->type == TYPESTRUCT && (SInt32)((TypeStruct *)type)->stype >= 4 &&
            (SInt32)((TypeStruct *)type)->stype <= 0xe) {
            data_00560648[right->type](right, 0, 0, &value);
            if (value.kind == OpndType_Immediate) {
                if (value.kind != OpndType_VR)
                    Operands_ForceVR(&value, type, 0);
            } else if (value.kind != OpndType_VR) {
                Operands_ForceVR(&value, right->rtype, 0);
            }
            if (InstrSelection_MatchPostIncDecRegister(left, &destination, &displacement) != 0) {
                Operands_MakeIndirect(&destination, node);
                Operands_EmitSTVX(value.reg, &destination, type);
                EmitAddImm(destination.reg, destination.reg, displacement);
            } else {
                data_00560648[left->type](left, 0, 0, &destination);
                Operands_MakeIndirect(&destination, node);
                Operands_EmitSTVX(value.reg, &destination, type);
            }
            out->kind = OpndType_VR;
            out->reg = value.reg;
            break;
        }
        if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
            (type->type == TYPEMEMBERPOINTER && type->size == 4)) {
            data_00560648[right->type](right, 0, 0, &value);
            if (value.kind != OpndType_GPR)
                Operands_ForceGPR((Operand *)&value, right->rtype, 0);
            if (left->type == EBITFIELD) {
                data_00560648[left->data.monadic->type](left->data.monadic, 0, 0, &destination);
                Operands_MakeIndirect(&destination, node);
                bitfield = destination;
                if (bitfield.kind != OpndType_GPR)
                    Operands_ForceGPR((Operand *)&bitfield, type, 0);
                bitfieldType = (TypeBitfield *)left->rtype;
                Operands_InsertBitField(value.reg, &bitfield, bitfieldType);
                Operands_EmitTypedGPRMemoryInstruction(bitfield.reg, &destination, type);
                if (!node->ignored)
                    Operands_ExtractBitfield(&bitfield, TYPE_BITFIELD(left->rtype), value.reg, &destination);
            } else {
                if (InstrSelection_MatchPostIncDecRegister(left, &destination, &displacement) != 0) {
                    Operands_MakeIndirect(&destination, node);
                    Operands_EmitTypedGPRMemoryInstruction(value.reg, &destination, type);
                    EmitAddImm(destination.reg, destination.reg, displacement);
                } else {
                    data_00560648[left->type](left, 0, 0, &destination);
                    Operands_MakeIndirect(&destination, node);
                    Operands_EmitTypedGPRMemoryInstruction(value.reg, &destination, type);
                }
            }
            out->kind = OpndType_GPR;
            out->reg = value.reg;
            break;
        }
        data_00560648[right->type](right, 0, 0, &value);
        data_00560648[left->type](left, 0, 0, out);
        Operands_MakeIndirect(out, node);
        if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && !Type_RequiresMemoryReturn(type)) {
            if (value.kind == OpndType_GPRPair) {
                Type *integerStorageType = (Type *)&stunsignedlonglong;
                Operands_StoreGPRPair(value.reg, value.regHi, out, integerStorageType);
            } else if (value.kind == OpndType_GPR) {
                Type *integerStorageType = (Type *)&stunsignedlong;
                Operands_EmitTypedGPRMemoryInstruction(value.reg, out, integerStorageType);
            } else {
                alignment = StackFrameEABI_GetTypeAlignment(type);
                StructMoves_EmitCopy(out, &value, type->size, alignment);
            }
        } else {
            alignment = StackFrameEABI_GetTypeAlignment(type);
            StructMoves_EmitCopy(out, &value, type->size, alignment);
        }
    } while (0);
    return;
}

void select_or(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    ENode *left = expr->data.diadic.left;
    ENode *right = expr->data.diadic.right;

    if (copts.operandsDebug && expr->rtype->type == TYPEFLOAT) {
        CError_FATAL(1564);
        return;
    }
    if ((expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) && expr->rtype->size == 8) {
        emit_gpr_pair_or(expr, outputReg, outputRegHi, output);
        return;
    }
    if (left->type == EINTCONST)
        emit_gpr_immediate_operation(PC_ORI, right, left->data.intval.lo, outputReg, output);
    else if (right->type == EINTCONST)
        emit_gpr_immediate_operation(PC_ORI, left, right->data.intval.lo, outputReg, output);
    else if (right->type == EBINNOT)
        InstrSelection_EmitBinaryGPRInstruction(PC_ORC, left, right->data.monadic, outputReg, output);
    else if (left->type == EBINNOT)
        InstrSelection_EmitBinaryGPRInstruction(PC_ORC, right, left->data.monadic, outputReg, output);
    else
        InstrSelection_EmitBinaryGPRInstruction(PC_OR, left, right, outputReg, output);
}

static void SelectOperand(ENode *n, Operand *op)
{
    data_00560648[n->type](n, 0, 0, op);
    if (op->kind >= 9 && op->kind != OpndType_GPR)
        Operands_ForceGPR(op, n->rtype, 0);
}

void gen_xor(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    ENode *left = expr->data.diadic.left;
    ENode *right = expr->data.diadic.right;

    if (copts.operandsDebug && expr->rtype->type == TYPEFLOAT) {
        CError_FATAL(1513);
        return;
    }
    if ((expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) && expr->rtype->size == 8) {
        gen_xor_reg_pair(expr, outputReg, outputRegHi, output);
        return;
    }
    if (left->type == EINTCONST)
        emit_gpr_immediate_operation(PC_XORI, right, left->data.intval.lo, outputReg, output);
    else if (right->type == EINTCONST)
        emit_gpr_immediate_operation(PC_XORI, left, right->data.intval.lo, outputReg, output);
    else if (right->type == EBINNOT)
        InstrSelection_EmitBinaryGPRInstruction(PC_EQV, left, right->data.monadic, outputReg, output);
    else if (left->type == EBINNOT)
        InstrSelection_EmitBinaryGPRInstruction(PC_EQV, left->data.monadic, right, outputReg, output);
    else
        InstrSelection_EmitBinaryGPRInstruction(PC_XOR, left, right, outputReg, output);
}

static void InstrSelection_EmitMask(Operand *op, ENode *operand, SInt16 cnt, SInt16 lo, SInt16 hi, SInt16 arg2,
                                    Operand *out)
{
    SInt32 reg;

    memclrw(op, sizeof(Operand));
    (*data_00560648[operand->type])(operand, 0, 0, op);
    if (op->kind)
        Operands_ForceGPR((Operand *)op, operand->rtype, 0);
    reg = (arg2 != 0) ? arg2 : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_RLWINM, reg, op->reg, cnt, lo, hi);
    out->kind = OpndType_GPR;
    out->reg = reg;
}

void emit_bitwise_and(ENode *node, SInt16 outputReg, SInt16 outputRegHi, Operand *output)
{
    ENode *left;
    ENode *right;
    SInt16 maskBegin, maskEnd;

    left = node->data.diadic.left;
    right = node->data.diadic.right;

    if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
        CError_FATAL(1440);
        return;
    }
    if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8) {
        emit_gpr_pair_and(node, outputReg, outputRegHi, output);
        return;
    }

    do {
        if (right->type == EINTCONST) {
            SInt32 mask;
            SInt32 validMask;
            SInt16 complementEnd, complementBegin;

            mask = right->data.intval.lo;
            if (is_contiguous_mask(mask, &maskBegin, &maskEnd))
                validMask = 1;
            else if (mask != 0 && is_contiguous_mask(~mask, &complementBegin, &complementEnd))
                maskBegin = complementEnd + 1, maskEnd = complementBegin - 1, validMask = 1;
            else
                validMask = 0;
            if (validMask) {
                Operand shiftedLeft, shiftedRight, masked;

                if (left->type == ESHL && left->data.diadic.right->type == EINTCONST) {
                    SInt32 shiftCount = left->data.diadic.right->data.intval.lo;
                    if (maskEnd + shiftCount < 32) {
                        InstrSelection_EmitMask(&shiftedLeft, left->data.diadic.left,
                                                left->data.diadic.right->data.intval.lo, maskBegin, maskEnd, outputReg,
                                                output);
                        break;
                    }
                }
                if (left->type == ESHR && left->data.diadic.right->type == EINTCONST) {
                    SInt32 shiftCount = left->data.diadic.right->data.intval.lo;
                    if (shiftCount <= maskBegin && maskEnd >= maskBegin) {
                        InstrSelection_EmitMask(&shiftedRight, left->data.diadic.left,
                                                32 - left->data.diadic.right->data.intval.lo, maskBegin, maskEnd,
                                                outputReg, output);
                        break;
                    }
                }
                InstrSelection_EmitMask(&masked, left, 0, maskBegin, maskEnd, outputReg, output);
                break;
            }
        }
        if (right->type == EINTCONST && (right->data.intval.lo & 0xffff) == right->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_ANDI, left, right->data.intval.lo, outputReg, output);
            break;
        }
        if (right->type == EINTCONST && (right->data.intval.lo & 0xffff0000) == right->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_ANDIS, left, right->data.intval.lo >> 16, outputReg, output);
            break;
        }

        if (left->type == EINTCONST) {
            SInt32 mask;
            SInt32 validMask;
            SInt16 complementEnd, complementBegin;

            mask = left->data.intval.lo;
            if (is_contiguous_mask(mask, &maskBegin, &maskEnd))
                validMask = 1;
            else if (mask != 0 && is_contiguous_mask(~mask, &complementBegin, &complementEnd))
                maskBegin = complementEnd + 1, maskEnd = complementBegin - 1, validMask = 1;
            else
                validMask = 0;
            if (validMask) {
                Operand shiftedLeft, shiftedRight, masked;

                if (right->type == ESHL && right->data.diadic.right->type == EINTCONST) {
                    SInt32 shiftCount = right->data.diadic.right->data.intval.lo;
                    if (maskEnd + shiftCount < 32) {
                        InstrSelection_EmitMask(&shiftedLeft, right->data.diadic.left,
                                                right->data.diadic.right->data.intval.lo, maskBegin, maskEnd, outputReg,
                                                output);
                        break;
                    }
                }
                if (right->type == ESHR && right->data.diadic.right->type == EINTCONST) {
                    SInt32 shiftCount = right->data.diadic.right->data.intval.lo;
                    if (shiftCount <= maskBegin) {
                        InstrSelection_EmitMask(&shiftedRight, right->data.diadic.left,
                                                32 - right->data.diadic.right->data.intval.lo, maskBegin, maskEnd,
                                                outputReg, output);
                        break;
                    }
                }
                InstrSelection_EmitMask(&masked, right, 0, maskBegin, maskEnd, outputReg, output);
                break;
            }
        }
        if (left->type == EINTCONST && (left->data.intval.lo & 0xffff) == left->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_ANDI, right, left->data.intval.lo, outputReg, output);
            break;
        }
        if (left->type == EINTCONST && (left->data.intval.lo & 0xffff0000) == left->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_ANDIS, right, left->data.intval.lo >> 16, outputReg, output);
            break;
        }

        if (right->type == EBINNOT) {
            InstrSelection_EmitBinaryGPRInstruction(PC_ANDC, left, right->data.monadic, outputReg, output);
            break;
        }
        if (left->type == EBINNOT) {
            InstrSelection_EmitBinaryGPRInstruction(PC_ANDC, right, left->data.monadic, outputReg, output);
            break;
        }
        InstrSelection_EmitBinaryGPRInstruction(PC_AND, left, right, outputReg, output);
    } while (0);
}

void select_right_shift(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    Type *type;
    ENode *left;
    ENode *right;
    Operand op;
    short shift;
    int reg;

    left = expr->data.diadic.left;
    right = expr->data.diadic.right;
    type = expr->rtype;

    if (copts.operandsDebug && type->type == TYPEFLOAT) {
        CError_FATAL(1403);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        generate_gpr_pair_shift(expr, outputReg, outputRegHi, output);
        return;
    }
    if (right->type == EINTCONST) {
        shift = right->data.intval.lo;
        memclrw(&op, sizeof(Operand));
        data_00560648[left->type](left, 0, 0, &op);
        if (op.kind)
            Operands_ForceGPR(&op, (Type *)left->rtype, 0);
        reg = outputReg ? outputReg : gUsedVirtualRegistersGPR++;
        shift &= 31;
        if (Type_IsUnsigned(type))
            PCodeUtilities_EmitInstruction(PC_RLWINM, reg, op.reg, 32 - shift, 32 - type->size * 8 + shift, 31);
        else
            PCodeUtilities_EmitInstruction(PC_SRAWI, reg, op.reg, shift);
        output->kind = OpndType_GPR;
        output->reg = reg;
    } else {
        InstrSelection_EmitBinaryGPRInstruction(Type_IsUnsigned(type) ? 0x6b : 0x6d, left, right, outputReg, output);
    }
}

void generate_left_shift(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    ENode *left = expr->data.diadic.left;
    ENode *right = expr->data.diadic.right;
    Operand op;
    short shift;
    int reg;

    if (copts.operandsDebug && expr->rtype->type == TYPEFLOAT) {
        CError_FATAL(1363);
        return;
    }
    if ((expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) && expr->rtype->size == 8) {
        generate_gpr_pair_shift(expr, outputReg, outputRegHi, output);
        return;
    }
    if (right->type == EINTCONST) {
        shift = right->data.intval.lo;
        memclrw(&op, sizeof(op));
        data_00560648[left->type](left, 0, 0, &op);
        if (op.kind)
            Operands_ForceGPR(&op, left->rtype, 0);
        reg = outputReg ? outputReg : gUsedVirtualRegistersGPR++;
        shift &= 31;
        PCodeUtilities_EmitInstruction(PC_RLWINM, reg, op.reg, shift, 0, 31 - shift);
        output->kind = OpndType_GPR;
        output->reg = reg;
    } else {
        InstrSelection_EmitBinaryGPRInstruction(PC_SLW, left, right, outputReg, output);
    }
}

void select_subtraction(ENode *input, int resultReg, int secondaryReg, Operand *flags)
{
    Type *type;
    ENode *left;
    ENode *right;
    int leftOpcode;
    int rightOpcode;
    int opcode;
    char mode;
    mode = copts.operandsDebug;
    left = input->data.diadic.left;
    right = input->data.diadic.right;
    type = input->rtype;
    if (mode != 0 && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateDiadicArithmetic(input, resultReg, secondaryReg, flags);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        emit_gpr_pair_subtraction(input, resultReg, secondaryReg, flags);
        return;
    }
    if (type->type == TYPEFLOAT) {
        if (left->type == EMUL && copts.debugOptions != 0) {
            if (type->size == 4) {
                leftOpcode = 173;
            } else {
                leftOpcode = 172;
            }
            InstrSelection_EmitThreeOperandFPRInstruction(leftOpcode, left->data.diadic.left, left->data.diadic.right,
                                                          right, resultReg, flags);
        } else if (right->type == EMUL && copts.debugOptions != 0) {
            if (type->size == 4) {
                rightOpcode = 177;
            } else {
                rightOpcode = 176;
            }
            InstrSelection_EmitThreeOperandFPRInstruction(rightOpcode, right->data.diadic.left,
                                                          right->data.diadic.right, left, resultReg, flags);
        } else {
            if (type->size == 4) {
                opcode = 165;
            } else {
                opcode = 164;
            }
            emit_binary_fpr_instruction(opcode, left, right, resultReg, flags);
        }
    } else if (left->type == EINTCONST && (SInt32)left->data.intval.lo == (short)left->data.intval.lo) {
        emit_gpr_immediate_instruction(PC_SUBFIC, right, left->data.intval.lo, resultReg, flags);
    } else {
        InstrSelection_EmitBinaryGPRInstruction(PC_SUBF, right, left, resultReg, flags);
    }
}

void emit_add(ENode *node, SInt16 a, SInt16 b, Operand *c)
{
    ENode *left;
    ENode *right;
    Type *type;
    Operand opL;
    Operand opR;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    type = node->rtype;

    memclrw(&opL, sizeof(opL));
    memclrw(&opR, sizeof(opR));

    if (copts.operandsDebug && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateDiadicArithmetic(node, a, b, c);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        emit_gpr_pair_add(node, a, b, c);
        return;
    }
    if (type->type == TYPEFLOAT) {
        if (left->type == EMUL && copts.debugOptions) {
            SInt16 k;
            k = (type->size == 4) ? 0xab : 0xaa;
            InstrSelection_EmitThreeOperandFPRInstruction(k, left->data.diadic.left, left->data.diadic.right, right, a,
                                                          c);
        } else if (right->type == EMUL && copts.debugOptions) {
            SInt16 k;
            k = (type->size == 4) ? 0xab : 0xaa;
            InstrSelection_EmitThreeOperandFPRInstruction(k, right->data.diadic.left, right->data.diadic.right, left, a,
                                                          c);
        } else {
            SInt16 k;
            k = (type->size == 4) ? 0xa3 : 0xa2;
            emit_binary_fpr_instruction(k, left, right, a, c);
        }
    } else {
        if (right->hascall != 0) {
            SelectOperand(right, &opR);
            SelectOperand(left, &opL);
        } else {
            SelectOperand(left, &opL);
            SelectOperand(right, &opR);
        }
        if (node->rtype->type == TYPEPOINTER) {
            if ((node->data.diadic.left->rtype->type == TYPEINT || node->data.diadic.left->rtype->type == TYPEENUM) &&
                node->data.diadic.left->rtype->size == 8) {
                opL.kind = OpndType_GPR;
                opL.regHi = 0;
            }
            if ((node->data.diadic.right->rtype->type == TYPEINT || node->data.diadic.right->rtype->type == TYPEENUM) &&
                node->data.diadic.right->rtype->size == 8) {
                opR.kind = OpndType_GPR;
                opR.regHi = 0;
            }
        }
        Operands_Add(&opL, &opR, a, c);
    }
}

/* Parameters for division by a constant. */

static void unsigned_mod_pow2(ENode *left, int sh, short reg, Operand *result)
{
    Operand op;
    int r;

    memclrw(&op, 0x16);
    (*data_00560648[left->type])(left, 0, 0, &op);
    if (op.kind)
        Operands_ForceGPR(&op, left->rtype, 0);
    r = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_RLWINM, r, (SInt16)op.reg, 0, (SInt16)(0x20 - sh), 0x1f);
    result->kind = OpndType_GPR;
    result->reg = r;
}

static void signed_mod_pow2(ENode *left, int sh, short reg, Operand *result)
{
    Operand op;
    int r;

    memclrw(&op, 0x16);
    (*data_00560648[left->type])(left, 0, 0, &op);
    if (op.kind)
        Operands_ForceGPR(&op, left->rtype, 0);
    r = (reg != 0 && reg != (SInt16)op.reg) ? reg : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_SRAWI, r, (SInt16)op.reg, sh);
    PCodeUtilities_EmitInstruction(PC_ADDZE, r, r);
    PCodeUtilities_EmitInstruction(PC_RLWINM, r, r, sh, 0, 0x1f - sh);
    PCodeUtilities_EmitInstruction(PC_SUBFC, r, r, (SInt16)op.reg);
    result->kind = OpndType_GPR;
    result->reg = r;
}

void generate_modulo(ENode *node, short outputReg, short outputRegHi, Operand *result)
{
    ENode *left;
    ENode *right;
    SInt32 quotient, unsignedResult, signedDividend, unsignedDividend;
    SInt32 signBit, correctedQuotient, signedProduct, unsignedProduct, divisor;
    SInt32 shift, difference, halfDifference, sum, adjustedQuotient;
    SInt32 signedResult, multiplier, signedQuotient, unsignedMultiplier;
    SInt32 adjustment, subtraction, productReg, divisionReg, resultReg;
    Operand leftOperand, rightOperand;
    DivisionParameters unsignedMagic;
    ConstInfo2 signedMagic;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    memclrw(&leftOperand, 22);
    memclrw(&rightOperand, 22);

    if (copts.operandsDebug != 0 && node->rtype->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateDiadicArithmetic(node, outputReg, outputRegHi, result);
        return;
    }
    if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8) {
        generate_gpr_pair_division_or_modulo(node, outputReg, outputRegHi, result);
        return;
    }

    if (right->type == EINTCONST) {
        shift = getbit(right->data.intval.lo);
        shift = (shift > 0 && shift < 0x1f) ? shift : 0;
        if (shift != 0) {
            if (Type_IsUnsigned(node->rtype))
                unsigned_mod_pow2(left, shift, outputReg, result);
            else
                signed_mod_pow2(left, shift, outputReg, result);
            return;
        }
    }

    if (copts.uniformSpillBlockWeight == 0 && right->type == EINTCONST &&
        (divisor = (SInt32)right->data.intval.lo) != 1 && divisor != -1) {
        (*data_00560648[left->type])(left, 0, 0, &leftOperand);
        if (leftOperand.kind)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);

        if (Type_IsUnsigned(node->rtype)) {
            unsignedDividend = leftOperand.reg;
            quotient = gUsedVirtualRegistersGPR++;
            unsignedMultiplier = gUsedVirtualRegistersGPR++;
            unsignedProduct = gUsedVirtualRegistersGPR++;
            unsignedResult = outputReg != 0 ? outputReg : gUsedVirtualRegistersGPR++;
            compute_unsigned_division_parameters(right->data.intval.lo, &unsignedMagic);
            PCodeUtilities_LoadImmediate(unsignedMultiplier, unsignedMagic.multiplier);
            PCodeUtilities_EmitInstruction(PC_MULHWU, quotient, unsignedMultiplier, unsignedDividend);
            if (unsignedMagic.addIndicator == 0 && unsignedMagic.shift)
                PCodeUtilities_EmitInstruction(PC_RLWINM, quotient, quotient, 0x20 - unsignedMagic.shift,
                                               unsignedMagic.shift, 0x1f);
            if (unsignedMagic.addIndicator == 1) {
                difference = gUsedVirtualRegistersGPR++;
                if (copts.deleteDeadInstructions > 1) {
                    halfDifference = gUsedVirtualRegistersGPR++;
                    sum = gUsedVirtualRegistersGPR++;
                    adjustedQuotient = gUsedVirtualRegistersGPR++;
                } else {
                    halfDifference = difference;
                    sum = difference;
                    adjustedQuotient = difference;
                }
                PCodeUtilities_EmitInstruction(PC_SUBF, difference, quotient, unsignedDividend);
                PCodeUtilities_EmitInstruction(PC_RLWINM, halfDifference, difference, 0x1f, 1, 0x1f);
                PCodeUtilities_EmitInstruction(PC_ADD, sum, halfDifference, quotient);
                PCodeUtilities_EmitInstruction(PC_RLWINM, adjustedQuotient, sum, 0x20 - (unsignedMagic.shift - 1),
                                               unsignedMagic.shift - 1, 0x1f);
                quotient = adjustedQuotient;
            }
            if (divisor > 0 && divisor < 0x7fff) {
                PCodeUtilities_EmitInstruction(PC_MULLI, unsignedProduct, quotient, divisor);
            } else {
                (*data_00560648[right->type])(right, 0, 0, &rightOperand);
                if (rightOperand.kind)
                    Operands_ForceGPR(&rightOperand, right->rtype, 0);
                PCodeUtilities_EmitInstruction(PC_MULLW, unsignedProduct, quotient, rightOperand.reg);
            }
            PCodeUtilities_EmitInstruction(PC_SUBF, unsignedResult, unsignedProduct, unsignedDividend);
            result->kind = OpndType_GPR;
            result->reg = unsignedResult;
        } else {
            signedDividend = leftOperand.reg;
            signedQuotient = gUsedVirtualRegistersGPR++;
            multiplier = gUsedVirtualRegistersGPR++;
            signBit = gUsedVirtualRegistersGPR++;
            correctedQuotient = gUsedVirtualRegistersGPR++;
            signedProduct = gUsedVirtualRegistersGPR++;
            signedResult = outputReg != 0 ? outputReg : gUsedVirtualRegistersGPR++;
            compute_signed_division_multiplier_shift(right->data.intval.lo, &signedMagic);
            PCodeUtilities_LoadImmediate(multiplier, signedMagic.multiplier);
            PCodeUtilities_EmitInstruction(PC_MULHW, signedQuotient, multiplier, signedDividend);
            if (divisor > 0 && signedMagic.multiplier < 0) {
                adjustment = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_ADD, adjustment, signedQuotient, signedDividend);
                signedQuotient = adjustment;
            } else if (divisor < 0 && signedMagic.multiplier > 0) {
                subtraction = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SUBF, subtraction, signedDividend, signedQuotient);
                signedQuotient = subtraction;
            }
            if (signedMagic.shift) {
                adjustment = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SRAWI, adjustment, signedQuotient, signedMagic.shift);
                signedQuotient = adjustment;
            }
            PCodeUtilities_EmitInstruction(PC_RLWINM, signBit, signedQuotient, 1, 0x1f, 0x1f);
            PCodeUtilities_EmitInstruction(PC_ADD, correctedQuotient, signedQuotient, signBit);
            if (divisor < 0x7fff && divisor > -0x4000) {
                PCodeUtilities_EmitInstruction(PC_MULLI, signedProduct, correctedQuotient, divisor);
            } else {
                (*data_00560648[right->type])(right, 0, 0, &rightOperand);
                if (rightOperand.kind)
                    Operands_ForceGPR(&rightOperand, right->rtype, 0);
                PCodeUtilities_EmitInstruction(PC_MULLW, signedProduct, correctedQuotient, rightOperand.reg);
            }
            PCodeUtilities_EmitInstruction(PC_SUBF, signedResult, signedProduct, signedDividend);
            result->kind = OpndType_GPR;
            result->reg = signedResult;
        }
    } else {
        if (right->hascall) {
            (*data_00560648[right->type])(right, 0, 0, &rightOperand);
            if (rightOperand.kind)
                Operands_ForceGPR(&rightOperand, right->rtype, 0);
            (*data_00560648[left->type])(left, 0, 0, &leftOperand);
            if (leftOperand.kind)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
        } else {
            (*data_00560648[left->type])(left, 0, 0, &leftOperand);
            if (leftOperand.kind)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
            (*data_00560648[right->type])(right, 0, 0, &rightOperand);
            if (rightOperand.kind)
                Operands_ForceGPR(&rightOperand, right->rtype, 0);
        }
        divisionReg = gUsedVirtualRegistersGPR++;
        productReg = gUsedVirtualRegistersGPR++;
        resultReg = outputReg != 0 ? outputReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(Type_IsUnsigned(node->rtype) ? 0x46 : 0x45, divisionReg, leftOperand.reg,
                                       rightOperand.reg);
        PCodeUtilities_EmitInstruction(PC_MULLW, productReg, divisionReg, rightOperand.reg);
        PCodeUtilities_EmitInstruction(PC_SUBF, resultReg, productReg, leftOperand.reg);
        result->kind = OpndType_GPR;
        result->reg = resultReg;
    }
}

static void shift_right_imm(ENode *left, short sh, Type *type, short reg, Operand *result)
{
    Operand op;
    int r;

    memclrw(&op, 0x16);
    (*data_00560648[left->type])(left, 0, 0, &op);
    if (op.kind)
        Operands_ForceGPR(&op, left->rtype, 0);
    r = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
    sh &= 0x1f;
    if (Type_IsUnsigned(type))
        PCodeUtilities_EmitInstruction(PC_RLWINM, r, op.reg, 0x20 - sh, 0x20 - type->size * 8 + sh, 0x1f);
    else
        PCodeUtilities_EmitInstruction(PC_SRAWI, r, op.reg, sh);
    result->kind = OpndType_GPR;
    result->reg = r;
}

static void signed_div_pow2(ENode *left, int sh, short reg, Operand *result)
{
    Operand op;
    int r;

    memclrw(&op, 0x16);
    (*data_00560648[left->type])(left, 0, 0, &op);
    if (op.kind)
        Operands_ForceGPR(&op, left->rtype, 0);
    r = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_SRAWI, r, op.reg, sh);
    PCodeUtilities_EmitInstruction(PC_ADDZE, r, r);
    result->kind = OpndType_GPR;
    result->reg = r;
}

static void signed_div_negpow2(ENode *left, int sh, short reg, Operand *result)
{
    Operand op;
    int r;

    memclrw(&op, 0x16);
    (*data_00560648[left->type])(left, 0, 0, &op);
    if (op.kind)
        Operands_ForceGPR(&op, left->rtype, 0);
    r = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_SRAWI, r, op.reg, sh);
    PCodeUtilities_EmitInstruction(PC_ADDZE, r, r);
    PCodeUtilities_EmitInstruction(PC_NEG, r, r);
    result->kind = OpndType_GPR;
    result->reg = r;
}

void generate_division(ENode *node, short reg, short flags, Operand *result)
{
    ENode *left;
    ENode *right;
    Type *type;
    Operand unsignedOperand;
    DivisionParameters unsignedMagic;
    Operand signedOperand;
    ConstInfo2 signedMagic;
    SInt32 shift, divisor, unsignedSource, unsignedDivisor;
    SInt32 product, multiplier, unsignedDest, difference, adjusted, half, addIndicator;
    SInt32 dest, source, quotient, signedMultiplier, signBit, temp, negativeTemp;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    type = node->rtype;

    if (copts.operandsDebug != 0 && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateDiadicArithmetic(node, reg, flags, result);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        generate_gpr_pair_division_or_modulo(node, reg, flags, result);
        return;
    }
    do {
        if (type->type == TYPEFLOAT) {
            emit_binary_fpr_instruction(type->size == 4 ? 0xa9 : 0xa8, left, right, reg, result);
        } else if (Type_IsUnsigned(type)) {
            if (right->type == EINTCONST) {
                shift = getbit(right->data.intval.lo);
                shift = (shift > 0 && shift < 0x1f) ? shift : 0;
                if (shift != 0) {
                    shift_right_imm(left, shift, type, reg, result);
                    break;
                }
            }
            if (copts.uniformSpillBlockWeight == 0 && right->type == EINTCONST && right->data.intval.lo != 1) {
                unsignedDivisor = right->data.intval.lo;
                product = gUsedVirtualRegistersGPR++;
                multiplier = gUsedVirtualRegistersGPR++;
                unsignedDest = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
                memclrw(&unsignedOperand, sizeof(unsignedOperand));
                (*data_00560648[left->type])(left, 0, 0, &unsignedOperand);
                if (unsignedOperand.kind)
                    Operands_ForceGPR(&unsignedOperand, left->rtype, 0);
                unsignedSource = unsignedOperand.reg;
                compute_unsigned_division_parameters(unsignedDivisor, &unsignedMagic);
                PCodeUtilities_LoadImmediate(multiplier, unsignedMagic.multiplier);
                PCodeUtilities_EmitInstruction(PC_MULHWU, product, multiplier, unsignedSource);
                addIndicator = unsignedMagic.addIndicator;
                if (addIndicator == 0) {
                    if (unsignedMagic.shift)
                        PCodeUtilities_EmitInstruction(PC_RLWINM, unsignedDest, product, 0x20 - unsignedMagic.shift,
                                                       unsignedMagic.shift, 0x1f);
                    else
                        PCodeUtilities_EmitInstruction(PC_MR, unsignedDest, product);
                } else if (addIndicator == 1) {
                    difference = gUsedVirtualRegistersGPR++;
                    if (copts.deleteDeadInstructions > 1) {
                        half = gUsedVirtualRegistersGPR++;
                        adjusted = gUsedVirtualRegistersGPR++;
                    } else {
                        half = difference;
                        adjusted = difference;
                    }
                    PCodeUtilities_EmitInstruction(PC_SUBF, difference, product, unsignedSource);
                    PCodeUtilities_EmitInstruction(PC_RLWINM, half, difference, 0x1f, 1, 0x1f);
                    PCodeUtilities_EmitInstruction(PC_ADD, adjusted, half, product);
                    PCodeUtilities_EmitInstruction(PC_RLWINM, unsignedDest, adjusted, 0x20 - (unsignedMagic.shift - 1),
                                                   unsignedMagic.shift - 1, 0x1f);
                }
                result->kind = OpndType_GPR;
                result->reg = unsignedDest;
            } else {
                InstrSelection_EmitBinaryGPRInstruction(PC_DIVWU, left, right, reg, result);
            }
        } else {
            if (right->type == EINTCONST) {
                shift = getbit(right->data.intval.lo);
                shift = (shift > 0 && shift < 0x1f) ? shift : 0;
                if (shift != 0) {
                    signed_div_pow2(left, shift, reg, result);
                    break;
                }
            }
            if (right->type == EINTCONST) {
                shift = getbit(-right->data.intval.lo);
                shift = (shift > 0 && shift < 0x1f) ? shift : 0;
                if (shift != 0) {
                    signed_div_negpow2(left, shift, reg, result);
                    break;
                }
            }
            if (copts.uniformSpillBlockWeight == 0 && right->type == EINTCONST && right->data.intval.lo != 1 &&
                (divisor = right->data.intval.lo) != -1) {
                quotient = gUsedVirtualRegistersGPR++;
                signedMultiplier = gUsedVirtualRegistersGPR++;
                signBit = gUsedVirtualRegistersGPR++;
                dest = reg != 0 ? reg : gUsedVirtualRegistersGPR++;
                memclrw(&signedOperand, sizeof(signedOperand));
                (*data_00560648[left->type])(left, 0, 0, &signedOperand);
                if (signedOperand.kind)
                    Operands_ForceGPR(&signedOperand, left->rtype, 0);
                source = signedOperand.reg;
                compute_signed_division_multiplier_shift(divisor, &signedMagic);
                PCodeUtilities_LoadImmediate(signedMultiplier, signedMagic.multiplier);
                PCodeUtilities_EmitInstruction(PC_MULHW, quotient, signedMultiplier, source);
                if (divisor > 0 && signedMagic.multiplier < 0) {
                    temp = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_ADD, temp, quotient, source);
                    quotient = temp;
                } else if (divisor < 0 && signedMagic.multiplier > 0) {
                    negativeTemp = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SUBF, negativeTemp, source, quotient);
                    quotient = negativeTemp;
                }
                if (signedMagic.shift) {
                    temp = gUsedVirtualRegistersGPR++;
                    PCodeUtilities_EmitInstruction(PC_SRAWI, temp, quotient, signedMagic.shift);
                    quotient = temp;
                }
                PCodeUtilities_EmitInstruction(PC_RLWINM, signBit, quotient, 1, 0x1f, 0x1f);
                PCodeUtilities_EmitInstruction(PC_ADD, dest, quotient, signBit);
                result->kind = OpndType_GPR;
                result->reg = dest;
            } else {
                InstrSelection_EmitBinaryGPRInstruction(PC_DIVW, left, right, reg, result);
            }
        }
    } while (0);
}

void compute_unsigned_division_parameters(unsigned int divisor, struct DivisionParameters *parameters)
{
    unsigned int boundary;
    unsigned int boundaryQuotient;
    unsigned int divisorQuotient;
    unsigned int delta;
    unsigned int boundaryRemainder;
    int divisorRemainder;
    int exponent;

    parameters->addIndicator = 0;
    exponent = 0x1f;
    boundary = -(-divisor % divisor) - 1;
    boundaryQuotient = 0x80000000 / boundary;
    boundaryRemainder = 0x80000000 - boundaryQuotient * boundary;
    divisorQuotient = 0x7fffffff / divisor;
    divisorRemainder = 0x7fffffff - divisorQuotient * divisor;
    do {
        exponent = exponent + 1;
        if (boundary - boundaryRemainder <= boundaryRemainder) {
            boundaryQuotient = boundaryQuotient * 2 + 1;
            boundaryRemainder = boundaryRemainder * 2 - boundary;
        } else {
            boundaryQuotient = boundaryQuotient << 1;
            boundaryRemainder = boundaryRemainder << 1;
        }
        if (divisorRemainder + 1 >= divisor - divisorRemainder) {
            if (0x7fffffff <= divisorQuotient) {
                parameters->addIndicator = 1;
            }
            divisorQuotient = divisorQuotient * 2 + 1;
            divisorRemainder = (divisorRemainder * 2 + 1) - divisor;
        } else {
            if (0x80000000 <= divisorQuotient) {
                parameters->addIndicator = 1;
            }
            divisorQuotient = divisorQuotient << 1;
            divisorRemainder = divisorRemainder * 2 + 1;
        }
        delta = (divisor - 1) - divisorRemainder;
    } while ((exponent < 0x40) &&
             ((boundaryQuotient < delta || ((boundaryQuotient == delta && (boundaryRemainder == 0))))));
    parameters->multiplier = divisorQuotient + 1;
    parameters->shift = exponent + -0x20;
    return;
}

void compute_signed_division_multiplier_shift(SInt32 divisor, ConstInfo2 *magic)
{
    int exponent;
    UInt32 absoluteDivisor, criticalDivisor, delta;
    UInt32 criticalQuotient, criticalRemainder, divisorQuotient, divisorRemainder;
    UInt32 adjustedNumerator;
    const UInt32 two31 = 0x80000000;

    absoluteDivisor = abs(divisor);
    adjustedNumerator = two31 + ((UInt32)divisor >> 31);
    criticalDivisor = adjustedNumerator - 1 - adjustedNumerator % absoluteDivisor;
    exponent = 31;
    criticalQuotient = two31 / criticalDivisor;
    criticalRemainder = two31 - criticalQuotient * criticalDivisor;
    divisorQuotient = two31 / absoluteDivisor;
    divisorRemainder = two31 - divisorQuotient * absoluteDivisor;
    do {
        exponent = exponent + 1;
        criticalQuotient = 2 * criticalQuotient;
        criticalRemainder = 2 * criticalRemainder;
        if (criticalRemainder >= criticalDivisor) {
            criticalQuotient = criticalQuotient + 1;
            criticalRemainder = criticalRemainder - criticalDivisor;
        }
        divisorQuotient = 2 * divisorQuotient;
        divisorRemainder = 2 * divisorRemainder;
        if (divisorRemainder >= absoluteDivisor) {
            divisorQuotient = divisorQuotient + 1;
            divisorRemainder = divisorRemainder - absoluteDivisor;
        }
        delta = absoluteDivisor - divisorRemainder;
    } while (criticalQuotient < delta || (criticalQuotient == delta && criticalRemainder == 0));
    magic->multiplier = divisorQuotient + 1;
    if (divisor < 0)
        magic->multiplier = -magic->multiplier;
    magic->shift = exponent - 32;
}

void emit_multiply(ENode *node, SInt16 dstreg, SInt16 src, Operand *result)
{
    Type *type;
    ENode *left;
    ENode *right;

    left = node->data.diadic.left;
    right = node->data.diadic.right;
    type = node->rtype;

    if (copts.operandsDebug && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateDiadicArithmetic(node, dstreg, src, result);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        emit_gpr_pair_multiply(node, dstreg, src, result);
        return;
    }
    do {
        if (type->type == TYPEFLOAT) {
            emit_binary_fpr_instruction(type->size == 4 ? PC_FMULS : PC_FMUL, left, right, dstreg, result);
            break;
        }

        if (right->type == EINTCONST) {
            SInt32 shift = getbit(right->data.intval.lo);
            SInt32 valid = shift > 0 && shift < 31;
            if (!valid)
                shift = 0;
            if ((valid = shift) != 0) {
                Operand operand;
                int reg;
                memclrw(&operand, sizeof(operand));
                (*data_00560648[left->type])(left, 0, 0, &operand);
                if (operand.kind)
                    Operands_ForceGPR(&operand, left->rtype, 0);
                reg = dstreg ? dstreg : gUsedVirtualRegistersGPR++;
                shift &= 31;
                PCodeUtilities_EmitInstruction(PC_RLWINM, reg, operand.reg, (SInt16)shift, 0, 31 - (SInt16)shift);

                result->kind = OpndType_GPR;
                result->reg = reg;
                break;
            }
        }

        if (right->type == EINTCONST) {
            SInt32 shift = getbit(-right->data.intval.lo);
            SInt32 valid = shift > 0 && shift < 31;
            if (!valid)
                shift = 0;
            if ((valid = shift) != 0) {
                Operand operand;
                int reg;
                memclrw(&operand, sizeof(operand));
                (*data_00560648[left->type])(left, 0, 0, &operand);
                if (operand.kind)
                    Operands_ForceGPR(&operand, left->rtype, 0);
                reg = dstreg ? dstreg : gUsedVirtualRegistersGPR++;
                shift &= 31;
                PCodeUtilities_EmitInstruction(PC_RLWINM, reg, operand.reg, (SInt16)shift, 0, 31 - (SInt16)shift);
                PCodeUtilities_EmitInstruction(PC_NEG, reg, reg);

                result->kind = OpndType_GPR;
                result->reg = reg;
                break;
            }
        }

        if (left->type == EINTCONST) {
            SInt32 shift = getbit(left->data.intval.lo);
            SInt32 valid = shift > 0 && shift < 31;
            if (!valid)
                shift = 0;
            if ((valid = shift) != 0) {
                Operand operand;
                int reg;
                memclrw(&operand, sizeof(operand));
                (*data_00560648[right->type])(right, 0, 0, &operand);
                if (operand.kind)
                    Operands_ForceGPR(&operand, right->rtype, 0);
                reg = dstreg ? dstreg : gUsedVirtualRegistersGPR++;
                shift &= 31;
                PCodeUtilities_EmitInstruction(PC_RLWINM, reg, operand.reg, (SInt16)shift, 0, 31 - (SInt16)shift);

                result->kind = OpndType_GPR;
                result->reg = reg;
                break;
            }
        }

        if (left->type == EINTCONST) {
            SInt32 shift = getbit(-left->data.intval.lo);
            SInt32 valid = shift > 0 && shift < 31;
            if (!valid)
                shift = 0;
            if ((valid = shift) != 0) {
                Operand operand;
                int reg;
                memclrw(&operand, sizeof(operand));
                (*data_00560648[right->type])(right, 0, 0, &operand);
                if (operand.kind)
                    Operands_ForceGPR(&operand, right->rtype, 0);
                reg = dstreg ? dstreg : gUsedVirtualRegistersGPR++;
                shift &= 31;
                PCodeUtilities_EmitInstruction(PC_RLWINM, reg, operand.reg, (SInt16)shift, 0, 31 - (SInt16)shift);
                PCodeUtilities_EmitInstruction(PC_NEG, reg, reg);

                result->kind = OpndType_GPR;
                result->reg = reg;
                break;
            }
        }

        if (right->type == EINTCONST && right->data.intval.lo == (SInt16)right->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_MULLI, left, right->data.intval.lo, dstreg, result);
        } else if (left->type == EINTCONST && left->data.intval.lo == (SInt16)left->data.intval.lo) {
            emit_gpr_immediate_instruction(PC_MULLI, right, left->data.intval.lo, dstreg, result);
        } else {
            InstrSelection_EmitBinaryGPRInstruction(PC_MULLW, left, right, dstreg, result);
        }
    } while (0);
}

enum { STRUCT_VECTOR_FIRST = 4, STRUCT_VECTOR_LAST = 14 };

void force_monadic_operand_register(ENode *expr, short outputReg, short outputRegHi, Operand *output)
{
    ENode *inner = expr->data.monadic;
    data_00560648[inner->type](inner, 0, 0, output);
    if (copts.operandsDebug && inner->rtype->type == TYPEFLOAT) {
        if (inner->rtype->size == 4) {
            if (output->kind)
                Operands_ForceGPR(output, inner->rtype, outputReg);
        } else {
            Operands_ForceGPRPair(output, inner->rtype, outputReg, outputRegHi);
        }
    } else if (inner->rtype->type == TYPEFLOAT) {
        if (output->kind != OpndType_FPR)
            Operands_ForceFPR(output, inner->rtype, outputReg);
    } else if (inner->rtype->type == TYPESTRUCT && TYPE_STRUCT(inner->rtype)->stype >= STRUCT_VECTOR_FIRST &&
               TYPE_STRUCT(inner->rtype)->stype <= STRUCT_VECTOR_LAST) {
        if (output->kind != OpndType_VR)
            Operands_ForceVR(output, inner->rtype, outputReg);
    } else if (inner->rtype->type == TYPEINT || inner->rtype->type == TYPEENUM || inner->rtype->type == TYPEPOINTER ||
               (inner->rtype->type == TYPEMEMBERPOINTER && inner->rtype->size == 4)) {
        if ((inner->rtype->type == TYPEINT || inner->rtype->type == TYPEENUM) && inner->rtype->size == 8)
            Operands_ForceGPRPair(output, inner->rtype, outputReg, outputRegHi);
        else if (output->kind)
            Operands_ForceGPR(output, inner->rtype, outputReg);
    } else {
        CError_ASSERT(610, inner->rtype->type == TYPEVOID ||
                               ((inner->rtype->type == TYPESTRUCT || inner->rtype->type == TYPECLASS) &&
                                !Type_RequiresMemoryReturn(inner->rtype)));
    }
}

void emit_bitwise_not(ENode *node, SInt16 requestedReg, SInt16 requestedHighReg, Operand *dst)
{
    SInt32 resultHighReg;
    SInt32 resultReg;
    Operand wideOperand;
    Operand operand;
    ENode *input = node->data.monadic;
    ENode *wideInput = node->data.monadic;

    if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
        CError_FATAL(531);
        return;
    }
    if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8) {
        memclrw(&wideOperand, sizeof(wideOperand));
        data_00560648[wideInput->type](wideInput, 0, 0, &wideOperand);
        Operands_ForceGPRPair(&wideOperand, wideInput->rtype, 0, 0);
        resultReg = requestedReg ? requestedReg : gUsedVirtualRegistersGPR++;
        resultHighReg = requestedHighReg ? requestedHighReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_NOR, resultReg, wideOperand.reg, wideOperand.reg);
        PCodeUtilities_EmitInstruction(PC_NOR, resultHighReg, wideOperand.regHi, wideOperand.regHi);
        dst->kind = OpndType_GPRPair;
        dst->reg = resultReg;
        dst->regHi = resultHighReg;
        return;
    }
    if (input->type == EAND) {
        InstrSelection_EmitBinaryGPRInstruction(PC_NAND, input->data.diadic.left, input->data.diadic.right,
                                                requestedReg, dst);
    } else if (input->type == EOR) {
        InstrSelection_EmitBinaryGPRInstruction(PC_NOR, input->data.diadic.left, input->data.diadic.right, requestedReg,
                                                dst);
    } else if (input->type == EXOR) {
        InstrSelection_EmitBinaryGPRInstruction(PC_EQV, input->data.diadic.left, input->data.diadic.right, requestedReg,
                                                dst);
    } else {
        memclrw(&operand, sizeof(operand));
        data_00560648[input->type](input, 0, 0, &operand);
        if (operand.kind != OpndType_GPR)
            Operands_ForceGPR(&operand, input->rtype, 0);
        resultReg = requestedReg ? requestedReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_NOT, resultReg, operand.reg);
        dst->kind = OpndType_GPR;
        dst->reg = resultReg;
    }
}

void emit_negation(ENode *node, SInt16 targetReg, SInt16 targetHighReg, Operand *out)
{
    Operand pairOperand;
    Operand floatOperand;
    Operand integerOperand;
    SInt32 resultHighReg;
    SInt32 resultReg;
    Type *type;
    ENode *pairExpr;
    ENode *expr;

    expr = node->data.diadic.left;
    pairExpr = node->data.diadic.left;
    type = node->rtype;

    if (copts.operandsDebug != 0 && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_ToggleHighBit(node, targetReg, targetHighReg, out);
        return;
    }

    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        memclrw(&pairOperand, sizeof(pairOperand));
        data_00560648[pairExpr->type](pairExpr, 0, 0, &pairOperand);
        Operands_ForceGPRPair(&pairOperand, pairExpr->rtype, 0, 0);
        resultReg = targetReg ? targetReg : gUsedVirtualRegistersGPR++;
        resultHighReg = targetHighReg ? targetHighReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_SUBFIC, resultReg, pairOperand.reg, 0);
        PCodeUtilities_EmitInstruction(PC_SUBFZE, resultHighReg, pairOperand.regHi);
        out->kind = OpndType_GPRPair;
        out->reg = resultReg;
        out->regHi = resultHighReg;
        return;
    }

    if (type->type == TYPEFLOAT) {
        if (expr->type == EADD && expr->data.diadic.left->type == EMUL && copts.debugOptions != 0)
            InstrSelection_EmitThreeOperandFPRInstruction(
                (type->size == 4) ? PC_FNMADDS : PC_FNMADD, expr->data.diadic.left->data.diadic.left,
                expr->data.diadic.left->data.diadic.right, expr->data.diadic.right, targetReg, out);
        else if (expr->type == EADD && expr->data.diadic.right->type == EMUL && copts.debugOptions != 0)
            InstrSelection_EmitThreeOperandFPRInstruction(
                (type->size == 4) ? PC_FNMADDS : PC_FNMADD, expr->data.diadic.right->data.diadic.left,
                expr->data.diadic.right->data.diadic.right, expr->data.diadic.left, targetReg, out);
        else if (expr->type == ESUB && expr->data.diadic.left->type == EMUL && copts.debugOptions != 0)
            InstrSelection_EmitThreeOperandFPRInstruction(
                (type->size == 4) ? PC_FNMSUBS : PC_FNMSUB, expr->data.diadic.left->data.diadic.left,
                expr->data.diadic.left->data.diadic.right, expr->data.diadic.right, targetReg, out);
        else {
            memclrw(&floatOperand, sizeof(floatOperand));
            data_00560648[expr->type](expr, 0, 0, &floatOperand);
            if (floatOperand.kind != OpndType_FPR) {
                Operands_ForceFPR(&floatOperand, expr->rtype, 0);
            }
            resultReg = targetReg ? targetReg : gUsedVirtualRegistersFPR++;
            PCodeUtilities_EmitInstruction(PC_FNEG, resultReg, floatOperand.reg);
            out->kind = OpndType_FPR;
            out->reg = resultReg;
        }
    } else {
        memclrw(&integerOperand, sizeof(integerOperand));
        data_00560648[expr->type](expr, 0, 0, &integerOperand);
        if (integerOperand.kind != OpndType_GPR) {
            Operands_ForceGPR(&integerOperand, expr->rtype, 0);
        }
        resultReg = targetReg ? targetReg : gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_NEG, resultReg, integerOperand.reg);
        out->kind = OpndType_GPR;
        out->reg = resultReg;
    }
}

void select_indirect_operand(ENode *expression, int targetReg, int flags, Operand *result)
{
    Type *type;
    short sourceReg;
    int physicalReg;
    int structKind;
    VarInfo *registerInfo;
    int highReg;
    ENode *value;
    int offset;
    unsigned char kind;
    unsigned char valueKind;
    Operand operand;
    SInt32 displacement;

    type = expression->rtype;
    value = expression->data.monadic;
    if (copts.operandsDebug != 0 && type->type == TYPEFLOAT) {
        SFPE_PPC_EABI_GenerateMonadicOperand(expression, targetReg, flags, result);
        return;
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        select_monadic_operand(expression, targetReg, flags, result);
        return;
    }
    memclrw(&operand, sizeof(operand));
    if (value->type == EOBJREF &&
        (Registers_GetInfo(value->data.objref) != NULL ? (physicalReg = Registers_GetInfo(value->data.objref)->reg)
                                                       : (physicalReg = 0)) != 0) {
        registerInfo = Registers_GetInfo(value->data.objref);
        if (registerInfo->is_fpr != 0) {
            result->kind = OpndType_FPR;
        } else if (registerInfo->is_vector != 0) {
            result->kind = OpndType_VR;
        } else {
            result->kind = OpndType_GPR;
        }
        result->reg = registerInfo->reg;
        result->object = NULL;
    } else {
        if (value->type == EBITFIELD) {
            kind = value->data.monadic->type;
            data_00560648[kind](value->data.monadic, 0, 0, &operand);
            Operands_MakeIndirect(&operand, expression);
            if (operand.kind != OpndType_GPR) {
                Operands_ForceGPR(&operand, type, targetReg);
            }
            Operands_ExtractBitfield(&operand, TYPE_BITFIELD(value->rtype), targetReg, result);
        } else if (InstrSelection_MatchPostIncDecRegister(value, &operand, &displacement) != 0 &&
                   (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                    type->type == TYPEMEMBERPOINTER && type->size == 4 || type->type == TYPEFLOAT ||
                    type->type == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= 4 && structKind <= 14)) {
            Operands_MakeIndirect(&operand, expression);
            *result = operand;
            if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                type->type == TYPEMEMBERPOINTER && type->size == 4) {
                if (result->kind != OpndType_GPR) {
                    Operands_ForceGPR(result, type, targetReg);
                }
            } else if (type->type == TYPEFLOAT) {
                if (result->kind != OpndType_FPR) {
                    Operands_ForceFPR(result, type, targetReg);
                }
            } else if (result->kind != OpndType_VR) {
                Operands_ForceVR(result, type, targetReg);
            }
            offset = displacement;
            sourceReg = operand.reg;
            if (displacement != (short)displacement) {
                PCodeUtilities_EmitInstruction(66, highReg = operand.reg, sourceReg, 0,
                                               (short)((offset >> 16) + (offset >> 15 & 1)));
                if ((short)offset != 0) {
                    PCodeUtilities_EmitInstruction(63, highReg, highReg, 0, (short)offset);
                }
            } else {
                PCodeUtilities_EmitInstruction(63, operand.reg, sourceReg, 0, offset);
            }
        } else {
            valueKind = value->type;
            data_00560648[valueKind](value, 0, 0, result);
            Operands_MakeIndirect(result, expression);
        }
    }
}

static inline void ClearOps(Operand *a, Operand *b, Operand *c)
{
    memclrw(a, 0x16);
    memclrw(b, 0x16);
    memclrw(c, 0x16);
}

static SInt32 InstrSelection_GetVR0(ENode *node)
{
    if (Registers_GetInfo(node->data.objref) != NULL)
        return Registers_GetInfo(node->data.objref)->reg;
    return 0;
}

static inline SInt32 InstrSelection_GetVR(ENode *n)
{
    return InstrSelection_GetVR0(n);
}

static inline void StoreOp(SInt16 r, Operand *o, Type *t)
{
    Operands_EmitTypedGPRMemoryInstruction(r, o, t);
}

static inline int AddOffset(SInt16 dest, SInt16 source, SInt32 size)
{
    if (size != (SInt16)size) {
        SInt32 hi = size >> 16;
        SInt32 carry = (size >> 15) & 1;
        UInt32 high = hi + carry;
        PCodeUtilities_EmitInstruction(PC_ADDIS, (SInt16)dest, source, 0, (SInt16)high);
        if ((SInt16)size != 0)
            PCodeUtilities_EmitInstruction(PC_ADDI, (SInt16)dest, (SInt16)dest, 0, (SInt16)size);
    } else {
        PCodeUtilities_EmitInstruction(PC_ADDI, (SInt16)dest, source, 0, size);
    }
    return 1;
}

static inline int AddOffset2(SInt32 d, Operand *s, SInt32 n)
{
    return AddOffset(d, s->reg, n);
}

void generate_postinc_postdec(ENode *expr, SInt32 outputReg, SInt32 outputRegHi, Operand *result)
{
    SInt32 variableReg;
    SInt32 constantReg;
    SInt32 step;
    ENode *lvalue;
    Type *type;
    TypeBitfield *bitfield;
    Operand address;
    Operand value;
    Operand bitfieldValue;
    Float one;
    SInt32 secondReg;

    bitfield = NULL;
    lvalue = expr->data.monadic->data.monadic;
    type = expr->rtype;
    ClearOps(&address, &value, &bitfieldValue);
    secondReg = outputRegHi;
    if (copts.operandsDebug) {
        if (type->type == TYPEFLOAT) {
            SInt16 returnReg = outputRegHi;
            SFPE_PPC_EABI_GenerateFloatPostIncDec(expr, outputReg, returnReg, result);
            return;
        }
    }
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        emit_postinc_postdec_gpr_pair(expr, outputReg, secondReg, result);
        return;
    }
    if (type->type == TYPEFLOAT) {
        SInt32 updatedReg;
        int useRequestedReg;
        SInt16 reg;
        if (lvalue->type == EOBJREF && (variableReg = InstrSelection_GetVR(lvalue))) {
            result->kind = OpndType_FPR;
            useRequestedReg = 0;
            if ((SInt16)outputReg != 0 && (SInt16)outputReg != variableReg)
                useRequestedReg = 1;
            if (useRequestedReg)
                reg = outputReg;
            else
                reg = gUsedVirtualRegistersFPR++;
            result->reg = reg;
            PCodeUtilities_EmitInstruction(PC_FMR, result->reg, variableReg);
            one = float_one;
            Operands_EmitOpcodeWithObjectBaseOffset(constantReg = gUsedVirtualRegistersFPR++, type,
                                                    TOC_GetFloatObject(type, &one));
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitInstruction(type->size == 4 ? PC_FADDS : PC_FADD, variableReg, variableReg,
                                               constantReg);
            else
                PCodeUtilities_EmitInstruction(type->size == 4 ? PC_FSUBS : PC_FSUB, variableReg, variableReg,
                                               constantReg);
        } else {
            (*data_00560648[lvalue->type])(lvalue, 0, 0, &address);
            Operands_MakeIndirect(&address, lvalue);
            value = address;
            if (value.kind != OpndType_FPR)
                Operands_ForceFPR(&value, type, 0);
            result->kind = OpndType_FPR;
            result->reg = gUsedVirtualRegistersFPR++;
            PCodeUtilities_EmitInstruction(PC_FMR, result->reg, value.reg);
            one = float_one;
            Operands_EmitOpcodeWithObjectBaseOffset(constantReg = gUsedVirtualRegistersFPR++, type,
                                                    TOC_GetFloatObject(type, &one));
            updatedReg = gUsedVirtualRegistersFPR++;
            if (expr->type == EPOSTINC)
                PCodeUtilities_EmitInstruction(type->size == 4 ? PC_FADDS : PC_FADD, updatedReg, value.reg,
                                               constantReg);
            else
                PCodeUtilities_EmitInstruction(type->size == 4 ? PC_FSUBS : PC_FSUB, updatedReg, value.reg,
                                               constantReg);
            Operands_EmitGPRMemoryInstruction(updatedReg, &address, type);
        }
    } else {
        SInt32 useRequestedReg;
        SInt16 reg;
        SInt32 updatedReg;
        if (type->type == TYPEPOINTER) {
            if (expr->type == EPOSTINC)
                step = TPTR_TARGET(type)->size;
            else
                step = -TPTR_TARGET(type)->size;
        } else {
            if (expr->type == EPOSTINC)
                step = 1;
            else
                step = -1;
        }
        if (lvalue->type == EOBJREF && (variableReg = InstrSelection_GetVR(lvalue))) {
            result->kind = OpndType_GPR;
            useRequestedReg = 0;
            if ((SInt16)outputReg != 0 && (SInt16)outputReg != variableReg)
                useRequestedReg = 1;
            if (useRequestedReg)
                reg = outputReg;
            else
                reg = gUsedVirtualRegistersGPR++;
            result->reg = reg;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, variableReg);
            AddOffset(variableReg, variableReg, step);
        } else {
            if (lvalue->type == EBITFIELD) {
                bitfield = TYPE_BITFIELD(lvalue->rtype);
                lvalue = lvalue->data.monadic;
            }
            (*data_00560648[lvalue->type])(lvalue, 0, 0, &address);
            Operands_MakeIndirect(&address, lvalue);
            value = address;
            if (value.kind != OpndType_GPR)
                Operands_ForceGPR(&value, type, 0);
            if (bitfield != NULL) {
                bitfieldValue = value;
                Operands_ExtractBitfield(&bitfieldValue, bitfield, 0, &value);
            }
            result->kind = OpndType_GPR;
            result->reg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, value.reg);
            updatedReg = gUsedVirtualRegistersGPR++;
            AddOffset2(updatedReg, &value, step);
            if (bitfield != NULL) {
                Operands_InsertBitField(updatedReg, &bitfieldValue, bitfield);
                updatedReg = bitfieldValue.reg;
            }
            constantReg = updatedReg;
            StoreOp(constantReg, &address, type);
        }
    }
}

/* Evaluators write the cached result for an expression node. */

void get_objaccess_cached_value(ENode *node, UInt32 argument2, UInt32 argument3, Operand *result, UInt32 argument5)
{
    ENode *objectAccess;
    Operand *cachedValue;

    objectAccess = node->data.monadic;
    if (objectAccess->type != EINSTRUCTION) {
        CError_FATAL(231);
    }
    if (objectAccess->data.objaccess.cachedValue == NULL) {
        cachedValue = (Operand *)CompilerTools_AllocatePool(0x16);
        objectAccess->data.objaccess.cachedValue = cachedValue;
        data_00560648[objectAccess->data.objaccess.expression->type](objectAccess->data.objaccess.expression, 0, 0,
                                                                     cachedValue);
    }
    *result = *objectAccess->data.objaccess.cachedValue;
}

void get_dispatch_result(struct DeferredDispatch *dispatch, unsigned int argument2, unsigned int argument3,
                         struct DispatchResult *output)
{
    struct DispatchResult *result;

    if (dispatch->result == NULL) {
        result = (struct DispatchResult *)CompilerTools_AllocatePool(sizeof(struct DispatchResult));
        dispatch->result = result;
        data_00560648[*dispatch->input](dispatch->input, 0, 0, result);
    }
    *output = *dispatch->result;
}

void InstrSelection_EmitSwitchTables(Object *function)
{
    int remaining;
    SInt32 *entry;
    Object *table;
    while (switch_tables != NULL) {
        table = switch_tables->object.value;
        entry = table->u.data.u.switchtable.data;
        remaining = table->u.data.u.switchtable.size;
        while (remaining != 0) {
            PCodeLabel *label = (PCodeLabel *)*entry;
            PCodeBlock *block = label->target.block;
            *entry = CTool_EndianConvertWord32(block->code_offset);
            remaining--;
            entry++;
        }
        ObjGen_PPC_EABI_EmitSwitchTable(table, function);
        switch_tables = switch_tables->next;
    }
}
