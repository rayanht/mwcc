#define CERROR_FILE "PCodeListing.c"
#include "compiler/common.h"
#include "compiler/PCodeListing.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include <string.h>
#include <stdio.h>

/* The condition register bits, and the condition each BO field value tests. */
static char *data_005621b0[4] = {"lt", "gt", "eq", "un"};
static char *data_005621e0[31] = {"",   "lgt", "llt", "", "eq", "lge", "lle", "", "gt", "", "", "", "ge", "", "", "",
                                  "lt", "",    "",    "", "le", "",    "",    "", "ne", "", "", "", "",   "", ""};

static inline void formatdataflowset(char *name, UInt32 *vec, UInt32 size)
{
    UInt32 i;

    fprintf(stdout, "%s = {", name);
    for (i = 0; i < size; i++) {
        if (i && !(i & 7))
            fprintf(stdout, "\n\t\t");
        fprintf(stdout, "B%ld ", vec[i]);
    }
    fprintf(stdout, " }\n");
}

/* Instruction data consumed by the PowerPC instruction formatter. */
static inline void FormatFirstOperand(PCodeInstruction *instruction, char *out)
{
    {
        const char *sign;
        int *symbol;
        if (instruction->operandData.operands[0].kind == PCOp_MEMORY) {
            sprintf(out, "%.200s", COptimizer_GetFunctionObject(instruction->operandData.operands[0].object)->name);
        } else if (instruction->operandData.operands[0].kind != PCOp_LABEL) {
            if (instruction->operandData.operands[0].value.signed_value >= 0)
                sign = "+";
            else
                sign = "";
            sprintf(out, "*%s%ld", sign, instruction->operandData.operands[0].value.unsigned_value);
        } else {
            if ((symbol = (int *)((int *)instruction->operandData.operands[0].value.signed_value)[1]) == NULL)
                sprintf(out, "B<unknown>");
            else
                sprintf(out, "B%ld", symbol[7]);
        }
    }
}

void format_operand(PCodeOperand *n, char *ctx)
{
    char buf[20];

    if (n->kind == 7) {
        if (((struct PCodeLabelDifference *)n)->negate == 1)
            CPrep_AppendStringBounded(ctx, "-", 300);
        CPrep_AppendStringBounded(ctx, "(B", 300);
        if (((struct PCodeLabelDifference *)n)->subtractLabel->target.block == NULL)
            CPrep_AppendStringBounded(ctx, "<unknown>-B", 300);
        else {
            sprintf(buf, "%ld-B", ((struct PCodeLabelDifference *)n)->subtractLabel->target.block->index);
            CPrep_AppendStringBounded(ctx, buf, 300);
        }
        if (((struct PCodeLabelDifference *)n)->addLabel->target.block == NULL)
            CPrep_AppendStringBounded(ctx, "<unknown>+", 300);
        else {
            sprintf(buf, "%ld+", ((struct PCodeLabelDifference *)n)->addLabel->target.block->index);
            CPrep_AppendStringBounded(ctx, buf, 300);
        }
        sprintf(buf, "%ld)", ((struct PCodeLabelDifference *)n)->addend);
        CPrep_AppendStringBounded(ctx, buf, 300);
    } else if (n->kind == 4) {
        sprintf(buf, "%ld", n->value.immediate_value);
        CPrep_AppendStringBounded(ctx, buf, 300);
        if (n->object != NULL) {
            CPrep_AppendStringBounded(ctx, "{", 300);
            CPrep_AppendStringBounded(ctx, COptimizer_GetFunctionObject(n->object)->name, 300);
            CPrep_AppendStringBounded(ctx, "}", 300);
        }
    } else {
        sprintf(buf, "?unknown operand kind=%d", n->kind);
        CPrep_AppendStringBounded(ctx, buf, 300);
    }
}

