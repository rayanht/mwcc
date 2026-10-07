#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/BitVectors.h"
#include "compiler/CABI.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CRTTI.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/Coloring.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LiveVariables.h"
#include "compiler/LoopOptimization.h"
#include "compiler/MachineSimulation821.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeListing.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/SpillCode.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"

/* Declarations gathered from the merged files. */

#include <string.h>

static UInt32 *gInterferenceBits;
static short *gCoalescedRegisters;

static void SC_Interfere(unsigned int a, unsigned int b);

static const char *SpillCode_RegisterFormat(int reg_class)
{
    if (reg_class == 0) {
        return " r%ld";
    }
    if (reg_class == 1) {
        return " f%ld";
    }
    return " vr%ld";
}

/* 0x5842e1, byte access */

/* PCodeBlock: the object a liveness entry is indexed by; its block number
 * lives at 0x1c. */

/* Per-block liveness record: four bit vectors. */

/* TYPESTRUCT record with the byte classification field at 0x0e. */
void SpillCode_BuildInterference(Object *function, int reg_class, int register_count)
{
    SpillCode_InitializeLiveness(function, reg_class, register_count);
    SpillCode_MarkLastUses(reg_class, register_count);
    SpillCode_ConstructInterference(reg_class, register_count);
    if (copts.cOptimizerDumpEnabled) {
        fn_004c4bc0(SpillCode_RegisterFormat(reg_class), register_count);
    }
    SpillCode_CoalesceCopies(reg_class, register_count);
    SpillCode_MaterializeGraph(register_count);
}

static void SpillCode_ClearLive(UInt32 *live, short reg)
{
    live[reg >> 5] &= ~(1U << (reg & 31));
}

static int SpillCode_IsLive(UInt32 *live, short reg)
{
    return (live[reg >> 5] & (1U << (reg & 31))) != 0;
}

static void SpillCode_SetLive(UInt32 *live, short reg)
{
    live[reg >> 5] |= 1U << (reg & 31);
}

static int SpillCode_WordCount(int register_count)
{
    return (register_count + 31) >> 5;
}

static void SpillCode_CopyBits(UInt32 *destination, const UInt32 *source, int bit_count)
{
    int word;

    for (word = 0; word < SpillCode_WordCount(bit_count); word++) {
        destination[word] = source[word];
    }
}

static void SpillCode_OrBits(UInt32 *destination, const UInt32 *source, int bit_count)
{
    int word;

    for (word = 0; word < SpillCode_WordCount(bit_count); word++) {
        destination[word] |= source[word];
    }
}

static unsigned int *SpillCode_AllocateEmptyBits(int register_count)
{
    unsigned int *bits;
    int word;

    bits = (void *)CompilerTools_AllocatePoolMemory(SpillCode_WordCount(register_count) * sizeof(*bits));
    for (word = 0; word < SpillCode_WordCount(register_count); word++) {
        bits[word] = 0;
    }
    return bits;
}

static void SpillCode_AddBlockUse(PCodeBlock *block, short reg)
{
    if (block != NULL) {
        SpillCode_SetLive(gPCodeBlockLiveness[block->index].use, reg);
    }
}

static int SpillCode_IsDirectGPRScalar(const Type *type)
{
    return type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
           (type->type == TYPEMEMBERPOINTER && type->size == 4);
}

static void SpillCode_SeedGPRReturn(Type *type)
{
    if (SpillCode_IsDirectGPRScalar(type)) {
        SpillCode_AddBlockUse(gReturnBlock, 3);
        if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
            SpillCode_AddBlockUse(gCurrentBlock, 4);
        }
    } else if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && !Type_RequiresMemoryReturn(type)) {
        SpillCode_AddBlockUse(gReturnBlock, 3);
        if (type->size > 4) {
            SpillCode_AddBlockUse(gCurrentBlock, 4);
        }
    } else if (copts.operandsDebug && type->type == TYPEFLOAT) {
        SpillCode_AddBlockUse(gReturnBlock, 3);
        if (type->size == 8) {
            SpillCode_AddBlockUse(gCurrentBlock, 4);
        }
    }
}

static void SpillCode_SeedReturnRegisters(Type *type, int reg_class)
{
    if (reg_class == 0) {
        SpillCode_SeedGPRReturn(type);
    } else if (reg_class == 1 && type->type == TYPEFLOAT) {
        SpillCode_AddBlockUse(gReturnBlock, 1);
    } else if (reg_class == 9 && type->type == TYPESTRUCT && TYPE_STRUCT(type)->stype >= 4 &&
               TYPE_STRUCT(type)->stype <= 14) {
        SpillCode_AddBlockUse(gReturnBlock, 2);
    }
}

