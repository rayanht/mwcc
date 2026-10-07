#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/PCode.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Switch.h"

#pragma options align = mac68k
static short next_label_number;
static SInt32 pcodeBlockOrderIndex;
#pragma options align = reset

enum {
    OperandKind_GPR = 0,
    OperandKind_Address = 1,
    OperandKind_GPRSum = 2,
    OperandKind_GPRPair = 3,
    OperandKind_Immediate = 4,
    OperandKind_FPR = 5,
    OperandKind_Condition = 7,
    OperandKind_DirectMemory = 9,
    OperandKind_IndexedMemory = 10,
    PCode_LBZ = 0x15,
    PCode_LBZX = 0x17,
    PCode_LHZ = 0x19,
    PCode_LHZX = 0x1b,
    PCode_LHA = 0x1d,
    PCode_LHAX = 0x1f,
    PCode_LWZ = 0x22,
    PCode_LWZX = 0x24,
    PCode_ADD = 0x3c,
    PCode_ADDI = 0x3f,
    PCode_XORI = 0x5a,
    PCode_RLWINM = 0x67,
    PCode_MFCR = 0x82,
    PCode_LI = 0x89,
    PCode_LIS = 0x8a,
    PCode_LFS = 0x8e,
    PCode_LFSX = 0x90,
    PCode_LFD = 0x92,
    PCode_LFDX = 0x94
};

void PCode_ResetBlocks(void)
{
    unsigned short *statusValue = (unsigned short *)&gPCodeBlockCount;

    gPCodeBlocks = gCurrentBlock = NULL;
    gPCodeBlockCount = 0;
    next_label_number = *statusValue;
    data_00587ffc = 1;
}

PCodeInstruction *PCode_CloneInstruction(PCodeInstruction *instr)
{
    PCodeInstruction *clone;
    int i;
    if ((((PCodeInstruction *)(instr))->flags & 0x200) && !(((PCodeInstruction *)(instr))->flags & fRecordBit))
        clone = (PCodeInstruction *)CompilerTools_AllocatePool(
            (((PCodeInstruction *)(instr))->operand_count + 1) * sizeof(PCodeOperand) + sizeof(PCodeInstruction));
    else
        clone = (PCodeInstruction *)CompilerTools_AllocatePool(
            ((PCodeInstruction *)(instr))->operand_count * sizeof(PCodeOperand) + sizeof(PCodeInstruction));
    clone->opcode = ((PCodeInstruction *)(instr))->opcode;
    clone->flags = ((PCodeInstruction *)(instr))->flags;
    clone->operand_count = ((PCodeInstruction *)(instr))->operand_count;
    for (i = 0; i < ((PCodeInstruction *)(instr))->operand_count; i++)
        clone->operandData.operands[i] = ((PCodeInstruction *)(instr))->operandData.operands[i];
    return (PCodeInstruction *)(clone);
}

PCodeLabel *PCode_NewLabel(void)
{
    PCodeLabel *node;

    node = (PCodeLabel *)CompilerTools_AllocatePool(12);
    node->next = NULL;
    node->target.pendingLinks = NULL;
    node->resolved = 0;
    node->number = next_label_number;
    ++next_label_number;
    return node;
}

PCodeBlock *PCode_CreateBlock(void)
{
    PCodeBlock *block;

    block = (PCodeBlock *)CompilerTools_AllocatePool(sizeof(*block));
    block->next = NULL;
    block->prev = gCurrentBlock;
    block->labels = NULL;
    block->successors = NULL;
    block->predecessors = block->successors;
    block->reverse_instructions = NULL;
    block->instructions = block->reverse_instructions;
    block->line = -1;
    block->instruction_count = 0;
    block->execution_weight = data_00587ffc;
    block->flags = 0;
    block->index = gPCodeBlockCount;
    gPCodeBlockCount += 1U;
    if (gCurrentBlock != NULL) {
        gCurrentBlock->next = block;
    } else {
        gPCodeBlocks = block;
    }
    gCurrentBlock = block;
    return block;
}

void PCode_ResolveLabel(PCodeBlock *target, PCodeLabel *entry)
{
    PCodeBlockLink *current;
    PCodeLabel *previous;
    PCodeBlockLink *next;
    current = entry->target.pendingLinks; /* unresolved label: pending successor links */
    while (current) {
        next = current->payload.pendingLinks;
        current->payload.block = target;
        current = next;
    }
    entry->target.block = target;
    entry->resolved = 1;
    previous = target->labels;
    entry->next = previous;
    target->labels = entry;
}

/* Links a saved name ID into the block's list. */
void PCode_AddSuccessor(PCodeBlock *block, PCodeLabel *name)
{
    PCodeBlockLink *entry;
    entry = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    entry->payload = name->target;
    if (name->resolved == 0)
        name->target.pendingLinks = entry;
    entry->next = (PCodeBlockLink *)block->successors;
    block->successors = (PCodeBlockLink *)entry;
}

