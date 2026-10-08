#define CERROR_FILE "LoopDetection.c"
#include "compiler/common.h"
#include "compiler/LoopDetection.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BitVectors.h"
#include "compiler/CError.h"
#include "compiler/CFunc.h"
#include "compiler/CRTTI.h"
#include "compiler/CodeMotion.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/TOC.h"

int gPCodeBlockCount;
signed long data_005871a4;

static UInt32 **dominators;
static struct SelectedNode *selected_nodes;
static SInt32 predecessor_bitset_node_count;
static struct PCodeBlock **data_00582c64;

#define SETB(a, i) ((a)[(i) >> 5] |= (UInt32)1 << (i))
#define TESTB(a, i) ((a)[(i) >> 5] & ((UInt32)1 << (i)))
#define bitvectorgetbit(n, bv) ((1 << ((n) & 31)) & (bv)[(n) >> 5])

static inline PCodeBlockLink *code_00522b40_inline1(PCodeBlock *p0)
{
    PCodeBlockLink *t5;
    t5 = (PCodeBlockLink *)lalloc(8);
    t5->payload.block = p0;
    return t5;
}

static void AddNode(Loop *w, PCodeBlock *item)
{
    PCodeBlockLink *nn = (PCodeBlockLink *)lalloc(8);
    SETB(w->memberblocks, item->index);
    nn->payload.block = item;
    nn->next = w->blocks;
    w->blocks = nn;
}

static void AddCount(PCodeBlock *item, SInt32 *count)
{
    data_00582c64[*count] = item;
    (*count)++;
}

void compute_dominators(void)
{
    SInt32 count;
    PCodeBlockLink *node;
    UInt32 *bits;
    PCodeBlock *block;
    SInt32 i;
    SInt32 changed;

    count = gPCodeBlockCount;
    dominators = (UInt32 **)oalloc(count * 4);
    for (i = 0; i < gPCodeBlockCount; i++)
        dominators[i] = (UInt32 *)oalloc(((count + 0x1f) >> 5) << 2);

    bits = (UInt32 *)oalloc(((count + 0x1f) >> 5) << 2);

    CRTTI_FillWords(dominators[gPCodeBlocks->index], count, 0);
    dominators[gPCodeBlocks->index][0] |= 1;
    for (block = gPCodeBlocks->next; block != NULL; block = block->next)
        CRTTI_FillWords(dominators[block->index], count, -1);

    SpillCode_BuildBlockOrder();

    do {
        changed = 0;
        for (i = 0; i < gPCodeBlockCount; i++) {
            if ((block = gPCodeBlockOrder[i]) != NULL && block->index != gPCodeBlocks->index) {
                CodeMotion_AllocateBits(bits, dominators[block->predecessors->payload.block->index], count);
                for (node = block->predecessors->next; node != NULL; node = node->next)
                    CRTTI_IntersectBitVectors(bits, dominators[node->payload.block->index], count);
                bits[block->index >> 5] |= 1 << block->index;
                if (BitVectors_CopyAndCheckChanged(dominators[block->index], bits, count))
                    changed = 1;
            }
        }
    } while (changed);
}

struct SelectedNode *collect_nodes_in_predecessor_bitsets(void)
{
    PCodeBlock *node;
    PCodeBlockLink *edge;

    selected_nodes = NULL;
    predecessor_bitset_node_count = 0;
    for (node = gPCodeBlocks->next; node != NULL; node = node->next) {
        for (edge = node->predecessors; edge != NULL; edge = edge->next) {
            if (((1 << node->index) & dominators[edge->payload.block->index][node->index >> 5]) != 0)
                break;
        }
        if (edge != NULL) {
            SelectedNode *entry = (SelectedNode *)oalloc(8);
            entry->node = (PCodeBlock *)node;
            entry->next = selected_nodes;
            selected_nodes = entry;
            ++predecessor_bitset_node_count;
        }
    }
    return selected_nodes;
}

void LoopDetection_AddBlock(Loop *loop, PCodeBlock *block)
{
    PCodeBlockLink *entry;
    entry = (PCodeBlockLink *)lalloc(8);
    loop->memberblocks[block->index >> 5] |= 1 << (block->index & 31);
    entry->payload.block = block;
    entry->next = loop->blocks;
    loop->blocks = entry;
}

