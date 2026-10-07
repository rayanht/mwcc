#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/LoadDeletion.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BitVectors.h"
#include "compiler/CRTTI.h"
#include "compiler/CodeMotion.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/LoopDetection.h"
#include "compiler/PCode.h"
/* Per-entry storage for the two load-deletion bit sets. */

void LoadDeletion_InitializeLoadLivenessRecordCounts(void)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;
    unsigned int count;

    block_record_counts = oalloc(gPCodeBlockCount * sizeof(*block_record_counts));
    load_liveness_record_start = oalloc(gPCodeBlockCount * sizeof(*load_liveness_record_start));
    data_0058820c = 0;
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        load_liveness_record_start[block->index] = data_0058820c;
        count = 0;
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if (instruction->opcode == PC_LI || (unsigned short)(instruction->opcode - 350) <= 2) {
                if (instruction->operandData.operands[0].value.reg >= 32) {
                    ++data_0058820c;
                    ++count;
                }
            }
        }
        block_record_counts[block->index] = count;
    }
}

/* Instruction records inspected for counting. */
void LoadDeletion_RecordImmediateLoadLiveness(void)
{
    PCodeInstruction *instruction;
    struct CodeMotionEntryLink *entry;
    CodeMotionEntry *definition;
    CodeMotionEntry *use;
    PCodeBlock *block;
    struct E *record;
    int definitionIndex;
    UInt32 *liveBits;
    int registerIndex;
    int useIndex;

    immediateLoadLiveness = (struct E *)oalloc(data_0058820c * sizeof(*immediateLoadLiveness));
    liveBits = (UInt32 *)oalloc(((data_00587e38 + 31) >> 5) * sizeof(*liveBits));
    block = gPCodeBlocks;
    while (block != NULL) {
        if (block_record_counts[block->index] != 0) {
            CodeMotion_AllocateBits(liveBits, data_00587fe4[block->index].use_sets[3], data_00587e38);
            record = immediateLoadLiveness +
                     (load_liveness_record_start[block->index] + block_record_counts[block->index] - 1);
            instruction = block->reverse_instructions;
            if (instruction != NULL) {
                do {
                    if ((instruction->flags & PCodeInstruction_SkipCodeMotion) == 0 &&
                        instruction->operand_count != 0) {
                        if (instruction->opcode == PC_LI) {
                            registerIndex = instruction->operandData.operands[0].value.reg;
                            if (registerIndex >= 32) {
                                record->inst = instruction;
                                record->flag = 0;
                                entry = code_motion_register_use_heads[registerIndex];
                                if (entry != NULL) {
                                    do {
                                        if ((1 << entry->entry_index & liveBits[entry->entry_index >> 5]) != 0) {
                                            record->flag = 1;
                                            break;
                                        }
                                        entry = entry->next;
                                    } while (entry != NULL);
                                }
                                record--;
                            }
                        }
                        if (instruction->opcode == PC_VSPLTISB || instruction->opcode == PC_VSPLTISH ||
                            instruction->opcode == PC_VSPLTISW) {
                            registerIndex = instruction->operandData.operands[0].value.reg;
                            if (registerIndex >= 32) {
                                record->inst = instruction;
                                record->flag = 0;
                                entry = register_use_entry_heads[registerIndex];
                                if (entry != NULL) {
                                    do {
                                        if ((1 << entry->entry_index & liveBits[entry->entry_index >> 5]) != 0) {
                                            record->flag = 1;
                                            break;
                                        }
                                        entry = entry->next;
                                    } while (entry != NULL);
                                }
                                record--;
                            }
                        }
                        definition = &code_motion_entries[definitionIndex = instruction->useStart];
                        for (; definitionIndex < codeMotionEntryCount && definition->instruction == instruction;
                             definitionIndex++) {
                            if (definition->kind == 0) {
                                entry = code_motion_register_use_heads[definition->value.reg];
                                if (entry != NULL) {
                                    do {
                                        liveBits[entry->entry_index >> 5] &= ~(1 << (entry->entry_index & 31));
                                        entry = entry->next;
                                    } while (entry != NULL);
                                }
                            } else if (definition->kind == 1) {
                                entry = codeMotionUseEntryHeads[definition->value.reg];
                                if (entry != NULL) {
                                    do {
                                        liveBits[entry->entry_index >> 5] &= ~(1 << (entry->entry_index & 31));
                                        entry = entry->next;
                                    } while (entry != NULL);
                                }
                            } else if (definition->kind == 9) {
                                entry = register_use_entry_heads[definition->value.reg];
                                if (entry != NULL) {
                                    do {
                                        liveBits[entry->entry_index >> 5] &= ~(1 << (entry->entry_index & 31));
                                        entry = entry->next;
                                    } while (entry != NULL);
                                }
                            }
                            definition++;
                        }
                        use = &cm_entries[useIndex = instruction->definitionStart];
                        for (; useIndex < data_00587e38 && use->instruction == instruction; useIndex++) {
                            if (use->kind == 0 || use->kind == 1 || use->kind == 9) {
                                liveBits[useIndex >> 5] |= 1 << useIndex;
                            }
                            use++;
                        }
                    }
                    instruction = instruction->previous;
                } while (instruction != NULL);
            }
        }
        block = block->next;
    }
}

