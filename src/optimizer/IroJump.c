#define CERROR_FILE "IRO_RemoveLabels.c"
#include "compiler/common.h"
#include "compiler/IroJump.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroFlowgraph.h"
#include "compiler/IroLoop.h"
#include "compiler/IroRangePropagation.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"

SInt32 chain_label(CLabel **label)
{
    IRONode *node;
    IROLinear *p;
    CLabel *saved;

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        if (node->first != NULL && node->first->type == IROLinearLabel && node->first->u.label == *label) {
            p = node->first->next;
            saved = NULL;
            while (p != NULL && (p->type == IROLinearLabel || p->type == IROLinearNop)) {
                if (p->type == IROLinearLabel)
                    saved = p->u.label;
                p = p->next;
            }
            if (p->type == IROLinearGoto && *label != p->u.label) {
                *label = p->u.label;
                IroDump_Print("Chaining goto at %d\n", p->index);
                return 1;
            }
            if (saved != NULL && *label != saved)
                *label = saved;
            return 0;
        }
    }
    return 0;
}

SInt32 IRO_DoJumpChaining(void)
{
    SInt32 changed = 0;
    Boolean didChange;
    IRONode *block;
    SwitchInfo *targets;
    SwitchCase *target;
    CLabel **destination;

    do {
        didChange = 0;
        for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
            if (block->first == NULL)
                continue;
            switch (block->last->type) {
                case IROLinearGoto:
                    destination = (CLabel **)&block->last->u.label;
                    if (chain_label(destination))
                        didChange = 1;
                    break;
                case IROLinearIf:
                case IROLinearIfNot:
                    destination = (CLabel **)&block->last->u.label;
                    if (chain_label(destination))
                        didChange = 1;
                    break;
                case IROLinearSwitch:
                    targets = block->last->u.swtch.info;
                    for (target = targets->cases; target != NULL; target = target->next) {
                        destination = &target->label;
                        if (chain_label(destination))
                            didChange = 1;
                    }
                    destination = &targets->defaultlabel;
                    if (chain_label(destination))
                        didChange = 1;
                    break;
            }
        }
        changed |= didChange;
        IroVars_CheckTimedLongjmp();
    } while (didChange);
    return changed;
}

void IroJump_MarkReachable(IRONode *param)
{
    Boolean changed;
    UInt16 i;
    IRONode *node;
    IRONode *t;

    param->reachable = 1;
    do {
        changed = 0;
        for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
            if (node->reachable && !node->visited) {
                for (i = 0; i < node->numsucc; i++) {
                    t = iroNodesByIndex[node->succ[i]];
                    if (!t->reachable) {
                        changed = 1;
                        t->reachable = 1;
                    }
                }
                node->visited = 1;
            }
        }
    } while (changed);
}