void compute_loop_block_sets(Loop *w)
{
    PCodeBlockLink *first;
    PCodeBlockLink *sub;
    PCodeBlockLink *node;
    SInt32 n;

    n = 0;
    AddNode(w, w->body);

    for (first = w->body->predecessors; first != NULL; first = first->next) {
        if (TESTB(dominators[first->payload.block->index], w->body->index) && first->payload.block != w->body) {
            AddNode(w, first->payload.block);
            AddCount(first->payload.block, &n);
        }
    }

    while (n != 0) {
        n--;
        for (first = data_00582c64[n]->predecessors; first != NULL; first = first->next) {
            if (!TESTB(w->memberblocks, first->payload.block->index)) {
                AddNode(w, first->payload.block);
                AddCount(first->payload.block, &n);
            }
        }
    }

    for (node = w->blocks; node != NULL; node = node->next) {
        for (sub = node->payload.block->successors; sub != NULL; sub = sub->next) {
            if (!TESTB(w->memberblocks, sub->payload.block->index)) {
                SETB(w->exitblocks, node->payload.block->index);
                break;
            }
        }
    }

    for (node = w->blocks; node != NULL; node = node->next) {
        for (sub = w->blocks; sub != NULL; sub = sub->next) {
            if (TESTB(w->exitblocks, sub->payload.block->index) &&
                !TESTB(dominators[sub->payload.block->index], node->payload.block->index))
                break;
        }
        if (sub == NULL)
            SETB(w->block_membership, node->payload.block->index);
    }

    for (sub = w->blocks; sub != NULL; sub = sub->next) {
        for (node = w->body->predecessors; node != NULL; node = node->next) {
            if (TESTB(w->memberblocks, node->payload.block->index) &&
                !TESTB(dominators[node->payload.block->index], sub->payload.block->index))
                break;
        }
        if (node == NULL)
            SETB(w->backedge_dominators, sub->payload.block->index);
    }
}

void fn_00523000(Loop *node, Loop **siblings)
{
    Loop *candidate;
    Loop **link;
    SInt32 scopeId;
    link = siblings;
    while ((candidate = *link) != NULL) {
        scopeId = node->body->index;
        if (candidate->memberblocks[scopeId >> 5] & (1 << scopeId)) {
            node->parent = candidate;
            fn_00523000(node, &candidate->children);
            return;
        }
        scopeId = candidate->body->index;
        if ((1 << scopeId) & node->memberblocks[scopeId >> 5]) {
            *link = candidate->sibling;
            candidate->parent = node;
            candidate->sibling = node->children;
            node->children = candidate;
        } else {
            link = &candidate->sibling;
        }
    }
    node->sibling = *siblings;
    *siblings = node;
}

/* Code-motion work record; the pointer and scalar roles are not yet known. */
void create_loops(void)
{
    Loop *block;

    data_005871a4 = predecessor_bitset_node_count * 5 + gPCodeBlockCount;
    data_00582c64 = oalloc(gPCodeBlockCount * 4);
    while (selected_nodes != NULL) {
        block = (Loop *)lalloc(0x58);
        block->parent = block->sibling = block->children = NULL;
        block->body = selected_nodes->node;
        block->preheader = NULL;
        block->blocks = NULL;
        block->codeMotionSearches = NULL;
        block->footer = NULL;
        block->inductionUpdate = NULL;
        block->execution_weight = block->body->execution_weight;
        CRTTI_FillWords(block->memberblocks = lalloc(((data_005871a4 + 31) >> 5) << 2), data_005871a4, 0);
        CRTTI_FillWords(block->exitblocks = lalloc(((data_005871a4 + 31) >> 5) << 2), data_005871a4, 0);
        CRTTI_FillWords(block->block_membership = lalloc(((data_005871a4 + 31) >> 5) << 2), data_005871a4, 0);
        CRTTI_FillWords(block->backedge_dominators = lalloc(((data_005871a4 + 31) >> 5) << 2), data_005871a4, 0);
        compute_loop_block_sets(block);
        fn_00523000(block, &data_0058763c);
        selected_nodes = selected_nodes->next;
    }
}

