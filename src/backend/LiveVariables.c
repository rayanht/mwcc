#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "compiler/LiveVariables.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BitVectors.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CRTTI.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"

#include <string.h>

void SpillCode_BuildLocalLiveness(int reg_class)
{
    PCodeBlock *block;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        PCodeLiveness *liveness;
        PCodeInstruction *instruction;
        UInt32 *use;
        UInt32 *def;

        liveness = &gPCodeBlockLiveness[block->index];
        use = liveness->use;
        def = liveness->def;
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            PCodeOperand *operand;
            int count;

            operand = instruction->operandData.operands;
            count = instruction->operand_count;
            while (count--) {
                if (operand->kind == reg_class && ((signed char)operand->flags & PCodeOperand_Use) != 0) {
                    int reg = operand->value.reg;
                    if ((def[reg >> 5] & (1U << reg)) == 0) {
                        use[reg >> 5] |= 1U << reg;
                    }
                }
                operand++;
            }
            operand = instruction->operandData.operands;
            count = instruction->operand_count;
            while (count--) {
                if (operand->kind == reg_class && ((signed char)operand->flags & PCodeOperand_Definition) != 0) {
                    int reg = operand->value.reg;
                    if ((use[reg >> 5] & (1U << reg)) == 0) {
                        def[reg >> 5] |= 1U << reg;
                    }
                }
                operand++;
            }
        }
    }
}

void SpillCode_SolveLiveness(UInt32 nbits)
{
    PCodeBlockLink *successor;
    PCodeLiveness *live;
    UInt32 *def;
    UInt32 *live_out;
    UInt32 *use;
    UInt32 *live_in;
    UInt32 value;
    int word;
    SInt32 changed;
    SInt32 nwords;
    SInt32 count;
    PCodeBlock *block;

    nwords = (nbits + 31) >> 5;
    do {
        changed = 0;
        count = gPCodeBlockCount;
        while (count) {
            count--;
            if ((block = gPCodeBlockOrder[count]) == NULL)
                continue;
            live = &gPCodeBlockLiveness[block->index];
            if ((successor = block->successors) != NULL) {
                live_out = live->liveout;
                CodeMotion_AllocateBits(live_out, gPCodeBlockLiveness[successor->payload.block->index].livein, nbits);
                for (successor = successor->next; successor != NULL; successor = successor->next)
                    CRTTI_OrBitVector(live_out, gPCodeBlockLiveness[successor->payload.block->index].livein, nbits);
            }
            live_out = live->liveout;
            live_in = live->livein;
            use = live->use;
            def = live->def;
            for (word = 0; word < nwords; word++) {
                value = (~*def & *live_out) | *use;
                if (value != *live_in) {
                    *live_in = value;
                    changed = 1;
                }
                live_in++;
                live_out++;
                use++;
                def++;
            }
        }
    } while (changed);
}

/* PCodeBlock: the object a liveness entry is indexed by; its block number
 * lives at 0x1c. */

void SpillCode_InitializeLiveness(Object *func, SInt32 mode, UInt32 nbits)
{
    Type *returnType;
    int blockIndex;
    PCodeLiveness *liveness;

    returnType = ((TypeFunc *)func->type)->functype;

    SpillCode_BuildBlockOrder();
    gPCodeBlockLiveness = (PCodeLiveness *)CompilerTools_AllocatePoolMemory(gPCodeBlockCount * sizeof(PCodeLiveness));
    liveness = gPCodeBlockLiveness;

    for (blockIndex = 0; blockIndex < gPCodeBlockCount; blockIndex++) {
        CRTTI_FillWords(liveness->use = CompilerTools_AllocatePoolMemory(((nbits + 31) >> 5) << 2), nbits, 0);
        CRTTI_FillWords(liveness->def = CompilerTools_AllocatePoolMemory(((nbits + 31) >> 5) << 2), nbits, 0);
        CRTTI_FillWords(liveness->livein = CompilerTools_AllocatePoolMemory(((nbits + 31) >> 5) << 2), nbits, 0);
        CRTTI_FillWords(liveness->liveout = CompilerTools_AllocatePoolMemory(((nbits + 31) >> 5) << 2), nbits, 0);
        liveness++;
    }

    SpillCode_BuildLocalLiveness(mode);

    if (mode == 0 && (returnType->type == TYPEINT || returnType->type == TYPEENUM || returnType->type == TYPEPOINTER ||
                      (returnType->type == TYPEMEMBERPOINTER && returnType->size == 4))) {
        gPCodeBlockLiveness[gReturnBlock->index].use[0] |= 8;
        if ((returnType->type == TYPEINT || returnType->type == TYPEENUM) && returnType->size == 8) {
            gPCodeBlockLiveness[gCurrentBlock->index].use[0] |= 0x10;
        }
    } else if (mode == 0 && (returnType->type == TYPESTRUCT || returnType->type == TYPECLASS) &&
               !Type_RequiresMemoryReturn(returnType)) {
        gPCodeBlockLiveness[gReturnBlock->index].use[0] |= 8;
        if (returnType->size > 4) {
            gPCodeBlockLiveness[gCurrentBlock->index].use[0] |= 0x10;
        }
    } else if (mode == 0 && copts.operandsDebug && returnType->type == TYPEFLOAT) {
        gPCodeBlockLiveness[gReturnBlock->index].use[0] |= 8;
        if (returnType->size == 8) {
            gPCodeBlockLiveness[gCurrentBlock->index].use[0] |= 0x10;
        }
    } else if (mode == 1 && returnType->type == TYPEFLOAT) {
        gPCodeBlockLiveness[gReturnBlock->index].use[0] |= 2;
    } else if (mode == 9 && returnType->type == TYPESTRUCT) {
        SInt32 structKind = ((TypeStruct *)returnType)->stype;
        if (structKind >= 4 && structKind <= 0xe) {
            gPCodeBlockLiveness[gReturnBlock->index].use[0] |= 4;
        }
    }

    SpillCode_SolveLiveness(nbits);
}

SInt32 SpillCode_IsDeadInstruction(PCodeInstruction *instruction, SInt16 regClass, UInt32 *live)
{
    PCodeOperand *operand;
    SInt32 remaining;

    if (instruction->flags & 0x20434)
        return 0;
    if (instruction->block->flags & 3)
        return 0;

    remaining = instruction->operand_count;
    operand = instruction->operandData.operands;
    if (remaining--) {
        SInt32 operandClass = regClass;
        do {
            if (operand->kind == PCOp_GPR && (operand->flags & 2)) {
                if (operandClass != 0 || (live[operand->value.reg >> 5] & (1 << operand->value.reg)) != 0)
                    return 0;
            } else if (operand->kind == PCOp_FPR && (operand->flags & 2)) {
                if (operandClass != 1 || (live[operand->value.reg >> 5] & (1 << operand->value.reg)) != 0)
                    return 0;
            } else if (operand->kind == PCOp_VR && (operand->flags & 2)) {
                if (operandClass != 9 || (live[operand->value.reg >> 5] & (1 << operand->value.reg)) != 0)
                    return 0;
            } else if (operand->kind == PCOp_SPR && (operand->flags & 2)) {
                return 0;
            } else if (operand->kind == PCOp_CRFIELD && (operand->flags & 2)) {
                return 0;
            }
            operand++;
        } while (remaining--);
    }

    if (copts.deleteDeadInstructions <= 0)
        return 0;
    return 1;
}
