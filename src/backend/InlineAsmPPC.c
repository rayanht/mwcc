#define CERROR_FILE "InlineAsmPPC.c"
#include "compiler/common.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "compiler/Exceptions.h"
#include "compiler/FuncLevelAsmPPC.h"
#include "compiler/InlineAsmMnemonicsPPC.h"
#include "compiler/InlineAsmRegisters.h"
#include "compiler/InlineAsmRegistersPPC.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/TOC.h"
#include "compiler/Unmangle.h"
#include "driver/TargetPanels-eabi-ppc.h"
#include "version.h"
#include <string.h>

/* Copies an asm statement and the instruction its expr slot holds. */
typedef enum RegClass { RC_GPR = 0, RC_FPR = 1, RC_SPR = 2, RC_CRFIELD = 3, RC_CRFIELDBIT = 8, RC_VR = 9 } RegClass;

static int InlineAsm_Register(EncodedOperand *operand);

static int InlineAsm_RegisterHi(EncodedOperand *operand);
static void ReplaceArg(EncodedOperand *e, Object *b, Object *q);

static inline int recovery_inline_eval(int arg)
{
    InlineAsmExpression s;
    UInt32 isConst;
    int result;
    parse_expression(&s, arg);
    isConst = (s.object == NULL && s.object_label == NULL) && (s.label == NULL);
    if (!isConst) {
        if (s.object)
            PPCError_ReportError(0x7a, s.object->name->name);
        else if (s.object_label)
            PPCError_ReportError(0x7a, s.object_label->name->name);
        else if (s.label)
            PPCError_ReportError(0xa6, s.label->name->name);
        result = 0;
    } else {
        switch (s.type) {
            case 8:
                result = (short)((s.value >> 16) + ((s.value >> 15) & 1));
                break;
            case 7:
                result = (short)(s.value >> 16);
                break;
            case 6:
                result = (short)s.value;
                break;
            default:
                result = s.value;
                break;
        }
    }
    return result;
}

static inline int recovery_inline_ranged(int lo, int hi)
{
    int result;

    result = recovery_inline_eval(0);
    if (result < lo || result > hi)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
    return result;
}

static inline int recovery_inline_ranged2(int lo, int hi)
{
    int result;

    result = recovery_inline_eval(0);
    if (result < lo || result > hi) {
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
        return result;
    }
    return result;
}

static inline int recovery_inline_eval1(int arg)
{
    return recovery_inline_eval(arg);
}

static inline int recovery_inline_crbit(void)
{
    int value;
    int checked;
    if ((checked = value = recovery_inline_eval(1)) < 0 || checked > 31)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
    return value;
}

static inline void RegisterClassError(const char *name, unsigned char kind)
{
    const char *p;
    switch (kind) {
        case 0:
            p = "GPR";
            break;
        case 1:
            p = "FPR";
            break;
        case 2:
            p = "SPR";
            break;
        case 3:
            p = "CRFIELD";
            break;
        case 8:
            p = "CRFIELDBIT";
            break;
        case 9:
            p = "VR";
            break;
    }
    PPCError_ReportError(0xa7, name, p);
}

static inline int recovery_inline_isreg(int cls)
{
    int valid;
    InlineAsmRegisterEntry *obj;
    int cls_flag;

    cls_flag = FALSE;
    valid = FALSE;
    if ((tk == TK_IDENTIFIER) && (obj = CTemplateNew_GetInlineAsmRegisterEntry(data_00587fa0), obj != NULL)) {
        valid = TRUE;
    }
    if ((valid) && (obj->kind == cls)) {
        cls_flag = TRUE;
    }
    return cls_flag;
}

static inline SInt32 expression_immediate(const InlineAsmExpression *expression)
{
    switch (expression->type) {
        case 8:
            return (short)((expression->value >> 16) + (expression->value >> 15 & 1));
        case 7:
            return (short)(expression->value >> 16);
        case 6:
            return (short)expression->value;
        default:
            return expression->value;
    }
}

static inline int BranchImmediateOperand(EncodedOperand *out, SInt32 lo, SInt32 hi)
{
    SInt32 b;
    InlineAsmExpression r;
    SInt32 value;
    parse_expression(&r, 0);
    b = (r.object == NULL) && (r.object_label == NULL) && (r.label == NULL);
    if (!b) {
        if (r.object != NULL)
            PPCError_ReportError(0x7a, ((Object *)r.object)->name->name);
        else if (r.object_label != NULL)
            PPCError_ReportError(0x7a, ((Object *)r.object_label)->name->name);
        else if (r.label != NULL)
            PPCError_ReportError(0xa6, r.label->name->name);
        value = 0;
    } else {
        switch (r.type) {
            case 8:
                value = (SInt16)((r.value >> 16) + ((r.value >> 15) & 1));
                break;
            case 7:
                value = (SInt16)(r.value >> 16);
                break;
            case 6:
                value = (SInt16)r.value;
                break;
            default:
                value = r.value;
                break;
        }
    }
    if (value < lo || value > hi)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
    out->kind = 1;
    out->data.value = value;
    return value;
}

#pragma sym on

static inline int register_value_zero(void)
{
    InlineAsmExpression s;
    UInt32 isConst;
    int result;
    parse_expression(&s, 0);
    isConst = (s.object == NULL && s.object_label == NULL) && (s.label == NULL);
    if (!isConst) {
        if (s.object)
            PPCError_ReportError(0x7a, ((Object *)s.object)->name->name);
        else if (s.object_label)
            PPCError_ReportError(0x7a, ((Object *)s.object_label)->name->name);
        else if (s.label)
            PPCError_ReportError(0xa6, s.label->name->name);
        result = 0;
    } else {
        switch (s.type) {
            case 8:
                result = (short)((s.value >> 16) + ((s.value >> 15) & 1));
                break;
            case 7:
                result = (short)(s.value >> 16);
                break;
            case 6:
                result = (short)s.value;
                break;
            default:
                result = s.value;
                break;
        }
    }
    return result;
}

#pragma sym reset

static inline int register_value(void)
{
    InlineAsmExpression s;
    UInt32 isConst;
    int value;

    parse_expression(&s, 1);
    isConst = (s.object == NULL && s.object_label == NULL) && (s.label == NULL);
    if (!isConst) {
        if (s.object)
            PPCError_ReportError(0x7a, s.object->name->name);
        else if (s.object_label)
            PPCError_ReportError(0x7a, s.object_label->name->name);
        else if (s.label)
            PPCError_ReportError(0xa6, s.label->name->name);
        value = 0;
    } else {
        switch (s.type) {
            case 8:
                value = (short)((s.value >> 16) + ((s.value >> 15) & 1));
                break;
            case 7:
                value = (short)(s.value >> 16);
                break;
            case 6:
                value = (short)s.value;
                break;
            default:
                value = s.value;
                break;
        }
    }
    return value;
}

static inline void IAExpr_Const(InlineAsmExpression *expr, SInt32 value)
{
    expr->type = 5;
    expr->flags = 0;
    expr->value = value;
    expr->islocal = 0;
    expr->object_label = NULL;
    expr->object = NULL;
    expr->second_object = NULL;
    expr->label = NULL;
    expr->second_label = NULL;
}

static inline int IAExpr_IsConst(InlineAsmExpression *expr)
{
    int noobject = 0;
    int result = 0;
    if (!expr->object && !expr->object_label)
        noobject = 1;
    if (noobject && !expr->label)
        result = 1;
    return result;
}

static inline Object *IA_LookupVariable(void)
{
    struct AsmOperand pr;
    Object *obj;

    if (tk == TK_IDENTIFIER) {
        InlineAsm_ResolveOperandNameDefault(data_00587fa0, &pr);
        if ((obj = pr.object)) {
            ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, 0);
            if (obj->datatype == DLOCAL) {
                if (Registers_GetInfo(obj) ? Registers_GetInfo(obj)->reg : 0)
                    obj = NULL;
                else
                    return obj;
            } else if ((UInt8)(obj->datatype - 3) > 1) {
                if (obj->datatype && !PCodeUtilities_Require(obj))
                    return NULL;
                BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
            }
            return obj;
        }
    }
    return NULL;
}

static inline int IA_IsRegisterClass(InlineAsmRegisterEntry **reg, SInt16 cls)
{
    int ok = 0;
    int found = 0;
    if (tk == TK_IDENTIFIER && (*reg = CTemplateNew_GetInlineAsmRegisterEntry(data_00587fa0)))
        found = 1;
    if (found && (*reg)->kind == cls)
        ok = 1;
    return ok;
}

static inline SInt32 IA_GetRegister(SInt16 cls)
{
    InlineAsmRegisterEntry *reg;
    SInt32 num = 0;
    if (tk == TK_IDENTIFIER && (reg = CTemplateNew_GetInlineAsmRegisterEntry(data_00587fa0)) && reg->kind == cls)
        num = (SInt16)reg->number;
    else
        PPCError_ReportError(0xa7, data_00587fa0->name);
    tk = CPrepTokenizer_GetNextToken();
    return num;
}

void report_binary_expression_error(HashNameNode *name1, HashNameNode *name2, SInt16 kind)
{
    char *p;
    switch (kind) {
        case 42:
            p = "*";
            break;
        case 47:
            p = "/";
            break;
        case 37:
            p = "%";
            break;
        case 43:
            p = "+";
            break;
        case 45:
            p = "-";
            break;
        case 364:
            p = "<<";
            break;
        case 365:
            p = ">>";
            break;
        case 60:
            p = "<";
            break;
        case 62:
            p = ">";
            break;
        case 362:
            p = ">=";
            break;
        case 363:
            p = "<=";
            break;
        case 360:
            p = "==";
            break;
        case 361:
            p = "!=";
            break;
        case 38:
            p = "&";
            break;
        case 94:
            p = "^";
            break;
        case 124:
            p = "|";
            break;
        case 359:
            p = "&&";
            break;
        case 358:
            p = "||";
            break;
        default:
            p = "?\??";
            break;
    }
    if (name2 == NULL) {
        PPCError_ReportError(0x77, p, name1->name);
    } else if (name1 == NULL) {
        PPCError_ReportError(0x78, p, name2->name);
    } else {
        PPCError_ReportError(0x76, name1->name, p, name2->name);
    }
}

void report_register_error(unsigned int value, unsigned char kind)
{
    char *selectedData;
    switch (kind) {
        case 0:
            selectedData = "GPR";
            break;
        case 1:
            selectedData = "FPR";
            break;
        case 2:
            selectedData = "SPR";
            break;
        case 3:
            selectedData = "CRFIELD";
            break;
        case 8:
            selectedData = "CRFIELDBIT";
            break;
        case 9:
            selectedData = "VR";
            break;
    }
    PPCError_ReportError(0xa7, value, selectedData);
}

int parse_constant_in_range(int lowerBound, int upperBound)
{
    InlineAsmExpression constant;
    UInt32 isConstant;
    int result;

    parse_expression(&constant, 0);

    isConstant = (constant.object == NULL && constant.object_label == NULL) && (constant.label == NULL);

    if (!isConstant) {
        Object *symbol = constant.object;
        if (symbol)
            PPCError_ReportError(0x7a, symbol->name->name);
        else if (constant.object_label) {
            Object *secondarySymbol = constant.object_label;
            PPCError_ReportError(0x7a, secondarySymbol->name->name);
        } else if (constant.label) {
            struct CLabel *parameter = constant.label;
            PPCError_ReportError(0xa6, parameter->name->name);
        }
        result = 0;
    } else {
        switch (constant.type) {
            case 8:
                result = (short)((constant.value >> 16) + ((constant.value >> 15) & 1));
                break;
            case 7:
                result = (short)(constant.value >> 16);
                break;
            case 6: {
                SInt16 value = constant.value;
                result = value;
                break;
            }
            default:
                result = constant.value;
                break;
        }
    }

    if (result < lowerBound || result > upperBound)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);

    return result;
}

