#define CERROR_FILE "CExpr.c"
#include "compiler/common.h"
#include "compiler/CExpr.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CIRTransform.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CRTTI.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <setjmp.h>
#include <string.h>
#include "compiler/ENode.h"

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

typedef struct _res res;

static __inline CLabel *FindNode(HashNameNode *key)
{
    CLabel *p = clabels;
    while (p != NULL) {
        if (key == p->name)
            break;
        p = p->next;
    }
    return p;
}

static __inline CLabel *AddNode(HashNameNode *key)
{
    CLabel *p = newlabel();
    p->name = key;
    p->next = clabels;
    clabels = p;
    return p;
}

static __inline void FindOrAdd(HashNameNode *key)
{
    CLabel *node;
    if ((node = FindNode(key)) == NULL)
        node = AddNode(key);
    else if (node->target.stmt != NULL)
        CError_ReportError(ERR_LABEL_REDEFINED, key->name);
    {
        Statement *entry = CFunc_AppendStatement(2);
        entry->target.label = node;
        node->target.stmt = entry;
    }
}

static void expect(SInt16 tok, SInt16 err)
{
    if (tk != tok) {
        SInt16 e = err;
        if (data_00587f18 != 0)
            longjmp(data_00583a68, 1);
        if (tk == TK_EOL || tk == ';')
            e = 0x70;
        CError_ReportError(e);
    }
}

static void expectToken(SInt16 tok, SInt16 err)
{
    if (tk != tok) {
        SInt16 e = err;
        if (data_00587f18 != 0)
            longjmp(data_00583a68, 1);
        if (tk == TK_EOL || tk == ';')
            e = 0x70;
        CError_ReportError(e);
    }
}

void CExpr_CheckUnwantedAssignment(ENode *node)
{
    if (((copts.warn_possunwant != '\0') && (node->type == EASS)) && ((node->flags & ENODE_FLAG_80) == 0)) {
        CError_Warning(ERR_POSSIBLE_UNWANTED_ASSIGNMENT);
    }
}

static CInt64 Inner(void)
{
    ENode *expr;

    expr = CExpr_RewriteConst(pointer_generation(conditional_expression()));
    if (expr->type == EINTCONST) {
        switch ((char)expr->rtype->type) {
            case TYPEINT:
                return expr->data.intval;
            case TYPEENUM:
                return expr->data.intval;
        }
    }
    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
    return cint64_zero;
}

CInt64 fn_004f0b30(void)
{
    return Inner();
}

static ENode *AdjustBoolAssign(ENode *enode)
{
    switch (enode->type) {
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
            if (enode->rtype == (Type *)&stbool) {
                enode = expand_compound_assignment(enode);
                if (enode->type == EASS) {
                    enode->data.diadic.right = makemonadicnode(enode->data.diadic.right, 7);
                    enode->data.diadic.right->rtype = (Type *)&stbool;
                    enode->data.diadic.right = makemonadicnode(enode->data.diadic.right, 7);
                }
            }
            break;
    }
    return enode;
}

static inline ENode *promote_assignment_operand(ENode *target, ENode *operand, Boolean *bitfieldPromotion)
{
    if (*bitfieldPromotion && target->type == EINDIRECT && target->data.monadic->type == EBITFIELD &&
        operand->type != EINTCONST) {
        *bitfieldPromotion = 0;
        operand = oldassignmentpromotion(operand, target->rtype, target->flags, 1);
        *bitfieldPromotion = 1;
    } else {
        operand = oldassignmentpromotion(operand, target->rtype, target->flags, 1);
    }
    return operand;
}

#define D0(e) ((e)->data.diadic.left)
#define D1(e) ((e)->data.diadic.right)
#define D2(e) ((e)->data.cond.expr2)

static ENode *normalize(ENode *e)
{
    return CExpr_RewriteConst(pointer_generation(e));
}

static Boolean IsZeroCInt64(CInt64 *v)
{
    return v->hi == 0 && v->lo == 0;
}

static inline Boolean IsAggregateType(Type *type)
{
    switch ((SInt8)type->type) {
        case TYPESTRUCT:
        case TYPECLASS:
            return 1;
        case TYPEMEMBERPOINTER:
            if (type->size != 4)
                return 1;
            return 0;
        default:
            return 0;
    }
}

static inline void warn_unwanted_assignment(ENode *expression)
{
    if (copts.warn_possunwant != 0 && expression->type == EASS && (expression->flags & ENODE_FLAG_80) == 0)
        CError_Warning(ERR_POSSIBLE_UNWANTED_ASSIGNMENT);
}

static ENode *CExpr_intnode(ENode *e, SInt32 v)
{
    e->type = EINTCONST;
    e->rtype = CParser_GetBoolType();
    e->data.intval.lo = (UInt32)v;
    e->data.intval.hi = 0;
    return e;
}

static ENode *recovered_normalize(ENode *value)
{
    return CExpr_RewriteConst((ENode *)pointer_generation(value));
}

