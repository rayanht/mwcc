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

static void AddRef(IRONode *node, IRONode *t)
{
    if (t != NULL) {
        node->succ[node->numsucc++] = t->index;
        t->numpred++;
        t->referenced = 1;
    } else {
        CError_FATAL(109);
    }
}

static void AddNext(IRONode *node, IRONode *t)
{
    node->succ[node->numsucc++] = t->index;
    t->numpred++;
}

static void AddList(IRONode *node, SwitchInfo *info)
{
    SwitchCase *it;
    for (it = info->cases; it != NULL; it = it->next)
        AddRef(node, (IRONode *)it->label->stmt);
}

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

void IroFlowgraph_RebuildSuccPred(void)
{
    IRONode *node;
    UInt16 successorIndex;
    IRONode *successor;
    IROLinear *statement;
    SwitchInfo *branchList;
    SwitchCase *branch;
    ExceptionAction *entry;
    AsmOut references;
    SInt32 successorCount;
    SInt32 nodeIndex;
    IRONode *scanNode;
    CLabel *list;
    CLabel *target;
    Statement *entryList;

    for (list = Labels; list != NULL; list = list->next)
        list->stmt = NULL;

    for (scanNode = iro_flowgraph_head; scanNode != NULL; scanNode = scanNode->nextnode) {
        scanNode->referenced = 0;
        scanNode->numsucc = 0;
        scanNode->numpred = 0;
        scanNode->reachable = 0;
        scanNode->visited = 0;
        {
            IROLinear *labelStatement;
            if ((labelStatement = scanNode->first) != NULL && labelStatement->type == IROLinearLabel) {
                CLabel *label = labelStatement->u.label;
                label->stmt = (Statement *)scanNode;
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
                        AddRef(node, (IRONode *)target->stmt);
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        node->succ = oalloc(2 * sizeof(*node->succ));
                        AddNext(node, node->nextnode);
                        target = statement->u.label;
                        AddRef(node, (IRONode *)target->stmt);
                        break;
                    case IROLinearSwitch:
                        branchList = node->last->u.swtch.info;
                        for (branch = branchList->cases, successorCount = 1; branch != NULL; branch = branch->next)
                            successorCount++;
                        node->succ = oalloc(successorCount * sizeof(*node->succ));
                        AddList(node, branchList);
                        target = branchList->defaultlabel;
                        AddRef(node, (IRONode *)target->stmt);
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
                                AddRef(node, (IRONode *)target->stmt);
                            } else if (entry->kind == EAT_SPECIFICATION) {
                                target = entry->data.specification.label;
                                AddRef(node, (IRONode *)target->stmt);
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
                            AddRef(node, (IRONode *)target->stmt);
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

    for (scanNode = iro_flowgraph_head; scanNode != NULL; scanNode = scanNode->nextnode) {
        if (scanNode->numpred != 0)
            scanNode->pred = oalloc(scanNode->numpred * sizeof(*scanNode->pred));
        else
            scanNode->pred = NULL;
        scanNode->numpred = 0;
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        for (successorIndex = 0; successorIndex < node->numsucc; successorIndex++) {
            nodeIndex = node->index;
            successor = iroNodesByIndex[node->succ[successorIndex]];
            successor->pred[successor->numpred++] = nodeIndex;
        }
    }

    for (scanNode = iro_flowgraph_head; scanNode != NULL; scanNode = scanNode->nextnode) {
        if ((statement = scanNode->first) != NULL && statement->type == IROLinearLabel) {
            for (;;) {
                if (statement->type == IROLinearBeginCatch || statement->type == IROLinearEndCatch ||
                    statement->type == IROLinearEndCatchDtor) {
                    scanNode->referenced = 1;
                    break;
                }
                if (statement == scanNode->last)
                    break;
                statement = statement->next;
                if (statement == NULL)
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
