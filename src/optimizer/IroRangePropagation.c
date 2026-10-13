#define CERROR_FILE "IRO_RangePropagateInFNode.c"
#include "compiler/common.h"
#include "version.h"
#include "compiler/IroRangePropagation.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"

struct ERangeVar *range_vars;
struct ERangeVar *first_range_var;

/* The bounds of the integral types. */
CInt64 signed_char_max = {0, 0x7F};
CInt64 type_range_minimum = {-1, 0xFFFFFF80};
CInt64 data_005539c8 = {0, 0xFF};
CInt64 int16_max = {0, 0x7FFF};
CInt64 data_005539d8 = {-1, 0xFFFF8000};
CInt64 data_005539e0 = {0, 0xFFFF};
static CInt64 lbl_005539E8 = {0, 0x7FFF};
static CInt64 lbl_005539F0 = {-1, 0xFFFF8000};
static CInt64 lbl_005539F8 = {0, 0xFFFF};
CInt64 int32_max = {0, 0x7FFFFFFF};
CInt64 type_range_lower_bound = {-1, 0x80000000};
CInt64 data_00553a10 = {0, 0xFFFFFFFF};
CInt64 range_int32_max = {0, 0x7FFFFFFF};
CInt64 type_range_min = {-1, 0x80000000};
CInt64 data_00553a28 = {0, 0xFFFFFFFF};
static CInt64 lbl_00553A30 = {-1, 0xFFFFFFFF};
static ERange *NewRange(UInt8 type)
{
    ERange *range = (void *)oalloc(sizeof(ERange));
    range->type = type;
    return range;
}

static void AddVar(Object *key, ERange *range)
{
    ERangeVar *var = (void *)oalloc(sizeof(ERangeVar));
    var->object = key;
    var->range = range;
    var->next = range_vars;
    range_vars = var;
    if (!first_range_var)
        first_range_var = var;
}

static ERangeVar *FindVar(Object *key)
{
    ERangeVar *var;
    for (var = range_vars; var && key != var->object; var = var->next)
        ;
    return var;
}

static inline IROLinear *FirstOperand(IROLinear *nd)
{
    IROLinear *operand = NULL;
    if (!(nd->type != IROLinearOp2Arg && nd->type != IROLinearOp1Arg))
        operand = nd->u.diadic.left;
    return operand;
}

static int mask_covers_range(ERange *range, CInt64 val)
{
    UInt16 count;
    CInt64 mask;
    CInt64 x;

    count = 0;
    x = range->upper;
    while (CInt64_NotEqual(x = CInt64_ShrU(x, cint64_one), cint64_zero))
        count++;
    if (CInt64_NotEqual(range->upper, cint64_zero))
        count++;
    x.hi = 0;
    x.lo = count;
    mask = CInt64_Sub(CInt64_Shl(cint64_one, x), cint64_one);
    if (CInt64_NotEqual(cint64_zero, CInt64_And(CInt64_Inv(val), mask)))
        return 0;
    return 1;
}

static int IsUnsignedType(Type *type)
{
    TypeIntegral *t = (TypeIntegral *)type;
    return t == &stunsignedchar || t == &stunsignedshort || t == &stunsignedint || t == &stunsignedlong ||
           t == &stunsignedlonglong;
}

#if VERSION == VERSION_GC_1_0
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
#endif

static void invalidate_range_vars(void)
{
    ERangeVar *var;

    for (var = range_vars; var != NULL; var = var->next)
        var->range->type = 3;
}

#if VERSION == VERSION_GC_1_0
int initialize_node_range(IROLinear *nd)
#else
static inline int initialize_node_range(IROLinear *nd)
#endif
{
    switch (nd->u.node->type) {
        case EOBJREF:
            nd->range = NULL;
            break;
        case EINTCONST:
            nd->range = NewRange(0);
            nd->range->upper = nd->range->lower = nd->u.node->data.intval;
            break;
        case EFLOATCONST:
        case ESTRINGCONST:
            nd->range = NewRange(0);
            break;
    }
    return 1;
}