static ENode *generate_pointer_and_rewrite_const(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static ENode *pointer_generation_and_rewrite_const(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static void recovered_set_integer(ENode *n, SInt32 v)
{
    CInt64 *p = &n->data.intval;
    p->lo = v;
    p->hi = v < 0 ? -1 : 0;
}

static ENode *rewrite_const_after_pointer_generation(ENode *value)
{
    return CExpr_RewriteConst((ENode *)pointer_generation(value));
}

static ENode *rewrite_pointer_generation_const(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static Boolean IsZeroValue(ENode *e)
{
    return e->data.intval.hi == 0 && e->data.intval.lo == 0;
}

static ENode *NormalizeOperand(ENode *e)
{
    return CExpr_RewriteConst(pointer_generation(e));
}

static inline Boolean CExpr_IsZeroValue(const CInt64 *value)
{
    return value->hi == 0 && value->lo == 0;
}

static inline void set_comparison_result_type(ENode *node)
{
    if (copts.cplusplus && copts.booltruefalse)
        node->rtype = (Type *)&stbool;
    else
        node->rtype = (Type *)&stsignedint;
}

static ENode *CExpr_WrapPrecomp(ENode *n, ENode *r)
{
    ENode *nw = CompilerTools_AllocatePool(0x1a);
    *nw = *r;
    nw->type = EMFPOINTER;
    nw->data.diadic.left = CompilerTools_AllocatePool(0x1a);
    *nw->data.diadic.left = *n;
    nw->data.diadic.right = r;
    nw->data.precomp.labelId = CParser_GetUniqueID();
    n->type = ENULLCHECK;
    n->data.longval = nw->data.precomp.labelId;
    return nw;
}

static ENode *rewrite_const_pointer_generation(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static inline void RemoveRef(ENode *n)
{
    n->rtype = TYPE_POINTER(n->rtype)->target;
}

static inline void PutType(ENode *n, Type *t)
{
    n->rtype = t;
}

static inline void SetInt64(CInt64 *arg, int n)
{
    CInt64 *pN = arg;
    pN->lo = n;
    pN->hi = n < 0 ? -1 : 0;
}

static inline ENode *recovery_construct_4059(ENodeList *arguments, Type *targetType, SInt32 qualifiers)
{
    ENode *node = CExpr_NewTemplDepENode(TDE_CAST);
    node->data.templdep.u.cast.args = arguments;
    node->data.templdep.u.cast.type = targetType;
    node->data.templdep.u.cast.qual = qualifiers;
    return node;
}

#define STUNSIG ((Type *)&stunsignedlong)
#define STBOOL ((Type *)&stbool)

static Boolean IsZero_4f5aa0(ENode *e)
{
    int result = 0;
    if (e->data.intval.hi == 0 && e->data.intval.lo == 0)
        result = 1;
    return result;
}

static inline UInt32 PreserveEvaluation(ENode *expr, ENode **res)
{
    if (isnotzero(expr) == 0) {
        ENode *n1 = CompilerTools_AllocatePool(0x1a);
        *n1 = *(*res);
        n1->type = EMFPOINTER;
        n1->data.diadic.left = CompilerTools_AllocatePool(0x1a);
        *n1->data.diadic.left = *expr;
        n1->data.diadic.right = (*res);
        n1->data.precomp.labelId = CParser_GetUniqueID();
        expr->type = ENULLCHECK;
        expr->data.longval = n1->data.precomp.labelId;
        (*res) = n1;
    }
    return (UInt32)*res;
}

static ENode *pointer_generation_then_rewrite_const(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static inline ENode *materialize_temporary(ENode *base)
{
    ENode *t, *c, *n;
    t = CExpr2_NewESCOPEBEGINNode(base->rtype, 1);
    c = (ENode *)CompilerTools_AllocatePool(0x1a);
    *c = *t;
    n = makemonadicnode(t, 4);
    n->rtype = base->rtype;
    n = makediadicnode(n, base, 0x1e);
    n = makediadicnode(n, c, 0x29);
    n->rtype = c->rtype;
    n = makemonadicnode(n, 4);
    n->rtype = base->rtype;
    return n;
}

static inline ENode *unwrap_reference(ENode *node)
{
    if (node->rtype->type != TYPEPOINTER || !(((TypePointer *)node->rtype)->qual & Q_REFERENCE))
        return node;
    node = makemonadicnode(node, EINDIRECT);
    node->rtype = ((TypePointer *)node->rtype)->target;
    return node;
}

static ENode *callable_expression(ENode *e)
{
    switch (*(UInt8 *)e) {
        case 4: {
            switch ((SInt8)e->rtype->type) {
                case TYPEARRAY:
                    ((ENode *)e->data.monadic)->rtype = (Type *)CDecl_NewPointerType(((TypePointer *)e->rtype)->target);
                    return (ENode *)e->data.monadic;
                case TYPEFUNC:
                    return (ENode *)e->data.monadic;
            }
            break;
        }
    }
    return e;
}

static ENode *generate_pointer_then_rewrite_const(ENode *value)
{
    return CExpr_RewriteConst(pointer_generation(value));
}

static inline ENode *parse_comma_expression(void)
{
    ENode *l;
    ENode *t;
    BinaryOperatorResult oA;
    l = assignment_expression();
    if ((SInt16)tk == ',') {
        do {
            l = generate_pointer_then_rewrite_const(l);
            tk = CPrepTokenizer_GetNextToken();
            t = generate_pointer_then_rewrite_const(assignment_expression());
            if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2c, (ENode *)(l), (ENode *)(t), &oA)) {
                l = oA.expression;
                if (l == NULL)
                    CError_FATAL(6531);
            } else {
                CExpr_CheckUnusedExpression(l);
                l = makecommaexpression(l, t);
                l->rtype = t->rtype;
            }
        } while ((SInt16)tk == ',');
    }
    l = generate_pointer_then_rewrite_const(l);
    return l;
}

static inline ENode *parse_comma_operator_expression(void)
{
    ENode *e, *a, *b;
    void *local_14[3];
    e = assignment_expression();
    while (tk == ',') {
        a = CExpr_RewriteConst(pointer_generation(e));
        tk = (SInt16)CPrepTokenizer_GetNextToken();
        b = CExpr_RewriteConst(pointer_generation(assignment_expression()));
        if (copts.cplusplus != 0 &&
            CExpr_CheckOperator(0x2c, (ENode *)(a), (ENode *)(b), (BinaryOperatorResult *)(local_14))) {
            e = (ENode *)local_14[0];
            if (e == NULL)
                CError_FATAL(6531);
        } else {
            CExpr_CheckUnusedExpression(a);
            e = makecommaexpression(a, b);
            e->rtype = b->rtype;
        }
    }
    return e;
}

static inline void set_signed_integer(ENode *n, SInt32 v)
{
    CInt64 *p = &n->data.intval;
    p->lo = v;
    p->hi = v < 0 ? -1 : 0;
}

static inline SInt32 builtin_align(Type *op)
{
    return CMachine_GetTypeAlignment(op);
}

static inline void builtin_classify(ENode *n, Type *op)
{
    SInt32 v;
    CInt64 *p;

    switch ((SInt8)op->type) {
        case TYPEVOID:
            v = 0;
            break;
        case TYPEFUNC:
            v = 10;
            break;
        case TYPEENUM:
            v = 3;
            break;
        case TYPEINT:
            v = 1;
            break;
        case TYPEFLOAT:
            v = 8;
            break;
        case TYPEMEMBERPOINTER:
        case TYPEPOINTER:
            v = 5;
            break;
        case TYPEARRAY:
            v = 14;
            break;
        case TYPESTRUCT:
            v = 12;
            break;
        case TYPECLASS:
            v = 12;
            break;
        default:
            v = -1;
            break;
    }
    p = &n->data.intval;
    CInt64_SetLong(p, v);
}

static inline ENode *parse_4e9940(void)
{
    ENode *e = CClass_CreateThisSelfExpr();
    if (e == NULL)
        e = nullnode();
    tk = (SInt16)CPrepTokenizer_GetNextToken();
    return e;
}

static inline ENode *parse_builtin_8b50(void)
{
    ENode *e = intconstnode((Type *)&stsignedint, 0);
    set_signed_integer(e, encode_type_bits((TypeKind *)scan_type_or_expression_type()));
    return e;
}

static inline int CExpr_StructKind(Type *type)
{
    return TYPE_STRUCT(type)->stype;
}

/* The null test reads nspace through a cast, so its address differs from
 * et->nspace and IRO does not CSE the two: cmp [et+6],0 then a reload. */

static Boolean IsZero(CInt64 *v)
{
    return v->hi == 0 && v->lo == 0;
}

static inline ENode *dereference_reference_node(ENode *node)
{
    if (node->rtype->type != TYPEPOINTER || !(TYPE_POINTER(node->rtype)->qual & Q_REFERENCE))
        return node;
    node = makemonadicnode(node, EINDIRECT);
    node->rtype = TYPE_POINTER(node->rtype)->target;
    return node;
}

static inline char CExpr_IsMemberFunction(NameResult *candidates)
{
    Object *object = (Object *)candidates->object;
    return OBJECT(candidates->object)->type->type == TYPEFUNC && (((TypeMemberFunc *)object->type)->flags & 1024) != 0;
}

static Boolean CExpr_QualMismatch(SInt32 from, SInt32 to)
{
    Boolean result =
        ((from & Q_CONST) != 0 && (to & Q_CONST) == 0) || ((from & Q_VOLATILE) != 0 && (to & Q_VOLATILE) == 0);
    return result;
}

static UInt32 applyqual(Type *t, SInt16 x)
{
    return CParser_GetCVTypeQualifiers(TYPE_POINTER(t)->target, x);
}

ENode *CExpr_IntegralConstOrDepExpr(void)
{
    ENode *expr;

    expr = CExpr_RewriteConst((ENode *)pointer_generation(conditional_expression()));

    if (expr->type == EINTCONST) {
        switch ((signed char)expr->rtype->type) {
            case TYPEINT:
                return expr;
            case TYPEENUM:
                expr->rtype = ((TypeEnum *)expr->rtype)->enumtype;
                return expr;
            default:
                CError_FATAL(6597);
                break;
        }
    }

    if (CTemplTool_IsTypeDepExpr(expr)) {
        return expr;
    }

    CError_ReportError(124U);
    expr = (ENode *)nullnode();
    expr->rtype = (Type *)&stchar;
    return expr;
}

static SInt32 TypSize(TypeIntegral *t)
{
    return t->size;
}

CInt64 CExpr_IntegralConstExprType(Type **ptype)
{
    ENode *node;

    node = CExpr_RewriteConst(pointer_generation(conditional_expression()));
    if (node->type == EINTCONST) {
        switch ((char)node->rtype->type) {
            case TYPEINT:
                *ptype = node->rtype;
                return node->data.intval;
            case TYPEENUM:
                *ptype = TYPE_ENUM(node->rtype)->enumtype;
                return node->data.intval;
        }
    }
    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
    *ptype = (Type *)&stchar;
    return cint64_zero;
}

ENode *s_expression(void)
{
    Boolean handled;
    ENode *node;
    ENode *converted;
    ENode *left;
    ENode *right;
    BinaryOperatorResult result;

    node = assignment_expression();
    while (tk == ',') {
        converted = pointer_generation(node);
        left = CExpr_RewriteConst(converted);
        tk = CPrepTokenizer_GetNextToken();
        node = assignment_expression();
        converted = pointer_generation(node);
        right = CExpr_RewriteConst(converted);
        if ((copts.cplusplus != 0) &&
            (handled = CExpr_CheckOperator(0x2c, (ENode *)(left), (ENode *)(right), &result), handled != 0)) {
            node = result.expression;
            if (node == NULL) {
                CError_FATAL(6531);
            }
        } else {
            CExpr_CheckUnusedExpression(left);
            node = makecommaexpression(left, right);
            node->rtype = right->rtype;
        }
    }
    converted = pointer_generation(node);
    return CExpr_RewriteConst(converted);
}

ENode *CExpr_ParseCommaExpression(void)
{
    ENode *node;
    ENode *converted;
    ENode *left;
    ENode *right;
    BinaryOperatorResult result;

    node = assignment_expression();
    while (tk == ',') {
        converted = pointer_generation(node);
        left = CExpr_RewriteConst(converted);
        tk = CPrepTokenizer_GetNextToken();
        node = assignment_expression();
        converted = pointer_generation(node);
        right = CExpr_RewriteConst(converted);
        if ((copts.cplusplus != 0) && (CExpr_CheckOperator(',', (ENode *)(left), (ENode *)(right), &result) != 0)) {
            node = result.expression;
            if (node == NULL) {
                CError_FATAL(6531);
            }
        } else {
            CExpr_CheckUnusedExpression(left);
            node = makecommaexpression(left, right);
            node->rtype = right->rtype;
        }
    }
    return node;
}

void CExpr_CheckUnusedExpression(ENode *node)
{
    ENode *n;
    Boolean result;

    if (copts.warn_possunwant) {
        n = node;
        while (n->type == ETYPCON)
            n = n->data.monadic;
        if (n->type == EEQU) {
            CError_Warning(ERR_POSSIBLE_UNWANTED_COMPARE);
            return;
        }
    }

    if (copts.warn_no_side_effect) {
        switch (node->type) {
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EMUL:
            case EDIV:
            case EMODULO:
            case EADD:
            case ESUB:
            case ESHL:
            case ESHR:
            case ELESS:
            case EGREATER:
            case ELESSEQU:
            case EGREATEREQU:
            case EEQU:
            case ENOTEQU:
            case EAND:
            case EXOR:
            case EOR:
            case ELAND:
            case ELOR:
            case EROTL:
            case EROTR:
            case EBITFIELD:
            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case EOBJREF:
            case EPRECOMP:
            case ETEMP:
            case EARGOBJ:
            case ENEWEXCEPTION:
            case ENEWEXCEPTIONARRAY:
            case EASSBLK:
                result = 0;
                break;
            case ETYPCON:
                result = (node->rtype->type == TYPEVOID);
                break;
            case EINDIRECT:
                switch (node->data.monadic->type) {
                    case EFUNCCALL:
                    case EFUNCCALLP:
                        result = 1;
                        break;
                    default:
                        result = 0;
                        break;
                }
                break;
            case ECOMMA:
                result = CInline_00513910(node->data.diadic.right);
                break;
            case ECOND:
                result = CInline_00513910(node->data.cond.expr1) || CInline_00513910(node->data.cond.expr2);
                break;
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EFORCELOAD:
            case EASS:
            case EMULASS:
            case EDIVASS:
            case EMODASS:
            case EADDASS:
            case ESUBASS:
            case ESHLASS:
            case ESHRASS:
            case EANDASS:
            case EXORASS:
            case EORASS:
            case EFUNCCALL:
            case EFUNCCALLP:
            case EQUALNAME:
            case EMFPOINTER:
            case ENULLCHECK:
            case ELOCOBJ:
            case EMEMBER:
                result = 1;
                break;
            case EMULV:
            case EADDV:
            case ESUBV:
            case EPMODULO:
            case EBCLR:
            case EBTST:
            case EBSET:
            case ETEMPX:
            case ELABEL:
            case ESETCONST:
            case EOBJLIST:
            case EINSTRUCTION:
            case EDEFINE:
            case EREUSE:
            default:
                CError_FATAL(6466);
                result = 0;
                break;
        }
        if (!result) {
            CError_Warning(ERR_EXPRESSION_NO_SIDE_EFFECT);
            return;
        }
    }

    if (copts.warn_resultnotused) {
        ENode *m = node;
        if (node->rtype->type != TYPEVOID) {
            if (node->type == EINDIRECT)
                m = node->data.monadic;
            switch (m->type) {
                case EFUNCCALL:
                case EFUNCCALLP:
                    CError_Warning(ERR_RESULT_FUNCTION_CALL_NOT_USED);
                    return;
                default:
                    break;
            }
        }
    }
}

Boolean fn_004f0f40(ENode *node)
{
    int result;
    Boolean operandResult;

    switch (node->type) {
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case EMUL:
        case EDIV:
        case EMODULO:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
        case ELAND:
        case ELOR:
        case EROTL:
        case EROTR:
        case EBITFIELD:
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case EPRECOMP:
        case ETEMP:
        case EARGOBJ:
        case ENEWEXCEPTION:
        case ENEWEXCEPTIONARRAY:
        case EASSBLK:
            return 0;
        case ETYPCON:
            return node->rtype->type == TYPEVOID;
        case EINDIRECT:
            switch (node->data.objref->otype) {
                case 0x36:
                case 0x37:
                    return 1;
            }
            return 0;
        case ECOMMA:
            return CInline_00513910(node->data.cond.expr1);
        case ECOND:
            result = 1;
            operandResult = CInline_00513910(node->data.cond.expr1);
            if ((operandResult == 0) && (operandResult = CInline_00513910(node->data.cond.expr2), operandResult == 0)) {
                result = 0;
            }
            return result;
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
        case EFORCELOAD:
        case EASS:
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
        case EFUNCCALL:
        case EFUNCCALLP:
        case EQUALNAME:
        case EMFPOINTER:
        case ENULLCHECK:
        case ELOCOBJ:
        case EMEMBER:
            return 1;
        default:
            CError_FATAL(6466);
            return 0;
    }
}

ENode *conv_assignment_expression(void)
{
    ENode *expr;
    ENode *result;

    expr = assignment_expression();
    result = pointer_generation(expr);
    return CExpr_RewriteConst(result);
}

ENode *assignment_expression(void)
{
    ENode *enode;

    if (tk == TK_THROW)
        return CExcept_ScanThrowExpression();

    enode = conditional_expression();

    switch (tk) {
        case '=':
            return parse_assignment_operator(enode, 0x1e, tk);
        case TK_ADD_ASSIGN:
            return AdjustBoolAssign(parse_arithmetic_compound_assignment(enode, 0x22, tk));
        case TK_SUB_ASSIGN:
            return AdjustBoolAssign(parse_arithmetic_compound_assignment(enode, 0x23, tk));
        case TK_MULT_ASSIGN:
            enode = parse_arithmetic_binary_expression(enode, 0x1f, tk);
            if (enode->type == EMULASS && CExpr_IsOne(enode->data.diadic.right))
                return enode->data.diadic.left;
            return AdjustBoolAssign(enode);
        case TK_DIV_ASSIGN:
            enode = parse_arithmetic_binary_expression(enode, 0x20, tk);
            if (enode->type == EDIVASS) {
                if (CExpr2_IsZero(enode->data.diadic.right) && enode->rtype->type != TYPEFLOAT) {
                    CError_Warning(ERR_DIVISION_BY_0);
                    return enode->data.diadic.left;
                }
                if (CExpr_IsOne(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return AdjustBoolAssign(enode);
        case TK_MOD_ASSIGN:
            enode = parse_arithmetic_binary_expression(enode, 0x21, tk);
            if (enode->type == EMODASS) {
                if (CExpr2_IsZero(enode->data.diadic.right)) {
                    CError_Warning(ERR_DIVISION_BY_0);
                    return enode->data.diadic.left;
                }
            }
            return AdjustBoolAssign(enode);
        case TK_SHL_ASSIGN:
            enode = parse_assignment_operator(enode, 0x24, tk);
            if (enode->type == ESHLASS) {
                if (CExpr2_IsZero(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return AdjustBoolAssign(enode);
        case TK_SHR_ASSIGN:
            enode = parse_assignment_operator(enode, 0x25, tk);
            if (enode->type == ESHRASS) {
                if (CExpr2_IsZero(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return AdjustBoolAssign(enode);
        case TK_AND_ASSIGN:
            enode = parse_assignment_operator(enode, 0x26, tk);
            if (enode->type == EANDASS) {
                if (CExpr_AllBitsSet(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return enode;
        case TK_XOR_ASSIGN:
            enode = parse_assignment_operator(enode, 0x27, tk);
            if (enode->type == EXORASS) {
                if (CExpr2_IsZero(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return AdjustBoolAssign(enode);
        case TK_OR_ASSIGN:
            enode = parse_assignment_operator(enode, 0x28, tk);
            if (enode->type == EORASS) {
                if (CExpr2_IsZero(enode->data.diadic.right))
                    return enode->data.diadic.left;
            }
            return AdjustBoolAssign(enode);
    }
    return enode;
}

ENode *parse_arithmetic_binary_expression(ENode *left, char op, SInt16 token)
{
    Boolean wasArray;
    ENode *operand;
    ENode *right;
    ENode *promotedLeft;
    ENode *parsedRight;
    BinaryOperatorResult result;

    wasArray = left->rtype->type == TYPEARRAY;
    left = pointer_generation(left);
    promotedLeft = CExpr_RewriteConst(left);
    operand = promotedLeft;
    if (copts.cplusplus != 0) {
        tk = CPrepTokenizer_GetNextToken();
        parsedRight = CExpr_RewriteConst(pointer_generation(assignment_expression()));
        right = parsedRight;
        if (CExpr_CheckOperator(token, operand, parsedRight, &result)) {
            CE_ASSERT(result.expression == 0, CError_FATAL(6166));
            return result.expression;
        }
        if (operand->rtype->type != TYPEINT && (operand->rtype->type != TYPEFLOAT || op == 33)) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return nullnode();
        }
        promotedLeft = CExpr_LValue(operand, 1, 1);
    } else {
        if (operand->rtype->type == TYPEENUM)
            operand = CExpr_ConvertToIntegral(operand);
        if (operand->rtype->type != TYPEINT && (operand->rtype->type != TYPEFLOAT || op == 33)) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return nullnode();
        }
        promotedLeft = CExpr_LValue(operand, 1, 1);
        tk = CPrepTokenizer_GetNextToken();
        right = CExpr_RewriteConst(pointer_generation(assignment_expression()));
    }
    if (wasArray != 0)
        CError_ReportError(ERR_ILLEGAL_OPERAND);
    if (right->rtype->type == TYPEENUM)
        right = CExpr_ConvertToIntegral(right);
    if (promotedLeft->rtype->type == TYPEINT && right->rtype->type == TYPEFLOAT && op == 33) {
        CError_ReportError(ERR_ILLEGAL_OPERAND);
        return nullnode();
    }
    convert_right_and_make_diadic_node(promotedLeft, right, op);
}

ENode *parse_arithmetic_compound_assignment(ENode *e, char operatorKind, SInt16 operation)
{
    Boolean isArray;
    ENode *right;
    ENode *parsedRight;
    BinaryOperatorResult result;
    ENode *pointerExpr;

    isArray = (e->rtype->type == TYPEARRAY);
    e = CExpr_RewriteConst(pointer_generation(e));

    if (copts.cplusplus != 0) {
        tk = CPrepTokenizer_GetNextToken();
        parsedRight = CExpr_RewriteConst(pointer_generation(assignment_expression()));
        right = parsedRight;
        if (CExpr_CheckOperator(operation, (ENode *)(e), (ENode *)(parsedRight), &result)) {
            CError_ASSERT(6082, result.expression != NULL);
            return result.expression;
        }
        e = CExpr_LValue(e, 1, 1);
    } else {
        e = CExpr_LValue(e, 1, 1);
        tk = CPrepTokenizer_GetNextToken();
        right = CExpr_RewriteConst(pointer_generation(assignment_expression()));
    }

    if (isArray) {
        CError_ReportError(ERR_ILLEGAL_OPERAND);
    }

    switch ((SInt8)e->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
            break;
        case TYPEENUM:
            if (copts.cplusplus != 0) {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                return e;
            }
            e = CExpr_ConvertToIntegral(e);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return e;
    }

    if (right->rtype->type == TYPEENUM) {
        right = CExpr_ConvertToIntegral(right);
    }
    if (CExpr2_IsZero(right) != 0) {
        return e;
    }
    if (e->rtype->type == TYPEPOINTER) {
        if (right->rtype->type == TYPEINT) {
            if (operatorKind == ESUBASS) {
                pointerExpr = make_pointer_subtraction(e, right);
                if (pointerExpr->type == ESUB) {
                    pointerExpr->type = ESUBASS;
                }
            } else {
                pointerExpr = add_pointer_offset(e, right);
                if (pointerExpr->type == EADD) {
                    pointerExpr->type = EADDASS;
                }
            }
            return pointerExpr;
        }
        CError_ReportError(ERR_ILLEGAL_OPERAND);
        return e;
    }
    convert_right_and_make_diadic_node(e, right, operatorKind);
}

ENode *parse_assignment_operator(ENode *expr, UInt8 assignmentKind, SInt16 overloadToken)
{
    ENodeList *callArguments;
    ENode *functionCall;
    ENode *assignment;
    ENode *unwrappedRight;
    BinaryOperatorResult overloadResult;
    ENode *right;
    ENode *left;
    Boolean arrayTarget;

    arrayTarget = (expr->rtype->type == TYPEARRAY);
    left = CExpr_RewriteConst(pointer_generation(expr));
    if (copts.cplusplus != 0 && overloadToken != 0) {
        tk = CPrepTokenizer_GetNextToken();
        right = assignment_expression();
        if (right->type != ENEWEXCEPTIONARRAY)
            right = CExpr_RewriteConst(pointer_generation(right));
        if (CExpr_CheckOperator(overloadToken, left, right, &overloadResult) != 0) {
            if (overloadResult.expression == NULL)
                CError_FATAL(5959);
            return overloadResult.expression;
        }
        if (left->rtype->type == TYPECLASS) {
            if (CClass_AssignmentOperator(TYPE_CLASS(left->rtype)) != NULL)
                CError_ReportError(ERR_ILLEGAL_OPERAND);
        }
        expr = CExpr_LValue(left, 1, 1);
    } else {
        expr = CExpr_LValue(left, 1, 1);
        tk = CPrepTokenizer_GetNextToken();
        right = assignment_expression();
    }
    if (arrayTarget)
        CError_ReportError(ERR_ILLEGAL_OPERAND);
    if (assignmentKind != EASS) {
        if (right->rtype->type != TYPEINT) {
            right = CExpr2_ConvertScalarOperand(right, 1, 0);
            if (right->rtype->type != TYPEINT) {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                return expr;
            }
        }
        if (expr->rtype->type != TYPEINT) {
            if (copts.cplusplus != 0) {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                return expr;
            }
            expr = CExpr_ConvertToIntegral(expr);
            if (expr->rtype->type != TYPEINT) {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                return expr;
            }
        }
        convert_right_and_make_diadic_node(expr, right, assignmentKind);
        return;
    }
    if (expr->rtype->type == TYPECLASS && TYPE_CLASS(expr->rtype)->sominfo != NULL) {
        CError_ReportError(ERR_ASSIGNMENT_NOT_SUPPORTED_SOM_CLASSES);
        return expr;
    }
    right = promote_assignment_operand(expr, right, &copts.warn_implicitconv);
    unwrappedRight = right;
    if (right->rtype->type == TYPEFLOAT && right->type == ETYPCON &&
        right->rtype->size == right->data.monadic->rtype->size) {
        unwrappedRight = right->data.monadic;
    }
    if (expr->type == EINDIRECT && expr->data.monadic->type == EOBJREF && unwrappedRight->type == EINDIRECT) {
        functionCall = right->data.monadic;
        if ((functionCall->type == EFUNCCALL || functionCall->type == EFUNCCALLP) &&
            expr->rtype == functionCall->data.funccall.functype->functype &&
            CMachine_FunctionRequiresMemoryReturn(functionCall->data.funccall.functype) == 1 &&
            (callArguments = functionCall->data.funccall.args) != NULL) {
            switch (CInline_ReturnZero((Type *)functionCall->data.funccall.functype)) {
                case 0:
                    break;
                case 1:
                    callArguments = callArguments->next;
                    if (callArguments != NULL)
                        break;
                    CError_FATAL(6041);
                default:
                    CError_FATAL(6042);
            }
            if (callArguments->node->type == EPRECOMP &&
                (expr->rtype->type != TYPECLASS || (CClass_Destructor(TYPE_CLASS(expr->rtype)) == NULL &&
                                                    CClass_AssignmentOperator(TYPE_CLASS(expr->rtype)) == NULL))) {
                callArguments->node = getnodeaddress(expr, 0);
                return right;
            }
        }
    }
    assignment = makediadicnode(expr, right, assignmentKind);
    assignment->flags = expr->flags;
    return assignment;
}

void convert_right_and_make_diadic_node(ENode *left, ENode *right, UInt8 type)
{
    if (left->rtype != right->rtype) {
        switch ((SInt8)right->rtype->type) {
            case TYPEINT:
            case TYPEFLOAT:
                break;
            case TYPEENUM:
                right->rtype = TYPE_ENUM(right->rtype)->enumtype;
                break;
            default:
                right = oldassignmentpromotion(right, left->rtype, 0, 1);
                break;
        }

        if (left->rtype->type == TYPEFLOAT) {
            if (right->rtype->type == TYPEINT ||
                (right->rtype->type == TYPEFLOAT && left->rtype->size >= right->rtype->size))
                right = oldassignmentpromotion(right, left->rtype, 0, 1);
        } else if (left->rtype->type == TYPEINT) {
            if (right->rtype->type == TYPEINT && (left->rtype->size > right->rtype->size ||
                                                  (left->rtype->size == right->rtype->size &&
                                                   Type_IsUnsigned(left->rtype) == Type_IsUnsigned(right->rtype))))
                right = oldassignmentpromotion(right, left->rtype, 0, 1);
        }
    }

    makediadicnode(left, right, type);
}

ENode *conditional_expression(void)
{
    Boolean td;
    SInt32 simple;
    ENode *cond;
    ENode *cv;
    ENode *e;
    ENode *left;
    ENode *right;
    BinaryOperatorResult tmp;
    ENode *then;
    ENode *els;
    ENode *p;

    td = 0;
    cond = parse_binary_expression(NULL, 0, 0);
    if (tk != '?')
        return cond;

    cv = CExpr_RewriteConst(pointer_generation(cond));
    cond = cv;
    if (cv->rtype->type != TYPETEMPLDEPEXPR) {
        cond = CExpr2_ConvertScalarOperand(cv, 0, 1);
        if (cond->rtype->type != TYPEINT && cond->rtype->type != TYPEFLOAT && cond->rtype->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return nullnode();
        }
    } else {
        td = 1;
    }

    tk = CPrepTokenizer_GetNextToken();
    e = assignment_expression();
    while (tk == ',') {
        left = CExpr_RewriteConst(pointer_generation(e));
        tk = CPrepTokenizer_GetNextToken();
        right = CExpr_RewriteConst(pointer_generation(assignment_expression()));
        if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2c, left, right, &tmp)) {
            e = tmp.expression;
            CError_ASSERT(6531, tmp.expression != 0);
        } else {
            CExpr_CheckUnusedExpression(left);
            e = makecommaexpression(left, right);
            e->rtype = right->rtype;
        }
    }
    then = CExpr_RewriteConst(pointer_generation(e));
    if (tk != ':') {
        CError_ReportErrorAndUpdateToken(ERR_EXPRESSION_SYNTAX_ERROR);
    } else {
        tk = CPrepTokenizer_GetNextToken();
    }
    simple = copts.cplusplus && !copts.ARM_conform;
    if (simple)
        els = assignment_expression();
    else
        els = conditional_expression();
    if (td || then->rtype->type == TYPETEMPLDEPEXPR || els->rtype->type == TYPETEMPLDEPEXPR) {
        p = CExpr_NewENode(0x35);
        p->rtype = &data_0055d5c0;
        p->data.cond.cond = cond;
        p->data.cond.expr1 = then;
        p->data.cond.expr2 = els;
        return p;
    }
    return CExpr_New_ECOND_Node(cond, then, els);
}

#include <string.h>

ENode *CExpr_New_ECOND_Node(ENode *condition, ENode *trueExpr, ENode *falseExpr)
{
    SInt16 cost;
    ENode *node;
    BinaryOperatorResult conversion;
    ENode *trueOperand;
    ENode *falseOperand;

    condition = CExpr2_ConvertScalarOperand(normalize(condition), 0, 1);
    trueExpr = normalize(trueExpr);
    falseExpr = normalize(falseExpr);

    if (condition->rtype->type != TYPEINT && condition->rtype->type != TYPEFLOAT &&
        condition->rtype->type != TYPEPOINTER) {
        CError_ReportError(ERR_ILLEGAL_OPERAND);
        return nullnode();
    }

    cost = (SInt16)(condition->cost + 1);
    if (trueExpr->cost > falseExpr->cost)
        cost += trueExpr->cost;
    else
        cost += falseExpr->cost;
    if (falseExpr->cost > cost)
        cost = falseExpr->cost;
    if (cost > 200)
        cost = 200;
    node = CExpr_NewENode(ECOND);
    node->cost = (UInt8)cost;
    node->rtype = trueExpr->rtype;
    node->flags = (UInt16)(trueExpr->flags | falseExpr->flags);
    node->data.cond.cond = condition;

    if (trueExpr->type == EFUNCCALL && trueExpr->rtype == &stvoid && (trueExpr->flags & 2)) {
        node->rtype = falseExpr->rtype;
        node->flags = falseExpr->flags;
        node->data.cond.expr1 = trueExpr;
        node->data.cond.expr2 = falseExpr;
        return node;
    }
    if (falseExpr->type == EFUNCCALL && falseExpr->rtype == &stvoid && (falseExpr->flags & 2)) {
        node->rtype = trueExpr->rtype;
        node->flags = trueExpr->flags;
        node->data.cond.expr1 = trueExpr;
        node->data.cond.expr2 = falseExpr;
        return node;
    }
    if (trueExpr->type == EINDIRECT && falseExpr->type == EINDIRECT &&
        iscpp_typeequal(trueExpr->rtype, falseExpr->rtype) && (trueExpr->flags & 3) == (falseExpr->flags & 3)) {
        ENode *indirect;

        if (IsAggregateType(trueExpr->rtype) && trueExpr->data.monadic->type != EBITFIELD &&
            falseExpr->data.monadic->type != EBITFIELD) {
            if (isnotzero(condition))
                return trueExpr;
            if (CExpr2_IsZero(condition))
                return falseExpr;
            node->data.cond.expr1 = getnodeaddress(trueExpr, 0);
            node->data.cond.expr2 = getnodeaddress(falseExpr, 0);
            node->rtype = node->data.cond.expr1->rtype;
            indirect = makemonadicnode(node, EINDIRECT);
            indirect->rtype = TPTR_TARGET(indirect->rtype);
            return indirect;
        }
    }
    if ((trueExpr->rtype->type == TYPECLASS || falseExpr->rtype->type == TYPECLASS) &&
        !iscpp_typeequal(trueExpr->rtype, falseExpr->rtype)) {
        ENodeList *arguments = CompilerTools_AllocatePool(sizeof(ENodeList));
        arguments->node = trueExpr;
        arguments->next = CompilerTools_AllocatePool(sizeof(ENodeList));
        arguments->next->node = falseExpr;
        arguments->next->next = NULL;
        if (CExpr_CheckOperatorConversion(EMFPOINTER, trueExpr, falseExpr, arguments, &conversion)) {
            if (conversion.expression != NULL)
                CError_FATAL(5726);
            trueExpr = conversion.left;
            falseExpr = conversion.right;
        }
        node->rtype = trueExpr->rtype;
    }

    switch ((SInt8)trueExpr->rtype->type) {
        case TYPEENUM:
            if (trueExpr->rtype == falseExpr->rtype)
                break;
            trueExpr = CExpr_ConvertToIntegral(trueExpr);
        case TYPEINT:
            if (falseExpr->rtype->type == TYPEPOINTER && trueExpr->type == EINTCONST &&
                IsZeroCInt64(&trueExpr->data.intval)) {
                trueExpr->rtype = (Type *)&stunsignedlong;
                node->rtype = falseExpr->rtype;
                break;
            }
        case TYPEFLOAT:
            if (trueExpr->rtype != falseExpr->rtype) {
                CExpr_ArithmeticConversion(&trueExpr, &falseExpr);
                node->rtype = trueExpr->rtype;
            }
            break;
        case TYPEPOINTER:
            if (falseExpr->type == EINTCONST && IsZeroCInt64(&falseExpr->data.intval)) {
                falseExpr->rtype = (Type *)&stunsignedlong;
                break;
            }
            if (falseExpr->rtype->type == TYPEPOINTER) {
                if (TPTR_TARGET(trueExpr->rtype)->type == TYPECLASS &&
                    TPTR_TARGET(falseExpr->rtype)->type == TYPECLASS) {
                    if (TPTR_TARGET(trueExpr->rtype) != TPTR_TARGET(falseExpr->rtype)) {
                        ENode *convertedTrue;
                        ENode *falseCopy;
                        ENode *convertedFalse;
                        CClass_Init();
                        if (CClass_FindBasePath(TYPE_CLASS(TPTR_TARGET(trueExpr->rtype)),
                                                TYPE_CLASS(TPTR_TARGET(falseExpr->rtype)), 0, 1)) {
                            TypeClass *targetClass;
                            targetClass = TYPE_CLASS(TPTR_TARGET(falseExpr->rtype));
                            convertedTrue = (ENode *)TPTR_TARGET(trueExpr->rtype);
                            trueOperand = trueExpr;
                            CClass_Init();
                            convertedTrue =
                                CClass_ConvertClassPointer(trueOperand, (TypeClass *)convertedTrue, targetClass, 0, 1);
                            if (convertedTrue != trueOperand &&
                                !(convertedTrue->type == ETYPCON && convertedTrue->data.monadic == trueOperand) &&
                                !isnotzero(trueOperand)) {
                                ENode *copy = CompilerTools_AllocatePool(sizeof(ENode));
                                *copy = *convertedTrue;
                                copy->type = EMFPOINTER;
                                copy->data.precomp.label = CompilerTools_AllocatePool(sizeof(ENode));
                                *copy->data.precomp.label = *trueOperand;
                                copy->data.precomp.expression = convertedTrue;
                                copy->data.precomp.labelId = CParser_GetUniqueID();
                                trueOperand->type = ENULLCHECK;
                                trueOperand->data.longval = copy->data.precomp.labelId;
                                convertedTrue = copy;
                            }
                            trueExpr = convertedTrue;
                            convertedTrue->rtype = falseExpr->rtype;
                        } else if (CClass_FindBasePath(TYPE_CLASS(TPTR_TARGET(falseExpr->rtype)),
                                                       TYPE_CLASS(TPTR_TARGET(trueExpr->rtype)), 0, 1)) {
                            TypeClass *sourceClass;
                            convertedFalse = (ENode *)TPTR_TARGET(trueExpr->rtype);
                            sourceClass = TYPE_CLASS(TPTR_TARGET(falseExpr->rtype));
                            falseOperand = falseExpr;
                            CClass_Init();
                            convertedFalse = CClass_ConvertClassPointer(falseOperand, sourceClass,
                                                                        (TypeClass *)convertedFalse, 0, 1);
                            if (convertedFalse != falseOperand &&
                                !(convertedFalse->type == ETYPCON && convertedFalse->data.monadic == falseOperand) &&
                                !isnotzero(falseOperand)) {
                                falseCopy = CompilerTools_AllocatePool(sizeof(ENode));
                                *falseCopy = *convertedFalse;
                                falseCopy->type = EMFPOINTER;
                                falseCopy->data.precomp.label = CompilerTools_AllocatePool(sizeof(ENode));
                                *falseCopy->data.precomp.label = *falseOperand;
                                falseCopy->data.precomp.expression = convertedFalse;
                                falseCopy->data.precomp.labelId = CParser_GetUniqueID();
                                falseOperand->type = ENULLCHECK;
                                falseOperand->data.longval = falseCopy->data.precomp.labelId;
                                convertedFalse = falseCopy;
                            }
                            falseExpr = convertedFalse;
                            convertedFalse->rtype = trueExpr->rtype;
                        } else {
                            CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3,
                                               falseExpr->rtype, falseExpr->flags & 3);
                        }
                        (void)convertedTrue;
                        (void)convertedFalse;
                    }
                    node->rtype = trueExpr->rtype;
                    break;
                }
                if (TPTR_TARGET(falseExpr->rtype) == &stvoid)
                    node->rtype = falseExpr->rtype;
            }
            if (!is_typesame(trueExpr->rtype, falseExpr->rtype)) {
                if (copts.objective_c && CObjC_IsIdCompatiblePointerPair(trueExpr->rtype, falseExpr->rtype)) {
                    trueExpr->rtype = falseExpr->rtype = CObjC_GetIdType(1);
                } else {
                    CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3, falseExpr->rtype,
                                       falseExpr->flags & 3);
                }
            }
            break;
        case TYPEVOID:
            if (!is_typesame(trueExpr->rtype, falseExpr->rtype))
                CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3, falseExpr->rtype,
                                   falseExpr->flags & 3);
            break;
        case TYPESTRUCT:
        case TYPECLASS:
            if (!is_typesame(trueExpr->rtype, falseExpr->rtype))
                CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3, falseExpr->rtype,
                                   falseExpr->flags & 3);
            node->rtype = trueExpr->rtype;
            break;
        case TYPEMEMBERPOINTER:
            if (!is_typesame(trueExpr->rtype, falseExpr->rtype)) {
                if (falseExpr->rtype->type == TYPEMEMBERPOINTER)
                    trueExpr = CExpr_CastMemberPointer(trueExpr, TYPE_MEMBER_POINTER(trueExpr->rtype),
                                                       TYPE_MEMBER_POINTER(falseExpr->rtype));
                else
                    CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3, falseExpr->rtype,
                                       falseExpr->flags & 3);
            }
            node->rtype = trueExpr->rtype;
            break;
        default:
            CError_ReportError(ERR_TYPE_MISMATCH, trueExpr->rtype, trueExpr->flags & 3, falseExpr->rtype,
                               falseExpr->flags & 3);
            return nullnode();
    }

    node->data.cond.expr1 = trueExpr;
    node->data.cond.expr2 = falseExpr;
    if (isnotzero(condition))
        node = trueExpr;
    else if (CExpr2_IsZero(condition))
        node = falseExpr;
    return node;
}

ENode *parse_binary_expression(ENode *left, unsigned char precedence, char conditional)
{
    ENode *leftValue;
    ENode *result;
    ENode *rightValue;
    ENode *right;
    ENode *convertedRight;
    char logical;
    char done;
    char savedTemplateArgumentMode;
    struct BinaryOperatorInfo {
        unsigned char kind;
        unsigned char precedence;
    } operatorInfo, lookaheadOperator;
    BinaryOperatorResult conversion;

    savedTemplateArgumentMode = non_type_template_argument_mode;
    if (left == NULL) {
        non_type_template_argument_mode = 0;
        left = member_pointer_expression();
        non_type_template_argument_mode = savedTemplateArgumentMode;
    }
    do {
        if (get_binary_operator_info(tk, &operatorInfo.kind) == 0)
            return left;
        switch (operatorInfo.kind) {
            case ELAND:
                warn_unwanted_assignment(left);
                if (CExpr2_IsZero(left) != 0)
                    conditional = 1;
                logical = 1;
                break;
            case ELOR:
                warn_unwanted_assignment(left);
                if (isnotzero(left) != 0)
                    conditional = 1;
                logical = 1;
                break;
            default:
                logical = 0;
        }
        tk = CPrepTokenizer_GetNextToken();
        non_type_template_argument_mode = 0;
        right = member_pointer_expression();
        non_type_template_argument_mode = savedTemplateArgumentMode;
        for (;;) {
            if (get_binary_operator_info(tk, &lookaheadOperator.kind) != 0) {
                if (operatorInfo.precedence >= lookaheadOperator.precedence) {
                    done = lookaheadOperator.precedence <= precedence;
                    break;
                }
                right = parse_binary_expression(right, operatorInfo.precedence, conditional);
            } else {
                done = 1;
                break;
            }
        }
        if (logical != 0)
            warn_unwanted_assignment(right);
        switch (operatorInfo.kind) {
            case EDIV:
                if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
                    left = CTempl_MakeTemplDepExpr(left, EDIV, right);
                    continue;
                }
                leftValue = CExpr_RewriteConst(pointer_generation(left));
                left = leftValue;
                rightValue = CExpr_RewriteConst(pointer_generation(right));
                convertedRight = rightValue;
                if (copts.cplusplus != 0 && CExpr_CheckOperator('/', leftValue, rightValue, &conversion) != 0) {
                    if (conversion.expression != NULL) {
                        result = conversion.expression;
                        break;
                    }
                    CE_ASSERT((left = conversion.left) == 0, CError_FATAL(4139));
                    CE_ASSERT((convertedRight = conversion.right) == 0, CError_FATAL(4140));
                }
                result = CExpr_New_EDIV_Node(left, convertedRight, conditional);
                break;
            case EMODULO:
                result = CExpr_New_EMODULO_Node(left, right, conditional);
                break;
            default:
                result = CExpr_NewDyadicNode(left, operatorInfo.kind, right);
        }
        left = result;
    } while (done == 0);
    return left;
}

unsigned char get_binary_operator_info(short token, unsigned char *operatorInfo)
{
    switch (token) {
        case '*':
            operatorInfo[0] = 9;
            operatorInfo[1] = 20;
            return 1;
        case '/':
            operatorInfo[0] = 11;
            operatorInfo[1] = 20;
            return 1;
        case '%':
            operatorInfo[0] = 12;
            operatorInfo[1] = 20;
            return 1;
        case '+':
            operatorInfo[0] = 15;
            operatorInfo[1] = 19;
            return 1;
        case '-':
            operatorInfo[0] = 16;
            operatorInfo[1] = 19;
            return 1;
        case 364:
            operatorInfo[0] = 17;
            operatorInfo[1] = 18;
            return 1;
        case 365:
            operatorInfo[0] = 18;
            operatorInfo[1] = 18;
            return 1;
        case '<':
            operatorInfo[0] = 19;
            operatorInfo[1] = 17;
            return 1;
        case 362:
            operatorInfo[0] = 21;
            operatorInfo[1] = 17;
            return 1;
        case '>':
            if (non_type_template_argument_mode != 0)
                return 0;
            operatorInfo[0] = 20;
            operatorInfo[1] = 17;
            return 1;
        case 363:
            operatorInfo[0] = 22;
            operatorInfo[1] = 17;
            return 1;
        case 360:
            operatorInfo[0] = 23;
            operatorInfo[1] = 16;
            return 1;
        case 361:
            operatorInfo[0] = 24;
            operatorInfo[1] = 16;
            return 1;
        case '&':
            operatorInfo[0] = 25;
            operatorInfo[1] = 15;
            return 1;
        case '^':
            operatorInfo[0] = 26;
            operatorInfo[1] = 14;
            return 1;
        case '|':
            operatorInfo[0] = 27;
            operatorInfo[1] = 13;
            return 1;
        case 359:
            operatorInfo[0] = 28;
            operatorInfo[1] = 12;
            return 1;
        case 358:
            operatorInfo[0] = 29;
            operatorInfo[1] = 11;
            return 1;
    }
    return 0;
}

ENode *CExpr_NewDyadicNode(ENode *left, UInt8 op, ENode *right)
{
    switch (op) {
        default:
            CError_FATAL(5112);

        case EADD: {
            BinaryOperatorResult fold;
            ENode *l, *r;
            ENode *lnode, *rnode;
            ENode *result;
            do {
                if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
                    result = CTempl_MakeTemplDepExpr(left, EADD, right);
                } else {
                    lnode = CExpr_RewriteConst(pointer_generation(left));
                    l = lnode;
                    rnode = CExpr_RewriteConst(pointer_generation(right));
                    r = rnode;
                    if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2b, (ENode *)(lnode), (ENode *)(rnode),
                                                                    (BinaryOperatorResult *)(&fold))) {
                        if (fold.expression != NULL) {
                            result = fold.expression;
                            break;
                        }
                        if ((l = fold.left) == NULL)
                            CError_FATAL(4212);
                        if ((r = fold.right) == NULL)
                            CError_FATAL(4213);
                    }
                    result = CExpr_New_EADD_Node(l, r);
                }
            } while (0);
            return result;
        }

        case ESUB: {
            BinaryOperatorResult fold;
            ENode *l, *r;
            ENode *lnode, *rnode;
            ENode *result;
            do {
                if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
                    result = CTempl_MakeTemplDepExpr(left, ESUB, right);
                } else {
                    lnode = CExpr_RewriteConst(pointer_generation(left));
                    l = lnode;
                    rnode = CExpr_RewriteConst(pointer_generation(right));
                    r = rnode;
                    if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2d, (ENode *)(lnode), (ENode *)(rnode),
                                                                    (BinaryOperatorResult *)(&fold))) {
                        if (fold.expression != NULL) {
                            result = fold.expression;
                            break;
                        }
                        if ((l = fold.left) == NULL)
                            CError_FATAL(4237);
                        if ((r = fold.right) == NULL)
                            CError_FATAL(4238);
                    }
                    result = CExpr_New_ESUB_Node(l, r);
                }
            } while (0);
            return result;
        }

        case EMUL: {
            BinaryOperatorResult fold;
            ENode *l, *r;
            ENode *lnode, *rnode;
            ENode *result;
            do {
                if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
                    result = CTempl_MakeTemplDepExpr(left, EMUL, right);
                } else {
                    lnode = CExpr_RewriteConst(pointer_generation(left));
                    l = lnode;
                    rnode = CExpr_RewriteConst(pointer_generation(right));
                    r = rnode;
                    if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2a, (ENode *)(lnode), (ENode *)(rnode),
                                                                    (BinaryOperatorResult *)(&fold))) {
                        if (fold.expression != NULL) {
                            result = fold.expression;
                            break;
                        }
                        if ((l = fold.left) == NULL)
                            CError_FATAL(4113);
                        if ((r = fold.right) == NULL)
                            CError_FATAL(4114);
                    }
                    result = CExpr_New_EMUL_Node(l, r);
                }
            } while (0);
            return result;
        }

        case EDIV: {
            BinaryOperatorResult fold;
            ENode *l, *r;
            ENode *lnode, *rnode;
            ENode *result;
            do {
                if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
                    result = CTempl_MakeTemplDepExpr(left, EDIV, right);
                } else {
                    lnode = CExpr_RewriteConst(pointer_generation(left));
                    l = lnode;
                    rnode = CExpr_RewriteConst(pointer_generation(right));
                    r = rnode;
                    if (copts.cplusplus != 0 && CExpr_CheckOperator(0x2f, (ENode *)(lnode), (ENode *)(rnode),
                                                                    (BinaryOperatorResult *)(&fold))) {
                        if (fold.expression != NULL) {
                            result = fold.expression;
                            break;
                        }
                        if ((l = fold.left) == NULL)
                            CError_FATAL(4139);
                        if ((r = fold.right) == NULL)
                            CError_FATAL(4140);
                    }
                    result = CExpr_New_EDIV_Node(l, r, 1);
                }
            } while (0);
            return result;
        }
        case EMODULO:
            return CExpr_New_EMODULO_Node(left, right, 1);
        case EAND:
            return CExpr_New_EAND_Node(left, right);
        case EXOR:
            return CExpr_New_EXOR_Node(left, right);
        case EOR:
            return CExpr_New_EOR_Node(left, right);
        case ESHL:
            return CExpr_New_ESHL_Node(left, right);
        case ESHR:
            return CExpr_New_ESHR_Node(left, right);
        case ELESS:
            return CExpr_New_ELESS_Node(left, right);
        case EGREATER:
            return CExpr_New_EGREATER_Node(left, right);
        case ELESSEQU:
            return CExpr_New_ELESSEQU_Node(left, right);
        case EGREATEREQU:
            return CExpr_New_EGREATEREQU_Node(left, right);
        case EEQU:
            return CExpr_MakeComparisonNode(left, right);
        case ENOTEQU:
            return fold_or_make_comparison_node(left, right);
        case ELAND:
            return CExpr_New_ELAND_Node(left, right);
        case ELOR:
            return make_logical_or_node(left, right);
    }
    return NULL;
}

