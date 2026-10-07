#define CERROR_FILE "StrengthReduction.c"
#include "compiler/common.h"
#include "compiler/StrengthReduction.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
/* 12-byte table entry: byte tag plus a 2-byte aligned payload. */

void StrengthReduction_RunLoopPasses(void)

{
    gStrengthReductionChanged = 0;
    if (data_0058763c != NULL) {
        COpt_SetLoopCodeMotionMode(0);
        visit_loops_children_first(data_0058763c);
        fn_00527e80(data_0058763c);
        visit_code_motion_searches(data_0058763c);
        CompilerTools_ResetPool();
    }
    return;
}

int visit_code_motion_searches(Loop *p)
{
    while (p) {
        if (p->children)
            visit_code_motion_searches(p->children);
        if (p->codeMotionSearches)
            fn_00527290(p);
        p = p->sibling;
    }
}

void fn_00527290(Loop *holder)
{
    CMRegisterNode *node;
    CodeMotionCandidate *child;
    CMRegisterNode *candidate, *other;
    Loop *evaluationContext;
    ClassLookupResult candidateInfo, otherInfo;

    for (node = holder->codeMotionSearches; node != NULL; node = node->next) {
        if (node->candidates != NULL)
            rewrite_code_motion_candidates(node);
        evaluationContext = node->loop;
        for (child = node->candidates; child != NULL; child = child->next)
            hoist_child_code_motion_instructions(child->destination_register, child->scale * node->increment,
                                                 child->replacement_instruction, evaluationContext);
        hoist_child_code_motion_instructions(node->reg, node->increment, node->reachingDefinition, evaluationContext);
    }

    for (candidate = holder->codeMotionSearches; candidate != NULL; candidate = candidate->next) {
        if (candidate->reg == -1)
            continue;
        fn_005275a0(candidate, &candidateInfo);
        if (candidateInfo.index == -1)
            continue;
        for (other = holder->codeMotionSearches; other != NULL; other = other->next) {
            if (other->reg == -1)
                continue;
            if (other == candidate)
                continue;
            fn_005275a0(other, &otherInfo);
            if (otherInfo.index == -1)
                continue;
            if (candidateInfo.index == otherInfo.index && candidateInfo.kind60Value == otherInfo.kind60Value &&
                candidateInfo.kind63Value == otherInfo.kind63Value &&
                candidateInfo.extraValue == otherInfo.extraValue && candidate->increment == other->increment) {
                if (candidateInfo.value < otherInfo.value) {
                    rewrite_and_relocate_addi_definition(holder, other, candidate,
                                                         otherInfo.value - candidateInfo.value);
                } else {
                    rewrite_and_relocate_addi_definition(holder, candidate, other,
                                                         candidateInfo.value - otherInfo.value);
                    break;
                }
            }
        }
    }
}

void rewrite_and_relocate_addi_definition(Loop *obj, CMRegisterNode *firstId, CMRegisterNode *secondId, SInt32 offset)
{
    PCodeInstruction *firstDefinition;
    PCodeInstruction *secondDefinition;
    SInt32 firstReg;
    SInt32 secondReg;
    SInt32 i;
    PCodeBlock *target;
    PCodeBlock *targetBlock;
    PCodeInstruction *targetInstruction;
    PCodeInstruction *ownerInstruction;
    PCodeBlockLink *block;

    firstDefinition = NULL;
    secondDefinition = NULL;

    firstReg = firstId->reg;
    CError_ASSERT(932, 0 <= firstReg);
    secondReg = secondId->reg;
    CError_ASSERT(936, 0 <= secondReg);

    if (offset != (SInt16)offset)
        return;

    for (block = obj->blocks; block != NULL; block = block->next) {
        PCodeOperand *operand;
        PCodeInstruction *instruction;
        for (instruction = block->payload.block->instructions; instruction != NULL; instruction = instruction->next) {
            if (firstDefinition != NULL) {
                operand = instruction->operandData.operands;
                i = instruction->operand_count;
                while (i--) {
                    if (operand->kind == PCOp_GPR && operand->value.reg == firstReg)
                        return;
                    operand++;
                }
            }
            if (instruction->opcode == PC_ADDI) {
                if (instruction->operandData.operands[0].value.reg == firstReg) {
                    if (firstDefinition != NULL)
                        return;
                    firstDefinition = instruction;
                } else if (instruction->operandData.operands[0].value.reg == secondReg) {
                    if (secondDefinition != NULL)
                        return;
                    secondDefinition = instruction;
                }
            }
        }
    }

    ownerInstruction = obj->body->reverse_instructions;
    if ((ownerInstruction->flags & fIsBranch) != 0) {
        target = NULL;
        for (i = 0; i < ownerInstruction->operand_count; i++) {
            if (ownerInstruction->operandData.operands[i].kind == PCOp_LABEL) {
                target = ownerInstruction->operandData.operands[i].value.label->target.block;
                break;
            }
        }
        if (target == NULL)
            return;
    } else {
        target = obj->body->next;
    }

    PCode_UnlinkInstruction(firstDefinition);
    firstDefinition->operandData.operands[1].value.reg = secondReg;
    firstDefinition->operandData.operands[2].value.signed_value = offset;
    if (target->instructions != NULL) {
        PCode_InsertInstructionBefore(target->instructions, firstDefinition);
    } else {
        targetBlock = target;
        targetInstruction = firstDefinition;
        PCode_AppendInstruction(targetBlock, targetInstruction);
    }
    firstId->reg = -1;
    gStrengthReductionChanged = 1;
}

