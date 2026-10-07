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

static struct PCodeInstruction *data_00574ce8 = NULL;

#define CM_BIT(set, i) (((UInt32 *)(set))[(i) >> 5] & (1 << (i)))

typedef struct CMOState CMOState;

#define CM_SET(bv, n) ((bv)[(n) >> 5] |= 1 << (n))
#define CM_CLR(bv, n) ((bv)[(n) >> 5] &= ~(1 << ((n) & 31)))
#define CM_TEST(bv, n) ((1 << ((n) & 31)) & (bv)[(n) >> 5])
static UInt32 *CodeMotion_NewBits(int bit_count)
{
    return oalloc(((bit_count + 31) >> 5) * sizeof(unsigned int));
}

static CodeMotionObjectNode *FindNode(CodeMotionObjectNode *p, Object *key)
{
    while (p != NULL) {
        if (key < p->object) {
            p = p->left;
        } else if (key > p->object) {
            p = p->right;
        } else {
            return p;
        }
    }
    return NULL;
}

static int CodeMotion_TestBit(const UInt32 *bits, int index)
{
    return (bits[index >> 5] & (1U << (31 & index))) != 0;
}

static void CodeMotion_ClearBit(UInt32 *bits, int index)
{
    bits[index >> 5] &= ~(1U << (index & 31));
}

static void CodeMotion_SetBit(UInt32 *bits, int index)
{
    bits[index >> 5] |= 1U << (index & 31);
}

static void CodeMotion_LinkEntry(CodeMotionEntryLink **head, int entry_index)
{
    CodeMotionEntryLink *link = (CodeMotionEntryLink *)oalloc(sizeof(*link));

    link->entry_index = entry_index;
    link->next = *head;
    *head = link;
}

static CodeMotionEntryLink **CodeMotion_RegisterEntryHead(unsigned char kind, short reg, int is_definition)
{
    if (kind == 0) {
        return is_definition ? &code_motion_register_definition_heads[reg] : &code_motion_register_use_heads[reg];
    }
    if (kind == 9) {
        return is_definition ? &register_definition_heads[reg] : &register_use_entry_heads[reg];
    }
    return is_definition ? &data_00587f04[reg] : &codeMotionUseEntryHeads[reg];
}

static void CodeMotion_SetExplicitEntry(CodeMotionEntry *entry, PCodeInstruction *instruction, PCodeOperand *operand)
{
    entry->instruction = instruction;
    entry->kind = operand->kind;
    entry->value.reg = operand->value.reg;
}

static void CodeMotion_SetObjectEntry(CodeMotionEntry *entry, PCodeInstruction *instruction, Object *object,
                                      int is_implicit)
{
    entry->instruction = instruction;
    entry->kind = 5;
    entry->is_implicit = is_implicit;
    entry->value.object = object;
}

static CodeMotionObjectNode *CodeMotion_FindObjectNode(Object *object)
{
    CodeMotionObjectNode *node = gCodeMotionObjectTree_005880ac;

    while (node != NULL) {
        if (object < node->object) {
            node = node->left;
        } else if (object > node->object) {
            node = node->right;
        } else {
            return node;
        }
    }
    return NULL;
}

static Object *CMIdentity(Object *o)
{
    return o;
}

static unsigned char CMKind(Object *p)
{
    return p->datatype;
}

static inline void CMInit(void)
{
    int count;
    cm_entries = oalloc(data_00587e38 * sizeof(CodeMotionEntry));
    code_motion_entries = oalloc(codeMotionEntryCount * sizeof(CodeMotionEntry));
    code_motion_register_use_heads = (void *)oalloc(gUsedVirtualRegistersGPR * sizeof(CodeMotionEntryLink *));
    code_motion_register_definition_heads = (void *)oalloc(gUsedVirtualRegistersGPR * sizeof(CodeMotionEntryLink *));
    for (count = 0; count < gUsedVirtualRegistersGPR; count++) {
        code_motion_register_use_heads[count] = NULL;
        code_motion_register_definition_heads[count] = NULL;
    }
    codeMotionUseEntryHeads = (void *)oalloc(gUsedVirtualRegistersFPR * sizeof(CodeMotionEntryLink *));
    data_00587f04 = (void *)oalloc(gUsedVirtualRegistersFPR * sizeof(CodeMotionEntryLink *));
    for (count = 0; count < gUsedVirtualRegistersFPR; count++) {
        codeMotionUseEntryHeads[count] = NULL;
        data_00587f04[count] = NULL;
    }
    register_use_entry_heads = (void *)oalloc(gUsedVirtualRegistersVR * sizeof(CodeMotionEntryLink *));
    register_definition_heads = (void *)oalloc(gUsedVirtualRegistersVR * sizeof(CodeMotionEntryLink *));
    for (count = 0; count < gUsedVirtualRegistersVR; count++) {
        register_use_entry_heads[count] = NULL;
        register_definition_heads[count] = NULL;
    }
}