ENode *make_logical_or_node(ENode *left, ENode *right)
{
    ENode *result;
    ENode *convertedRight;
    BinaryOperatorResult nodes;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, 0x1d, right);

    result = CExpr_RewriteConst(pointer_generation(left));
    left = result;
    convertedRight = CExpr_RewriteConst(pointer_generation(right));
    right = convertedRight;

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x166, left, convertedRight, &nodes)) {
            if (nodes.expression != NULL)
                return nodes.expression;
            CError_ASSERT(5022, (left = nodes.left) != NULL);
            CError_ASSERT(5023, (right = nodes.right) != NULL);
        }
    }

    switch ((SInt8)left->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
        case TYPEARRAY:
            break;
        case TYPEENUM:
        case TYPEMEMBERPOINTER:
            left = CExpr2_ConvertScalarOperand(left, 0, 0);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            left = nullnode();
    }

    switch ((SInt8)right->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
        case TYPEARRAY:
            break;
        case TYPEENUM:
        case TYPEMEMBERPOINTER:
            right = CExpr2_ConvertScalarOperand(right, 0, 0);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            right = nullnode();
    }

    if (isnotzero(left))
        return CExpr_intnode(left, 1);

    if (CExpr2_IsZero(left)) {
        if (CExpr2_IsZero(right))
            return CExpr_intnode(left, 0);
        if (isnotzero(right))
            return CExpr_intnode(left, 1);
        result = makemonadicnode(right, 7);
        result->rtype = CParser_GetBoolType();
        return makemonadicnode(result, 7);
    }

    if (isnotzero(right)) {
        CExpr_intnode(right, 1);
    } else if (CExpr2_IsZero(right)) {
        result = makemonadicnode(left, 7);
        result->rtype = CParser_GetBoolType();
        return makemonadicnode(result, 7);
    }

    result = makediadicnode(left, right, 0x1d);
    result->rtype = CParser_GetBoolType();
    return result;
}

ENode *CExpr_New_ELAND_Node(ENode *left, ENode *right)
{
    ENode *convertedLeft;
    ENode *convertedRight;
    BinaryOperatorResult result;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, ELAND, right);

    convertedLeft = CExpr_RewriteConst(pointer_generation(left));
    left = convertedLeft;
    convertedRight = CExpr_RewriteConst(pointer_generation(right));
    right = convertedRight;

    if (copts.cplusplus != 0 && CExpr_CheckOperator(0x167, convertedLeft, convertedRight, &result) != 0) {
        if (result.expression != NULL)
            return result.expression;
        CError_ASSERT(4926, (left = result.left) != 0);
        CError_ASSERT(4927, (right = result.right) != 0);
    }

    switch ((SInt8)left->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
        case TYPEARRAY:
            break;
        case TYPEENUM:
        case TYPEMEMBERPOINTER:
            left = CExpr2_ConvertScalarOperand(left, 0, 0);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            left = nullnode();
            break;
    }

    switch ((SInt8)right->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
        case TYPEARRAY:
            break;
        case TYPEENUM:
        case TYPEMEMBERPOINTER:
            right = CExpr2_ConvertScalarOperand(right, 0, 0);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            right = nullnode();
            break;
    }

    if (CExpr2_IsZero(left) != 0) {
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
        left->data.intval.lo = 0;
        left->data.intval.hi = 0;
        return left;
    }

    if (isnotzero(left) != 0) {
        if (CExpr2_IsZero(right) != 0) {
            left->type = EINTCONST;
            left->rtype = CParser_GetBoolType();
            left->data.intval.lo = 0;
            left->data.intval.hi = 0;
            return left;
        }
        if (isnotzero(right) != 0) {
            left->type = EINTCONST;
            left->rtype = CParser_GetBoolType();
            left->data.intval.lo = 1;
            left->data.intval.hi = 0;
            return left;
        }
        left = makemonadicnode(right, ELOGNOT);
        left->rtype = CParser_GetBoolType();
        return makemonadicnode(left, ELOGNOT);
    } else {
        if (isnotzero(right) != 0) {
            left = makemonadicnode(left, ELOGNOT);
            left->rtype = CParser_GetBoolType();
            return makemonadicnode(left, ELOGNOT);
        }
        if (CExpr2_IsZero(right) != 0) {
            right->type = EINTCONST;
            right->rtype = CParser_GetBoolType();
            right->data.intval.lo = 0;
            right->data.intval.hi = 0;
            return makecommaexpression(left, right);
        }
        right = makediadicnode(left, right, ELAND);
        right->rtype = CParser_GetBoolType();
        return right;
    }
}

ENode *CExpr_New_EOR_Node(ENode *left, ENode *right)
{
    BinaryOperatorResult out;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, 0x1b, right);

    left = recovered_normalize(left);
    right = recovered_normalize(right);

    if (copts.cplusplus && CExpr_CheckOperator(0x7c, left, right, &out)) {
        if (out.expression)
            return out.expression;
        left = out.left;
        CError_ASSERT(4890, left != NULL);
        right = out.right;
        CError_ASSERT(4891, right != NULL);
    }

    left = forceintegral(left);
    right = forceintegral(right);
    CExpr_ArithmeticConversion(&left, &right);

    if (CExpr2_IsZero(right) || CExpr_AllBitsSet(left))
        return left;
    if (CExpr2_IsZero(left) || CExpr_AllBitsSet(right))
        return right;

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 0x7c, right->data.intval);
        return left;
    }

    left = makediadicnode(left, right, 0x1b);
    optimizecomm(left);
    return left;
}

ENode *CExpr_New_EXOR_Node(ENode *left, ENode *right)
{
    BinaryOperatorResult operatorResult;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, EXOR, right);

    left = generate_pointer_and_rewrite_const(left);
    right = generate_pointer_and_rewrite_const(right);
    if (copts.cplusplus) {
        if (CExpr_CheckOperator('^', left, right, &operatorResult)) {
            if (operatorResult.expression)
                return operatorResult.expression;
            left = operatorResult.left;
            CError_ASSERT(4854, operatorResult.left != 0);
            right = operatorResult.right;
            CError_ASSERT(4855, operatorResult.right != 0);
        }
    }
    left = forceintegral(left);
    right = forceintegral(right);
    CExpr_ArithmeticConversion(&left, &right);
    if (CExpr2_IsZero(right))
        return left;
    if (CExpr2_IsZero(left))
        return right;
    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, '^', right->data.intval);
        return left;
    }
    left = makediadicnode(left, right, EXOR);
    optimizecomm(left);
    return left;
}

ENode *CExpr_New_EAND_Node(ENode *left, ENode *right)
{
    BinaryOperatorResult comb;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, EAND, right);

    left = pointer_generation_and_rewrite_const(left);
    right = pointer_generation_and_rewrite_const(right);

    if (copts.cplusplus &&
        CExpr_CheckOperator(EANDASS, (ENode *)(left), (ENode *)(right), (BinaryOperatorResult *)(&comb))) {
        {
            if (comb.expression != NULL)
                return comb.expression;
            left = comb.left;
            CError_ASSERT(4818, left != NULL);
            right = comb.right;
            CError_ASSERT(4819, right != NULL);
        }
    }

    left = forceintegral(left);
    right = forceintegral(right);
    CExpr_ArithmeticConversion(&left, &right);

    if (CExpr2_IsZero(left) || CExpr_AllBitsSet(right))
        return left;
    if (CExpr2_IsZero(right) || CExpr_AllBitsSet(left))
        return right;

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, EANDASS, right->data.intval);
        return left;
    }

    left = makediadicnode(left, right, EAND);
    optimizecomm(left);
    return left;
}

ENode *fold_or_make_comparison_node(ENode *left, ENode *right)
{
    BinaryOperatorResult result;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, 0x18, right);

    left = rewrite_const_after_pointer_generation(left);
    right = rewrite_const_after_pointer_generation(right);

    if (copts.cplusplus && CExpr_CheckOperator(0x169, left, right, &result)) {
        if (result.expression != NULL)
            return result.expression;
        left = result.left;
        if (left == NULL)
            CError_FATAL(4766);
        right = result.right;
        if (right == NULL)
            CError_FATAL(4767);
    }

    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(0x18, left, right);
        return;
    }

    if (left->rtype->type == TYPEMEMBERPOINTER || right->rtype->type == TYPEMEMBERPOINTER)
        return memberpointercompare(0x18, left, right);

    unify_arithmetic_rtypes(&left, &right, 1);

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 0x169, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        recovered_set_integer(left, CMach_CalcFloatDiadicBool(left->rtype, left->data.floatval.data.value, 0x169,
                                                              right->data.floatval.data.value));
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = makediadicnode(left, right, 0x18);
        optimizecomm(left);
    }

    {
        ENode *comparison = left;

        if (copts.cplusplus && copts.booltruefalse)
            comparison->rtype = (Type *)&stbool;
        else
            comparison->rtype = (Type *)&stsignedint;
        return comparison;
    }
}

ENode *CExpr_MakeComparisonNode(ENode *left, ENode *right)
{
    BinaryOperatorResult out;
    Boolean t;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, 0x17, right);

    left = rewrite_pointer_generation_const(left);
    right = rewrite_pointer_generation_const(right);

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x168, (ENode *)(left), (ENode *)(right), &out)) {
            if (out.expression != NULL)
                return out.expression;
            left = out.left;
            CError_ASSERT(4713, left != NULL);
            right = out.right;
            CError_ASSERT(4714, right != NULL);
        }
    }

    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(0x17, left, right);
        return;
    }

    if (left->rtype->type == TYPEMEMBERPOINTER || right->rtype->type == TYPEMEMBERPOINTER)
        return memberpointercompare(0x17, left, right);

    unify_arithmetic_rtypes(&left, &right, 1);

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 0x168, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        ENode *floatLeft, *floatRight;
        left->type = EINTCONST;
        floatLeft = (ENode *)left;
        floatRight = (ENode *)right;
        t = CMach_CalcFloatDiadicBool(left->rtype, floatLeft->data.floatval.data.value, 0x168,
                                      floatRight->data.floatval.data.value);
        recovered_set_integer(left, t);
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = makediadicnode(left, right, 0x17);
        optimizecomm(left);
    }

    {
        ENode *recovered_result = left;
        if (copts.cplusplus != 0 && copts.booltruefalse > 0)
            recovered_result->rtype = (Type *)&stbool;
        else
            recovered_result->rtype = (Type *)&stsignedint;

        return recovered_result;
    }
}

ENode *memberpointercompare(UInt8 op, ENode *left, ENode *right)
{
    ENodeList *args;
    Object *func;
    ENode *node;

    if (left->rtype->type != TYPEMEMBERPOINTER) {
        if (!(left->rtype->type == TYPEINT && left->type == EINTCONST && IsZeroValue(left))) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return nullnode();
        }
    } else if (right->rtype->type != TYPEMEMBERPOINTER) {
        if (!(right->rtype->type == TYPEINT && right->type == EINTCONST && IsZeroValue(right))) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return nullnode();
        }
    } else if (is_typesame(left->rtype, right->rtype) == 0) {
        left = CExpr_CastMemberPointer(left, TYPE_MEMBER_POINTER(left->rtype), TYPE_MEMBER_POINTER(right->rtype));
    }

    if ((left->type == EINTCONST || TYPE_MEMBER_POINTER(left->rtype)->ty1->type != TYPEFUNC) &&
        (right->type == EINTCONST || TYPE_MEMBER_POINTER(right->rtype)->ty1->type != TYPEFUNC)) {
        left->rtype = (Type *)&stsignedlong;
        right->rtype = (Type *)&stsignedlong;
        node = makediadicnode(left, right, op);
        if (copts.cplusplus > 0 && copts.booltruefalse != 0)
            node->rtype = (Type *)&stbool;
        else
            node->rtype = (Type *)&stsignedint;
        return node;
    }

    args = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    if (left->type == EINTCONST || right->type == EINTCONST) {
        func = memberpointercompare_func;
        if (left->type == EINTCONST)
            args->node = getnodeaddress(right, 0);
        else
            args->node = getnodeaddress(left, 0);
        args->next = NULL;
    } else {
        func = rt_memberpointercompare;
        args->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
        args->node = getnodeaddress(left, 0);
        args->next->node = getnodeaddress(right, 0);
        args->next->next = NULL;
    }
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = EFUNCCALL;
    node->rtype = (Type *)&stsignedlong;
    node->cost = 4;
    node->data.funccall.funcref = create_objectrefnode(func);
    node->data.funccall.args = args;
    node->data.funccall.functype = TYPE_FUNC(func->type);
    node->flags = TYPE_FUNC(func->type)->qual & Q_CV;
    if (op == EEQU)
        node = makemonadicnode(node, ELOGNOT);
    return node;
}

ENode *CExpr_New_EGREATEREQU_Node(ENode *left, ENode *right)
{
    Boolean value;
    BinaryOperatorResult fold;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, EGREATEREQU, right);

    left = NormalizeOperand(left);
    right = NormalizeOperand(right);

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x16b, (ENode *)(left), (ENode *)(right), &fold)) {
            if (fold.expression != NULL)
                return fold.expression;
            CError_ASSERT(4584, (left = fold.left));
            CError_ASSERT(4585, (right = fold.right));
        }
    }

    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(EGREATEREQU, left, right);
        return;
    }

    unify_arithmetic_rtypes(&left, &right, 0);

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 0x16b, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        ENode *floatLeft = (ENode *)left;
        ENode *floatRight = (ENode *)right;
        value = CMach_CalcFloatDiadicBool(left->rtype, floatLeft->data.floatval.data.value, 0x16b,
                                          floatRight->data.floatval.data.value);
        recovered_set_integer(left, value);
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = simplify_unsigned_zero_comparison(makediadicnode(left, right, EGREATEREQU), 0, 1);
    }

    return left;
}

ENode *CExpr_New_EGREATER_Node(ENode *left, ENode *right)
{
    Boolean comparison;
    BinaryOperatorResult fold;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, EGREATER, right);

    left = NormalizeOperand(left);
    right = NormalizeOperand(right);

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(62, (ENode *)(left), (ENode *)(right), &fold)) {
            if (fold.expression != NULL)
                return fold.expression;
            CError_ASSERT(4540, (left = fold.left));
            CError_ASSERT(4541, (right = fold.right));
        }
    }

    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(EGREATER, left, right);
        return;
    }

    unify_arithmetic_rtypes(&left, &right, 0);

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 62, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        ENode *floatLeft = (ENode *)left;
        ENode *floatRight = (ENode *)right;
        comparison = CMach_CalcFloatDiadicBool(left->rtype, floatLeft->data.floatval.data.value, 62,
                                               floatRight->data.floatval.data.value);
        recovered_set_integer(left, comparison);
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = simplify_unsigned_zero_comparison(makediadicnode(left, right, EGREATER), 0, 0);
    }

    return left;
}

ENode *CExpr_New_ELESSEQU_Node(ENode *left, ENode *right)
{
    Boolean result;
    BinaryOperatorResult fold;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, ELESSEQU, right);
    left = NormalizeOperand(left);
    right = NormalizeOperand(right);
    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(362, (ENode *)(left), (ENode *)(right), &fold)) {
            if (fold.expression != NULL)
                return fold.expression;
            CError_ASSERT(4496, (left = fold.left));
            CError_ASSERT(4497, (right = fold.right));
        }
    }
    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(ELESSEQU, left, right);
        return;
    }
    unify_arithmetic_rtypes(&left, &right, 0);
    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 362, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        ENode *floatLeft = (ENode *)left;
        ENode *floatRight = (ENode *)right;
        result = CMach_CalcFloatDiadicBool(left->rtype, floatLeft->data.floatval.data.value, 362,
                                           floatRight->data.floatval.data.value);
        recovered_set_integer(left, result);
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = simplify_unsigned_zero_comparison(makediadicnode(left, right, ELESSEQU), 1, 1);
    }
    return left;
}

