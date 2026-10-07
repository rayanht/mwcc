#define CERROR_FILE "AddPropagation.c"
#include "compiler/common.h"
#include "compiler/AddPropagation.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/BitVectors.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CRTTI.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Registers.h"
#include "compiler/StrengthReduction.h"
#include "compiler/Switch.h"
static int COpt_AllLinked(SInt32 j, CodeMotionEntryLink *p)
{
    for (; p != NULL; p = p->next)
        if (!can_propagate_add(j, p->entry_index))
            return 0;
    return 1;
}

static int Contains(PCodeInstruction *list, PCodeInstruction *use)
{
    while (list != NULL) {
        if (list == use)
            return 1;
        list = list->next;
    }
    return 0;
}

void fn_00521950(void)
{
    PCodeBlock *b;
    PCodeInstruction *inst;
    int n;

    add_propagation_entry_counts = (int *)CompilerTools_AllocatePoolMemory(gPCodeBlockCount << 2);
    add_propagation_block_bit_indices = (int *)CompilerTools_AllocatePoolMemory(gPCodeBlockCount << 2);
    add_propagation_entry_count = 0;
    for (b = gPCodeBlocks; b != (PCodeBlock *)0x0; b = b->next) {
        add_propagation_block_bit_indices[b->index] = add_propagation_entry_count;
        n = 0;
        for (inst = b->instructions; inst != (PCodeInstruction *)0x0; inst = inst->next) {
            if (inst->opcode == PC_ADD) {
                if (inst->operandData.operands[0].value.reg >= 0x20) {
                    add_propagation_entry_count++;
                    n++;
                }
            } else if (inst->opcode == PC_ADDI && inst->operandData.operands[0].value.reg >= 0x20 &&
                       (inst->operandData.operands[2].kind == PCOp_IMMEDIATE ||
                        (inst->operandData.operands[2].kind == PCOp_MEMORY &&
                         inst->operandData.operands[2].flags == 1))) {
                add_propagation_entry_count++;
                n++;
            }
        }
        add_propagation_entry_counts[b->index] = n;
    }
}

void build_add_propagation_entries(void)
{
    struct AddPropagationEntry *out;
    UInt32 *bits;
    struct PCodeBlock *block;
    struct PCodeInstruction *node;
    SInt32 i;
    SInt32 j;
    struct CodeMotionEntryLink *u;
    CodeMotionEntryLink *e;
    struct CodeMotionEntry *d;
    struct CodeMotionEntry *us;

    add_propagation_entries =
        (struct AddPropagationEntry *)CompilerTools_AllocatePoolMemory(add_propagation_entry_count << 3);
    bits = (UInt32 *)CompilerTools_AllocatePoolMemory(((data_00587e38 + 0x1f) >> 5) << 2);
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (add_propagation_entry_counts[block->index] != 0) {
            CodeMotion_AllocateBits(bits, data_00587fe4[block->index].use_sets[3], data_00587e38);
            out = add_propagation_entries +
                  (add_propagation_block_bit_indices[block->index] + add_propagation_entry_counts[block->index] - 1);
            for (node = block->reverse_instructions; node != NULL; node = node->previous) {
                if ((node->flags & fIsBranch) != 0)
                    continue;
                if (node->operand_count == 0)
                    continue;
                if (node->opcode == PC_ADD) {
                    int reg = node->operandData.operands[0].value.reg;
                    if (reg >= 0x20) {
                        out->instruction = node;
                        out->list = NULL;
                        for (u = code_motion_register_use_heads[reg]; u != NULL; u = u->next) {
                            if (bits[u->entry_index >> 5] & (1 << (u->entry_index & 0x1f))) {
                                e = (CodeMotionEntryLink *)CompilerTools_AllocatePoolMemory(8);
                                e->entry_index = u->entry_index;
                                e->next = out->list;
                                out->list = e;
                            }
                        }
                        out--;
                    }
                } else if (node->opcode == PC_ADDI) {
                    int reg = node->operandData.operands[0].value.reg;
                    if (reg >= 0x20 && (node->operandData.operands[2].kind == PCOp_IMMEDIATE ||
                                        (node->operandData.operands[2].kind == PCOp_MEMORY &&
                                         node->operandData.operands[2].flags == 1))) {
                        out->instruction = node;
                        out->list = NULL;
                        for (u = code_motion_register_use_heads[reg]; u != NULL; u = u->next) {
                            if (bits[u->entry_index >> 5] & (1 << (u->entry_index & 0x1f))) {
                                e = (CodeMotionEntryLink *)CompilerTools_AllocatePoolMemory(8);
                                e->entry_index = u->entry_index;
                                e->next = out->list;
                                out->list = e;
                            }
                        }
                        out--;
                    }
                }
                d = code_motion_entries + (i = node->useStart);
                while (i < codeMotionEntryCount && d->instruction == node) {
                    if (d->kind == 0) {
                        for (u = code_motion_register_use_heads[d->value.reg]; u != NULL; u = u->next)
                            bits[u->entry_index >> 5] &= ~(1 << (u->entry_index & 0x1f));
                    }
                    d++;
                    i++;
                }
                us = cm_entries + (j = node->definitionStart);
                while (j < data_00587e38 && us->instruction == node) {
                    if (us->kind == 0 || us->kind == 1 || us->kind == 9)
                        bits[j >> 5] |= 1 << (j & 0x1f);
                    us++;
                    j++;
                }
            }
        }
    }
}

