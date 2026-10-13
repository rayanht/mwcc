#define CERROR_FILE "CodeMotion.c"
#include "compiler/common.h"
#include "compiler/CodeMotion.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BitVectors.h"
#include "compiler/CError.h"
#include "compiler/CRTTI.h"
#include "compiler/CodeGen.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"

struct CodeMotionObjectNode *gCodeMotionAllocationList_005870fc;
struct CodeMotionEntry *code_motion_entries;
struct CodeMotionEntry *cm_entries;
struct CodeMotionEntryLink **register_definition_heads;
struct CodeMotionEntryLink **register_use_entry_heads;
int data_00587e38;
SInt32 codeMotionEntryCount;
struct CodeMotionEntryLink **code_motion_register_definition_heads;
struct CodeMotionEntryLink **codeMotionUseEntryHeads;
struct CodeMotionEntryLink **data_00587f04;
struct CodeMotionEntryLink **code_motion_register_use_heads;
struct CodeMotionDataflowState *data_00587fe4;
struct CodeMotionObjectNode *gCodeMotionObjectTree_005880ac;
int gCodeMotionCounter_005880b8;

#define CM_BIT(set, i) (((UInt32 *)(set))[(i) >> 5] & (1 << (i)))

typedef struct CMOState CMOState;

#define CM_SET(bv, n) ((bv)[(n) >> 5] |= 1 << (n))
#define CM_CLR(bv, n) ((bv)[(n) >> 5] &= ~(1 << ((n) & 31)))
#define CM_TEST(bv, n) ((1 << ((n) & 31)) & (bv)[(n) >> 5])
static inline void CodeMotion_ClearBit524d90(UInt32 *bits, int index)
{
    bits[index >> 5] &= ~(1U << (index & 31));
}

static inline void CodeMotion_SetBit524d90(UInt32 *bits, int index)
{
    bits[index >> 5] |= 1U << (index & 31);
}

static inline struct CodeMotionEntryLink *fn_00526950_inline1(struct CodeMotionEntryLink *a3x, int a0,
                                                              struct CodeMotionEntry *v2)
{
    struct CodeMotionEntryLink *v4;
    v4 = a3x;
    if (v4 != NULL) {
        do {
            if (v4->entry_index != a0 &&
                (1 << v4->entry_index &
                 data_00587fe4[v2->instruction->block->index].definition_sets[2][v4->entry_index >> 5]) != 0) {
                return v4;
            }
            v4 = v4->next;
        } while (v4 != NULL);
    }
    return v4;
}

int is_loop_invariant(PCodeInstruction *instruction, Loop *loop, UInt32 *definitions, SInt32 allow_spr,
                      SInt32 allow_crfield)
{
    PCodeOperand *operand;
    SInt32 remaining;
    SInt32 definition_index;
    SInt32 block_index;
    CodeMotionObjectNode *object_node;
    CodeMotionEntryLink *object_definition;
    CodeMotionEntryLink *register_definition;
    CodeMotionEntryLink *gpr_definition;

    operand = instruction->operandData.operands;
    remaining = instruction->operand_count;
    while (remaining--) {
        switch (operand->kind) {
            case PCOp_MEMORY:
                if (instruction->flags & fIsRead) {
                    if ((operand->object->qual & Q_INLINE_DATA) == 0) {
                        object_node = find_object_node(operand->object);
                        for (object_definition = object_node->definition_entries; object_definition != NULL;
                             object_definition = object_definition->next) {
                            definition_index = object_definition->entry_index;
                            if (definitions[definition_index >> 5] & (1 << definition_index)) {
                                block_index = code_motion_entries[definition_index].instruction->block->index;
                                if (loop->memberblocks[block_index >> 5] & (1 << block_index))
                                    return 0;
                            }
                        }
                    }
                }
                if (instruction->flags & fIsWrite) {
                    block_index = instruction->block->index;
                    if ((loop->backedge_dominators[block_index >> 5] & (1 << block_index)) == 0)
                        return 0;
                }
                break;
            case PCOp_GPR:
                if (operand->flags & 1) {
                    if (operand->value.reg != stack_base_reg && (SInt32)operand->value.reg != 2 &&
                        (SInt32)operand->value.reg != 13) {
                        if (operand->value.reg < 32)
                            return 0;
                        for (gpr_definition = code_motion_register_definition_heads[operand->value.reg];
                             gpr_definition != NULL; gpr_definition = gpr_definition->next) {
                            definition_index = gpr_definition->entry_index;
                            if (definitions[definition_index >> 5] & (1 << definition_index)) {
                                block_index = code_motion_entries[definition_index].instruction->block->index;
                                if (loop->memberblocks[block_index >> 5] & (1 << block_index))
                                    return 0;
                            }
                        }
                    }
                }
                break;
            case PCOp_FPR:
                if (operand->flags & 1) {
                    if (operand->value.reg < 32)
                        return 0;
                    for (register_definition = data_00587f04[operand->value.reg]; register_definition != NULL;
                         register_definition = register_definition->next) {
                        definition_index = register_definition->entry_index;
                        if (definitions[definition_index >> 5] & (1 << definition_index)) {
                            block_index = code_motion_entries[definition_index].instruction->block->index;
                            if (loop->memberblocks[block_index >> 5] & (1 << block_index))
                                return 0;
                        }
                    }
                }
                break;
            case PCOp_VR:
                if (operand->flags & 1) {
                    if (operand->value.reg < 32)
                        return 0;
                    for (register_definition = register_definition_heads[operand->value.reg];
                         register_definition != NULL; register_definition = register_definition->next) {
                        definition_index = register_definition->entry_index;
                        if (definitions[definition_index >> 5] & (1 << definition_index)) {
                            block_index = code_motion_entries[definition_index].instruction->block->index;
                            if (loop->memberblocks[block_index >> 5] & (1 << block_index))
                                return 0;
                        }
                    }
                }
                break;
            case PCOp_SPR:
                if (allow_spr == 0)
                    return 0;
                break;
            case PCOp_CRFIELD:
                if (allow_crfield == 0 || (operand->flags & 2) == 0)
                    return 0;
                break;
        }
        operand++;
    }
    return 1;
}

