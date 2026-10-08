#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CopyPropagation.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BitVectors.h"
#include "compiler/CPrec.h"
#include "compiler/CRTTI.h"
#include "compiler/CodeMotion.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/LoopDetection.h"
#include "compiler/PCode.h"

struct CopyPropagationBitSets *copyPropagationBitSets;
UInt32 *block_copy_counts;
SInt32 copyCount;
struct CodeMotionRec *code_motion_records;
SInt32 *blockCopyStartIndices;
short gInitialObjectGPRLast;

static SInt32 copy_propagation_mode;

static inline SInt32 CanPropagateCopy(SInt32 copyIndex, CodeMotionListNode *useNode)
{
    for (; useNode != NULL; useNode = useNode->next) {
        if (can_propagate_copy_to_use(copyIndex, useNode->index) == 0)
            return 0;
    }
    return 1;
}

/* Whether END comes at or after INSTR in its block. */
static int follows(PCodeInstruction *instr, PCodeInstruction *end)
{
    while (instr) {
        if (instr == end)
            return 1;
        instr = instr->next;
    }
    return 0;
}

static inline UInt32 *genptr(struct CopyPropagationBitSets *bv)
{
    return bv->gen;
}

void CopyPropagation_CountBlockCopies(void)
{
    PCodeInstruction *instruction;
    PCodeBlock *block;
    int count;
    block_copy_counts = (UInt32 *)oalloc(gPCodeBlockCount * sizeof(*block_copy_counts));
    blockCopyStartIndices = (SInt32 *)oalloc(gPCodeBlockCount * sizeof(*blockCopyStartIndices));
    copyCount = 0;
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        blockCopyStartIndices[block->index] = copyCount;
        count = 0;
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if (((instruction->opcode == PC_MR) || (instruction->opcode == PC_FMR) ||
                 (instruction->opcode == PC_VMR)) &&
                (0x20 <= instruction->operandData.operands[0].value.reg)) {
                ++copyCount;
                ++count;
            }
        }
        block_copy_counts[block->index] = count;
    }
}

void CopyPropagation_BuildCodeMotionRecords(void)
{
    int reg;
    struct PCodeBlock *block;
    PCodeInstruction *instruction;
    CodeMotionRec *record;
    UInt32 *bits;
    CodeMotionEntry *entry;
    CodeMotionEntryLink *link;
    SInt32 useIndex;
    SInt32 definitionIndex;

    code_motion_records = oalloc(copyCount * sizeof(CodeMotionRec));
    bits = (UInt32 *)oalloc(((data_00587e38 + 31) >> 5) * sizeof(UInt32));
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (block_copy_counts[block->index] == 0)
            continue;
        CodeMotion_AllocateBits(bits, data_00587fe4[block->index].use_sets[3], data_00587e38);
        record = code_motion_records + (blockCopyStartIndices[block->index] + block_copy_counts[block->index] - 1);
        for (instruction = block->reverse_instructions; instruction != NULL; instruction = instruction->previous) {
            if ((instruction->flags & fIsBranch) != 0)
                continue;
            if (instruction->operand_count == 0)
                continue;
            if ((instruction->flags & fIsMove) != 0) {
                reg = instruction->operandData.operands[0].value.reg;
                if (reg >= 0x20) {
                    record->node = instruction;
                    record->list = NULL;
                    if (instruction->opcode == PC_MR)
                        link = code_motion_register_use_heads[reg];
                    else if (instruction->opcode == PC_FMR)
                        link = codeMotionUseEntryHeads[reg];
                    else
                        link = register_use_entry_heads[reg];
                    for (; link != NULL; link = link->next) {
                        if ((1 << (link->entry_index & 31)) & bits[link->entry_index >> 5]) {
                            CodeMotionListNode *listNode = (CodeMotionListNode *)oalloc(sizeof(CodeMotionListNode));
                            listNode->index = link->entry_index;
                            listNode->next = record->list;
                            record->list = listNode;
                        }
                    }
                    record--;
                }
            }
            for (entry = &code_motion_entries[useIndex = instruction->useStart];
                 useIndex < codeMotionEntryCount && entry->instruction == instruction; entry++, useIndex++) {
                if (entry->kind == 0) {
                    for (link = code_motion_register_use_heads[entry->value.reg]; link != NULL; link = link->next)
                        bits[link->entry_index >> 5] &= ~(1 << (link->entry_index & 31));
                } else if (entry->kind == 1) {
                    for (link = codeMotionUseEntryHeads[entry->value.reg]; link != NULL; link = link->next)
                        bits[link->entry_index >> 5] &= ~(1 << (link->entry_index & 31));
                } else if (entry->kind == 9) {
                    for (link = register_use_entry_heads[entry->value.reg]; link != NULL; link = link->next)
                        bits[link->entry_index >> 5] &= ~(1 << (link->entry_index & 31));
                }
            }
            for (entry = &cm_entries[definitionIndex = instruction->definitionStart];
                 definitionIndex < data_00587e38 && entry->instruction == instruction; entry++, definitionIndex++) {
                if (entry->kind == 0 || entry->kind == 1 || entry->kind == 9)
                    bits[definitionIndex >> 5] |= 1 << (definitionIndex & 31);
            }
        }
    }
}