void parse_unary_expression(InlineAsmExpression *expr, int flag)
{
    UInt8 mode = 0;
    SInt16 islocal = 0;
    Object *label;
    Object *object;
    InlineAsmRegisterEntry *reg;
    SInt32 base;
    SInt32 value;
    SInt16 halfword;
    InlineAsmRegisterEntry *conditionRegister;
    InlineAsmRegisterEntry *conditionBit;
    struct AsmOperand operand;
    InlineAsmExpression offset;
    char registerName[4];

    switch (tk) {
        case TK_IDENTIFIER:
            if ((label = get_struct_or_class_pointer_object())) {
                tk = CPrepTokenizer_GetNextToken();
                IAExpr_Const(expr, InlineAsm_ParseStructOrClassMemberOffset(TPTR_TARGET(label->type)));
                expr->object_label = label;
                return;
            }
            if ((object = IA_LookupVariable())) {
                if (!object->datatype && (object->type->type == TYPEINT || object->type->type == TYPEENUM) &&
                    (object->qual & Q_INLINE_DATA)) {
                    IAExpr_Const(expr, object->u.data.u.intconst.lo);
                } else {
                    IAExpr_Const(expr, 0);
                    expr->object = object;
                    if (object->datatype == DLOCAL)
                        islocal = 1;
                    else
                        islocal = 0;
                }
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '.' || tk == '[') {
                    IAExpr_Const(&offset, InlineAsm_ParseMemberArrayOffset(object->type));
                    evaluate_inline_asm_binary_expression(expr, '+', &offset);
                }
                if (tk == '+') {
                    tk = CPrepTokenizer_GetNextToken();
                    parse_expression(&offset, flag);
                    evaluate_inline_asm_binary_expression(expr, '+', &offset);
                } else if (tk == '-') {
                    tk = CPrepTokenizer_GetNextToken();
                    parse_expression(&offset, flag);
                    evaluate_inline_asm_binary_expression(expr, '-', &offset);
                } else if (!strcmp(data_00587fa0->name, "@loword")) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (low_word_offset) {
                        IAExpr_Const(&offset, low_word_offset);
                        evaluate_inline_asm_binary_expression(expr, '+', &offset);
                    }
                } else if (!strcmp(data_00587fa0->name, "@hiword")) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (high_word_offset) {
                        IAExpr_Const(&offset, high_word_offset);
                        evaluate_inline_asm_binary_expression(expr, '+', &offset);
                    }
                }
                expr->islocal = islocal;
                return;
            }
            if (InlineAsm_ResolveOperandName(data_00587fa0, &operand, 1)) {
                if (operand.is_register) {
                    tk = CPrepTokenizer_GetNextToken();
                    IAExpr_Const(expr, operand.register_number);
                    return;
                }
                if (operand.value && (operand.value->type == TYPESTRUCT || operand.value->type == TYPECLASS)) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk != '.')
                        CError_ReportError(ERR_UNEXPECTED_TOKEN);
                    if (data_005652f8) {
                        IAExpr_Const(expr, InlineAsm_ParseMemberArrayOffset(operand.value));
                        return;
                    }
                    IAExpr_Const(expr, InlineAsm_ParseMemberOffset(operand.value));
                    return;
                }
                if (operand.object && operand.object->datatype == DABSOLUTE) {
                    tk = CPrepTokenizer_GetNextToken();
                    IAExpr_Const(expr, operand.object->u.data.u.intconst.hi);
                    return;
                }
            } else if (flag) {
                if (IA_IsRegisterClass(&conditionRegister, 3)) {
                    value = IA_GetRegister(3);
                    IAExpr_Const(expr, value);
                    return;
                }
                if (IA_IsRegisterClass(&conditionBit, 8)) {
                    value = IA_GetRegister(8);
                    IAExpr_Const(expr, value);
                    return;
                }
                if (strlen(data_00587fa0->name) == 6 && !strncmp(data_00587fa0->name, "cr", 2) &&
                    data_00587fa0->name[3] == '_') {
                    registerName[0] = data_00587fa0->name[0];
                    registerName[1] = data_00587fa0->name[1];
                    registerName[2] = data_00587fa0->name[2];
                    registerName[3] = 0;
                    if ((reg = CTemplateNew_LookupInlineAsmRegister(registerName)) && reg->kind == 3) {
                        base = reg->number << 2;
                        registerName[0] = data_00587fa0->name[4];
                        registerName[1] = data_00587fa0->name[5];
                        registerName[2] = 0;
                        if ((reg = CTemplateNew_LookupInlineAsmRegister(registerName)) && reg->kind == 8) {
                            tk = CPrepTokenizer_GetNextToken();
                            IAExpr_Const(expr, reg->number + base);
                            return;
                        }
                    }
                }
            }
            if (!strcmp("ha16", data_00587fa0->name))
                mode = 8;
            else if (!strcmp("hi16", data_00587fa0->name))
                mode = 7;
            else if (!strcmp("lo16", data_00587fa0->name))
                mode = 6;
            if (mode) {
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '(') {
                    tk = CPrepTokenizer_GetNextToken();
                    parse_unary_expression(expr, flag);
                    expr->type = mode;
                    if (tk != ')')
                        CError_ReportError(ERR_RPAREN_EXPECTED);
                    tk = CPrepTokenizer_GetNextToken();
                    if (IAExpr_IsConst(expr)) {
                        switch (expr->type) {
                            case 8:
                                halfword = (expr->value >> 16) + ((expr->value >> 15) & 1);
                                value = halfword;
                                break;
                            case 7:
                                halfword = expr->value >> 16;
                                value = halfword;
                                break;
                            case 6:
                                halfword = expr->value;
                                value = halfword;
                                break;
                            default:
                                value = expr->value;
                                break;
                        }
                        expr->value = value;
                        expr->type = 5;
                    }
                    return;
                }
                CError_ReportError(ERR_LPAREN_EXPECTED);
                break;
            }
            if (!operand.label)
                operand.label = InlineAsm_CreateLabel(data_00587fa0);
            IAExpr_Const(expr, 0);
            expr->flags |= 1;
            expr->label = operand.label;
            tk = CPrepTokenizer_GetNextToken();
            return;

        case TK_INTCONST:
            value = intconst_lo;
            tk = CPrepTokenizer_GetNextToken();
            IAExpr_Const(expr, value);
            return;

        case TK_SIZEOF:
            IAExpr_Const(expr, scansizeof());
            return;

        case '+':
            tk = CPrepTokenizer_GetNextToken();
            parse_unary_expression(expr, flag);
            return;

        case '-':
            tk = CPrepTokenizer_GetNextToken();
            parse_unary_expression(expr, flag);
            if (IAExpr_IsConst(expr))
                expr->value = -expr->value;
            else
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
            return;

        case '!':
            tk = CPrepTokenizer_GetNextToken();
            parse_unary_expression(expr, flag);
            if (IAExpr_IsConst(expr))
                expr->value = !expr->value;
            else
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
            return;

        case '~':
            tk = CPrepTokenizer_GetNextToken();
            parse_unary_expression(expr, flag);
            if (IAExpr_IsConst(expr))
                expr->value = ~expr->value;
            else
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
            return;

        case '(':
            tk = CPrepTokenizer_GetNextToken();
            parse_expression(expr, flag);
            if (tk != ')')
                CError_ReportError(ERR_RPAREN_EXPECTED);
            tk = CPrepTokenizer_GetNextToken();
            return;

        default:
            CError_ReportError(ERR_UNEXPECTED_TOKEN);
            break;
    }
    IAExpr_Const(expr, 0);
}

void evaluate_inline_asm_binary_expression(InlineAsmExpression *left, short op, InlineAsmExpression *right)
{
    enum Kind kind;
    CLabel *label;
    CInt64 left_value;
    CInt64 result;

    left_value.lo = left->value;
    left_value.hi = (SInt32)left_value.lo < 0 ? -1 : 0;
    result.lo = right->value;
    result.hi = (SInt32)result.lo < 0 ? -1 : 0;
    result = CMach_CalcIntDiadic((Type *)&stsignedint, left_value, op, result);
    if (left->object != NULL) {
        if (right->label != NULL) {
            PPCError_ReportError(124, left->object->name->name, right->label->name->name);
        }
        if (right->object != NULL) {
            if (left->second_object != NULL) {
                PPCError_ReportError(121, left->object->name->name, left->second_object->name->name,
                                     right->object->name->name);
            } else if (right->second_object != NULL) {
                PPCError_ReportError(121, left->object->name->name, right->object->name->name,
                                     right->second_object->name->name);
            } else if (op == '-') {
                left->value = result.lo;
                left->second_object = right->object;
            } else {
                report_binary_expression_error(left->object->name, right->object->name, op);
            }
        } else if (op == '-' || op == '+') {
            left->value = result.lo;
        } else {
            report_binary_expression_error(left->object->name, NULL, op);
        }
    } else if (right->object != NULL) {
        if (right->label != NULL) {
            PPCError_ReportError(124, right->object->name->name, right->label->name->name);
        }
        if (op == '+') {
            left->object = right->object;
            left->second_object = right->second_object;
            left->value = result.lo;
        } else {
            report_binary_expression_error(NULL, right->object->name, op);
        }
    } else if (left->label != NULL) {
        if (left->object != NULL) {
            PPCError_ReportError(124, left->label->name->name, left->object->name->name);
        }
        if ((label = right->label) != NULL) {
            if (left->second_label != NULL) {
                PPCError_ReportError(121, left->label->name->name, left->second_label->name->name, label->name->name);
            } else if (right->second_label != NULL) {
                PPCError_ReportError(121, left->label->name->name, label->name->name, right->second_label->name->name);
            } else if (op == '-') {
                left->value = result.lo;
                left->second_label = right->label;
            } else {
                report_binary_expression_error(left->label->name, label->name, op);
            }
        } else if (op == '+' || op == '-') {
            left->value = result.lo;
        } else {
            report_binary_expression_error(NULL, left->label->name, op);
        }
    } else if (right->label != NULL) {
        if (op == '+') {
            left->label = right->label;
            left->second_label = right->second_label;
            left->value = result.lo;
        } else {
            report_binary_expression_error(NULL, right->label->name, op);
        }
    } else {
        left->value = result.lo;
    }
    left->flags |= right->flags;
    if (left->type == K5) {
        if (right->type != K5) {
            left->type = ((volatile InlineAsmExpression *)right)->type;
        }
    } else {
        kind = right->type;
        if (kind != K5 && kind != left->type)
            PPCError_ReportError(126);
    }
}

void parse_binary_expression_tail(InlineAsmExpression *result, int parseMode)
{
    short token;
    short nextPrecedence;
    short precedence;
    InlineAsmExpression operand;
    while (1) {
        token = tk;
        tk = CPrepTokenizer_GetNextToken();
        parse_unary_expression(&operand, parseMode);
        nextPrecedence = GetPrec(tk);
        if (nextPrecedence == 0) {
            evaluate_inline_asm_binary_expression(result, token, &operand);
            return;
        }
        precedence = GetPrec(token);
        if (precedence >= nextPrecedence) {
            evaluate_inline_asm_binary_expression(result, token, &operand);
            continue;
        }
        parse_binary_expression_tail(&operand, parseMode);
        evaluate_inline_asm_binary_expression(result, token, &operand);
        if (GetPrec(tk) == 0)
            return;
    }
}

int parse_range_checked_expression(EncodedOperand *out, SInt32 lo, SInt32 hi)
{
    SInt32 hasNoReferences;
    InlineAsmExpression expression;
    SInt32 value;
    SInt16 immediate;

    parse_expression(&expression, 0);

    hasNoReferences = (expression.object == NULL) && (expression.object_label == NULL) && (expression.label == NULL);
    if (!hasNoReferences) {
        if (expression.object != NULL) {
            Object *object = expression.object;
            PPCError_ReportError(0x7a, object->name->name);
        } else if (expression.object_label != NULL) {
            Object *object = expression.object_label;
            PPCError_ReportError(0x7a, object->name->name);
        } else if (expression.label != NULL)
            PPCError_ReportError(0xa6, expression.label->name->name);
        value = 0;
    } else {
        switch (expression.type) {
            case 8:
                immediate = (expression.value >> 16) + ((expression.value >> 15) & 1);
                value = immediate;
                break;
            case 7:
                immediate = expression.value >> 16;
                value = immediate;
                break;
            case 6:
                immediate = expression.value;
                value = immediate;
                break;
            default:
                value = expression.value;
                break;
        }
    }

    if (value < lo || value > hi)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);

    out->kind = 1;
    out->data.value = value;
    return value;
}