int is_only_definition_in_loop(PCodeInstruction *def, Loop *ctx)
{
    int index = def->useStart;
    CodeMotionEntry *entry = &code_motion_entries[index];
    CMDefInfo *operand = (CMDefInfo *)&entry->kind;
    CodeMotionEntryLink *definition;
    if (index >= codeMotionEntryCount)
        return 0;
    if (entry->instruction != def)
        return 0;
    if (index + 1 < codeMotionEntryCount && entry[1].instruction == def)
        return 0;
    if (entry->kind == 0) {
        for (definition = code_motion_register_definition_heads[operand->u.reg]; definition != NULL;
             definition = definition->next) {
            SInt32 blockIndex = code_motion_entries[definition->entry_index].instruction->block->index;
            if ((ctx->memberblocks[blockIndex >> 5] & (1 << blockIndex)) != 0 && definition->entry_index != index)
                return 0;
        }
    } else if (entry->kind == 1) {
        for (definition = data_00587f04[operand->u.reg]; definition != NULL; definition = definition->next) {
            SInt32 blockIndex = code_motion_entries[definition->entry_index].instruction->block->index;
            if ((ctx->memberblocks[blockIndex >> 5] & (1 << blockIndex)) != 0 && definition->entry_index != index)
                return 0;
        }
    } else if (entry->kind == 9) {
        for (definition = register_definition_heads[operand->u.reg]; definition != NULL;
             definition = definition->next) {
            SInt32 blockIndex = code_motion_entries[definition->entry_index].instruction->block->index;
            if ((ctx->memberblocks[blockIndex >> 5] & (1 << blockIndex)) != 0 && definition->entry_index != index)
                return 0;
        }
    } else {
        CodeMotionObjectNode *holder = find_object_node(operand->u.object);
        for (definition = holder->definition_entries; definition != NULL; definition = definition->next) {
            SInt32 blockIndex = code_motion_entries[definition->entry_index].instruction->block->index;
            if ((ctx->memberblocks[blockIndex >> 5] & (1 << blockIndex)) != 0 && definition->entry_index != index)
                return 0;
        }
    }
    return 1;
}

int fn_00526950(int itemIndex, int targetIndex)
{
    CMDefInfo *descriptor;
    struct CodeMotionEntry *target;
    struct CodeMotionEntryLink *link;
    CodeMotionEntry *item;
    PCodeInstruction *node;
    CodeMotionObjectNode *owner;
    item = &code_motion_entries[itemIndex];
    target = &cm_entries[targetIndex];
    descriptor = (CMDefInfo *)&item->kind;
    if (item->kind == 0) {
        link = code_motion_register_definition_heads[descriptor->u.reg];
        while (link != NULL) {
            if (link->entry_index != itemIndex &&
                (1 << link->entry_index &
                 data_00587fe4[target->instruction->block->index].definition_sets[2][link->entry_index >> 5]) != 0) {
                break;
            }
            link = link->next;
        }
    } else if (item->kind == 1) {
        link = fn_00526950_inline1(data_00587f04[descriptor->u.reg], itemIndex, target);
    } else if (item->kind == 9) {
        link = fn_00526950_inline1(register_definition_heads[descriptor->u.reg], itemIndex, target);
    } else {
        owner = find_object_node(descriptor->u.object);
        link = owner->definition_entries;
        while (link != NULL) {
            if (link->entry_index != itemIndex &&
                (1 << link->entry_index &
                 data_00587fe4[target->instruction->block->index].definition_sets[2][link->entry_index >> 5]) != 0) {
                break;
            }
            link = link->next;
        }
    }
    if (link == NULL) {
        return 1;
    }
    if (item->instruction->block == target->instruction->block) {
        for (node = target->instruction->previous; node != NULL; node = node->previous) {
            if (node == item->instruction) {
                return 1;
            }
        }
    }
    return 0;
}