ENode *CExpr_New_ELESS_Node(ENode *left, ENode *right)
{
    Boolean comparison;
    BinaryOperatorResult fold;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, ELESS, right);

    left = NormalizeOperand(left);
    right = NormalizeOperand(right);

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(60, (ENode *)(left), (ENode *)(right), &fold)) {
            if (fold.expression != NULL)
                return fold.expression;
            CError_ASSERT(4452, (left = fold.left));
            CError_ASSERT(4453, (right = fold.right));
        }
    }

    if ((SInt8)left->rtype->type >= TYPEPOINTER || (SInt8)right->rtype->type >= TYPEPOINTER) {
        make_pointer_comparison(ELESS, left, right);
        return;
    }

    unify_arithmetic_rtypes(&left, &right, 0);

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 60, right->data.intval);
        left->rtype = CParser_GetBoolType();
    } else if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        ENode *floatLeft = (ENode *)left;
        ENode *floatRight = (ENode *)right;
        comparison = CMach_CalcFloatDiadicBool(left->rtype, floatLeft->data.floatval.data.value, 60,
                                               floatRight->data.floatval.data.value);
        recovered_set_integer(left, comparison);
        left->type = EINTCONST;
        left->rtype = CParser_GetBoolType();
    } else {
        left = simplify_unsigned_zero_comparison(makediadicnode(left, right, ELESS), 1, 0);
    }

    return left;
}

ENode *simplify_unsigned_zero_comparison(ENode *node, Boolean lessThan, Boolean inclusive)
{
    ENode *constant;
    Type *operandType;

    operandType = node->data.diadic.left->rtype;
    if (Type_IsUnsigned(operandType)) {
        if (node->data.diadic.left->type == EINTCONST && CExpr_IsZeroValue(&node->data.diadic.left->data.intval)) {
            lessThan = !lessThan;
        } else if (node->data.diadic.right->type != EINTCONST ||
                   !CExpr_IsZeroValue(&node->data.diadic.right->data.intval)) {
            set_comparison_result_type(node);
            return node;
        }

        if (lessThan && inclusive) {
            node->type = EEQU;
            set_comparison_result_type(node);
            return node;
        }
        if (!lessThan && !inclusive) {
            node->type = ENOTEQU;
            set_comparison_result_type(node);
            return node;
        }
        set_comparison_result_type(node);

        if (node->rtype->type == TYPEFLOAT) {
            constant = intconstnode((Type *)&stsignedint, !lessThan);
            constant->type = EFLOATCONST;
            constant->data.floatval = CMach_CalcFloatConvertFromInt((Type *)&stsignedint, constant->data.intval);
            constant->rtype = node->rtype;
        } else {
            constant = intconstnode(node->rtype, !lessThan);
        }
        return CInline_00513910(node) ? makediadicnode(node, constant, ECOMMA) : constant;
    }
    set_comparison_result_type(node);
    return node;
}

void make_pointer_comparison(UInt8 op, ENode *left, ENode *right)
{
    Type *leftTarget;
    Type *rightType;
    Type *rightTarget;
    Type *leftType;
    UInt8 rightKind;
    ENode *converted;

    leftTarget = leftType = left->rtype;
    rightTarget = rightType = right->rtype;

    if (leftType->type == TYPEPOINTER && (rightKind = rightTarget->type) == TYPEPOINTER) {
        if (leftTarget->type == TYPEPOINTER && rightKind == TYPEPOINTER) {
            leftTarget = TPTR_TARGET(leftTarget);
            rightTarget = TPTR_TARGET(rightTarget);
        }
        if (leftTarget->type == TYPECLASS && rightTarget->type == TYPECLASS) {
            if (leftTarget != rightTarget) {
                if (leftTarget == TPTR_TARGET(leftType)) {
                    CClass_Init();
                    if (CClass_FindBasePath((TypeClass *)leftTarget, (TypeClass *)rightTarget, 0, 1)) {
                        CClass_Init();
                        converted =
                            CClass_ConvertClassPointer(left, TYPE_CLASS(leftTarget), TYPE_CLASS(rightTarget), 0, 1);
                        if (converted != left && !(converted->type == ETYPCON && converted->data.diadic.left == left) &&
                            !isnotzero(left))
                            converted = CExpr_WrapPrecomp(left, converted);
                        left = converted;
                    } else if (CClass_FindBasePath((TypeClass *)rightTarget, (TypeClass *)leftTarget, 0, 1)) {
                        CClass_Init();
                        converted =
                            CClass_ConvertClassPointer(right, TYPE_CLASS(rightTarget), TYPE_CLASS(leftTarget), 0, 1);
                        if (converted != right &&
                            !(converted->type == ETYPCON && converted->data.diadic.left == right) && !isnotzero(right))
                            converted = CExpr_WrapPrecomp(right, converted);
                        right = converted;
                    } else {
                        CError_ReportError(ERR_TYPE_MISMATCH, left->rtype, left->flags & 3, right->rtype,
                                           right->flags & 3);
                    }
                } else {
                    CError_ReportError(ERR_TYPE_MISMATCH, leftType, left->flags & 3, rightType, right->flags & 3);
                }
            }
        } else if (!is_typesame(leftType, rightType)) {
            if (!copts.objective_c || !CObjC_IsIdCompatiblePointerPair(left->rtype, right->rtype))
                CError_ReportError(ERR_TYPE_MISMATCH, left->rtype, left->flags & 3, right->rtype, right->flags & 3);
        }
    } else if ((UInt8)(op - EEQU) <= ENOTEQU - EEQU) {
        if (leftTarget->type == TYPEINT) {
            if (left->type == EINTCONST) {
                Boolean isNull = left->data.intval.hi == 0 && left->data.intval.lo == 0;
                if (!isNull)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
            } else {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            }
            left->rtype = (Type *)&stunsignedlong;
        } else if (rightTarget->type == TYPEINT) {
            if (right->type == EINTCONST) {
                Boolean isNull = right->data.intval.hi == 0 && right->data.intval.lo == 0;
                if (!isNull)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
            } else {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            }
            right->rtype = (Type *)&stunsignedlong;
        } else if (!is_typesame(leftTarget, rightTarget)) {
            CError_ReportError(ERR_TYPE_MISMATCH, left->rtype, left->flags & 3, right->rtype, right->flags & 3);
        }
    } else if (!is_typesame(leftTarget, rightTarget)) {
        CError_ReportError(ERR_TYPE_MISMATCH, left->rtype, left->flags & 3, right->rtype, right->flags & 3);
    }

    {
        ENode *result = makediadicnode(left, right, op);
        if (copts.cplusplus && copts.booltruefalse)
            result->rtype = (Type *)&stbool;
        else
            result->rtype = (Type *)&stsignedint;
    }
}

ENode *CExpr_New_ESHR_Node(ENode *left, ENode *right)
{
    ENode *leftOperand;
    ENode *rightOperand;
    ENode *normalizedLeft;
    ENode *normalizedRight;
    BinaryOperatorResult result;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, ESHR, right);

    normalizedLeft = (ENode *)CExpr_RewriteConst(pointer_generation(left));
    left = normalizedLeft;
    normalizedRight = (ENode *)CExpr_RewriteConst(pointer_generation(right));
    right = normalizedRight;

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x16d, (ENode *)(normalizedLeft), (ENode *)(normalizedRight),
                                (BinaryOperatorResult *)(&result)) != 0) {
            if (result.expression != NULL)
                return result.expression;
            CError_ASSERT(4295, (left = result.left) != 0);
            CError_ASSERT(4296, (right = result.right) != 0);
        }
    }

    leftOperand = forceintegral(left);
    rightOperand = forceintegral(right);

    if (CExpr2_IsZero(leftOperand) != 0 || CExpr2_IsZero(rightOperand) != 0)
        return leftOperand;

    if (leftOperand->type == EINTCONST && rightOperand->type == EINTCONST) {
        leftOperand->data.intval =
            CMach_CalcIntDiadic(leftOperand->rtype, leftOperand->data.intval, 0x16d, rightOperand->data.intval);
        return leftOperand;
    }
    return makediadicnode(leftOperand, rightOperand, ESHR);
}

ENode *CExpr_New_ESHL_Node(ENode *left, ENode *right)
{
    BinaryOperatorResult overloadResult;
    ENode *leftOperand;
    ENode *rightOperand;
    ENode *convertedLeft;
    ENode *convertedRight;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR) {
        return CTempl_MakeTemplDepExpr(left, ESHL, right);
    }

    convertedLeft = CExpr_RewriteConst(pointer_generation(left));
    left = convertedLeft;
    convertedRight = CExpr_RewriteConst(pointer_generation(right));
    right = convertedRight;

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x16c, (ENode *)(left), (ENode *)(right), &overloadResult)) {
            if (overloadResult.expression != NULL) {
                return overloadResult.expression;
            }
            CError_ASSERT(4262, left = overloadResult.left);
            CError_ASSERT(4263, right = overloadResult.right);
        }
    }

    leftOperand = forceintegral(left);
    rightOperand = forceintegral(right);

    if (CExpr2_IsZero(leftOperand) != 0 || CExpr2_IsZero(rightOperand) != 0)
        return leftOperand;

    if (leftOperand->type == EINTCONST && rightOperand->type == EINTCONST) {
        leftOperand->data.intval =
            CMach_CalcIntDiadic(leftOperand->rtype, leftOperand->data.intval, 0x16c, rightOperand->data.intval);
        return leftOperand;
    }
    return makediadicnode(leftOperand, rightOperand, ESHL);
}

ENode *CExpr_New_EMODULO_Node(ENode *left, ENode *right, Boolean suppressWarning)
{
    BinaryOperatorResult overloadResult;

    if (left->rtype->type == TYPETEMPLDEPEXPR || right->rtype->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(left, EMODULO, right);

    left = rewrite_const_pointer_generation(left);
    right = rewrite_const_pointer_generation(right);

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator('%', (ENode *)(left), (ENode *)(right), &overloadResult)) {
            if (overloadResult.expression != NULL)
                return overloadResult.expression;
            left = overloadResult.left;
            CError_ASSERT(4165, left != NULL);
            right = overloadResult.right;
            CError_ASSERT(4166, right != NULL);
        }
    }

    left = forceintegral(left);
    right = forceintegral(right);
    CExpr_ArithmeticConversion(&left, &right);

    if (CExpr2_IsZero(right) != 0) {
        if (suppressWarning == 0)
            CError_Warning(ERR_DIVISION_BY_0);
        return left;
    }

    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, '%', right->data.intval);
        return left;
    }

    if (CExpr2_IsZero(left) != 0)
        return makediadicnode(right, left, ECOMMA);

    if (CExpr_IsOne(right) != 0) {
        right = nullnode();
        right->rtype = left->rtype;
        return makediadicnode(left, right, ECOMMA);
    }

    return makediadicnode(left, right, EMODULO);
}

ENode *member_pointer_expression(void)
{
    ENode *right, *left;
    Type *memberType;
    UInt16 flags;
    ENode *wrapped, *resultNode, *expr;
    BinaryOperatorResult result;
    CInt64 nullMemberOffset;
    ENode *arrowLeft;
    Type *type;
    expr = cast_expression();
    for (;;) {
        switch (tk) {
            case TK_ARROW_STAR: {
                ENode *operand;
                arrowLeft = CExpr_RewriteConst(pointer_generation(expr));
                tk = CPrepTokenizer_GetNextToken();
                operand = CExpr_RewriteConst(pointer_generation(cast_expression()));
                right = operand;
                if (CExpr_CheckOperator(371, arrowLeft, operand, &result) != 0) {
                    expr = result.expression;
                    if (expr == NULL)
                        CError_FATAL(4010);
                    continue;
                }
                if ((SInt8)arrowLeft->rtype->type < 11) {
                    CError_ReportError(ERR_POINTER_ARRAY_REQUIRED);
                    return arrowLeft;
                }
                expr = makemonadicnode(arrowLeft, 4);
                expr->rtype = TYPE_POINTER(expr->rtype)->target;
                break;
            }
            case TK_DOT_STAR:
                left = CExpr_RewriteConst(pointer_generation(expr));
                expr = left;
                tk = CPrepTokenizer_GetNextToken();
                right = CExpr_RewriteConst(pointer_generation(cast_expression()));
                if (left->type != EINDIRECT) {
                    CError_ReportError(ERR_NOT_LVALUE);
                    return left;
                }
                break;
            default:
                goto done;
        }
        if (expr->rtype->type != TYPECLASS) {
            CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS);
            return expr;
        }
        if (right->rtype->type != TYPEMEMBERPOINTER) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return expr;
        }
        if (expr->rtype != TYPE_MEMBER_POINTER(right->rtype)->ty2) {
            CClass_Init();
            if (CClass_FindBasePath(TYPE_CLASS(expr->rtype), TYPE_CLASS(TYPE_MEMBER_POINTER(right->rtype)->ty2), 1,
                                    1) != 0) {
                type = right->rtype;
                expr->data.diadic.left = CClass_ConvertClassPointer(expr->data.diadic.left, TYPE_CLASS(expr->rtype),
                                                                    TYPE_CLASS(TYPE_MEMBER_POINTER(type)->ty2), 0, 1);
                expr->rtype = TYPE_MEMBER_POINTER(right->rtype)->ty2;
            } else {
                CError_ReportError(ERR_ILLEGAL_TYPE);
                return expr;
            }
        }
        memberType = TYPE_MEMBER_POINTER(right->rtype)->ty1;
        flags = expr->flags;
        if (memberType->type == TYPEFUNC)
            break;
        do {
            if (right->type != EINTCONST) {
                nullMemberOffset.lo = -1;
                nullMemberOffset.hi = -1;
                if (add_to_expression_constant(expr->data.diadic.left, nullMemberOffset) == 0) {
                    expr->data.diadic.left = makediadicnode(expr->data.diadic.left, nullnode(), 15);
                    SetInt64(&expr->data.diadic.left->data.diadic.right->data.intval, -1);
                    optimizecomm(expr->data.diadic.left);
                }
            } else {
                right->data.intval = CInt64_Sub(right->data.intval, cint64_one);
                if (add_to_expression_constant(expr->data.diadic.left, right->data.intval) != 0)
                    break;
            }
            right->rtype = (Type *)&stsignedlong;
            expr->data.diadic.left = makediadicnode(expr->data.diadic.left, right, 15);
            optimizecomm(expr->data.diadic.left);
        } while (0);
        if (memberType->type == TYPEBITFIELD) {
            expr->data.diadic.left = makemonadicnode(expr->data.diadic.left, 49);
            expr->data.diadic.left->rtype = memberType;
            expr->rtype = TYPE_POINTER(memberType)->target;
        } else
            expr->rtype = memberType;
        expr->flags = flags;
        if ((SInt8)expr->rtype->type != TYPEPOINTER || !(TYPE_POINTER(expr->rtype)->qual & Q_REFERENCE)) {
            (void)expr;
            continue;
        }
        wrapped = makemonadicnode(expr, 4);
        wrapped->rtype = TYPE_POINTER(wrapped->rtype)->target;
        expr = wrapped;
    }
    CE_ASSERT(right->type != EINDIRECT, CError_FATAL(4082));
    resultNode = CompilerTools_AllocatePool(26);
    resultNode->type = EQUALNAME;
    resultNode->cost = 4;
    resultNode->flags = 0;
    resultNode->rtype = &stvoid;
    resultNode->data.diadic.left = expr;
    resultNode->data.diadic.right = right;
    return resultNode;
done:
    return expr;
}

ENode *cast_expression(void)
{
    ENode *right;
    ENodeList *link;
    SInt32 structKind;
    Boolean success;
    DeclInfo typeInfo;
    ENodeUnion value;
    BinaryOperatorResult result;
    ENode *expr;
    ENode *node;

    if (tk != '(' || !islookaheaddeclaration())
        return unary_expression();

    tk = CPrepTokenizer_GetNextToken();
    memclrw(&typeInfo, sizeof(typeInfo));
    CParser_GetDeclSpecs(&typeInfo, 0);
    CDecl_ParseDeclarator(&typeInfo);
    if (tk != ')')
        CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
    else
        tk = CPrepTokenizer_GetNextToken();
    if (typeInfo.name != NULL)
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);

    if (copts.altivec_model != 0 && tk == '(' && typeInfo.thetype->type == TYPESTRUCT &&
        (structKind = TYPE_STRUCT(typeInfo.thetype)->stype) >= 4 && structKind <= 0xe) {
        tk = CPrepTokenizer_GetNextToken();
        expr = assignment_expression();
        while (tk == ',') {
            expr = CExpr_RewriteConst(pointer_generation(expr));
            tk = CPrepTokenizer_GetNextToken();
            right = CExpr_RewriteConst(pointer_generation(assignment_expression()));
            if (copts.cplusplus != 0 && (success = CExpr_CheckOperator(',', expr, right, &result)) != 0) {
                expr = result.expression;
                CError_ASSERT(6531, expr != 0);
            } else {
                CExpr_CheckUnusedExpression(expr);
                expr = makecommaexpression(expr, right);
                expr->rtype = right->rtype;
            }
        }
        if (tk != ')')
            CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        success = IrOptimizer_ConvertToVectorConstant(expr, &value.vector128val, TYPE_STRUCT(typeInfo.thetype));
        if (success != 0) {
            node = CompilerTools_AllocatePool(sizeof(ENode));
            node->type = EASSBLK;
            node->cost = expr->cost;
            if (node->cost == 0)
                node->cost = 1;
            node->flags = expr->flags & ENODE_FLAG_QUALS;
            node->rtype = typeInfo.thetype;
            node->data = value;
        } else {
            node = makemonadicnode(expr, ETYPCON);
        }
        node->rtype = typeInfo.thetype;
        node->flags = expr->flags;
        return node;
    }

    if (copts.ANSIstrict == 0 && tk == '{' &&
        (typeInfo.thetype->type != TYPESTRUCT || (structKind = TYPE_STRUCT(typeInfo.thetype)->stype) < 4 ||
         structKind > 0xe)) {
        return CInit_AutoObject(NULL, typeInfo.thetype, typeInfo.qual);
    }

    expr = cast_expression();
    if (copts.cplusplus != 0 && (CTemplateTools_IsDependentType(typeInfo.thetype) || CTemplTool_IsTypeDepExpr(expr))) {
        link = CompilerTools_AllocatePool(sizeof(*link));
        link->next = NULL;
        link->node = expr;
        node = recovery_construct_4059(link, typeInfo.thetype, typeInfo.qual);
        return node;
    }
    if (typeInfo.thetype->type != TYPEPOINTER || ((TYPE_POINTER(typeInfo.thetype)->qual & Q_REFERENCE) == 0)) {
        expr = CExpr_RewriteConst(pointer_generation(expr));
    }
    return do_typecast(expr, typeInfo.thetype, typeInfo.qual);
}

ENode *do_typecast(ENode *expr, Type *type, UInt32 qual)
{
    ENode *result;
    Object *object;
    UInt32 maskedQual;
    UInt16 nodeQual;

    if (copts.cpp_extensions && iscpp_typeequal(expr->rtype, type) && expr->type != ENEWEXCEPTION) {
        expr->rtype = type;
        expr->flags &= ~ENODE_FLAG_QUALS;
        expr->flags |= qual & ENODE_FLAG_QUALS;
        return expr;
    }

    switch ((SInt8)type->type) {
        case TYPEARRAY:
            CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & ENODE_FLAG_QUALS, type,
                               qual);
            return expr;
        case TYPEMEMBERPOINTER: {
            ENode *memberExpr;
            if (expr->rtype->type != TYPECLASS) {
                memberExpr = expr;
                if (expr->rtype->type != TYPEMEMBERPOINTER) {
                    if (expr->type == EINTCONST && IsZero_4f5aa0(expr)) {
                        if (TYPE_MEMBER_POINTER(type)->ty1->type == TYPEFUNC)
                            memberExpr = create_objectnode(data_00587678);
                        memberExpr->rtype = type;
                    } else if (expr->type == ENEWEXCEPTIONARRAY)
                        memberExpr = getpointertomemberfunc(expr, type, 1);
                    else
                        memberExpr = expr;
                }
                if (memberExpr->rtype->type != TYPEMEMBERPOINTER) {
                    CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
                    result = nullnode();
                } else {
                    result = CExpr_CastMemberPointer(memberExpr, TYPE_MEMBER_POINTER(memberExpr->rtype),
                                                     TYPE_MEMBER_POINTER(type));
                    result->flags = qual & ENODE_FLAG_QUALS;
                }
                return result;
            }
            break;
        }
    }

    nodeQual = maskedQual = qual & ENODE_FLAG_QUALS;
    if (expr->type == ENEWEXCEPTION)
        return ((ENode * (*)(ENode *, Type *, int, int)) oldassignmentpromotion)(expr, type, maskedQual, 1);
    if (expr->type == EOBJREF) {
        TypeMemberFunc *func;
        if ((func = (TypeMemberFunc *)(object = expr->data.addr.objref)->type)->type == TYPEFUNC &&
            (func->flags & FUNC_METHOD) && func->is_static == 0) {
            CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
            return nullnode();
        }
    }
    if (type == &stvoid) {
        expr = makemonadicnode(expr, ETYPCON);
        expr->rtype = type;
        expr->flags = nodeQual;
        return expr;
    }
    if (type->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE)) {
        TypePointer *targetPointer = TYPE_POINTER(type);
        ENode *operand = getnodeaddress(expr, 0);
        Type *pointerType = galloc(sizeof(TypePointer));
        *TYPE_POINTER(pointerType) = *targetPointer;
        TYPE_POINTER(pointerType)->qual &= ~Q_REFERENCE;
        result = do_typecast(operand, pointerType, qual);
        result = makemonadicnode(result, EINDIRECT);
        result->rtype = targetPointer->target;
        result->flags = nodeQual;
        return result;
    }
    if (expr->rtype->type == TYPECLASS || type->type == TYPECLASS) {
        if (expr->rtype->size == 0)
            CDecl_CompleteType(expr->rtype);
        if (expr->rtype == type && CClass_CopyConstructor(TYPE_CLASS(type)) == NULL)
            return expr;
        if (user_assign_check(expr, type, qual, 1, 1, 1)) {
            converted_expr->flags = nodeQual;
            return converted_expr;
        }
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & ENODE_FLAG_QUALS, type,
                           qual);
        return nullnode();
    }
    if (type->type == TYPESTRUCT && copts.cplusplus && iscpp_typeequal(expr->rtype, type)) {
        expr->rtype = type;
        expr->flags = nodeQual;
        return expr;
    }
    if (type == STBOOL) {
        result = CExpr_ConvertToBool(expr, 1);
        result->flags = nodeQual;
        return result;
    }
    if (type->type == TYPEENUM) {
        result = do_typecast(expr, TYPE_ENUM(type)->enumtype, qual);
        if (result->type != EINTCONST)
            result = makemonadicnode(result, ETYPCON);
        result->rtype = type;
        result->flags = nodeQual;
        return result;
    }
    if ((UInt8)(type->type - TYPEINT) <= 1) {
        if (expr->type == ETYPCON && expr->rtype->type == type->type && expr->rtype->size == type->size &&
            Type_IsUnsigned(expr->rtype) == Type_IsUnsigned(type) && (expr->flags & ENODE_FLAG_QUALS) == qual) {
            expr->rtype = type;
            expr->flags |= ENODE_FLAG_80;
            return expr;
        }
        if (expr->rtype->type == TYPEENUM)
            expr = CExpr_ConvertToIntegral(expr);
        if (expr->rtype->type == TYPEINT || expr->rtype->type == TYPEFLOAT) {
            result = CExpr2_00473720(expr, type);
            result->flags = nodeQual;
            return result;
        }
        if (expr->rtype->type != TYPEPOINTER || type->type == TYPEFLOAT)
            CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & ENODE_FLAG_QUALS, type,
                               qual);
        if (expr->type == ETYPCON && expr->data.monadic->type == EINTCONST) {
            ENode *constant = expr->data.monadic;
            constant->rtype = type;
            constant->flags = nodeQual;
            constant->data.intval = CExpr_IntConstConvert(type, STUNSIG, constant->data.intval);
            return constant;
        }
        if (type->size != stunsignedlong.size) {
            expr = makemonadicnode(expr, ETYPCON);
            expr->rtype = STUNSIG;
        }
        result = makemonadicnode(expr, ETYPCON);
        result->rtype = type;
        result->flags = nodeQual;
        return result;
    }

    if ((SInt8)type->type >= TYPEPOINTER) {
        if ((SInt8)expr->rtype->type >= TYPEPOINTER) {
            TypeClass *to;
            TypeClass *from;
            if ((from = TYPE_CLASS(TYPE_POINTER(expr->rtype)->target))->type == TYPECLASS &&
                (to = TYPE_CLASS(TYPE_POINTER(type)->target))->type == TYPECLASS) {
                CClass_Init();
                result = CClass_ConvertClassPointer(expr, from, to, 1, 1);
                if (result != expr) {
                    if (result->type != ETYPCON || result->data.monadic != expr)
                        result = (ENode *)PreserveEvaluation(expr, &result);
                }
                expr = result;
            }
            if (expr->type != ETYPCON)
                expr = makemonadicnode(expr, ETYPCON);
            expr->rtype = type;
            expr->flags = nodeQual;
            return expr;
        }

        if (expr->rtype->type != TYPEINT) {
            if (expr->rtype->type != TYPEENUM) {
                CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & ENODE_FLAG_QUALS,
                                   type, qual);
                return expr;
            }
            expr = CExpr_ConvertToIntegral(expr);
        }
        if (expr->rtype->size != 4) {
            if (expr->type != EINTCONST)
                expr = makemonadicnode(expr, ETYPCON);
            expr->rtype = STUNSIG;
        }
        result = makemonadicnode(expr, ETYPCON);
        result->rtype = type;
        result->flags = nodeQual;
        return result;
    }

    {
        ENode *converted = CodeGen_MakeAltivecStructCast(expr, type, qual);
        if (converted)
            return converted;
    }
    CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & ENODE_FLAG_QUALS, type, qual);
    return nullnode();
}