void fn_004c4bf0(PCodeInstruction *instruction, char *out)
{
    char extraOperands[20];
    char operandSuffix[20];
    char addressSuffix[20];

    switch (instruction->opcode) {
        case PC_BL: {
            const char *sign;
            PCodeBlock *target;
            if (instruction->operandData.operands[0].kind == PCOp_MEMORY) {
                sprintf(out, "%.200s", COptimizer_GetFunctionObject(instruction->operandData.operands[0].object)->name);
            } else if (instruction->operandData.operands[0].kind != PCOp_LABEL) {
                if (instruction->operandData.operands[0].value.signed_value >= 0)
                    sign = "+";
                else
                    sign = "";
                sprintf(out, "*%s%ld", sign, instruction->operandData.operands[0].value.unsigned_value);
            } else {
                if ((target = instruction->operandData.operands[0].value.label->target.block) == NULL)
                    sprintf(out, "B<unknown>");
                else
                    sprintf(out, "B%ld", target->index);
            }
        } break;

        case PC_B:
        case PC_BDNZ:
        case PC_BDZ:
            FormatFirstOperand(instruction, out);
            break;

        case PC_BC: {
            const char *sign;
            PCodeBlock *target;
            if (instruction->operandData.operands[3].kind == PCOp_MEMORY) {
                sprintf(out, "%.200s", COptimizer_GetFunctionObject(instruction->operandData.operands[3].object)->name);
            } else if (instruction->operandData.operands[3].kind != PCOp_LABEL) {
                if (instruction->operandData.operands[3].value.signed_value >= 0)
                    sign = "+";
                else
                    sign = "";
                sprintf(out, "%d,cr%d,%s,*%s%ld", instruction->operandData.operands[0].value.unsigned_value,
                        instruction->operandData.operands[1].value.reg,
                        data_005621b0[instruction->operandData.operands[2].value.unsigned_value & 3], sign,
                        instruction->operandData.operands[3].value.unsigned_value);
            } else {
                if ((target = instruction->operandData.operands[3].value.label->target.block) == NULL)
                    sprintf(out, "B<unknown>");
                else
                    sprintf(out, "%d,cr%d,%s,B%ld", instruction->operandData.operands[0].value.unsigned_value,
                            instruction->operandData.operands[1].value.reg,
                            data_005621b0[instruction->operandData.operands[2].value.unsigned_value & 3],
                            target->index);
            }
        } break;

        case PC_BT:
        case PC_BF:
        case PC_BDNZT:
        case PC_BDNZF:
        case PC_BDZT:
        case PC_BDZF: {
            const char *sign;
            PCodeBlock *target;
            if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                sprintf(out, "%.200s", COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
            } else if (instruction->operandData.operands[2].kind != PCOp_LABEL) {
                if (instruction->operandData.operands[2].value.signed_value >= 0)
                    sign = "+";
                else
                    sign = "";
                sprintf(out, "cr%d,%s,*%s%ld", instruction->operandData.operands[0].value.reg,
                        data_005621b0[instruction->operandData.operands[1].value.unsigned_value & 3], sign,
                        instruction->operandData.operands[2].value.unsigned_value);
            } else {
                if ((target = instruction->operandData.operands[2].value.label->target.block) == NULL)
                    sprintf(out, "B<unknown>");
                else
                    sprintf(out, "cr%d,%s,B%ld", instruction->operandData.operands[0].value.reg,
                            data_005621b0[instruction->operandData.operands[1].value.unsigned_value & 3],
                            target->index);
            }
        } break;

        case PC_BTLR:
        case PC_BFLR:
            sprintf(out, "cr%d,%s", instruction->operandData.operands[0].value.reg,
                    data_005621b0[instruction->operandData.operands[1].value.unsigned_value & 3]);
            break;
        case PC_BCLR:
        case PC_BCCTR:
            sprintf(out, "%d,cr%d,%s", instruction->operandData.operands[0].value.unsigned_value,
                    instruction->operandData.operands[1].value.reg,
                    data_005621b0[instruction->operandData.operands[2].value.unsigned_value & 3]);
            break;
        case PC_CRAND:
        case PC_CRANDC:
        case PC_CREQV:
        case PC_CRNAND:
        case PC_CRNOR:
        case PC_CROR:
        case PC_CRORC:
        case PC_CRXOR:
            sprintf(out, "cr%d[%s],cr%d[%s],cr%d[%s]", instruction->operandData.operands[0].value.reg,
                    data_005621b0[instruction->operandData.operands[1].value.unsigned_value & 3],
                    instruction->operandData.operands[2].value.reg,
                    data_005621b0[instruction->operandData.operands[3].value.unsigned_value & 3],
                    instruction->operandData.operands[4].value.reg,
                    data_005621b0[instruction->operandData.operands[5].value.unsigned_value & 3]);
            break;
        case PC_MCRF:
            sprintf(out, "cr%d,cr%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
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
        case PC_PSQ_L:
        case PC_PSQ_LU:
        case PC_PSQ_LX:
        case PC_PSQ_LUX:
        case PC_PSQ_ST:
        case PC_PSQ_STU:
        case PC_PSQ_STX:
        case PC_PSQ_STUX:
            if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                unsigned char operandFlags = instruction->operandData.operands[2].flags;
                switch (operandFlags) {
                    case 6:
                        if (instruction->operandData.operands[2].value.signed_value == 0)
                            sprintf(out, "r%d,LO(%.200s)(r%d)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[1].value.reg);
                        else
                            sprintf(out, "r%d,LO(%.200s)+%d(r%d)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[2].value.unsigned_value,
                                    instruction->operandData.operands[1].value.reg);
                        break;
                    default: {
                        int displacement;
                        if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0)
                            sprintf(out, "r%d,%.200s(r%d)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[1].value.reg);
                        else if (displacement > 0)
                            sprintf(out, "r%d,%.200s+%d(r%d)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[2].value.unsigned_value,
                                    instruction->operandData.operands[1].value.reg);
                        else
                            sprintf(out, "r%d,%.200s-%d(r%d)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    -instruction->operandData.operands[2].value.unsigned_value,
                                    instruction->operandData.operands[1].value.reg);
                    } break;
                }
                switch (instruction->opcode) {
                    case PC_PSQ_L:
                    case PC_PSQ_LU:
                    case PC_PSQ_LX:
                    case PC_PSQ_LUX:
                    case PC_PSQ_ST:
                    case PC_PSQ_STU:
                    case PC_PSQ_STX:
                    case PC_PSQ_STUX:
                        sprintf(extraOperands, ",%d,%d", instruction->operandData.operands[3].value.unsigned_value,
                                instruction->operandData.operands[4].value.unsigned_value);
                        CPrep_AppendStringBounded(out, extraOperands, 300);
                        break;
                    default:
                        break;
                }
            } else {
                sprintf(out, "r%d,", instruction->operandData.operands[0].value.reg);
                format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
                sprintf(operandSuffix, "(r%d)", instruction->operandData.operands[1].value.reg);
                CPrep_AppendStringBounded(out, operandSuffix, 300);
            }
            break;
        case PC_LBZX:
        case PC_LBZUX:
        case PC_LHZX:
        case PC_LHZUX:
        case PC_LHAX:
        case PC_LHAUX:
        case PC_LHBRX:
        case PC_LWZX:
        case PC_LWZUX:
        case PC_LWBRX:
        case PC_STBX:
        case PC_STBUX:
        case PC_STHX:
        case PC_STHUX:
        case PC_STHBRX:
        case PC_STWX:
        case PC_STWUX:
        case PC_STWBRX:
        case PC_LWARX:
        case PC_LSWX:
        case PC_STSWX:
        case PC_STWCX:
        case PC_ECIWX:
        case PC_ECOWX:
            sprintf(out, "r%d,(r%d,r%d)", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_DCBF:
        case PC_DCBST:
        case PC_DCBT:
        case PC_DCBTST:
        case PC_DCBZ:
        case PC_DCBI:
        case PC_ICBI:
        case PC_DCBA:
        case PC_DCBZ_L:
            sprintf(out, "r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_ADD:
        case PC_ADDC:
        case PC_ADDE:
        case PC_DIVW:
        case PC_DIVWU:
        case PC_MULHW:
        case PC_MULHWU:
        case PC_MULLW:
        case PC_SUBF:
        case PC_SUBFC:
        case PC_SUBFE:
        case PC_AND:
        case PC_OR:
        case PC_XOR:
        case PC_NAND:
        case PC_NOR:
        case PC_EQV:
        case PC_ANDC:
        case PC_ORC:
        case PC_SLW:
        case PC_SRW:
        case PC_SRAW:
            sprintf(out, "r%d,r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_CMP:
        case PC_CMPL:
            sprintf(out, "cr%d,r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_ADDI:
        case PC_ORI:
            if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                unsigned char operandFlags = instruction->operandData.operands[2].flags;
                switch (operandFlags) {
                    case 6:
                        sprintf(out, "r%d,r%d,LO(%.200s)", instruction->operandData.operands[0].value.reg,
                                instruction->operandData.operands[1].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                        break;
                    default: {
                        int displacement;
                        if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0)
                            sprintf(out, "r%d,r%d,%.200s", instruction->operandData.operands[0].value.reg,
                                    instruction->operandData.operands[1].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                        else if (displacement > 0)
                            sprintf(out, "r%d,r%d,%.200s+%d", instruction->operandData.operands[0].value.reg,
                                    instruction->operandData.operands[1].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[2].value.unsigned_value);
                        else
                            sprintf(out, "r%d,r%d,%.200s-%d", instruction->operandData.operands[0].value.reg,
                                    instruction->operandData.operands[1].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    -instruction->operandData.operands[2].value.unsigned_value);
                    } break;
                }
            } else {
                sprintf(out, "r%d,r%d,", instruction->operandData.operands[0].value.reg,
                        instruction->operandData.operands[1].value.reg);
                format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
            }
            break;
        case PC_ADDIS:
            if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                unsigned char operandFlags = instruction->operandData.operands[2].flags;
                switch (operandFlags) {
                    case 8:
                        sprintf(out, "r%d,r%d,HA(%.200s)", instruction->operandData.operands[0].value.reg,
                                instruction->operandData.operands[1].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                        break;
                    case 7:
                        sprintf(out, "r%d,r%d,HI(%.200s)", instruction->operandData.operands[0].value.reg,
                                instruction->operandData.operands[1].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                        break;
                    default: {
                        int displacement;
                        if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0)
                            sprintf(out, "r%d,r%d,%.200s", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[1].value.reg);
                        else if (displacement > 0)
                            sprintf(out, "r%d,r%d,%.200s+%d", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    instruction->operandData.operands[2].value.unsigned_value,
                                    instruction->operandData.operands[1].value.reg);
                        else
                            sprintf(out, "r%d,r%d,%.200s-%d", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                    -instruction->operandData.operands[2].value.unsigned_value,
                                    instruction->operandData.operands[1].value.reg);
                    } break;
                }
            } else {
                sprintf(out, "r%d,r%d,", instruction->operandData.operands[0].value.reg,
                        instruction->operandData.operands[1].value.reg);
                format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
            }
            break;
        case PC_ADDIC:
        case PC_ADDICR:
        case PC_MULLI:
        case PC_SUBFIC:
        case PC_SRAWI:
            sprintf(out, "r%d,r%d,", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
            break;
        case PC_CMPI:
        case PC_CMPLI:
            sprintf(out, "cr%d,r%d,", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
            break;
        case PC_ANDI:
        case PC_ANDIS:
        case PC_ORIS:
        case PC_XORI:
        case PC_XORIS:
            sprintf(out, "r%d,r%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_ADDME:
        case PC_ADDZE:
        case PC_NEG:
        case PC_SUBFME:
        case PC_SUBFZE:
        case PC_EXTSB:
        case PC_EXTSH:
        case PC_CNTLZW:
        case PC_MR:
        case PC_NOT:
            sprintf(out, "r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_LI:
        case PC_LIS:
            if (instruction->operand_count == 2) {
                if (instruction->operandData.operands[1].kind == PCOp_MEMORY) {
                    unsigned char operandFlags = instruction->operandData.operands[1].flags;
                    switch (operandFlags) {
                        case 8:
                            sprintf(out, "r%d,HA(%.200s)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[1].object)->name);
                            break;
                        default:
                            if (instruction->operandData.operands[1].value.signed_value == 0)
                                sprintf(
                                    out, "r%d,%.200s", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[1].object)->name);
                            else if (instruction->operandData.operands[2].value.signed_value > 0)
                                sprintf(out, "r%d,%.200s+%d", instruction->operandData.operands[0].value.reg,
                                        COptimizer_GetFunctionObject(instruction->operandData.operands[1].object)->name,
                                        instruction->operandData.operands[1].value.unsigned_value);
                            else
                                sprintf(out, "r%d,%.200s-%d", instruction->operandData.operands[0].value.reg,
                                        COptimizer_GetFunctionObject(instruction->operandData.operands[1].object)->name,
                                        -instruction->operandData.operands[1].value.unsigned_value);
                            break;
                    }
                } else {
                    sprintf(out, "r%d,%d", instruction->operandData.operands[0].value.reg,
                            instruction->operandData.operands[1].value.unsigned_value);
                }
            } else {
                if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                    unsigned char operandFlags = instruction->operandData.operands[2].flags;
                    switch (operandFlags) {
                        case 8:
                            sprintf(out, "r%d,HA(%.200s)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                            break;
                        case 7:
                            sprintf(out, "r%d,HI(%.200s)", instruction->operandData.operands[0].value.reg,
                                    COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name);
                            break;
                        default: {
                            int secondOperand;
                            if ((secondOperand = instruction->operandData.operands[1].value.reg) == 0) {
                                {
                                    int displacement;
                                    if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0)
                                        sprintf(
                                            out, "r%d,%.200s", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name);
                                    else if (displacement > 0)
                                        sprintf(
                                            out, "r%d,%.200s+%d", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name,
                                            instruction->operandData.operands[2].value.unsigned_value);
                                    else
                                        sprintf(
                                            out, "r%d,%.200s-%d", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name,
                                            -instruction->operandData.operands[2].value.unsigned_value);
                                }
                            } else {
                                {
                                    int displacement;
                                    if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0)
                                        sprintf(
                                            out, "r%d,%.200s(r%d)", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name,
                                            secondOperand);
                                    else if (displacement > 0)
                                        sprintf(
                                            out, "r%d,%.200s+%d(r%d)", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name,
                                            instruction->operandData.operands[2].value.unsigned_value, secondOperand);
                                    else
                                        sprintf(
                                            out, "r%d,%.200s-%d(r%d)", instruction->operandData.operands[0].value.reg,
                                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)
                                                ->name,
                                            -instruction->operandData.operands[2].value.unsigned_value, secondOperand);
                                }
                            }
                        } break;
                    }
                } else {
                    sprintf(out, "r%d,r%d,%d", instruction->operandData.operands[0].value.reg,
                            instruction->operandData.operands[1].value.reg,
                            instruction->operandData.operands[2].value.unsigned_value);
                }
            }
            break;
        case PC_RLWINM:
        case PC_RLWIMI:
            sprintf(out, "r%d,r%d,%d,%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value,
                    instruction->operandData.operands[3].value.unsigned_value,
                    instruction->operandData.operands[4].value.unsigned_value);
            break;
        case PC_RLWNM:
            sprintf(out, "r%d,r%d,r%d,%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg,
                    instruction->operandData.operands[3].value.unsigned_value,
                    instruction->operandData.operands[4].value.unsigned_value);
            break;
        case PC_MTXER:
        case PC_MTCTR:
        case PC_MTLR:
        case PC_MFXER:
        case PC_MFCTR:
        case PC_MFLR:
        case PC_MFCR:
            sprintf(out, "r%d", instruction->operandData.operands[0].value.reg);
            break;
        case PC_MTCRF:
            sprintf(out, "%#x,r%d", instruction->operandData.operands[0].value.unsigned_value,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_MFFS:
            sprintf(out, "f%d", instruction->operandData.operands[0].value.reg);
            break;
        case PC_MTFSF:
            sprintf(out, "%#x,f%d", instruction->operandData.operands[0].value.unsigned_value,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_BLR:
        case PC_BCTR:
        case PC_BCTRL:
        case PC_BLRL:
        case PC_EIEIO:
        case PC_ISYNC:
        case PC_SYNC:
        case PC_NOP:
            *out = 0;
            break;
        case PC_LFS:
        case PC_LFSU:
        case PC_LFD:
        case PC_LFDU:
        case PC_STFS:
        case PC_STFSU:
        case PC_STFD:
        case PC_STFDU:
            if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                if (instruction->operandData.operands[2].flags == 6) {
                    sprintf(out, "f%d,LO(%.200s)(r%d)", instruction->operandData.operands[0].value.reg,
                            COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                            instruction->operandData.operands[1].value.reg);
                } else {
                    int displacement;
                    if ((displacement = instruction->operandData.operands[2].value.signed_value) == 0) {
                        sprintf(out, "f%d,%.200s(r%d)", instruction->operandData.operands[0].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                instruction->operandData.operands[1].value.reg);
                    } else if (displacement > 0) {
                        sprintf(out, "f%d,%.200s+%d(r%d)", instruction->operandData.operands[0].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                instruction->operandData.operands[2].value.unsigned_value,
                                instruction->operandData.operands[1].value.reg);
                    } else {
                        sprintf(out, "f%d,%.200s-%d(r%d)", instruction->operandData.operands[0].value.reg,
                                COptimizer_GetFunctionObject(instruction->operandData.operands[2].object)->name,
                                -instruction->operandData.operands[2].value.unsigned_value,
                                instruction->operandData.operands[1].value.reg);
                    }
                }
            } else {
                sprintf(out, "f%d,", instruction->operandData.operands[0].value.reg);
                format_operand((PCodeOperand *)&instruction->operandData.operands[2], out);
                sprintf(addressSuffix, "(r%d)", instruction->operandData.operands[1].value.reg);
                CPrep_AppendStringBounded(out, addressSuffix, 300);
            }
            break;
        case PC_LFSX:
        case PC_LFSUX:
        case PC_LFDX:
        case PC_LFDUX:
        case PC_STFSX:
        case PC_STFSUX:
        case PC_STFDX:
        case PC_STFDUX:
        case PC_STFIWX:
            sprintf(out, "f%d,(r%d,r%d)", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_FMR:
        case PC_FABS:
        case PC_FNEG:
        case PC_FNABS:
        case PC_FRES:
        case PC_FRSQRTE:
        case PC_FRSP:
        case PC_FCTIW:
        case PC_FCTIWZ:
        case PC_PS_RES:
        case PC_PS_ABS:
        case PC_PS_NABS:
        case PC_PS_NEG:
        case PC_PS_MR:
        case PC_PS_RSQRTE:
            sprintf(out, "f%d,f%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_FADD:
        case PC_FADDS:
        case PC_FSUB:
        case PC_FSUBS:
        case PC_FMUL:
        case PC_FMULS:
        case PC_FDIV:
        case PC_FDIVS:
        case PC_PS_ADD:
        case PC_PS_SUB:
        case PC_PS_MUL:
        case PC_PS_DIV:
        case PC_PS_MERGE00:
        case PC_PS_MERGE01:
        case PC_PS_MERGE10:
        case PC_PS_MERGE11:
        case PC_PS_MULS0:
        case PC_PS_MULS1:
            sprintf(out, "f%d,f%d,f%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_FMADD:
        case PC_FMADDS:
        case PC_FMSUB:
        case PC_FMSUBS:
        case PC_FNMADD:
        case PC_FNMADDS:
        case PC_FNMSUB:
        case PC_FNMSUBS:
        case PC_FSEL:
        case PC_PS_MADD:
        case PC_PS_MSUB:
        case PC_PS_NMADD:
        case PC_PS_NMSUB:
        case PC_PS_SEL:
        case PC_PS_SUM0:
        case PC_PS_SUM1:
        case PC_PS_MADDS0:
        case PC_PS_MADDS1:
            sprintf(out, "f%d,f%d,f%d,f%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg,
                    instruction->operandData.operands[3].value.reg);
            break;
        case PC_FCMPU:
        case PC_FCMPO:
        case PC_PS_CMPU0:
        case PC_PS_CMPO0:
        case PC_PS_CMPU1:
        case PC_PS_CMPO1:
            sprintf(out, "cr%d,f%d,f%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_MTSPR:
            if (instruction->operandData.operands[0].kind == PCOp_IMMEDIATE)
                sprintf(out, "%d,r%d", instruction->operandData.operands[0].value.unsigned_value,
                        instruction->operandData.operands[1].value.reg);
            else
                sprintf(out, "%d,r%d", instruction->operandData.operands[0].value.reg,
                        instruction->operandData.operands[1].value.reg);
            break;
        case PC_MFSPR:
            if (instruction->operandData.operands[0].kind == PCOp_IMMEDIATE)
                sprintf(out, "r%d,%d", instruction->operandData.operands[0].value.reg,
                        instruction->operandData.operands[1].value.unsigned_value);
            else
                sprintf(out, "r%d,%d", instruction->operandData.operands[0].value.reg,
                        instruction->operandData.operands[1].value.reg);
            break;
        case PC_MTDCR:
            sprintf(out, "%d,r%d", instruction->operandData.operands[0].value.unsigned_value,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_MFDCR:
            sprintf(out, "r%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_LSWI:
        case PC_STSWI:
            sprintf(out, "r%d,r%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_MCRFS:
            sprintf(out, "%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_MCRXR:
            sprintf(out, "%d", instruction->operandData.operands[0].value.reg);
            break;
        case PC_MFTB:
            sprintf(out, "r%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_MFSR:
        case PC_MTSR:
            sprintf(out, "r%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_MTFSB0:
        case PC_MTFSB1:
            sprintf(out, "%d", instruction->operandData.operands[0].value.unsigned_value);
            break;
        case PC_MFSRIN:
        case PC_MTSRIN:
        case PC_MTFSFI:
        case PC_MFROM:
            sprintf(out, "r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_FSQRT:
        case PC_FSQRTS:
            sprintf(out, "f%d,f%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_MTMSR:
        case PC_MFMSR:
        case PC_TLBIE:
        case PC_TLBLD:
        case PC_TLBLI:
            sprintf(out, "r%d", instruction->operandData.operands[0].value.reg);
            break;
        case PC_TW:
            sprintf(out, "%s,r%d,r%d", data_005621e0[instruction->operandData.operands[0].value.unsigned_value],
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_TWI:
            sprintf(out, "%s,r%d,0x%x", data_005621e0[instruction->operandData.operands[0].value.unsigned_value],
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_OPWORD:
            *out = 0;
            format_operand((PCodeOperand *)&instruction->operandData.operands[0], out);
            break;
        case PC_CLCS:
            sprintf(out, "r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_NABS:
        case PC_ABS:
        case PC_DOZI:
            sprintf(out, "r%d,r%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_RLMI:
            sprintf(out, "r%d,r%d,r%d,%d,%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg,
                    instruction->operandData.operands[3].value.unsigned_value,
                    instruction->operandData.operands[4].value.unsigned_value);
            break;
        case PC_SLE:
        case PC_SLEQ:
        case PC_SLLQ:
        case PC_SLQ:
        case PC_SRAQ:
        case PC_SRE:
        case PC_SREA:
        case PC_SREQ:
        case PC_SRLQ:
        case PC_SRQ:
        case PC_MASKG:
        case PC_MASKIR:
        case PC_LSCBX:
        case PC_DIV:
        case PC_DIVS:
        case PC_DOZ:
        case PC_MUL:
        case PC_RRIB:
            sprintf(out, "r%d,r%d,r%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_SLIQ:
        case PC_SLLIQ:
        case PC_SRAIQ:
        case PC_SRIQ:
        case PC_SRLIQ:
            sprintf(out, "r%d,r%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_DSTT:
        case PC_DSTSTT:
            sprintf(out, "r%d,r%d,STRM %d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_DST:
        case PC_DSTST:
            sprintf(out, "r%d,r%d,STRM %d, %d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value,
                    instruction->operandData.operands[3].value.unsigned_value);
            break;
        case PC_DSS:
        case PC_DSSALL:
            sprintf(out, "STRM %d, %d", instruction->operandData.operands[0].value.unsigned_value,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_LVEBX:
        case PC_LVEHX:
        case PC_LVEWX:
        case PC_LVSL:
        case PC_LVSR:
        case PC_LVX:
        case PC_LVXL:
        case PC_STVEBX:
        case PC_STVEHX:
        case PC_STVEWX:
        case PC_STVX:
        case PC_STVXL:
            sprintf(out, "vr%d,(r%d,r%d)", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_VADDCUW:
        case PC_VADDFP:
        case PC_VADDSBS:
        case PC_VADDSHS:
        case PC_VADDSWS:
        case PC_VADDUBM:
        case PC_VADDUBS:
        case PC_VADDUHM:
        case PC_VADDUHS:
        case PC_VADDUWM:
        case PC_VADDUWS:
        case PC_VAND:
        case PC_VANDC:
        case PC_VAVGSB:
        case PC_VAVGSH:
        case PC_VAVGSW:
        case PC_VAVGUB:
        case PC_VAVGUH:
        case PC_VAVGUW:
        case PC_VCMPBFP:
        case PC_VCMPEQFP:
        case PC_VCMPEQUB:
        case PC_VCMPEQUH:
        case PC_VCMPEQUW:
        case PC_VCMPGEFP:
        case PC_VCMPGTFP:
        case PC_VCMPGTSB:
        case PC_VCMPGTSH:
        case PC_VCMPGTSW:
        case PC_VCMPGTUB:
        case PC_VCMPGTUH:
        case PC_VCMPGTUW:
        case PC_VMAXFP:
        case PC_VMAXSB:
        case PC_VMAXSH:
        case PC_VMAXSW:
        case PC_VMAXUB:
        case PC_VMAXUH:
        case PC_VMAXUW:
        case PC_VMINFP:
        case PC_VMINSB:
        case PC_VMINSH:
        case PC_VMINSW:
        case PC_VMINUB:
        case PC_VMINUH:
        case PC_VMINUW:
        case PC_VMRGHB:
        case PC_VMRGHH:
        case PC_VMRGHW:
        case PC_VMRGLB:
        case PC_VMRGLH:
        case PC_VMRGLW:
        case PC_VMULESB:
        case PC_VMULESH:
        case PC_VMULEUB:
        case PC_VMULEUH:
        case PC_VMULOSB:
        case PC_VMULOSH:
        case PC_VMULOUB:
        case PC_VMULOUH:
        case PC_VNOR:
        case PC_VOR:
        case PC_VPKPX:
        case PC_VPKSHSS:
        case PC_VPKSHUS:
        case PC_VPKSWSS:
        case PC_VPKSWUS:
        case PC_VPKUHUM:
        case PC_VPKUHUS:
        case PC_VPKUWUM:
        case PC_VPKUWUS:
        case PC_VRLB:
        case PC_VRLH:
        case PC_VRLW:
        case PC_VSL:
        case PC_VSLB:
        case PC_VSLH:
        case PC_VSLO:
        case PC_VSLW:
        case PC_VSR:
        case PC_VSRAB:
        case PC_VSRAH:
        case PC_VSRAW:
        case PC_VSRB:
        case PC_VSRH:
        case PC_VSRO:
        case PC_VSRW:
        case PC_VSUBCUW:
        case PC_VSUBFP:
        case PC_VSUBSBS:
        case PC_VSUBSHS:
        case PC_VSUBSWS:
        case PC_VSUBUBM:
        case PC_VSUBUBS:
        case PC_VSUBUHM:
        case PC_VSUBUHS:
        case PC_VSUBUWM:
        case PC_VSUBUWS:
        case PC_VSUMSWS:
        case PC_VSUM2SWS:
        case PC_VSUM4SBS:
        case PC_VSUM4SHS:
        case PC_VSUM4UBS:
        case PC_VXOR:
            sprintf(out, "vr%d,vr%d,vr%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg);
            break;
        case PC_VCFSX:
        case PC_VCFUX:
        case PC_VCTSXS:
        case PC_VCTUXS:
        case PC_VSPLTB:
        case PC_VSPLTH:
        case PC_VSPLTW:
            sprintf(out, "vr%d,vr%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg,
                    instruction->operandData.operands[2].value.unsigned_value);
            break;
        case PC_VEXPTEFP:
        case PC_VLOGEFP:
        case PC_VREFP:
        case PC_VRFIM:
        case PC_VRFIN:
        case PC_VRFIP:
        case PC_VRFIZ:
        case PC_VRSQRTEFP:
        case PC_VUPKHPX:
        case PC_VUPKHSB:
        case PC_VUPKHSH:
        case PC_VUPKLPX:
        case PC_VUPKLSB:
        case PC_VUPKLSH:
        case PC_VMR:
        case PC_VMRP:
            sprintf(out, "vr%d,vr%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg);
            break;
        case PC_VSPLTISB:
        case PC_VSPLTISH:
        case PC_VSPLTISW:
            sprintf(out, "vr%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.unsigned_value);
            break;
        case PC_VMADDFP:
        case PC_VMHADDSHS:
        case PC_VMHRADDSHS:
        case PC_VMLADDUHM:
        case PC_VMSUMMBM:
        case PC_VMSUMSHM:
        case PC_VMSUMSHS:
        case PC_VMSUMUBM:
        case PC_VMSUMUHM:
        case PC_VMSUMUHS:
        case PC_VNMSUBFP:
        case PC_VPERM:
        case PC_VSEL:
            sprintf(out, "vr%d,vr%d,vr%d,vr%d", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg,
                    instruction->operandData.operands[3].value.reg);
            break;
        case PC_VSLDOI:
            sprintf(out, "vr%d,vr%d,vr%d,0x%x", instruction->operandData.operands[0].value.reg,
                    instruction->operandData.operands[1].value.reg, instruction->operandData.operands[2].value.reg,
                    instruction->operandData.operands[3].value.unsigned_value);
            break;
        case PC_MFVSCR:
        case PC_MTVSCR:
            sprintf(out, "vr%d", instruction->operandData.operands[0].value.reg);
            break;
        case PC_RFI:
        case PC_SC:
        case PC_TLBIA:
        case PC_TLBSYNC:
        case PC_TRAP:
        case PC_DSA:
        case PC_ESA:
            break;
        case PC_BTCTR:
        case PC_BFCTR:
        case PC_DCCCI:
        case PC_DCREAD:
        case PC_ICBT:
        case PC_ICCCI:
        case PC_ICREAD:
        case PC_RFCI:
        case PC_TLBRE:
        case PC_TLBSX:
        case PC_TLBWE:
        case PC_WRTEE:
        case PC_WRTEEI:
        default:
            CError_FATAL(1107);
            break;
    }
    if (instruction->flags & fIsConst)
        strcat(out, "; fIsConst");
    if (instruction->flags & fIsVolatile)
        strcat(out, "; fIsVolatile");
    if (instruction->flags & fSideEffects)
        strcat(out, "; fSideEffects");
    if (instruction->flags & fLink)
        strcat(out, "; fLink");
    if (instruction->flags & fAbsolute)
        strcat(out, "; fAbsolute");
    if (instruction->flags & fOverflow)
        strcat(out, "; fOverflow");
    if (instruction->flags & fCallerSPRelative)
        strcat(out, "; fCallerSPRelative");
    if (instruction->flags & fIsLive)
        strcat(out, "; fIsLive");
    if (instruction->flags & fIsPtrOp)
        strcat(out, "; fIsPtrOp");
}

static void PrintOp0(PCodeInstruction *p, char *out)
{
    unsigned char t = p->operandData.operands[0].kind;
    const char *sign;
    int *q;
    if (t == 5) {
        sprintf(out, "%.200s", COptimizer_GetFunctionObject(p->operandData.operands[0].object)->name);
    } else if (t != 6) {
        sign = p->operandData.operands[0].value.signed_value >= 0 ? "+" : "";
        sprintf(out, "*%s%ld", sign, p->operandData.operands[0].value.signed_value);
    } else {
        q = (int *)((int *)p->operandData.operands[0].value.signed_value)[1];
        if (q == NULL)
            sprintf(out, "B<unknown>");
        else
            sprintf(out, "B%ld", q[7]);
    }
}

void fn_004c4be0(void)
{
    return;
}

void CodeGen_DumpPCode_004c4bd0(const char *function_name, const char *stage)
{
    return;
}

void fn_004c4bc0(const char *format, int register_count)
{
    return;
}

void fn_004c4bb0(char *listing, char *filename)
{
}

void fn_004c4ba0(void)
{
    return;
}

void pclistdataflowanalysis(UInt32 *use, UInt32 *def, UInt32 *in, UInt32 *out, UInt32 size)
{
    formatdataflowset("use", use, size);
    formatdataflowset("def", def, size);
    formatdataflowset("in ", in, size);
    formatdataflowset("out", out, size);
}

static void pclistblock(PCodeBlock *block)
{
    PCodeInstruction *instr;
    PCodeBlockLink *link;

    fprintf(stdout, ":SRC_LINE=%ld:{%4.4x}::::::::::::::::::::::::::::::::::::::::LOOPWEIGHT=%ld\n", block->line,
            block->code_offset, block->execution_weight);
    fprintf(stdout, "B%ld: ", block->index);
    fprintf(stdout, "Successors = { ");
    for (link = block->successors; link; link = link->next)
        fprintf(stdout, "B%ld ", link);
    fprintf(stdout, "}  ");
    fprintf(stdout, "Predecessors = { ");
    for (link = block->predecessors; link; link = link->next)
        fprintf(stdout, "B%ld ", link);
    fprintf(stdout, "}  Labels = { ");
    fprintf(stdout, "L%ld ", block->labels);
    fprintf(stdout, "}\n\n");
    for (instr = block->instructions; instr; instr = instr->next)
        fprintf(stdout, "    %.8lX  %.8lX %4ld    %-7s%c %s\n", instr, block, block->line, "", ' ', "");
    fprintf(stdout, "............................................................\n");
}

static void pclistonoff(int flag)
{
    if (flag)
        fprintf(stdout, "On\n");
    else
        fprintf(stdout, "Off\n");
}

static void formatflags(char *buf, UInt32 flags)
{
    *buf = 0;
    if (flags & 1)
        strcat(buf, "fSpilled");
    if (flags & 2) {
        if (*buf)
            strcat(buf, "|");
        strcat(buf, "fPushed");
    }
    if (flags & 4) {
        if (*buf)
            strcat(buf, "|");
        strcat(buf, "fCoalesced");
    }
    if (flags & 8) {
        if (*buf)
            strcat(buf, "|");
        strcat(buf, "fCoalescedInto");
    }
    if (flags & 0x10) {
        if (*buf)
            strcat(buf, "|");
        strcat(buf, "fPairHigh");
    }
    if (flags & 0x20) {
        if (*buf)
            strcat(buf, "|");
        strcat(buf, "fPairLow");
    }
    if (!*buf)
        strcpy(buf, "no_flags");
}