static int SpillCode_DefinitionBlocksRemoval(PCodeOperand *operand, int reg_class, UInt32 *live)
{
    if ((operand->flags & PCodeOperand_Definition) == 0) {
        return 0;
    }
    if (operand->kind == PCOp_GPR) {
        return reg_class != 0 || SpillCode_IsLive(live, operand->value.reg);
    }
    if (operand->kind == PCOp_FPR) {
        return reg_class != 1 || SpillCode_IsLive(live, operand->value.reg);
    }
    if (operand->kind == PCOp_VR) {
        return reg_class != 9 || SpillCode_IsLive(live, operand->value.reg);
    }
    return operand->kind == PCOp_SPR || operand->kind == PCOp_CRFIELD;
}

static unsigned int SpillCode_MatrixIndex(int first, int second)
{
    unsigned int index;
    int larger;
    int smaller;

    if (first == second) {
        return (unsigned int)((first * first) / 2);
    }
    if (first > second) {
        larger = first;
        smaller = second;
    } else {
        larger = second;
        smaller = first;
    }
    index = (unsigned int)((larger * larger) / 2 + smaller);
    return index;
}

static int SpillCode_Interferes(int first, int second)
{
    unsigned int index;

    if (first == second) {
        return 0;
    }
    index = SpillCode_MatrixIndex(first, second);
    return (gInterferenceBits[index >> 5] & (1U << (index & 31))) != 0;
}

static void SpillCode_SetMatrixBit(int first, int second)
{
    unsigned int index;

    index = SpillCode_MatrixIndex(first, second);
    gInterferenceBits[index >> 5] |= 1U << (index & 31);
}

static void SpillCode_SetInterference(int first, int second)
{
    if (first != second) {
        SpillCode_SetMatrixBit(first, second);
    }
}

static void SpillCode_ClearWords(unsigned int *bits, int bit_count)
{
    int word;
    int word_count;

    word_count = SpillCode_WordCount(bit_count);
    for (word = 0; word < word_count; word++) {
        bits[word] = 0;
    }
}

static void SpillCode_PrecolorPhysicalRegisters(void)
{
    int first;
    int second;

    for (first = 0; first < 32; first++) {
        for (second = 0; second < 32; second++) {
            SpillCode_SetInterference(first, second);
        }
    }
}

static int SpillCode_CopySourceExcluded(PCodeInstruction *instruction, int reg)
{
    return (instruction->flags & PCodeInstruction_CopySourceExclusion) != 0 &&
           instruction->operandData.operands[1].value.reg == reg;
}

static void SpillCode_AddDefinitionEdges(PCodeInstruction *instruction, int reg_class, UInt32 *live, int register_count)
{
    int index;

    for (index = 0; index < instruction->operand_count; index++) {
        PCodeOperand *operand;
        int other;

        operand = &instruction->operandData.operands[index];
        if (operand->kind != reg_class || (operand->flags & PCodeOperand_Definition) == 0) {
            continue;
        }

        SpillCode_ClearLive(live, operand->value.reg);
        for (other = 0; other < register_count; other++) {
            if (SpillCode_IsLive(live, (short)other) && !SpillCode_CopySourceExcluded(instruction, other)) {
                SpillCode_SetInterference(operand->value.reg, other);
            }
        }
    }
}

static void SpillCode_AddUses(PCodeInstruction *instruction, int reg_class, UInt32 *live)
{
    int index;

    for (index = 0; index < instruction->operand_count; index++) {
        PCodeOperand *operand;

        operand = &instruction->operandData.operands[index];
        if (operand->kind == reg_class && (operand->flags & PCodeOperand_Use) != 0) {
            if (!SpillCode_IsLive(live, operand->value.reg)) {
                operand->flags |= PCodeOperand_LastUse;
            }
            SpillCode_SetLive(live, operand->value.reg);
        }
    }
}

static void SpillCode_MarkConstrainedRegister(short reg)
{
    if (reg >= 32) {
        SpillCode_SetMatrixBit(reg, reg);
    }
}

