#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/VectorArraysToRegs.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/BitVectors.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CRTTI.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoadDeletion.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Registers.h"
#include "compiler/StrengthReduction.h"
#include "compiler/Switch.h"
/* Removed unused two-operand PCodeInstruction view; VectorArraysToRegs_ReplaceArrayUsesWithVMR uses opcode, flags and operand count at these offsets. */
/* Vector-array uses and the tables used by this pass. */

static int load_index_count;
static VectorArrayEntry *load_index_entries;
static struct CodeMotionBits *codeMotionBits;
static int *load_index_entry_counts;
static int *block_entry_start;
static unsigned char data_00582c9c;

int fn_0052ce10(void)
{
    struct AggregateRecord *analysis;
    CodeMotionBits *bitsets;
    int index;

    data_00582c9c = 0;
    analysis = VectorArraysToRegs_BuildAggregateRecords();
    if (analysis != NULL) {
        fn_0052d8d0(analysis);
        if (load_index_count > 0) {
            COpt_SetLoopCodeMotionMode(0);
            VectorArraysToRegs_BuildLoadIndexEntries(analysis);
            codeMotionBits = CompilerTools_AllocatePoolMemory(gPCodeBlockCount * sizeof(CodeMotionBits));
            bitsets = codeMotionBits;
            for (index = 0; index < gPCodeBlockCount; ++index) {
                bitsets->gen = CompilerTools_AllocatePoolMemory(((load_index_count + 31) >> 5) * sizeof(*bitsets->gen));
                bitsets->kill =
                    CompilerTools_AllocatePoolMemory(((load_index_count + 31) >> 5) * sizeof(*bitsets->kill));
                bitsets->in = CompilerTools_AllocatePoolMemory(((load_index_count + 31) >> 5) * sizeof(*bitsets->in));
                bitsets->out = CompilerTools_AllocatePoolMemory(((load_index_count + 31) >> 5) * sizeof(*bitsets->out));
                bitsets++;
            }
            VectorArraysToRegs_ComputeGenKill(analysis);
            SpillCode_BuildBlockOrder();
            VectorArraysToRegs_ComputeInOutBits();
            fn_0052cf10(analysis);
        }
    }
    CompilerTools_ResetPool();
    return data_00582c9c;
}

static struct AggregateRecord *FindCandidate(struct AggregateRecord *list, Object *object)
{
    struct AggregateRecord *match;
    for (match = list; match; match = match->next) {
        if (match->object == object)
            return match;
    }
    return NULL;
}

static void CheckUses(long classIndex, VectorArrayUse *entries, PCodeInstruction *classType,
                      struct AggregateRecord *list)
{
    VectorArrayUse *entry;
    struct AggregateRecord *match;
    entry = entries;
    if (entries != NULL) {
        do {
            if (entry != NULL && fn_0052d1e0(classIndex, entry->instructionIndex) == 0) {
                match = FindCandidate(list, classType->operandData.operands[2].object);
                match->flag = 1;
                break;
            }
            entry = entry->next;
        } while (entry != NULL);
    }
}

void fn_0052cf10(struct AggregateRecord *candidates)
{
    long classIndex;
    int candidateCount;
    int slotCount;
    struct AggregateRecord **head;
    struct AggregateRecord *candidate;
    int slot;
    struct AggregateRecord *previous;
    struct AggregateRecord *smallest;
    long smallestTotal;
    struct AggregateRecord *scan;
    struct AggregateRecord *remaining;
    long index;
    struct AggregateRecord *next;

    for (classIndex = 0; classIndex < load_index_count; classIndex++) {
        CheckUses(classIndex, load_index_entries[classIndex].uses, load_index_entries[classIndex].array, candidates);
    }
    candidateCount = 0;
    slotCount = 0;
    head = &candidates;
    candidate = candidates;
    while (candidate != NULL) {
        if (candidate->flag != 0) {
            *head = candidate->next;
            candidate = *head;
        } else {
            candidateCount++;
            slotCount += candidate->count;
            for (slot = 0; slot < candidate->count; slot++) {
                candidate->index += candidate->slots[slot];
            }
            candidate = candidate->next;
        }
    }
    if (candidates != NULL) {
        while (slotCount > 32) {
            smallestTotal = 0;
            smallest = NULL;
            scan = candidates;
            while (scan != NULL) {
                if (smallest != NULL) {
                    if (scan->index < smallestTotal) {
                        smallestTotal = scan->index;
                        smallest = scan;
                    }
                } else {
                    smallest = scan;
                    smallestTotal = scan->index;
                }
                scan = scan->next;
            }
            if (smallest == NULL) {
                break;
            }
            if (smallest == candidates) {
                candidates = smallest->next;
            } else if ((previous = candidates) != NULL) {
                do {
                    if ((next = previous->next) == smallest) {
                        previous->next = smallest->next;
                        break;
                    }
                    previous = next;
                } while (next != NULL);
            }
            candidateCount--;
            slotCount -= smallest->count;
        }
        remaining = candidates;
        if (remaining == NULL) {
            return;
        }
        while (remaining != NULL) {
            for (slot = 0; slot < remaining->count; slot++) {
                remaining->slots[slot] = gUsedVirtualRegistersVR;
                gUsedVirtualRegistersVR++;
            }
            remaining = remaining->next;
        }
        if (candidates != NULL) {
            for (index = 0; index < load_index_count; index++) {
                VectorArraysToRegs_ReplaceArrayUsesWithVMR(candidates, index);
            }
        }
    }
}