void parse_expression(InlineAsmExpression *op, int x)
{
    parse_unary_expression(op, x);
    if (GetPrec(tk)) {
        parse_binary_expression_tail(op, x);
    }
    if (GetPrec(tk)) {
        parse_binary_expression_tail(op, x);
    }
    if (op->type == 5 && tk == TK_IDENTIFIER) {
        if (!strcmp(data_00587fa0->name, "@l")) {
            op->type = 6;
            tk = CPrepTokenizer_GetNextToken();
        } else if (!strcmp(data_00587fa0->name, "@ha")) {
            op->type = 8;
            tk = CPrepTokenizer_GetNextToken();
        } else if (!strcmp(data_00587fa0->name, "@h")) {
            op->type = 7;
            tk = CPrepTokenizer_GetNextToken();
        }
    }
}

int fn_00469d40(void *arg)
{
    InlineAsmExpression expression;
    int is_constant;
    int has_no_objects;
    int result;

    parse_expression(&expression, (int)arg);

    has_no_objects = 0;
    is_constant = 0;
    if (expression.object == NULL && expression.object_label == NULL)
        has_no_objects = 1;
    if (has_no_objects && expression.label == NULL)
        is_constant = 1;

    if (!is_constant) {
        if (expression.object != NULL) {
            PPCError_ReportError(0x7a, expression.object->name->name);
        } else if (expression.object_label != NULL) {
            PPCError_ReportError(0x7a, expression.object_label->name->name);
        } else if (expression.label != NULL) {
            PPCError_ReportError(0xa6, expression.label->name->name);
        }
        return 0;
    }

    switch (expression.type) {
        case 8:
            result = (SInt16)((expression.value >> 16) + ((expression.value >> 15) & 1));
            break;
        case 7:
            result = (SInt16)(expression.value >> 16);
            break;
        case 6:
            result = (SInt16)expression.value;
            break;
        default:
            result = expression.value;
            break;
    }
    return result;
}

int check_register_value_range(void)
{
    int value;
    int checked;
    if ((checked = value = register_value()) < 0 || checked > 31)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
    return value;
}

Object *get_struct_or_class_pointer_object(void)
{
    struct AsmOperand buf;
    Object *obj;
    SInt32 v;
    Type *type;
    if (tk != TK_IDENTIFIER)
        return NULL;
    InlineAsm_ResolveOperandNameDefault(data_00587fa0, &buf);
    obj = buf.object;
    if (obj != NULL) {
        if (obj->datatype != DLOCAL)
            return NULL;
        if (Registers_GetInfo(obj) != NULL)
            v = Registers_GetInfo(obj)->reg;
        else
            v = 0;
        if (v == 0 && obj->sclass != TK_REGISTER)
            return NULL;
        if (obj->type->type != TYPEPOINTER)
            return NULL;
        if (TPTR_TARGET(obj->type)->type != TYPESTRUCT && TPTR_TARGET(obj->type)->type != TYPECLASS)
            return NULL;
        return obj;
    }
    return NULL;
}

void parse_expression_operand(EncodedOperand *dest, struct ParsedAsmInstruction *instruction, Boolean allowAddress)
{
    InlineAsmExpression info;
    SInt32 relocationValue;
    SInt32 immediateValue;
    SInt16 adjustedHigh;
    SInt16 highValue;

    parse_expression(&info, 0);
    if (info.object_label != NULL) {
        if (!allowAddress)
            CError_ReportError(ERR_ILLEGAL_ADDRESSING_MODE);
        if (info.object != NULL)
            PPCError_ReportError(0x7a, info.object);
        dest->kind = 2;
        dest->modifier.reg.register_class = 0;
        dest->target.object = info.object_label;
        dest->data.value = 0;
        dest->modifier.reg.flags = 1;
        instruction->operand_count++;
        dest[1].kind = 1;
        switch (info.type) {
            case 5:
                info.type = 2;
            case 2:
            case 6:
            case 7:
            case 8:
                switch (info.type) {
                    case 8:
                        adjustedHigh = (info.value >> 16) + ((info.value >> 15) & 1);
                        relocationValue = adjustedHigh;
                        break;
                    case 7:
                        highValue = info.value >> 16;
                        relocationValue = highValue;
                        break;
                    case 6:
                        relocationValue = (SInt16)info.value;
                        break;
                    default:
                        relocationValue = info.value;
                        break;
                }
                dest[1].data.value = relocationValue;
                break;
            default:
                CError_ReportError(ERR_ILLEGAL_ADDRESSING_MODE);
                break;
        }
        info.object_label->flags |= 1;
        return;
    }
    if (info.object != NULL) {
        if (info.second_object != NULL)
            PPCError_ReportError(0x7b, info.object->name->name, info.second_object->name->name);
        if (allowAddress) {
            if (tk == '(') {
                if (allowAddress) {
                    tk = CPrepTokenizer_GetNextToken();
                    parse_register_operand(dest, 0, 0);
                    if (tk != ')')
                        CError_ReportError(ERR_RPAREN_EXPECTED);
                    tk = CPrepTokenizer_GetNextToken();
                }
            } else {
                dest->kind = 2;
                dest->modifier.reg.register_class = 0;
                dest->target.object = info.object_label;
                dest->data.value = info.islocal;
                dest->modifier.reg.flags = 1;
            }
            dest++;
            instruction->operand_count++;
        }
        if (info.object->datatype == DLOCAL) {
            dest->kind = 4;
            dest->modifier.reg.flags = 1;
        } else {
            dest->kind = 3;
            if (info.type == 5)
                info.type = 2;
            dest->modifier.reg.flags = info.type;
        }
        dest->target.object = info.object;
        dest->data.value = info.value;
        info.object->flags |= 1;
        return;
    }
    if (allowAddress) {
        if (tk == '(') {
            tk = CPrepTokenizer_GetNextToken();
            parse_register_operand(dest, 0, 0);
            dest++;
            instruction->operand_count++;
            if (tk != ')')
                CError_ReportError(ERR_RPAREN_EXPECTED);
            tk = CPrepTokenizer_GetNextToken();
        } else {
            dest->kind = 2;
            dest->modifier.reg.register_class = 0;
            dest->target.object = info.object_label;
            dest->data.value = info.islocal;
            dest++;
            instruction->operand_count++;
        }
    }
    if (info.label != NULL) {
        if (info.second_label != NULL) {
            dest->kind = 6;
            dest->negative = 0;
            dest->data.label = info.label;
            dest->target.label = info.second_label;
            dest->modifier.value = info.value;
        } else {
            PPCError_ReportError(0x7d, info.label->name->name);
        }
    } else {
        dest->kind = 1;
        switch (info.type) {
            case 8:
                adjustedHigh = (info.value >> 16) + ((info.value >> 15) & 1);
                immediateValue = adjustedHigh;
                break;
            case 7:
                highValue = info.value >> 16;
                immediateValue = highValue;
                break;
            case 6:
                immediateValue = (SInt16)info.value;
                break;
            default:
                immediateValue = info.value;
                break;
        }
        dest->data.value = immediateValue;
    }
}

EncodedOperand *parse_displacement_operand(EncodedOperand *operand, struct ParsedAsmInstruction *instruction)
{
    InlineAsmExpression expression;
    SInt32 displacement;
    SInt16 immediate;
    SInt32 is_constant;
    SInt32 has_no_object;

    parse_expression(&expression, 0);
    has_no_object = 0;
    is_constant = 0;
    if (expression.object == NULL && expression.object_label == NULL)
        has_no_object = 1;
    if (has_no_object && expression.label == NULL)
        is_constant = 1;
    if (!is_constant) {
        if (expression.object != NULL)
            PPCError_ReportError(0x7a, expression.object->name->name);
        else if (expression.object_label != NULL)
            PPCError_ReportError(0x7a, expression.object_label->name->name);
        else if (expression.label != NULL)
            PPCError_ReportError(0xa6, expression.label->name->name);
        displacement = 0;
    } else {
        switch (expression.type) {
            case 8:
                immediate = (expression.value >> 16) + ((expression.value >> 15) & 1);
                displacement = immediate;
                break;
            case 7:
                immediate = expression.value >> 16;
                displacement = immediate;
                break;
            case 6:
                immediate = expression.value;
                displacement = immediate;
                break;
            default:
                displacement = expression.value;
                break;
        }
    }
    if (displacement < -0x800 || displacement > 0xfff)
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        parse_register_operand(operand, 0, 0);
        operand++;
        instruction->operand_count++;
        if (tk != ')')
            CError_ReportError(ERR_RPAREN_EXPECTED);
        tk = CPrepTokenizer_GetNextToken();
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
    operand->kind = 1;
    operand->data.value = displacement;
    return operand;
}

void parse_register_operand(EncodedOperand *out, int regClass, Boolean isOutput)
{
    InlineAsmRegisterEntry *entry;
    char *registerName;
    Object *object;
    UInt16 registerNumber;
    int value;
    char *name;

    if (tk == TK_IDENTIFIER && (entry = CTemplateNew_GetInlineAsmRegisterEntry(data_00587fa0)) != NULL &&
        entry->kind == regClass) {
        object = entry->object;
        registerNumber = entry->number;
        out->kind = 2;
        out->modifier.reg.register_class = regClass;
        out->target.object = object;
        out->data.value = registerNumber;
        if (isOutput) {
            out->modifier.reg.flags = 2;
            if (regClass == RC_CRFIELD && registerNumber > 1 && registerNumber < 5)
                data_005883ee = 1;
        } else {
            out->modifier.reg.flags = 1;
        }
        if (object != NULL) {
            entry->object->flags |= OBJECT_USED;
            Registers_GetInfo(object)->usage = 100000;
        }
    } else if (regClass == RC_SPR) {
        value = register_value_zero();
        if (value < 0 || value > 0x3ff)
            CError_ReportError(ERR_NUMBER_OUT_RANGE);
        out->kind = 1;
        out->data.value = value;
        return;
    } else {
        if (tk == TK_IDENTIFIER) {
            name = data_00587fa0->name;
            switch ((RegClass)regClass) {
                case RC_GPR:
                    registerName = "GPR";
                    break;
                case RC_FPR:
                    registerName = "FPR";
                    break;
                case RC_SPR:
                    registerName = "SPR";
                    break;
                case RC_CRFIELD:
                    registerName = "CRFIELD";
                    break;
                case RC_CRFIELDBIT:
                    registerName = "CRFIELDBIT";
                    break;
                case RC_VR:
                    registerName = "VR";
                    break;
            }
            PPCError_ReportError(0xa7, name, registerName);
        } else {
            PPCError_ReportError(0xab);
        }
    }

    tk = CPrepTokenizer_GetNextToken();
    if (memcmp(data_00587fa0->name, "@loword", 8) == 0) {
        tk = CPrepTokenizer_GetNextToken();
        return;
    }
    if (memcmp(data_00587fa0->name, "@hiword", 8) == 0) {
        if (entry->object != NULL)
            out->modifier.reg.flags |= 4;
        else
            PPCError_ReportError(0xa8);
        tk = CPrepTokenizer_GetNextToken();
        return;
    }
    if (regClass == RC_GPR && entry->object != NULL && entry->object->type->size == 8) {
        HashNameNode *objectName = entry->object->name;
        PPCError_ReportError(0x7f, objectName->name, objectName->name, objectName->name);
    }
}