ENode *CExpr_MemberPointerConversion(ENode *enode, Type *type, Boolean flag)
{
    if (enode->type == EINTCONST) {
        Boolean zero = (enode->data.intval.hi == 0 && enode->data.intval.lo == 0);
        if (zero) {
            if (TYPE_POINTER(type)->target->type == TYPEFUNC)
                enode = create_objectnode(data_00587678);
            enode->rtype = type;
            return enode;
        }
    }
    if (enode->type == ENEWEXCEPTIONARRAY) {
        return getpointertomemberfunc(enode, type, flag);
    }
    return enode;
}

ENode *CExpr_CastMemberPointer(ENode *value, TypeMemberPointer *sourceType, TypeMemberPointer *targetType)
{
    int offset;
    UInt32 sourceSize;
    ENode *castValue;
    ENode *expression;
    BClassList *path;
    char reversed;
    SInt16 pathFlags;
    Boolean ambiguous;
    CInt64 words;
    Type *sourceClass;
    Type *targetClass;
    CE_ASSERT(sourceType->ty2->type != TYPECLASS, CError_FATAL(3532));
    CE_ASSERT(targetType->ty2->type != TYPECLASS, CError_FATAL(3533));
    sourceClass = sourceType->ty2;
    targetClass = targetType->ty2;
    if (sourceClass == targetClass) {
        value->rtype = (Type *)targetType;
        return value;
    }
    do {
        reversed = 0;
        path = CClass_GetBasePath(TYPE_CLASS(targetType->ty2), TYPE_CLASS(sourceType->ty2), &pathFlags, &ambiguous);
        if (path != NULL || ((path = CClass_GetBasePath(TYPE_CLASS(sourceType->ty2), TYPE_CLASS(targetType->ty2),
                                                        &pathFlags, &ambiguous)) != NULL &&
                             (reversed = 1, 1))) {
            if (ambiguous != 0) {
                CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
            }
            offset = CClass_GetPathOffset(path);
            if (offset >= 0) {
                if (reversed != 0) {
                    offset = -offset;
                }
                CClass_CheckBaseAccess(path, ACCESSPUBLIC);
                sourceSize = sourceType->size;
                if (sourceSize == targetType->size) {
                    break;
                }
            }
        }
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, sourceType, 0, targetType, 0);
        return nullnode();
    } while (0);
    if (offset == 0) {
        value->rtype = (Type *)targetType;
        return value;
    }
    sourceSize = sourceType->size;
    if (sourceSize == 4) {
        if (value->type == EINTCONST) {
            words.lo = offset;
            words.hi = (offset < 0) ? -1 : 0;
            value->data.intval = CInt64_Add(value->data.intval, words);
            value->rtype = (Type *)targetType;
            return value;
        }
        value->rtype = (Type *)&stsignedlong;
        value = makemonadicnode(makediadicnode(value, intconstnode((Type *)&stsignedlong, offset), EADD), ETYPCON);
        value->rtype = (Type *)targetType;
        return value;
    }
    expression = funccallexpr(cast_member_pointer_func, intconstnode((Type *)&stsignedlong, offset),
                              getnodeaddress(value, 0), create_temp_node(&data_0058847c), NULL);
    castValue = makemonadicnode(expression, EINDIRECT);
    castValue->rtype = (Type *)targetType;
    return castValue;
}

ENode *CExpr_New_EPRECOMP_Node(ENode *node, ENode *label)
{
    ENode *result;
    if (isnotzero(label) != 0) {
        return node;
    }
    result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *result = *node;
    result->type = EMFPOINTER;
    result->data.diadic.left = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *result->data.diadic.left = *label;
    result->data.diadic.right = node;
    ((ENode *)result)->data.precomp.labelId = CParser_GetUniqueID();
    label->type = ENULLCHECK;
    ((ENode *)label)->data.longval = ((ENode *)result)->data.precomp.labelId;
    return result;
}

#pragma opt_propagation off

ENode *unary_expression(void)
{
    BinaryOperatorResult overloadResult;
    BinaryOperatorResult complementResult;

    switch (tk) {
        case TK_COLON_COLON:
            switch (CPrepTokenizer_GetNextTokenAndRestorePosition()) {
                case 0x147:
                    tk = CPrepTokenizer_GetNextToken();
                    return scannew(1);
                case 0x145:
                    tk = CPrepTokenizer_GetNextToken();
                    return scandelete(1);
                default:
                    return parse_postfix_expression(0);
            }

        case TK_NEW:
            return scannew(0);

        case TK_DELETE:
            return scandelete(0);

        case TK_INCREMENT: {
            ENode *operand;
            ENode *converted;
            tk = CPrepTokenizer_GetNextToken();
            operand = pointer_generation_then_rewrite_const(unary_expression());
            if (copts.cplusplus != 0) {
                if (CExpr_CheckOperator(0x16e, operand, NULL, &overloadResult)) {
                    CError_ASSERT(3321, overloadResult.expression != 0);
                    return overloadResult.expression;
                }
            }
            converted = CExpr_LValue(operand, 1, 1);
            if (converted->rtype == (Type *)&stbool) {
                operand = nullnode();
                operand->rtype = (Type *)&stbool;
                operand->data.temp.uniqueid = 1;
                operand->data.temp.type = NULL;
                return makediadicnode(converted, operand, EASS);
            } else {
                CExpr_004fb400(converted);
                return makemonadicnode(converted, EPREINC);
            }
        }

        case TK_DECREMENT: {
            ENode *operand;
            tk = CPrepTokenizer_GetNextToken();
            operand = pointer_generation_then_rewrite_const(unary_expression());
            if (copts.cplusplus != 0) {
                if (CExpr_CheckOperator(0x16f, operand, NULL, &overloadResult)) {
                    CError_ASSERT(3340, overloadResult.expression != 0);
                    return overloadResult.expression;
                }
            }
            operand = CExpr_LValue(operand, 1, 1);
            CExpr_004fb400(operand);
            return makemonadicnode(operand, EPREDEC);
        }

        case TK_BITAND: {
            ENode *operand;
            if (copts.cplusplus != 0) {
                tk = CPrepTokenizer_GetNextToken();
                switch (tk) {
                    case TK_IDENTIFIER:
                    case 0x151:
                    case TK_COLON_COLON:
                        operand = parse_postfix_expression(1);
                        if (operand->type == ENEWEXCEPTIONARRAY)
                            return make_memberpointer(operand);
                        break;
                    default:
                        operand = cast_expression();
                        break;
                }
                if (CExpr_CheckOperator(0x26, operand, NULL, &overloadResult)) {
                    CError_ASSERT(3366, overloadResult.expression != 0);
                    return overloadResult.expression;
                }
            } else {
                tk = CPrepTokenizer_GetNextToken();
                operand = cast_expression();
            }
            if (copts.mpwc_relax != 0 && copts.cplusplus == 0 && operand->rtype->type == TYPEARRAY &&
                operand->type == EINDIRECT) {
                return CExpr_RewriteConst(pointer_generation(operand));
            }
            if (operand->type == ENEWEXCEPTION)
                return operand;
            return getnodeaddress(operand, 1);
        }

        case '*': {
            ENode *operand;
            ENode *normalized;
            ENode *result;
            tk = CPrepTokenizer_GetNextToken();
            operand = normalized = pointer_generation_then_rewrite_const(cast_expression());
            if (copts.cplusplus != 0) {
                if (CExpr_CheckOperator(0x2a, normalized, NULL, &overloadResult)) {
                    result = overloadResult.expression;
                    if (result != NULL)
                        return result;
                    operand = overloadResult.left;
                    CError_ASSERT(3390, operand != 0);
                }
            }
            if ((SInt8)operand->rtype->type < TYPEPOINTER) {
                CError_ReportError(ERR_POINTER_ARRAY_REQUIRED);
                return operand;
            }
            operand = makemonadicnode(operand, EINDIRECT);
            {
                ENode *completed = operand;
                completed->rtype = TPTR_TARGET(completed->rtype), CDecl_CompleteType(completed->rtype);
                return completed;
            }
        }

        case '+': {
            ENode *operand;
            ENode *normalized;
            ENode *result;
            tk = CPrepTokenizer_GetNextToken();
            operand = normalized = pointer_generation_then_rewrite_const(cast_expression());
            if (copts.cplusplus != 0) {
                if (CExpr_CheckOperator(0x2b, normalized, NULL, &overloadResult)) {
                    result = overloadResult.expression;
                    if (result != NULL)
                        return result;
                    operand = overloadResult.left;
                    CError_ASSERT(3402, operand != 0);
                }
            }
            switch ((SInt8)operand->rtype->type) {
                case TYPEINT:
                case TYPEENUM:
                    return forceintegral(operand);
                case TYPEFLOAT:
                case TYPEPOINTER:
                case TYPEARRAY:
                    return operand;
                default:
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                    return operand;
            }
        }

        case '-':
            tk = CPrepTokenizer_GetNextToken();
            return CExpr_New_EMONMIN_Node(cast_expression());

        case TK_COMPL: {
            ENode *operand;
            ENode *normalized;
            ENode *promoted;
            ENode *result;
            ENode *value;
            tk = CPrepTokenizer_GetNextToken();
            operand = cast_expression();
            if (operand->rtype->type == TYPETEMPLDEPEXPR) {
                value = CTempl_MakeTemplDepExpr(NULL, EBINNOT, operand);
            } else {
                do {
                    operand = normalized = pointer_generation_then_rewrite_const(operand);
                    if (copts.cplusplus != 0) {
                        if (CExpr_CheckOperator(0x7e, normalized, NULL, &complementResult)) {
                            result = complementResult.expression;
                            if (result != NULL) {
                                value = result;
                                break;
                            }
                            operand = complementResult.left;
                            CError_ASSERT(3283, operand != 0);
                        }
                    }
                    promoted = forceintegral(operand);
                    if (promoted->type == EINTCONST) {
                        promoted->data.intval = CMach_CalcIntMonadic(promoted->rtype, 0x7e, promoted->data.intval);
                        value = promoted;
                    } else {
                        value = makemonadicnode(promoted, EBINNOT);
                    }
                } while (0);
            }
            return value;
        }

        case TK_NOT:
            tk = CPrepTokenizer_GetNextToken();
            return CExpr_New_ELOGNOT_Node(cast_expression());

        case TK_SIZEOF: {
            Type *type;
            ENode *result;
            type = scan_type_or_expression_type();
            if (type->type == TYPECLASS && TYPE_CLASS(type)->sominfo != NULL)
                CError_ReportError(ERR_SIZEOF_NOT_SUPPORTED_SOM_CLASSES);
            if (type->type == TYPETEMPLATE) {
                result = CExpr_NewTemplDepENode(1);
                result->data.objref = (Object *)type;
            } else {
                if (type->size == 0) {
                    UInt8 kindOffset = (UInt8)(type->type - TYPESTRUCT);
                    if (kindOffset <= 1)
                        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, type, 0);
                    else
                        CError_ReportError(ERR_ILLEGAL_TYPE);
                }
                result = intconstnode(CABI_GetSizeTType(), type->size);
            }
            return result;
        }

        case TK_LOGICAL_AND: {
            ENode *result;
            CLabel *label;
            if (copts.ANSIstrict == 0) {
                tk = CPrepTokenizer_GetNextToken();
                if (tk != TK_IDENTIFIER) {
                    CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                    return nullnode();
                }
                result = (ENode *)CompilerTools_AllocatePool(0x1a);
                result->type = ELOCOBJ;
                result->cost = 0;
                result->flags = 0;
                result->rtype = (Type *)&void_ptr;
                label = findlabel();
                result->data.label = label;
                if (result->data.label == NULL) {
                    result->data.label = newlabel();
                    result->data.label->name = data_00587fa0;
                    result->data.label->next = clabels;
                    clabels = result->data.label;
                }
                tk = CPrepTokenizer_GetNextToken();
                return result;
            }
            /* fall through */
        }

        default:
            return parse_postfix_expression(0);
    }
}

#pragma opt_propagation reset

ENode *CExpr_New_EBINNOT_Node(ENode *node)
{
    BinaryOperatorResult result;
    ENode *operand;
    ENode *expression;

    if (node->rtype->type == TYPETEMPLDEPEXPR) {
        expression = CTempl_MakeTemplDepExpr(NULL, EBINNOT, node);
        return expression;
    }
    node = pointer_generation(node);
    node = CExpr_RewriteConst(node);
    operand = node;
    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x7e, (ENode *)(node), (ENode *)(0), &result)) {
            expression = result.expression;
            if (expression != NULL)
                return expression;
            if ((operand = result.left) == NULL)
                CError_FATAL(3283);
        }
    }
    expression = forceintegral(operand);
    if (expression->type == EINTCONST) {
        expression->data.intval = CMach_CalcIntMonadic(expression->rtype, 0x7e, expression->data.intval);
        return expression;
    }
    expression = makemonadicnode(expression, EBINNOT);
    return expression;
}

ENode *CExpr_New_EMONMIN_Node(ENode *ene)
{
    ENode *node;
    ENode *res;
    BinaryOperatorResult out;

    if (TYPE(ene->rtype)->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(NULL, 5, ene);
    ene = pointer_generation(ene);
    ene = CExpr_RewriteConst(ene);
    node = ene;
    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x2d, (ENode *)(ene), (ENode *)(NULL), (BinaryOperatorResult *)(&out))) {
            res = out.expression;
            if (res != NULL)
                return res;
            if ((node = out.left) == NULL)
                CError_FATAL(3241);
        }
    }
    switch ((SInt8)TYPE(node->rtype)->type) {
        case TYPEINT:
        case TYPEENUM:
            node = forceintegral(node);
            if (node->type == EINTCONST) {
                node->data.intval = CMach_CalcIntMonadic(node->rtype, 0x2d, node->data.intval);
                return node;
            }
            return makemonadicnode(node, 5);
        case TYPEFLOAT:
            if (node->type == EFLOATCONST) {
                ENode *floating = (ENode *)node;
                floating->data.floatval = CMach_CalcFloatMonadic(node->rtype, 0x2d, floating->data.floatval.data.value);
                return (ENode *)floating;
            }
            return CExpr2_ReturnENode(makemonadicnode(node, 5));
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return node;
    }
}

#pragma opt_lifetimes off

ENode *CExpr_New_ELOGNOT_Node(ENode *expr)
{
    SInt32 value;
    ENode *node;
    BinaryOperatorResult result;

    if ((node = (ENode *)expr->rtype)->type == TYPETEMPLDEPEXPR)
        return CTempl_MakeTemplDepExpr(NULL, 7, expr);

    expr = CExpr_RewriteConst(pointer_generation(expr));
    node = expr;

    if (copts.cplusplus != 0) {
        if (CExpr_CheckOperator(0x21, expr, NULL, &result)) {
            expr = result.expression;
            if (expr != NULL)
                return expr;
            CError_ASSERT(3185, node = result.left);
        }
    }

    switch ((SInt8)node->rtype->type) {
        case TYPEENUM:
            node = CExpr_ConvertToIntegral(node);
            break;
        case TYPEMEMBERPOINTER:
            node = CExpr2_ConvertScalarOperand(node, 0, 0);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            return node;
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEPOINTER:
        case TYPEARRAY:
            break;
    }

    switch (node->type) {
        case EINTCONST:
            node->data.intval = CInt64_Not(node->data.intval);
            break;
        case EFLOATCONST:
            node->type = EINTCONST;
            value = CMach_FloatIsZero(node->data.floatval.data.value);
            node->data.intval.lo = value;
            node->data.intval.hi = value < 0 ? -1 : 0;
            break;
        default:
            node = makemonadicnode(node, ELOGNOT);
            break;
    }

    if (copts.cplusplus > 0 && copts.booltruefalse != 0)
        node->rtype = (Type *)&stbool;
    else
        node->rtype = (Type *)&stsignedint;
    return node;
}

#pragma opt_lifetimes reset

void *make_memberpointer(ENode *node)
{
    SInt32 offset;
    ENode *result;
    TypeMemberPointer *memberType;
    BClassList *base;
    ObjMemberVar *member;

    CError_ASSERT(3132, node->type == ENEWEXCEPTIONARRAY);
    if (node->data.emember->expr != NULL) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
    }
    node->data.emember->addressTaken = 1;
    if (node->data.emember->list->next == NULL) {
        if ((member = (ObjMemberVar *)node->data.emember->list->object)->otype == OT_MEMBERVAR) {
            base = node->data.emember->path;
            while (base->next != NULL) {
                base = base->next;
            }
            memberType = galloc(sizeof(TypeMemberPointer));
            memclrw(memberType, sizeof(TypeMemberPointer));
            memberType->type = TYPEMEMBERPOINTER;
            memberType->size = 4;
            memberType->ty2 = base->type;
            memberType->ty1 = member->type;
            result = nullnode();
            result->rtype = (Type *)memberType;
            offset = member->offset + 1;
            result->data.intval.lo = offset;
            result->data.intval.hi = (offset < 0) ? -1 : 0;
            return result;
        } else {
            return getpointertomemberfunc(node, NULL, 1);
        }
    } else {
        return node;
    }
}

ENode *getpointertomemberfunc(ENode *node, Type *targetType, Boolean initialize)
{
    EMemberInfo *memberRef;
    ObjectList *candidate;
    TypeMemberPointer *memberPointer;
    TypeMemberFunc *functionCopy;
    Object *method;
    Object *result;
    BClassList *bases;
    OLinkList *reference;
    MemberPointerInitializer initializer;
    TypeMemberFunc *functionType;
    ObjectList *methods;

    CError_ASSERT(3021, node->type == ENEWEXCEPTIONARRAY);

    memberRef = node->data.emember;
    if (memberRef->expr != NULL && copts.cpp_extensions == 0)
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);

    if (copts.cpp_extensions == 0 &&
        ((memberRef = node->data.emember)->addressTaken == 0 || memberRef->is_qualified == 0) && targetType != NULL &&
        targetType->type == TYPEMEMBERPOINTER)
        CError_Warning(ERR_ILLEGAL_IMPLICIT_MEMBER_POINTER_CONVERSION);

    memberRef = node->data.emember;
    if ((methods = (ObjectList *)memberRef->list)->next != NULL) {
        if (targetType != NULL) {
            if (targetType->type == TYPEMEMBERPOINTER) {
                Type *functionTarget = ((TypeMemberPointer *)targetType)->ty1;
                for (candidate = methods; candidate != NULL; candidate = candidate->next) {
                    if (candidate->object->otype == OT_OBJECT &&
                        is_memberpointerequal(candidate->object->type, functionTarget) != 0) {
                        method = candidate->object;
                        break;
                    }
                }
                if (candidate == NULL) {
                    CError_ReportError(ERR_ILLEGAL_TYPE);
                    return nullnode();
                }
            } else {
                make_static_method_setconst(methods);
                return;
            }
        }
    } else {
        method = methods->object;
    }

    while (method->datatype == DALIAS)
        method = method->u.alias.object;

    CError_ASSERT(3068, method->otype == OT_OBJECT && method->type->type == TYPEFUNC &&
                            (TYPE_METHOD(method->type)->flags & FUNC_METHOD) != 0 &&
                            TYPE_METHOD(method->type)->is_static == 0);

    memberRef = node->data.emember;
    bases = memberRef->path;
    while ((bases = bases->next) != NULL)
        ;

    functionCopy = (TypeMemberFunc *)galloc(sizeof(*functionCopy));
    memclrw(functionCopy, sizeof(*functionCopy));
    *functionCopy = *TYPE_METHOD(method->type);
    functionCopy->args = functionCopy->args->next;
    CDecl_MakePTMFuncType(TYPE_FUNC(functionCopy));
    functionCopy->flags &= ~FUNC_DEFINED;

    memberPointer = (TypeMemberPointer *)galloc(sizeof(*memberPointer));
    memclrw(memberPointer, sizeof(*memberPointer));
    memberPointer->type = TYPEMEMBERPOINTER;
    memberPointer->size = 0xc;
    memberPointer->ty2 = TYPE(TYPE_METHOD(method->type)->theclass);
    memberPointer->ty1 = (Type *)functionCopy;

    functionType = TYPE_METHOD(method->type);
    result = CParser_NewObject(NULL);
    result->name = CParser_GetUniqueName();
    result->nspace = cscope_root;
    result->type = (Type *)memberPointer;
    result->sclass = TK_STATIC;

    if (initialize != 0) {
        initializer.adjustment = CTool_EndianConvertWord32(0);
        if (method->datatype == DVFUNC) {
            reference = NULL;
            initializer.index = CTool_EndianConvertWord32(functionType->vtbl_index);
            initializer.tableSize = CTool_EndianConvertWord32(functionType->theclass->vtable->offset);
        } else {
            initializer.index = CTool_EndianConvertWord32(-1);
            initializer.tableSize = CTool_EndianConvertWord32(0);
            reference = (OLinkList *)galloc(sizeof(*reference));
            reference->next = NULL;
            reference->obj = method;
            reference->addend = 0;
            reference->offset = 8;
        }
        fn_004ceab0(result, &initializer, reference, result->type->size);
    }
    return create_objectnode(result);
}

void make_static_method_setconst(ObjectList *objects)
{
    TypeMemberFunc *functionType;
    ENode *result;
    ObjectList *matches;

    matches = NULL;
    if (objects != NULL) {
        do {
            if (((objects->object->otype == OT_OBJECT) &&
                 ((functionType = (TypeMemberFunc *)objects->object->type)->type == TYPEFUNC)) &&
                ((functionType->flags & FUNC_METHOD) != 0) && (functionType->is_static != 0)) {
                ObjectList *entry;
                entry = (ObjectList *)galloc(sizeof(ObjectList));
                *entry = *objects;
                entry->next = matches;
                matches = entry;
            }
            objects = objects->next;
        } while (objects != NULL);
    }
    if (matches == NULL) {
        CError_Warning(ERR_ILLEGAL_IMPLICIT_MEMBER_POINTER_CONVERSION);
        nullnode();
        return;
    }
    result = CExpr_NewENode(ENEWEXCEPTION);
    result->rtype = matches->object->type;
    /* This node's object-reference slot holds the matching object list. */
    result->data.overloadCandidates = matches;
}

ENode *getnodeaddress(ENode *node, Boolean flag)
{
    ENode *newnode;
    Object *obj;

    if (!ENODE_IS(node, EINDIRECT)) {
        node = CExpr_LValue(node, flag, 1);
        if (!ENODE_IS(node, EINDIRECT))
            return (ENode *)nullnode();
    }
    newnode = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *newnode = *node;
    for (;;) {
        switch (newnode->data.monadic->type) {
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
                newnode->type = ETYPCON;
                if (newnode->rtype->type == TYPEPOINTER)
                    newnode->flags = (UInt16)(TYPE_POINTER(newnode->rtype)->qual & ENODE_FLAG_QUALS);
                newnode->rtype = (Type *)CDecl_NewPointerType(newnode->rtype);
                return newnode;
            case EOBJREF:
                obj = newnode->data.monadic->data.objref;
                if (obj->datatype == DALIAS) {
                    CExpr_AliasTransform(newnode->data.monadic);
                    continue;
                }
                if (obj->datatype == DINLINEFUNC)
                    CError_ReportError(ERR_ILLEGAL_USE_INLINE_FUNCTION);
                obj->flags |= OBJECT_FLAGS_2;
                if (flag != 0 && copts.cplusplus == 0 && obj->sclass == TK_REGISTER)
                    CError_ReportError(ERR_ILLEGAL_USE_REGISTER_VARIABLE);
                break;
            case EFUNCCALL:
                if (flag != 0 && newnode->data.monadic->data.funccall.functype->functype->type != TYPEPOINTER)
                    CError_Warning(ERR_NOT_LVALUE);
                break;
            case EBITFIELD:
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                return (ENode *)nullnode();
            default:
                break;
        }
        switch ((SInt8)newnode->rtype->type) {
            case TYPEPOINTER:
                newnode->data.monadic->rtype = (Type *)CDecl_NewPointerType(newnode->rtype);
                newnode->data.monadic->flags = newnode->flags;
                break;
            default:
                newnode->data.monadic->rtype = (Type *)CDecl_NewPointerType(newnode->rtype);
                newnode->data.monadic->flags = newnode->flags;
                break;
        }
        return newnode->data.monadic;
    }
}

