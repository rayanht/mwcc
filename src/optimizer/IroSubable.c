#include "compiler/common.h"
#include "compiler/IroSubable.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/IroDump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/Registers.h"

/* Whether an expression of each kind can be a common subexpression. */
static char data_00565458[76] = {
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

unsigned int fn_004f0040(IROLinear *node)
{
    Object *localObject;
    VarInfo *registerInfo;
    int enabled;
    ENode *operand;
    switch (node->type) {
        case 3:
            if ((node->nodetype == 15 || node->nodetype == 16) && fn_0044d520(node->u.diadic.right) != 0) {
                localObject = IroDump_GetObjRef(node->u.diadic.left);
                if (localObject != NULL && localObject->datatype == DLOCAL) {
                    registerInfo = localObject->u.var.info;
                    if (registerInfo != NULL && registerInfo->noregister == 0) {
                        return 0;
                    }
                }
            }
            enabled = !!data_00565458[node->nodetype];
            return enabled;
        case 2:
            if (data_00565458[node->nodetype] != 0) {
                return 1;
            }
            if (node->nodetype == 4 && (node->flags & IROLF_Assigned) == 0) {
                if ((node->flags & IROLF_Ind) != 0) {
                    node = node->u.monadic;
                    if (node->type == 1) {
                        operand = node->u.node;
                        if (operand->type == EOBJREF && operand->data.objref->datatype == DLOCAL &&
                            operand->data.objref->u.var.info != NULL &&
                            operand->data.objref->u.var.info->noregister == 0) {
                            return 0;
                        }
                    }
                    return 1;
                }
                if (IroDump_GetObjRef(node) != NULL &&
                    IroPropagate_IsRegisterEligible(node->u.monadic->u.node->data.objref) != 0) {
                    return 0;
                }
                return 1;
            }
            if (node->nodetype == 48 && node->rtype->type == TYPEINT &&
                node->rtype->size > node->u.monadic->rtype->size) {
                return 1;
            }
            return 0;
        case 1:
        default:
            return 0;
    }
}