static void SpillCode_AddGPRConstraints(PCodeInstruction *instruction)
{
    if ((instruction->flags & PCodeInstruction_GPRResultMask) != 0) {
        SpillCode_MarkConstrainedRegister(instruction->operandData.operands[1].value.reg);
        if ((instruction->flags & PCodeInstruction_GPRPairInterference) != 0) {
            SpillCode_SetInterference(instruction->operandData.operands[0].value.reg,
                                      instruction->operandData.operands[1].value.reg);
        }
    } else if (instruction->opcode == PC_ADDI || instruction->opcode == PC_ADDIS) {
        SpillCode_MarkConstrainedRegister(instruction->operandData.operands[1].value.reg);
    } else if (instruction->opcode >= 0x37 && instruction->opcode <= 0x3b) {
        SpillCode_MarkConstrainedRegister(instruction->operandData.operands[0].value.reg);
    }

    if ((instruction->flags & PCodeInstruction_GPRFixedRange) != 0) {
        int index;

        for (index = 50; index < instruction->operand_count; index++) {
            int physical;
            short reg;

            reg = instruction->operandData.operands[index].value.reg;
            SpillCode_MarkConstrainedRegister(reg);
            for (physical = 3; physical <= 12; physical++) {
                SpillCode_SetInterference(reg, physical);
            }
        }
    }
}

static void SpillCode_CollectSuccessorLiveIn(PCodeBlock *block, UInt32 *live_out, int register_count)
{
    PCodeBlockLink *successor;

    successor = block->successors;
    if (successor == NULL) {
        return;
    }
    SpillCode_CopyBits(live_out, gPCodeBlockLiveness[successor->payload.block->index].livein, register_count);
    for (successor = successor->next; successor != NULL; successor = successor->next) {
        SpillCode_OrBits(live_out, gPCodeBlockLiveness[successor->payload.block->index].livein, register_count);
    }
}

static int SpillCode_UpdateLiveIn(PCodeBlockLiveness *liveness, int register_count)
{
    int changed;
    int word;

    changed = 0;
    for (word = 0; word < SpillCode_WordCount(register_count); word++) {
        unsigned int live_in;

        live_in = (~liveness->def[word] & liveness->live_out[word]) | liveness->use[word];
        if (live_in != liveness->live_in[word]) {
            liveness->live_in[word] = live_in;
            changed = 1;
        }
    }
    return changed;
}

static short SpillCode_CoalesceRoot(short reg)
{
    short parent;

    do {
        parent = gCoalescedRegisters[reg];
        if (parent == reg) {
            return reg;
        }
        reg = parent;
    } while (1);
}

static short SpillCode_CopyOpcode(int reg_class)
{
    if (reg_class == 0) {
        return 0x8b;
    }
    if (reg_class == 1) {
        return 0x9e;
    }
    return 0x18e;
}

static int SpillCode_CoalesceEligible(int reg_class, short reg)
{
    if (reg_class == 0) {
        return reg >= gGPRCoalesceFirst && reg <= gGPRCoalesceLast;
    }
    if (reg_class == 1) {
        return reg >= gFPRCoalesceFirst && reg <= gFPRCoalesceLast;
    }
    return reg >= gVRCoalesceFirst && reg <= gVRCoalesceLast;
}

static int SpillCode_CanCoalesce(int reg_class, short first, short second)
{
    if (SpillCode_Interferes(first, second)) {
        return 0;
    }
    if (first < 32 || second < 32) {
        return 1;
    }
    return SpillCode_CoalesceEligible(reg_class, first) && SpillCode_CoalesceEligible(reg_class, second);
}

static void SpillCode_MergeCoalesceRoots(short first, short second, int register_count)
{
    short root;
    short child;
    int reg;

    root = first < second ? first : second;
    child = first < second ? second : first;
    gCoalescedRegisters[child] = root;
    for (reg = 0; reg < register_count; reg++) {
        if (SpillCode_Interferes(child, reg)) {
            SpillCode_SetInterference(root, reg);
        }
    }
}