SInt32 scansizeof(void)
{
    Type *ty;
    ENode *node;

    ty = scan_type_or_expression_type();
    if (ty->type == TYPECLASS && TYPE_CLASS(ty)->sominfo != NULL)
        CError_ReportError(ERR_SIZEOF_NOT_SUPPORTED_SOM_CLASSES);
    if (ty->type == TYPETEMPLATE) {
        node = CExpr_NewTemplDepENode(1);
        node->data.monadic = (ENode *)ty;
    } else {
        if (ty->size == 0) {
            if (ty->type == TYPESTRUCT || ty->type == TYPECLASS)
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, ty, 0);
            else
                CError_ReportError(ERR_ILLEGAL_TYPE);
        }
        node = intconstnode(CABI_GetSizeTType(), ty->size);
    }
    if (node->type != EINTCONST) {
        CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
        return 0;
    }
    return node->data.intval.lo;
}

ENode *parse_postfix_expression(Boolean allowSpecial)
{
    ENode *expr;
    ENode *left;
    ENode *right;
    union {
        ENode *node;
        ENodeList *list;
    } operand;
    SInt8 typeKind;
    ENode *result;
    BinaryOperatorResult overload;
    NameResult info;
    DeclInfo declaration;
    StructMember *member;
    TypeClass *classType;

    if (copts.cplusplus != 0) {
        switch (tk) {
            case TK_VOID:
            case TK_CHAR:
            case TK_SHORT:
            case TK_INT:
            case 0x10a:
            case TK_FLOAT:
            case TK_DOUBLE:
            case TK_SIGNED:
            case TK_UNSIGNED:
            case 0x113:
            case 0x114:
            case 0x115:
            case 0x116:
            case 0x117:
            case 0x118:
            case 0x119:
            case 0x11a:
            case TK_BOOL:
            case TK_WCHAR_T:
                memclrw(&declaration, sizeof(declaration));
                if (copts.cpp_extensions == 0 && CPrepTokenizer_GetNextTokenAndRestorePosition() != '(')
                    CError_ReportError(ERR_LPAREN_EXPECTED);
                CParser_GetDeclSpecs(&declaration, 0);
                return scan_explicit_conversion(declaration.thetype, declaration.qual);
            default:
                expr = parse_primary_expression(allowSpecial);
                break;
            case TK_CONST_CAST:
                expr = CRTTI_ParseConstCast();
                break;
            case TK_DYNAMIC_CAST:
                expr = CRTTI_ParseDynamicCast();
                break;
            case TK_REINTERPRET_CAST:
                expr = CRTTI_ReinterpretCast();
                break;
            case TK_STATIC_CAST:
                expr = CRTTI_ParseExplicitTypecast();
                break;
            case TK_TYPEID:
                expr = CRTTI_ParseTypeid();
                break;
        }
    } else {
        expr = parse_primary_expression(allowSpecial);
    }

    for (;;) {
        switch (tk) {
            case '[':
                right = generate_pointer_then_rewrite_const(expr);
                expr = right;
                tk = CPrepTokenizer_GetNextToken();
                left = parse_comma_expression();
                if (copts.cplusplus != 0 && CExpr_CheckOperator('[', right, left, &overload)) {
                    expr = overload.expression;
                    if (expr != NULL) {
                        if (tk != ']')
                            CError_ReportErrorAndUpdateToken(ERR_RBRACKET_EXPECTED);
                        else
                            tk = CPrepTokenizer_GetNextToken();
                        continue;
                    }
                    if ((expr = overload.left) == NULL)
                        CError_FATAL(2553);
                    if ((left = overload.right) == NULL)
                        CError_FATAL(2554);
                }
                do {
                    typeKind = expr->rtype->type;
                    if (typeKind >= TYPEPOINTER)
                        result = add_pointer_offset(expr, left);
                    else {
                        typeKind = left->rtype->type;
                        if (typeKind >= TYPEPOINTER)
                            result = add_pointer_offset(left, expr);
                        else {
                            CError_ReportError(ERR_POINTER_ARRAY_REQUIRED);
                            break;
                        }
                    }
                    expr = makemonadicnode(result, EINDIRECT);
                    expr->rtype = ((TypePointer *)expr->rtype)->target;
                } while (0);
                if (tk != ']')
                    CError_ReportErrorAndUpdateToken(ERR_RBRACKET_EXPECTED);
                else
                    tk = CPrepTokenizer_GetNextToken();
                continue;

            case '(':
                expr = callable_expression(expr);
                if (copts.cplusplus != 0) {
                    if (CExpr_CheckOperator('(', expr, NULL, &overload)) {
                        expr = overload.expression;
                        if (expr == NULL)
                            CError_FATAL(2572);
                        continue;
                    }
                    if (expr->type == EQUALNAME) {
                        result = scan_member_function_pointer_call(expr);
                        expr = unwrap_reference(result);
                        continue;
                    }
                }
                tk = CPrepTokenizer_GetNextToken();
                operand.list = CExpr_ScanExpressionList(1);
                if (tk != ')')
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                if (expr->type == EOBJLIST) {
                    result = CExpr_NewENode(EFUNCCALL);
                    result->rtype = &data_0055d5c0;
                    result->data.funccall.funcref = expr;
                    result->data.funccall.args = operand.list;
                    result->data.funccall.functype = &data_0055d5e8;
                    tk = CPrepTokenizer_GetNextToken();
                    expr = result;
                } else {
                    result = CExpr_MakeFunctionCall(expr, operand.list);
                    expr = unwrap_reference(result);
                    tk = CPrepTokenizer_GetNextToken();
                }
                continue;

            case TK_ARROW:
                expr = generate_pointer_then_rewrite_const(expr);
                if (copts.cplusplus != 0) {
                    while (expr->rtype->type == TYPECLASS && CExpr_CheckOperator(TK_ARROW, expr, NULL, &overload)) {
                        ENode *overloaded = overload.expression;
                        if (overloaded == NULL)
                            CError_FATAL(2607);
                        expr = generate_pointer_then_rewrite_const(overloaded);
                    }
                }
                typeKind = expr->rtype->type;
                if (typeKind < TYPEPOINTER) {
                    CError_ReportErrorAndUpdateToken(ERR_POINTER_ARRAY_REQUIRED);
                    return expr;
                }
                if (copts.cplusplus != 0 && copts.objective_c != 0 && CObjC_IsIdOrSelType(expr->rtype)) {
                    expr = makemonadicnode(expr, EINDIRECT);
                    expr->rtype = ((TypePointer *)expr->rtype)->target;
                    result = CObjCModern_TryParseMethodCall(NULL, expr);
                    if (result != NULL)
                        return result;
                } else {
                    expr = makemonadicnode(expr, EINDIRECT);
                    expr->rtype = ((TypePointer *)expr->rtype)->target;
                }
                /* fall through */

            case '.':
                left = generate_pointer_then_rewrite_const(expr);
                expr = left;
                if (left->type == EFUNCCALL) {
                    if ((left->rtype->type == TYPECLASS || left->rtype->type == TYPESTRUCT) &&
                        !Type_RequiresMemoryReturn(left->rtype)) {
                        expr = materialize_temporary(left);
                    }
                }
                if ((allowSpecial = expr->rtype->type) == TYPECLASS) {
                    CDecl_CompleteType(expr->rtype);
                    if (((TypeClass *)expr->rtype)->objcinfo != NULL && copts.cplusplus != 0) {
                        classType = (TypeClass *)expr->rtype;
                        result = CObjCModern_TryParseMethodCall(classType, expr);
                        if (result != NULL)
                            return result;
                    }
                    if (!(((TypeClass *)expr->rtype)->flags & CLASS_COMPLETED))
                        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, expr->rtype, 0);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_TEMPLATE)
                        tk = CPrepTokenizer_GetNextToken();
                    classType = (TypeClass *)expr->rtype;
                    if (CScope_ParseMemberName(classType, &info, 0)) {
                        if (info.is_destructor != 0) {
                            tk = CPrepTokenizer_GetNextToken();
                            if (tk == '(') {
                                tk = CPrepTokenizer_GetNextToken();
                                if (tk != ')')
                                    CError_ReportError(ERR_RPAREN_EXPECTED);
                                else
                                    tk = CPrepTokenizer_GetNextToken();
                            } else {
                                CError_ReportError(ERR_LPAREN_EXPECTED);
                            }
                            expr = makemonadicnode(expr, ETYPCON);
                            expr->rtype = &stvoid;
                        } else {
                            result = make_scope_parse_result_expr(&info, expr, 0, 0);
                            result = unwrap_reference(result);
                            expr = result;
                        }
                    }
                    continue;
                }
                if (allowSpecial != TYPESTRUCT || ((TypeStruct *)expr->rtype)->stype > 3) {
                    if (copts.cplusplus != 0) {
                        result = scan_pseudo_destructor_call(expr);
                        if (result != NULL)
                            return result;
                    }
                    CError_ReportErrorAndUpdateToken(ERR_NOT_STRUCT_UNION_CLASS);
                    return expr;
                }
                if (expr->type != EINDIRECT)
                    expr = materialize_temporary(expr);
                tk = CPrepTokenizer_GetNextToken();
                if (tk != TK_IDENTIFIER) {
                    CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                    return expr;
                }
                member = ismember(expr->rtype, data_00587fa0);
                if (member == NULL) {
                    if (expr->rtype->size == 0)
                        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, expr->rtype, 0);
                    else
                        CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
                    return expr;
                }
                typeKind = expr->data.monadic->rtype->type;
                if (typeKind < TYPEPOINTER) {
                    CError_ReportErrorAndUpdateToken(ERR_NOT_STRUCT_UNION_CLASS);
                    return expr;
                }
                result = CClass_AccessMember(expr, member->type, member->qual, member->offset);
                result = unwrap_reference(result);
                tk = CPrepTokenizer_GetNextToken();
                expr = result;
                continue;

            case TK_INCREMENT:
                expr = generate_pointer_then_rewrite_const(expr);
                if (copts.cplusplus != 0 && CExpr_CheckOperator(TK_INCREMENT, expr, nullnode(), &overload)) {
                    expr = overload.expression;
                    if (expr == NULL)
                        CError_FATAL(2737);
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                operand.node = CExpr_LValue(expr, 1, 1);
                if (operand.node->rtype == (Type *)&stbool) {
                    expr = CExpr_TempModifyExpr(operand.node);
                    tk = CPrepTokenizer_GetNextToken();
                } else {
                    CExpr_004fb400(operand.node);
                    expr = makemonadicnode(operand.node, EPOSTINC);
                    tk = CPrepTokenizer_GetNextToken();
                }
                continue;

            case TK_DECREMENT:
                left = generate_pointer_then_rewrite_const(expr);
                if (copts.cplusplus != 0 && CExpr_CheckOperator(TK_DECREMENT, left, nullnode(), &overload)) {
                    expr = overload.expression;
                    if (expr == NULL)
                        CError_FATAL(2753);
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                operand.node = CExpr_LValue(left, 1, 1);
                CExpr_004fb400(operand.node);
                expr = makemonadicnode(operand.node, EPOSTDEC);
                tk = CPrepTokenizer_GetNextToken();
                continue;

            default:
                return expr;
        }
    }
}

ENode *scan_pseudo_destructor_call(ENode *node)
{
    DeclInfo decl;
    SInt32 savedPosition;
    NameResult lookup;
    NameSpace *scope;

    CPrep_GetBufferedTokenPosition(&savedPosition);
    scope = cscope_current;
    tk = CPrepTokenizer_GetNextToken();
    if (tk == TK_COLON_COLON) {
        scope = cscope_root;
        tk = CPrepTokenizer_GetNextToken();
    } else if (tk != TK_COMPL && tk != TK_IDENTIFIER) {
        SInt32 token = tk;
        if (token < 0x100 || token > 0x131) {
            CPrep_SetPosition(&savedPosition);
            return NULL;
        }
    }
    for (;;) {
        if (tk == TK_COMPL)
            break;
        if (tk == TK_IDENTIFIER) {
            if (CScope_FindTypeName(scope, data_00587fa0, &lookup)) {
                if (lookup.nspace != NULL) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_COLON_COLON) {
                        tk = CPrepTokenizer_GetNextToken();
                        scope = lookup.nspace;
                        continue;
                    }
                } else {
                    if (lookup.qual != 0 || iscpp_typeequal(lookup.type, node->rtype) == 0)
                        CError_ReportError(ERR_ILLEGAL_TYPE);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_COLON_COLON) {
                        tk = CPrepTokenizer_GetNextToken();
                        if (tk == TK_COMPL)
                            break;
                    }
                }
            }
        } else {
            SInt32 token = tk;
            if (token >= 0x100 && token <= 0x131) {
                memclrw(&decl, sizeof(decl));
                CParser_GetDeclSpecs(&decl, 0);
                if (decl.storageclass != 0 || decl.qual != 0 || iscpp_typeequal(decl.thetype, node->rtype) == 0)
                    CError_ReportError(ERR_ILLEGAL_TYPE);
                if (tk == TK_COLON_COLON) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_COMPL)
                        break;
                }
            }
        }
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        return NULL;
    }
    tk = CPrepTokenizer_GetNextToken();
    memclrw(&decl, sizeof(decl));
    CParser_GetDeclSpecs(&decl, 0);
    if (decl.storageclass != 0 || iscpp_typeequal(decl.thetype, node->rtype) == 0)
        CError_ReportError(ERR_ILLEGAL_TYPE);
    if (CParser_IsConst(node->rtype, node->flags & 3))
        CError_ReportError(ERR_CANNOT_DESTROY_CONST_OBJECT);
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk != ')')
            CError_ReportError(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
    if (node->type != EINDIRECT)
        CError_FATAL(2483);
    node->type = ETYPCON;
    node->rtype = &stvoid;
    return node;
}

ENode *scan_member_function_pointer_call(ENode *expr)
{
    ENodeList *link;
    ENode *next;
    ENode *operand;
    Type *functionType;
    ENodeList *argument;
    Object *object;
    ENodeList *arguments;
    functionType = ((TypeMemberFunc *)((TypePointer *)expr->data.diadic.right->rtype)->target)->functype;
    tk = (SInt16)CPrepTokenizer_GetNextToken();
    argument = CExpr_ScanExpressionList(1);
    if (tk != ')') {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return (ENode *)nullnode();
    }
    if (Type_RequiresMemoryReturn(functionType))
        object = member_function_pointer_call_rtfunc;
    else
        object = data_00587fd0;
    link = (ENodeList *)CompilerTools_AllocatePool(8);
    link->next = argument;
    operand = expr->data.diadic.left;
    arguments = link;
    next = operand->data.diadic.left;
    link->node = next;
    link = (ENodeList *)CompilerTools_AllocatePool(8);
    link->next = arguments;
    operand = expr->data.diadic.right;
    next = operand->data.diadic.left;
    link->node = next;
    expr = CExpr2_0046e9d0(object, ((TypePointer *)expr->data.diadic.right->rtype)->target, link);
    tk = (SInt16)CPrepTokenizer_GetNextToken();
    return expr;
}

ENode *parse_primary_expression(Boolean expressionMode)
{
    NameResult lookupResult;
    ENode *expression;
    ENode *stringNode;

    switch (tk) {
        case TK_TRUE:
            expression = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            expression->type = EINTCONST;
            expression->cost = 0;
            expression->flags = 0;
            expression->rtype = (Type *)&stbool;
            expression->data.intval.lo = 1;
            expression->data.intval.hi = 0;
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_FALSE:
            expression = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            expression->type = EINTCONST;
            expression->cost = 0;
            expression->flags = 0;
            expression->rtype = (Type *)&stbool;
            expression->data.intval.lo = 0;
            expression->data.intval.hi = 0;
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_INTCONST:
            expression = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            expression->type = EINTCONST;
            expression->cost = 0;
            expression->flags = 0;
            expression->rtype = (Type *)atomtype();
            expression->data.intval = token_integer;
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_FLOATCONST:
            expression = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            expression->type = EFLOATCONST;
            expression->cost = 0;
            expression->flags = 0;
            expression->rtype = (Type *)atomtype();
            expression->data.floatval = token_float;
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_STRING:
            expression = CExpr_NewENode(ESTRINGCONST);
            expression->rtype = CDecl_NewArrayType((data_005882de != 0) ? (Type *)&stunsignedchar : (Type *)&stchar,
                                                   token_value_kind_or_string_length);
            expression->data.string.size = token_value_kind_or_string_length;
            expression->data.string.data = string_token_data;
            expression->data.string.useExplicitSize = data_005882de;
            if (copts.const_strings != 0)
                expression->flags = ENODE_FLAG_CONST;
            expression = makemonadicnode(expression, EINDIRECT);
            expression->data.monadic->rtype = CDecl_NewPointerType(expression->rtype);
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_STRING_WIDE:
            stringNode = CExpr_NewENode(ESTRINGCONST);
            stringNode->rtype = CDecl_NewArrayType(CParser_GetWCharType(), token_value_kind_or_string_length);
            stringNode->data.string.size = token_value_kind_or_string_length;
            stringNode->data.string.data = string_token_data;
            stringNode->data.string.useExplicitSize = data_005882de;
            if (copts.const_strings != 0)
                stringNode->flags = ENODE_FLAG_CONST;
            expression = makemonadicnode(stringNode, EINDIRECT);
            expression->data.monadic->rtype = CDecl_NewPointerType(expression->rtype);
            tk = CPrepTokenizer_GetNextToken();
            return expression;
        case TK_THIS:
        case 0x180:
            return parse_4e9940();
        case TK_AT_SELECTOR:
            return CDecl_ParseSelectorExpression();
        case TK_AT_ENCODE:
            return CObjC_ParseEncodeExpression();
        case TK_AT_PROTOCOL:
            return CDecl_ParseProtocolExpression();
        case '@':
            if (copts.objective_c != 0)
                return CObjC_ParseStringConstant();
            break;
        case '(': {
            tk = CPrepTokenizer_GetNextToken();
            expression = parse_comma_operator_expression();
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (expression->type == EASS)
                expression->flags |= ENODE_FLAG_80;
            return expression;
        }
        case '[':
            if (copts.objective_c != 0)
                return CObjC_ParseMessageExpression();
            break;
        case TK_IDENTIFIER:
            if (data_00587fa0->name[0] == '_' && data_00587fa0->name[1] == '_') {
                if (memcmp(data_00587fa0->name, "__builtin_align", 16) == 0) {
                    expression = intconstnode((Type *)&stsignedint, 0);
                    set_signed_integer(expression, builtin_align(scan_type_or_expression_type()));
                    return expression;
                }
                if (memcmp(data_00587fa0->name, "__builtin_ntype", 16) == 0)
                    return parse_builtin_8b50();
                if (memcmp(data_00587fa0->name, "__builtin_type", 15) == 0 ||
                    memcmp(data_00587fa0->name, "__builtin_vargtype", 19) == 0) {
                    expression = intconstnode((Type *)&stsignedint, 0);
                    set_signed_integer(expression, CExpr_004f8a40(scan_type_or_expression_type()));
                    return expression;
                }
                if (memcmp(data_00587fa0->name, "__builtin_classify_type", 24) == 0) {
                    expression = intconstnode((Type *)&stsignedint, 0);
                    builtin_classify(expression, scan_type_or_expression_type());
                    return expression;
                }
            }
            if (copts.altivec_model != 0) {
                if (memcmp("vec_step", data_00587fa0->name, 9) == 0)
                    return scan_vec_step();
            }
            if (memcmp("_INFO", data_00587fa0->name, 6) == 0 ||
                memcmp("_var_arg_typeof", data_00587fa0->name, 16) == 0) {
                expression = intconstnode((Type *)&stsignedint, 0);
                set_signed_integer(expression, CExpr_004f8a40(scan_type_or_expression_type()));
                return expression;
            }
            /* fall through */
        case TK_COMPL:
        case TK_OPERATOR:
        case 0x151:
        case TK_COLON_COLON:
            if (CScope_ParseExprName(&lookupResult))
                return make_scope_parse_result_expr(&lookupResult, NULL, expressionMode, 1);
            tk = CPrepTokenizer_GetNextToken();
            return nullnode();
        default:
            break;
    }
    CError_ReportErrorAndUpdateToken(ERR_EXPRESSION_SYNTAX_ERROR);
    return nullnode();
}

Type *scan_type_or_expression_type(void)
{
    UInt8 isType;
    Type *type;
    ENode *expression;
    DeclInfo declaration;

    tk = CPrepTokenizer_GetNextToken();
    if ((tk == '(') && (isType = islookaheaddeclaration(), isType != '\0')) {
        tk = CPrepTokenizer_GetNextToken();
        memclrw((unsigned char *)&declaration, sizeof(declaration));
        CParser_GetDeclSpecs((DeclInfo *)&declaration, 0);
        CDecl_ParseDeclarator(&declaration);
        if (declaration.name != NULL) {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        }
        if (tk != ')') {
            CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
        } else {
            tk = CPrepTokenizer_GetNextToken();
        }
        type = declaration.thetype;
        if (type->type == TYPEPOINTER && (((TypePointer *)type)->qual & Q_REFERENCE) != 0) {
            type = ((TypePointer *)type)->target;
        }
    } else {
        expression = unary_expression();
        if ((expression->type == TYPESTRUCT) && (expression->data.objref->otype == '1')) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
        }
        type = expression->rtype;
    }
    CDecl_CompleteType(type);
    return type;
}

int CExpr_004f8a40(Type *type)
{
    int result;
    switch ((SInt8)type->type) {
        case TYPESTRUCT:
            if (type->type == TYPESTRUCT && CExpr_StructKind(type) >= 4 && CExpr_StructKind(type) <= 14) {
                result = 4;
                break;
            }
            /* fallthrough */
        case TYPECLASS:
            result = 0;
            break;
        case TYPEFLOAT:
            if (copts.operandsDebug) {
                if (TYPE_INTEGRAL(type)->integral == IT_FLOAT)
                    result = 1;
                else if (copts.incompatible_sfpe_double_params != 0)
                    result = 10;
                else
                    result = 2;
            } else
                result = 3;
            break;
        case TYPEINT:
        case TYPEENUM:
            if (TYPE_INTEGRAL(type)->integral == IT_LONGLONG || TYPE_INTEGRAL(type)->integral == IT_ULONGLONG)
                result = 2;
            else
                result = 1;
            break;
        default:
            result = 1;
            break;
    }
    return result;
}

unsigned int fn_004f8ae0(const signed char *kind)
{
    switch (*kind) {
        case 0:
            return 0;
        case 6:
            return 10;
        case 3:
            return 3;
        case 1:
            return 1;
        case 2:
            return 8;
        case 10:
        case 11:
            return 5;
        case 12:
            return 14;
        case 4:
            return 12;
        case 5:
            return 12;
        case 7:
            return -1U;
        case 9:
            return -1U;
        case 8:
        default:
            return -1U;
    }
}

UInt32 encode_type_bits(TypeKind *e)
{
    switch ((SInt8)e->type) {
        case TYPEINT:
            return encode_kind(e->u.integral) | 0x100;
        case TYPEFLOAT:
            return encode_kind(e->u.integral) | 0x200;
        case TYPEENUM:
            return (encode_type_bits(e->u.tenum.enumtype) & 0xff) | 0x400;
        case TYPEPOINTER:
            return 0x800;
        case TYPEARRAY:
            return 0x1000;
        case TYPESTRUCT:
            return 0x2000;
        case TYPECLASS:
            return 0x2000;
        case TYPEMEMBERPOINTER:
            return 0x4000;
        case TYPEFUNC:
            return 0x8000;
        default:
            CError_ReportError(ERR_ILLEGAL_TYPE);
        case TYPEVOID:
            return 0;
    }
}