void LoopDetection_CreatePreheader(Loop *region)
{
    int executionWeight;
    PCodeInstruction *instruction;
    Object *table;
    PCodeLabel **targets;
    int targetIndex;
    struct TOCNameEntry *entry;
    PCodeBlockLink *successor;
    PCodeBlock *insertionBlock;
    PCodeBlock *allocatedBlock;
    PCodeLabel *label;
    int previousIndex;
    PCodeBlockLink *successorLink;
    PCodeBlockLink *predecessorLink;
    PCodeBlockLink *memberLink;
    int oldBlockIndex;
    int exitBlockIndex;
    PCodeBlockLink **predecessorSlot;
    PCodeBlock *newBlock;
    PCodeBlock *oldBlock;
    PCodeBlockLink *predecessor;

    label = PCode_NewLabel();
    allocatedBlock = (PCodeBlock *)lalloc(sizeof(PCodeBlock));
    allocatedBlock->next = NULL;
    allocatedBlock->prev = NULL;
    allocatedBlock->labels = NULL;
    allocatedBlock->predecessors = allocatedBlock->successors = NULL;
    allocatedBlock->instructions = allocatedBlock->reverse_instructions = NULL;
    allocatedBlock->line = -1;
    allocatedBlock->instruction_count = 0;
    allocatedBlock->flags = 0;
    allocatedBlock->index = gPCodeBlockCount;
    gPCodeBlockCount += 1;
    PCode_ResolveLabel(allocatedBlock, label);
    region->preheader = allocatedBlock;
    oldBlock = region->body;
    (newBlock = region->preheader)->line = oldBlock->line;
    if (oldBlock->labels == NULL) {
        PCode_ResolveLabel(oldBlock, PCode_NewLabel());
    }
    PCode_AppendInstruction(newBlock, PCodeUtilities_CreateInstruction(0, oldBlock->labels));
    if (region->parent != NULL) {
        executionWeight = region->parent->execution_weight;
    } else {
        executionWeight = 1;
    }
    newBlock->execution_weight = executionWeight;
    predecessorSlot = &oldBlock->predecessors;
    while ((predecessor = *predecessorSlot) != NULL) {
        if ((1 << predecessor->payload.block->index & region->memberblocks[predecessor->payload.block->index >> 5]) !=
            0) {
            predecessorSlot = &predecessor->next;
        } else {
            if (predecessor->payload.block->instruction_count != 0) {
                instruction = predecessor->payload.block->reverse_instructions;
                if (instruction->opcode == PC_B) {
                    if (instruction->operandData.operands[0].value.label->target.block == oldBlock) {
                        instruction->operandData.operands[0].value.label = newBlock->labels;
                    }
                } else if (instruction->opcode == PC_BT || instruction->opcode == PC_BF) {
                    if (instruction->operandData.operands[2].value.label->target.block == oldBlock) {
                        instruction->operandData.operands[2].value.label = newBlock->labels;
                    }
                } else if (instruction->opcode == PC_BCTR) {
                    if (instruction->operand_count > 1 && instruction->operandData.operands[1].kind == PCOp_MEMORY) {
                        table = instruction->operandData.operands[1].object;
                        targets = (PCodeLabel **)instruction->operandData.operands[1].object->u.data.u.switchtable.data;
                        for (targetIndex = 0; targetIndex < table->u.data.u.switchtable.size; targetIndex++) {
                            if (targets[targetIndex]->target.block == oldBlock) {
                                targets[targetIndex] = newBlock->labels;
                            }
                        }
                    } else {
                        entry = toc_name_entries;
                        while (entry != NULL) {
                            if (entry->label->pclabel->target.block == oldBlock) {
                                entry->label->pclabel = newBlock->labels;
                            }
                            entry = entry->next;
                        }
                    }
                } else
                    CError_ASSERT(496, predecessor->payload.block->next == oldBlock);
            }
            successor = predecessor->payload.block->successors;
            if (successor != NULL) {
                do {
                    if (successor->payload.block == oldBlock) {
                        successor->payload.block = newBlock;
                    }
                    successor = successor->next;
                } while (successor != NULL);
            }
            *predecessorSlot = predecessor->next;
            predecessor->next = newBlock->predecessors;
            newBlock->predecessors = predecessor;
        }
    }
    previousIndex = oldBlock->prev->index;
    if ((1 << previousIndex & region->memberblocks[previousIndex >> 5]) == 0) {
        newBlock->next = oldBlock;
        newBlock->prev = oldBlock->prev, oldBlock->prev->next = newBlock;
        oldBlock->prev = newBlock;
    } else {
        insertionBlock = gPCodeBlocks;
        while (insertionBlock != NULL) {
            if ((1 << insertionBlock->index & region->memberblocks[insertionBlock->index >> 5]) != 0) {
                break;
            }
            insertionBlock = insertionBlock->next;
        }
        newBlock->next = insertionBlock;
        newBlock->prev = insertionBlock->prev;
        insertionBlock->prev->next = newBlock;
        insertionBlock->prev = newBlock;
    }
    predecessorLink = code_00522b40_inline1(newBlock);
    predecessorLink->next = oldBlock->predecessors;
    oldBlock->predecessors = predecessorLink;
    successorLink = code_00522b40_inline1(oldBlock);
    successorLink->next = newBlock->successors;
    newBlock->successors = successorLink;
    region = region->parent;
    while (region != NULL) {
        memberLink = (PCodeBlockLink *)lalloc(sizeof(PCodeBlockLink));
        region->memberblocks[newBlock->index >> 5] |= 1 << newBlock->index;
        memberLink->payload.block = newBlock;
        memberLink->next = region->blocks;
        region->blocks = memberLink;
        oldBlockIndex = oldBlock->index;
        if ((1 << oldBlockIndex & region->block_membership[oldBlockIndex >> 5]) != 0) {
            region->block_membership[newBlock->index >> 5] |= 1 << newBlock->index;
        }
        exitBlockIndex = oldBlock->index;
        if ((1 << exitBlockIndex & region->backedge_dominators[exitBlockIndex >> 5]) != 0) {
            region->backedge_dominators[newBlock->index >> 5] |= 1 << newBlock->index;
        }
        region = region->parent;
    }
}