void parse_branch_operand(struct ParsedAsmInstruction *stmt, EncodedOperand *out, Boolean wide, Boolean absolute,
                          Boolean link)
{
    struct AsmOperand info;
    Object *label;
    SInt32 value;

    if (tk == TK_IDENTIFIER) {
        if (!InlineAsm_ResolveOperandNameDefault(data_00587fa0, &info))
            info.label = InlineAsm_CreateLabel(data_00587fa0);
        if (info.label != NULL) {
            out->kind = 5;
            out->data.value = (SInt32)info.label;
        } else if ((label = info.object) != NULL && label->datatype == DFUNC) {
            if (link)
                stmt->specialFlags |= 2;
            out->kind = 3;
            out->target.object = info.object;
            out->data.value = 0;
            if (wide) {
                out->modifier.kind = 4;
                if (absolute)
                    out->modifier.kind = 9;
            } else {
                out->modifier.kind = 2;
                if (absolute)
                    out->modifier.kind = 10;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
        }
        tk = CPrepTokenizer_GetNextToken();
    } else if (tk == '*') {
        if (!absolute) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == '+') {
                tk = CPrepTokenizer_GetNextToken();
                if (wide)
                    BranchImmediateOperand(out, -0x2000000, 0x1ffffff);
                else
                    BranchImmediateOperand(out, -0x8000, 0xffff);
            } else if (tk == '-') {
                tk = CPrepTokenizer_GetNextToken();
                if (wide)
                    value = -BranchImmediateOperand(out, -0x1ffffff, 0x2000000);
                else
                    value = -BranchImmediateOperand(out, -0x7fff, 0x10000);
                out->data.value = value;
            } else {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
        }
    } else if (absolute) {
        if (wide)
            BranchImmediateOperand(out, -0x2000000, 0x1ffffff);
        else
            BranchImmediateOperand(out, -0x8000, 0xffff);
    } else {
        CError_ReportError(ERR_ILLEGAL_OPERAND);
    }
}

void encode_expression_operand(EncodedOperand *operand, int minimum, int maximum, char negative)
{
    CLabel *reference;
    int value;
    InlineAsmExpression parsed;
    parse_expression(&parsed, 0);
    if ((reference = parsed.label) != NULL) {
        if (parsed.second_label != NULL) {
            operand->kind = 6;
            if (negative != 0) {
                operand->negative = 1;
            } else {
                operand->negative = 0;
            }
            operand->data.label = parsed.label;
            operand->target.label = parsed.second_label;
            operand->modifier.value = parsed.value;
        } else {
            PPCError_ReportError(125, reference->name->name);
        }
    } else if (parsed.object != NULL) {
        if (parsed.second_object == NULL) {
            if (parsed.type == 0) {
                PPCError_ReportDiagnostic(170, parsed.object->name->name);
            }
            if (parsed.object->datatype == DLOCAL) {
                operand->kind = 4;
                operand->modifier.kind = 1;
            } else {
                operand->kind = 3;
                if (parsed.type == 5) {
                    parsed.type = 2;
                }
                operand->modifier.kind = parsed.type;
            }
            operand->target.object = parsed.object;
            operand->data.value = parsed.value;
        } else {
            PPCError_ReportError(123, parsed.object->name->name, parsed.second_object->name->name);
        }
    } else {
        value = expression_immediate(&parsed);
        if (value < minimum || value > maximum) {
            CError_ReportError(ERR_NUMBER_OUT_RANGE);
        }
        operand->kind = 1;
        if (negative != 0) {
            value = -value;
        }
        operand->data.value = value;
    }
}

struct ParsedAsmInstruction *parse_asm_instruction_operands(struct AsmOperandPattern *pattern)
{
    PCodeOpcodeDescriptor *descriptor;
    struct ParsedAsmInstruction *instruction;
    int value;
    Object *object;
    unsigned int operand_count, allocation_size, pattern_index;
    struct InlineAsmRegisterEntry *register_entry;
    InlineAsmRegisterEntry *name_rec;
    EncodedOperand *operand;
    long second_value;
    int first_value, last_value, comma_state;
    long cr_bit;
    long insert_position, extract_position, rotation, mask_shift;
    int width;

