#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BitVectors.h"
#include "compiler/CExpr2.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CRTTI.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/Coloring.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/InstrSelection.h"
#include "compiler/LiveVariables.h"
#include "compiler/PCode.h"
#include "compiler/PCodeListing.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/SpillCode.h"
#include "compiler/StackFrameEABI.h"

struct PCodeBlock *gPCodeBlocks;
short gGPRCoalesceFirst;
short gFPRCoalesceFirst;
short gVRCoalesceFirst;

static UInt32 *gInterferenceBits;
static short *gCoalescedRegisters;

static void Coalesce_AddInterference(UInt32 r1, UInt32 r2)
{
    UInt32 bit;

    if (r1 < r2) {
        bit = (r2 * r2 >> 1) + r1;
        gInterferenceBits[bit >> 5] |= 1 << bit;
    } else if (r1 > r2) {
        bit = (r1 * r1 >> 1) + r2;
        gInterferenceBits[bit >> 5] |= 1 << bit;
    }
}

static int test_interference(UInt32 a, UInt32 b)
{
    if (a < b) {
        UInt32 idx = (b * b >> 1) + a;
        return (gInterferenceBits[idx >> 5] & (1 << idx)) != 0;
    } else if (a > b) {
        UInt32 idx = (a * a >> 1) + b;
        return (gInterferenceBits[idx >> 5] & (1 << idx)) != 0;
    }
    return 0;
}

#define SC_Clear(n, bits) ((bits)[(SInt16)(n) >> 5] &= ~(1 << ((UInt32)(SInt16)(n) & 31)))
#define SC_Test(n, bits) ((bits)[(SInt16)(n) >> 5] & (1 << ((UInt32)(SInt16)(n) & 31)))
static inline void SC_Add(SInt16 n, UInt32 *bits)
{
    int v = n;
    bits[v >> 5] |= 1 << ((UInt32)v & 31);
}

void SpillCode_ConstructInterference(SInt32 registerClass, UInt32 registerCount)
{
    UInt32 *bits;
    PCodeBlock *block;
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    SInt16 reg;
    UInt32 i;
    UInt32 j;
    UInt32 k;
    gInterferenceBits = (UInt32 *)oalloc(((registerCount * registerCount / 2 + 0x1f) >> 5) << 2);
    BitVectors_FillWords(gInterferenceBits, registerCount * registerCount / 2, 0);
    for (i = 0; i < 0x20; i++) {
        for (j = 0; j < 0x20; j++) {
            if (i != j)
                Coalesce_AddInterference(i, j);
        }
    }
    bits = (UInt32 *)oalloc(((registerCount + 0x1f) >> 5) << 2);
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        CodeMotion_AllocateBits(bits, gPCodeBlockLiveness[block->index].liveout, registerCount);
        for (instruction = block->reverse_instructions; instruction != NULL; instruction = instruction->previous) {
            for (i = instruction->operand_count, operand = instruction->operandData.operands; i--; operand++) {
                if (operand->kind == registerClass && (operand->flags & 2)) {
                    SC_Clear(reg = operand->value.reg, bits);
                    for (j = 0; j < registerCount; j++) {
                        if (bits[j >> 5] & (1 << (j & 0x1f))) {
                            if (!(instruction->flags & fIsMove) ||
                                instruction->operandData.operands[1].value.reg != j) {
                                Coalesce_AddInterference(reg, j);
                            }
                        }
                    }
                }
            }
            for (j = instruction->operand_count, operand = instruction->operandData.operands; j--; operand++) {
                if (operand->kind == registerClass && (operand->flags & 1)) {
                    if (!SC_Test(reg = operand->value.reg, bits))
                        operand->flags |= 4;
                    SC_Add(reg, bits);
                }
            }
            if (registerClass == 0) {
                if (instruction->flags & (fIsRead | fIsWrite)) {
                    if (instruction->operandData.operands[1].value.reg >= 0x20)
                        Coalesce_AddInterference(0, instruction->operandData.operands[1].value.reg);
                    if (instruction->flags & 0x8000)
                        Coalesce_AddInterference(instruction->operandData.operands[0].value.reg,
                                                 instruction->operandData.operands[1].value.reg);
                } else if (instruction->opcode == PC_ADDI || instruction->opcode == PC_ADDIS) {
                    if (instruction->operandData.operands[1].value.reg >= 0x20)
                        Coalesce_AddInterference(0, instruction->operandData.operands[1].value.reg);
                } else if (instruction->opcode >= 0x37 && instruction->opcode <= 0x3b) {
                    if (instruction->operandData.operands[0].value.reg >= 0x20)
                        Coalesce_AddInterference(0, instruction->operandData.operands[0].value.reg);
                }
            }
            if (registerClass == 0 && (instruction->flags & 0x20)) {
                PCodeOperand *extraOperand;
                for (k = 0x32, extraOperand = &instruction->operandData.operands[k]; k < instruction->operand_count;
                     k++, extraOperand++) {
                    SInt16 tailReg;
                    Coalesce_AddInterference(tailReg = extraOperand->value.reg, 0);
                    for (i = 3; i <= 0xc; i++)
                        Coalesce_AddInterference(tailReg, i);
                }
            }
        }
    }
}