SInt32 IRO_RemoveUnreachable(void)
{
    IRONode *previous;
    IRONode *current;
    SInt32 changed = 0;
    IROLinear *block;
    IROLinear *entry;
    ExceptionAction **link;
    ExceptionAction *use;

    IroFlowgraph_RebuildSuccPred();
    IroJump_MarkReachable(iro_flowgraph_head);
    previous = iro_flowgraph_head;
    current = previous->nextnode;
    while (current != NULL) {
        if (current->first != NULL && !current->reachable) {
            IroDump_Print("Removing unreachable code at: %d\n", current->index);
            previous->nextnode = current->nextnode;
            previous->last->next = current->last->next;
            changed = 1;
            for (block = current->first; block != NULL && block->type == IROLinearLabel && block != current->last->next;
                 block = block->next) {
                for (entry = linear_head; entry != NULL; entry = entry->next) {
                    if (entry->stmt != NULL)
                        entry->stmt->marked = 0;
                }
                for (entry = linear_head; entry != NULL; entry = entry->next) {
                    if (entry->stmt != NULL && entry->stmt->marked == 0) {
                        entry->stmt->marked = 1;
                        for (link = &entry->stmt->dobjstack; (use = *link) != NULL; link = &use->next) {
                            if ((use->kind == 0xd && use->data.catch_block.label == block->u.label) ||
                                (use->kind == 0xf && use->data.specification.label == block->u.label)) {
                                *link = use->next;
                            }
                        }
                    }
                }
            }
            if (current == iroNodeTail)
                iroNodeTail = previous;
        } else {
            previous = current;
        }
        current = current->nextnode;
    }
    if (changed != 0) {
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
    IroVars_CheckTimedLongjmp();
    return changed;
}

int IRO_RemoveRedundantJumps(void)
{
    IRONode *func;
    int changed;
    IROLinear *stmt;
    IROLinear *p;
    IROLinear *q;

    changed = 0;
    for (func = iro_flowgraph_head; func != NULL; func = func->nextnode) {
        if (func->first != NULL) {
            stmt = func->last;
            switch (stmt->type) {
                case IROLinearGoto:
                    for (p = stmt->next; p != NULL && (p->type == IROLinearNop ||
                                                       (p->type == IROLinearLabel && p->u.label != stmt->u.label));
                         p = p->next)
                        ;
                    for (;;) {
                        if (p == NULL || p->type != IROLinearLabel)
                            break;
                        if (p->u.label == stmt->u.label) {
                            IroDump_Print("Removing goto next at %d\n", stmt->index);
                            stmt->type = IROLinearNop;
                            changed = 1;
                            break;
                        }
                        p = p->next;
                    }
                    break;
                case IROLinearIf:
                case IROLinearIfNot:
                    for (p = stmt->next; p != NULL && p->type == IROLinearNop; p = p->next)
                        ;
                    if (p != NULL && p->type == IROLinearGoto) {
                        for (q = p->next; q != NULL && (q->type == IROLinearNop ||
                                                        (q->type == IROLinearLabel && q->u.label != stmt->u.label));
                             q = q->next)
                            ;
                        if (q != NULL && q->type == IROLinearLabel && q->u.label == stmt->u.label) {
                            if (stmt->type == IROLinearIf)
                                stmt->type = IROLinearIfNot;
                            else
                                stmt->type = IROLinearIf;
                            stmt->u.label = p->u.label;
                            p->type = IROLinearNop;
                            IroDump_Print("Removing branch around goto at %d\n", stmt->index);
                            changed = 1;
                        }
                    }
                    for (p = stmt->next; p != NULL && (p->type == IROLinearNop ||
                                                       (p->type == IROLinearLabel && p->u.label != stmt->u.label));
                         p = p->next)
                        ;
                    for (;;) {
                        if (p == NULL || p->type != IROLinearLabel)
                            break;
                        if (p->u.label == stmt->u.label) {
                            IroDump_Print("Removing If/IfNot_Goto next at %d\n", stmt->index);
                            stmt->type = IROLinearNop;
                            IroVars_NopOutWithSideEffectsChecking(stmt->u.branch.cond);
                            changed = 1;
                            break;
                        }
                        p = p->next;
                    }
                    break;
                case IROLinearSwitch: {
                    SwitchInfo *head = stmt->u.swtch.info;
                    SwitchCase *e = head->cases;

                    while (e != NULL && e->label == head->defaultlabel)
                        e = e->next;
                    if (e == NULL) {
                        IroDump_Print("Removing Switch next at %d\n", stmt->index);
                        IroVars_NopOutWithSideEffectsChecking(stmt->u.swtch.cond);
                        stmt->type = IROLinearGoto;
                        stmt->u.label = head->defaultlabel;
                        changed = 1;
                    }
                    break;
                }
            }
        }
    }
    if (changed != 0) {
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
    IroVars_CheckTimedLongjmp();
    return changed;
}

int IRO_RemoveLabels(void)
{
    IRONode *p;
    unsigned int changed = 0;

    IroFlowgraph_RebuildSuccPred();
    for (p = iro_flowgraph_head; p != NULL; p = p->nextnode) {
        if (p->first != NULL && p->first->type == IROLinearLabel && p->referenced == 0) {
            p->first->type = IROLinearNop;
            p->first->flags &= ~1;
            changed = 1;
        }
    }
    IroVars_CheckTimedLongjmp();
    return changed;
}

void IroJump_ConvertCInt64ToType(CInt64 *value, Type *type)
{
    if (is_unsigned(type)) {
        switch (type->size) {
            case 1:
                CInt64_ConvertUInt8(value);
                break;
            case 2:
                CInt64_ConvertUInt16(value);
                break;
            case 4:
                CInt64_ConvertUInt32(value);
        }
    } else {
        switch (type->size) {
            case 1:
                CInt64_ConvertInt8(value);
                break;
            case 2:
                CInt64_ConvertInt16(value);
                break;
            case 4:
                CInt64_ConvertInt32(value);
        }
    }
}