int fn_005266e0(int definitionIndex, Loop *ctx)
{
    CMDefInfo *info;
    CodeMotionEntryLink *use;
    CodeMotionEntry *definition;

    definition = &code_motion_entries[definitionIndex];
    info = (CMDefInfo *)&definition->kind;

    if (definition->kind == 0) {
        for (use = code_motion_register_use_heads[info->u.reg]; use != NULL; use = use->next) {
            if (CM_BIT(data_00587fe4[ctx->preheader->index].use_sets[3], use->entry_index) ||
                (CM_BIT(ctx->memberblocks, cm_entries[use->entry_index].instruction->block->index) &&
                 (fn_00526950(definitionIndex, use->entry_index) == 0)))
                return 0;
        }
    } else if (definition->kind == 1) {
        for (use = codeMotionUseEntryHeads[info->u.reg]; use != NULL; use = use->next) {
            if (CM_BIT(data_00587fe4[ctx->preheader->index].use_sets[3], use->entry_index) ||
                (CM_BIT(ctx->memberblocks, cm_entries[use->entry_index].instruction->block->index) &&
                 (fn_00526950(definitionIndex, use->entry_index) == 0)))
                return 0;
        }
    } else if (definition->kind == 9) {
        for (use = register_use_entry_heads[info->u.reg]; use != NULL; use = use->next) {
            if (CM_BIT(data_00587fe4[ctx->preheader->index].use_sets[3], use->entry_index) ||
                (CM_BIT(ctx->memberblocks, cm_entries[use->entry_index].instruction->block->index) &&
                 (fn_00526950(definitionIndex, use->entry_index) == 0)))
                return 0;
        }
    } else {
        CodeMotionObjectNode *objectUses = find_object_node(info->u.object);
        for (use = objectUses->use_entries; use != NULL; use = use->next) {
            if (CM_BIT(data_00587fe4[ctx->preheader->index].use_sets[3], use->entry_index) ||
                (CM_BIT(ctx->memberblocks, cm_entries[use->entry_index].instruction->block->index) &&
                 (fn_00526950(definitionIndex, use->entry_index) == 0)))
                return 0;
        }
    }
    return 1;
}

int has_use_outside_loop_in_preheader_set(PCodeOperand *ref, Loop *info)
{
    UInt32 *blockmask;
    struct CodeMotionEntryLink *use;

    blockmask = data_00587fe4[info->preheader->index].use_sets[3];

    if (ref->kind == PCOp_GPR) {
        for (use = code_motion_register_use_heads[ref->value.reg]; use != NULL; use = use->next) {
            if (CM_BIT(blockmask, use->entry_index) &&
                !CM_BIT(info->memberblocks, cm_entries[use->entry_index].instruction->block->index))
                return 1;
        }
    } else if (ref->kind == PCOp_FPR) {
        for (use = codeMotionUseEntryHeads[ref->value.reg]; use != NULL; use = use->next) {
            if (CM_BIT(blockmask, use->entry_index) &&
                !CM_BIT(info->memberblocks, cm_entries[use->entry_index].instruction->block->index))
                return 1;
        }
    } else if (ref->kind == PCOp_VR) {
        for (use = register_use_entry_heads[ref->value.reg]; use != NULL; use = use->next) {
            if (CM_BIT(blockmask, use->entry_index) &&
                !CM_BIT(info->memberblocks, cm_entries[use->entry_index].instruction->block->index))
                return 1;
        }
    } else {
        CodeMotionObjectNode *object = find_object_node((Object *)ref->value.unsigned_value);
        for (use = object->use_entries; use != NULL; use = use->next) {
            if (CM_BIT(blockmask, use->entry_index) &&
                !CM_BIT(info->memberblocks, cm_entries[use->entry_index].instruction->block->index))
                return 1;
        }
    }
    return 0;
}

/* (whether the instruction's block is in the loop's block_membership set) */
static int CodeMotion_TestBit(PCodeInstruction *instruction, Loop *node)
{
    return node->block_membership[instruction->block->index >> 5] & (1 << instruction->block->index);
}

static int CodeMotion_CanMove(PCodeInstruction *instruction, Loop *node)
{
    if (is_only_definition_in_loop(instruction, node) == 0)
        return 0;
    if (fn_005266e0(instruction->useStart, node) == 0)
        return 0;
    if (!CodeMotion_TestBit(instruction, node) &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[instruction->useStart].kind, node) !=
            0)
        return 0;
    if (instruction->opcode == PC_LI && node->bodySize > 0x19)
        return 0;
    return 1;
}