static int CodeMotion_HasDefinitionInNode(CodeMotionEntryLink *entries, int current_index, Loop *node)
{
    for (; entries != NULL; entries = entries->next) {
        int entry_index = entries->entry_index;
        PCodeBlock *block = code_motion_entries[entry_index].instruction->block;

        if (CodeMotion_TestBit(node->memberblocks, block->index) && entry_index != current_index) {
            return 1;
        }
    }
    return 0;
}

static int CodeMotion_CanMove(PCodeInstruction *instruction, Loop *node)
{
    if (is_only_definition_in_loop(instruction, node) == 0)
        return 0;
    if (fn_005266e0(instruction->useStart, node) == 0)
        return 0;
    if (!CodeMotion_TestBit(node->block_membership, instruction->block->index) &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[instruction->useStart].kind, node) !=
            0)
        return 0;
    if (instruction->opcode == PC_LI && node->bodySize > 0x19)
        return 0;
    return 1;
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

static inline void CodeMotion_ClearBit524d90(UInt32 *bits, int index)
{
    bits[index >> 5] &= ~(1U << (index & 31));
}

static inline void CodeMotion_SetBit524d90(UInt32 *bits, int index)
{
    bits[index >> 5] |= 1U << (index & 31);
}

static inline int CodeMotion_CanMove524d90(PCodeInstruction *instruction, Loop *node)
{
    if (is_only_definition_in_loop(instruction, node) == 0)
        return 0;
    if (fn_005266e0(instruction->useStart, node) == 0)
        return 0;
    if (!(node->block_membership[instruction->block->index >> 5] & (1U << (instruction->block->index & 31))) &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[instruction->useStart].kind, node) !=
            0)
        return 0;
    if (instruction->opcode == PC_LI && node->bodySize > 0x19)
        return 0;
    return 1;
}

static inline SInt32 CodeMotionStructureKind(TypeStruct *type)
{
    return type->stype;
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
    bit = definition->block->index;
    if (((1 << (bit & 0x1f)) & context->block_membership[bit >> 5]) == 0 &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[definition->useStart].kind,
                                              context) != 0)
        return 0;
    bit = (relatedDefinition = definition->next)->block->index;
    if (((1 << (bit & 0x1f)) & context->block_membership[bit >> 5]) == 0 &&
        has_use_outside_loop_in_preheader_set((PCodeOperand *)&code_motion_entries[definition->next->useStart].kind,
                                              context) != 0)
        return 0;
    return 1;
}

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

        CRTTI_FillWords(newLoop->memberblocks = lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->memberblocks)),
                        data_005871a4, 0);
        CRTTI_FillWords(newLoop->exitblocks = lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->exitblocks)),
                        data_005871a4, 0);
        CRTTI_FillWords(newLoop->block_membership =
                            lalloc(((data_005871a4 + 31) >> 5) * sizeof(*newLoop->block_membership)),
                        data_005871a4, 0);
        CRTTI_FillWords(newLoop->backedge_dominators =
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
                    CodeMotion_CanMove524d90(instruction, node)) {
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

void visit_loops_postorder(register Loop *loop)
{
    register Loop *child;
    register Loop *grandchild;
    register Loop *thirdLevelLoop;
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
                            for (thirdLevelLoop = grandchild->children; thirdLevelLoop != NULL;
                                 thirdLevelLoop = thirdLevelLoop->sibling) {
                                if (thirdLevelLoop->children != NULL) {
                                    for (fourthLevelLoop = thirdLevelLoop->children; fourthLevelLoop != NULL;
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
                                                                    visit_loops_postorder(seventhLevelLoop->children);
                                                                }
                                                                move_instructions_to_preheader(seventhLevelLoop);
                                                            }
                                                        }
                                                        move_instructions_to_preheader(sixthLevelLoop);
                                                    }
                                                }
                                                move_instructions_to_preheader(fifthLevelLoop);
                                            }
                                        }
                                        move_instructions_to_preheader(fourthLevelLoop);
                                    }
                                }
                                move_instructions_to_preheader(thirdLevelLoop);
                            }
                        }
                        move_instructions_to_preheader(grandchild);
                    }
                }
                move_instructions_to_preheader(child);
            }
        }
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

