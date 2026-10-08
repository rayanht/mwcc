#define CERROR_FILE "IroPropagate.c"
#include "compiler/common.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroRangePropagation.h"
#include "compiler/CInt64.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BitVector.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CParser.h"
#include "compiler/CompilerTools.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/Registers.h"

SInt32 propagationIndex;
struct ReplacementCandidate *replacementCandidateTail;
struct ReplacementCandidate *replacementCandidate;
struct BitVector *data_00588018;
struct BitVector *availableExpressions;

#define BVGet(bv, i)                                                                                                   \
    (((UInt32)((SInt32)(i) >> 5) < (bv)->size) && (((bv)->bits[(i) >> 5] & ((UInt32)1 << ((i) & 31)))) != 0)
#define BVTEST(bv, i)                                                                                                  \
    (((UInt32)((SInt32)(i) >> 5) < (bv)->size) &&                                                                      \
     (((bv)->bits[(UInt32)((SInt32)(i) >> 5)] & ((UInt32)1 << ((i) & 31)))) != 0)

static int NeedProp(IROLinear *instr)
{
    if (instr->type == IROLinearFunccall)
        return 1;
    if (data_00551d6c[instr->nodetype] && (instr->type == IROLinearOp1Arg || instr->type == IROLinearOp2Arg))
        return 1;
    if (instr->type == IROLinearAsm)
        return 1;
    return 0;
}

/* Whether OBJECT is a local that may live in a register. */
unsigned int IroPropagate_IsRegisterEligible(Object *object)
{
    unsigned int result;
    if (object->datatype == DLOCAL && object->u.var.info != NULL) {
        result = 0U;
        if (object->u.var.info->noregister == 0)
            result += 1U;
        return result;
    }
    result = 0U;
    return result;
}