void move_instruction_to_preheader(PCodeInstruction *instruction, Loop *region)
{
    CMDefInfo *info;
    SInt32 definitionIndex;
    CodeMotionEntry *definition;
    CodeMotionEntryLink *otherDefinition;
    PCodeBlockLink *successor;

    definitionIndex = instruction->useStart;
    definition = &code_motion_entries[definitionIndex];
    info = (CMDefInfo *)&definition->kind;

    PCode_UnlinkInstruction(instruction);
    PCode_InsertInstructionBefore(region->preheader->reverse_instructions, instruction);
    region->bodySize--;
    gCodeMotionChanged = 1;

    if (definition->kind == 0) {
        successor = region->blocks;
        if (successor != NULL) {
            SInt32 wordIndex = definitionIndex >> 5;
            UInt32 mask = 1 << (definitionIndex & 31);
            do {
                for (otherDefinition = code_motion_register_definition_heads[info->u.reg]; otherDefinition != NULL;
                     otherDefinition = otherDefinition->next) {
                    data_00587fe4[successor->payload.block->index]
                        .definition_sets[2][otherDefinition->entry_index >> 5] &=
                        ~(1 << (otherDefinition->entry_index & 31));
                }
                data_00587fe4[successor->payload.block->index].definition_sets[2][wordIndex] |= mask;
                successor = successor->next;
            } while (successor != NULL);
        }
    } else if (definition->kind == 1) {
        successor = region->blocks;
        if (successor != NULL) {
            SInt32 wordIndex = definitionIndex >> 5;
            UInt32 mask = 1 << (definitionIndex & 31);
            do {
                for (otherDefinition = data_00587f04[info->u.reg]; otherDefinition != NULL;
                     otherDefinition = otherDefinition->next) {
                    data_00587fe4[successor->payload.block->index]
                        .definition_sets[2][otherDefinition->entry_index >> 5] &=
                        ~(1 << (otherDefinition->entry_index & 31));
                }
                data_00587fe4[successor->payload.block->index].definition_sets[2][wordIndex] |= mask;
                successor = successor->next;
            } while (successor != NULL);
        }
    } else if (definition->kind == 9) {
        successor = region->blocks;
        if (successor != NULL) {
            SInt32 wordIndex = definitionIndex >> 5;
            UInt32 mask = 1 << (definitionIndex & 31);
            do {
                for (otherDefinition = register_definition_heads[info->u.reg]; otherDefinition != NULL;
                     otherDefinition = otherDefinition->next) {
                    data_00587fe4[successor->payload.block->index]
                        .definition_sets[2][otherDefinition->entry_index >> 5] &=
                        ~(1 << (otherDefinition->entry_index & 31));
                }
                data_00587fe4[successor->payload.block->index].definition_sets[2][wordIndex] |= mask;
                successor = successor->next;
            } while (successor != NULL);
        }
    } else {
        CodeMotionObjectNode *registerInfo = find_object_node(info->u.object);
        successor = region->blocks;
        if (successor != NULL) {
            SInt32 wordIndex = definitionIndex >> 5;
            UInt32 mask = 1 << (definitionIndex & 31);
            do {
                for (otherDefinition = registerInfo->definition_entries; otherDefinition != NULL;
                     otherDefinition = otherDefinition->next) {
                    data_00587fe4[successor->payload.block->index]
                        .definition_sets[2][otherDefinition->entry_index >> 5] &=
                        ~(1 << (otherDefinition->entry_index & 31));
                }
                data_00587fe4[successor->payload.block->index].definition_sets[2][wordIndex] |= mask;
                successor = successor->next;
            } while (successor != NULL);
        }
    }
}

SInt32 fn_00526070(PCodeInstruction *definition, Loop *context)
{
    CodeMotionEntry *entry;
    PCodeOperand *registerInfo;
    CodeMotionEntryLink *registerDefinition;
    PCodeInstruction *relatedDefinition;
    int definitionIndex;
    int relatedIndex;
    SInt32 bit;

    definitionIndex = definition->useStart;
    relatedIndex = definition->next->useStart;
    entry = &code_motion_entries[definitionIndex];
    registerInfo = (PCodeOperand *)&entry->kind;
    if (definitionIndex >= codeMotionEntryCount)
        return 0;
    if (entry->instruction != definition)
        return 0;
    if (definitionIndex + 1 < codeMotionEntryCount && entry[1].instruction == definition)
        return 0;
    if (entry->kind == 0) {
        for (registerDefinition = code_motion_register_definition_heads[registerInfo->value.reg];
             registerDefinition != NULL; registerDefinition = registerDefinition->next) {
            bit = code_motion_entries[registerDefinition->entry_index].instruction->block->index;
            if (((1 << (bit & 0x1f)) & context->memberblocks[bit >> 5]) != 0 &&
                registerDefinition->entry_index != definitionIndex && registerDefinition->entry_index != relatedIndex)
                return 0;
        }
    } else {
        CError_FATAL(572);
    }
    if (fn_005266e0(definition->useStart, context) == 0)
        return 0;
    if (!CodeMotion_TestBit(definition, context) &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[definition->useStart].kind,
                                              context) != 0)
        return 0;
    if (!CodeMotion_TestBit(relatedDefinition = definition->next, context) &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[definition->next->useStart].kind,
                                              context) != 0)
        return 0;
    return 1;
}

static struct PCodeInstruction *data_00574ce8 = NULL;