void fn_005275a0(CMRegisterNode *input, ClassLookupResult *result)
{
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    SInt32 operandCount;

    instruction = input->reachingDefinition;
    result->value = 0;
    result->index = -1;
    result->kind60Value = -1;
    result->kind63Value = 0;
    result->extraValue = 0;
    if (input->reachingDefinition == NULL ||
        (input->reachingDefinition->opcode != PC_ADDI && input->reachingDefinition->opcode != PC_ADD))
        return;
    if (instruction->opcode == PC_ADDI) {
        if (instruction->operandData.operands[1].value.reg == input->reg) {
            result->value = instruction->operandData.operands[2].value.signed_value;
            for (instruction = instruction->previous; instruction != NULL; instruction = instruction->previous) {
                operandCount = instruction->operand_count;
                operand = instruction->operandData.operands;
                while (operandCount--) {
                    if (operand->kind == PCOp_GPR && operand->value.reg == input->reg && (operand->flags & 2)) {
                        if (instruction->opcode == PC_ADD) {
                            result->index = instruction->operandData.operands[1].value.reg;
                            result->kind60Value = instruction->operandData.operands[2].value.reg;
                        } else if (instruction->opcode == PC_ADDI) {
                            if (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                                result->index = instruction->operandData.operands[1].value.reg;
                                result->kind63Value = instruction->operandData.operands[2].value.signed_value;
                            } else if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                                result->index = instruction->operandData.operands[1].value.reg;
                                result->kind63Value = instruction->operandData.operands[2].value.signed_value;
                                result->extraValue = (SInt32)instruction->operandData.operands[2].object;
                            }
                        }
                        return;
                    }
                    operand++;
                }
            }
        } else if (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE) {
            result->index = instruction->operandData.operands[1].value.reg;
            result->kind63Value = instruction->operandData.operands[2].value.signed_value;
        } else if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
            result->index = instruction->operandData.operands[1].value.reg;
            result->kind63Value = instruction->operandData.operands[2].value.signed_value;
            result->extraValue = (SInt32)instruction->operandData.operands[2].object;
        }
    } else if (instruction->opcode == PC_ADD) {
        if (instruction->operandData.operands[1].value.reg == input->reg) {
            result->kind60Value = instruction->operandData.operands[2].value.reg;
            for (instruction = instruction->previous; instruction != NULL; instruction = instruction->previous) {
                operandCount = instruction->operand_count;
                operand = instruction->operandData.operands;
                while (operandCount--) {
                    if (operand->kind == PCOp_GPR && operand->value.reg == input->reg && (operand->flags & 2) &&
                        instruction->opcode == PC_ADDI) {
                        if (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                            result->index = instruction->operandData.operands[1].value.reg;
                            result->kind63Value = instruction->operandData.operands[2].value.signed_value;
                        } else if (instruction->operandData.operands[2].kind == PCOp_MEMORY) {
                            result->index = instruction->operandData.operands[1].value.reg;
                            result->kind63Value = instruction->operandData.operands[2].value.signed_value;
                            result->extraValue = (SInt32)instruction->operandData.operands[2].object;
                        }
                        return;
                    }
                    operand++;
                }
            }
        } else {
            result->index = instruction->operandData.operands[1].value.reg;
            result->kind60Value = instruction->operandData.operands[2].value.reg;
        }
    }
}