    descriptor = &gPCodeOpcodeDescriptors[pattern->opcode];
    operand_count = descriptor->operand_count;
    if (descriptor->flags & 0x200)
        operand_count++;
    if (!(descriptor->flags & fSetsCarry) && (descriptor->flags & 0x1000))
        operand_count++;
    allocation_size = operand_count * sizeof(EncodedOperand) + offsetof(struct ParsedAsmInstruction, data);
    instruction = galloc(allocation_size);
    memset(instruction, 0, allocation_size);
    instruction->opcode = pattern->opcode;
    instruction->specialFlags = 0;
    instruction->operand_count = 0;
    comma_state = 0;
    operand = instruction->data.operands;
    pattern_index = 0;
    while (pattern_index < 6 && pattern->operands[pattern_index] != 0) {
        if (comma_state != 0) {
            if (tk == ',')
                tk = CPrepTokenizer_GetNextToken();
            else
                CError_ReportError(ERR_COMMA_EXPECTED);
        }
        switch (pattern->operands[pattern_index]) {
            case 0x51:
                comma_state = -1;
            case 0x50:
                if (!recovery_inline_isreg(0) && tk == TK_INTCONST && intconst_lo == 0) {
                    register_entry = CTemplateNew_LookupInlineAsmRegister("r0");
                    if (register_entry != NULL) {
                        operand->kind = 2;
                        operand->modifier.reg.register_class = 0;
                        operand->target.object = register_entry->object;
                        operand->data.value = register_entry->number;
                        operand->modifier.reg.flags = PCodeOperand_Use;
                        tk = CPrepTokenizer_GetNextToken();
                    } else {
                        RegisterClassError("r0", 0);
                    }
                } else {
                    parse_register_operand(operand, 0, 0);
                }
                break;
            case 4:
            case 6:
            case 8:
            case 0xb:
                comma_state = -1;
            case 3:
            case 5:
            case 7:
            case 10:
                parse_register_operand(operand, 0, 0);
                break;
            case 2:
                comma_state = -1;
            case 1:
            case 9:
                parse_register_operand(operand, 0, 1);
                break;
            case 0x2d:
                if (tk == TK_INTCONST && intconst_lo == 0) {
                    value = recovery_inline_ranged(0, 0);
                    if (tk == ',')
                        tk = CPrepTokenizer_GetNextToken();
                    else
                        CError_ReportError(ERR_COMMA_EXPECTED);
                }
                parse_register_operand(operand, 0, 0);
                break;
            case 0x80:
                comma_state = -1;
            case 0x7f:
                parse_register_operand(&operand[1], 0, 0);
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                parse_register_operand(operand, 0, 0);
                instruction->operand_count++;
                operand++;
                break;
            case 0x3b:
            case 0x3d:
                comma_state = -1;
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3c:
                parse_register_operand(operand, 1, 0);
                break;
            case 0x37:
                comma_state = -1;
            case 0x36:
                parse_register_operand(operand, 1, 1);
                break;
            case 0x14:
                comma_state = -1;
            case 0x10:
            case 0x11:
            case 0x13:
            case 0x15:
                parse_register_operand(operand, 9, 0);
                break;
            case 0x12:
                parse_register_operand(operand, 9, 0);
                operand[1] = operand[0];
                operand++;
                break;
            case 0xf:
                comma_state = -1;
            case 0xe:
                parse_register_operand(operand, 9, 1);
                break;
            case 0x19:
                parse_register_operand(operand, 9, 0);
                operand->modifier.reg.flags = PCodeOperand_Use | PCodeOperand_Definition;
                operand[1] = operand[0];
                operand[1].modifier.reg.flags = PCodeOperand_Use;
                operand[2] = operand[0];
                operand[2].modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count += 2;
                operand += 2;
                break;
            case 0x1a:
                parse_register_operand(operand, 9, 0);
                operand->modifier.reg.flags = PCodeOperand_Use | PCodeOperand_Definition;
                operand[1] = operand[0];
                instruction->operand_count++;
                operand[1].modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0;
                break;
            case 0x70:
                parse_register_operand(operand, 0, 1);
                instruction->operand_count++;
                operand++;
                if (tk == ',') {
                    tk = CPrepTokenizer_GetNextToken();
                    value = recovery_inline_ranged(0x10c, 0x10d);
                    operand->kind = 1;
                    operand->data.value = value;
                } else {
                    operand->kind = 1;
                    operand->data.value = 0x10c;
                }
                break;
            case 0x71:
                parse_register_operand(operand, 0, 1);
                instruction->operand_count++;
                operand++;
                operand->kind = 1;
                operand->data.value = 0x10d;
                break;
            case 0x31:
                parse_register_operand(operand, 3, 0);
                break;
            case 0x2f:
                comma_state = -1;
            case 0x2e:
                parse_register_operand(operand, 3, 1);
                break;
            case 0x30:
                if (recovery_inline_isreg(3)) {
                    parse_register_operand(operand, 3, 1);
                } else {
                    register_entry = CTemplateNew_LookupInlineAsmRegister("cr0");
                    if (register_entry != NULL) {
                        operand->kind = 2;
                        operand->modifier.reg.register_class = 3;
                        operand->target.object = register_entry->object;
                        operand->data.value = register_entry->number;
                        operand->modifier.reg.flags = PCodeOperand_Definition;
                    } else {
                        RegisterClassError("cr0", 3);
                    }
                    comma_state = -1;
                }
                break;
            case 0x28:
                comma_state = -1;
            case 0x29:
                if (recovery_inline_isreg(3)) {
                    parse_register_operand(operand, 3, 0);
                } else {
                    register_entry = CTemplateNew_LookupInlineAsmRegister("cr0");
                    if (register_entry != NULL) {
                        operand->kind = 2;
                        operand->modifier.reg.register_class = 3;
                        operand->target.object = register_entry->object;
                        operand->data.value = register_entry->number;
                        operand->modifier.reg.flags = PCodeOperand_Use;
                    } else {
                        RegisterClassError("cr0", 3);
                    }
                    comma_state = -1;
                }
                break;
            case 0x72:
                parse_branch_operand(instruction, operand, 0, 1, pattern->instruction & 1);
                break;
            case 0x2c:
                parse_branch_operand(instruction, operand, 0, 0, pattern->instruction & 1);
                break;
            case 0x2a:
                parse_branch_operand(instruction, operand, 1, 0, pattern->instruction & 1);
                break;
            case 0x85:
                comma_state = -1;
            case 0x84:
            case 0x87:
                encode_expression_operand(operand, -0x8000, 0xffff, 0);
                break;
            case 0x89:
                comma_state = -1;
            case 0x88:
                encode_expression_operand(operand, -0x7fff, 0x10000, 1);
                break;
            case 0x34:
            case 0x35: {
                int bit_value, checked_bit;
                if ((checked_bit = bit_value = recovery_inline_eval1(1)) < 0 || checked_bit > 0x1f)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                cr_bit = bit_value;
                operand->kind = 2;
                operand->data.value = cr_bit >> 2;
                operand->modifier.reg.register_class = 3;
                operand->modifier.reg.flags = PCodeOperand_Use;
                operand++;
                instruction->operand_count++;
                operand->kind = 1;
                operand->data.value = cr_bit % 4;
                break;
            }
            case 0x33:
                comma_state = -1;
            case 0x32: {
                int bit_value, checked_bit;
                if ((checked_bit = bit_value = recovery_inline_eval1(1)) < 0 || checked_bit > 0x1f)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                cr_bit = bit_value;
                operand->kind = 2;
                operand->data.value = cr_bit >> 2;
                operand->modifier.reg.register_class = 3;
                operand->modifier.reg.flags = PCodeOperand_Definition;
                operand++;
                instruction->operand_count++;
                operand->kind = 1;
                operand->data.value = cr_bit % 4;
                break;
            }
            case 0x6b: {
                int bit_value, checked_bit;
                int bit_part;
                if ((checked_bit = bit_value = recovery_inline_eval1(1)) < 0 || checked_bit > 0x1f)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                cr_bit = bit_value;
                operand->kind = 2;
                operand->data.value = cr_bit >> 2;
                operand->modifier.reg.register_class = 3;
                operand->modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count++;
                bit_part = cr_bit % 4;
                operand[1].kind = 1;
                operand[1].data.value = bit_part;
                instruction->operand_count++;
                operand[2].kind = 2;
                operand[2].data.value = cr_bit >> 2;
                operand[2].modifier.reg.register_class = 3;
                operand[2].modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count++;
                operand += 3;
                operand->kind = 1;
                operand->data.value = bit_part;
                break;
            }
            case 0x6c: {
                int bit_part;
                cr_bit = recovery_inline_crbit();
                operand->kind = 2;
                operand->data.value = cr_bit >> 2;
                operand->modifier.reg.register_class = 3;
                operand->modifier.reg.flags = PCodeOperand_Definition;
                instruction->operand_count++;
                bit_part = cr_bit % 4;
                operand[1].kind = 1;
                operand[1].data.value = bit_part;
                instruction->operand_count++;
                operand[2].kind = 2;
                operand[2].data.value = cr_bit >> 2;
                operand[2].modifier.reg.register_class = 3;
                operand[2].modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count++;
                operand[3].kind = 1;
                operand[3].data.value = bit_part;
                instruction->operand_count++;
                operand[4].kind = 2;
                operand[4].data.value = cr_bit >> 2;
                operand[4].modifier.reg.register_class = 3;
                operand[4].modifier.reg.flags = PCodeOperand_Use;
                instruction->operand_count++;
                operand += 5;
                operand->kind = 1;
                operand->data.value = bit_part;
                break;
            }
            case 0x3e:
                parse_expression_operand(operand, instruction, 1);
                break;
            case 0x3f:
                if (tk == TK_FLOATCONST) {
                    object = TOC_GetFloatObject(TYPE(&stfloat), &token_float);
                    operand->kind = 2;
                    operand->modifier.reg.register_class = 0;
                    operand->target.object = NULL;
                    operand->modifier.reg.flags = PCodeOperand_Use;
                    operand->data.value = 0;
                    instruction->operand_count++;
                    operand[1].kind = 4;
                    operand[1].target.object = object;
                    operand[1].modifier.reg.flags = PCodeOperand_Definition;
                    operand[1].data.value = 0;
                    tk = CPrepTokenizer_GetNextToken();
                } else {
                    parse_expression_operand(operand, instruction, 1);
                }
                break;
            case 0x40:
                if (tk == TK_FLOATCONST) {
                    object = TOC_GetFloatObject(TYPE(&stdouble), &token_float);
                    operand->kind = 2;
                    operand->modifier.reg.register_class = 0;
                    operand->target.object = NULL;
                    operand->modifier.reg.flags = PCodeOperand_Use;
                    operand->data.value = 0;
                    instruction->operand_count++;
                    operand[1].kind = 4;
                    operand[1].target.object = object;
                    operand[1].modifier.reg.flags = PCodeOperand_Definition;
                    operand[1].data.value = 0;
                    tk = CPrepTokenizer_GetNextToken();
                } else {
                    parse_expression_operand(operand, instruction, 1);
                }
                break;
            case 0x41:
                operand = parse_displacement_operand(operand, instruction);
                break;
            case 0x16:
                value = recovery_inline_ranged(0, 3);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x17:
                value = recovery_inline_ranged(0, 3);
                operand->kind = 1;
                operand->data.value = value;
                instruction->operand_count++;
                operand++;
                if ((first_value = tk) == ',') {
                    tk = CPrepTokenizer_GetNextToken();
                    value = recovery_inline_ranged(0, 1);
                    operand->kind = 1;
                    operand->data.value = value;
                } else {
                    operand->kind = 1;
                    operand->data.value = 0;
                }
                break;
            case 0xc:
                value = recovery_inline_ranged(0, 1);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0xd:
                value = recovery_inline_ranged(0, 1);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x1c:
                comma_state = -1;
            case 0x1b:
                value = recovery_inline_ranged(-0x8000, 0xffff);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x1e:
                comma_state = -1;
            case 0x1d:
                value = recovery_inline_ranged(-0x10, 0xf);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x20:
                comma_state = -1;
            case 0x1f:
                value = recovery_inline_ranged(-0x7fff, 0x10000);
                operand->kind = 1;
                operand->data.value = value;
                operand->data.value = -value;
                break;
            case 0x22:
                comma_state = -1;
            case 0x21:
                value = recovery_inline_ranged(0, 0xffff);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x2b:
                parse_branch_operand(instruction, operand, 1, 1, pattern->instruction & 1);
                break;
            case 0x42:
                value = recovery_inline_ranged(1, 0x20);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x43:
                parse_register_operand(operand, 2, 0);
                break;
            case 0x44:
                if (tk == TK_IDENTIFIER && (name_rec = fn_004f06d0(data_00587fa0->name)) != NULL) {
                    operand->kind = 1;
                    operand->data.value = name_rec->number;
                } else {
                    value = recovery_inline_ranged(0, 0x3ff);
                    operand->kind = 1;
                    operand->data.value = value;
                }
                break;
            case 0x46:
            case 0x47:
                value = recovery_inline_ranged(0, 0xff);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x49:
                comma_state = -1;
            case 0x25:
            case 0x45:
            case 0x48:
                value = recovery_inline_ranged(0, 0xf);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x8a:
                value = recovery_inline_ranged(0, 7);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x24:
            case 0x4b:
            case 0x4e:
                comma_state = -1;
            case 0x23:
            case 0x26:
            case 0x4a:
            case 0x4c:
            case 0x4d:
            case 0x4f:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x27: {
                int bit_value, checked_bit;
                if ((checked_bit = bit_value = recovery_inline_eval1(1)) < 0 || checked_bit > 0x1f)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                cr_bit = bit_value;
                operand->kind = 2;
                operand->data.value = cr_bit >> 2;
                operand->modifier.reg.register_class = 3;
                operand->modifier.reg.flags = PCodeOperand_Use;
                operand++;
                instruction->operand_count++;
                operand->kind = 1;
                operand->data.value = cr_bit % 4;
                break;
            }
            case 0x58:
                comma_state = -1;
            case 0x57:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            case 0x5a:
                comma_state = -1;
            case 0x59:
                value = recovery_inline_ranged(1, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                operand->data.value = 0x20 - value;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            case 100:
                comma_state = -1;
            case 99:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                last_value = recovery_inline_ranged2(0, value);
                operand->kind = 1;
                operand->data.value = last_value;
                mask_shift = last_value;
                operand->data.value = mask_shift;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = value - mask_shift;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f - mask_shift;
                break;
            case 0x60:
                comma_state = -1;
            case 0x5f:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                operand->data.value = 0;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = value;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            case 0x62:
                comma_state = -1;
            case 0x61:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                operand->data.value = 0;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f - value;
                break;
            case 0x54:
                comma_state = -1;
            case 0x53:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                last_value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = last_value;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = value - 1;
                break;
            case 0x56:
                comma_state = -1;
            case 0x55: {
                int second_shift, parsed_shift;
                first_value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = first_value;
                rotation = first_value;
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                parsed_shift = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = parsed_shift;
                second_shift = parsed_shift;
                if (0x1f < second_shift + rotation)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                second_shift += rotation;
                operand->data.value = second_shift;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0x20 - rotation;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            }
            case 0x5c:
                comma_state = -1;
            case 0x5b:
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f - value;
                break;
            case 0x5e:
                comma_state = -1;
            case 0x5d:
                value = recovery_inline_ranged(1, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                rotation = value;
                operand->data.value = 0x20 - rotation;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = rotation;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            case 0x6a:
                comma_state = -1;
            case 0x69:
                parse_register_operand(operand, 0, 0);
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = 0;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = 0x1f;
                break;
            case 0x66:
                comma_state = -1;
            case 0x65:
                first_value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = first_value;
                extract_position = first_value;
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                second_value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = second_value;
                width = second_value;
                if (0x20 < width + extract_position)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                operand->data.value = 0x20 - width;
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = width;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                width += extract_position;
                operand->data.value = --width;
                break;
            case 0x68:
                comma_state = -1;
            case 0x67: {
                long shift;
                value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = value;
                insert_position = value;
                if (tk == ',')
                    tk = CPrepTokenizer_GetNextToken();
                else
                    CError_ReportError(ERR_COMMA_EXPECTED);
                last_value = recovery_inline_ranged(0, 0x1f);
                operand->kind = 1;
                operand->data.value = last_value;
                shift = last_value;
                if (shift + insert_position > 0x20)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                operand->data.value = 0x20 - (shift + insert_position);
                instruction->operand_count++;
                operand[1].kind = 1;
                operand[1].data.value = shift;
                instruction->operand_count++;
                operand += 2;
                operand->kind = 1;
                operand->data.value = shift + insert_position - 1;
                break;
            }
            case 0x76:
                comma_state = -1;
            case 0x75:
                operand->kind = 2;
                operand->modifier.reg.register_class = 2;
                operand->target.object = NULL;
                operand->data.value = (pattern->instruction >> 0x10 & 0x1f) + (pattern->instruction >> 6 & 0x3e0);
                if ((pattern->instruction >> 1 & 0x3ff) == 0x1d3)
                    operand->modifier.reg.flags = PCodeOperand_Definition;
                break;
            case 0x6f:
                comma_state = -1;
            case 0x6e:
                operand->kind = 2;
                operand->modifier.reg.register_class = 2;
                operand->target.object = NULL;
                operand->data.value = (pattern->instruction >> 0x10 & 0x1f) + (pattern->instruction >> 6 & 0x3e0);
                value = recovery_inline_ranged(0, 3);
                operand->data.value =
                    (pattern->instruction >> 0x10 & 0x1f) + (pattern->instruction >> 6 & 0x3e0) + value;
                if ((pattern->instruction >> 1 & 0x3ff) == 0x1d3)
                    operand->modifier.reg.flags = PCodeOperand_Definition;
                break;
            case 0x74:
                comma_state = -1;
            case 0x73:
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0xb & 7;
                break;
            case 0x77:
                comma_state = -1;
            case 0x78:
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0x15 & 0x1f;
                break;
            case 0x79:
                comma_state = -1;
            case 0x7a:
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0x15 & 0x1f;
                break;
            case 0x7b:
                comma_state = -1;
            case 0x7c:
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0x10 & 0x1f;
                break;
            case 0x7d:
                comma_state = -1;
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0xc & 0xff;
                break;
            case 0x7e:
                comma_state = -1;
                operand->kind = 1;
                operand->data.value = pattern->instruction >> 0x11 & 0xff;
                break;
            case 0x6d:
                operand->kind = 1;
                value = recovery_inline_ranged(0, 3);
                rotation = value * 2;
                rotation += (pattern->instruction >> 0x10 & 0x1f) + (pattern->instruction >> 6 & 0x3e0);
                operand->data.value = rotation;
                break;
            case 0x81:
                encode_expression_operand(operand, -0x80000000, 0x7fffffff, 0);
                break;
            case 0x82:
                operand->kind = 2;
                operand->modifier.reg.register_class = 2;
                operand->target.object = NULL;
                operand->data.value = 0;
                operand->modifier.reg.flags = PCodeOperand_Definition;
                comma_state = -1;
                break;
            case 0x83:
                operand->kind = 2;
                operand->modifier.reg.register_class = 3;
                operand->target.object = NULL;
                if ((descriptor->flags & 3) == 2)
                    operand->data.value = 1;
                else if ((descriptor->flags & 3) == 3)
                    operand->data.value = 6;
                else
                    operand->data.value = 0;
                operand->modifier.reg.flags = PCodeOperand_Definition;
                comma_state = -1;
                break;
            case 0x8b:
                value = recovery_inline_ranged(0, 1);
                operand->kind = 1;
                operand->data.value = value;
                break;
            case 0x8c:
                value = recovery_inline_ranged(0, 7);
                operand->kind = 1;
                operand->data.value = value;
                break;
            default:
                CError_ReportError(ERR_ILLEGAL_OPERAND);
                break;
        }
        comma_state++;
        operand++;
        instruction->operand_count++;
        pattern_index++;
    }
    return instruction;
}

/* The linker stripped the function that used these literals; they stay in the unit's .data. */
static void InlineAsmPPC_StrippedLiterals(const char **literals)
{
    literals[0] = "bso";
    literals[1] = "fcmpo";
    literals[2] = "eieio";
    literals[3] = "vslo";
    literals[4] = "vsro";
}

void InlineAsmPPC_Initialize(void)
{
    unsigned int allowed;
    short cpu;
    unsigned int enabled;
    fn_0042c0c0();
    data_005652f8 = 1U;
    inlineAsmPPCEnabled = 1;
    asm_instruction_count = 0U;
    cpu = copts.processor;
    switch (cpu) {
        case 0:
            data_00587128 = 0x08000040U;
            break;
        case 1:
            data_00587128 = 0x08000080U;
            break;
        case 2:
            data_00587128 = 0x90000800U;
            break;
        case 3:
            data_00587128 = 0x90000800U;
            break;
        case 4:
            data_00587128 = 0x90001000U;
            break;
        case 5:
            data_00587128 = 0x8F000001U;
            break;
        case 6:
            data_00587128 = 0x9D800002U;
            break;
        case 18:
            data_00587128 = 0x9F808000U;
            break;
        case 19:
            data_00587128 = 0x1F008000U;
            break;
        case 7:
            data_00587128 = 0x9F800004U;
            break;
        case 8:
            data_00587128 = 0x9F800004U;
            break;
        case 9:
            data_00587128 = 0x9F800008U;
            break;
        case 10:
            data_00587128 = 0x9F800008U;
            break;
        case 11:
            data_00587128 = 0x9F802000U;
            break;
        case 12:
            data_00587128 = 0x9F802000U;
            break;
        case 13:
            data_00587128 = 0x1C000010U;
            break;
        case 14:
            data_00587128 = 0x1C000010U;
            break;
        case 15:
            data_00587128 = 0x1C000020U;
            break;
        case 16:
            data_00587128 = 0x1C000020U;
            break;
        case 17:
            data_00587128 = 0x1C000010U;
            break;
        case 21:
            data_00587128 = 0xDF806000U;
            break;
        case 22:
            data_00587128 = 0xBF802000U;
            break;
        case 20:
            data_00587128 = 0x000FFFFFU;
            break;
        default:
            CError_FATAL(2665);
            break;
    }
    enabled = 0U;
    if (copts.debugEnabled != 0) {
        allowed = 0;
        if (copts.operandsDebug == 0U)
            allowed = 1;
        if (allowed)
            enabled = 1U;
    }
    inlineAsmPPCEnabled = enabled;
    if (copts.altivec_model != 0U)
        data_00587128 |= 0x40000000U;
}

void InlineAsmPPC_Init(char mode)
{
    void InlineAsmPPC_Initialize(void);

    inlineAsmMode = mode;
    if (copts.catssupport != '\0' && copts.forcecatssupport == '\0') {
        ObjGen_PPC_EABI_AddSectionAttribute(cscope_currentfunc, 1);
    }
    if (inlineAsmMode == '\0') {
        InlineAsmPPC_Initialize();
    }
    CTemplateNew_InitAsmOperandPatternLookup();
    CTemplateNew_ClearGlobalArray();
    CTemplateNew_InitRegistrationHashTables();
    if (inlineAsmMode != '\0') {
        CodeGen_EnumerateArgumentRegisters(assign_object_register);
        FuncLevelAsmPPC_AllocateLocals();
    }
}

void *create_function_asm_directive(HashNameNode *name, Boolean isGlobal)
{
    struct AsmOperand lookup;
    Object *object;
    ParsedAsmInstruction *instruction;
    ParsedAsmInstruction(*allocation)[1];

    if (InlineAsm_ResolveOperandNameDefault(name, &lookup) && (object = lookup.object) != NULL) {
        if (object->datatype != DFUNC || (object->flags & OBJECT_DEFINED) != 0)
            CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
        object->flags |= OBJECT_DEFINED;
        object->sclass = isGlobal ? 0x102 : 0x103;
        allocation = (ParsedAsmInstruction(*)[1])lalloc(0x10);
        memclrw(allocation, 0x10);
        instruction = (ParsedAsmInstruction *)allocation;
        instruction->opcode = 1;
        instruction->specialFlags = 1;
        (*allocation)->data.directive.object = object;
        (*allocation)->data.directive.size = asm_instruction_count << 2;
    } else {
        CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
    }
    return instruction;
}

SInt32 InlineAsmPPC_ClassifyIdentifier(Boolean flag)
{
    SInt32 result = 0;
    if (tk == '.')
        tk = CPrepTokenizer_GetNextToken();
    if (tk == TK_IDENTIFIER) {
        char *s = data_00587fa0->name;
        if (memcmp(s, "machine", 8) == 0)
            result = 5;
        else if (flag == 1) {
            if (memcmp(s, "entry", 6) == 0)
                result = 1;
            else if (memcmp(s, "fralloc", 8) == 0)
                result = 2;
            else if (memcmp(s, "nofralloc", 10) == 0)
                result = 3;
            else if (memcmp(s, "frfree", 7) == 0)
                result = 4;
            else if (memcmp(s, "smclass", 8) == 0)
                result = 6;
            else
                result = 0;
        }
    }
    return result;
}

#define CPUNAME (data_00587fa0->name)

void InlineAsmPPC_ParseDirective(int directive)
{
    switch (directive) {
        case 1: {
            Boolean flag = 0;
            Statement *node;
            int token;
            tk = CPrepTokenizer_GetNextToken();
            if ((token = tk) == 0x102) {
                flag = 1;
                tk = CPrepTokenizer_GetNextToken();
            } else if (token == 0x103) {
                tk = CPrepTokenizer_GetNextToken();
            }
            if (tk != TK_IDENTIFIER)
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            node = CFunc_AppendStatement(0x10);
            node->expr = NULL;
            node->expr = (ENode *)create_function_asm_directive(data_00587fa0, flag);
            node->sourceoffset = -1;
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        case 2: {
            InlineAsmExpression constant;
            int absolute, noNodes;
            Object *base;
            Object *offset;
            SInt32 value;
            SInt16 halfword;
            if (asm_instruction_count != 0)
                CError_ReportError(ERR_FUNCTION_NO_INITIALIZED_STACKFRAME);
            if (data_00588521 == 0)
                CError_ReportError(ERR_FUNCTION_ALREADY_STACKFRAME);
            if (data_005884f4 != 0)
                CError_ReportError(ERR_FUNCTION_NO_INITIALIZED_STACKFRAME);
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_EOL && tk != ';') {
                parse_expression(&constant, 0);
                absolute = 0;
                noNodes = 0;
                if ((base = constant.object) == NULL && constant.object_label == NULL)
                    noNodes = 1;
                if (noNodes && constant.label == NULL)
                    absolute = 1;
                if (!absolute) {
                    if (base != NULL) {
                        PPCError_ReportError(0x7a, base->name->name);
                    } else if ((offset = constant.object_label) != NULL) {
                        PPCError_ReportError(0x7a, offset->name->name);
                    } else if (constant.label) {
                        PPCError_ReportError(0xa6, constant.label->name->name);
                    }
                    value = 0;
                } else {
                    switch (constant.type) {
                        case 8:
                            halfword = (constant.value >> 16) + ((constant.value >> 15) & 1);
                            value = halfword;
                            break;
                        case 7:
                            halfword = constant.value >> 16;
                            value = halfword;
                            break;
                        case 6:
                            halfword = constant.value;
                            value = halfword;
                            break;
                        default:
                            value = constant.value;
                            break;
                    }
                }
                if (value < 0x20 || value > 0x7ffe)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                data_005871a0 = value;
            }
            data_00588521 = 0;
            break;
        }
        case 3: {
            if (data_00588521 == 0)
                CError_ReportError(ERR_FUNCTION_ALREADY_STACKFRAME);
            if (asm_instruction_count != 0)
                CError_ReportError(ERR_FUNCTION_NO_INITIALIZED_STACKFRAME);
            tk = CPrepTokenizer_GetNextToken();
            data_005884f4 = 1;
            break;
        }
        case 4: {
            Statement *node;
            struct ParsedAsmInstruction *instruction;
            if (data_00588521 != 0)
                CError_ReportError(ERR_FUNCTION_NO_INITIALIZED_STACKFRAME);
            node = CFunc_AppendStatement(0x10);
            node->expr = NULL;
            instruction = lalloc(offsetof(struct ParsedAsmInstruction, data));
            memclrw(instruction, offsetof(struct ParsedAsmInstruction, data));
            instruction->opcode = 4;
            instruction->specialFlags = 1;
            node->expr = (ENode *)instruction;
            if (copts.filesyminfo != 0)
                node->sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
            else
                node->sourceoffset = -1;
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        case 5: {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_INTCONST) {
                switch (intconst_lo) {
                    case 0x191:
                        data_00587128 = 0x8000040;
                        break;
                    case 0x193:
                        data_00587128 = 0x8000080;
                        break;
                    case 0x1f9:
                        data_00587128 = 0x90000800;
                        break;
                    case 0x1fd:
                        data_00587128 = 0x90000800;
                        break;
                    case 0x22b:
                        data_00587128 = 0x90001000;
                        break;
                    case 0x259:
                        data_00587128 = 0x8f000001;
                        break;
                    case 0x25a:
                        data_00587128 = 0x9d800002;
                        break;
                    case 0x2030:
                        data_00587128 = 0x9f808000;
                        break;
                    case 0x2044:
                        data_00587128 = 0x1f008000;
                        break;
                    case 0x25b:
                        data_00587128 = 0x9f800004;
                        break;
                    case 0x25c:
                        data_00587128 = 0x9f800008;
                        break;
                    case 0x2e4:
                        data_00587128 = 0x9f802000;
                        break;
                    case 0x2ee:
                        data_00587128 = 0x9f802000;
                        break;
                    case 0x321:
                        data_00587128 = 0x1c000010;
                        break;
                    case 0x335:
                        data_00587128 = 0x1c000010;
                        break;
                    case 0x337:
                        data_00587128 = 0x1c000020;
                        break;
                    case 0x352:
                        data_00587128 = 0x1c000020;
                        break;
                    case 0x35c:
                        data_00587128 = 0x1c000010;
                        break;
                    case 0x1ce8:
                        data_00587128 = 0xdf806000;
                        break;
                    default:
                        CError_ReportError(ERR_ILLEGAL_OPERAND);
                        break;
                }
            } else if (tk == TK_IDENTIFIER) {
                if (memcmp(CPUNAME, "all", 4) == 0)
                    data_00587128 = 0xff8fffff;
                else if (memcmp(CPUNAME, "generic", 8) == 0)
                    data_00587128 = 0xfffff;
                else if (memcmp(CPUNAME, "603e", 5) == 0)
                    data_00587128 = 0x9f800004;
                else if (memcmp(CPUNAME, "604e", 5) == 0)
                    data_00587128 = 0x9f800008;
                else if (memcmp(CPUNAME, "PPC603e", 8) == 0)
                    data_00587128 = 0x9f800004;
                else if (memcmp(CPUNAME, "PPC604e", 8) == 0)
                    data_00587128 = 0x9f800008;
                else if (memcmp(CPUNAME, "PPC403GA", 9) == 0)
                    data_00587128 = 0x8000180;
                else if (memcmp(CPUNAME, "PPC403GB", 9) == 0)
                    data_00587128 = 0x8000080;
                else if (memcmp(CPUNAME, "PPC403GC", 9) == 0)
                    data_00587128 = 0xc000280;
                else if (memcmp(CPUNAME, "PPC403GCX", 10) == 0)
                    data_00587128 = 0xc000480;
                else if (memcmp(CPUNAME, "altivec", 8) == 0)
                    data_00587128 = 0xdf806000;
                else if (memcmp(CPUNAME, "gekko", 6) == 0 || memcmp(CPUNAME, "gecko", 6) == 0)
                    data_00587128 = 0xbf802000;
                else
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
            } else {
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            }
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        case 6: {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_IDENTIFIER && memcmp(CPUNAME, "PR", 3) == 0)
                function_header_index = ObjGen_PPC_EABI_GetHeaderIndex(1);
            else
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        default:
            CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
            break;
    }
}

void InlineAsmPPC_ParseDirectiveIdentifier(void)
{
    int instruction;

    instruction = InlineAsmPPC_ClassifyIdentifier(inlineAsmMode);
    if (instruction != 0) {
        InlineAsmPPC_ParseDirective(instruction);
    }
}

void InlineAsmPPC_ParseInstruction(void)
{
    Boolean hasDot;
    Statement *record;
    UInt32 flag0, flag1, flag2;
    Boolean hasPlus = 0;
    Boolean hasMinus = 0;
    char name[12];
    char *mnemonic;
    AsmOperandPattern *operand;
    PCodeOpcodeDescriptor *descriptor;
    struct ParsedAsmInstruction *instruction;
    struct ParsedAsmInstruction *parsed;
    int specialInstruction;

    specialInstruction = InlineAsmPPC_ClassifyIdentifier(inlineAsmMode);
    if (specialInstruction != 0) {
        InlineAsmPPC_ParseDirective(specialInstruction);
        return;
    }
    record = CFunc_AppendStatement(0x10);
    record->expr = NULL;
    if (copts.filesyminfo != 0)
        record->sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
    else
        record->sourceoffset = -1;
    strncpy(name, data_00587fa0->name, 0xb);
    mnemonic = name;
    hasDot = 0;
    if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x2e) {
        tk = CPrepTokenizer_GetNextToken();
        hasDot = 1;
        strcat(mnemonic, ".");
    }
    operand = CTemplateNew_FindAsmOperandPattern(mnemonic);
    if (operand == NULL)
        CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
    descriptor = &gPCodeOpcodeDescriptors[operand->opcode];
    flag0 = 0;
    if ((descriptor->flags & 0x1000) && (operand->instruction & 0x400))
        flag0 = 1;
    flag1 = 0;
    if ((descriptor->flags & 0x8000) && (operand->instruction & 2))
        flag1 = 1;
    flag2 = 0;
    if ((descriptor->flags & 0x2000) && (operand->instruction & 1))
        flag2 = 1;
    if (data_00587128 == 0xfffff && (data_00587128 & 0xfffff) != (data_00587128 & operand->processorMask & 0xfffff)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((data_00587128 & operand->processorMask & 0xfffff) == 0) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x10000000) && !(data_00587128 & 0x10000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x8000000) && !(data_00587128 & 0x8000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x2000000) && !(data_00587128 & 0x2000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x4000000) && !(data_00587128 & 0x4000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x1000000) && !(data_00587128 & 0x1000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x40000000) && !(data_00587128 & 0x40000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x20000000) && !(data_00587128 & 0x20000000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if ((operand->processorMask & 0x800000) && !(data_00587128 & 0x800000)) {
        CError_ReportError(ERR_ILLEGAL_INSTRUCTION_PROCESSOR);
    } else if (operand->processorMask & 0x80000000) {
        if (inlineAsmPPCEnabled == 0)
            PPCError_ReportError(0x84);
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '+' || tk == '-') {
        if ((((operand->instruction >> 26) & 0x3f) == 0x10 && ((operand->instruction >> 21) & 0x1f) != 0x14) ||
            (((operand->instruction >> 26) & 0x3f) == 0x13 && ((operand->instruction >> 1) & 0x3ff) == 0x210 &&
             ((operand->instruction >> 21) & 0x1f) != 0x14) ||
            (((operand->instruction >> 26) & 0x3f) == 0x13 && ((operand->instruction >> 1) & 0x3ff) == 0x10 &&
             ((operand->instruction >> 21) & 0x1f) != 0x14)) {
            if (tk == '+')
                hasPlus = 1;
            else if (tk == '-')
                hasMinus = 1;
            tk = CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportError(ERR_UNEXPECTED_TOKEN);
        }
    }
    parsed = parse_asm_instruction_operands(operand);
    record->expr = (ENode *)parsed;
    instruction = (ParsedAsmInstruction *)record->expr;
    if (hasDot)
        instruction->branch_flags |= 1;
    if ((UInt8)flag0)
        instruction->branch_flags |= 2;
    if ((UInt8)flag1)
        instruction->branch_flags |= 4;
    if ((UInt8)flag2)
        instruction->branch_flags |= 8;
    if (hasPlus)
        instruction->branch_flags |= 0x10;
    if (hasMinus)
        instruction->branch_flags |= 0x20;
    asm_instruction_count++;
}

/* The PCode instruction for a parsed inline-assembler instruction: its operands converted (registers bound to the
   variables they name, labels created on first use), a load/store-multiple given its register list, and a call
   given its clobbers and the exception actions in force. */
PCodeInstruction *create_pcode_asm_instruction(ParsedAsmInstruction *ia, SInt32 argcount, UInt8 flag)
{
    PCodeInstruction *instr;
    struct PCodeAsmOperand *out;
    EncodedOperand *operand;
    PCodeOpcodeDescriptor *info;
    SInt32 i = 0;
    SInt32 firstReg = 0;
    SInt32 extra = 0;
    SInt32 count, size;
    UInt32 flags;
    int reg;

    info = &gPCodeOpcodeDescriptors[count = ia->opcode];
    if ((flags = info->flags) & 0x200)
        extra++;
    if (!(flags & 0x100) && (flags & 0x1000))
        extra++;
    if (ia->specialFlags & 2) {
        count = argcount + 49;
        if (copts.exceptions && gCurrentStatement && !inlineAsmMode)
            count += Exceptions_CountBoundObjectFields(gCurrentStatement->dobjstack);
        size = sizeof(*instr) + (count + extra) * sizeof(*out);
        instr = lalloc(size);
        memset(instr, 0, size);
        instr->operand_count = count;
    } else if (count == PC_STMW || count == PC_LMW) {
        operand = ia->data.operands;
        firstReg = operand->target.object ? InlineAsm_Register(operand) : operand->data.value;
        count = 32 - firstReg + argcount;
        size = sizeof(*instr) + count * sizeof(*out);
        instr = lalloc(size);
        memset(instr, 0, size);
        instr->operand_count = count;
    } else {
        size = sizeof(*instr) + (argcount + extra) * sizeof(*out);
        instr = lalloc(size);
        memset(instr, 0, size);
        instr->operand_count = argcount;
    }
    instr->opcode = ia->opcode;
    instr->flags = info->flags;
    out = instr->operandData.assemblyOperands;
    operand = ia->data.operands;
    for (; i < argcount; i++, out++, operand++) {
        if (i >= ia->operand_count)
            out->kind = 10;
        else
            switch ((UInt8)operand->kind) {
                case 0:
                    out->kind = 10;
                    break;
                case 1:
                    out->kind = 4;
                    out->data.imm.value = operand->data.value;
                    if (instr->flags & (fIsRead | fIsWrite))
                        instr->flags |= fIsPtrOp;
                    out->data.imm.obj = NULL;
                    break;
                case 2: {
                    SInt16 registerFlags;
                    out->kind = operand->modifier.reg.register_class;
                    if (operand->target.object) {
                        registerFlags = operand->modifier.reg.flags;
#if VERSION >= VERSION_GC_1_2_5
                        if (registerFlags & 4) {
#else
                        if (registerFlags == 4) {
#endif
                            out->data.reg.reg = InlineAsm_RegisterHi(operand);
                            operand->modifier.reg.flags &= ~4;
                        } else
                            out->data.reg.reg = InlineAsm_Register(operand);
                        if (!InlineAsm_Register(operand)) {
                            if (Registers_GetInfo(operand->target.object)->usage >= 100000)
                                PPCError_ReportError(172, operand->target.object->name->name);
                            else
                                PPCError_ReportError(167, operand->target.object->name->name);
                        }
                    } else
                        out->data.reg.reg = operand->data.value;
                    out->arg = operand->modifier.reg.flags;
                    if (instr->opcode == PC_RLWIMI && ((SInt8)out->arg & 2))
                        out->arg |= 1;
                    if (out->kind == 2) {
                        switch (out->data.reg.reg) {
                            case 0:
                                out->data.reg.reg = 0;
                                break;
                            case 8:
                                out->data.reg.reg = 2;
                                break;
                            case 9:
                                out->data.reg.reg = 1;
                                break;
                            default:
                                out->kind = 4;
                                out->data.imm.value = out->data.reg.reg;
                                out->data.imm.obj = NULL;
                                break;
                        }
                    }
                    registerFlags = operand->modifier.reg.flags;
                    if (registerFlags & 2) {
                        if (out->kind == 0 && out->data.reg.reg < 32) {
                            if (operand->target.object && InlineAsm_RegisterHi(operand))
                                Registers_BindGPRPair(operand->target.object, InlineAsm_Register(operand),
                                                      InlineAsm_RegisterHi(operand));
                            else
                                Registers_BindGPR(operand->target.object, out->data.reg.reg);
                        } else if (out->kind == 1 && out->data.reg.reg < 32)
                            Registers_BindFPR(operand->target.object, out->data.reg.reg);
                        else if (out->kind == 9 && out->data.reg.reg < 32)
                            Registers_BindVR(operand->target.object, out->data.reg.reg);
                    }
                    break;
                }
                case 5:
                    if (!operand->data.label->pclabel)
                        operand->data.label->pclabel = PCode_NewLabel();
                    out->kind = 6;
                    out->data.label.label = operand->data.label->pclabel;
                    break;
                case 3:
                case 4:
                    out->kind = 5;
                    out->arg = operand->modifier.reg.flags;
                    out->data.imm.obj = operand->target.object;
                    out->data.imm.value = operand->data.value;
                    break;
                case 6:
                    if (!operand->data.label->pclabel)
                        operand->data.label->pclabel = PCode_NewLabel();
                    if (!operand->target.label->pclabel)
                        operand->target.label->pclabel = PCode_NewLabel();
                    if (instr->flags & (fIsRead | fIsWrite))
                        instr->flags |= fIsPtrOp;
                    out->kind = 7;
                    out->data.labeldiff.labelA = operand->data.label->pclabel;
                    out->data.labeldiff.labelB = operand->target.label->pclabel;
                    out->arg = operand->negative;
                    out->data.labeldiff.offset = operand->modifier.kind;
                    break;
                default:
                    CError_FATAL(3514);
                    break;
            }
    }
    if (ia->opcode == PC_STMW || ia->opcode == PC_LMW)
        for (reg = firstReg; reg < 32; reg++, out++) {
            SInt8 access;
            out->kind = 0;
            out->data.reg.reg = reg;
            if (ia->opcode == PC_LMW)
                access = 2;
            else
                access = 1;
            out->arg = access;
        }
    if (ia->specialFlags & 2) {
        fn_004a2290((PCodeOperand *)out, 4096, 0, 0);
        if (copts.exceptions && gCurrentStatement && !inlineAsmMode)
            Exceptions_CollectRegisterOperands(gCurrentStatement->dobjstack, (PCodeOperand *)out);
    }
    return instr;
}

static int InlineAsm_RegisterHi(EncodedOperand *operand)
{
    return Registers_GetInfo(operand->target.object) ? Registers_GetInfo(operand->target.object)->regHi : 0;
}

static int InlineAsm_Register(EncodedOperand *operand)
{
    return Registers_GetInfo(operand->target.object) ? Registers_GetInfo(operand->target.object)->reg : 0;
}

void InlineAsmPPC_GenerateAsmInstruction(Statement *o)
{
    UInt32 op;
    ParsedAsmInstruction *q = (ParsedAsmInstruction *)o->expr;
    PCodeInstruction *instr;
    PCodeLabel *found;
    PCodeLabel *r;

    instr = create_pcode_asm_instruction(q, gPCodeOpcodeDescriptors[q->opcode].operand_count, inlineAsmMode);
    PCode_AppendInstruction(gCurrentBlock, instr);
    Operands_AllocateGPR(0x400);

    if (q->branch_flags & 1) {
        if (instr->flags & 0x200) {
            PCodeUtilities_MakeRecordForm(gCurrentBlock->reverse_instructions);
        } else {
            CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
        }
    }
    if (q->branch_flags & 2) {
        if (instr->flags & 0x1000) {
            Operands_AllocateGPR(0x80000);
        } else {
            CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
        }
    }
    if (q->branch_flags & 4) {
        if (instr->flags & 0x8000) {
            Operands_AllocateGPR(0x40000);
        } else {
            CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
        }
    }
    if (q->branch_flags & 8) {
        if (instr->flags & 0x2000) {
            Operands_AllocateGPR(0x4000);
        } else {
            CError_ReportError(ERR_UNKNOWN_ASSEMBLER_INSTRUCTION_MNEMONIC);
        }
    }
    if (q->branch_flags & 0x10) {
        Operands_AllocateGPR(0x100000);
    }
    if (q->branch_flags & 0x20) {
        Operands_AllocateGPR(0x200000);
    }

    op = gPCodeOpcodeDescriptors[instr->opcode].encoding >> 26;
    if (op == 0x10) {
        found = NULL;
        r = PCode_NewLabel();
        switch (instr->opcode) {
            case PC_BC:
                if (instr->operandData.operands[3].kind == PCOp_LABEL)
                    found = instr->operandData.operands[3].value.label;
                break;
            case PC_BT:
            case PC_BF:
            case PC_BDNZT:
            case PC_BDNZF:
            case PC_BDZT:
            case PC_BDZF:
                if (instr->operandData.operands[2].kind == PCOp_LABEL)
                    found = instr->operandData.operands[2].value.label;
                break;
            case PC_BDNZ:
            case PC_BDZ:
                if (instr->operandData.operands[0].kind == PCOp_LABEL)
                    found = instr->operandData.operands[0].value.label;
                break;
            default:
                CError_FATAL(3664);
        }
        if (found != NULL) {
            PCode_AddSuccessor(gCurrentBlock, found);
            PCode_AddSuccessor(gCurrentBlock, r);
            PCode_CreateBlock();
            PCode_ResolveLabel(gCurrentBlock, r);
        }
    } else if (op == 0x12 && instr->operandData.operands[0].kind == PCOp_LABEL) {
        PCode_AddSuccessor(gCurrentBlock, instr->operandData.operands[0].value.label);
        PCode_CreateBlock();
    }
}

const char *InlineAsmPPC_GetOpcodeMnemonic(struct ParsedAsmInstruction *instruction)
{
    return gPCodeOpcodeDescriptors[instruction->opcode].mnemonic;
}

SInt32 fn_004631f0(ParsedAsmInstruction *operand)
{
    PCodeOpcodeDescriptor *descriptor = &gPCodeOpcodeDescriptors[operand->opcode];
    SInt32 opcode = operand->opcode;

    if (descriptor->flags & (fIsRead | fIsWrite)) {
        switch (opcode) {
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x28:
            case 0x29:
            case 0x2a:
            case 0x2b:
                return 1;
            case 0x19:
            case 0x1a:
            case 0x1b:
            case 0x1c:
            case 0x1d:
            case 0x1e:
            case 0x1f:
            case 0x20:
            case 0x21:
            case 0x2c:
            case 0x2d:
            case 0x2e:
            case 0x2f:
            case 0x30:
                return 2;
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x8e:
            case 0x8f:
            case 0x90:
            case 0x91:
            case 0x96:
            case 0x97:
            case 0x98:
            case 0x99:
            case 0xba:
            case 0xbd:
            case 0xc0:
            case 0xc1:
            case 0xc2:
            case 0xdb:
            case 0x1c7:
                return 4;
            case 0x27:
            case 0x36:
                if (operand->data.operands[0].kind == 2 && operand->data.operands[0].target.object == NULL)
                    return (0x20 - operand->data.operands[0].data.value) * 4;
                return 0x80;
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x9a:
            case 0x9b:
            case 0x9c:
            case 0x9d:
                return 8;
            case 0xbb:
            case 0xbe:
                return operand->data.operands[2].data.value;
            case 0xbc:
            case 0xbf:
                return 0x80;
            case 0xf2:
            case 0xf9:
                return 1;
            case 0xf3:
            case 0xfa:
                return 2;
            case 0xf4:
            case 0xfb:
                return 4;
            case 0xf5:
            case 0xf6:
            case 0xf7:
            case 0xf8:
            case 0xfc:
            case 0xfd:
                return 0x10;
            default:
                CError_FATAL(3843);
        }
    } else {
        if (descriptor->flags & 1)
            return 4;
        if (descriptor->flags & 2)
            return 8;
        if (descriptor->flags & 3)
            return 0x10;
        if (descriptor->flags & fSideEffects) {
            switch (opcode) {
                case 0xd3:
                case 0xd4:
                case 0xd5:
                    return 4;
                default:
                    CError_FATAL(3860);
            }
        }
    }
    CError_FATAL(3863);
    return 0;
}

void fn_00462d70(Statement *stmt, AsmOut *out)
{
    ParsedAsmInstruction *instruction = (ParsedAsmInstruction *)stmt->expr;
    EncodedOperand *operand;
    SInt32 operandIndex;
    PCodeOpcodeDescriptor *descriptor = &gPCodeOpcodeDescriptors[instruction->opcode];

    out->numoperands = 0;
    out->numlabels = 0;
    out->writesMemory = 0;
    out->readsMemory = 0;
    out->unmodeledControlFlow = 0;
    out->branchWithLink = 0;
    out->optimizationBarrier = 0;
    out->noFallthrough = 0;

    if (descriptor->flags & fIsPtrOp) {
        if (descriptor->flags & fIsRead)
            out->writesMemory = 1;
        if (descriptor->flags & fIsWrite)
            out->readsMemory = 1;
    }
    if (descriptor->flags & 0x2000) {
        if (instruction->branch_flags & 8)
            out->branchWithLink = 1;
        else if ((descriptor->flags & 0x20) || (descriptor->flags & fIsBranch))
            out->unmodeledControlFlow = 1;
        if (instruction->opcode == 0)
            out->noFallthrough = 1;
    }
    if ((descriptor->flags & fSideEffects) || !(descriptor->flags & 3))
        out->optimizationBarrier = 1;
    if (instruction->opcode == 2 && (instruction->branch_flags & 8))
        out->branchWithLink = 1;

    for (operandIndex = 0, operand = instruction->data.operands; operandIndex < instruction->operand_count;
         operandIndex++, operand++) {
        unsigned char operandKind = operand->kind;
        switch (operandKind) {
            case 0:
            case 1:
                break;
            case 2:
                if (operand->target.object != NULL) {
                    SInt16 registerFlags = operand->modifier.reg.flags;
                    if (registerFlags & 1) {
                        SInt8 objectType;
                        out->operands[out->numoperands].type = 0;
                        out->operands[out->numoperands].object = operand->target.object;
                        out->operands[out->numoperands].offset = 0;
                        objectType = operand->target.object->type->type;
                        switch (objectType) {
                            case TYPEINT:
                            case TYPEPOINTER:
                                out->operands[out->numoperands].size = 4;
                                break;
                            case TYPEFLOAT:
                                out->operands[out->numoperands].size = 8;
                                break;
                            case TYPESTRUCT:
                                switch (((TypeStruct *)operand->target.object->type)->stype) {
                                    case STRUCT_VECTOR_UCHAR:
                                    case STRUCT_VECTOR_SCHAR:
                                    case STRUCT_VECTOR_BCHAR:
                                    case STRUCT_VECTOR_USHORT:
                                    case STRUCT_VECTOR_SSHORT:
                                    case STRUCT_VECTOR_BSHORT:
                                    case STRUCT_VECTOR_UINT:
                                    case STRUCT_VECTOR_SINT:
                                    case STRUCT_VECTOR_BINT:
                                    case STRUCT_VECTOR_FLOAT:
                                    case STRUCT_VECTOR_PIXEL:
                                        out->operands[out->numoperands].size = 0x10;
                                        break;
                                    default:
                                        CError_FATAL(3957);
                                }
                                break;
                            default:
                                CError_FATAL(3961);
                        }
                        out->numoperands++;
                    }
                    registerFlags = operand->modifier.reg.flags;
                    if (registerFlags & 2) {
                        SInt8 objectType;
                        out->operands[out->numoperands].type = 1;
                        out->operands[out->numoperands].object = operand->target.object;
                        out->operands[out->numoperands].offset = 0;
                        objectType = operand->target.object->type->type;
                        switch (objectType) {
                            case TYPEINT:
                            case TYPEPOINTER:
                                out->operands[out->numoperands].size = 4;
                                break;
                            case TYPEFLOAT:
                                out->operands[out->numoperands].size = 8;
                                break;
                            case TYPESTRUCT:
                                switch (((TypeStruct *)operand->target.object->type)->stype) {
                                    case STRUCT_VECTOR_UCHAR:
                                    case STRUCT_VECTOR_SCHAR:
                                    case STRUCT_VECTOR_BCHAR:
                                    case STRUCT_VECTOR_USHORT:
                                    case STRUCT_VECTOR_SSHORT:
                                    case STRUCT_VECTOR_BSHORT:
                                    case STRUCT_VECTOR_UINT:
                                    case STRUCT_VECTOR_SINT:
                                    case STRUCT_VECTOR_BINT:
                                    case STRUCT_VECTOR_FLOAT:
                                    case STRUCT_VECTOR_PIXEL:
                                        out->operands[out->numoperands].size = 0x10;
                                        break;
                                    default:
                                        CError_FATAL(3993);
                                }
                                break;
                            default:
                                CError_FATAL(3997);
                        }
                        out->numoperands++;
                    }
                }
                break;
            case 3:
            case 4:
                if (operand->target.object != NULL) {
                    if (descriptor->flags & fIsRead) {
                        out->operands[out->numoperands].type = 0;
                        out->operands[out->numoperands].object = operand->target.object;
                        out->operands[out->numoperands].offset = operand->data.value;
                        out->operands[out->numoperands].size = fn_004631f0(instruction);
                        out->numoperands++;
                    } else if (descriptor->flags & fIsWrite) {
                        out->operands[out->numoperands].type = 1;
                        out->operands[out->numoperands].object = operand->target.object;
                        out->operands[out->numoperands].offset = operand->data.value;
                        out->operands[out->numoperands].size = fn_004631f0(instruction);
                        out->numoperands++;
                    } else {
                        out->operands[out->numoperands].type = 3;
                        out->operands[out->numoperands].object = operand->target.object;
                        out->operands[out->numoperands].offset = operand->data.value;
                        out->operands[out->numoperands].size = fn_004631f0(instruction);
                        out->numoperands++;
                    }
                }
                break;
            case 5:
                out->labels[out->numlabels] = operand->data.label;
                out->numlabels++;
                break;
            case 6:
                out->labels[out->numlabels] = operand->data.label;
                out->numlabels++;
                out->labels[out->numlabels] = operand->target.label;
                out->numlabels++;
                out->unmodeledControlFlow = 1;
                break;
            default:
                CError_FATAL(4039);
        }
    }
    if ((descriptor->flags & 0x24) && out->numlabels == 0)
        out->unmodeledControlFlow = 1;
}

void InlineAsmPPC_ReplaceObjectReferenceArguments(Statement *owner, Object *object, ENode *expr)
{
    ParsedAsmInstruction *list = (ParsedAsmInstruction *)owner->expr;
    Object *referencedObject;
    int i;

    if (expr->type == EOBJREF) {
        referencedObject = expr->data.objref;
        if (object->otype == referencedObject->otype && object->datatype == referencedObject->datatype) {
            for (i = 0; i < list->operand_count; i++) {
                switch ((UInt8)list->data.operands[i].kind) {
                    case 2:
                        ReplaceArg(&list->data.operands[i], object, referencedObject);
                        break;
                    case 3:
                    case 4:
                        ReplaceArg(&list->data.operands[i], object, referencedObject);
                        break;
                }
            }
        }
    }
}

static void ReplaceArg(EncodedOperand *e, Object *b, Object *q)
{
    if (e->target.object == b)
        e->target.object = q;
}

Statement *InlineAsmPPC_CopyStatement(Statement *stmt)
{
    Statement *result;
    SInt32 size;
    ParsedAsmInstruction *copy;
    ParsedAsmInstruction *instruction;

    result = galloc(sizeof(Statement));
    *result = *stmt;
    instruction = (ParsedAsmInstruction *)stmt->expr;
    size = offsetof(ParsedAsmInstruction, data) + instruction->operand_count * sizeof(EncodedOperand);
    copy = galloc(size);
    memcpy(copy, instruction, size);
    result->expr = (ENode *)copy;
    return result;
}