static inline struct AggregateRecord *VectorArraysToRegs_0052d0a0_inline1(struct AggregateRecord *v1,
                                                                          PCodeInstruction *v3)
{
    Object *v2;
    v2 = v3->operandData.operands[2].object;
    while ((int)v1 != 0) {
        if (v1->object == v2) {
            return v1;
        }
        v1 = v1->next;
    }
    return NULL;
}

void VectorArraysToRegs_ReplaceArrayUsesWithVMR(struct AggregateRecord *registerMap, int arrayIndex)
{
    VectorArrayUse *use;
    PCodeInstruction *instruction;
    int vectorRegister;
    struct AggregateRecord *registers;
    VectorArrayEntry *entry;
    int slot;
    entry = &load_index_entries[arrayIndex];

    if (entry->array == NULL)
        return;
    instruction = entry->array;
    registers = VectorArraysToRegs_0052d0a0_inline1(registerMap, instruction);
    slot = instruction->operandData.operands[2].value.signed_value / 16;
    if (registers == NULL || slot > registers->count)
        return;
    vectorRegister = registers->slots[slot];
    use = entry->uses;
    if (use != NULL) {
        do {
            instruction = cm_entries[use->instructionIndex].instruction;
            if (instruction->opcode == PC_LVX) {
                data_00582c9c = 1;
                instruction->opcode = PC_VMR;
                instruction->flags = 2051;
                instruction->operand_count = 2;
                instruction->operandData.operands[1].kind = PCOp_VR;
                instruction->operandData.operands[1].flags = 1;
                instruction->operandData.operands[1].value.reg = vectorRegister;
            } else if (instruction->opcode == PC_STVX) {
                data_00582c9c = 1;
                instruction->opcode = PC_VMR;
                instruction->flags = 2051;
                instruction->operand_count = 2;
                instruction->operandData.operands[1] = instruction->operandData.operands[0];
                instruction->operandData.operands[0].value.reg = vectorRegister;
                instruction->operandData.operands[0].flags = 2;
            } else {
                CError_Internal("VectorArraysToRegs.c", 674);
            }
            use = use->next;
        } while (use != NULL);
    }
    PCode_UnlinkInstruction(entry->array);
}

static void ClearRec(struct AggregateRecord *r, long n)
{
    int i;
    ((char *)r)[8] = (char)(((char *)r)[8] & 254);
    i = 0;
    while (i < n) {
        r->slots[i] = 0;
        i = i + 1;
    }
}

static inline struct AggregateRecord *fn_0052d8d0_inline1(PCodeOperand *operand, struct AggregateRecord *analysis)
{
    int object = (int)operand->object;
    struct AggregateRecord *counts = analysis;
    while (counts != NULL) {
        if ((int)counts->object == object)
            return counts;
        counts = counts->next;
    }
    return NULL;
}

static inline struct AggregateRecord *fn_0052d650_inline1(PCodeOperand *v4, struct AggregateRecord *a0)
{
    int v6;
    struct AggregateRecord *v7;
    v6 = (int)v4->object;
    v7 = a0;
    while ((int)v7 != 0) {
        if ((int)v7->object == v6)
            return v7;
        v7 = v7->next;
    }
    return NULL;
}

