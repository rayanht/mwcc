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

static inline UInt32 *CodeMotion_NewBits(int bit_count)
{
    return oalloc(((bit_count + 31) >> 5) * sizeof(unsigned int));
}

static inline Object *CMIdentity(Object *o)
{
    return o;
}

static inline unsigned char CMKind(Object *p)
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

static inline SInt32 CodeMotionStructureKind(TypeStruct *type)
{
    return type->stype;
}

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

/* (the objects memory operands name, in a tree for lookup and a list in allocation order) */
static void collect_memory_objects(void)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;

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
    int count;
    PCodeOperand *operand;

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
                        } else {
                            if (operand->kind == PCOp_VR) {
                                link->next = register_definition_heads[operand->value.reg];
                                register_definition_heads[operand->value.reg] = link;
                            } else {
                                link->next = data_00587f04[operand->value.reg];
                                data_00587f04[operand->value.reg] = link;
                            }
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
                    found = find_object_node(object);
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
                    found = find_object_node(object);
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

static void allocate_dataflow_sets(void)
{
    struct CodeMotionDataflowState *state;
    int index;

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
    int entryIndex;

    block = gPCodeBlocks;
    while (block != NULL) {
        state = &data_00587fe4[block->index];
        generatedDefinitions = state->definition_sets[0];
        killedDefinitions = state->definition_sets[1];
        exposedUses = state->use_sets[0];
        killedUses = state->use_sets[1];
        BitVectors_FillWords(generatedDefinitions, codeMotionEntryCount, 0);
        BitVectors_FillWords(killedDefinitions, codeMotionEntryCount, 0);
        BitVectors_FillWords(exposedUses, data_00587e38, 0);
        BitVectors_FillWords(killedUses, data_00587e38, 0);
        BitVectors_FillWords(state->definition_sets[2], codeMotionEntryCount, 0);
        BitVectors_FillWords(state->definition_sets[3], codeMotionEntryCount, 0);
        BitVectors_FillWords(state->use_sets[2], data_00587e38, 0);
        BitVectors_FillWords(state->use_sets[3], data_00587e38, 0);
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if (instruction->flags & fIsBranch)
                continue;
            if (instruction->operand_count == 0)
                continue;
            for (use = &cm_entries[useIndex = instruction->definitionStart];
                 useIndex < data_00587e38 && use->instruction == instruction; use++, useIndex++) {
                exposedUses[useIndex >> 5] |= 1u << useIndex;
                if (use->kind == 0) {
                    for (link = code_motion_register_definition_heads[use->value.reg]; link != NULL;
                         link = link->next) {
                        entryIndex = link->entry_index;
                        if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                            exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                    }
                } else if (use->kind == 1) {
                    for (link = data_00587f04[use->value.reg]; link != NULL; link = link->next) {
                        entryIndex = link->entry_index;
                        if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                            exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                    }
                } else if (use->kind == 9) {
                    for (link = register_definition_heads[use->value.reg]; link != NULL; link = link->next) {
                        entryIndex = link->entry_index;
                        if (generatedDefinitions[entryIndex >> 5] & (1u << entryIndex))
                            exposedUses[useIndex >> 5] &= ~(1u << useIndex);
                    }
                } else {
                    objectNode = find_object_node(use->value.object);
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
                        if (cm_entries[link->entry_index].instruction->block != block)
                            killedUses[link->entry_index >> 5] |= 1u << link->entry_index;
                    }
                    for (link = code_motion_register_definition_heads[definition->value.reg]; link != NULL;
                         link = link->next) {
                        if (code_motion_entries[link->entry_index].instruction->block != block)
                            killedDefinitions[link->entry_index >> 5] |= 1u << link->entry_index;
                        else
                            generatedDefinitions[link->entry_index >> 5] &= ~(1u << (link->entry_index & 31));
                    }
                } else if (definition->kind == 1) {
                    for (link = codeMotionUseEntryHeads[definition->value.reg]; link != NULL; link = link->next) {
                        if (cm_entries[link->entry_index].instruction->block != block)
                            killedUses[link->entry_index >> 5] |= 1u << link->entry_index;
                    }
                    for (link = data_00587f04[definition->value.reg]; link != NULL; link = link->next) {
                        if (code_motion_entries[link->entry_index].instruction->block != block)
                            killedDefinitions[link->entry_index >> 5] |= 1u << link->entry_index;
                        else
                            generatedDefinitions[link->entry_index >> 5] &= ~(1u << (link->entry_index & 31));
                    }
                } else if (definition->kind == 9) {
                    for (link = register_use_entry_heads[definition->value.reg]; link != NULL; link = link->next) {
                        if (cm_entries[link->entry_index].instruction->block != block)
                            killedUses[link->entry_index >> 5] |= 1u << link->entry_index;
                    }
                    for (link = register_definition_heads[definition->value.reg]; link != NULL; link = link->next) {
                        if (code_motion_entries[link->entry_index].instruction->block != block)
                            killedDefinitions[link->entry_index >> 5] |= 1u << link->entry_index;
                        else
                            generatedDefinitions[link->entry_index >> 5] &= ~(1u << (link->entry_index & 31));
                    }
                } else {
                    if (definition->is_implicit == 0) {
                        objectNode = find_object_node(definition->value.object);
                        for (link = objectNode->use_entries; link != NULL; link = link->next) {
                                if (cm_entries[link->entry_index].instruction->block != block)
                                killedUses[link->entry_index >> 5] |= 1u << link->entry_index;
                        }
                        for (link = objectNode->definition_entries; link != NULL; link = link->next) {
                                if (code_motion_entries[link->entry_index].instruction->block != block)
                                killedDefinitions[link->entry_index >> 5] |= 1u << link->entry_index;
                            else
                                generatedDefinitions[link->entry_index >> 5] &= ~(1u << (link->entry_index & 31));
                        }
                    }
                }
                generatedDefinitions[definitionIndex >> 5] |= 1u << definitionIndex;
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
                        BitVectors_OrBitVector(mergedBits,
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
                        BitVectors_OrBitVector(dst, data_00587fe4[range->payload.block->index].use_sets[2],
                                               data_00587e38);
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
    if (mode != 0)
        collect_memory_objects();
    assign_definition_and_use_starts(mode);
    build_use_definition_entries(mode);
    allocate_dataflow_sets();
    compute_block_definition_and_use_sets();
    SpillCode_BuildBlockOrder();
    solve_definition_sets();
    propagate_use_sets();
}