void traverse_loops_postorder(register Loop *node)
{
    register Loop *child;
    register Loop *grandchild;
    register Loop *greatGrandchild;
    register Loop *fourthLevelLoop;
    register Loop *fifthLevelLoop;
    register Loop *sixthLevelLoop;
    register Loop *seventhLevelLoop;
    for (; node != NULL; node = node->sibling) {
        if (node->children != NULL) {
            child = node->children;
            for (; child != NULL; child = child->sibling) {
                if (child->children != NULL) {
                    grandchild = child->children;
                    for (; grandchild != NULL; grandchild = grandchild->sibling) {
                        if (grandchild->children != NULL) {
                            greatGrandchild = grandchild->children;
                            for (; greatGrandchild != NULL; greatGrandchild = greatGrandchild->sibling) {
                                if (greatGrandchild->children != NULL) {
                                    fourthLevelLoop = greatGrandchild->children;
                                    for (; fourthLevelLoop != NULL; fourthLevelLoop = fourthLevelLoop->sibling) {
                                        if (fourthLevelLoop->children != NULL) {
                                            fifthLevelLoop = fourthLevelLoop->children;
                                            for (; fifthLevelLoop != NULL; fifthLevelLoop = fifthLevelLoop->sibling) {
                                                if (fifthLevelLoop->children != NULL) {
                                                    sixthLevelLoop = fifthLevelLoop->children;
                                                    for (; sixthLevelLoop != NULL;
                                                         sixthLevelLoop = sixthLevelLoop->sibling) {
                                                        if (sixthLevelLoop->children != NULL) {
                                                            seventhLevelLoop = sixthLevelLoop->children;
                                                            for (; seventhLevelLoop != NULL;
                                                                 seventhLevelLoop = seventhLevelLoop->sibling) {
                                                                if (seventhLevelLoop->children != NULL) {
                                                                    traverse_loops_postorder(
                                                                        seventhLevelLoop->children);
                                                                }
                                                                LoopDetection_CreatePreheader(seventhLevelLoop);
                                                            }
                                                        }
                                                        LoopDetection_CreatePreheader(sixthLevelLoop);
                                                    }
                                                }
                                                LoopDetection_CreatePreheader(fifthLevelLoop);
                                            }
                                        }
                                        LoopDetection_CreatePreheader(fourthLevelLoop);
                                    }
                                }
                                LoopDetection_CreatePreheader(greatGrandchild);
                            }
                        }
                        LoopDetection_CreatePreheader(grandchild);
                    }
                }
                LoopDetection_CreatePreheader(child);
            }
        }
        LoopDetection_CreatePreheader(node);
    }
}

void LoopDetection_DetectLoops(void)
{
    data_0058763c = NULL;
    compute_dominators();
    if (collect_nodes_in_predecessor_bitsets() != NULL) {
        create_loops();
        traverse_loops_postorder(data_0058763c);
    }
    freeoheap();
}

int compute_iteration_count(int op, int reg, SInt32 lower, SInt32 upper, SInt32 step, SInt32 *result)
{
    if (op == 5) {
        if (reg == 0) {
            if (step <= 0)
                return 0;
            if (lower < upper)
                *result = ((upper - lower) + step - 1) / step;
            else
                *result = 0;
        } else if (reg == 1) {
            if (step >= 0)
                return 0;
            if (lower > upper)
                *result = ((lower - upper) - step - 1) / -step;
            else
                *result = 0;
        } else {
            return 0;
        }
    } else if (reg == 0) {
        if (step >= 0)
            return 0;
        if (lower >= upper)
            *result = ((lower - upper) - step) / -step;
        else
            *result = 0;
    } else if (reg == 1) {
        if (step <= 0)
            return 0;
        if (lower <= upper)
            *result = ((upper - lower) + step) / step;
        else
            *result = 0;
    } else if (lower < upper) {
        if (step <= 0)
            return 0;
        if ((upper - lower) % step != 0)
            return 0;
        *result = (upper - lower) / step;
    } else if (lower > upper) {
        if (step >= 0)
            return 0;
        if ((lower - upper) % -step != 0)
            return 0;
        *result = (lower - upper) / -step;
    } else {
        *result = 0;
    }
    return 1;
}