unsigned int fn_00525fc0(PCodeInstruction *node, Loop *loop, UInt32 *defs)
{
    PCodeInstruction *next = node->next;

    if (node->opcode == PC_ADDZE && data_00574ce8 == node) {
        data_00574ce8 = NULL;
        return 1;
    }
    if (node->opcode == PC_SRAWI && next != NULL && next->opcode == PC_ADDZE &&
        node->operandData.operands[0].value.reg == next->operandData.operands[0].value.reg &&
        next->operandData.operands[0].value.reg == next->operandData.operands[1].value.reg &&
        (node->flags & 0x20460) == 0 && (next->flags & 0x20460) == 0) {
        if (is_loop_invariant(node, loop, defs, 1, 0)) {
            if (fn_00526070(node, loop)) {
                data_00574ce8 = next;
                return 1;
            }
        }
    }
    data_00574ce8 = NULL;
    return 0;
}

static void remove_block(Loop *co, PCodeBlock *nb)
{
    PCodeBlockLink **pp;
    PCodeBlockLink *l;

    CM_CLR(co->memberblocks, nb->index);
    CM_CLR(co->exitblocks, nb->index);
    CM_CLR(co->block_membership, nb->index);
    CM_CLR(co->backedge_dominators, nb->index);
    co->bodySize -= nb->instruction_count;
    pp = &co->blocks;
    while ((l = *pp) != NULL) {
        if (l->payload.block == nb)
            *pp = l->next;
        else
            pp = &l->next;
    }
}

void replace_successor(PCodeBlock *block, PCodeBlock *oldSuccessor, PCodeBlock *newSuccessor)
{
    PCodeBlockLink *successor;
    PCodeBlockLink **predecessorLink;
    PCodeBlockLink *predecessor;

    successor = block->successors;
    while (successor != NULL) {
        if (successor->payload.block == oldSuccessor) {
            successor->payload.block = newSuccessor;
        }
        successor = successor->next;
    }
    predecessorLink = &oldSuccessor->predecessors;
    while ((predecessor = *predecessorLink) != NULL) {
        if (predecessor->payload.block == block) {
            *predecessorLink = predecessor->next;
            predecessor->next = newSuccessor->predecessors;
            newSuccessor->predecessors = predecessor;
        } else {
            predecessorLink = &predecessor->next;
        }
    }
}

void fn_00525f20(PCodeBlock *replacement, PCodeBlock *block, PCodeBlock *successor)
{
    PCodeBlockLink *predecessor;
    PCodeBlockLink **link;
    PCodeBlockLink *edge;

    for (predecessor = successor->predecessors; predecessor != NULL; predecessor = predecessor->next) {
        if (predecessor->payload.block == block)
            predecessor->payload.block = replacement;
    }

    link = &block->successors;
    while ((edge = *link) != NULL) {
        if (edge->payload.block == successor) {
            *link = edge->next;
            edge->next = replacement->successors;
            replacement->successors = edge;
        } else {
            link = &edge->next;
        }
    }
}

void CodeMotion_00525e70(Loop *region, PCodeBlock *block, PCodeInstruction *instruction, PCodeInstruction *replacement,
                         PCodeOperand *argument)
{
    PCodeBlock *oldRegion = region->preheader;
    PCodeInstruction *terminator;

    if (instruction->flags & fRecordBit) {
        move_instruction_to_preheader(instruction, region);
    } else {
        PCode_UnlinkInstruction(instruction);
        PCode_InsertInstructionBefore(region->preheader->reverse_instructions, instruction);
        region->bodySize--;
        gCodeMotionChanged = 1;
    }
    region->preheader = NULL;
    LoopDetection_CreatePreheader(region);
    terminator = oldRegion->reverse_instructions;
    CError_ASSERT(765, terminator->opcode == PC_B);
    PCode_UnlinkInstruction(terminator);
    PCode_UnlinkInstruction(replacement);
    PCode_AppendInstruction(oldRegion, replacement);
    fn_00525f20(oldRegion, block, argument->value.label->target.block);
}