static void ConvertToMove(CodeMotionCandidate *node)
{
    node->instruction->opcode -= 2;
    node->instruction->operandData.operands[1].value.reg = node->destination_register;
    node->instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
    node->instruction->operandData.operands[2].value.signed_value = 0;
    node->instruction->operandData.operands[2].object = NULL;
}

void rewrite_code_motion_candidates(CMRegisterNode *func)
{
    SInt32 unitScaleCount;
    CodeMotionCandidate *node;
    CodeMotionCandidate *candidate;
    CodeMotionCandidate *matchingNode;
    SInt16 registerValue;
    SInt16 operandIndex;

    unitScaleCount = 0;
    for (candidate = func->candidates; candidate != NULL; candidate = candidate->next) {
        if (candidate->scale == 1)
            unitScaleCount++;
    }
    for (node = func->candidates; node != NULL; node = node->next) {
        if (unitScaleCount > 4 && node->scale == 1)
            continue;
        if (node->instruction->block == NULL)
            continue;
        if (node->source_operand != 0 && node->instruction->operandData.operands[2].kind == PCOp_IMMEDIATE)
            continue;
        if (node->destination_register == -1) {
            node->destination_register = gUsedVirtualRegistersGPR++;
            initialize_candidate_register(node);
            insert_scaled_increment(node);
            if (node->scale == 1) {
                registerValue = node->instruction->operandData.operands[node->source_operand].value.reg;
                for (matchingNode = node->next; matchingNode != NULL; matchingNode = matchingNode->next) {
                    if ((operandIndex = matchingNode->source_operand) != 0 &&
                        matchingNode->instruction->operandData.operands[operandIndex].value.reg == registerValue)
                        matchingNode->destination_register = node->destination_register;
                }
            } else {
                for (candidate = node->next; candidate != NULL; candidate = candidate->next) {
                    if (candidate->scale == node->scale)
                        candidate->destination_register = node->destination_register;
                }
            }
        }
        if ((node->instruction->flags & (fIsRead | fIsWrite)) != 0) {
            ConvertToMove(node);
        } else {
            PCode_InsertInstructionAfter(
                node->instruction,
                PCodeUtilities_CreateInstruction(0x8b, node->source_register, node->destination_register));
            PCode_UnlinkInstruction(node->instruction);
        }
        gStrengthReductionChanged = 1;
    }
}

void hoist_child_code_motion_instructions(SInt16 operandIndex, int mode, void *destination, Loop *context)
{
    Loop *candidate;
    SInt32 index;
    CMRegisterNode *block;
    CodeMotionCandidate *entry;
    PCodeInstruction *operands;
    SInt32 remaining;
    PCodeBlock *destinations;
    PCodeOperand *operand;
    PCodeInstruction *first;

    for (candidate = context->children; candidate != NULL; candidate = candidate->sibling) {
        if (candidate->isKnownCountingLoop == 0 || candidate->skip_leaf_pass_4f == 0)
            continue;
        index = candidate->body->index;
        if ((context->backedge_dominators[index >> 5] & (1 << index)) == 0)
            continue;
        for (block = (CMRegisterNode *)candidate->codeMotionSearches; block != NULL; block = block->next) {
            for (entry = block->candidates; entry != NULL; entry = entry->next) {
                if (matches_redundancy_without_prior_reg_use(entry, block->increment, operandIndex, mode, context,
                                                             candidate) != 0) {
                    PCode_UnlinkInstruction(entry->replacement_instruction);
                    if (destination != NULL) {
                        PCode_InsertInstructionAfter(destination, entry->replacement_instruction);
                    } else {
                        first = (destinations = context->body)->reverse_instructions;
                        if (first != NULL) {
                            for (operands = first; operands != NULL; operands = operands->previous) {
                                operand = operands->operandData.operands;
                                for (remaining = operands->operand_count; remaining--; operand++) {
                                    if (operand->kind == PCOp_GPR && (operand->flags & PCodeOperand_Definition) != 0 &&
                                        operand->value.reg == operandIndex)
                                        break;
                                }
                            }
                            if (operands != NULL)
                                PCode_InsertInstructionAfter(operands, entry->replacement_instruction);
                            else
                                PCode_InsertInstructionBefore(destinations->instructions,
                                                              entry->replacement_instruction);
                        } else {
                            PCode_AppendInstruction(destinations, (PCodeInstruction *)entry->replacement_instruction);
                        }
                    }
                }
            }
        }
    }
}