SInt32 compute_unsigned_iteration_count(SInt32 op, SInt32 mode, UInt32 a, UInt32 b, SInt32 step, SInt32 *result)
{
    if (op == 5) {
        if (mode == 0) {
            if (step <= 0)
                return 0;
            if (a < b)
                *result = (b - a + step - 1) / step;
            else
                *result = 0;
        } else if (mode == 1) {
            if (step >= 0)
                return 0;
            if (a > b)
                *result = (a - b - step - 1) / -step;
            else
                *result = 0;
        } else {
            return 0;
        }
    } else {
        if (mode == 0) {
            if (step >= 0)
                return 0;
            if (a >= b)
                *result = (a - b - step) / -step;
            else
                *result = 0;
        } else if (mode == 1) {
            if (step <= 0)
                return 0;
            if (a <= b)
                *result = (b - a + step) / step;
            else
                *result = 0;
        } else {
            if (a < b) {
                if (step <= 0)
                    return 0;
                if ((b - a) % step != 0)
                    return 0;
                *result = (b - a) / step;
            } else if (a > b) {
                if (step >= 0)
                    return 0;
                if ((a - b) % -step != 0)
                    return 0;
                *result = (a - b) / -step;
            } else {
                *result = 0;
            }
        }
    }
    if (*result < 0)
        return 0;
    return 1;
}

unsigned int fn_00522640(unsigned int selector, unsigned int mode, int value, UInt8 *result)
{
    if (selector == 5U) {
        if (mode == 0U) {
            if (value <= 0)
                return 0;
            *result = 19;
        } else if (mode == 1U) {
            if (value >= 0)
                return 0;
            *result = 20;
        } else {
            return 0;
        }
    } else {
        if (mode == 0U) {
            if (value >= 0)
                return 0;
            *result = 22;
        } else if (mode == 1U) {
            if (value <= 0)
                return 0;
            *result = 21;
        } else {
            if (value == 1)
                *result = 24;
            else if (value == -1)
                *result = 24;
            else
                return 0;
        }
    }
    return 1;
}