PCodeBlock *clone_block_with_bridge(Loop *region, PCodeBlock *insertionPoint, PCodeBlock *source,
                                    PCodeBlock *destination)
{
    PCodeBlock *cloneBlock;
    PCodeBlock *bridgeBlock;
    PCodeBlockLink *link;
    PCodeBlockLink *successor;
    PCodeInstruction *instruction;

    cloneBlock = lalloc(sizeof(PCodeBlock));
    bridgeBlock = lalloc(sizeof(PCodeBlock));

    cloneBlock->labels = NULL;
    cloneBlock->predecessors = cloneBlock->successors = NULL;
    cloneBlock->instructions = cloneBlock->reverse_instructions = NULL;
    cloneBlock->line = -1;
    cloneBlock->instruction_count = 0;
    cloneBlock->execution_weight = region->body->execution_weight;
    cloneBlock->flags = 0;
    cloneBlock->index = gPCodeBlockCount++;

    bridgeBlock->labels = NULL;
    bridgeBlock->predecessors = bridgeBlock->successors = NULL;
    bridgeBlock->instructions = bridgeBlock->reverse_instructions = NULL;
    bridgeBlock->line = -1;
    bridgeBlock->instruction_count = 0;
    bridgeBlock->execution_weight = region->body->execution_weight;
    bridgeBlock->flags = 0;
    bridgeBlock->index = gPCodeBlockCount++;

    cloneBlock->next = bridgeBlock;
    bridgeBlock->prev = cloneBlock;
    cloneBlock->prev = insertionPoint;
    bridgeBlock->next = insertionPoint->next;
    insertionPoint->next = cloneBlock;
    bridgeBlock->next->prev = bridgeBlock;

    PCode_ResolveLabel(cloneBlock, PCode_NewLabel());
    PCode_ResolveLabel(bridgeBlock, PCode_NewLabel());

    successor = insertionPoint->successors;
    replace_successor(insertionPoint, successor->payload.block, cloneBlock);

    link = lalloc(sizeof(PCodeBlockLink));
    link->payload.block = bridgeBlock;
    link->next = cloneBlock->successors;
    cloneBlock->successors = link;

    link = lalloc(sizeof(PCodeBlockLink));
    link->payload.block = cloneBlock;
    link->next = bridgeBlock->predecessors;
    bridgeBlock->predecessors = link;

    PCode_AppendInstruction(bridgeBlock, PCodeUtilities_CreateInstruction(0, source->next->labels));
    PCode_AddSuccessor(bridgeBlock, source->next->labels);
    LoopOptimization_AddMissingSuccessorPredecessors(bridgeBlock);

    for (instruction = source->instructions; instruction != NULL; instruction = instruction->next)
        PCode_AppendInstruction(cloneBlock, PCode_CloneInstruction(instruction));

    PCode_AddSuccessor(cloneBlock, destination->labels);

    link = lalloc(sizeof(PCodeBlockLink));
    link->payload.block = cloneBlock;
    link->next = destination->predecessors;
    destination->predecessors = link;

    LoopDetection_AddBlock(region, cloneBlock);

    if (region->block_membership[source->index >> 5] & (1 << (source->index & 31)))
        region->block_membership[cloneBlock->index >> 5] |= 1 << (cloneBlock->index & 31);
    if (region->backedge_dominators[source->index >> 5] & (1 << (source->index & 31)))
        region->backedge_dominators[cloneBlock->index >> 5] |= 1 << (cloneBlock->index & 31);

    for (region = region->parent; region != NULL; region = region->parent) {
        LoopDetection_AddBlock(region, cloneBlock);
        if (region->block_membership[source->index >> 5] & (1 << (source->index & 31)))
            region->block_membership[cloneBlock->index >> 5] |= 1 << (cloneBlock->index & 31);
        if (region->backedge_dominators[source->index >> 5] & (1 << (source->index & 31)))
            region->backedge_dominators[cloneBlock->index >> 5] |= 1 << (cloneBlock->index & 31);
        LoopDetection_AddBlock(region, bridgeBlock);
        if (region->block_membership[source->index >> 5] & (1 << (source->index & 31)))
            region->block_membership[bridgeBlock->index >> 5] |= 1 << (bridgeBlock->index & 31);
        if (region->backedge_dominators[source->index >> 5] & (1 << (source->index & 31)))
            region->backedge_dominators[bridgeBlock->index >> 5] |= 1 << (bridgeBlock->index & 31);
    }

    return cloneBlock;
}

PCodeBlockLink *collect_single_successor_memberblocks(Loop *cm, PCodeBlock *cur)
{
    PCodeBlockLink *result = NULL;
    PCodeBlockLink *tail = NULL;
    PCodeBlock *b = cur;
    while (b != NULL && b != cm->body) {
        if (!((1 << (b->index & 31)) & cm->memberblocks[b->index >> 5]))
            return NULL;
        if (b->successors != NULL && b->successors->next != NULL)
            return NULL;
        {
            PCodeBlockLink *node = (PCodeBlockLink *)oalloc(8);
            node->payload.block = b;
            node->next = NULL;
            if (result != NULL)
                tail->next = node;
            else
                result = node;
            tail = node;
        }
        b = b->successors->payload.block;
    }
    return result;
}

