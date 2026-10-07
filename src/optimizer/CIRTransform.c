#define CERROR_FILE "CIRTransform.c"
#include "compiler/common.h"
#include "compiler/CIRTransform.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/ENode.h"
/* Declarations gathered from the merged files. */

static TypeFunc data_005741b0 = {TYPEFUNC, 0, NULL, NULL, TYPE(&stsignedshort), 0, 0};
static TypeFunc data_005741cc = {TYPEFUNC, 0, NULL, NULL, TYPE(&stunsignedlong), 0, 0};
static TypeFunc data_005741e8 = {TYPEFUNC, 0, NULL, NULL, TYPE(&void_ptr), 0, 0};

Object *get_or_create_type_name_object(CIRTypeName *entry, Type *ty)
{
    Object *obj;

    if ((obj = entry->object) == NULL) {
        obj = CParser_NewFunctionObject(NULL);
        entry->object = obj;
        obj->nspace = cscope_root;
        obj->name = GetHashNameNode(entry->name);
        obj->flags = OBJECT_INTERNAL;
        if (ty) {
            switch (ty->size) {
                case 2:
                    obj->type = TYPE(&data_005741b0);
                    break;
                case 4:
                    obj->type = TYPE(&data_005741cc);
                    break;
                case 8:
                    obj->type = TYPE(&data_005741e8);
                    break;
                default:
                    CError_FATAL(423);
            }
        } else {
            obj->type = TYPE(&data_005741e8);
        }
    }
    return obj;
}

ENode *expand_compound_assignment(ENode *expr)
{
    ENode *left;
    ENode *target;
    ENode *address;
    ENode *value;
    ENode *converted;
    ENode *operation;
    ENode *storeAddress;
    ENode *storeTarget;
    ENode *assignment;
    ENode *result;
    struct {
        ENode *direct;
        Object *temporary;
        ENode *setup;
        Type *type;
        Type *qualifiedType;
    } state;
    left = expr->data.diadic.left;
    memclrw(&state, sizeof(state));
    state.type = left->rtype;
    if (left->type != EINDIRECT) {
        CError_FATAL(518);
    }
    target = left->data.diadic.left;
    target->rtype = CDecl_NewPointerType(state.type);
    if (target->type == EOBJREF) {
        state.direct = target->data.diadic.left;
    } else {
        if (target->type == EBITFIELD) {
            state.qualifiedType = target->rtype;
            target = target->data.diadic.left;
        }
        state.temporary = create_temp_object(target->rtype);
        state.setup = ((ENode * (*)(ENode *, ENode *, UInt8))
                           makediadicnode)(((ENode * (*)(Object *)) create_objectnode)(state.temporary), target, 30);
    }
    if (state.direct == NULL) {
        address = ((ENode * (*)(Object *)) create_objectnode)(state.temporary);
        if (state.qualifiedType != NULL) {
            address = makemonadicnode(address, 49);
            address->rtype = state.qualifiedType;
        }
        value = makemonadicnode(address, 4);
    } else {
        value = ((ENode * (*)(ENode *)) create_objectnode)(state.direct);
    }
    converted = value;
    value->rtype = state.type;
    if (expr->data.diadic.left->rtype != expr->data.diadic.right->rtype) {
        converted = makemonadicnode(value, 48);
        converted->rtype = expr->data.diadic.right->rtype;
    }
    operation = ((ENode * (*)(ENode *, ENode *, UInt8)) makediadicnode)(converted, expr->data.diadic.right, 41);
    switch (expr->type) {
        case EMULASS:
            operation->type = EMUL;
            break;
        case EDIVASS:
            operation->type = EDIV;
            break;
        case EMODASS:
            operation->type = EMODULO;
            break;
        case EADDASS:
            operation->type = EADD;
            break;
        case ESUBASS:
            operation->type = ESUB;
            break;
        case ESHLASS:
            operation->type = ESHL;
            break;
        case ESHRASS:
            operation->type = ESHR;
            break;
        case EANDASS:
            operation->type = EAND;
            break;
        case EXORASS:
            operation->type = EXOR;
            break;
        case EORASS:
            operation->type = EOR;
            break;
        default:
            CError_FATAL(618);
    }
    if (expr->data.diadic.left->rtype != expr->data.diadic.right->rtype) {
        operation = ((ENode * (*)(ENode *, UInt8)) makemonadicnode)(operation, 48);
        operation->rtype = expr->data.diadic.left->rtype;
    }
    if (operation->rtype->type == TYPEFLOAT) {
        operation = CExpr2_ReturnNode(operation);
    }
    if (state.direct == NULL) {
        storeAddress = ((ENode * (*)(Object *)) create_objectnode)(state.temporary);
        if (state.qualifiedType != NULL) {
            storeAddress = ((ENode * (*)(ENode *, UInt8)) makemonadicnode)(storeAddress, 49);
            storeAddress->rtype = state.qualifiedType;
        }
        storeTarget = ((ENode * (*)(ENode *, UInt8)) makemonadicnode)(storeAddress, 4);
    } else {
        storeTarget = ((ENode * (*)(ENode *)) create_objectnode)(state.direct);
    }
    storeTarget->rtype = state.type;
    assignment = ((ENode * (*)(ENode *, ENode *, UInt8)) makediadicnode)(storeTarget, operation, 30);
    result = assignment;
    if (state.setup != NULL) {
        result = ((ENode * (*)(ENode *, ENode *, UInt8)) makediadicnode)(state.setup, assignment, 41);
        result->rtype = result->data.diadic.right->rtype;
    }
    return result;
}