int matches_redundancy_without_prior_reg_use(CodeMotionCandidate *entry, SInt32 stride, unsigned int reg,
                                             unsigned int displacement, Loop *outerLoop, Loop *innerLoop)
{
    PCodeInstruction *instruction;
    int operandCount;
    PCodeBlockLink *blockLink;
    PCodeOperand *operand;
    unsigned int offset;
    PCodeBlock *block;
    if (entry->replacement_instruction != NULL && entry->source_register == reg) {
        if (entry->replacement_instruction->opcode == PC_MR) {
            offset = 0;
        } else if (entry->replacement_instruction->opcode == PC_ADDI) {
            offset = entry->replacement_instruction->operandData.operands[2].value.immediate_value;
        } else {
            return 0;
        }
        if (displacement == (unsigned long)stride * entry->scale * innerLoop->iterationCount + offset) {
            blockLink = outerLoop->blocks;
            while (blockLink != NULL && blockLink->payload.block != innerLoop->blocks->payload.block) {
                block = blockLink->payload.block;
                for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
                    operandCount = instruction->operand_count;
                    operand = instruction->operandData.operands;
                    while (operandCount--) {
                        if (operand->kind == PCOp_GPR && operand->value.reg == reg)
                            return 0;
                        operand++;
                    }
                }
                blockLink = blockLink->next;
            }
            return 1;
        }
    }
    return 0;
}

static inline SInt16 signed_immediate(SInt32 value)
{
    return value;
}

void insert_scaled_increment(CodeMotionCandidate *request)
{
    CMRegisterNode *group;
    CodeMotionRef *target;
    SInt32 product;
    SInt32 low;
    SInt32 high;

    group = request->owner;
    product = request->scale * group->increment;
    target = group->refs;
    if (target != NULL) {
        high = signed_immediate((product >> 16) + ((product >> 15) & 1));
        low = signed_immediate(product);
        do {
            if (product != low) {
                PCodeInstruction *result;
                result = PCodeUtilities_CreateInstruction(0x42, request->destination_register,
                                                          request->destination_register, 0, high);
                PCode_InsertInstructionAfter(target->def, result);
                if (signed_immediate(product) != 0) {
                    PCodeInstruction *result;
                    result = PCodeUtilities_CreateInstruction(0x3f, request->destination_register,
                                                              request->destination_register, 0, low);
                    PCode_InsertInstructionAfter(target->def->next, result);
                }
            } else {
                PCodeInstruction *result;
                result = PCodeUtilities_CreateInstruction(0x3f, request->destination_register,
                                                          request->destination_register, 0, product);
                PCode_InsertInstructionAfter(target->def, result);
            }
            target = target->next;
        } while (target != NULL);
    }
}