#pragma auto_inline off

CodeMotionObjectNode *find_object_node(Object *object)
{
    CodeMotionObjectNode *node = gCodeMotionObjectTree_005880ac;

    while (node != NULL) {
        if (object < node->object) {
            node = node->left;
        } else if (object > node->object) {
            node = node->right;
        } else {
            return node;
        }
    }
    return NULL;
}

#pragma auto_inline reset

void COpt_00524b20(Object *object)
{
    CodeMotionObjectNode **link = &gCodeMotionObjectTree_005880ac;
    CodeMotionObjectNode *node;

    while ((node = *link) != NULL) {
        if (object < node->object) {
            link = &node->left;
        } else if (object > node->object) {
            link = &node->right;
        } else {
            return;
        }
    }

    node = (CodeMotionObjectNode *)oalloc(sizeof(CodeMotionObjectNode));
    node->right = NULL;
    node->left = node->right;
    node->object = object;
    node->definition_entries = NULL;
    node->use_entries = node->definition_entries;
    node->allocation_next = gCodeMotionAllocationList_005870fc;
    gCodeMotionAllocationList_005870fc = node;
    *link = node;
}

int is_object_access_compatible(PCodeInstruction *instruction, Object *object)
{
    Type *type = object->type;

    if (object->datatype != DDATA && !PCodeUtilities_Require(object) &&
        (object->datatype != DLOCAL || (object->u.var.info->noregister == 0 && object->type->type != TYPEARRAY &&
                                        !(object->type->type == TYPESTRUCT || object->type->type == TYPECLASS) &&
                                        (object->type->type != TYPEMEMBERPOINTER || object->type->size != 12)))) {
        return 0;
    }

    switch (instruction->opcode) {
        case PC_LFS:
        case PC_LFSU:
        case PC_LFSX:
        case PC_LFSUX:
        case PC_STFS:
        case PC_STFSU:
        case PC_STFSX:
        case PC_STFSUX:
            while (type->type == TYPEARRAY) {
                type = TPTR_TARGET(type);
            }
            if (type->type == TYPEFLOAT && type->size == 4) {
                return 1;
            }
            if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && type->size >= 4) {
                return 1;
            }
            return 0;

        case PC_LFD:
        case PC_LFDU:
        case PC_LFDX:
        case PC_LFDUX:
        case PC_STFD:
        case PC_STFDU:
        case PC_STFDX:
        case PC_STFDUX:
            while (type->type == TYPEARRAY) {
                type = TPTR_TARGET(type);
            }
            if (type->type == TYPEFLOAT && type->size != 4) {
                return 1;
            }
            if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && type->size >= 8) {
                return 1;
            }
            return 0;

        case PC_LVX:
        case PC_STVX:
            while (type->type == TYPEARRAY) {
                type = TPTR_TARGET(type);
            }
            if (type->type == TYPESTRUCT && CodeMotionStructureKind(TYPE_STRUCT(type)) >= 4 &&
                CodeMotionStructureKind(TYPE_STRUCT(type)) <= 14) {
                return 1;
            }
            if (type->type == TYPESTRUCT) {
                if (TYPE_STRUCT(type)->align == 16) {
                    return 1;
                }
            } else if (type->type == TYPECLASS && TYPE_CLASS(type)->align == 16) {
                return 1;
            }
            return 0;

        case PC_LWZ:
        case PC_LWZU:
        case PC_LWZX:
        case PC_LWZUX:
        case PC_STW:
        case PC_STWU:
        case PC_STWX:
        case PC_STWUX:
            if (type->type == TYPEARRAY || type->type == TYPESTRUCT || type->type == TYPECLASS ||
                (type->type == TYPEMEMBERPOINTER && type->size == 12)) {
                return 1;
            }
            if (type->type != TYPEFLOAT) {
                return type->size == 4;
            }
            return 0;

        case PC_LHZ:
        case PC_LHZU:
        case PC_LHZX:
        case PC_LHZUX:
        case PC_LHA:
        case PC_LHAU:
        case PC_LHAX:
        case PC_LHAUX:
        case PC_STH:
        case PC_STHU:
        case PC_STHX:
        case PC_STHUX:
            if (type->type == TYPEARRAY || type->type == TYPESTRUCT || type->type == TYPECLASS ||
                (type->type == TYPEMEMBERPOINTER && type->size == 12)) {
                if (type->size & 2) {
                    return 1;
                }
            }
            return type->size == 2;

        default:
            return 1;
    }
}