int fn_004592e0(IROLinear *node)
{
    Object *left;
    Object *right;
    int leftFlag;
    int rightFlag;
    IROLinear *record;
    IROLinear *reference;
    Object *object;

    if (node->type == IROLinearOp2Arg && node->nodetype == EASS) {
        record = node->u.diadic.left;
        left = IroDump_GetObjRef(record);
        if (left != NULL && left->type == node->rtype) {
            object = left;
            if (!is_volatile_object(object)) {
                reference = node->u.diadic.right;
                if (fn_0044d520(reference) != 0)
                    return 1;
                record = node->u.diadic.right;
                right = IroDump_GetObjRef(record);
                if (right != NULL) {
                    object = right;
                    if (!is_volatile_object(object)) {
                        if (IroUtil_AreTypesEqual(left->type, node->rtype) != 0) {
                            if (node->u.diadic.right->u.monadic->u.node->data.objref->datatype != DDATA) {
                                if (left->datatype == DLOCAL && left->u.var.info != NULL) {
                                    leftFlag = 0;
                                    if (left->u.var.info->noregister == 0)
                                        leftFlag = 1;
                                } else {
                                    leftFlag = 0;
                                }
                                if (leftFlag) {
                                    if (right->datatype == DLOCAL && right->u.var.info != NULL) {
                                        rightFlag = 0;
                                        if (right->u.var.info->noregister == 0)
                                            rightFlag = 1;
                                    } else {
                                        rightFlag = 0;
                                    }
                                    if (!rightFlag)
                                        return 0;
                                }
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void IRO_CopyAndConstantPropagation(void)
{
    IRONode *block;
    IROLinear *instruction;
    ReplacementCandidate *definition;
    VarRecord *variableId;
    VarRecord *propagatedVariableId;
    ReplacementCandidate *node;
    BitVector *incoming;
    AsmOut list;
    SInt32 candidateIndex;
    UInt16 predecessorIndex;
    UInt8 changed;
    UInt16 bit;
    ENode *replacement;
    ENode *constant;
    ReplacementCandidate *nextDefinition;
    IROLinear *operand;
    ENode *expression;
    Object *candidate;

    replacementCandidate = NULL;
    propagationIndex = 0;
    data_005880c8 = 0;
    block = iro_flowgraph_head;
    while (block != NULL) {
        instruction = block->first;
        for (;;) {
            if (instruction == NULL)
                break;
            if (fn_004592e0(instruction) != 0) {
                variableId = IroVars_GetOperandVarRecord(instruction);
                if (variableId != NULL) {
                    IroDump_Print("Found propagatable assignment at: %d\n", instruction->index);
                    definition = (ReplacementCandidate *)oalloc(sizeof(ReplacementCandidate));
                    definition->node = instruction;
                    definition->next = NULL;
                    propagationIndex = propagationIndex + 1;
                    definition->index = propagationIndex;
                    definition->uvarIndex = variableId->index;
                    definition->value = instruction->u.diadic.right;
                    definition->var = NULL;
                    definition->object = IroDump_GetObjRef(instruction->u.diadic.right);
                    definition->block = block;
                    if (definition->object != NULL)
                        definition->var = fn_0044ba70(definition->object, 0, 1);
                    if (replacementCandidate == NULL)
                        replacementCandidate = definition;
                    else
                        replacementCandidateTail->next = definition;
                    replacementCandidateTail = definition;
                }
            }
            if (instruction == block->last)
                break;
            instruction = instruction->next;
        }
        block = block->nextnode;
    }

    IroVars_CheckTimedLongjmp();
    block = iro_flowgraph_head;
    nextDefinition = replacementCandidate;
    IroBitVect_AllocateBitVector(&data_00588018, iroVarCount + 1);
    while (block != NULL) {
        IroBitVect_AllocateBitVector(&block->in, propagationIndex);
        if (block != iro_flowgraph_head)
            IroBitVect_SetAllBits(block->in);
        IroBitVect_AllocateBitVector(&block->kill, propagationIndex);
        IroBitVect_AllocateBitVector(&block->gen, propagationIndex);
        IroBitVect_AllocateBitVector(&block->copyOut, propagationIndex);
        block->out = NULL;
        instruction = block->first;
        for (;;) {
            if (instruction == NULL)
                break;
            if (NeedProp(instruction)) {
                IroBitVect_ClearBitVector(data_00588018);
                fn_0044b2d0(instruction);
                for (node = replacementCandidate; node != NULL; node = node->next) {
                    bit = node->uvarIndex;
                    if (BVGet(data_00588018, bit)) {
                        IroBitVect_SetBit(node->index, block->kill);
                        IroBitVect_ClearBit(node->index, block->gen);
                    }
                    if (node->var != NULL) {
                        if (BVTEST(data_00588018, (SInt32)node->var->index)) {
                            IroBitVect_SetBit(node->index, block->kill);
                            IroBitVect_ClearBit(node->index, block->gen);
                        }
                    }
                }
            }
            while (nextDefinition != NULL && nextDefinition->node == instruction) {
                IroBitVect_SetBit(nextDefinition->index, block->gen);
                nextDefinition = nextDefinition->next;
            }
            if (instruction == block->last)
                break;
            instruction = instruction->next;
        }
        IroBitVect_CopyBitVector(block->in, block->copyOut);
        IroBitVect_Subtract(block->kill, block->copyOut);
        IroBitVect_Or(block->gen, block->copyOut);
        block = block->nextnode;
    }

    IroVars_CheckTimedLongjmp();
    IroBitVect_AllocateBitVector(&incoming, propagationIndex);
    do {
        block = iro_flowgraph_head;
        changed = 0;
        while (block != NULL) {
            if (block == iro_flowgraph_head) {
                IroBitVect_ClearBitVector(incoming);
            } else {
                IroBitVect_SetAllBits(incoming);
                for (predecessorIndex = 0; predecessorIndex < block->numpred; predecessorIndex++)
                    IroBitVect_Intersect(iroNodesByIndex[block->pred[predecessorIndex]]->copyOut, incoming);
            }
            if (IroBitVect_AreEqual(incoming, block->in) == 0) {
                changed = 1;
                IroBitVect_CopyBitVector(incoming, block->in);
            }
            IroBitVect_CopyBitVector(block->in, block->copyOut);
            IroBitVect_Subtract(block->kill, block->copyOut);
            IroBitVect_Or(block->gen, block->copyOut);
            block = block->nextnode;
        }
    } while (changed);

    IroVars_CheckTimedLongjmp();
    nextDefinition = replacementCandidate;
    for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
        availableExpressions = block->in;
        instruction = block->first;
        for (;;) {
            if (instruction == NULL)
                break;
            if (IroDump_GetObjRef(instruction) != NULL && (instruction->flags & IROLF_Assigned) == 0 &&
                (instruction->flags & 0x4000) == 0) {
                operand = instruction->u.monadic;
                expression = operand->u.node;
                propagatedVariableId = fn_0044ba70(expression->data.objref, 0, 1);
                if (propagatedVariableId != NULL) {
                    for (node = replacementCandidate; node != NULL; node = node->next) {
                        if (node->uvarIndex != propagatedVariableId->index)
                            continue;
                        if (!BVTEST(availableExpressions, (SInt32)node->index))
                            continue;
                        if (block->loopdepth > node->block->loopdepth)
                            continue;
                        if (node->value->type == IROLinearOperand &&
                            IroUtil_IsTypeSame(instruction->rtype, node->node->u.diadic.left->rtype) != 0) {
                            constant = IrOptimizer_NewENode(node->value->u.node->type);
                            *constant = *node->value->u.node;
                            if (constant->type == EINTCONST) {
                                IroJump_ConvertCInt64ToType(&constant->data.intval, instruction->rtype);
                                constant->rtype = instruction->rtype;
                            }
                            instruction->u.monadic->type = IROLinearNop;
                            instruction->type = IROLinearOperand;
                            instruction->u.node = constant;
                        } else if (node->object != NULL &&
                                   IroUtil_IsTypeSame(instruction->rtype, node->node->rtype) != 0) {
                            instruction->u.monadic->u.node = create_objectrefnode(node->object);
                        }
                        IroDump_Print("Found propagation at %d from %d\n", instruction->index, node->node->index);
                        break;
                    }
                }
            } else if (instruction->type == IROLinearAsm) {
                fn_00462d70(instruction->u.asm_stmt, &list);
                for (candidateIndex = 0; candidateIndex < list.numoperands; candidateIndex++) {
                    if (list.operands[candidateIndex].type != 0)
                        continue;
                    if (list.operands[candidateIndex].offset != 0)
                        continue;
                    candidate = list.operands[candidateIndex].object;
                    if (candidate->type->size != list.operands[candidateIndex].size)
                        continue;
                    propagatedVariableId = fn_0044ba70(list.operands[candidateIndex].object, 0, 1);
                    if (propagatedVariableId == NULL)
                        continue;
                    for (node = replacementCandidate; node != NULL; node = node->next) {
                        if (node->uvarIndex != propagatedVariableId->index)
                            continue;
                        if (!BVTEST(availableExpressions, (SInt32)node->index))
                            continue;
                        if (node->node->rtype->size != list.operands[candidateIndex].size)
                            continue;
                        if (node->object != NULL) {
                            replacement = create_objectrefnode(node->object);
                        } else if (node->value->type == IROLinearOperand) {
                            replacement = node->value->u.node;
                        } else {
                            CError_FATAL(532);
                        }
                        InlineAsmPPC_ReplaceObjectReferenceArguments(instruction->u.asm_stmt,
                                                                     list.operands[candidateIndex].object, replacement);
                        break;
                    }
                }
            }
            if (NeedProp(instruction)) {
                IroBitVect_ClearBitVector(data_00588018);
                fn_0044b2d0(instruction);
                for (node = replacementCandidate; node != NULL; node = node->next) {
                    bit = node->uvarIndex;
                    if (BVGet(data_00588018, bit))
                        IroBitVect_ClearBit(node->index, availableExpressions);
                    if (node->var != NULL) {
                        if (BVTEST(data_00588018, (SInt32)node->var->index))
                            IroBitVect_ClearBit(node->index, availableExpressions);
                    }
                }
                while (nextDefinition != NULL && nextDefinition->node == instruction) {
                    IroBitVect_SetBit(nextDefinition->index, availableExpressions);
                    nextDefinition = nextDefinition->next;
                }
            }
            if (instruction == block->last)
                break;
            instruction = instruction->next;
        }
    }
    IroVars_CheckTimedLongjmp();
}

void IroPropagate_PropagateExpressions(void)
{
    int wordIndex;
    IROLinear *node, *currentNode;
    int eligibleLocal, isCandidate;
    Object *localObject;
    ReplacementCandidate *matchingCandidate;
    VarRecord *localRegister;
    ReplacementCandidate *candidateToMark, *candidateToInvalidate;
    VarRecord *currentRegister;
    ReplacementCandidate *newCandidate, *previousCandidate;
    char replacementKind;
    IROLinear *record;
    int invalidatesCandidates;
    Object *referencedObject;
    unsigned int intersects;
    VarRecord *objectRegister;
    VarRecord *candidateRegister;
    IROLinear *conversion;
    ENode *replacement;
    IRONode *block;
    IROList initialRange, replacementRange, invalidationRange;
    unsigned short registerIndex;
    block = iro_flowgraph_head;
    while (block != NULL) {
        replacementCandidate = NULL;
        replacementCandidateTail = NULL;
        propagationIndex = 0;
        node = block->first;
        for (;;) {
            if (node == NULL)
                break;
            IroUtil_InitList(&initialRange);
            if (node->type == IROLinearOp2Arg && node->nodetype == EASS && (node->flags & IROLF_Reffed) == 0) {
                localObject = IroDump_GetObjRef(node->u.diadic.left);
                if (localObject != NULL && is_volatile_object(localObject) == 0) {
                    if (localObject->datatype == DLOCAL && localObject->u.var.info != NULL) {
                        eligibleLocal = 0;
                        if (localObject->u.var.info->noregister == 0) {
                            eligibleLocal = eligibleLocal + 1;
                        }
                    } else {
                        eligibleLocal = 0;
                    }
                    if (eligibleLocal != 0 && !(node->u.diadic.right->type == IROLinearOperand &&
                                                node->u.diadic.right->u.node->type == ESTRINGCONST)) {
                        fn_0044f350(node->u.diadic.right);
                        if (data_00587630 != 0) {
                            isCandidate = 0;
                        } else {
                            isCandidate = 1;
                        }
                    } else {
                        isCandidate = 0;
                    }
                } else {
                    isCandidate = 0;
                }
            } else {
                isCandidate = 0;
            }
            if (isCandidate != 0) {
                candidateRegister = IroVars_GetOperandVarRecord(node);
                if (candidateRegister != NULL) {
                    IroDump_Print("Found propagatable expression assignment at: %d\n", node->index);
                    newCandidate = (ReplacementCandidate *)oalloc(sizeof(ReplacementCandidate));
                    newCandidate->node = node;
                    newCandidate->next = NULL;
                    propagationIndex += 1;
                    newCandidate->index = propagationIndex;
                    newCandidate->varIndex = candidateRegister->index;
                    newCandidate->value = node->u.diadic.right;
                    newCandidate->uses = 0;
                    newCandidate->block = block;
                    fn_0044f350(node->u.diadic.right);
                    newCandidate->registers = data_00552b88;
                    if (replacementCandidate == NULL) {
                        replacementCandidate = newCandidate;
                        newCandidate->previous = NULL;
                    } else {
                        replacementCandidateTail->next = newCandidate;
                        newCandidate->previous = replacementCandidateTail;
                    }
                    replacementCandidateTail = newCandidate;
                }
            }
            if (IroDump_GetObjRef(node) != NULL && (node->flags & IROLF_Assigned) == 0) {
                localRegister = fn_0044ba70(node->u.monadic->u.node->data.objref, 0, 1);
                if (localRegister != NULL) {
                    previousCandidate = replacementCandidateTail;
                    while (previousCandidate != NULL) {
                        if (previousCandidate->uvarIndex == localRegister->index) {
                            previousCandidate->uses += 1;
                        }
                        previousCandidate = previousCandidate->previous;
                    }
                }
            }
            if (node == block->last)
                break;
            node = node->next;
        }
        IroBitVect_AllocateBitVector(&availableExpressions, propagationIndex);
        currentNode = block->first;
        for (;;) {
            if (currentNode == NULL)
                break;
            if (IroDump_GetObjRef(currentNode) != NULL && (currentNode->flags & IROLF_Assigned) == 0 &&
                (currentNode->flags & 16384) == 0) {
                if ((currentRegister = fn_0044ba70(currentNode->u.monadic->u.node->data.objref, 0, 1)) != NULL) {
                    matchingCandidate = replacementCandidate;
                    while (matchingCandidate != NULL) {
                        if (matchingCandidate->uvarIndex == currentRegister->index &&
                            (matchingCandidate->index >> 5) < availableExpressions->size &&
                            (1 << matchingCandidate->index &
                             availableExpressions->bits[matchingCandidate->index >> 5]) != 0) {
                            if (matchingCandidate->value->type == IROLinearOperand &&
                                IroUtil_IsTypeSame(currentNode->rtype, matchingCandidate->node->u.diadic.left->rtype) !=
                                    0) {
                                replacement = IrOptimizer_NewENode(matchingCandidate->value->u.node->type);
                                *replacement = *matchingCandidate->value->u.node;
                                if ((replacementKind = replacement->type) == EINTCONST) {
                                    IroJump_ConvertCInt64ToType(&replacement->data.intval, currentNode->rtype);
                                    replacement->rtype = currentNode->rtype;
                                } else if (replacementKind == EOBJREF) {
                                    record = IroUtil_FindNextUse(currentNode);
                                    if (record != NULL && (record->flags & IROLF_Assigned) != 0) {
                                        currentNode->flags |= IROLF_Assigned;
                                    }
                                }
                                currentNode->u.monadic->type = IROLinearNop;
                                currentNode->type = IROLinearOperand;
                                currentNode->u.node = replacement;
                            } else if (IroDump_GetObjRef(matchingCandidate->value) != NULL &&
                                       IroUtil_IsTypeSame(currentNode->rtype, matchingCandidate->node->rtype) != 0) {
                                currentNode->u.monadic->u.node =
                                    create_objectrefnode(matchingCandidate->value->u.monadic->u.node->data.objref);
                            } else if (IroUtil_IsTypeSame(currentNode->rtype, matchingCandidate->node->rtype) != 0 &&
                                       matchingCandidate->uses == 1) {
                                IroUtil_InitList(&replacementRange);
                                IroUtil_CopyLinearToList(matchingCandidate->node->u.diadic.right, &replacementRange);
                                if (matchingCandidate->node->rtype->type == TYPEFLOAT) {
                                    conversion = IrOptimizer_NewLinear(IROLinearOp1Arg);
                                    linear_index_counter += 1;
                                    conversion->index = linear_index_counter;
                                    conversion->rtype = matchingCandidate->node->rtype;
                                    conversion->nodetype = ETYPCON;
                                    conversion->nodeflags = 128;
                                    conversion->u.monadic = replacementRange.tail;
                                    IroUtil_AppendLinear(conversion, &replacementRange);
                                }
                                IroUtil_InsertLinearRangeAfter(replacementRange.head, replacementRange.tail,
                                                               currentNode);
                                IroUtil_ReplaceFirstReference(currentNode, replacementRange.tail);
                                currentNode = replacementRange.tail;
                            }
                            IroDump_Print("Found expression propagation at %d from %d\n", currentNode->index,
                                          matchingCandidate->node->index);
                            break;
                        }
                        matchingCandidate = matchingCandidate->next;
                    }
                }
            }
            if (currentNode->type != IROLinearNop) {
                if (currentNode->type == IROLinearFunccall) {
                    invalidatesCandidates = 1;
                } else if (data_00551d6c[currentNode->nodetype] != 0 &&
                           (currentNode->type == IROLinearOp1Arg || currentNode->type == IROLinearOp2Arg)) {
                    invalidatesCandidates = 1;
                } else if (currentNode->type == IROLinearAsm) {
                    invalidatesCandidates = 1;
                } else {
                    invalidatesCandidates = 0;
                }
                if (invalidatesCandidates != 0) {
                    IroBitVect_ClearBitVector(data_00588018);
                    fn_0044b2d0(currentNode);
                    candidateToInvalidate = replacementCandidate;
                    for (; candidateToInvalidate != NULL; candidateToInvalidate = candidateToInvalidate->next) {
                        registerIndex = candidateToInvalidate->uvarIndex;
                        if ((registerIndex >> 5) < data_00588018->size &&
                            (1 << registerIndex & data_00588018->bits[registerIndex >> 5]) != 0) {
                            IroBitVect_ClearBit(candidateToInvalidate->index, availableExpressions);
                        }
                        if ((referencedObject = IroDump_GetObjRef(candidateToInvalidate->node->u.diadic.right)) !=
                            NULL) {
                            objectRegister = fn_0044ba70(referencedObject, 0, 1);
                            if (objectRegister == NULL)
                                continue;
                            wordIndex = objectRegister->index;
                            wordIndex >>= 5;
                            if (wordIndex >= data_00588018->size)
                                continue;
                            if ((1 << objectRegister->index & data_00588018->bits[wordIndex]) == 0)
                                continue;
                            IroBitVect_ClearBit(candidateToInvalidate->index, availableExpressions);
                        } else {
                            IroUtil_InitList(&invalidationRange);
                            data_00552b88 = candidateToInvalidate->registers;
                            IroCSE_CollectExpressionVarRefsAndFlags(candidateToInvalidate->node->u.diadic.right);
                            intersects = IroBitVect_Intersects(data_00588018, candidateToInvalidate->registers);
                            if (intersects == 0)
                                continue;
                            IroBitVect_ClearBit(candidateToInvalidate->index, availableExpressions);
                        }
                    }
                    candidateToMark = replacementCandidate;
                    for (; candidateToMark != NULL; candidateToMark = candidateToMark->next) {
                        data_00552b88 = candidateToMark->registers;
                        IroCSE_CollectExpressionVarRefsAndFlags(candidateToMark->node->u.diadic.right);
                        if (candidateToMark->node == currentNode &&
                            ((candidateToMark->uvarIndex >> 5) >= candidateToMark->registers->size ||
                             (1 << candidateToMark->uvarIndex &
                              candidateToMark->registers->bits[candidateToMark->uvarIndex >> 5]) == 0)) {
                            IroBitVect_SetBit(candidateToMark->index, availableExpressions);
                        }
                    }
                }
            }
            if (currentNode == block->last)
                break;
            currentNode = currentNode->next;
        }
        block = block->nextnode;
    }
    IroVars_CheckTimedLongjmp();
}

void check_range_for_type(ERange *p, Type *type)
{
    TypeIntegral *t = (TypeIntegral *)type;

    if (p == NULL) {
        return;
    }
    if (type->type != TYPEINT) {
        p->type = 3;
        return;
    }
    if (t == &stchar || t == &stsignedchar) {
        if (CInt64_Greater(p->upper, signed_char_max) || CInt64_Less(p->lower, type_range_minimum)) {
            p->type = 3;
        }
    } else if (t == &stunsignedchar) {
        if (CInt64_GreaterU(p->upper, data_005539c8)) {
            p->type = 3;
        }
    } else if (t == &stsignedshort) {
        if (CInt64_Greater(p->upper, int16_max) || CInt64_Less(p->lower, data_005539d8)) {
            p->type = 3;
        }
    } else if (t == &stunsignedshort) {
        if (CInt64_GreaterU(p->upper, data_005539e0)) {
            p->type = 3;
        }
    } else if (t == &stsignedint) {
        if (CInt64_Greater(p->upper, int32_max) || CInt64_Less(p->lower, type_range_lower_bound)) {
            p->type = 3;
        }
    } else if (t == &stunsignedint) {
        if (CInt64_GreaterU(p->upper, data_00553a10)) {
            p->type = 3;
        }
    } else if (t == &stsignedlong) {
        if (CInt64_Greater(p->upper, range_int32_max) || CInt64_Less(p->lower, type_range_min)) {
            p->type = 3;
        }
    } else if (t == &stunsignedlong) {
        if (CInt64_GreaterU(p->upper, data_00553a28)) {
            p->type = 3;
        }
    } else if (t == &stsignedlonglong || t == &stunsignedlonglong) {
        p->type = 3;
    }
}

int initialize_node_range(IROLinear *record)
{
    ERange *storage;
    ENode *operand;
    ERange *result;
    ERange *value;
    ERange *emptyStorage;
    switch (record->u.node->type) {
        case EOBJREF:
            record->range = NULL;
            break;
        case EINTCONST:
            storage = (ERange *)oalloc(18);
            storage->type = 0;
            record->range = storage;
            operand = record->u.node;
            result = record->range;
            result->lower = operand->data.intval;
            value = record->range;
            value->upper = result->lower;
            break;
        case EFLOATCONST:
        case ESTRINGCONST:
            emptyStorage = (ERange *)oalloc(18);
            emptyStorage->type = 0;
            record->range = emptyStorage;
    }
    return 1;
}