void LoadDeletion_BuildLoadLivenessSets(void)
{
    PCodeBlock *cb;
    CBlockData *bd;
    PCodeInstruction *obj;
    PCodeInstruction *o;
    PCodeOperand *rec;
    E *p;
    UInt32 *setA;
    UInt32 *setB;
    SInt32 num;
    SInt32 i;
    SInt32 start;
    SInt32 n;

    for (cb = gPCodeBlocks; cb != NULL; cb = cb->next) {
        bd = data_00587c98 + cb->index;
        setA = bd->generatedLoads;
        setB = bd->killedLoads;
        CRTTI_FillWords(setA, data_0058820c, 0);
        CRTTI_FillWords(setB, data_0058820c, 0);
        start = load_liveness_record_start[cb->index];
        for (obj = cb->instructions; obj != NULL; obj = obj->next) {
            if ((obj->flags & fIsBranch) == 0 && obj->operand_count != 0) {
                rec = obj->operandData.operands;
                n = obj->operand_count;
                while (n--) {
                    if ((rec->kind == PCOp_GPR || rec->kind == PCOp_VR) && (rec->flags & 2) != 0) {
                        p = immediateLoadLiveness;
                        for (i = 0; data_0058820c > i; i++, p++) {
                            if (((PCodeInstruction *)p->inst)->operandData.operands[0].kind == rec->kind &&
                                (((PCodeInstruction *)p->inst)->operandData.operands[0].value.reg == rec->value.reg ||
                                 ((PCodeInstruction *)p->inst)->operandData.operands[1].value.reg == rec->value.reg)) {
                                if (((PCodeInstruction *)p->inst)->block == cb)
                                    setA[i >> 5] &= ~(1 << (i & 31));
                                else
                                    setB[i >> 5] |= 1 << (i & 31);
                            }
                        }
                    }
                    rec++;
                }
                if (obj->opcode == PC_LI && obj->operandData.operands[0].value.reg >= 0x20) {
                    setA[start >> 5] |= 1 << (start & 31);
                    start++;
                }
                if ((obj->opcode == PC_VSPLTISB || obj->opcode == PC_VSPLTISH || obj->opcode == PC_VSPLTISW) &&
                    obj->operandData.operands[0].value.reg >= 0x20) {
                    setA[start >> 5] |= 1 << (start & 31);
                    start++;
                }
            }
        }
    }
}

/* Class and allocation-entry list used by the allocation pass. */
/* Pointer at the beginning of a ten-byte table entry. */