void CopyPropagation_ComputeGenKill(void)
{
    PCodeBlock *block = gPCodeBlocks;
    while (block != NULL) {
        struct CopyPropagationBitSets *sets;
        UInt32 *localBits;
        UInt32 *otherBits;
        PCodeInstruction *instruction;
        UInt32 remainingOperands;
        SInt32 bitIndex;
        sets = copyPropagationBitSets + block->index;
        localBits = sets->gen;
        otherBits = sets->kill;
        CRTTI_FillWords(localBits, copyCount, 0);
        CRTTI_FillWords(otherBits, copyCount, 0);
        bitIndex = blockCopyStartIndices[block->index];
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if ((instruction->flags & PCodeInstruction_SkipCodeMotion) == 0 && instruction->operand_count != 0) {
                PCodeOperand *operand;
                CodeMotionRec *entry;
                SInt32 i;
                operand = instruction->operandData.operands;
                remainingOperands = instruction->operand_count;
                while (remainingOperands--) {
                    if ((operand->kind == PCOp_GPR || operand->kind == PCOp_FPR || operand->kind == PCOp_VR) &&
                        ((SInt8)operand->flags & PCodeOperand_Definition)) {
                        entry = code_motion_records;
                        for (i = 0; i < copyCount; i++) {
                            if (entry->node->operandData.operands[0].kind == operand->kind &&
                                (entry->node->operandData.operands[0].value.reg == operand->value.reg ||
                                 entry->node->operandData.operands[1].value.reg == operand->value.reg)) {
                                if (entry->node->block == block) {
                                    localBits[i >> 5] &= ~(1 << (i & 31));
                                } else {
                                    otherBits[i >> 5] |= 1 << i;
                                }
                            }
                            entry++;
                        }
                    }
                    operand++;
                }
                if ((instruction->flags & PCodeInstruction_CopySourceExclusion) &&
                    instruction->operandData.operands[0].value.reg >= 0x20) {
                    localBits[bitIndex >> 5] |= 1 << bitIndex;
                    bitIndex++;
                }
            }
        }
        block = block->next;
    }
}

void CopyPropagation_ComputeInOutSets(void)
{
    SInt32 wordCount;
    PCodeBlock *block;
    struct CopyPropagationBitSets *sets;
    UInt32 *out;
    SInt32 blockIndex;
    SInt32 changed;
    SInt32 value;
    PCodeBlockLink *predecessor;
    SInt32 wordIndex;
    UInt32 *gen;
    UInt32 *in;
    UInt32 *kill;
    UInt32 *predecessorOut;

    wordCount = (copyCount + 31) >> 5;
    block = gPCodeBlocks;
    {
        struct CopyPropagationBitSets *entrySets = &copyPropagationBitSets[block->index];
        CRTTI_FillWords(entrySets->out, wordIndex = copyCount, 0);
        CodeMotion_AllocateBits(entrySets->in, entrySets->gen, copyCount);
    }

    for (block = gPCodeBlocks->next; block != NULL; block = block->next) {
        UInt32 *initialIn;
        SInt32 initialWord;
        sets = &copyPropagationBitSets[block->index];
        initialIn = sets->in;
        kill = sets->kill;
        for (initialWord = 0; initialWord < wordCount; initialWord++)
            *initialIn++ = ~*kill++;
    }

    do {
        changed = 0;
        blockIndex = 0;
        if (gPCodeBlockCount > 0)
            do {
                if ((block = gPCodeBlockOrder[blockIndex]) == NULL)
                    continue;
                sets = &copyPropagationBitSets[block->index];
                if ((predecessor = block->predecessors) != NULL) {
                    predecessorOut = sets->out;
                    CodeMotion_AllocateBits(predecessorOut,
                                            copyPropagationBitSets[predecessor->payload.block->index].in, copyCount);
                    for (predecessor = predecessor->next; predecessor != NULL; predecessor = predecessor->next)
                        CRTTI_IntersectBitVectors(
                            predecessorOut, copyPropagationBitSets[predecessor->payload.block->index].in, copyCount);
                }
                {
                    wordIndex = 0;
                    in = sets->in;
                    out = sets->out;
                    gen = sets->gen;
                    kill = sets->kill;
                    for (; wordIndex < wordCount; wordIndex++, out++, in++, kill++, gen++) {
                        value = (~*kill & *out) | *gen;
                        if (value != *in) {
                            *in = value;
                            changed = 1;
                        }
                    }
                }
            } while (++blockIndex < gPCodeBlockCount);
    } while (changed);
}