static SInt16 Coalesce_Find(SInt16 r)
{
    while (gCoalescedRegisters[r] != r)
        r = gCoalescedRegisters[r];
    return r;
}

static inline void Coalesce_FindNext(PCodeOperand *sig)
{
    SInt16 r;

    r = sig->value.reg;
    if (gCoalescedRegisters[r] != r)
        sig->value.reg = Coalesce_Find(r);
}

static inline int Coalesce_InRange(SInt16 r, SInt32 regclass)
{
    if (regclass == 0)
        return r >= gGPRCoalesceFirst && r <= gGPRCoalesceLast;
    if (regclass == 1)
        return r >= gFPRCoalesceFirst && r <= gFPRCoalesceLast;
    return r >= gVRCoalesceFirst && r <= gVRCoalesceLast;
}

void SpillCode_CoalesceCopies(SInt32 regclass, UInt32 numRegs)
{
    SInt16 opcode;
    UInt32 i;
    PCodeBlock *block;
    PCodeInstruction *instr;
    SInt16 destinationReg, sourceReg;
    SInt16 lowerReg, upperReg;

    if (regclass == 0)
        opcode = 0x8b;
    else if (regclass == 1)
        opcode = 0x9e;
    else
        opcode = 0x18e;

    gCoalescedRegisters = oalloc(numRegs * sizeof(*gCoalescedRegisters));

    for (i = 0; i < numRegs; i++)
        gCoalescedRegisters[i] = (SInt16)i;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (instr = block->instructions; instr != NULL; instr = instr->next) {
            if (instr->opcode != opcode || (instr->flags & fSideEffects) != 0)
                continue;
            destinationReg = Coalesce_Find(instr->operandData.operands[0].value.reg);
            sourceReg = Coalesce_Find(instr->operandData.operands[1].value.reg);
            if (destinationReg == sourceReg) {
                PCode_UnlinkInstruction(instr);
                continue;
            }
            if (test_interference(destinationReg, sourceReg))
                continue;
            if (destinationReg < 32 || sourceReg < 32 ||
                (Coalesce_InRange(destinationReg, regclass) && Coalesce_InRange(sourceReg, regclass))) {
                lowerReg = (sourceReg < destinationReg) ? sourceReg : destinationReg;
                upperReg = (sourceReg > destinationReg) ? sourceReg : destinationReg;
                gCoalescedRegisters[upperReg] = lowerReg;
                for (i = 0; i < numRegs; i++)
                    if (test_interference(upperReg, i))
                        Coalesce_AddInterference(lowerReg, i);
                PCode_UnlinkInstruction(instr);
            }
        }
    }

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        PCodeOperand *operand;

        for (instr = block->instructions; instr != NULL; instr = instr->next) {
            operand = instr->operandData.operands;
            i = instr->operand_count;
            while (i--) {
                if (operand->kind == regclass)
                    Coalesce_FindNext(operand);
                operand++;
            }
        }
    }
}