void assign_definition_and_use_starts(int flag)
{
    PCodeBlock *block;
    PCodeInstruction *node;
    PCodeOperand *entry;
    SInt32 i;
    CodeMotionObjectNode *alloc;

    data_00587e38 = codeMotionEntryCount = 0;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (node = block->instructions; node != NULL; node = node->next) {
            if (node->flags & fIsBranch)
                continue;
            if (node->operand_count == 0)
                continue;

            node->definitionStart = data_00587e38;
            node->useStart = codeMotionEntryCount;

            entry = node->operandData.operands;
            for (i = node->operand_count; i--;) {
                if ((entry->kind == PCOp_GPR || entry->kind == PCOp_FPR || entry->kind == PCOp_VR) &&
                    entry->value.reg >= 0x20) {
                    if (entry->flags & 1)
                        data_00587e38++;
                    if (entry->flags & 2)
                        codeMotionEntryCount++;
                }
                entry++;
            }

            if (flag == 0)
                continue;

            if (node->flags & fIsRead) {
                if (node->flags & fIsPtrOp) {
                    for (alloc = gCodeMotionAllocationList_005870fc; alloc != NULL; alloc = alloc->allocation_next) {
                        if (is_object_access_compatible(node, alloc->object))
                            data_00587e38++;
                    }
                } else {
                    data_00587e38++;
                }
            } else if (node->flags & fIsWrite) {
                if (node->flags & fIsPtrOp) {
                    for (alloc = gCodeMotionAllocationList_005870fc; alloc != NULL; alloc = alloc->allocation_next) {
                        if (is_object_access_compatible(node, alloc->object))
                            codeMotionEntryCount++;
                    }
                } else {
                    codeMotionEntryCount++;
                }
            } else if (node->flags & 0x20) {
                for (alloc = gCodeMotionAllocationList_005870fc; alloc != NULL; alloc = alloc->allocation_next) {
                    if (alloc->object->datatype == DDATA || (UInt8)PCodeUtilities_Require(alloc->object) ||
                        (alloc->object->datatype == DLOCAL &&
                         (alloc->object->u.var.info->noregister != 0 || alloc->object->type->type == TYPEARRAY ||
                          (UInt8)(alloc->object->type->type - 4) <= 1 ||
                          (alloc->object->type->type == TYPEMEMBERPOINTER && alloc->object->type->size == 0xc)))) {
                        data_00587e38++;
                        codeMotionEntryCount++;
                    }
                }
            }
        }
    }
}

