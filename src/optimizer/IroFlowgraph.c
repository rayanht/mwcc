#define CERROR_FILE "IroFlowgraph.c"
#include "compiler/common.h"
#include "compiler/IroFlowgraph.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BitVector.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CPrec.h"
#include "compiler/CompilerTools.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/Switch.h"
#include <string.h>

struct IRONode *iroNodeTail;


void fn_0044a640(IROLinear *value)
{
    IRONode *node;
    IRONode *tail;

    node = (IRONode *)oalloc(sizeof(*node));
    node->index = iro_node_count;
    node->numsucc = 0U;
    node->succ = NULL;
    node->numpred = 0U;
    node->pred = NULL;
    node->first = value;
    node->last = value;
    node->in = NULL;
    node->out = NULL;
    node->gen = NULL;
    node->kill = NULL;
    node->x26 = 0;
    node->copyOut = NULL;
    node->dom = NULL;
    node->nextnode = NULL;
    node->reachable = 0U;
    node->visited = 0U;
    node->mustreach = 0U;
    node->referenced = 0U;
    node->loopdepth = 0U;
    iro_node_count += 1U;
    if (iro_flowgraph_head == NULL)
        iro_flowgraph_head = node;
    else {
        tail = iroNodeTail;
        tail->nextnode = node;
    }
    iroNodeTail = node;
}

static void AddNext(IRONode *node, IRONode *t)
{
    node->succ[node->numsucc++] = t->index;
    t->numpred++;
}

static void AddRef(IRONode *node, CLabel *label)
{
    IRONode *target = (IRONode *)label->stmt;

    if (target != NULL) {
        AddNext(node, target);
        target->referenced = 1;
        return;
    }
    CError_FATAL(109);
}

static void AddList(IRONode *node)
{
    SwitchInfo *info;
    SwitchCase *branch;
    SInt32 count;

    info = node->last->u.swtch.info;
    for (branch = info->cases, count = 1; branch != NULL; branch = branch->next)
        count++;
    node->succ = oalloc(count * sizeof(*node->succ));
    for (branch = info->cases; branch != NULL; branch = branch->next)
        AddRef(node, branch->label);
    AddRef(node, info->defaultlabel);
}

static void AddPred(int index, int pred)
{
    IRONode *node = iroNodesByIndex[index];

    node->pred[node->numpred++] = pred;
}

void IroFlowgraph_RebuildSuccPred(void)
{
    IRONode *node;
    UInt16 successorIndex;
    IROLinear *statement;
    ExceptionAction *entry;
    AsmOut references;
    SInt32 successorCount;
    SInt32 nodeIndex;
    CLabel *list;
    CLabel *target;
    Statement *entryList;

    for (list = Labels; list != NULL; list = list->next)
        list->stmt = NULL;

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        node->referenced = 0;
        node->numsucc = 0;
        node->numpred = 0;
        node->reachable = 0;
        node->visited = 0;
        {
            IROLinear *labelStatement;
            if ((labelStatement = node->first) != NULL && labelStatement->type == IROLinearLabel) {
                CLabel *label = labelStatement->u.label;
                label->stmt = (Statement *)node;
            }
        }
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        if (node->first == NULL) {
            if (node->nextnode != NULL) {
                node->succ = oalloc(sizeof(*node->succ));
                AddNext(node, node->nextnode);
            }
        } else {
            statement = node->last;
            for (;;) {
                switch (statement->type) {
                    case IROLinearGoto:
                        node->succ = oalloc(sizeof(*node->succ));
                        target = statement->u.label;
                        AddRef(node, target);
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        node->succ = oalloc(2 * sizeof(*node->succ));
                        AddNext(node, node->nextnode);
                        target = statement->u.label;
                        AddRef(node, target);
                        break;
                    case IROLinearSwitch:
                        AddList(node);
                        break;
                    case IROLinearFunccall:
                        successorCount = 1;
                        entryList = statement->stmt;
                        for (entry = entryList->dobjstack; entry != NULL; entry = entry->next)
                            if (entry->kind == EAT_CATCHBLOCK || entry->kind == EAT_SPECIFICATION)
                                successorCount++;
                        node->succ = oalloc(successorCount * sizeof(*node->succ));
                        AddNext(node, node->nextnode);
                        entryList = statement->stmt;
                        for (entry = entryList->dobjstack; entry != NULL; entry = entry->next) {
                            if (entry->kind == EAT_CATCHBLOCK) {
                                target = entry->data.catch_block.label;
                                AddRef(node, target);
                            } else if (entry->kind == EAT_SPECIFICATION) {
                                target = entry->data.specification.label;
                                AddRef(node, target);
                            }
                        }
                        break;
                    case IROLinearAsm:
                        fn_00462d70(statement->u.asm_stmt, &references);
                        successorCount = 0;
                        if (references.noFallthrough == 0)
                            successorCount = 1;
                        successorCount += references.numlabels;
                        node->succ = oalloc(successorCount * sizeof(*node->succ));
                        if (references.noFallthrough == 0)
                            AddNext(node, node->nextnode);
                        for (successorIndex = 0; successorIndex < references.numlabels; successorIndex++) {
                            target = references.labels[successorIndex];
                            AddRef(node, target);
                        }
                        break;
                    case IROLinearReturn:
                    case IROLinearEnd:
                        break;
                    case IROLinearOp2Arg:
                        if (statement->nodetype == ECOMMA) {
                            statement = statement->u.diadic.right;
                            continue;
                        }
                        /* fall through */
                    default:
                        if (node->nextnode != NULL) {
                            node->succ = oalloc(sizeof(*node->succ));
                            AddNext(node, node->nextnode);
                        }
                        break;
                }
                break;
            }
        }
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        if (node->numpred != 0)
            node->pred = oalloc(node->numpred * sizeof(*node->pred));
        else
            node->pred = NULL;
        node->numpred = 0;
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        for (successorIndex = 0; successorIndex < node->numsucc; successorIndex++)
            AddPred(node->succ[successorIndex], node->index);
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        IROLinear *labelStatement;
        if ((labelStatement = node->first) != NULL && labelStatement->type == IROLinearLabel) {
            for (;;) {
                if (labelStatement->type == IROLinearBeginCatch || labelStatement->type == IROLinearEndCatch ||
                    labelStatement->type == IROLinearEndCatchDtor) {
                    node->referenced = 1;
                    break;
                }
                if (labelStatement == node->last)
                    break;
                labelStatement = labelStatement->next;
                if (labelStatement == NULL)
                    break;
            }
        }
    }
}