void initialize_candidate_register(CodeMotionCandidate *candidate)
{
    CMRegisterNode *motion;
    PCodeBlock *preheader;
    int offset;
    PCodeInstruction *instruction;
    PCodeInstruction *copy;
    short source_register;
    PCodeInstruction *high_instruction;
    short base_register;
    short high_offset;
    short low_offset;
    short immediate;

    motion = candidate->owner;
    preheader = candidate->owner->loop->preheader;
    if (candidate->source_operand != 0) {
        source_register = candidate->instruction->operandData.operands[candidate->source_operand].value.reg;
        base_register = candidate->instruction->operandData.operands[candidate->base_operand].value.reg;
        instruction = NULL;
        if (motion->reachingDefinition != NULL && motion->reachingDefinition->opcode == PC_LI &&
            motion->reachingDefinition->block == preheader) {
            if (motion->reachingDefinition->operandData.operands[1].value.signed_value == 0) {
                instruction = PCodeUtilities_CreateInstruction(PC_MR, candidate->destination_register, source_register);
            } else {
                immediate = motion->reachingDefinition->operandData.operands[1].value.signed_value;
                if (motion->reachingDefinition->operandData.operands[1].value.signed_value == immediate) {
                    instruction = PCodeUtilities_CreateInstruction(
                        PC_ADDI, candidate->destination_register, source_register, 0,
                        motion->reachingDefinition->operandData.operands[1].value.signed_value);
                }
            }
        }
        if (instruction == NULL) {
            instruction = PCodeUtilities_CreateInstruction(PC_ADD, candidate->destination_register, source_register,
                                                           base_register);
        }
        if (motion->reachingDefinition != NULL && instruction->opcode != PC_ADD) {
            PCode_InsertInstructionAfter(motion->reachingDefinition, instruction);
        } else if (candidate->lastLoop != NULL && candidate->lastLoop->preheader->reverse_instructions != NULL) {
            PCode_InsertInstructionBefore(candidate->lastLoop->preheader->reverse_instructions, instruction);
        } else {
            PCode_InsertInstructionBefore(preheader->reverse_instructions, instruction);
        }
        candidate->replacement_instruction = instruction;
        candidate->source_register = source_register;
        return;
    }
    if (motion->reachingDefinition == NULL || motion->reachingDefinition->opcode != PC_LI) {
        copy = PCode_CloneInstruction(candidate->instruction);
        copy->operandData.operands[0].value.reg = candidate->destination_register;
        PCode_InsertInstructionBefore(preheader->reverse_instructions, copy);
    } else {
        offset = motion->reachingDefinition->operandData.operands[1].value.signed_value * candidate->scale;
        if (offset != (short)offset) {
            high_offset = (offset >> 16) + (offset >> 15 & 1);
            high_instruction =
                PCodeUtilities_CreateInstruction(PC_LIS, candidate->destination_register, 0, high_offset);
            PCode_InsertInstructionAfter(motion->reachingDefinition, high_instruction);
            if ((short)offset != 0) {
                low_offset = offset;
                PCode_InsertInstructionAfter(
                    high_instruction, PCodeUtilities_CreateInstruction(PC_ADDI, candidate->destination_register,
                                                                       candidate->destination_register, 0, low_offset));
            }
        } else {
            PCode_InsertInstructionAfter(
                motion->reachingDefinition,
                PCodeUtilities_CreateInstruction(PC_LI, candidate->destination_register, offset));
        }
    }
}

static void process_searches(Loop *node)
{
    CMRegisterNode *item;
    CodeMotionEntryLink *link;
    int success;
    SInt32 value;
    short operandIndex;
    short otherOperandIndex;
    Loop *lastBlock;
    for (item = node->codeMotionSearches; item != NULL; item = item->next) {
        link = code_motion_register_use_heads[item->reg];
        if (link != NULL) {
            do {
                if ((1 << cm_entries[link->entry_index].instruction->block->index &
                     node->memberblocks[cm_entries[link->entry_index].instruction->block->index >> 5]) != 0) {
                    success = check_strength_reduction_use(item, link->entry_index, &value, &operandIndex,
                                                           &otherOperandIndex, &lastBlock);
                    if (success != 0) {
                        add_code_motion_candidate(item, cm_entries[link->entry_index].instruction, value, operandIndex,
                                                  otherOperandIndex, lastBlock);
                    }
                }
                link = link->next;
            } while (link != NULL);
        }
    }
}

void fn_00527e80(Loop *node)
{
    Loop *subtree, *child;
    while (node != NULL) {
        if (node->children != NULL) {
            subtree = node->children;
            while (subtree != NULL) {
                if (subtree->children != NULL) {
                    child = subtree->children;
                    while (child != NULL) {
                        if (child->children != NULL)
                            fn_00527e80(child->children);
                        if (child->codeMotionSearches != NULL)
                            collect_code_motion_candidates(child);
                        child = child->sibling;
                    }
                }
                if (subtree->codeMotionSearches != NULL)
                    process_searches(subtree);
                subtree = subtree->sibling;
            }
        }
        if (node->codeMotionSearches != NULL)
            process_searches(node);
        node = node->sibling;
    }
}