void build_use_definition_entries(int include_implicit)
{
    PCodeInstruction *instruction;
    PCodeBlock *block;
    CodeMotionEntry *use_entry;
    CodeMotionEntry *definition_entry;
    CodeMotionObjectNode *object_node;
    Object *object;
    CodeMotionObjectNode *found;
    Object *fixed_object;
    CodeMotionEntryLink *link;
    unsigned int use_index;
    unsigned int definition_index;
    PCodeOperand *operand;
    int count;

    CMInit();
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if ((instruction->flags & PCodeInstruction_SkipCodeMotion) != 0)
                continue;
            if (instruction->operand_count == 0)
                continue;
            use_entry = &cm_entries[use_index = instruction->definitionStart];
            definition_entry = &code_motion_entries[definition_index = instruction->useStart];
            operand = instruction->operandData.operands;
            for (count = instruction->operand_count; count--;) {
                if ((operand->kind == PCOp_GPR || operand->kind == PCOp_FPR || operand->kind == PCOp_VR) &&
                    operand->value.reg >= 32) {
                    if ((operand->flags & PCodeOperand_Use) != 0) {
                        use_entry->instruction = instruction;
                        use_entry->kind = operand->kind;
                        use_entry->value.reg = operand->value.reg;
                        link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                        link->entry_index = use_index;
                        if (operand->kind == PCOp_GPR) {
                            link->next = code_motion_register_use_heads[operand->value.reg];
                            code_motion_register_use_heads[operand->value.reg] = link;
                        } else if (operand->kind == PCOp_VR) {
                            link->next = register_use_entry_heads[operand->value.reg];
                            register_use_entry_heads[operand->value.reg] = link;
                        } else {
                            link->next = codeMotionUseEntryHeads[operand->value.reg];
                            codeMotionUseEntryHeads[operand->value.reg] = link;
                        }
                        use_entry++;
                        use_index++;
                    }
                    if ((operand->flags & PCodeOperand_Definition) != 0) {
                        definition_entry->instruction = instruction;
                        definition_entry->kind = operand->kind;
                        definition_entry->value.reg = operand->value.reg;
                        link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                        link->entry_index = definition_index;
                        if (operand->kind == PCOp_GPR) {
                            link->next = code_motion_register_definition_heads[operand->value.reg];
                            code_motion_register_definition_heads[operand->value.reg] = link;
                        } else if (operand->kind == PCOp_VR) {
                            link->next = register_definition_heads[operand->value.reg];
                            register_definition_heads[operand->value.reg] = link;
                        } else {
                            link->next = data_00587f04[operand->value.reg];
                            data_00587f04[operand->value.reg] = link;
                        }
                        definition_entry++;
                        definition_index++;
                    }
                }
                operand++;
            }

            if (include_implicit == 0)
                continue;
            if ((instruction->flags & PCodeInstruction_ImplicitUse) != 0) {
                if ((instruction->flags & PCodeInstruction_NullObjectMemory) != 0) {
                    for (object_node = gCodeMotionAllocationList_005870fc; object_node != NULL;
                         object_node = object_node->allocation_next) {
                        if (is_object_access_compatible(instruction, (object = object_node->object))) {
                            use_entry->instruction = instruction;
                            use_entry->kind = 5;
                            use_entry->is_implicit = 1;
                            use_entry->value.object = object;
                            link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                            link->entry_index = use_index;
                            use_entry++;
                            link->next = object_node->use_entries;
                            object_node->use_entries = link;
                            use_index++;
                        }
                    }
                } else {
                    object = instruction->operandData.operands[2].object;
                    use_entry->instruction = instruction;
                    use_entry->kind = 5;
                    use_entry->is_implicit = 0;
                    use_entry->value.object = object;
                    link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                    link->entry_index = use_index;
                    found = CodeMotion_FindObjectNode(object);
                    link->next = found->use_entries;
                    found->use_entries = link;
                }
            } else if ((instruction->flags & PCodeInstruction_ImplicitDefinition) != 0) {
                if ((instruction->flags & PCodeInstruction_NullObjectMemory) != 0) {
                    for (object_node = gCodeMotionAllocationList_005870fc; object_node != NULL;
                         object_node = object_node->allocation_next) {
                        if (is_object_access_compatible(instruction, (object = object_node->object))) {
                            definition_entry->instruction = instruction;
                            definition_entry->kind = 5;
                            definition_entry->is_implicit = 1;
                            definition_entry->value.object = object;
                            link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                            link->entry_index = definition_index;
                            definition_entry++;
                            link->next = object_node->definition_entries;
                            object_node->definition_entries = link;
                            definition_index++;
                        }
                    }
                } else {
                    object = instruction->operandData.operands[2].object;
                    definition_entry->instruction = instruction;
                    definition_entry->kind = 5;
                    definition_entry->is_implicit = 0;
                    definition_entry->value.object = object;
                    link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                    link->entry_index = definition_index;
                    found = CodeMotion_FindObjectNode(object);
                    link->next = found->definition_entries;
                    found->definition_entries = link;
                }
            } else if ((instruction->flags & PCodeInstruction_GPRFixedRange) != 0) {
                for (object_node = gCodeMotionAllocationList_005870fc; object_node != NULL;
                     object_node = object_node->allocation_next) {
                    object = fixed_object = object_node->object;
                    if (CMKind(object) != 0 && PCodeUtilities_Require(object = fixed_object) == 0) {
                        if ((object = object_node->object)->datatype != DLOCAL)
                            continue;
                        if (object->u.var.info->noregister == 0 && CMIdentity(object)->type->type != TYPEARRAY &&
                            (object = object_node->object)->type->type != TYPESTRUCT &&
                            (object = object_node->object)->type->type != TYPECLASS &&
                            ((object = object_node->object)->type->type != TYPEMEMBERPOINTER ||
                             (object = object_node->object)->type->size != 0x0c))
                            continue;
                    }
                    use_entry->instruction = instruction;
                    use_entry->kind = 5;
                    use_entry->is_implicit = 1;
                    use_entry->value.object = object;
                    link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                    link->entry_index = use_index;
                    use_entry++;
                    link->next = object_node->use_entries;
                    object_node->use_entries = link;
                    use_index++;
                    definition_entry->instruction = instruction;
                    definition_entry->kind = 5;
                    definition_entry->is_implicit = 1;
                    definition_entry->value.object = object;
                    link = (CodeMotionEntryLink *)oalloc(sizeof(CodeMotionEntryLink));
                    link->entry_index = definition_index;
                    definition_entry++;
                    link->next = object_node->definition_entries;
                    object_node->definition_entries = link;
                    definition_index++;
                }
            }
        }
    }
}