unsigned int encode_kind(unsigned char kind)
{
    switch (kind) {
        case 0:
            return 1U;
        case 1:
            return 2U;
        case 2:
            return 3U;
        case 3:
            return 4U;
        case 4:
            return 5U;
        case 5:
            return 6U;
        case 6:
            return 7U;
        case 7:
            return 8U;
        case 8:
            return 9U;
        case 9:
            return 10U;
        case 10:
            return 10U;
        case 11:
            return 12U;
        case 12:
            return 13U;
        case 13:
            return 14U;
        case 14:
            return 15U;
        case 15:
            return 16U;
        case 16:
            return 17U;
        case 17:
            return 32U;
        case 18:
            return 33U;
        case 19:
            return 34U;
        case 20:
            return 35U;
        case 21:
            return 36U;
        case 22:
            return 37U;
        case 23:
            return 38U;
        case 24:
            return 39U;
        default:
            CError_FATAL(1851);
            return 0U;
    }
}

ENode *scan_vec_step(void)
{
    ENode *node;
    Type *ty;
    DeclInfo decl;
    SInt32 kind;
    SInt32 size;

    node = intconstnode((Type *)&stsignedint, 0);
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        if (tk == '(' && islookaheaddeclaration()) {
            tk = CPrepTokenizer_GetNextToken();
            memclrw(&decl, sizeof(decl));
            CParser_GetDeclSpecs(&decl, 0);
            CDecl_ParseDeclarator(&decl);
            if (decl.name != NULL) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            if (tk != ')') {
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            } else {
                tk = CPrepTokenizer_GetNextToken();
            }
            ty = decl.thetype;
            if (ty->type == TYPEPOINTER && (TYPE_POINTER(ty)->qual & Q_REFERENCE) != 0) {
                ty = TPTR_TARGET(ty);
            }
        } else {
            node = unary_expression();
            if (node->type == EINDIRECT && node->data.monadic->type == EBITFIELD) {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            }
            ty = node->rtype;
        }
        CDecl_CompleteType(ty);
        if (ty->type == TYPESTRUCT && (kind = TYPE_STRUCT(ty)->stype) >= 4 && kind <= 14) {
            switch (kind) {
                case 4:
                case 5:
                case 6:
                    size = 0x10;
                    break;
                case 7:
                case 8:
                case 9:
                case 14:
                    size = 8;
                    break;
                default:
                    size = 4;
                    break;
            }
            node = intconstnode((Type *)&stsignedint, size);
        } else {
            PPCError_ReportError(0x68, "vec_step", "vec_step", ty, 0);
        }
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
    return node;
}

ENode *make_scope_parse_result_expr(NameResult *nameResult, ENode *expr, Boolean allowMemberReference,
                                    Boolean allowFunctionCall)
{
    ENode *result;
    Object *object;
    EMemberInfo *memberRef;
    NameSpaceObjectList *overload;
    UInt8 datatype;

    if (nameResult->type) {
        if (copts.cplusplus) {
            if (nameResult->type->type == TYPETEMPLATE) {
                if (TYPE_TEMPLATE(nameResult->type)->dtype == 0 && !TYPE_TEMPLATE(nameResult->type)->u.pid.type) {
                    result = CExpr_NewTemplDepENode(TDE_PARAM);
                    result->data.templdep.u.pid = TYPE_TEMPLATE(nameResult->type)->u.pid;
                    tk = CPrepTokenizer_GetNextToken();
                    return result;
                }
                if (TYPE_TEMPLATE(nameResult->type)->dtype == 1 && !nameResult->is_type) {
                    result = CExpr_NewTemplDepENode(TDE_QUALNAME);
                    result->data.templdep.u.qual.type = TYPE_TEMPLATE(nameResult->type)->u.qual.type;
                    result->data.templdep.u.qual.name = TYPE_TEMPLATE(nameResult->type)->u.qual.name;
                    tk = CPrepTokenizer_GetNextToken();
                    return result;
                }
            }
            tk = CPrepTokenizer_GetNextToken();
            scan_explicit_conversion(nameResult->type, nameResult->qual);
            return;
        }
        CError_ReportErrorAndUpdateToken(ERR_EXPRESSION_SYNTAX_ERROR);
        tk = CPrepTokenizer_GetNextToken();
        return nullnode();
    }

    if (nameResult->object) {
        switch (nameResult->object->otype) {
            case OT_OBJECT:
                if (OBJECT(nameResult->object)->nspace && OBJECT(nameResult->object)->nspace->theclass &&
                    (OBJECT(nameResult->object)->nspace->theclass->flags & CLASS_IS_TEMPL)) {
                    result = CExpr_NewTemplDepENode(5);
                    result->data.objref = OBJECT(nameResult->object);
                    tk = CPrepTokenizer_GetNextToken();
                    return result;
                }
                if (!(OBJECT(nameResult->object)->type->type == TYPEFUNC &&
                      (TYPE_FUNC(OBJECT(nameResult->object)->type)->flags & FUNC_METHOD) &&
                      !TYPE_METHOD(OBJECT(nameResult->object)->type)->is_static)) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == '<' && (result = make_member_function_esetconst(nameResult)))
                        return result;
                    if ((datatype = (object = OBJECT(nameResult->object))->datatype) == DLOCAL &&
                        object->u.var.info->func != cscope_currentfunc) {
                        CError_ReportError(ERR_ILLEGAL_ACCESS_LOCAL_VARIABLE_FROM_OTHER);
                        return nullnode();
                    }
                    if (datatype == DEXPR) {
                        result = fn_00513040(object->u.expr, 0);
                        if (result->rtype->type == TYPEPOINTER && result->type == EINTCONST) {
                            result = makemonadicnode(result, ETYPCON);
                            result->data.monadic->rtype = (Type *)&stunsignedlong;
                        }
                        return result;
                    }
                    CClass_CheckObjectAccess(nameResult->basePath, object);
                    if (tk == '(' && allowFunctionCall && OBJECT(nameResult->object)->datatype == DFUNC &&
                        copts.cplusplus && !nameResult->is_qualified &&
                        !(TYPE_FUNC(OBJECT(nameResult->object)->type)->flags & FUNC_METHOD)) {
                        result = CExpr_NewENode(ENEWEXCEPTION);
                        result->rtype = OBJECT(nameResult->object)->type;
                        result->data.objlist.list = galloc(sizeof(NameSpaceObjectList));
                        result->data.objlist.list->next = nameResult->objects;
                        result->data.objlist.list->object = nameResult->object;
                        result->data.objlist.name = OBJECT(nameResult->object)->name;
                        return result;
                    }
                    return create_objectnode(OBJECT(nameResult->object));
                }

                if (!CClass_HasTypeFuncFlag16384(OBJECT(nameResult->object)))
                    break;
                tk = CPrepTokenizer_GetNextToken();
                if (tk != '(') {
                    CError_ReportError(ERR_LPAREN_EXPECTED);
                    return nullnode();
                }
                tk = CPrepTokenizer_GetNextToken();
                if (tk != ')') {
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                    return nullnode();
                }
                if (!expr) {
                    CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                    tk = CPrepTokenizer_GetNextToken();
                    return nullnode();
                }
                if (nameResult->isambig)
                    CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                expr = CExpr2_004719c0(nameResult->basePath, NULL, expr, nameResult->object->access, 1);
                if (!expr)
                    break;
                tk = CPrepTokenizer_GetNextToken();
                return CABI_DestroyObject(OBJECT(nameResult->object), expr->data.monadic, 1, nameResult->is_qualified,
                                          0);

            case OT_ENUMCONST:
                CClass_CheckEnumAccess(nameResult->basePath, nameResult->object);
                if (IsZero(&OBJ_ENUM_CONST(nameResult->object)->val)) {
                    Type *type;
                    TypeEnum *enumType;
                    TypeClass *classType;
                    if ((type = OBJ_ENUM_CONST(nameResult->object)->type)->type == TYPEENUM &&
                        (enumType = TYPE_ENUM(type))->nspace && (classType = enumType->nspace->theclass) &&
                        (classType->flags & CLASS_IS_TEMPL)) {
                        TemplateAction *entry;
                        SInt32 offsetCount = 0;
                        ENode *found = NULL;
                        for (entry = ((TemplClass *)classType)->actions; entry; entry = entry->next) {
                            if (entry->type != TAT_ENUMERATOR)
                                continue;
                            if (entry->u.enumerator.initexpr) {
                                found = entry->u.enumerator.initexpr;
                                offsetCount = 0;
                            } else {
                                offsetCount++;
                            }
                            if (OBJ_BASE(entry->u.enumerator.objenumconst) != nameResult->object)
                                continue;
                            if (!found)
                                CError_FATAL(1398);
                            result = fn_00513040(found, 0);
                            if (offsetCount)
                                result = makediadicnode(result, intconstnode((Type *)&stsignedlong, offsetCount), EADD);
                            tk = CPrepTokenizer_GetNextToken();
                            return result;
                        }
                    }
                }
                result = CompilerTools_AllocatePool(sizeof(ENode));
                result->type = EINTCONST;
                result->cost = 0;
                result->flags = 0;
                result->rtype = OBJ_ENUM_CONST(nameResult->object)->type;
                result->data.intval = OBJ_ENUM_CONST(nameResult->object)->val;
                tk = CPrepTokenizer_GetNextToken();
                return result;

            case OT_MEMBERVAR:
                if (allowMemberReference && !expr && nameResult->is_qualified)
                    break;
                {
                    BClassList *baseClass;
                    ObjMemberVar *member;
                    if (nameResult->isambig)
                        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                    member = OBJ_MEMBER_VAR(nameResult->object);
                    baseClass = nameResult->basePath;
                    if (!baseClass)
                        CError_FATAL(1104);
                    if (TYPE_CLASS(baseClass->type)->sominfo) {
                        result = CSOM_CreateMemberAccessExpr(baseClass, member, expr);
                    } else {
                        BClassList *path = NULL;
                        if (member->has_path)
                            path = ((ObjMemberVarPath *)member)->path;
                        result = CExpr2_004719c0(baseClass, path, expr, member->access, 1);
                        if (!result)
                            result = nullnode();
                        else
                            result = CClass_AccessMember(result, member->type, member->qual, member->offset);
                    }
                    expr = dereference_reference_node(result);
                    tk = CPrepTokenizer_GetNextToken();
                    return expr;
                }

            default:
                CError_FATAL(1429);
        }

        memberRef = CompilerTools_AllocatePool(sizeof(EMemberInfo));
        memclrw(memberRef, sizeof(EMemberInfo));
        memberRef->path = nameResult->basePath;
        memberRef->expr = expr;
        memberRef->is_qualified = nameResult->is_qualified;
        memberRef->isambig = nameResult->isambig;
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '<' && (result = make_member_function_esetconst(nameResult))) {
            if (result->type != ENEWEXCEPTION)
                CError_FATAL(1441);
            memberRef->list = result->data.objlist.list;
            memberRef->templargs = result->data.objlist.templargs;
        } else {
            memberRef->list = galloc(sizeof(NameSpaceObjectList));
            memberRef->list->next = NULL;
            memberRef->list->object = nameResult->object;
        }
        result = CExpr_NewENode(ENEWEXCEPTIONARRAY);
        result->rtype = &stvoid;
        result->data.emember = memberRef;
        return result;
    }

    if (nameResult->objects) {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '<' && (result = make_member_function_esetconst(nameResult))) {
            if (result->type != ENEWEXCEPTION)
                CError_FATAL(1465);
            for (overload = result->data.objlist.list; overload; overload = overload->next) {
                if (overload->object->otype == OT_OBJECT && OBJECT(overload->object)->type->type == TYPEFUNC &&
                    (TYPE_FUNC(OBJECT(overload->object)->type)->flags & FUNC_METHOD) &&
                    !TYPE_METHOD(OBJECT(overload->object)->type)->is_static) {
                    memberRef = CompilerTools_AllocatePool(sizeof(EMemberInfo));
                    memclrw(memberRef, sizeof(EMemberInfo));
                    memberRef->path = nameResult->basePath;
                    memberRef->expr = expr;
                    memberRef->list = result->data.objlist.list;
                    memberRef->templargs = result->data.objlist.templargs;
                    memberRef->is_qualified = nameResult->is_qualified;
                    memberRef->isambig = nameResult->isambig;
                    result = CExpr_NewENode(ENEWEXCEPTIONARRAY);
                    result->rtype = &stvoid;
                    result->data.emember = memberRef;
                    return result;
                }
            }
            return result;
        }
        for (overload = nameResult->objects; overload; overload = overload->next) {
            if (overload->object->otype == OT_OBJECT && OBJECT(overload->object)->type->type == TYPEFUNC &&
                (TYPE_FUNC(OBJECT(overload->object)->type)->flags & FUNC_METHOD) &&
                !TYPE_METHOD(OBJECT(overload->object)->type)->is_static) {
                memberRef = CompilerTools_AllocatePool(sizeof(EMemberInfo));
                memclrw(memberRef, sizeof(EMemberInfo));
                memberRef->path = nameResult->basePath;
                memberRef->expr = expr;
                memberRef->list = nameResult->objects;
                memberRef->is_qualified = nameResult->is_qualified;
                memberRef->isambig = nameResult->isambig;
                result = CExpr_NewENode(ENEWEXCEPTIONARRAY);
                result->rtype = &stvoid;
                result->data.emember = memberRef;
                return result;
            }
        }
        if (tk == '<' && (result = make_member_function_esetconst(nameResult)))
            return result;
        result = CExpr_NewENode(ENEWEXCEPTION);
        result->rtype = OBJECT(nameResult->objects->object)->type;
        result->data.objlist.list = nameResult->objects;
        if (tk == '(' && copts.cplusplus && allowFunctionCall && !nameResult->is_qualified &&
            nameResult->objects->object->otype == OT_OBJECT)
            result->data.objlist.name = OBJECT(nameResult->objects->object)->name;
        return result;
    }

    if (nameResult->name) {
        Object *function;

        if (copts.cplusplus && allowFunctionCall) {
            if (CPrepTokenizer_GetNextTokenAndRestorePosition() == '(') {
                result = CExpr_NewENode(ENEWEXCEPTION);
                result->rtype = &stvoid;
                result->data.objlist.name = nameResult->name;
                tk = CPrepTokenizer_GetNextToken();
                return result;
            }
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, nameResult->name->name);
            tk = CPrepTokenizer_GetNextToken();
            return nullnode();
        }
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() != '(') {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, nameResult->name->name);
            tk = CPrepTokenizer_GetNextToken();
            return nullnode();
        }
        if (copts.checkprotos)
            CError_ReportError(ERR_FUNCTION_NO_PROTOTYPE);
        function = CParser_NewFunctionObject(NULL);
        function->name = nameResult->name;
        function->sclass = TK_EXTERN;
        {
            TypeFunc *functionType = galloc(sizeof(TypeFunc));
            memclrw(functionType, sizeof(TypeFunc));
            functionType->type = TYPEFUNC;
            functionType->functype = (Type *)&stsignedint;
            functionType->args = &data_00584748;
            fn_00504240(functionType);
            function->type = (Type *)functionType;
        }
        CScope_AddGlobalObject(function);
        tk = CPrepTokenizer_GetNextToken();
        return create_objectrefnode(function);
    }

    CError_FATAL(1586);
    return NULL;
}

ENode *CExpr_MakeNameLookupResultExpr(NameResult *p)
{
    if (p->object != NULL) {
        switch (p->object->otype) {
            case OT_OBJECT: {
                Object *object = (Object *)p->object;
                CClass_CheckObjectAccess(p->basePath, object);
                return create_objectnode((Object *)p->object);
            }
            case OT_ENUMCONST:
                CClass_CheckEnumAccess(p->basePath, p->object);
                {
                    ENode *node = (ENode *)CompilerTools_AllocatePool(0x1a);
                    node->type = EINTCONST;
                    node->cost = 0;
                    node->flags = 0;
                    node->rtype = ((ObjEnumConst *)p->object)->type;
                    node->data.intval = ((ObjEnumConst *)p->object)->val;
                    return node;
                }
            case OT_MEMBERVAR:
                CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
                return nullnode();
            default:
                CError_FATAL(1220);
                break;
        }
    }
    if (p->objects != NULL) {
        ENode *node = CExpr_NewENode(0x43);
        node->rtype = ((Object *)p->objects->object)->type;
        node->data.objlist.list = p->objects;
        return node;
    }
    CError_FATAL(1230);
    return NULL;
}

ENode *make_member_function_esetconst(NameResult *candidates)
{
    Type *functionType;
    NameSpaceObjectList *candidate;
    Object *listObject;
    Boolean listMatches;
    NameSpaceObjectList *first;
    NameSpaceObjectList *tail;
    Object *nextObject;
    Boolean nextMatches;
    ENode *expr;

    if (candidates->object != NULL) {
        if (candidates->object->otype != OT_OBJECT || !CExpr_IsMemberFunction(candidates))
            return NULL;
        candidate = (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
        memclrw(candidate, sizeof(*candidate));
        candidate->object = candidates->object;
    } else {
        if (candidates->objects != NULL) {
            candidate = candidates->objects;
            while (candidate != NULL) {
                if (candidate->object->otype == OT_OBJECT) {
                    listObject = OBJECT(candidate->object);
                    listMatches = OBJECT(candidate->object)->type->type == TYPEFUNC &&
                                  (((TypeMemberFunc *)listObject->type)->flags & 1024) != 0;
                    if (listMatches) {
                        tail = (NameSpaceObjectList *)galloc(sizeof(NameSpaceObjectList));
                        first = tail;
                        goto copy_candidate;
                        do {
                            if (candidate->object->otype == OT_OBJECT) {
                                nextObject = OBJECT(candidate->object);
                                nextMatches = (functionType = OBJECT(candidate->object)->type)->type == TYPEFUNC &&
                                              (((TypeMemberFunc *)nextObject->type)->flags & 1024) != 0;
                                if (nextMatches) {
                                    tail = tail->next = (NameSpaceObjectList *)galloc(sizeof(NameSpaceObjectList));
                                copy_candidate:
                                    *tail = *candidate;
                                    tail->next = NULL;
                                }
                            }
                            candidate = candidate->next;
                        } while (candidate != NULL);
                        candidate = first;
                        break;
                    }
                }
                candidate = candidate->next;
            }
            if (candidate == NULL)
                return NULL;
        } else {
            return NULL;
        }
    }
    expr = CExpr_NewENode(ENEWEXCEPTION);
    expr->rtype = OBJECT(candidate->object)->type;
    expr->data.objlist.list = candidate;
    expr->data.objlist.templargs = CTemplateNew_ParseTemplateArguments(NULL, 0);
    tk = (short)CPrepTokenizer_GetNextToken();
    return expr;
}

ENode *scan_explicit_conversion(Type *type, SInt32 qualifiers)
{
    ENodeList *arguments;
    ENodeList *argument;
    ENode *result;

    if (type->type == TYPECLASS && (TYPE_CLASS(type)->flags & Q_VIRTUAL) != 0 &&
        CParser_CheckTemplateClassScope(type) == 0)
        type = (Type *)&stsignedint;

    if (tk == '(')
        tk = (UInt16)CPrepTokenizer_GetNextToken();
    else
        CError_ReportError(ERR_LPAREN_EXPECTED);

    arguments = CExpr_ScanExpressionList(1);

    if (tk != ')') {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return nullnode();
    }
    tk = (UInt16)CPrepTokenizer_GetNextToken();

    if (CTemplateTools_IsDependentType(type)) {
        ENode *node = CExpr_NewTemplDepENode(TDE_CAST);
        node->data.templdep.u.cast.args = arguments;
        node->data.templdep.u.cast.type = type;
        node->data.templdep.u.cast.qual = qualifiers;
        return node;
    }

    argument = arguments;
    while (argument != NULL) {
        if (CTemplTool_IsTypeDepExpr(argument->node)) {
            ENode *node = CExpr_NewTemplDepENode(TDE_CAST);
            node->data.templdep.u.cast.args = arguments;
            node->data.templdep.u.cast.type = type;
            node->data.templdep.u.cast.qual = qualifiers;
            return node;
        }
        argument = argument->next;
    }

    if (type->type != TYPECLASS) {
        if (arguments == NULL) {
            result = do_typecast(nullnode(), type, qualifiers);
        } else {
            ENode *node;
            if (arguments->next != NULL)
                CError_ReportError(ERR_MORE_THAN_ONE_EXPRESSION_NON_CLASS);
            node = CExpr_RewriteConst(pointer_generation(arguments->node));
            result = do_typecast(node, type, qualifiers);
        }
    } else {
        CDecl_CompleteType(type);
        if ((TYPE_CLASS(type)->flags & CLASS_COMPLETED) == 0)
            CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, type, 0);
        CDecl_CheckObjectType(type);
        result = CExpr_ConstructObject(type, create_temp_node(type), arguments, 1, 1, 1, 1, 1);
    }
    return result;
}

ENode *CExpr_DoExplicitConversion(Type *classType, unsigned long qualifiers, ENodeList *arguments)
{
    ENode *node;
    if (classType->type != TYPECLASS) {
        if (arguments == NULL) {
            return do_typecast(nullnode(), classType, qualifiers);
        }
        if (arguments->next != NULL) {
            CError_ReportError(ERR_MORE_THAN_ONE_EXPRESSION_NON_CLASS);
        }
        node = CExpr_RewriteConst((ENode *)pointer_generation(arguments->node));
        return do_typecast(node, classType, qualifiers);
    }
    CDecl_CompleteType((Type *)classType);
    if ((((TypeClass *)classType)->flags & CLASS_COMPLETED) == 0) {
        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, classType, 0);
    }
    CDecl_CheckObjectType(classType);
    return CExpr_ConstructObject(classType, create_temp_node(classType), arguments, 1, 1, 1, 1, 1);
}

ENodeList *CExpr_ScanExpressionList(char parenthesized)
{
    ENodeList *head;
    ENode *expression;
    ENode *converted;
    ENodeList *next;
    ENodeList *current;

    if ((parenthesized != '\0') && (tk == ')')) {
        return NULL;
    }
    current = (ENodeList *)CompilerTools_AllocatePool(8);
    head = current;
    while (1) {
        current->next = NULL;
        expression = assignment_expression();
        current->node = expression;
        if (current->node->type != ENEWEXCEPTIONARRAY) {
            converted = pointer_generation(current->node);
            converted = CExpr_RewriteConst(converted);
            current->node = converted;
        }
        if (parenthesized != '\0') {
            if (tk == ')')
                break;
        } else if (tk == ']')
            break;
        if (tk != ',') {
            CError_ReportErrorAndUpdateToken(ERR_COMMA_EXPECTED);
            break;
        }
        tk = CPrepTokenizer_GetNextToken();
        next = (ENodeList *)CompilerTools_AllocatePool(8);
        current->next = next;
        current = current->next;
    }
    return head;
}

ENode *classargument(ENode *node)
{
    if (CClass_CopyConstructor(TYPE_CLASS(node->rtype))) {
        ENodeList *list = (ENodeList *)CompilerTools_AllocatePool(8);
        list->next = NULL;
        list->node = node;
        return CExpr_ConstructObject(node->rtype, create_temp_node(node->rtype), list, 1, 1, 1, 1, 0);
    }
    return node;
}

enum { TYPECLASS_004f9ed0 = 5 };

ENode *CExpr_AssignmentPromotion(ENode *expression, Type *type, unsigned short qualifiers, int mode)
{
    ENode *converted;
    ENodeList *arguments;
    ENode *result;
    ENode *cast;
    ENode *address;
    ENode *typeCopy;
    ENode *pointerType;
    if (type->type == TYPECLASS_004f9ed0 && CClass_ReferenceArgument(TYPE_CLASS(type)) != 0) {
        converted = CExpr_IsTempConstruction(expression, type, NULL);
        if (converted != NULL) {
            return converted;
        }
        arguments = (ENodeList *)CompilerTools_AllocatePool(8);
        arguments->next = NULL;
        arguments->node = expression;
        return getnodeaddress(CExpr_ConstructObject(type, create_temp_node(type), arguments, 1, 1, 1, 1, 0), 0);
    }
    if (CMach_PassResultInHiddenArg(type) != 0) {
        expression = oldassignmentpromotion(expression, type, qualifiers, 0);
        pointerType = CExpr2_NewESCOPEBEGINNode(type, 1);
        typeCopy = (ENode *)CompilerTools_AllocatePool(26);
        *typeCopy = *pointerType;
        address = makemonadicnode(pointerType, 4);
        address->rtype = type;
        cast = makediadicnode(address, expression, 30);
        result = makediadicnode(cast, typeCopy, 41);
        result->rtype = typeCopy->rtype;
        return result;
    }
    return oldassignmentpromotion(expression, type, qualifiers, mode);
}