static inline struct AggregateRecord *fn_0052d460_inline1(struct AggregateRecord *v6, PCodeOperand *v5)
{
    int v7;
    v7 = (int)v5->object;
    while ((int)v6 != 0) {
        if ((int)v6->object == v7)
            return v6;
        v6 = v6->next;
    }
    return NULL;
}

/* Instruction records inspected for counting. */
/* Pointer at the beginning of a ten-byte table entry. */
int fn_0052d1e0(int valueIndex, int entryIndex)
{
    PCodeInstruction *valueRecord;
    struct CodeMotionEntry *entry;
    int value;
    valueRecord = load_index_entries[valueIndex].array;
    entry = &cm_entries[entryIndex];
    if (valueRecord == NULL) {
        return 0;
    }
    value = valueRecord->operandData.operands[0].value.reg;
    if (entry->instruction->opcode != PC_LVX && entry->instruction->opcode != PC_STVX) {
        return 0;
    }
    if (entry->instruction->operandData.operands[1].kind != PCOp_GPR ||
        entry->instruction->operandData.operands[1].value.reg != 0) {
        return 0;
    }
    if (entry->instruction->operandData.operands[2].value.reg != value) {
        return 0;
    }
    return 1;
}

void VectorArraysToRegs_ComputeInOutBits(void)
{
    UInt32 value;
    PCodeBlock *blk;
    PCodeBlockLink *node;
    CodeMotionBits *bits;
    UInt32 *pp;
    UInt32 *qq;
    UInt32 *ip;
    UInt32 *gp;
    UInt32 *dp;
    UInt32 *sp;
    SInt32 nwords;
    int i;
    SInt32 j;
    SInt32 changed;
    nwords = (load_index_count + 0x1f) >> 5;
    bits = &codeMotionBits[gPCodeBlocks->index];
    CRTTI_FillWords(bits->in, load_index_count, 0);
    CodeMotion_AllocateBits(bits->out, bits->gen, load_index_count);
    for (blk = gPCodeBlocks->next; blk != NULL; blk = blk->next) {
        bits = &codeMotionBits[blk->index];
        qq = bits->out;
        pp = bits->kill;
        for (j = 0; j < nwords; j++)
            *qq++ = ~*pp++;
    }
    do {
        changed = 0;
        for (i = 0; i < gPCodeBlockCount; i++) {
            if (gPCodeBlockOrder[i] != NULL) {
                bits = &codeMotionBits[gPCodeBlockOrder[i]->index];
                if ((node = gPCodeBlockOrder[i]->predecessors) != NULL) {
                    ip = bits->in;
                    CodeMotion_AllocateBits(ip, codeMotionBits[node->payload.block->index].out, load_index_count);
                    for (node = node->next; node != NULL; node = node->next)
                        CRTTI_IntersectBitVectors(ip, codeMotionBits[node->payload.block->index].out, load_index_count);
                }
                dp = bits->out;
                ip = bits->in;
                gp = bits->gen;
                sp = bits->kill;
                for (j = 0; j < nwords; j++) {
                    value = (~*sp & *ip) | *gp;
                    if (value != *dp) {
                        *dp = value;
                        changed = 1;
                    }
                    ip++;
                    dp++;
                    sp++;
                    gp++;
                }
            }
        }
    } while (changed);
}