void detect_counting_loop(Loop *loop)
{
    struct CodeMotionEntryLink *definition;
    PCodeInstruction *branch;
    PCodeInstruction *compare;
    PCodeInstruction *update;
    PCodeInstruction *highLoad;
    PCodeInstruction *boundDefinition;
    PCodeInstruction *inductionUpdate;
    Loop *child;
    SInt16 compareOpcode;
    SInt16 conditionRegister;
    SInt16 inductionRegister;
    SInt16 boundRegister;
    SInt16 conditionBit;
    SInt16 definitionOpcode;

    if (!(branch = loop->body->reverse_instructions))
        return;
    if (branch->opcode != PC_BT && branch->opcode != PC_BF)
        return;
    if (!bitvectorgetbit(branch->operandData.operands[2].value.label->target.block->index, loop->memberblocks))
        return;
    if (bitvectorgetbit(loop->body->next->index, loop->memberblocks))
        return;

    conditionRegister = branch->operandData.operands[0].value.reg;
    conditionBit = branch->operandData.operands[1].value.reg;
    if (!(compare = branch->previous))
        return;

    if ((compareOpcode = compare->opcode) == PC_ADDI && compare->operandData.operands[2].kind == PCOp_IMMEDIATE) {
        update = compare;
        if (!(compare = compare->previous))
            return;
        compareOpcode = compare->opcode;
        if (update->operandData.operands[0].value.reg != update->operandData.operands[1].value.reg)
            return;
        if (compareOpcode != PC_CMP && compareOpcode != PC_CMPL && compareOpcode != PC_CMPI &&
            compareOpcode != PC_CMPLI)
            return;
        if (compare->operandData.operands[1].value.reg != update->operandData.operands[0].value.reg)
            return;
        if (!(loop->step = update->operandData.operands[2].value.signed_value))
            return;
    }

    if (compareOpcode != PC_CMP && compareOpcode != PC_CMPL && compareOpcode != PC_CMPI && compareOpcode != PC_CMPLI)
        return;
    if (compare->operandData.operands[0].value.reg != conditionRegister)
        return;
    if ((inductionRegister = compare->operandData.operands[1].value.reg) < 32)
        return;
    if (loop->preheader->next != branch->operandData.operands[2].value.label->target.block)
        return;

    if (compareOpcode == PC_CMPI) {
        if (compare->previous)
            return;
        loop->upper = compare->operandData.operands[2].value.signed_value;
        loop->upperType = 1;
    } else if (compareOpcode == PC_CMPLI) {
        if (compare->previous)
            return;
        loop->upper = (UInt16)compare->operandData.operands[2].value.signed_value;
        loop->upperType = 1;
    } else if (compareOpcode == PC_CMP || compareOpcode == PC_CMPL) {
        if (compare->previous) {
            if (compare->previous->opcode == PC_LI &&
                compare->previous->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                compare->previous->operandData.operands[0].value.reg == compare->operandData.operands[2].value.reg &&
                !compare->previous->previous) {
                loop->upper = compare->previous->operandData.operands[1].value.signed_value;
                loop->upperType = 1;
            } else if (compare->previous->opcode == PC_LIS &&
                       compare->previous->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                       compare->previous->operandData.operands[0].value.reg ==
                           compare->operandData.operands[2].value.reg &&
                       !compare->previous->previous) {
                loop->upper = compare->previous->operandData.operands[1].value.signed_value << 16;
                loop->upperType = 1;
            } else if (compare->previous->opcode == PC_ADDI &&
                       compare->previous->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                       compare->previous->operandData.operands[0].value.reg ==
                           (boundRegister = compare->operandData.operands[2].value.reg) &&
                       compare->previous->operandData.operands[1].value.reg == boundRegister &&
                       (highLoad = compare->previous->previous) && highLoad->opcode == PC_LIS &&
                       highLoad->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                       highLoad->operandData.operands[0].value.reg == boundRegister && !highLoad->previous) {
                loop->upper = (highLoad->operandData.operands[1].value.signed_value << 16) +
                              compare->previous->operandData.operands[2].value.signed_value;
                loop->upperType = 1;
            } else {
                return;
            }
        } else {
            boundDefinition = NULL;
            for (definition = code_motion_register_definition_heads[compare->operandData.operands[2].value.reg];
                 definition; definition = definition->next) {
                if (bitvectorgetbit(code_motion_entries[definition->entry_index].instruction->block->index,
                                    loop->memberblocks))
                    return;
                if (bitvectorgetbit(definition->entry_index,
                                    data_00587fe4[loop->preheader->index].definition_sets[2])) {
                    if (!boundDefinition) {
                        boundDefinition = code_motion_entries[definition->entry_index].instruction;
                        if (code_motion_entries[definition->entry_index].instruction->opcode == PC_LI &&
                            code_motion_entries[definition->entry_index].instruction->operandData.operands[1].kind ==
                                PCOp_IMMEDIATE) {
                            loop->upper = code_motion_entries[definition->entry_index]
                                              .instruction->operandData.operands[1]
                                              .value.signed_value;
                            loop->upperType = 1;
                        } else if ((definitionOpcode =
                                        code_motion_entries[definition->entry_index].instruction->opcode) == PC_LIS &&
                                   code_motion_entries[definition->entry_index]
                                           .instruction->operandData.operands[1]
                                           .kind == PCOp_IMMEDIATE) {
                            loop->upper = code_motion_entries[definition->entry_index]
                                              .instruction->operandData.operands[1]
                                              .value.signed_value
                                          << 16;
                            loop->upperType = 1;
                        } else if (code_motion_entries[definition->entry_index].instruction->opcode == PC_ADDI &&
                                   code_motion_entries[definition->entry_index]
                                           .instruction->operandData.operands[2]
                                           .kind == PCOp_IMMEDIATE &&
                                   code_motion_entries[definition->entry_index]
                                           .instruction->operandData.operands[1]
                                           .value.reg == (boundRegister = compare->operandData.operands[2].value.reg) &&
                                   (highLoad = code_motion_entries[definition->entry_index].instruction->previous) &&
                                   highLoad->opcode == PC_LIS &&
                                   highLoad->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                                   highLoad->operandData.operands[0].value.reg == boundRegister) {
                            loop->upper = (highLoad->operandData.operands[1].value.signed_value << 16) +
                                          code_motion_entries[definition->entry_index]
                                              .instruction->operandData.operands[2]
                                              .value.signed_value;
                            loop->upperType = 1;
                        } else {
                            loop->upperType = 2;
                            break;
                        }
                    } else {
                        loop->upperType = 2;
                        break;
                    }
                }
            }
            if (!loop->upperType)
                loop->upperType = 2;
        }
    }

    inductionUpdate = NULL;
    for (definition = code_motion_register_definition_heads[inductionRegister]; definition;
         definition = definition->next) {
        if (bitvectorgetbit(code_motion_entries[definition->entry_index].instruction->block->index,
                            loop->memberblocks)) {
            if (!inductionUpdate) {
                inductionUpdate = code_motion_entries[definition->entry_index].instruction;
                if (inductionUpdate->opcode != PC_ADDI)
                    return;
                if (inductionUpdate->operandData.operands[1].value.reg != inductionRegister)
                    return;
                if (inductionUpdate->operandData.operands[2].kind != PCOp_IMMEDIATE)
                    return;
                if (!(loop->step = inductionUpdate->operandData.operands[2].value.signed_value))
                    return;
            } else {
                return;
            }
        }
    }
    if (!inductionUpdate)
        return;

    if (inductionUpdate->block != compare->block &&
        !bitvectorgetbit(inductionUpdate->block->index, loop->backedge_dominators))
        return;
    if (loop->children) {
        for (child = loop->children; child; child = child->sibling) {
            if (bitvectorgetbit(inductionUpdate->block->index, child->memberblocks))
                return;
        }
    }
    loop->inductionUpdate = inductionUpdate;

    boundDefinition = NULL;
    for (definition = code_motion_register_definition_heads[inductionRegister]; definition;
         definition = definition->next) {
        if (bitvectorgetbit(definition->entry_index, data_00587fe4[loop->preheader->index].definition_sets[2])) {
            if (!boundDefinition) {
                boundDefinition = code_motion_entries[definition->entry_index].instruction;
                if (boundDefinition->opcode == PC_LI &&
                    boundDefinition->operandData.operands[1].kind == PCOp_IMMEDIATE) {
                    loop->lower = boundDefinition->operandData.operands[1].value.signed_value;
                    loop->lowerType = 1;
                } else if (boundDefinition->opcode == PC_LIS &&
                           boundDefinition->operandData.operands[1].kind == PCOp_IMMEDIATE) {
                    loop->lower = boundDefinition->operandData.operands[1].value.signed_value << 16;
                    loop->lowerType = 1;
                } else if (boundDefinition->opcode == PC_ADDI &&
                           boundDefinition->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                           boundDefinition->operandData.operands[1].value.reg == inductionRegister &&
                           (highLoad = boundDefinition->previous) && highLoad->opcode == PC_LIS &&
                           highLoad->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                           highLoad->operandData.operands[0].value.reg == inductionRegister) {
                    loop->lower = (highLoad->operandData.operands[1].value.signed_value << 16) +
                                  boundDefinition->operandData.operands[2].value.signed_value;
                    loop->lowerType = 1;
                } else {
                    loop->lowerType = 2;
                    break;
                }
            } else {
                loop->lowerType = 0;
                break;
            }
        }
    }
    if (!loop->lowerType)
        loop->lowerType = 2;

    if (loop->lowerType == 1 && loop->upperType == 1) {
        if (compareOpcode == PC_CMPI || compareOpcode == PC_CMP) {
            if (!compute_iteration_count(branch->opcode, conditionBit, loop->lower, loop->upper, loop->step,
                                         &loop->iterationCount))
                return;
        } else {
            if (!compute_unsigned_iteration_count(branch->opcode, conditionBit, loop->lower, loop->upper, loop->step,
                                                  &loop->iterationCount))
                return;
        }
        loop->isKnownCountingLoop = 1;
    } else if (loop->lowerType != 0 || loop->upperType != 0) {
        if (!fn_00522640(branch->opcode, conditionBit, loop->step, &loop->unknownCondition))
            return;
        loop->isUnknownCountingLoop = 1;
    }
}