void fn_00521480(void)
{
    UInt32 *local;
    UInt32 *external;
    PCodeBlock *block = gPCodeBlocks;
    SInt32 remaining;
    PCodeInstruction *instruction;
    SInt32 bitIndex;

    while (block != NULL) {
        struct CodeBits *vectors = &addPropagationBlockBits[block->index];

        local = vectors->gen;
        external = vectors->kill;
        CRTTI_FillWords(local, add_propagation_entry_count, 0);
        CRTTI_FillWords(external, add_propagation_entry_count, 0);
        bitIndex = add_propagation_block_bit_indices[block->index];
        for (instruction = (PCodeInstruction *)block->instructions; instruction != NULL;
             instruction = instruction->next) {
            if ((instruction->flags & fIsBranch) == 0 && instruction->operand_count != 0) {
                PCodeOperand *operand = instruction->operandData.operands;
                remaining = instruction->operand_count;
                while (remaining--) {
                    if (operand->kind == PCOp_GPR && (operand->flags & PCodeOperand_Definition) != 0) {
                        struct AddPropagationEntry *entry = add_propagation_entries;
                        SInt32 index;
                        for (index = 0; index < add_propagation_entry_count; index++) {
                            if ((char)entry->instruction->operandData.operands[0].kind == (char)operand->kind &&
                                (entry->instruction->operandData.operands[0].value.reg == operand->value.reg ||
                                 entry->instruction->operandData.operands[1].value.reg == operand->value.reg ||
                                 (entry->instruction->opcode != PC_ADDI &&
                                  entry->instruction->operandData.operands[2].value.reg == operand->value.reg))) {
                                if (entry->instruction->block == block)
                                    local[index >> 5] &= ~(1 << (index & 0x1f));
                                else
                                    external[index >> 5] |= 1 << (index & 0x1f);
                            }
                            entry++;
                        }
                    }
                    operand++;
                }
                if (instruction->opcode == PC_ADD) {
                    if (instruction->operandData.operands[0].value.reg >= 0x20) {
                        local[bitIndex >> 5] |= 1 << (bitIndex & 0x1f);
                        bitIndex++;
                    }
                } else if (instruction->opcode == PC_ADDI) {
                    if (instruction->operandData.operands[0].value.reg >= 0x20 &&
                        (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE ||
                         (instruction->operandData.operands[2].kind == PCOp_MEMORY &&
                          instruction->operandData.operands[2].flags == 1))) {
                        local[bitIndex >> 5] |= 1 << (bitIndex & 0x1f);
                        bitIndex++;
                    }
                }
            }
        }
        block = block->next;
    }
}