void compute_block_definition_and_use_sets(void)
{
    struct PCodeBlock *block;
    struct CodeMotionDataflowState *state;
    UInt32 *generatedDefinitions, *killedDefinitions, *exposedUses, *killedUses;
    struct PCodeInstruction *instruction;
    struct CodeMotionEntry *use;
    struct CodeMotionEntry *definition;
    struct CodeMotionEntryLink *link;
    CodeMotionObjectNode *objectNode;
    int useIndex, definitionIndex;

    block = gPCodeBlocks;
    while (block != NULL) {
        state = &data_00587fe4[block->index];
        generatedDefinitions = state->definition_sets[0];
        killedDefinitions = state->definition_sets[1];
        exposedUses = state->use_sets[0];
        killedUses = state->use_sets[1];
        CRTTI_FillWords(generatedDefinitions, codeMotionEntryCount, 0);
        CRTTI_FillWords(killedDefinitions, codeMotionEntryCount, 0);
        CRTTI_FillWords(exposedUses, data_00587e38, 0);
        CRTTI_FillWords(killedUses, data_00587e38, 0);
        CRTTI_FillWords(state->definition_sets[2], codeMotionEntryCount, 0);
        CRTTI_FillWords(state->definition_sets[3], codeMotionEntryCount, 0);
        CRTTI_FillWords(state->use_sets[2], data_00587e38, 0);
        CRTTI_FillWords(state->use_sets[3], data_00587e38, 0);
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if ((instruction->flags & fIsBranch) == 0 && instruction->operand_count != 0) {
                for (use = &cm_entries[useIndex = instruction->definitionStart];
                     useIndex < data_00587e38 && use->instruction == instruction; use++, useIndex++) {
                    exposedUses[useIndex >> 5] |= 1u << useIndex;
                    if (use->kind == 0) {
                        for (link = code_motion_register_definition_heads[use->value.reg]; link != NULL;
                             link = link->next) {
                            int entryIndex = link->entry_index;
                            if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                                exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                        }
                    } else if (use->kind == 1) {
                        for (link = data_00587f04[use->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                                exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                        }
                    } else if (use->kind == 9) {
                        for (link = register_definition_heads[use->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                                exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                        }
                    } else {
                        objectNode = FindNode(gCodeMotionObjectTree_005880ac, use->value.object);
                        for (link = objectNode->definition_entries; link != NULL; link = link->next) {
                            if (code_motion_entries[link->entry_index].is_implicit == 0 &&
                                (generatedDefinitions[link->entry_index >> 5] & (1u << link->entry_index)))
                                exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                        }
                    }
                }
                for (definition = &code_motion_entries[definitionIndex = instruction->useStart];
                     definitionIndex < codeMotionEntryCount && definition->instruction == instruction;
                     definition++, definitionIndex++) {
                    if (definition->kind == 0) {
                        for (link = code_motion_register_use_heads[definition->value.reg]; link != NULL;
                             link = link->next) {
                            int entryIndex = link->entry_index;
                            if (cm_entries[entryIndex].instruction->block != block)
                                killedUses[entryIndex >> 5] |= 1u << entryIndex;
                        }
                        for (link = code_motion_register_definition_heads[definition->value.reg]; link != NULL;
                             link = link->next) {
                            int entryIndex = link->entry_index;
                            if (code_motion_entries[entryIndex].instruction->block != block)
                                killedDefinitions[entryIndex >> 5] |= 1u << entryIndex;
                            else
                                generatedDefinitions[entryIndex >> 5] &= ~(1u << (entryIndex & 31));
                        }
                    } else if (definition->kind == 1) {
                        for (link = codeMotionUseEntryHeads[definition->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (cm_entries[entryIndex].instruction->block != block)
                                killedUses[entryIndex >> 5] |= 1u << entryIndex;
                        }
                        for (link = data_00587f04[definition->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (code_motion_entries[entryIndex].instruction->block != block)
                                killedDefinitions[entryIndex >> 5] |= 1u << entryIndex;
                            else
                                generatedDefinitions[entryIndex >> 5] &= ~(1u << (entryIndex & 31));
                        }
                    } else if (definition->kind == 9) {
                        for (link = register_use_entry_heads[definition->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (cm_entries[entryIndex].instruction->block != block)
                                killedUses[entryIndex >> 5] |= 1u << entryIndex;
                        }
                        for (link = register_definition_heads[definition->value.reg]; link != NULL; link = link->next) {
                            int entryIndex = link->entry_index;
                            if (code_motion_entries[entryIndex].instruction->block != block)
                                killedDefinitions[entryIndex >> 5] |= 1u << entryIndex;
                            else
                                generatedDefinitions[entryIndex >> 5] &= ~(1u << (entryIndex & 31));
                        }
                    } else {
                        if (definition->is_implicit == 0) {
                            objectNode = FindNode(gCodeMotionObjectTree_005880ac, definition->value.object);
                            for (link = objectNode->use_entries; link != NULL; link = link->next) {
                                int entryIndex = link->entry_index;
                                if (cm_entries[entryIndex].instruction->block != block)
                                    killedUses[entryIndex >> 5] |= 1u << entryIndex;
                            }
                            for (link = objectNode->definition_entries; link != NULL; link = link->next) {
                                int entryIndex = link->entry_index;
                                if (code_motion_entries[entryIndex].instruction->block != block)
                                    killedDefinitions[entryIndex >> 5] |= 1u << entryIndex;
                                else
                                    generatedDefinitions[entryIndex >> 5] &= ~(1u << (entryIndex & 31));
                            }
                        }
                    }
                    generatedDefinitions[definitionIndex >> 5] |= 1u << definitionIndex;
                }
            }
        }
        block = block->next;
    }
}

void solve_definition_sets(void)
{
    UInt32 *incoming;
    UInt32 *generated;
    UInt32 *killed;
    UInt32 *outgoing;
    UInt32 newBits;
    SInt32 word;
    SInt32 wordCount;
    SInt32 changed;
    int blockIndex;
    PCodeBlock *block;
    struct PCodeBlockLink *predecessor;
    struct CodeMotionDataflowState *state;
    UInt32 *mergedBits;

    wordCount = (codeMotionEntryCount + 31) >> 5;
    do {
        changed = 0;
        for (blockIndex = 0; blockIndex < gPCodeBlockCount; blockIndex++) {
            if ((block = gPCodeBlockOrder[blockIndex]) != NULL) {
                state = &data_00587fe4[block->index];
                if ((predecessor = block->predecessors) != NULL) {
                    mergedBits = state->definition_sets[2];
                    CodeMotion_AllocateBits(mergedBits,
                                            data_00587fe4[predecessor->payload.block->index].definition_sets[3],
                                            codeMotionEntryCount);
                    for (predecessor = predecessor->next; predecessor != NULL; predecessor = predecessor->next) {
                        CRTTI_OrBitVector(mergedBits,
                                          data_00587fe4[predecessor->payload.block->index].definition_sets[3],
                                          codeMotionEntryCount);
                    }
                }
                outgoing = state->definition_sets[3];
                incoming = state->definition_sets[2];
                generated = state->definition_sets[0];
                killed = state->definition_sets[1];
                for (word = 0; word < wordCount; word++) {
                    newBits = *generated | (*incoming & ~*killed);
                    if (newBits != *outgoing) {
                        *outgoing = newBits;
                        changed = 1;
                    }
                    incoming++;
                    outgoing++;
                    killed++;
                    generated++;
                }
            }
        }
    } while (changed);
}

void propagate_use_sets(void)
{
    PCodeBlockLink *range;
    PCodeBlock *block;
    struct CodeMotionDataflowState *state;
    struct CodeMotionDataflowState *srcstate;
    UInt32 *def;
    UInt32 *use;
    UInt32 *kill;
    UInt32 *out;
    UInt32 *dst;
    UInt32 bits;
    SInt32 nwords;
    SInt32 changed;
    SInt32 i;
    SInt32 j;

    nwords = (data_00587e38 + 31) >> 5;
    do {
        changed = 0;
        i = gPCodeBlockCount;
        while (0 != i) {
            i--;
            if ((block = gPCodeBlockOrder[i]) != NULL) {
                state = &data_00587fe4[block->index];
                if ((range = block->successors) != NULL) {
                    dst = state->use_sets[3];
                    srcstate = &data_00587fe4[range->payload.block->index];
                    CodeMotion_AllocateBits(dst, srcstate->use_sets[2], data_00587e38);
                    for (range = range->next; range != NULL; range = range->next)
                        CRTTI_OrBitVector(dst, data_00587fe4[range->payload.block->index].use_sets[2], data_00587e38);
                }
                def = state->use_sets[3];
                out = state->use_sets[2];
                use = state->use_sets[0];
                kill = state->use_sets[1];
                for (j = 0; j < nwords; j++) {
                    bits = (~*kill & *def) | *use;
                    if (bits != *out) {
                        *out = bits;
                        changed = 1;
                    }
                    out++;
                    def++;
                    kill++;
                    use++;
                }
            }
        }
    } while (changed);
}

void COpt_SetLoopCodeMotionMode(int mode)
{
    PCodeInstruction *instruction;
    PCodeBlock *block;
    int index;
    struct CodeMotionDataflowState *state;

    if (mode != 0) {
        gCodeMotionObjectTree_005880ac = gCodeMotionAllocationList_005870fc = NULL;

        for (block = gPCodeBlocks; block != NULL; block = block->next) {
            for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
                if ((instruction->flags & PCodeInstruction_GPRResultMask) != 0 &&
                    (instruction->flags & PCodeInstruction_NullObjectMemory) == 0) {
                    COpt_00524b20(instruction->operandData.operands[2].object);
                }
            }
        }
    }

    assign_definition_and_use_starts(mode);
    build_use_definition_entries(mode);

    data_00587fe4 = (struct CodeMotionDataflowState *)oalloc(gPCodeBlockCount * sizeof(struct CodeMotionDataflowState));
    state = data_00587fe4;
    for (index = 0; index < gPCodeBlockCount; index++, state++) {
        state->definition_sets[0] = CodeMotion_NewBits(codeMotionEntryCount);
        state->definition_sets[1] = CodeMotion_NewBits(codeMotionEntryCount);
        state->definition_sets[2] = CodeMotion_NewBits(codeMotionEntryCount);
        state->definition_sets[3] = CodeMotion_NewBits(codeMotionEntryCount);
        state->use_sets[0] = CodeMotion_NewBits(data_00587e38);
        state->use_sets[1] = CodeMotion_NewBits(data_00587e38);
        state->use_sets[2] = CodeMotion_NewBits(data_00587e38);
        state->use_sets[3] = CodeMotion_NewBits(data_00587e38);
    }

    compute_block_definition_and_use_sets();
    SpillCode_BuildBlockOrder();
    solve_definition_sets();
    propagate_use_sets();
}