/* Whether the copy DEF still holds at the use USE: no instruction between them redefines the copy's source register. */
int can_propagate_copy_to_use(int def, int use)
{
    PCodeInstruction *copy;
    PCodeInstruction *end;
    PCodeInstruction *p;
    PCodeOperand *op;
    unsigned int n;
    int kind, dst, reg;
    CodeMotionEntry *entry;
    PCodeInstruction *stop;

    copy = code_motion_records[def].node;
    entry = &cm_entries[use];
    kind = copy->operandData.operands[0].kind;
    dst = copy->operandData.operands[0].value.reg;
    reg = copy->operandData.operands[1].value.reg;
    if (entry->instruction->flags & fIsMove)
        return 0;
    if ((end = entry->instruction)->opcode == PC_RLWIMI && end->operandData.operands[0].value.reg == dst)
        return 0;
    if (copy->block == end->block && follows(p = copy->next, stop = end)) {
        while (p && p != end) {
            unsigned int n;
            PCodeOperand *op;

            for (op = p->operandData.operands, n = p->operand_count; n--; op++) {
                if (op->kind == kind && (op->flags & 2) && op->value.reg == reg)
                    return 0;
            }
            p = p->next;
        }
    } else {
        if (!(copyPropagationBitSets[end->block->index].out[def >> 5] & (1UL << (def & 31))))
            return 0;
        for (p = end->block->instructions; p; p = p->next) {
            for (op = p->operandData.operands, n = p->operand_count; n--; op++) {
                if (op->kind == kind && (op->flags & 2) && op->value.reg == reg)
                    return 0;
            }
            if (p == end)
                break;
        }
    }
    return 1;
}

void CopyPropagation_ReplaceRegisterUses(int index)
{
    int kind;
    int sourceRegister;
    int replacementRegister;
    CodeMotionRec *entry;
    int count;
    PCodeOperand *operand;
    CodeMotionListNode *use;

    entry = &code_motion_records[index];
    kind = entry->node->operandData.operands[0].kind;
    replacementRegister = entry->node->operandData.operands[1].value.reg;
    sourceRegister = entry->node->operandData.operands[0].value.reg;
    if (sourceRegister >= 32 && sourceRegister <= gInitialObjectGPRLast) {
        return;
    }
    if (copy_propagation_mode == 0 && replacementRegister < 32) {
        return;
    }
    use = entry->list;
    while (use != NULL) {
        operand = cm_entries[use->index].instruction->operandData.operands;
        count = cm_entries[use->index].instruction->operand_count;
        while (count--) {
            if (operand->kind == kind && operand->value.reg == sourceRegister && (operand->flags & 1) != 0) {
                operand->value.reg = replacementRegister;
            }
            operand++;
        }
        use = use->next;
    }
    PCode_UnlinkInstruction(entry->node);
    gCopyPropagationChanged = 1;
}

void COpt_CopyPropagation(SInt32 mode)
{
    int blockIndex;
    struct CopyPropagationBitSets *block;
    SInt32 copyIndex;
    SInt32 propagate;

    gCopyPropagationChanged = 0;
    copy_propagation_mode = mode;
    CopyPropagation_CountBlockCopies();
    if (copyCount > 0) {
        COpt_SetLoopCodeMotionMode(0);
        CopyPropagation_BuildCodeMotionRecords();
        copyPropagationBitSets = oalloc(gPCodeBlockCount * sizeof *block);
        block = copyPropagationBitSets;
        for (blockIndex = 0; blockIndex < gPCodeBlockCount; blockIndex++) {
            block->gen = oalloc(((copyCount + 31) >> 5) * sizeof *block->gen);
            block->kill = oalloc(((copyCount + 31) >> 5) * sizeof *block->kill);
            block->out = oalloc(((copyCount + 31) >> 5) * sizeof *block->out);
            block->in = oalloc(((copyCount + 31) >> 5) * sizeof *block->in);
            block++;
        }
        CopyPropagation_ComputeGenKill();
        SpillCode_BuildBlockOrder();
        CopyPropagation_ComputeInOutSets();
        for (copyIndex = 0; copyIndex < copyCount; copyIndex++) {
            if (code_motion_records[copyIndex].node->flags & PCodeInstruction_CoalesceDisabled) {
                propagate = 0;
            } else {
                propagate = CanPropagateCopy(copyIndex, code_motion_records[copyIndex].list);
            }
            if (propagate)
                CopyPropagation_ReplaceRegisterUses(copyIndex);
        }
    }
    freeoheap();
}