void collect_code_motion_candidates(Loop *context)
{
    CMRegisterNode *entry;
    PCodeInstruction *record;
    CodeMotionEntryLink *candidate;
    CodeMotionCandidate *link;
    int value14;
    short value18;
    short value1a;
    int value10;
    SInt32 result14;
    short result18;
    short result1a;
    Loop *result10;
    for (entry = context->codeMotionSearches; entry != NULL; entry = entry->next) {
        for (candidate = code_motion_register_use_heads[entry->reg]; candidate != NULL; candidate = candidate->next) {
            if ((1 << cm_entries[candidate->entry_index].instruction->block->index &
                 context->memberblocks[cm_entries[candidate->entry_index].instruction->block->index >> 5]) != 0 &&
                check_strength_reduction_use(entry, candidate->entry_index, &result14, &result18, &result1a,
                                             &result10) != 0) {
                value10 = (int)result10;
                value1a = result1a;
                value18 = result18;
                value14 = result14;
                record = cm_entries[candidate->entry_index].instruction;
                link = (CodeMotionCandidate *)CompilerTools_AllocatePoolMemory(32);
                link->next = entry->candidates;
                entry->candidates = link;
                link->owner = entry;
                link->instruction = record;
                link->replacement_instruction = NULL;
                link->scale = value14;
                link->base_operand = value18;
                link->source_operand = value1a;
                link->lastLoop = (Loop *)value10;
                if ((record->flags & (fIsRead | fIsWrite)) != 0) {
                    link->source_register = 65535;
                } else {
                    link->source_register = record->operandData.operands[0].value.reg;
                }
                link->destination_register = 65535;
            }
        }
    }
    return;
}

void add_code_motion_candidate(CMRegisterNode *list, PCodeInstruction *attributes, unsigned int argumentValue,
                               short firstValue, short secondValue, Loop *thirdValue)
{
    CodeMotionCandidate *record = (CodeMotionCandidate *)CompilerTools_AllocatePoolMemory(0x20);
    record->next = list->candidates;
    list->candidates = record;
    record->owner = list;
    record->instruction = attributes;
    record->replacement_instruction = NULL;
    record->scale = argumentValue;
    record->base_operand = (unsigned short)firstValue;
    record->source_operand = (unsigned short)secondValue;
    record->lastLoop = thirdValue;
    if ((attributes->flags & (fIsRead | fIsWrite)) != 0)
        record->source_register = 0xffff;
    else
        record->source_register = attributes->operandData.operands[0].value.reg;
    record->destination_register = 0xffff;
}