void VectorArraysToRegs_ComputeGenKill(struct AggregateRecord *entries)
{
    PCodeOperand *reference;
    int definition_index;
    VectorArrayEntry *definition;
    PCodeOperand *object_operand;
    struct AggregateRecord *entry;
    CodeMotionBits *block_data;
    UInt32 *local_bits;
    UInt32 *incoming_bits;
    PCodeBlock *block;
    int reference_count;
    PCodeInstruction *instruction;
    int object_index;
    block = gPCodeBlocks;
    while (block != NULL) {
        block_data = codeMotionBits + block->index;
        local_bits = block_data->gen;
        incoming_bits = block_data->kill;
        CRTTI_FillWords(local_bits, load_index_count, 0);
        CRTTI_FillWords(incoming_bits, load_index_count, 0);
        object_index = block_entry_start[block->index];
        instruction = block->instructions;
        while (instruction != NULL) {
            if ((instruction->flags & PCodeInstruction_SkipCodeMotion) == 0 && instruction->operand_count != 0) {
                reference = &instruction->operandData.operands[0];
                reference_count = instruction->operand_count;
                while (reference_count--) {
                    if (reference->kind == PCOp_GPR && (reference->flags & PCodeOperand_Definition) != 0) {
                        definition_index = 0;
                        definition = load_index_entries;
                        while (definition_index < load_index_count) {
                            if (definition->array != NULL &&
                                definition->array->operandData.operands[0].kind == reference->kind &&
                                definition->array->operandData.operands[0].value.reg == reference->value.reg) {
                                if (definition->array->block == block) {
                                    local_bits[definition_index >> 5] &= ~(1 << (definition_index & 31));
                                } else {
                                    incoming_bits[definition_index >> 5] |= 1 << definition_index;
                                }
                            }
                            definition_index = definition_index + 1;
                            definition = definition + 1;
                        }
                    }
                    reference++;
                }
                if (instruction->opcode == PC_ADDI) {
                    object_operand = &instruction->operandData.operands[2];
                    if (instruction->operandData.operands[2].kind == PCOp_MEMORY &&
                        object_operand->flags == PCodeOperand_Use) {
                        entry = fn_0052d460_inline1(entries, object_operand);
                        if (entry != NULL && entry->flag == 0) {
                            local_bits[object_index >> 5] |= 1 << object_index;
                            object_index = object_index + 1;
                        }
                    }
                }
            }
            instruction = instruction->next;
        }
        block = block->next;
    }
}

void VectorArraysToRegs_BuildLoadIndexEntries(struct AggregateRecord *argument)
{
    VectorArrayEntry *entry;
    PCodeOperand *operand;
    UInt32 *indexBits;
    int registerIndex;
    CodeMotionEntryLink *use;
    CodeMotionEntryLink *uses;
    CodeMotionEntry *useEntry;
    CodeMotionEntryLink *definitions;
    CodeMotionEntryLink *definition;
    int definitionIndex;
    CodeMotionEntry *definitionEntry;
    struct VectorArrayUse *newIndex;
    int useIndex;
    PCodeInstruction *instruction;
    PCodeBlock *block;
    struct AggregateRecord *info;
    int entryCount;
    int entryStart;

    load_index_entries = CompilerTools_AllocatePoolMemory(load_index_count * sizeof(*load_index_entries));
    memclrw(load_index_entries, load_index_count * sizeof(*load_index_entries));
    indexBits = CompilerTools_AllocatePoolMemory(((data_00587e38 + 31) >> 5) * sizeof(*indexBits));
    block = gPCodeBlocks;
    while (block != NULL) {
        if (load_index_entry_counts[block->index] != 0) {
            CodeMotion_AllocateBits(indexBits, data_00587fe4[block->index].use_sets[3], data_00587e38);
            entryStart = block_entry_start[block->index];
            entryCount = load_index_entry_counts[block->index];
            entry = load_index_entries + (entryStart + entryCount - 1);
            instruction = block->reverse_instructions;
            while (instruction != NULL) {
                if ((instruction->flags & PCodeInstruction_SkipCodeMotion) == 0 && instruction->operand_count != 0) {
                    if (instruction->opcode == PC_ADDI) {
                        operand = &instruction->operandData.operands[2];
                        registerIndex = instruction->operandData.operands[0].value.reg;
                        if (registerIndex >= 32 && operand->kind == PCOp_MEMORY && operand->flags == PCodeOperand_Use) {
                            info = fn_0052d650_inline1(operand, argument);
                            if (info != NULL && info->flag == 0) {
                                entry->array = instruction;
                                entry->uses = NULL;
                                use = uses = code_motion_register_use_heads[registerIndex];
                                if (uses != NULL) {
                                    do {
                                        if ((1 << use->entry_index & indexBits[use->entry_index >> 5]) != 0) {
                                            newIndex = CompilerTools_AllocatePoolMemory(sizeof(*newIndex));
                                            newIndex->instructionIndex = use->entry_index;
                                            newIndex->next = entry->uses;
                                            entry->uses = newIndex;
                                        }
                                        use = use->next;
                                    } while (use != NULL);
                                }
                                entry--;
                            }
                        }
                    }
                    useEntry = code_motion_entries + (useIndex = instruction->useStart);
                    for (; useIndex < codeMotionEntryCount && useEntry->instruction == instruction; useIndex++) {
                        if (useEntry->kind == 0) {
                            definition = definitions = code_motion_register_use_heads[useEntry->value.reg];
                            if (definitions != NULL) {
                                do {
                                    indexBits[definition->entry_index >> 5] &= ~(1 << (definition->entry_index & 31));
                                    definition = definition->next;
                                } while (definition != NULL);
                            }
                        }
                        useEntry++;
                    }
                    definitionEntry = cm_entries + (definitionIndex = instruction->definitionStart);
                    for (; definitionIndex < data_00587e38 && definitionEntry->instruction == instruction;
                         definitionIndex++) {
                        if (definitionEntry->kind == 0 || definitionEntry->kind == 1 || definitionEntry->kind == 9) {
                            indexBits[definitionIndex >> 5] |= 1 << definitionIndex;
                        }
                        definitionEntry++;
                    }
                }
                instruction = instruction->previous;
            }
        }
        block = block->next;
    }
}