void unswitch_loop(Loop *loop)
{
    UInt32 *definitions;
    PCodeInstruction *candidate;
    PCodeInstruction *terminator;
    PCodeOperand *target;
    PCodeBlockLink *fallthroughPath;
    PCodeBlockLink *branchPath;
    PCodeBlockLink *common;
    PCodeBlockLink *previous;
    PCodeBlockLink *path;
    PCodeBlockLink *otherPath;
    Loop *newLoop;
    PCodeInstruction *instruction;
    PCodeInstruction *firstInstruction;
    PCodeInstruction *sourceInstruction;
    PCodeBlock *block;
    PCodeBlock *preheader;
    PCodeBlock *newBlock;
    int i;

    if (loop->body->reverse_instructions == NULL)
        return;
    if (loop->body->reverse_instructions->opcode != PC_BT && loop->body->reverse_instructions->opcode != PC_BF)
        return;
    if (CM_TEST(loop->memberblocks,
                loop->body->reverse_instructions->operandData.operands[2].value.label->target.block->index) == 0)
        return;
    if (loop->has_memory_barrier != 0)
        return;
    if (loop->has_call != 0)
        return;
    if (CM_TEST(loop->memberblocks, loop->body->next->index) != 0)
        return;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (CM_TEST(loop->memberblocks, block->index))
            break;
    }
    if (block == NULL)
        return;

    definitions = oalloc(((codeMotionEntryCount + 31) >> 5) * sizeof(*definitions));
    CodeMotion_AllocateBits(definitions, data_00587fe4[block->index].definition_sets[2], codeMotionEntryCount);

    for (candidate = loop->preheader->next->instructions; candidate != NULL; candidate = candidate->next) {
        if ((candidate->flags & 0x204e0) == 0) {
            if (is_loop_invariant(candidate, loop, definitions, 0, 1) != 0)
                break;
        }
    }
    if (candidate == NULL || candidate->operand_count < 1)
        return;
    if (candidate->operand_count < 1 || candidate->operandData.operands[0].kind != PCOp_CRFIELD)
        return;

    terminator = candidate->block->reverse_instructions;
    if (terminator == NULL || (terminator->flags & fIsBranch) == 0 ||
        terminator->operandData.operands[0].kind != PCOp_CRFIELD)
        return;
    if (terminator->operandData.operands[0].value.reg != candidate->operandData.operands[0].value.reg)
        return;

    target = NULL;
    for (i = 0; i < terminator->operand_count; i++) {
        if (terminator->operandData.operands[i].kind == PCOp_LABEL)
            target = &terminator->operandData.operands[i];
    }
    if (target != NULL) {
        preheader = loop->preheader;
        fallthroughPath = collect_single_successor_memberblocks(loop, block->next);
        if (fallthroughPath == NULL)
            return;
        branchPath = collect_single_successor_memberblocks(loop, target->value.label->target.block);
        if (branchPath == NULL)
            return;

        common = NULL;
        previous = NULL;
        for (path = fallthroughPath; path != NULL; path = path->next) {
            for (otherPath = branchPath; otherPath != NULL; otherPath = otherPath->next) {
                if (path->payload.block == otherPath->payload.block) {
                    common = path;
                    break;
                }
            }
            if (common != NULL)
                break;
            previous = path;
        }
        CError_ASSERT(1181, previous->payload.block != NULL);

        if (previous->payload.block->reverse_instructions != NULL &&
            previous->payload.block->reverse_instructions->opcode == PC_B)
            PCode_UnlinkInstruction(previous->payload.block->reverse_instructions);

        for (path = common; path != NULL; path = path->next) {
            for (sourceInstruction = path->payload.block->instructions; sourceInstruction != NULL;
                 sourceInstruction = sourceInstruction->next) {
                if (sourceInstruction->opcode != PC_B)
                    PCode_AppendInstruction(previous->payload.block, PCode_CloneInstruction(sourceInstruction));
            }
        }

        newBlock = clone_block_with_bridge(loop, previous->payload.block, loop->body, block);
        CodeMotion_00525e70(loop, block, candidate, terminator, target);

        if (block->instruction_count != 0) {
            if ((firstInstruction = branchPath->payload.block->instructions) != NULL) {
                for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
                    if (instruction->opcode != PC_B)
                        PCode_InsertInstructionBefore(firstInstruction, PCode_CloneInstruction(instruction));
                }
            } else {
                for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
                    if (instruction->opcode != PC_B)
                        PCode_AppendInstruction(branchPath->payload.block, PCode_CloneInstruction(instruction));
                }
            }
        }

        target = NULL;
        for (i = 0; i < loop->body->reverse_instructions->operand_count; i++) {
            if (loop->body->reverse_instructions->operandData.operands[i].kind == PCOp_LABEL)
                target = &loop->body->reverse_instructions->operandData.operands[i];
        }
        if (target == NULL)
            CError_FATAL(1239);
        replace_successor(loop->body, target->value.label->target.block, branchPath->payload.block);
        target->value.label = branchPath->payload.block->labels;

        target = NULL;
        for (i = 0; i < preheader->reverse_instructions->operand_count; i++) {
            if (preheader->reverse_instructions->operandData.operands[i].kind == PCOp_LABEL)
                target = &preheader->reverse_instructions->operandData.operands[i];
        }
        if (target == NULL)
            CError_FATAL(1256);
        replace_successor(preheader, target->value.label->target.block, loop->body);
        target->value.label = loop->body->labels;

        target = NULL;
        for (i = 0; i < loop->preheader->reverse_instructions->operand_count; i++) {
            if (loop->preheader->reverse_instructions->operandData.operands[i].kind == PCOp_LABEL)
                target = &loop->preheader->reverse_instructions->operandData.operands[i];
        }
        if (target == NULL)
            CError_FATAL(1273);
        replace_successor(loop->preheader, target->value.label->target.block, newBlock);
        target->value.label = newBlock->labels;

        newLoop = lalloc(sizeof(*newLoop));
        newLoop->parent = loop->parent;
        newLoop->children = NULL;
        newLoop->sibling = loop->sibling;
        loop->sibling = newLoop;
        newLoop->body = loop->body;
        newLoop->preheader = NULL;
        newLoop->blocks = NULL;
        newLoop->codeMotionSearches = NULL;
        newLoop->footer = NULL;
        newLoop->inductionUpdate = NULL;
        newLoop->execution_weight = loop->execution_weight;

        BitVectors_FillWords(newLoop->memberblocks =
                                 lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->memberblocks)),
                             data_005871a4, 0);
        BitVectors_FillWords(newLoop->exitblocks = lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->exitblocks)),
                             data_005871a4, 0);
        BitVectors_FillWords(newLoop->block_membership =
                                 lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->block_membership)),
                             data_005871a4, 0);
        BitVectors_FillWords(newLoop->backedge_dominators =
                                 lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->backedge_dominators)),
                             data_005871a4, 0);

        remove_block(loop, newLoop->body);

        LoopDetection_AddBlock(newLoop, newLoop->body);
        CM_SET(newLoop->exitblocks, newLoop->body->index);
        CM_SET(newLoop->backedge_dominators, newLoop->body->index);
        CM_SET(newLoop->block_membership, newLoop->body->index);

        for (path = branchPath; path != NULL; path = path->next) {
            remove_block(loop, path->payload.block);
            LoopDetection_AddBlock(newLoop, path->payload.block);
            CM_SET(newLoop->backedge_dominators, path->payload.block->index);
        }

        newLoop->preheader = NULL;
        LoopDetection_CreatePreheader(newLoop);
        LoopDetection_ComputeLoopProperties(newLoop);
        loop->body = newBlock;

        for (path = loop->blocks; path != NULL; path = path->next)
            CM_SET(loop->backedge_dominators, path->payload.block->index);

        CM_SET(loop->exitblocks, newBlock->index);
        LoopDetection_ComputeLoopProperties(loop);
        gCodeMotionCounter_005880b8 = 1;
    }
}