static void SpillCode_AddOperandCosts(PCodeInstruction *instruction, int reg_class, int block_weight,
                                      unsigned char flag, int multiplier)
{
    int index;

    for (index = 0; index < instruction->operand_count; index++) {
        PCodeOperand *operand;

        operand = &instruction->operandData.operands[index];
        if (operand->kind == reg_class && (operand->flags & flag) != 0) {
            gInterferenceGraph[operand->value.reg]->spill_cost += block_weight * multiplier;
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

    bits = (UInt32 *)CompilerTools_AllocatePoolMemory(((count + 31) >> 5) * sizeof(UInt32));
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

    gInterferenceGraph = CompilerTools_AllocatePoolMemory(count * sizeof(*gInterferenceGraph));
    neighbors = CompilerTools_AllocatePoolMemory(count * sizeof(*neighbors));
    for (register_index = 0; register_index < count; register_index++) {
        neighbor_count = 0;
        for (neighbor_index = 0; neighbor_index < count; neighbor_index++) {
            if (test_interference(register_index, neighbor_index)) {
                neighbors[neighbor_count] = neighbor_index;
                neighbor_count++;
            }
        }
        node = gInterferenceGraph[register_index] =
            CompilerTools_AllocatePoolMemory(sizeof(InterferenceNode) + (neighbor_count - 1) * sizeof(*neighbors));
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

static int Coalesce_Interferes(UInt32 r1, UInt32 r2)
{
    UInt32 bit;

    if (r1 < r2) {
        bit = (r2 * r2 >> 1) + r1;
        return (gInterferenceBits[bit >> 5] & (1 << bit)) != 0;
    }
    if (r1 > r2) {
        bit = (r1 * r1 >> 1) + r2;
        return (gInterferenceBits[bit >> 5] & (1 << bit)) != 0;
    }
    return 0;
}

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

static SInt16 Coalesce_Find(SInt16 r)
{
    while (gCoalescedRegisters[r] != r)
        r = gCoalescedRegisters[r];
    return r;
}

static void Coalesce_FindNext(PCodeOperand *sig)
{
    SInt16 r;

    r = sig->value.reg;
    if (gCoalescedRegisters[r] != r) {
        while (gCoalescedRegisters[r] != r)
            r = gCoalescedRegisters[r];
        sig->value.reg = r;
    }
}

static int Coalesce_InRange(SInt16 r, SInt32 regclass)
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

    gCoalescedRegisters = CompilerTools_AllocatePoolMemory(numRegs * sizeof(*gCoalescedRegisters));

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
            if (Coalesce_Interferes(destinationReg, sourceReg))
                continue;
            if (destinationReg < 32 || sourceReg < 32 ||
                (Coalesce_InRange(destinationReg, regclass) && Coalesce_InRange(sourceReg, regclass))) {
                lowerReg = (sourceReg < destinationReg) ? sourceReg : destinationReg;
                upperReg = (sourceReg > destinationReg) ? sourceReg : destinationReg;
                gCoalescedRegisters[upperReg] = lowerReg;
                for (i = 0; i < numRegs; i++)
                    if (Coalesce_Interferes(upperReg, i))
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

#define SC_Clear(n, bits) ((bits)[(SInt16)(n) >> 5] &= ~(1 << ((UInt32)(SInt16)(n) & 31)))
#define SC_Test(n, bits) ((bits)[(SInt16)(n) >> 5] & (1 << ((UInt32)(SInt16)(n) & 31)))
static void SC_Add(SInt16 n, UInt32 *bits)
{
    int v = n;
    bits[v >> 5] |= 1 << ((UInt32)v & 31);
}
static void SC_SetBit(UInt32 n)
{
    gInterferenceBits[n >> 5] |= 1 << (n & 0x1f);
}

static void SC_Interfere(unsigned int a, unsigned int b)
{
    if (a < b)
        SC_SetBit(b * b / 2 + a);
    else if (a > b)
        SC_SetBit(a * a / 2 + b);
}

void SpillCode_ConstructInterference(SInt32 registerClass, UInt32 registerCount)
{
    UInt32 *bits;
    PCodeBlock *block;
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    SInt16 reg;
    UInt32 i, j, k;
    gInterferenceBits =
        (UInt32 *)CompilerTools_AllocatePoolMemory(((registerCount * registerCount / 2 + 0x1f) >> 5) << 2);
    CRTTI_FillWords(gInterferenceBits, registerCount * registerCount / 2, 0);
    for (i = 0; i < 0x20; i++) {
        for (j = 0; j < 0x20; j++) {
            if (i != j)
                SC_Interfere(i, j);
        }
    }
    bits = (UInt32 *)CompilerTools_AllocatePoolMemory(((registerCount + 0x1f) >> 5) << 2);
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
                                SC_Interfere(reg, j);
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
                        SC_Interfere(0, instruction->operandData.operands[1].value.reg);
                    if (instruction->flags & 0x8000)
                        SC_Interfere(instruction->operandData.operands[0].value.reg,
                                     instruction->operandData.operands[1].value.reg);
                } else if (instruction->opcode == PC_ADDI || instruction->opcode == PC_ADDIS) {
                    if (instruction->operandData.operands[1].value.reg >= 0x20)
                        SC_Interfere(0, instruction->operandData.operands[1].value.reg);
                } else if (instruction->opcode >= 0x37 && instruction->opcode <= 0x3b) {
                    if (instruction->operandData.operands[0].value.reg >= 0x20)
                        SC_Interfere(0, instruction->operandData.operands[0].value.reg);
                }
            }
            if (registerClass == 0 && (instruction->flags & 0x20)) {
                PCodeOperand *extraOperand;
                for (k = 0x32, extraOperand = &instruction->operandData.operands[k]; k < instruction->operand_count;
                     k++, extraOperand++) {
                    SInt16 tailReg;
                    SC_Interfere(tailReg = extraOperand->value.reg, 0);
                    for (i = 3; i <= 0xc; i++)
                        SC_Interfere(tailReg, i);
                }
            }
        }
    }
}

void InterferenceGraph_SpillRegisters(int reg_class, int register_count)
{
    PCodeOperand *destination;
    InterferenceNode *node;
    PCodeInstruction *instruction;
    const PCodeOperand *source_operand;
    PCodeOperand *operand;
    int needs_spill;
    Type *type;
    int operand_count;
    Object *object;
    int operand_index;
    int original_count;
    unsigned int register_index;
    PCodeBlock *block;
    PCodeInstruction *next_instruction;

    spill_address_register = 0;
    for (register_index = 32; register_index < register_count; register_index++) {
        node = gInterferenceGraph[register_index];
        if ((node->flags & 4) == 0 && (node->flags & 1) != 0) {
            if (node->object == NULL) {
                if (reg_class == 0) {
                    type = (Type *)&stunsignedlong;
                } else if (reg_class == 1) {
                    type = (Type *)&stdouble;
                } else {
                    type = TYPE(&stvectorunsignedchar);
                }
                object = (Object *)CompilerTools_AllocatePool(sizeof(Object));
                memclrw(object, sizeof(Object));
                object->otype = OT_OBJECT;
                object->access = ACCESSPUBLIC;
                object->datatype = DLOCAL;
                object->type = type;
                object->name = CParser_GetUniqueName();
                object->u.var.info = CPrep_AllocateVarInfo();
                object->u.var.uid = 0;
                node->object = object;
            }
            if (node->object->datatype == DLOCAL && Registers_GetInfo(node->object)->in_param_area == 0) {
                StackFrameEABI_AllocateObjectSlot(node->object);
            }
            Registers_GetInfo(node->object)->reg = 0;
        }
    }
    block = gPCodeBlocks;
    while (block != NULL) {
        instruction = block->instructions;
        while (instruction != NULL) {
            needs_spill = 0;
            next_instruction = instruction->next;
            operand = instruction->operandData.operands;
            operand_count = instruction->operand_count;
            while (operand_count--) {
                if (operand->kind == reg_class) {
                    node = gInterferenceGraph[operand->value.reg];
                    if ((node->flags & 1) != 0)
                        needs_spill = 1;
                }
                operand++;
            }
            if (needs_spill != 0) {
                if (reg_class == 9 && spill_address_register == 0) {
                    spill_address_register = gUsedVirtualRegistersGPR;
                    gUsedVirtualRegistersGPR += 1;
                }
                if (instruction->opcode == PC_MR) {
                    SpillCode_RewriteSpilledRegisterMove(block, instruction);
                } else if (instruction->opcode == PC_FMR) {
                    SpillCode_ReplaceInstructionWithFPRSpillCode(block, instruction);
                } else if (instruction->opcode == PC_VMR) {
                    SpillCode_EmitOperandSpills(block, instruction);
                } else if ((instruction->flags & PCodeInstruction_GPRFixedRange) != 0) {
                    original_count = instruction->operand_count;
                    operand_index = 50;
                    destination = &instruction->operandData.operands[operand_index];
                    source_operand = destination;
                    while (operand_index < original_count) {
                        if (source_operand->kind == PCOp_GPR &&
                            (gInterferenceGraph[source_operand->value.reg]->flags & 1) != 0) {
                            instruction->operand_count -= 1;
                        } else {
                            *destination = *source_operand;
                            destination++;
                        }
                        operand_index++;
                        source_operand = source_operand + 1;
                    }
                } else if (reg_class == 0) {
                    SpillCode_InsertGPRSpillCode(block, instruction);
                } else if (reg_class == 1) {
                    SpillCode_RewriteSpilledFPRs(block, instruction);
                } else if (reg_class == 9) {
                    SpillCode_RewriteSpilledVR(block, instruction);
                }
            }
            instruction = next_instruction;
        }
        block = block->next;
    }
}