void compute_add_propagation_in_out(void)
{
    UInt32 t;
    UInt32 *gen;
    PCodeBlock *b;
    UInt32 *src;
    UInt32 *dst;
    SInt32 words;
    int blkidx;
    SInt32 changed;
    CodeBits *rec;
    PCodeBlockLink *e;
    UInt32 *out;
    UInt32 *kill;
    UInt32 *in;
    SInt32 j;
    SInt32 i;

    words = (add_propagation_entry_count + 0x1f) >> 5;

    rec = &addPropagationBlockBits[gPCodeBlocks->index];
    CRTTI_FillWords(rec->in, add_propagation_entry_count, 0);
    CodeMotion_AllocateBits(rec->out, rec->gen, add_propagation_entry_count);

    for (b = gPCodeBlocks->next; b != NULL; b = b->next) {
        rec = &addPropagationBlockBits[b->index];
        dst = rec->out;
        src = rec->kill;
        for (j = 0; j < words; j++)
            *dst++ = ~*src++;
    }

    do {
        changed = 0;
        for (blkidx = 0; blkidx < gPCodeBlockCount; blkidx++) {
            if ((b = gPCodeBlockOrder[blkidx]) != NULL) {
                rec = &addPropagationBlockBits[b->index];
                if ((e = b->predecessors) != NULL) {
                    in = rec->in;
                    CodeMotion_AllocateBits(in, addPropagationBlockBits[e->payload.block->index].out,
                                            add_propagation_entry_count);
                    for (e = e->next; e != NULL; e = e->next)
                        CRTTI_IntersectBitVectors(in, addPropagationBlockBits[e->payload.block->index].out,
                                                  add_propagation_entry_count);
                }
                out = rec->out;
                in = rec->in;
                gen = rec->gen;
                kill = rec->kill;
                for (j = 0; j < words; j++) {
                    t = (~*kill & *in) | *gen;
                    if (t != *out) {
                        *out = t;
                        changed = 1;
                    }
                    in++;
                    out++;
                    kill++;
                    gen++;
                }
            }
        }
    } while (changed);
}

int can_propagate_add(int recordIndex, int useIndex)
{
    PCodeInstruction *record = add_propagation_entries[recordIndex].instruction;
    int operandType;
    int sourceValue;
    int resultValue;
    PCodeInstruction *node;
    PCodeInstruction *useNode;
    CodeMotionEntry *useEntry;
    int secondSourceValue;
    PCodeOperand *entry;

    useEntry = &cm_entries[useIndex];

    operandType = record->operandData.operands[0].kind;
    resultValue = record->operandData.operands[0].value.reg;
    sourceValue = record->operandData.operands[1].value.reg;
    if (cm_entries[useIndex].instruction->block == NULL) {
        addPropagationChanged = 1;
        return 0;
    }
    if ((useEntry->instruction->flags & (fIsRead | fIsWrite)) == 0 && useEntry->instruction->opcode != PC_ADDI)
        return 0;
    if (useEntry->instruction->operandData.operands[2].kind != PCOp_IMMEDIATE ||
        (useEntry->instruction->flags & 0x8000))
        return 0;
    if (record->opcode == PC_ADD) {
        if (useEntry->instruction->operandData.operands[2].value.signed_value != 0)
            return 0;
        secondSourceValue = record->operandData.operands[2].value.reg;
    } else if (record->operandData.operands[2].kind == PCOp_IMMEDIATE) {
        if (record->operandData.operands[2].value.signed_value +
                useEntry->instruction->operandData.operands[2].value.signed_value !=
            (SInt16)(record->operandData.operands[2].value.signed_value +
                     useEntry->instruction->operandData.operands[2].value.signed_value))
            return 0;
    } else {
        Object *referencedNode = record->operandData.operands[2].object;
        if (referencedNode->u.var.uid + record->operandData.operands[2].value.signed_value +
                useEntry->instruction->operandData.operands[2].value.signed_value !=
            (SInt16)(referencedNode->u.var.uid + record->operandData.operands[2].value.signed_value +
                     useEntry->instruction->operandData.operands[2].value.signed_value))
            return 0;
    }
    if ((useEntry->instruction->flags & fIsWrite) &&
        useEntry->instruction->operandData.operands[0].value.reg == resultValue)
        return 0;
    if (useEntry->instruction->operandData.operands[1].value.reg != resultValue)
        return 0;
    if (record->block == useEntry->instruction->block && Contains(record->next, useEntry->instruction)) {
        int remaining;
        if (record->opcode == PC_ADD) {
            for (node = record->next; node != NULL && node != useEntry->instruction; node = node->next) {
                entry = node->operandData.operands;
                remaining = node->operand_count;
                while (remaining--) {
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == sourceValue)
                        return 0;
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == secondSourceValue)
                        return 0;
                    entry++;
                }
            }
        } else {
            for (node = record->next; node != NULL && node != useEntry->instruction; node = node->next) {
                entry = node->operandData.operands;
                remaining = node->operand_count;
                while (remaining--) {
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == sourceValue)
                        return 0;
                    entry++;
                }
            }
        }
    } else {
        int remaining;
        if ((1 << (recordIndex & 31) &
             addPropagationBlockBits[useEntry->instruction->block->index].in[recordIndex >> 5]) == 0)
            return 0;
        for (useNode = useEntry->instruction->block->instructions; useNode != NULL; useNode = useNode->next) {
            if (useNode == useEntry->instruction)
                break;
            if (record->opcode == PC_ADD) {
                entry = useNode->operandData.operands;
                remaining = useNode->operand_count;
                while (remaining--) {
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == sourceValue)
                        return 0;
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == secondSourceValue)
                        return 0;
                    entry++;
                }
            } else {
                entry = useNode->operandData.operands;
                remaining = useNode->operand_count;
                while (remaining--) {
                    if (entry->kind == operandType && (entry->flags & 2) && entry->value.reg == sourceValue)
                        return 0;
                    entry++;
                }
            }
        }
    }
    return 1;
}