void visit_leaf_loops(Loop *p)
{
    while (p) {
        if (p->children)
            visit_leaf_loops(p->children);
        else if (!p->skip_leaf_pass_4f)
            unswitch_loop(p);
        p = p->sibling;
    }
}

void move_instructions_to_preheader(Loop *node)
{
    PCodeBlockLink *block_link;
    PCodeBlock *block;
    PCodeInstruction *instruction;
    PCodeInstruction *next_instruction;
    UInt32 *available_definitions;
    const UInt32 *definitions;
    CodeMotionEntry *entry;
    CodeMotionEntryLink *link;
    int definition_index;
    int changed;

    available_definitions = (UInt32 *)oalloc(((codeMotionEntryCount + 0x1f) >> 5) << 2);
    do {
        changed = 0;
        for (block_link = node->blocks; block_link != NULL; block_link = block_link->next) {
            block = block_link->payload.block;
            definitions = data_00587fe4[block->index].definition_sets[2];
            CodeMotion_AllocateBits(available_definitions, definitions, codeMotionEntryCount);
            for (instruction = block->instructions; instruction != NULL; instruction = next_instruction) {
                next_instruction = instruction->next;
                if ((instruction->flags & PCodeInstruction_SkipCodeMotion) != 0)
                    continue;
                if (instruction->operand_count == 0)
                    continue;
                if ((instruction->flags & 0x00020460) == 0 &&
                    is_loop_invariant(instruction, node, available_definitions, 0, 0) != 0 &&
                    CodeMotion_CanMove(instruction, node)) {
                    move_instruction_to_preheader(instruction, node);
                    changed = 1;
                } else if (fn_00525fc0(instruction, node, available_definitions) != 0) {
                    move_instruction_to_preheader(instruction, node);
                    changed = 1;
                }
                for (entry = &code_motion_entries[definition_index = instruction->useStart];
                     definition_index < codeMotionEntryCount && entry->instruction == instruction;
                     entry++, definition_index++) {
                    if (entry->kind == 0) {
                        for (link = code_motion_register_definition_heads[entry->value.reg]; link != NULL;
                             link = link->next)
                            CodeMotion_ClearBit524d90(available_definitions, link->entry_index);
                    } else if (entry->kind == 1) {
                        for (link = data_00587f04[entry->value.reg]; link != NULL; link = link->next)
                            CodeMotion_ClearBit524d90(available_definitions, link->entry_index);
                    } else if (entry->kind == 9) {
                        for (link = register_definition_heads[entry->value.reg]; link != NULL; link = link->next)
                            CodeMotion_ClearBit524d90(available_definitions, link->entry_index);
                    } else if (entry->is_implicit == 0) {
                        CodeMotionObjectNode *object_node = find_object_node(entry->value.object);

                        for (link = object_node->definition_entries; link != NULL; link = link->next)
                            CodeMotion_ClearBit524d90(available_definitions, link->entry_index);
                    }
                    CodeMotion_SetBit524d90(available_definitions, definition_index);
                }
            }
        }
    } while (changed);
}

void visit_loops_postorder(Loop *loop)
{
    for (; loop; loop = loop->sibling) {
        if (loop->children)
            visit_loops_postorder(loop->children);
        move_instructions_to_preheader(loop);
    }
}

void CodeMotion_VisitLoops(void)
{
    gCodeMotionCounter_005880b8 = 0;
    gCodeMotionChanged = 0;
    if (data_0058763c != NULL) {
        visit_loops_postorder(data_0058763c);
        visit_leaf_loops(data_0058763c);
    }
    freeoheap();
}