void LoopDetection_TraverseLoopsPostorder(Loop *p)
{
    if (!p)
        return;
    while (p) {
        if (p->children) {
            LoopDetection_TraverseLoopsPostorder(p->children);
        }
        detect_counting_loop(p);
        p = p->sibling;
    }
}

void LoopDetection_ComputeLoopProperties(Loop *node)
{
    PCodeBlockLink *link;

    node->bodySize = 0;
    node->has_call = 0;
    node->uses_count_register = 0;
    node->skip_leaf_pass_4f = 1;
    node->isKnownCountingLoop = 0;
    node->isUnknownCountingLoop = 0;
    node->lowerType = 0;
    node->upperType = 0;
    node->iterationCount = -1;
    node->has_memory_barrier = 0;
    node->has_block_flag_40 = 0;

    for (link = node->blocks; link != NULL; link = link->next) {
        PCodeBlock *block = link->payload.block;
        PCodeInstruction *instruction;

        node->bodySize += block->instruction_count;
        if (block != node->body && (block->successors->next != NULL || block->predecessors->next != NULL)) {
            node->skip_leaf_pass_4f = 0;
        }
        if ((block->flags & 0x40) == 0x40) {
            node->has_block_flag_40 = 1;
        }

        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if ((instruction->flags & fLink) != 0) {
                node->has_call = 1;
            }
            if (instruction->opcode == PC_BCTRL || instruction->opcode == PC_BCTR || instruction->opcode == PC_BCCTR ||
                instruction->opcode == PC_MTCTR || instruction->opcode == PC_MFCTR) {
                node->uses_count_register = 1;
            } else if ((instruction->flags & fIsRead) != 0) {
                if (instruction->opcode == PC_LBZX || instruction->opcode == PC_LHZX ||
                    instruction->opcode == PC_LHAX || instruction->opcode == PC_LWZX ||
                    instruction->opcode == PC_LFSX || instruction->opcode == PC_LFDX) {
                    node->has_indexed_load = 1;
                }
            } else if ((instruction->flags & fIsWrite) != 0) {
                if (instruction->opcode == PC_STBX || instruction->opcode == PC_STHX ||
                    instruction->opcode == PC_STWX || instruction->opcode == PC_STFSX ||
                    instruction->opcode == PC_STFDX) {
                    node->has_indexed_store = 1;
                }
            } else if ((unsigned short)(instruction->opcode - 0x85) <= 2) {
                node->has_memory_barrier = 1;
            }
        }
    }
}