ENode *simplify_unused_enode_values(ENode *e, UInt8 flag)
{
    switch (e->type) {
        case EINDIRECT:
        case EFORCELOAD:
        case EBITFIELD:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, flag);
            break;
        case EPOSTINC:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, 1);
            break;
        case EPOSTDEC:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, 1);
            break;
        case EPREINC:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, 1);
            break;
        case EPREDEC:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, 1);
            break;
        case ETYPCON:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, flag);
            if (flag == 0)
                return e->data.monadic;
            break;
        case EBINNOT:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, flag);
            break;
        case ELOGNOT:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, flag);
            break;
        case EMONMIN:
            e->data.monadic = simplify_unused_enode_values(e->data.monadic, flag);
            break;
        case EADD:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case ESUB:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EMUL:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
        case EDIV:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EMODULO:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case ESHL:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case ESHR:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EROTL:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EROTR:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EAND:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EXOR:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EOR:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            if (flag == 0) {
                e->type = ECOMMA;
                e->rtype = e->data.diadic.right->rtype;
                return e;
            }
            break;
        case ELAND:
        case ELOR:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EMULV:
        case EADDV:
        case ESUBV:
        case EPMODULO:
        case EBCLR:
        case EBTST:
        case EBSET:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EASS:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, 1);
            break;
        case EMULASS:
        case EDIVASS:
        case EADDASS:
        case ESUBASS:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, 1);
            break;
        case EMODASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, 1);
            break;
        case ECOMMA:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 0);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case ECOND:
            e->data.cond.cond = simplify_unused_enode_values(e->data.cond.cond, 1);
            e->data.cond.expr1 = simplify_unused_enode_values(e->data.cond.expr1, 1);
            e->data.cond.expr2 = simplify_unused_enode_values(e->data.cond.expr2, 1);
            break;
        case EQUALNAME:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, flag);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, flag);
            break;
        case EFUNCCALL:
        case EFUNCCALLP: {
            ENodeList *p;
            ENodeList *head;

            e->data.funccall.funcref = simplify_unused_enode_values(e->data.funccall.funcref, 1);
            head = e->data.funccall.args;
            for (p = head; p != NULL; p = p->next)
                p->node = simplify_unused_enode_values(p->node, 1);
            e->data.funccall.args = head;
            break;
        }
        case EMFPOINTER:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, 1);
            break;
        case ELABEL:
        case ESETCONST:
            e->data.diadic.left = simplify_unused_enode_values(e->data.diadic.left, 1);
            e->data.diadic.right = simplify_unused_enode_values(e->data.diadic.right, 1);
            break;
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case EPRECOMP:
        case ELOCOBJ:
        case ENEWEXCEPTIONARRAY:
        case EMEMBER:
        case EASSBLK:
            break;
        default:
            CError_FATAL(1884);
            break;
    }
    return e;
}