/* Builds every block's predecessor list from the successor lists. */
void PCode_BuildPredecessors(void)
{
    PCodeBlock *p;
    PCodeBlockLink *q;
    PCodeBlockLink *n;

    p = gPCodeBlocks;
    if (p != NULL) {
        do {
            q = p->successors;
            if (q != NULL) {
                do {
                    n = (PCodeBlockLink *)CompilerTools_AllocatePool(8);
                    n->payload.block = p;
                    n->next = q->payload.block->predecessors;
                    q->payload.block->predecessors = n;
                    q = q->next;
                } while (q != NULL);
            }
            p = p->next;
        } while (p != NULL);
    }
}

void PCode_UnlinkBlocksWithoutFlag4(void)
{
    PCodeBlock *p;

    SpillCode_BuildBlockOrder();
    p = gPCodeBlocks->next;
    while (p != NULL) {
        if ((p->flags & 4) == 0) {
            p->prev->next = p->next;
            if (p->next != NULL)
                p->next->prev = p->prev;
            p->flags |= 0x10;
        }
        p = p->next;
    }
}

PCodeInstruction *PCode_AppendInstruction(PCodeBlock *block, PCodeInstruction *instruction)
{
    PCodeInstruction *tail;
    if (block->instructions) {
        instruction->next = NULL;
        tail = block->reverse_instructions;
        instruction->previous = tail;
        tail = block->reverse_instructions;
        tail->next = instruction;
        block->reverse_instructions = instruction;
    } else {
        block->reverse_instructions = instruction;
        tail = block->reverse_instructions;
        block->instructions = tail;
        instruction->previous = NULL;
        tail = instruction->previous;
        instruction->next = tail;
    }
    instruction->block = block;
    block->instruction_count += 1U;
    return tail;
}

void PCode_UnlinkInstruction(PCodeInstruction *instruction)
{
    PCodeBlock *owner;

    owner = instruction->block;
    if (instruction->previous != NULL) {
        instruction->previous->next = instruction->next;
    } else {
        owner->instructions = instruction->next;
    }
    if (instruction->next != NULL) {
        instruction->next->previous = instruction->previous;
    } else {
        owner->reverse_instructions = instruction->previous;
    }
    instruction->block = NULL;
    owner->instruction_count--;
    owner->flags &= 0xfff7;
}

void PCode_InsertInstructionBefore(PCodeInstruction *h, PCodeInstruction *n)
{
    PCodeBlock *o = h->block;
    if (h->previous != NULL)
        h->previous->next = n;
    else
        o->instructions = n;
    n->next = h;
    n->previous = h->previous;
    h->previous = n;
    n->block = o;
    o->instruction_count++;
    o->flags &= ~8;
}

void PCode_InsertInstructionAfter(PCodeInstruction *h, PCodeInstruction *n)
{
    PCodeBlock *c = h->block;
    if (h->next != NULL)
        h->next->previous = n;
    else
        c->reverse_instructions = n;
    n->previous = h;
    n->next = h->next;
    h->next = n;
    n->block = c;
    c->instruction_count++;
    c->flags &= 0xfff7;
}

void Operands_AllocateGPR(unsigned int flags)
{
    gCurrentBlock->reverse_instructions->flags |= flags;
}

unsigned int PCode_SetCodeOffsets(void)
{
    unsigned int total = 0;
    PCodeBlock *record = gPCodeBlocks;
    while (record != NULL) {
        record->code_offset = total;
        total += record->instruction_count * 4;
        record = record->next;
    }
    return total;
}

void SpillCode_BuildBlockOrder(void)
{
    PCodeBlock *block;
    PCodeBlock *successor;
    int depth;
    BlockOrderEntry *stack;
    PCodeBlockLink *link;
    PCodeBlock *current;

    gPCodeBlockOrder = CompilerTools_AllocatePool(gPCodeBlockCount * sizeof(*gPCodeBlockOrder));
    pcodeBlockOrderIndex = gPCodeBlockCount;
    for (current = gPCodeBlocks; current != NULL; current = current->next) {
        current->flags &= ~4;
    }
    stack = (struct BlockOrderEntry *)CompilerTools_AllocatePoolMemory(gPCodeBlockCount * sizeof(*stack));
    gPCodeBlocks->flags |= 4;
    depth = 0;
    stack[0].block = gPCodeBlocks;
    stack[0].cursor = gPCodeBlocks->successors;
    depth++;
    while (depth != 0) {
        if ((link = stack[depth - 1].cursor) != NULL) {
            stack[depth - 1].cursor = link->next;
            successor = link->payload.block;
            if (!((block = successor)->flags & 4)) {
                block->flags |= 4;
                stack[depth].block = block;
                stack[depth].cursor = block->successors;
                depth++;
            }
        } else {
            block = stack[--depth].block;
            gPCodeBlockOrder[--pcodeBlockOrderIndex] = block;
        }
    }
    while (pcodeBlockOrderIndex != 0) {
        pcodeBlockOrderIndex--;
        gPCodeBlockOrder[pcodeBlockOrderIndex] = NULL;
    }
}