void compute_loop_properties_recursive(register Loop *node)
{
    register Loop *child;
    register Loop *grandchild;
    register Loop *greatGrandchild;
    register Loop *fourthLevelLoop;
    register Loop *fifthLevelLoop;
    register Loop *sixthLevelLoop;
    register Loop *seventhLevelLoop;
    for (; node != NULL; node = node->sibling) {
        if (node->children != NULL) {
            child = node->children;
            for (; child != NULL; child = child->sibling) {
                if (child->children != NULL) {
                    grandchild = child->children;
                    for (; grandchild != NULL; grandchild = grandchild->sibling) {
                        if (grandchild->children != NULL) {
                            greatGrandchild = grandchild->children;
                            for (; greatGrandchild != NULL; greatGrandchild = greatGrandchild->sibling) {
                                if (greatGrandchild->children != NULL) {
                                    fourthLevelLoop = greatGrandchild->children;
                                    for (; fourthLevelLoop != NULL; fourthLevelLoop = fourthLevelLoop->sibling) {
                                        if (fourthLevelLoop->children != NULL) {
                                            fifthLevelLoop = fourthLevelLoop->children;
                                            for (; fifthLevelLoop != NULL; fifthLevelLoop = fifthLevelLoop->sibling) {
                                                if (fifthLevelLoop->children != NULL) {
                                                    sixthLevelLoop = fifthLevelLoop->children;
                                                    for (; sixthLevelLoop != NULL;
                                                         sixthLevelLoop = sixthLevelLoop->sibling) {
                                                        if (sixthLevelLoop->children != NULL) {
                                                            seventhLevelLoop = sixthLevelLoop->children;
                                                            for (; seventhLevelLoop != NULL;
                                                                 seventhLevelLoop = seventhLevelLoop->sibling) {
                                                                if (seventhLevelLoop->children != NULL) {
                                                                    compute_loop_properties_recursive(
                                                                        seventhLevelLoop->children);
                                                                }
                                                                LoopDetection_ComputeLoopProperties(seventhLevelLoop);
                                                            }
                                                        }
                                                        LoopDetection_ComputeLoopProperties(sixthLevelLoop);
                                                    }
                                                }
                                                LoopDetection_ComputeLoopProperties(fifthLevelLoop);
                                            }
                                        }
                                        LoopDetection_ComputeLoopProperties(fourthLevelLoop);
                                    }
                                }
                                LoopDetection_ComputeLoopProperties(greatGrandchild);
                            }
                        }
                        LoopDetection_ComputeLoopProperties(grandchild);
                    }
                }
                LoopDetection_ComputeLoopProperties(child);
            }
        }
        LoopDetection_ComputeLoopProperties(node);
    }
}

void LoopDetection_ComputeLoopPropertiesRecursive(void)
{
    if (data_0058763c != NULL) {
        compute_loop_properties_recursive(data_0058763c);
    }
}