void propagate_add_operands(SInt32 index)
{
    CodeMotionEntryLink *node;
    PCodeInstruction *dst;
    PCodeInstruction **src;

    src = &add_propagation_entries[index].instruction;
    node = add_propagation_entries[index].list;
    while (node != NULL) {
        dst = cm_entries[node->entry_index].instruction;
        if ((*src)->opcode == PC_ADD) {
            if ((dst->flags & (fIsRead | fIsWrite)) != 0) {
                dst->opcode += 2;
                dst->flags |= fIsPtrOp;
                dst->operandData.operands[1] = (*src)->operandData.operands[1];
                dst->operandData.operands[2] = (*src)->operandData.operands[2];
            } else if (dst->opcode == PC_ADDI) {
                if (dst->operandData.operands[2].value.signed_value != 0)
                    CError_FATAL(680);
                dst->opcode = PC_ADD;
                dst->operandData.operands[1] = (*src)->operandData.operands[1];
                dst->operandData.operands[2] = (*src)->operandData.operands[2];
            } else {
                CError_FATAL(686);
            }
        } else {
            SInt32 tmp;
            dst->operandData.operands[1] = (*src)->operandData.operands[1];
            if ((*src)->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                tmp = dst->operandData.operands[2].value.signed_value +
                      (*src)->operandData.operands[2].value.signed_value;
                dst->operandData.operands[2] = (*src)->operandData.operands[2];
            } else {
                tmp = dst->operandData.operands[2].value.signed_value +
                      (*src)->operandData.operands[2].value.signed_value;
                dst->operandData.operands[2] = (*src)->operandData.operands[2];
            }
            dst->operandData.operands[2].value.signed_value = tmp;
        }
        node = node->next;
    }
    PCode_UnlinkInstruction(*src);
    gAddPropagationChanged = 1;
}

void COpt_AddPropagation(void)
{
    SInt16 iteration;
    SInt32 blockIndex;
    SInt32 canPropagate;
    struct CodeBits *blockBits;

    iteration = 0;
    gAddPropagationChanged = 0;
    do {
        addPropagationChanged = 0;
        fn_00521950();
        if (add_propagation_entry_count > 0) {
            COpt_SetLoopCodeMotionMode(0);
            build_add_propagation_entries();
            addPropagationBlockBits =
                CompilerTools_AllocatePoolMemory(gPCodeBlockCount * sizeof(*addPropagationBlockBits));
            blockBits = addPropagationBlockBits;
            for (blockIndex = 0; gPCodeBlockCount > blockIndex; blockIndex++) {
                blockBits->gen = CompilerTools_AllocatePoolMemory((add_propagation_entry_count + 31) >> 5 << 2);
                blockBits->kill = CompilerTools_AllocatePoolMemory((add_propagation_entry_count + 31) >> 5 << 2);
                blockBits->in = CompilerTools_AllocatePoolMemory((add_propagation_entry_count + 31) >> 5 << 2);
                blockBits->out = CompilerTools_AllocatePoolMemory((add_propagation_entry_count + 31) >> 5 << 2);
                blockBits++;
            }
            fn_00521480();
            SpillCode_BuildBlockOrder();
            compute_add_propagation_in_out();
            for (blockIndex = 0; blockIndex < add_propagation_entry_count; blockIndex++) {
                if (add_propagation_entries[blockIndex].instruction->flags & fRecordBit | 0x400)
                    canPropagate = 0;
                else
                    canPropagate = COpt_AllLinked(blockIndex, add_propagation_entries[blockIndex].list);
                if (canPropagate)
                    propagate_add_operands(blockIndex);
            }
        }
        CompilerTools_ResetPool();
        iteration++;
    } while (copts.deleteDeadInstructions >= 4 && addPropagationChanged != 0 && iteration < 3);
}