SInt32 check_strength_reduction_use(CMRegisterNode *info, SInt32 useIndex, SInt32 *value, SInt16 *operandIndex,
                                    SInt16 *otherOperandIndex, Loop **lastBlock)
{
    SInt32 finalCount;
    PCodeInstruction *use = cm_entries[useIndex].instruction;
    struct CodeMotionEntryLink *definition;
    struct CodeMotionEntryLink *definitions;
    SInt32 count;
    Loop *previousBlock;
    Loop *block;
    struct CodeMotionEntryLink *node;
    *operandIndex = 0;
    *otherOperandIndex = 0;
    *lastBlock = NULL;
    switch (use->opcode) {
        case PC_MULLI:
            *value = use->operandData.operands[2].value.immediate_value;
            break;
        case PC_RLWINM:
            if (use->operandData.operands[3].value.immediate_value != 0)
                return 0;
            if (use->operandData.operands[2].value.immediate_value > 0xf)
                return 0;
            if (use->operandData.operands[4].value.immediate_value !=
                0x1f - use->operandData.operands[2].value.immediate_value)
                return 0;
            if (use->flags & fRecordBit)
                return 0;
            *value = 1 << use->operandData.operands[2].value.immediate_value;
            break;
        case PC_LBZX:
        case PC_LHZX:
        case PC_LHAX:
        case PC_LWZX:
        case PC_STBX:
        case PC_STHX:
        case PC_STWX:
        case PC_LFSX:
        case PC_LFDX:
        case PC_STFSX:
        case PC_STFDX:
            *operandIndex = 0;
            *otherOperandIndex = 0;
            if (use->operandData.operands[1].value.reg == info->reg) {
                *operandIndex = 1;
                *otherOperandIndex = 2;
            } else if (use->operandData.operands[2].value.reg == info->reg) {
                *operandIndex = 2;
                *otherOperandIndex = 1;
            }
            definition = definitions =
                code_motion_register_definition_heads[use->operandData.operands[*otherOperandIndex].value.reg];
            count = 0;
            for (; definition != NULL; definition = definition->next) {
                PCodeInstruction *entry = code_motion_entries[definition->entry_index].instruction;
                SInt32 registerNumber = entry->block->index;
                if (info->loop->memberblocks[registerNumber >> 5] & (1 << registerNumber))
                    count++;
            }
            if (count != 0)
                return 0;
            previousBlock = info->loop, block = previousBlock->parent;
            while (block != NULL) {
                count = 0;
                for (node = definitions; node; node = node->next) {
                    SInt32 r = code_motion_entries[node->entry_index].instruction->block->index;
                    if (block->memberblocks[r >> 5] & (1 << r))
                        count++;
                }
                if (info->reachingDefinition == NULL) {
                    count++;
                } else {
                    SInt32 registerNumber = info->reachingDefinition->block->index;
                    if (block->memberblocks[registerNumber >> 5] & (1 << registerNumber))
                        count++;
                }
                if (count != 0)
                    break;
                previousBlock = block;
                block = block->parent;
            }
            *lastBlock = previousBlock;
            *value = 1;
            return 1;
        default:
            return 0;
    }
    finalCount = 0;
    for (node = code_motion_register_definition_heads[use->operandData.operands[0].value.reg]; node != NULL;
         node = node->next) {
        PCodeInstruction *entry = code_motion_entries[node->entry_index].instruction;
        SInt32 registerNumber = entry->block->index;
        if (info->loop->memberblocks[registerNumber >> 5] & (1 << registerNumber))
            finalCount++;
    }
    if (finalCount != 1)
        return 0;
    return 1;
}

int visit_loops_children_first(register Loop *loop)
{
    register Loop *child;
    register Loop *grandchild;
    register Loop *greatGrandchild;
    register Loop *fourthLevelLoop;
    register Loop *fifthLevelLoop;
    register Loop *sixthLevelLoop;
    register Loop *seventhLevelLoop;

    for (; loop != NULL; loop = loop->sibling) {
        if (loop->children != NULL) {
            for (child = loop->children; child != NULL; child = child->sibling) {
                if (child->children != NULL) {
                    for (grandchild = child->children; grandchild != NULL; grandchild = grandchild->sibling) {
                        if (grandchild->children != NULL) {
                            for (greatGrandchild = grandchild->children; greatGrandchild != NULL;
                                 greatGrandchild = greatGrandchild->sibling) {
                                if (greatGrandchild->children != NULL) {
                                    for (fourthLevelLoop = greatGrandchild->children; fourthLevelLoop != NULL;
                                         fourthLevelLoop = fourthLevelLoop->sibling) {
                                        if (fourthLevelLoop->children != NULL) {
                                            for (fifthLevelLoop = fourthLevelLoop->children; fifthLevelLoop != NULL;
                                                 fifthLevelLoop = fifthLevelLoop->sibling) {
                                                if (fifthLevelLoop->children != NULL) {
                                                    for (sixthLevelLoop = fifthLevelLoop->children;
                                                         sixthLevelLoop != NULL;
                                                         sixthLevelLoop = sixthLevelLoop->sibling) {
                                                        if (sixthLevelLoop->children != NULL) {
                                                            for (seventhLevelLoop = sixthLevelLoop->children;
                                                                 seventhLevelLoop != NULL;
                                                                 seventhLevelLoop = seventhLevelLoop->sibling) {
                                                                if (seventhLevelLoop->children != NULL) {
                                                                    visit_loops_children_first(
                                                                        seventhLevelLoop->children);
                                                                }
                                                                find_addi_code_motion_candidates(seventhLevelLoop);
                                                            }
                                                        }
                                                        find_addi_code_motion_candidates(sixthLevelLoop);
                                                    }
                                                }
                                                find_addi_code_motion_candidates(fifthLevelLoop);
                                            }
                                        }
                                        find_addi_code_motion_candidates(fourthLevelLoop);
                                    }
                                }
                                find_addi_code_motion_candidates(greatGrandchild);
                            }
                        }
                        find_addi_code_motion_candidates(grandchild);
                    }
                }
                find_addi_code_motion_candidates(child);
            }
        }
        find_addi_code_motion_candidates(loop);
    }
}