void IroFlowgraph_ComputeDom(void)
{
    BitVector *local;
    IRONode *p;
    SInt32 changed;
    SInt32 i;
    IroBitVect_AllocateBitVector(&iro_flowgraph_head->dom, iro_node_count);
    IroBitVect_SetBit(iro_flowgraph_head->index, iro_flowgraph_head->dom);
    for (p = iro_flowgraph_head->nextnode; p != NULL; p = p->nextnode) {
        IroBitVect_AllocateBitVector(&p->dom, iro_node_count);
        IroBitVect_SetAllBits(p->dom);
    }
    IroBitVect_AllocateBitVector(&local, iro_node_count);
    do {
        changed = 0;
        for (p = iro_flowgraph_head->nextnode; p != NULL; p = p->nextnode) {
            if (p->numpred > 0) {
                IroBitVect_SetAllBits(local);
                for (i = 0; i < p->numpred; i++)
                    IroBitVect_Intersect(iroNodesByIndex[p->pred[i]]->dom, local);
                IroBitVect_SetBit(p->index, local);
            } else {
                IroBitVect_ClearBitVector(local);
                IroBitVect_SetBit(p->index, local);
            }
            if (IroBitVect_AreEqual(local, p->dom) == 0) {
                IroBitVect_CopyBitVector(local, p->dom);
                changed = 1;
            }
        }
    } while (changed);
}

void IRO_BuildflowGraph(IROLinear *source)
{
    CLabel *label;
    ExceptionAction *exception;
    IROLinear *linear;
    IROLinear *next;
    IROLinear *record;
    AsmOut info;
    int done;
    IRONode *block;
    int i;

    for (label = Labels; label != NULL; label = label->next)
        label->stmt = NULL;
    iro_node_count = 0;
    iro_flowgraph_head = iroNodeTail = data_00587fac = NULL;
    linear = source;
    while (linear != NULL) {
        fn_0044a640(linear);
        if (linear->type == IROLinearLabel)
            ((CLabel *)linear->u.label)->stmt = (Statement *)iroNodeTail;
        done = 0;
        while (!done && (next = linear->next) != NULL && (next->flags & 1) == 0) {
            switch (linear->type) {
                case IROLinearGoto:
                case IROLinearReturn:
                case IROLinearEntry:
                case IROLinearExit:
                case IROLinearEnd:
                    done = 1;
                    break;
                case IROLinearIf:
                case IROLinearIfNot:
                case IROLinearSwitch:
                    done = 1;
                insert_label:
                    if (next->type == IROLinearLabel) {
                        record = IrOptimizer_NewLinear(IROLinearNop);
                        linear_index_counter++;
                        record->index = linear_index_counter;
                        record->next = linear->next;
                        linear->next = record;
                    }
                    break;
                case IROLinearFunccall:
                    for (exception = linear->stmt->dobjstack; exception != NULL; exception = exception->next) {
                        if (exception->kind == EAT_CATCHBLOCK || exception->kind == EAT_SPECIFICATION) {
                            done = 1;
                            goto insert_label;
                        }
                    }
                    break;
                case IROLinearAsm:
                    fn_00462d70(linear->u.asm_stmt, &info);
                    if (info.numlabels != 0)
                        done = 1;
                    break;
            }
            if (!done)
                linear = linear->next;
        }
        if (linear->type == IROLinearEnd)
            data_00587fac = iroNodeTail;
        iroNodeTail->last = linear;
        linear = linear->next;
    }
    iroNodesByIndex = (IRONode **)oalloc(iro_node_count * sizeof(*iroNodesByIndex));
    block = iro_flowgraph_head;
    for (i = 0; block != NULL; block = block->nextnode) {
        iroNodesByIndex[i] = block;
        i++;
    }
    IroFlowgraph_RebuildSuccPred();
    IroFlowgraph_ComputeDom();
    IroVars_CheckTimedLongjmp();
}

struct IRONode *IroFlowgraph_NewNode(void)
{
    IRONode *record;
    UInt16 number;
    record = (IRONode *)oalloc(60U);
    memset(record, 0, 60U);
    number = iro_node_count;
    record->index = number;
    iro_node_count += 1U;
    record->nextnode = 0U;
    return record;
}