void fn_0052d8d0(struct AggregateRecord *analysis)
{
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    struct AggregateRecord *counts;
    struct PCodeBlock *block;
    int *allocation;
    int remaining;
    int block_count;
    load_index_entry_counts = CompilerTools_AllocatePoolMemory(gPCodeBlockCount << 2);
    memclrw(load_index_entry_counts, gPCodeBlockCount << 2);
    allocation = (int *)CompilerTools_AllocatePoolMemory(gPCodeBlockCount << 2);
    block_entry_start = allocation;
    memclrw(allocation, gPCodeBlockCount << 2);
    load_index_count = 0;
    block = gPCodeBlocks;
    while (block != NULL) {
        block_entry_start[block->index] = load_index_count;
        block_count = 0;
        instruction = (PCodeInstruction *)block->instructions;
        if (instruction != NULL) {
            do {
                if ((instruction->flags & PCodeInstruction_SkipCodeMotion) == 0 && instruction->operand_count != 0) {
                    operand = (PCodeOperand *)&instruction->operandData.operands;
                    remaining = instruction->operand_count;
                    while (remaining--) {
                        if (operand->kind == PCOp_MEMORY && operand->flags == 1) {
                            counts = fn_0052d8d0_inline1(operand, analysis);
                            if (counts != NULL && counts->flag == 0) {
                                if (instruction->opcode != PC_ADDI) {
                                    counts->flag = 1;
                                } else {
                                    load_index_count += 1;
                                    block_count = block_count + 1;
                                }
                                if (counts->flag == 0) {
                                    if (operand->value.signed_value / 16 < counts->count) {
                                        counts->slots[operand->value.signed_value / 16] += 1;
                                    } else {
                                        counts->flag = 1;
                                    }
                                }
                            }
                        }
                        operand++;
                    }
                }
                instruction = instruction->next;
            } while (instruction != NULL);
        }
        load_index_entry_counts[block->index] = block_count;
        block = block->next;
    }
}

struct AggregateRecord *VectorArraysToRegs_BuildAggregateRecords(void)
{
    ObjectList *candidate;
    int size;
    UInt32 qualifiers;
    long count;
    struct AggregateRecord *records, *record;
    records = NULL;
    candidate = locals;
    while (candidate != NULL) {
        if (candidate->object != NULL) {
            if (candidate->object->type->type == TYPEPOINTER) {
                qualifiers = TYPE_POINTER(candidate->object->type)->qual;
            } else {
                qualifiers = candidate->object->qual;
            }
            qualifiers = qualifiers & Q_VOLATILE;
            if (qualifiers == 0 && candidate->object->type != NULL && candidate->object->type->type == TYPEARRAY &&
                TYPE_POINTER(candidate->object->type)->target->type == TYPESTRUCT &&
                (int)TYPE_STRUCT(TYPE_POINTER(candidate->object->type)->target)->stype >= 4 &&
                (int)TYPE_STRUCT(TYPE_POINTER(candidate->object->type)->target)->stype <= 14) {
                size = candidate->object->type->size;
                count = candidate->object->type->size / 16;
                if (count > 0 && count <= 8) {
                    record = CompilerTools_AllocatePoolMemory(sizeof(*record) + (count - 1) * sizeof(record->slots[0]));
                    record->next = records;
                    records = record;
                    record->object = candidate->object;
                    record->size = size;
                    record->count = size / 16;
                    record->index = 0;
                    ClearRec(record, count);
                }
            }
        }
        candidate = candidate->next;
    }
    return records;
}