static int StrengthDefsValid(Loop *cm, SInt16 reg, SInt32 off)
{
    struct CodeMotionEntryLink *p;
    for (p = code_motion_register_definition_heads[reg]; p != NULL; p = p->next) {
        PCodeInstruction *d = code_motion_entries[p->entry_index].instruction;
        if ((cm->memberblocks[d->block->index >> 5] & (1 << d->block->index)) != 0) {
            if (d->opcode != PC_ADDI)
                return 0;
            if (d->operandData.operands[1].value.reg != reg)
                return 0;
            if (d->operandData.operands[2].value.signed_value != off)
                return 0;
        }
    }
    return 1;
}

void find_addi_code_motion_candidates(Loop *cm)
{
    PCodeBlockLink *o;

    for (o = cm->blocks; o != NULL; o = o->next) {
        PCodeInstruction *d;

        for (d = o->payload.block->instructions; d != NULL; d = d->next) {
            SInt16 reg;
            SInt16 off;

            if (d->opcode == PC_ADDI) {
                reg = d->operandData.operands[0].value.reg;
                if (reg >= 0x20 && d->operandData.operands[1].value.reg == reg) {
                    off = d->operandData.operands[2].value.reg;
                    if (StrengthDefsValid(cm, reg, off))
                        add_code_motion_search(cm, reg, off);
                }
            }
        }
    }
}

void add_code_motion_search(Loop *block, SInt16 reg, SInt32 increment)
{
    CMRegisterNode *node;
    struct CodeMotionEntryLink *defs;
    PCodeInstruction *def;
    CodeMotionRef *ref;

    node = block->codeMotionSearches;
    while (node != NULL) {
        if (node->reg == reg)
            return;
        node = node->next;
    }

    node = (CMRegisterNode *)CompilerTools_AllocatePoolMemory(sizeof(CMRegisterNode));
    node->next = block->codeMotionSearches;
    block->codeMotionSearches = node;
    node->loop = block;
    node->candidates = NULL;
    node->refs = NULL;
    node->increment = increment;
    node->reg = reg;

    for (defs = code_motion_register_definition_heads[reg]; defs != NULL; defs = defs->next) {
        def = code_motion_entries[defs->entry_index].instruction;
        if (block->memberblocks[def->block->index >> 5] & (1 << def->block->index)) {
            ref = (CodeMotionRef *)CompilerTools_AllocatePoolMemory(sizeof(CodeMotionRef));
            ref->next = node->refs;
            node->refs = ref;
            ref->def = def;
        }
    }

    node->reachingDefinition = fn_005288e0(block, reg);
}

PCodeInstruction *fn_005288e0(Loop *search, SInt16 reg)
{
    PCodeInstruction *found;
    PCodeInstruction *instruction;
    UInt32 *definitions;
    struct CodeMotionEntryLink *definition;
    definitions = data_00587fe4[search->body->index].definition_sets[2];
    found = NULL;
    definition = code_motion_register_definition_heads[reg];
    while (definition != NULL) {
        instruction = code_motion_entries[definition->entry_index].instruction;
        if ((search->memberblocks[instruction->block->index >> 5] & (1 << (instruction->block->index & 31))) == 0) {
            if (definitions[definition->entry_index >> 5] & (1 << (definition->entry_index & 31))) {
                if (found != NULL)
                    return NULL;
                found = instruction;
            }
        }
        definition = definition->next;
    }
    if (found != NULL) {
        if (found->opcode == PC_LI || found->opcode == PC_ADDI || found->opcode == PC_ADD)
            return found;
    }
    return NULL;
}