ENode *oldassignmentpromotion(ENode *e, Type *t, SInt16 sz, SInt32 flag)
{
    Boolean isRef = 0;
    UInt32 size;

    if (t->type != TYPEMEMBERPOINTER) {
        if (t->type == TYPEPOINTER && (TYPE_POINTER(t)->qual & Q_REFERENCE)) {
            if (e->type == ECOND)
                e = CExpr_LValue(e, 0, 0);
            e = pointer_generation(e);
            size = CParser_GetCVTypeQualifiers(e->rtype, e->flags);
            isRef = 1;
        } else {
            e = CExpr_RewriteConst(pointer_generation(e));
        }
    }

    if (e->type == ENEWEXCEPTIONARRAY)
        e = getpointertomemberfunc(e, t, 1);

    if (!isRef)
        check_implicit_pointer_qual_conversion(e, t, sz);
    else
        check_implicit_pointer_qual_conversion(e, TYPE_POINTER(t)->target, sz);

    if (assign_check(e, t, sz, 1, 0, flag) == 0)
        return e;

    if (isRef) {
        if (data_0058850e != 0) {
            t = TYPE_POINTER(t)->target;
            switch ((SInt8)t->type) {
                case TYPEPOINTER:
                    sz = TYPE_POINTER(t)->qual;
                    break;
            }
            if ((sz & 1) == 0)
                CError_Warning(ERR_NON_CONST_AMPERSAND_REFERENCE_INITIALIZED_TEMPORARY);
        } else if (size != 0 && size > CParser_GetCVTypeQualifiers(TYPE_POINTER(t)->target, sz)) {
            CError_Warning(ERR_ILLEGAL_CONST_VOLATILE_AMPERSAND_REFERENCE_INITIALIZATION);
        }
    }
    return converted_expr;
}

void check_implicit_pointer_qual_conversion(ENode *expr, Type *destType, SInt16 destQual)
{
    Type *sourceType = expr->rtype;
    Type *originalType = sourceType;
    UInt16 sourceQual, originalFlags;
    Type *targetType;
    Type *loopTarget;

    if (destType->type == TYPEPOINTER && sourceType->type == TYPEPOINTER) {
        sourceQual = originalFlags = expr->flags;
        if ((targetType = TYPE_POINTER(destType)->target) == &stvoid) {
            sourceQual = applyqual(sourceType, sourceQual);
        } else {
            loopTarget = targetType;
            sourceType = TYPE_POINTER(sourceType)->target;
            while (loopTarget->type == TYPEPOINTER && sourceType->type == TYPEPOINTER) {
                if (CExpr_QualMismatch(TYPE_POINTER(sourceType)->qual, TYPE_POINTER(loopTarget)->qual)) {
                    CError_Warning(ERR_ILLEGAL_IMPLICIT_CONST_VOLATILE_POINTER_CONVERSION, originalType,
                                   originalFlags & 3, destType, destQual);
                    return;
                }
                loopTarget = TYPE_POINTER(loopTarget)->target;
                sourceType = TYPE_POINTER(sourceType)->target;
            }
        }
        if (CExpr_QualMismatch((SInt16)sourceQual, destQual)) {
            CError_Warning(ERR_ILLEGAL_IMPLICIT_CONST_VOLATILE_POINTER_CONVERSION, expr->rtype, expr->flags & 3,
                           destType, destQual);
        }
    }
}

ENode *CExpr_PointerGeneration(ENode *node)
{
    switch ((unsigned char)node->type) {
        case TYPESTRUCT:
            switch ((signed char)node->rtype->type) {
                case TYPEARRAY:
                    node->data.monadic->rtype = (Type *)CDecl_NewPointerType(((TypeMemberPointer *)node->rtype)->ty1);
                    return node->data.monadic;
                case TYPEFUNC:
                    return node->data.monadic;
            }
            break;
    }
    return node;
}

ENode *CExpr_GeneratePointerAndRewriteConst(ENode *expr)
{
    ENode *converted;

    converted = pointer_generation(expr);
    return CExpr_RewriteConst(converted);
}

ENode *pointer_generation(ENode *node)
{
    ENode *result;
    switch (node->type) {
        case EINDIRECT:
            switch ((char)node->rtype->type) {
                case TYPEARRAY:
                    switch (node->data.monadic->type) {
                        case EPOSTINC:
                        case EPOSTDEC:
                        case EPREINC:
                        case EPREDEC:
                            node->type = ETYPCON;
                            node->rtype = CDecl_NewPointerType(TPTR_TARGET(node->rtype));
                            result = node;
                            return result;
                        default:
                            node->data.monadic->rtype = CDecl_NewPointerType(TPTR_TARGET(node->rtype));
                            node->data.monadic->flags = node->flags;
                            result = node->data.monadic;
                            return result;
                    }
                case TYPEFUNC:
                    node = node->data.monadic;
                    if (node->type == EOBJREF && node->data.objref->datatype == DINLINEFUNC)
                        CError_ReportError(ERR_ILLEGAL_USE_INLINE_FUNCTION);
                    return node;
            }
    }
    return node;
}

ENode *checkreference(ENode *e)
{
    if (!(IS_TYPE_POINTER_ONLY(e->rtype) && (TYPE_POINTER(e->rtype)->qual & Q_REFERENCE)))
        return e;
    e = makemonadicnode(e, EINDIRECT);
    e->rtype = TPTR_TARGET(e->rtype);
    return e;
}

ENode *CExpr_New_ESUB_Node(ENode *left, ENode *right)
{
    if (right->type == EINTCONST && !Type_IsUnsigned(right->rtype) && left->rtype->type != TYPEFLOAT) {
        right->data.intval = CInt64_Neg(right->data.intval);
        return CExpr_New_EADD_Node(left, right);
    }
    if ((SInt8)left->rtype->type >= TYPEPOINTER) {
        return make_pointer_subtraction(left, right);
    }
    CExpr_ArithmeticConversion(&left, &right);
    if (CExpr2_IsZero(right)) {
        return left;
    }
    if (CExpr2_IsZero(left)) {
        if (right->type == EINTCONST) {
            right->data.intval = CInt64_Neg(right->data.intval);
            return right;
        }
        if (right->type == EFLOATCONST) {
            right->data.floatval = CMach_CalcFloatMonadic(right->rtype, '-', right->data.floatval.data.value);
            return right;
        }
        return CExpr2_ReturnENode(makemonadicnode(right, EMONMIN));
    }
    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, '-', right->data.intval);
        return left;
    }
    if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        left->data.floatval = CMach_CalcFloatDiadic(left->rtype, left->data.floatval, '-', right->data.floatval);
        return left;
    }
    if (right->type == EINTCONST) {
        if (add_to_expression_constant(left, CInt64_Neg(right->data.intval))) {
            return left;
        }
    }
    left = makediadicnode(left, right, ESUB);
    if (left->rtype->type == TYPEFLOAT) {
        left = CExpr2_ReturnNode(left);
    }
    return left;
}

/* View of an ENode whose 8-byte constant payload is a Float: the inline
 * constant stores its double by value at offset 0x0a. */

ENode *CExpr_New_EADD_Node(ENode *left, ENode *right)
{
    if ((SInt8)left->rtype->type >= TYPEPOINTER)
        return add_pointer_offset(left, right);
    if ((SInt8)right->rtype->type >= TYPEPOINTER)
        return add_pointer_offset(right, left);
    CExpr_ArithmeticConversion(&left, &right);
    if (CExpr2_IsZero(right))
        return left;
    if (CExpr2_IsZero(left))
        return right;
    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, '+', right->data.intval);
        return left;
    }
    if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        left->data.floatval = CMach_CalcFloatDiadic(left->rtype, left->data.floatval, '+', right->data.floatval);
        return left;
    }
    if (left->type == EINTCONST) {
        if (add_to_expression_constant(right, left->data.intval))
            return right;
    }
    if (right->type == EINTCONST) {
        if (add_to_expression_constant(left, right->data.intval))
            return left;
    }
    left = makediadicnode(left, right, EADD);
    optimizecomm(left);
    if (left->rtype->type == TYPEFLOAT)
        left = CExpr2_ReturnNode(left);
    return left;
}

ENode *make_pointer_subtraction(ENode *left, ENode *right)
{
    SInt32 elementSize;
    ENode *result;
    Type *targetType;

    if ((SInt8)right->rtype->type >= TYPEPOINTER) {
        elementSize = TPTR_TARGET(left->rtype)->size;
        if (elementSize == 0) {
            CDecl_CompleteType(TPTR_TARGET(left->rtype));
            elementSize = TPTR_TARGET(left->rtype)->size;
            if (elementSize == 0) {
                CError_ReportError(ERR_ILLEGAL_TYPE);
                return left;
            }
        }
        if (is_typesame(left->rtype, right->rtype) == 0) {
            CError_ReportError(ERR_TYPE_MISMATCH, left->rtype, left->flags & Q_CV, right->rtype, right->flags & Q_CV);
            return left;
        }
        if (left->type == ETYPCON && left->data.diadic.left->type == EINTCONST && right->type == ETYPCON &&
            right->data.diadic.left->type == EINTCONST) {
            left->data.diadic.left->rtype = right->data.diadic.left->rtype = CABI_GetPtrDiffTType();
            result = CExpr_New_ESUB_Node(left->data.diadic.left, right->data.diadic.left);
            if (elementSize > 1)
                result = CExpr_New_EDIV_Node(result, intconstnode(CABI_GetPtrDiffTType(), elementSize), EDIV);
            return result;
        }
        result = makediadicnode(left, right, ESUB);
        result->rtype = CABI_GetPtrDiffTType();
        if (elementSize > 1)
            result = makediadicnode(result, intconstnode(CABI_GetPtrDiffTType(), elementSize), EDIV);
        return result;
    }
    right = convert_to_stunsignedlong_size(right);
    targetType = TPTR_TARGET(left->rtype);
    elementSize = targetType->size;
    if (elementSize == 0) {
        CDecl_CompleteType(targetType);
        elementSize = targetType->size;
        if (elementSize == 0) {
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return left;
        }
    }
    right = CExpr_New_EMUL_Node(right, intconstnode(CABI_GetPtrDiffTType(), elementSize));
    if (right->type == EINTCONST) {
        if (add_to_expression_constant(left, CInt64_Neg(right->data.intval)) != 0)
            return left;
    }
    right = makediadicnode(left, right, ESUB);
    right->rtype = left->rtype;
    right->flags = left->flags;
    return right;
}

ENode *add_pointer_offset(ENode *node, ENode *arg)
{
    ENode *combined;
    Type *target;
    ENode *sizeconst;
    SInt32 size;
    ENode *folded;
    ENode *newnode;

    arg = convert_to_stunsignedlong_size(arg);
    target = TPTR_TARGET(node->rtype);
    size = target->size;
    if (size == 0) {
        CDecl_CompleteType(target);
        size = target->size;
        if (size == 0) {
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return node;
        }
    }
    sizeconst = intconstnode((Type *)(size > 0x7fff ? &stsignedlong : &stsignedint), size);
    combined = CExpr_New_EMUL_Node(arg, sizeconst);
    if (combined->type == EINTCONST) {
        if (add_to_expression_constant(node, combined->data.intval) != 0)
            return node;
    }
    if (node->type == EADD && node->data.diadic.right->type == EINTCONST) {
        node->data.diadic.left = makediadicnode(node->data.diadic.left, combined, EADD);
        return node;
    }
    newnode = makediadicnode(node, combined, EADD);
    newnode->rtype = node->rtype;
    newnode->flags = node->flags;
    return newnode;
}

ENode *convert_to_stunsignedlong_size(ENode *node)
{
    Type *targettype;
    Boolean isunsigned;
    Boolean isunsigned2;

    if (node->rtype->type != TYPEINT) {
        node = CExpr_ConvertToIntegral(node);
    }
    if (node->rtype->size != stunsignedlong.size) {
        targettype = (Type *)&stunsignedlong;
        isunsigned = Type_IsUnsigned(node->rtype);
        isunsigned2 = Type_IsUnsigned((Type *)&stunsignedlong);
        if (isunsigned2 != isunsigned) {
            if (isunsigned) {
                if (TypSize(&stunsignedlong) == stunsignedlong.size) {
                    targettype = (Type *)&stunsignedlong;
                } else if (stunsignedint.size == stunsignedlong.size) {
                    targettype = (Type *)&stunsignedint;
                } else {
                    CError_FATAL(443);
                }
            } else {
                if (stsignedlong.size == stunsignedlong.size) {
                    targettype = (Type *)&stsignedlong;
                } else if (stsignedint.size == stunsignedlong.size) {
                    targettype = (Type *)&stsignedint;
                } else {
                    CError_FATAL(449);
                }
            }
        }
        if (node->type == EINTCONST) {
            node->data.intval = CExpr_IntConstConvert(targettype, node->rtype, node->data.intval);
        } else {
            node = makemonadicnode(node, ETYPCON);
        }
        node->rtype = targettype;
    }
    return node;
}

SInt16 canadd(ENode *p, UInt32 n)
{
    CInt64 v;
    v.lo = n;
    v.hi = (SInt32)n < 0 ? -1 : 0;
    return add_to_expression_constant(p, v);
}

SInt16 add_to_expression_constant(ENode *node, CInt64 v)
{
    Boolean flag = (v.hi == 0) && (v.lo == 0);

    if (flag)
        return 1;

    switch (node->type) {
        case EINTCONST:
            node->data.intval = CMach_CalcIntDiadic(node->rtype, node->data.intval, 0x2b, v);
            return 1;

        case EFLOATCONST: {
            Type *type = (Type *)&stsignedlong;
            Float t = CMach_CalcFloatConvertFromInt(type, v);
            node->data.floatval = CMach_CalcFloatDiadic(node->rtype, node->data.floatval, 0x2b, t);
            return 1;
        }

        case EADD:
            if (add_to_expression_constant(node->data.diadic.left, v))
                return 1;
            if (add_to_expression_constant(node->data.diadic.right, v))
                return 1;
            return 0;

        case ESUB:
            if (add_to_expression_constant(node->data.diadic.left, v))
                return 1;
            if (add_to_expression_constant(node->data.diadic.right, CInt64_Neg(v)))
                return 1;
            return 0;

        case ETYPCON:
            if (node->rtype->type == TYPEPOINTER && node->data.monadic->type == EINTCONST) {
                node->data.monadic->data.intval =
                    CMach_CalcIntDiadic((Type *)&stunsignedlong, node->data.monadic->data.intval, 0x2b, v);
                return 1;
            }
            return 0;
    }
    return 0;
}

ENode *CExpr_New_EDIV_Node(ENode *left, ENode *right, Boolean flag)
{
    ENode *n;

    CExpr_ArithmeticConversion(&left, &right);
    if (CExpr2_IsZero(right) && right->rtype->type == TYPEINT) {
        if (!flag)
            CError_Warning(ERR_DIVISION_BY_0);
        return right;
    }
    if (CExpr_IsOne(right))
        return left;
    if (CExpr2_IsZero(left) && left->rtype->type == TYPEINT) {
        ENode *r = right;
        if (r->rtype->type == TYPEFLOAT) {
            n = intconstnode((Type *)&stsignedint, 0);
            n->type = EFLOATCONST;
            n->data.floatval = CMach_CalcFloatConvertFromInt((Type *)&stsignedint, n->data.intval);
            n->rtype = r->rtype;
        } else {
            n = intconstnode(r->rtype, 0);
        }
        return CInline_00513910(r) ? makediadicnode(r, n, ECOMMA) : n;
    }
    if (left->type == EINTCONST && right->type == EINTCONST) {
        left->data.intval = CMach_CalcIntDiadic(left->rtype, left->data.intval, 0x2f, right->data.intval);
        return left;
    }
    if (left->type == EFLOATCONST && right->type == EFLOATCONST) {
        left->data.floatval = CMach_CalcFloatDiadic(left->rtype, left->data.floatval, 0x2f, right->data.floatval);
        return left;
    }
    left = makediadicnode(left, right, EDIV);
    if (left->rtype->type == TYPEFLOAT)
        left = CExpr2_ReturnNode(left);
    return left;
}

ENode *CExpr_New_EMUL_Node(ENode *lhs, ENode *rhs)
{
    ENode *x, *n;
    CExpr_ArithmeticConversion(&lhs, &rhs);
    if (CExpr2_IsZero(lhs)) {
        x = rhs;
        if (x->rtype->type == TYPEFLOAT) {
            n = intconstnode((Type *)&stsignedint, 0);
            n->type = EFLOATCONST;
            n->data.floatval = CMach_CalcFloatConvertFromInt((Type *)&stsignedint, n->data.intval);
            n->rtype = x->rtype;
        } else {
            n = intconstnode(x->rtype, 0);
        }
        return CInline_00513910(x) ? makediadicnode(x, n, ECOMMA) : n;
    }
    if (CExpr2_IsZero(rhs)) {
        x = lhs;
        if (x->rtype->type == TYPEFLOAT) {
            n = intconstnode((Type *)&stsignedint, 0);
            n->type = EFLOATCONST;
            n->data.floatval = CMach_CalcFloatConvertFromInt((Type *)&stsignedint, n->data.intval);
            n->rtype = x->rtype;
        } else {
            n = intconstnode(x->rtype, 0);
        }
        return CInline_00513910(x) ? makediadicnode(x, n, ECOMMA) : n;
    }
    if (CExpr_IsOne(rhs))
        return lhs;
    if (CExpr_IsOne(lhs))
        return rhs;
    if (lhs->type == EINTCONST && rhs->type == EINTCONST) {
        lhs->data.intval = CMach_CalcIntDiadic(lhs->rtype, lhs->data.intval, 0x2a, rhs->data.intval);
        return lhs;
    }
    if (lhs->type == EFLOATCONST && rhs->type == EFLOATCONST) {
        lhs->data.floatval = CMach_CalcFloatDiadic(lhs->rtype, lhs->data.floatval, 0x2a, rhs->data.floatval);
        return lhs;
    }
    lhs = makediadicnode(lhs, rhs, EMUL);
    optimizecomm(lhs);
    if (lhs->rtype->type == TYPEINT && lhs->rtype->size > 2) {
        lhs->cost++;
        if (lhs->cost > 200)
            lhs->cost = 200;
    }
    if (lhs->rtype->type == TYPEFLOAT)
        lhs = CExpr2_ReturnNode(lhs);
    return lhs;
}

void unify_arithmetic_rtypes(ENode **leftp, ENode **rightp, SInt32 unused)
{
    ENode *left, *right;
    CInt64 convertedRight, restoredLeft, convertedLeft, restoredRight;

    left = *leftp;
    right = *rightp;

    switch ((SInt8)left->rtype->type) {
        case TYPEINT:
            break;
        case TYPEFLOAT:
            if (left->rtype == right->rtype)
                return;
            CExpr_ArithmeticConversion(leftp, rightp);
            return;
        case TYPEENUM:
            left->rtype = TYPE_ENUM(left->rtype)->enumtype;
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            left = nullnode();
    }

    switch ((SInt8)right->rtype->type) {
        case TYPEINT:
            break;
        case TYPEFLOAT:
            CExpr_ArithmeticConversion(leftp, rightp);
            return;
        case TYPEENUM:
            right->rtype = TYPE_ENUM(right->rtype)->enumtype;
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            right = nullnode();
    }

    if (left->rtype == right->rtype) {
        *leftp = left;
        *rightp = right;
        return;
    }
    if (left->rtype->size == right->rtype->size) {
        if (Type_IsUnsigned(left->rtype) == Type_IsUnsigned(right->rtype)) {
            left->rtype = right->rtype;
            *leftp = left;
            *rightp = right;
            return;
        }
    } else {
        if (right->type == EINTCONST && left->rtype->size <= right->rtype->size) {
            if (Type_IsUnsigned(left->rtype) == Type_IsUnsigned(right->rtype) || Type_IsUnsigned(left->rtype)) {
                convertedRight = CMach_CalcIntDiadic(left->rtype, right->data.intval, '+', cint64_zero);
                restoredRight = CMach_CalcIntDiadic(right->rtype, convertedRight, '+', cint64_zero);
                if (CInt64_Equal(restoredRight, right->data.intval)) {
                    right->rtype = left->rtype;
                    *leftp = left;
                    *rightp = right;
                    return;
                }
            }
        }
        if (left->type == EINTCONST && left->rtype->size >= right->rtype->size) {
            if (Type_IsUnsigned(left->rtype) == Type_IsUnsigned(right->rtype) || Type_IsUnsigned(right->rtype)) {
                convertedLeft = CMach_CalcIntDiadic(right->rtype, left->data.intval, '+', cint64_zero);
                restoredLeft = CMach_CalcIntDiadic(left->rtype, convertedLeft, '+', cint64_zero);
                if (CInt64_Equal(restoredLeft, left->data.intval)) {
                    left->rtype = right->rtype;
                    *leftp = left;
                    *rightp = right;
                    return;
                }
            }
        }
    }
    *leftp = left;
    *rightp = right;
    CExpr_ArithmeticConversion(leftp, rightp);
}

void CExpr_004fb400(ENode *e)
{
    switch ((SInt8)e->rtype->type) {
        case TYPEINT:
            if (e->rtype == (Type *)&stbool)
                break;
        case TYPEFLOAT:
            return;
        case TYPEENUM:
            if (copts.cplusplus == 0)
                return;
            break;
        case TYPEPOINTER:
            if (TYPE_POINTER(e->rtype)->target->size == 0)
                CDecl_CompleteType(TYPE_POINTER(e->rtype)->target);
            if (TYPE_POINTER(e->rtype)->target->size != 0)
                return;
            break;
        case TYPEARRAY:
            if (e->type == EOBJREF)
                return;
            break;
    }
    CError_ReportError(ERR_ILLEGAL_OPERAND);
}

void optimizecomm(ENode *expression)
{
    ENode *operand;

    if (expression->data.diadic.right->type == '2')
        return;
    if (expression->data.diadic.left->type == '2') {
    swap:
        operand = expression->data.diadic.right;
        expression->data.diadic.right = expression->data.diadic.left;
        expression->data.diadic.left = operand;
        return;
    }
    if (expression->data.diadic.left->type == '3')
        return;
    if (expression->data.diadic.right->type != '3') {
        if ('\x02' < (signed char)expression->rtype->type)
            return;
        if (expression->data.diadic.left->cost <= expression->data.diadic.right->cost)
            return;
    }
    goto swap;
}

ENode *CExpr_RewriteConst(ENode *enode)
{
    Object *obj;

    for (;;) {
        if (enode->type != EINDIRECT)
            break;
        if (enode->data.monadic->type != EOBJREF)
            break;
        obj = enode->data.monadic->data.objref;
        if (obj->datatype == DALIAS) {
            CExpr_AliasTransform(enode->data.monadic);
            continue;
        }
        if ((obj->qual & Q_INLINE_DATA) != 0 && enode->rtype == obj->type) {
            switch ((SInt8)enode->rtype->type) {
                case TYPEINT:
                case TYPEENUM:
                    enode->type = EINTCONST;
                    enode->data.intval = obj->u.data.u.intconst;
                    break;
                case TYPEPOINTER:
                    enode->type = EINTCONST;
                    enode->data.intval = obj->u.data.u.intconst;
                    enode->rtype = (Type *)&stunsignedlong;
                    enode = makemonadicnode(enode, ETYPCON);
                    enode->rtype = obj->type;
                    break;
                case TYPEFLOAT:
                    enode->type = EFLOATCONST;
                    if (obj->u.data.u.string != NULL)
                        enode->data.intval = *(CInt64 *)obj->u.data.u.string;
                    else
                        enode->data.floatval = CMach_CalcFloatConvertFromInt((Type *)&stsignedlong, cint64_zero);
                    break;
                default:
                    CError_FATAL(94);
                    break;
            }
        }
        break;
    }
    return enode;
}