int IroRangePropagation_PropagateRangeInLinear(struct IROLinear *nd)
{
    IROLinear *left;
    IROLinear *saved;
    ERange *range;
    ERangeVar *var;
    Object *obj;
    CInt64 val;

    switch (nd->nodetype) {
        case EINDIRECT:
            left = nd->u.monadic;
            if (nd->rtype->type == TYPEINT) {
                if (left->type == IROLinearOperand && left->u.node->type == EOBJREF) {
                    if (!left->range) {
                        obj = left->u.node->data.objref;
                        if (obj) {
                            if ((var = FindVar(obj)) != NULL) {
                                left->range = NewRange(3);
                                *left->range = *var->range;
                            } else {
                                left->range = NewRange(3);
                                left->range->upper = cint64_max;
                                left->range->lower = cint64_min;
                                AddVar(obj, left->range);
                            }
                        }
                    }
                    nd->range = left->range;
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            } else {
                if (left->type == IROLinearOperand && left->u.node->type == EOBJREF && !left->range)
                    left->range = NewRange(3);
                nd->range = NewRange(3);
            }
            break;
        case EAND:
        case EANDASS:
            if (IroUtil_IsType1NodeType50(nd->u.diadic.right)) {
                val = nd->u.diadic.right->u.node->data.intval;
                nd->range = NewRange(1);
                nd->range->upper = val;
                nd->range->lower = cint64_zero;
                range = nd->u.diadic.left->range;
                if (range && range->type != 3 && CInt64_LessEqualU(range->upper, val) &&
                    CInt64_LessEqualU(range->lower, val)) {
                    if (mask_covers_range(range, val) && !fn_0044be00(nd->u.diadic.left)) {
                        IroUtil_ClearZeroOperands(nd->u.diadic.right);
                        nd->type = IROLinearNop;
                        nd->expr = NULL;
                        saved = nd->u.diadic.left;
                        nd->u.diadic.left = nd->u.diadic.right;
                        if (!IroUtil_ReplaceFirstReference(nd, saved)) {
                            saved->flags &= ~IROLF_Reffed;
                            if (IroUtil_GetObjRef(saved))
                                IroUtil_ClearZeroOperands(saved);
                        }
                    }
                }
            } else if (nd->u.diadic.right->range) {
                nd->range = NewRange(3);
                *nd->range = *nd->u.diadic.right->range;
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case ELOGNOT:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case ELAND:
        case ELOR:
            nd->range = NewRange(1);
            nd->range->upper = cint64_one;
            nd->range->lower = cint64_zero;
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EBINNOT:
        case EFORCELOAD:
        case EXOR:
        case EOR:
        case EXORASS:
        case EORASS:
        case ECOMMA:
        case ETYPCON:
        case EBITFIELD:
        case ECOND:
        case ENULLCHECK:
            nd->range = NewRange(3);
            nd->range->upper = cint64_max;
            nd->range->lower = cint64_min;
            break;
        case EASS:
            if (nd->rtype->type == TYPEINT)
                nd->range = nd->u.diadic.right->range;
            break;
        case EMUL:
        case EMULV:
        case EMULASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    if (IsUnsignedType(nd->rtype)) {
                        nd->range->upper =
                            CInt64_MulU(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->upper);
                        nd->range->lower =
                            CInt64_MulU(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->lower);
                    } else {
                        nd->range->upper =
                            CInt64_Mul(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->upper);
                        nd->range->lower =
                            CInt64_Mul(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->lower);
                    }
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EDIV:
        case EDIVASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    if (IsUnsignedType(nd->rtype)) {
                        nd->range->upper =
                            CInt64_DivU(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_DivU(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    } else {
                        nd->range->upper =
                            CInt64_Div(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_Div(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    }
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EMODULO:
        case EMODASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    if (IsUnsignedType(nd->rtype)) {
                        nd->range->upper =
                            CInt64_ModU(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_ModU(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    } else {
                        nd->range->upper =
                            CInt64_Mod(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_Mod(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    }
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EADDV:
        case EADD:
        case EADDASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    nd->range->upper = CInt64_Add(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->upper);
                    nd->range->lower = CInt64_Add(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->lower);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case ESUBV:
        case ESUB:
        case ESUBASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    nd->range->upper = CInt64_Sub(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                    nd->range->lower = CInt64_Sub(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case ESHL:
        case ESHLASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    nd->range->upper = CInt64_Shl(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->upper);
                    nd->range->lower = CInt64_Shl(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->lower);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case ESHR:
        case ESHRASS:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.diadic.left->range && nd->u.diadic.left->range->type != 3 && nd->u.diadic.right->range &&
                    nd->u.diadic.right->range->type != 3) {
                    nd->range = NewRange(2);
                    if (IsUnsignedType(nd->rtype)) {
                        nd->range->upper =
                            CInt64_ShrU(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_ShrU(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    } else {
                        nd->range->upper =
                            CInt64_Shr(nd->u.diadic.left->range->upper, nd->u.diadic.right->range->lower);
                        nd->range->lower =
                            CInt64_Shr(nd->u.diadic.left->range->lower, nd->u.diadic.right->range->upper);
                    }
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EPOSTINC:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.monadic->range && nd->u.monadic->range->type != 3) {
                    nd->range = NewRange(2);
                    range = nd->u.monadic->range;
                    *nd->range = *range;
                    range->upper = CInt64_Add(range->upper, cint64_one);
                    range->lower = CInt64_Add(range->lower, cint64_one);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EPOSTDEC:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.monadic->range && nd->u.monadic->range->type != 3) {
                    nd->range = NewRange(2);
                    range = nd->u.monadic->range;
                    *nd->range = *range;
                    range->upper = CInt64_Sub(range->upper, cint64_one);
                    range->lower = CInt64_Sub(range->lower, cint64_one);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EPREINC:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.monadic->range && nd->u.monadic->range->type != 3) {
                    nd->range = NewRange(2);
                    range = nd->u.monadic->range;
                    nd->range->upper = CInt64_Add(range->upper, cint64_one);
                    nd->range->lower = CInt64_Add(range->lower, cint64_one);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EPREDEC:
            if (nd->rtype->type == TYPEINT) {
                if (nd->u.monadic->range && nd->u.monadic->range->type != 3) {
                    nd->range = NewRange(2);
                    range = nd->u.monadic->range;
                    nd->range->upper = CInt64_Sub(range->upper, cint64_one);
                    nd->range->lower = CInt64_Sub(range->lower, cint64_one);
                } else {
                    nd->range = NewRange(3);
                    nd->range->upper = cint64_max;
                    nd->range->lower = cint64_min;
                }
            }
            check_range_for_type(nd->range, nd->rtype);
            break;
        case EMONMIN:
            nd->range = NewRange(3);
            nd->range->upper = cint64_max;
            nd->range->lower = cint64_min;
            break;
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBCLR:
        case EBTST:
        case EBSET:
            nd->range = NewRange(3);
            nd->range->upper = cint64_max;
            nd->range->lower = cint64_min;
            break;
        default:
            check_range_for_type(nd->range, nd->rtype);
            break;
    }

    if ((nd->type == IROLinearOp1Arg || nd->type == IROLinearOp2Arg) && data_00551d6c[nd->nodetype] && nd->range &&
        nd->rtype->type == TYPEINT) {
        left = FirstOperand(nd);
        if (left->type == IROLinearOp1Arg && left->nodetype == EINDIRECT &&
            (left->u.monadic->nodetype == EINDIRECT || left->u.monadic->nodetype == EADD)) {
            invalidate_range_vars();
            nd->range = NewRange(3);
        } else {
            obj = NULL;
            if (left)
                obj = IroUtil_GetObjRef(left);
            if (!obj)
                return 0;
            range = nd->range;
            if (nd->nodetype == EPOSTINC || nd->nodetype == EPOSTDEC)
                range = left->range;
            if ((var = FindVar(obj)) == NULL)
                AddVar(obj, range);
            else
                var->range = range;
        }
    }
    return nd->range != NULL;
}

int initialize_linear_range(IROLinear *nd)
{
    nd->range = NULL;
    switch (nd->type) {
        case IROLinearOperand:
            initialize_node_range(nd);
            break;
        case IROLinearOp1Arg:
        case IROLinearOp2Arg:
            IroRangePropagation_PropagateRangeInLinear(nd);
            break;
        case IROLinearFunccall:
            invalidate_range_vars();
            break;
        case IROLinearNop:
        case IROLinearGoto:
        case IROLinearIf:
        case IROLinearIfNot:
        case IROLinearReturn:
        case IROLinearLabel:
        case IROLinearSwitch:
        case IROLinearEntry:
        case IROLinearExit:
        case IROLinearBeginCatch:
        case IROLinearEndCatch:
        case IROLinearEndCatchDtor:
        case IROLinearAsm:
        case 17:
        case 18:
        case IROLinearEnd:
            break;
    }
    return 0;
}

SInt32 IRO_RangePropagateInFNode(void)
{
    Boolean changed;
    IRONode *ns;
    IROLinear *nd;

    for (ns = iro_flowgraph_head; ns != NULL; ns = ns->nextnode) {
        range_vars = first_range_var = NULL;
        for (nd = ns->first; nd != ns->last; nd = nd->next)
            initialize_linear_range(nd);
        if (initialize_linear_range(nd))
            changed = 1;
    }
    if (changed) {
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
    IroVars_CheckTimedLongjmp();
    return changed;
}