void SpillCode_MaterializeGraph(UInt32 count)
{
    SInt16 *neighbors;
    InterferenceNode *node;
    UInt32 register_index;
    UInt32 neighbor_index;
    UInt32 neighbor_count;
    SInt16 representative;
    SInt16 *destination;
    SInt16 *source;
    UInt32 copied;
    int root;

    gInterferenceGraph = oalloc(count * sizeof(*gInterferenceGraph));
    neighbors = oalloc(count * sizeof(*neighbors));
    for (register_index = 0; register_index < count; register_index++) {
        neighbor_count = 0;
        for (neighbor_index = 0; neighbor_index < count; neighbor_index++) {
            if (test_interference(register_index, neighbor_index)) {
                neighbors[neighbor_count] = neighbor_index;
                neighbor_count++;
            }
        }
        node = gInterferenceGraph[register_index] =
            oalloc(sizeof(InterferenceNode) + (neighbor_count - 1) * sizeof(*neighbors));
        node->next = NULL;
        node->object = NULL;
        node->spill_cost = 0;
        node->virtual_register = register_index;
        node->physical_register = -1;
        node->flags = 0;
        node->degree = node->neighbor_count = neighbor_count;
        destination = node->neighbors;
        source = neighbors;
        for (copied = 0; copied < neighbor_count; copied++)
            *destination++ = *source++;
        if (register_index != gCoalescedRegisters[register_index]) {
            node->flags |= 4;
            representative = register_index;
            while (representative != gCoalescedRegisters[representative])
                representative = gCoalescedRegisters[representative];
            root = representative;
            gInterferenceGraph[root]->flags |= 8;
            node->physical_register = root;
        }
    }
}

void SpillCode_MarkLastUses(SInt32 var, UInt32 count)
{
    UInt32 *bits;
    PCodeBlock *block;
    PCodeInstruction *instr;
    PCodeOperand *operand;
    SInt32 remaining;
    SInt16 reg;

    bits = (UInt32 *)oalloc(((count + 31) >> 5) * sizeof(UInt32));
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        CodeMotion_AllocateBits(bits, gPCodeBlockLiveness[block->index].liveout, count);
        for (instr = block->reverse_instructions; instr != NULL; instr = instr->previous) {
            if (SpillCode_IsDeadInstruction(instr, var, bits) != 0) {
                PCode_UnlinkInstruction(instr);
            } else {
                operand = instr->operandData.operands;
                remaining = instr->operand_count;
                while (remaining--) {
                    if (operand->kind == var && (operand->flags & 2) != 0) {
                        bits[(reg = operand->value.reg) >> 5] &= ~(1 << ((reg = operand->value.reg) & 31));
                    }
                    operand++;
                }
                operand = instr->operandData.operands;
                remaining = instr->operand_count;
                while (remaining--) {
                    if (operand->kind == var && (operand->flags & 1) != 0) {
                        if ((bits[(reg = operand->value.reg) >> 5] & (1 << ((reg = operand->value.reg) & 31))) == 0)
                            operand->flags |= 4;
                        bits[reg >> 5] |= 1 << (reg & 31);
                    }
                    operand++;
                }
            }
        }
    }
}

static inline const char *SpillCode_RegisterFormat(int reg_class)
{
    if (reg_class == 0) {
        return " r%ld";
    }
    if (reg_class == 1) {
        return " f%ld";
    }
    return " vr%ld";
}

void SpillCode_BuildInterference(Object *function, int reg_class, int register_count)
{
    SpillCode_InitializeLiveness(function, reg_class, register_count);
    SpillCode_MarkLastUses(reg_class, register_count);
    SpillCode_ConstructInterference(reg_class, register_count);
    if (copts.debug_listing) {
        fn_004c4bc0(SpillCode_RegisterFormat(reg_class), register_count);
    }
    SpillCode_CoalesceCopies(reg_class, register_count);
    SpillCode_MaterializeGraph(register_count);
}
