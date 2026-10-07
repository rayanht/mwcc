#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/InlineAsm.h"
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
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/COptimizer.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
/* Inline-assembly operand resolved from a symbol name. */

/* Operand and label records used by the inline assembler. */
#include <string.h>
#include <setjmp.h>
#include <stdio.h>

static char inlineasm_instruction_buffer[1024];

SInt32 data_005652f8 = 1;
typedef struct _res res;

static inline void expectToken(SInt16 tok, SInt16 err)
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

void InlineAsm_LongJump(void)
{
    longjmp(inlineAsmJmpBuf, 1);
}

void InlineAsm_Error(short errorCode)
{
    if (data_00587f18 != 0) {
        longjmp(data_00583a68, 1);
    }
    if ((tk == TK_EOL) || (tk == ';')) {
        errorCode = 0x70;
    }
    CError_ReportError(errorCode);
}

CLabel *InlineAsm_CreateLabel(HashNameNode *name)
{
    CLabel *record;
    record = (CLabel *)newlabel();
    record->name = name;
    record->next = clabels;
    clabels = record;
    return record;
}

unsigned char InlineAsm_ResolveOperandName(HashNameNode *name, struct AsmOperand *operand, char allow_kind2)
{
    CLabel *label;
    NameSpace *scope;
    NameSpaceObjectList *lookup;
    Object *object;
    ObjEnumConst *enumConstant;
    ObjType *typeObject;

    operand->name = name;
    operand->object = NULL;
    operand->label = NULL;
    operand->value = NULL;
    operand->is_register = 0;
    label = clabels;
    while (label != NULL) {
        if (name == label->name) {
            break;
        }
        label = label->next;
    }
    operand->label = label;
    if (operand->label != NULL) {
        return 1;
    }
    scope = cscope_current;
    while (scope != NULL) {
        lookup = CScope_FindName(scope, name);
        if (lookup != NULL) {
            switch (lookup->object->otype) {
                case OT_ENUMCONST:
                    operand->is_register = 1;
                    enumConstant = (ObjEnumConst *)lookup->object;
                    operand->register_number = enumConstant->val.lo;
                    return 1;
                case OT_OBJECT:
                    object = (Object *)lookup->object;
                    if (object->datatype == DABSOLUTE) {
                        operand->is_register = 1;
                        operand->register_number = object->u.data.u.intconst.hi;
                    } else {
                        if (object->datatype == DDATA && (object->qual & Q_INLINE_DATA) != 0) {
                            CInit_ExportConst(object);
                        }
                        operand->object = object;
                    }
                    return 1;
                case OT_TYPE:
                    typeObject = (ObjType *)lookup->object;
                    operand->value = typeObject->type;
                    return 1;
                case OT_TYPETAG:
                    if (allow_kind2 != 0) {
                        typeObject = (ObjType *)lookup->object;
                        operand->value = typeObject->type;
                        return 1;
                    }
                case OT_NAMESPACE:
                case OT_MEMBERVAR:
                    return 0;
                default:
                    CError_Internal("InlineAsm.c", 233);
            }
        }
        scope = scope->parent;
    }
    return 0;
}

Boolean InlineAsm_ResolveOperandNameDefault(HashNameNode *name, struct AsmOperand *result)
{
    return InlineAsm_ResolveOperandName(name, result, '\0');
}

SInt32 InlineAsm_ParseMemberOffset(Type *type)
{
    SInt32 offset = 0;
    short error;
    ObjMemberVar *member;
    NameSpaceObjectList *objects;
    int isMember;
    struct StructMember *record;

    do {
        if (type->type == TYPESTRUCT) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                error = 0x6b;
                if (data_00587f18 != 0)
                    longjmp(data_00583a68, 1);
                if (tk == TK_EOL || tk == ';')
                    error = 0x70;
                CError_ReportError(error);
            }
            record = ismember(type, data_00587fa0);
            if (record == NULL)
                CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
            offset += record->offset;
            type = record->type;
            tk = CPrepTokenizer_GetNextToken();
        } else if (type->type == TYPECLASS) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                error = 0x6b;
                if (data_00587f18 != 0)
                    longjmp(data_00583a68, 1);
                if (tk == TK_EOL || tk == ';')
                    error = 0x70;
                CError_ReportError(error);
            }
            objects = CScope_FindName(((TypeClass *)type)->nspace, data_00587fa0);
            isMember = 0;
            if (objects != NULL && objects->object->otype == OT_MEMBERVAR)
                isMember = 1;
            if ((member = isMember ? (ObjMemberVar *)objects->object : (ObjMemberVar *)0) == NULL)
                CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
            offset += member->offset;
            type = member->type;
            tk = CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS);
        }
    } while (tk == '.');

    return offset;
}

SInt32 InlineAsm_ParseMemberArrayOffset(Type *type)
{
    SInt32 sum = 0;

    for (;;) {
        if (tk == '.') {
            if (type->type == TYPESTRUCT) {
                StructMember *member;
                tk = CPrepTokenizer_GetNextToken();
                expectToken(-3, 0x6b);
                member = ismember(type, data_00587fa0);
                if (member == NULL)
                    CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
                sum += member->offset;
                type = member->type;
                tk = CPrepTokenizer_GetNextToken();
            } else if (type->type == TYPECLASS) {
                NameSpaceObjectList *objects;
                ObjMemberVar *member;
                SInt32 found;
                tk = CPrepTokenizer_GetNextToken();
                expectToken(-3, 0x6b);
                objects = CScope_FindName(TYPE_CLASS(type)->nspace, data_00587fa0);
                found = 0;
                if (objects != NULL && objects->object->otype == OT_MEMBERVAR)
                    found = 1;
                if ((member = found ? OBJ_MEMBER_VAR(objects->object) : (ObjMemberVar *)0) == NULL)
                    CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
                sum += member->offset;
                type = member->type;
                tk = CPrepTokenizer_GetNextToken();
            } else {
                CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS);
            }
        } else if (type->type == TYPEARRAY) {
            type = TPTR_TARGET(type);
            tk = CPrepTokenizer_GetNextToken();
            sum += scan_expression() * type->size;
            expectToken(0x5d, 0x7d);
            tk = CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportError(ERR_POINTER_ARRAY_REQUIRED);
        }
        if (tk != '.' && tk != '[')
            return sum;
    }
}

SInt32 InlineAsm_ParseStructOrClassMemberOffset(Type *obj)
{
    SInt16 err;
    SInt32 found;
    StructMember *smem;
    NameSpaceObjectList *list;
    ObjMemberVar *mvar;
    SInt32 offset;
    Type *type;
    Type *search;
    TypeClass *tclass;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        err = 0x6b;
        if (data_00587f18 != 0)
            longjmp(data_00583a68, 1);
        if (tk == TK_EOL || tk == ';')
            err = 0x70;
        CError_ReportError(err);
    }
    if (obj->type == TYPESTRUCT) {
        search = obj;
        smem = ismember(search, data_00587fa0);
        if (smem == NULL)
            CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
        offset = smem->offset;
        type = smem->type;
    } else {
        tclass = (TypeClass *)obj;
        list = CScope_FindName(tclass->nspace, data_00587fa0);
        found = 0;
        if (list != NULL && list->object->otype == OT_MEMBERVAR)
            found = 1;
        if ((mvar = found ? (ObjMemberVar *)list->object : NULL) == NULL)
            CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, data_00587fa0->name);
        offset = mvar->offset;
        type = mvar->type;
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '.' || tk == '[')
        offset = offset + InlineAsm_ParseMemberArrayOffset(type);
    return offset;
}

int scan_unary_expression(void)
{
    struct AsmOperand v;
    SInt16 s;
    int t;

    switch (tk) {
        case TK_IDENTIFIER:
            if (InlineAsm_ResolveOperandName(data_00587fa0, &v, 0)) {
                if (v.is_register) {
                    tk = CPrepTokenizer_GetNextToken();
                    return v.register_number;
                }
                if (v.value != NULL && (v.value->type == TYPESTRUCT || v.value->type == TYPECLASS)) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk != '.') {
                        s = 0x78;
                        if (data_00587f18)
                            longjmp(data_00583a68, 1);
                        if (tk == TK_EOL || tk == ';')
                            s = 0x70;
                        CError_ReportError(s);
                    }
                    if (data_005652f8 != 0)
                        return InlineAsm_ParseMemberArrayOffset(v.value);
                    return InlineAsm_ParseMemberOffset(v.value);
                }
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                break;
            }
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, data_00587fa0->name);
            break;
        case TK_INTCONST:
            t = intconst_lo;
            tk = CPrepTokenizer_GetNextToken();
            return t;
        case TK_SIZEOF:
            return scansizeof();
        case '+':
            tk = CPrepTokenizer_GetNextToken();
            return scan_unary_expression();
        case '-':
            tk = CPrepTokenizer_GetNextToken();
            return -scan_unary_expression();
        case TK_NOT:
            tk = CPrepTokenizer_GetNextToken();
            return !scan_unary_expression();
        case TK_COMPL:
            tk = CPrepTokenizer_GetNextToken();
            return ~scan_unary_expression();
        case '(':
            tk = CPrepTokenizer_GetNextToken();
            t = scan_expression();
            if (tk != ')') {
                s = 0x73;
                if (data_00587f18)
                    longjmp(data_00583a68, 1);
                if (tk == TK_EOL || tk == ';')
                    s = 0x70;
                CError_ReportError(s);
            }
            tk = CPrepTokenizer_GetNextToken();
            return t;
        default:
            s = 0x78;
            if (data_00587f18)
                longjmp(data_00583a68, 1);
            if (tk == TK_EOL || tk == ';')
                s = 0x70;
            CError_ReportError(s);
            break;
    }
    return 0;
}

SInt32 evaluate_binary_expression(SInt32 left)
{
    SInt32 right;
    SInt32 res;
    SInt16 op;
    SInt16 prec;
    for (;;) {
        op = tk;
        tk = CPrepTokenizer_GetNextToken();
        right = scan_unary_expression();
        prec = GetPrec(tk);
        if (prec == 0) {
            CInt64 rhs, lhs;
            lhs.lo = left;
            lhs.hi = (left < 0) ? -1 : 0;
            rhs.lo = right;
            rhs.hi = (right < 0) ? -1 : 0;
            rhs = CMach_CalcIntDiadic((Type *)&stsignedint, lhs, op, rhs);
            return rhs.lo;
        }
        if (GetPrec(op) >= prec) {
            CInt64 rhs, lhs;
            lhs.lo = left;
            lhs.hi = (left < 0) ? -1 : 0;
            rhs.lo = right;
            rhs.hi = (right < 0) ? -1 : 0;
            rhs = CMach_CalcIntDiadic((Type *)&stsignedint, lhs, op, rhs);
            left = rhs.lo;
        } else {
            CInt64 rhs, lhs;
            right = evaluate_binary_expression(right);
            lhs.lo = left;
            lhs.hi = (left < 0) ? -1 : 0;
            rhs.lo = right;
            rhs.hi = (right < 0) ? -1 : 0;
            rhs = CMach_CalcIntDiadic((Type *)&stsignedint, lhs, op, rhs);
            res = left = rhs.lo;
            if (GetPrec(tk) == 0)
                return res;
        }
    }
}

unsigned int scan_expression(void)
{
    unsigned int result;

    result = scan_unary_expression();
    if (GetPrec(tk) != 0) {
        result = evaluate_binary_expression(result);
    }
    return result;
}

void parse_label(void)
{
    if (data_00587fa0->name[0] == '@') {
        FindOrAdd(data_00587fa0);
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ':')
            tk = CPrepTokenizer_GetNextToken();
    } else {
        HashNameNode *save = data_00587fa0;
        UInt16 ch = CPrepTokenizer_GetNextTokenAndRestorePosition();
        data_00587fa0 = save;
        if (ch != ':')
            return;
        FindOrAdd(data_00587fa0);
        tk = CPrepTokenizer_GetNextToken();
        tk = CPrepTokenizer_GetNextToken();
    }
}

void parse_asm_lines(volatile SInt16 endToken, int parseOption)
{
    if (setjmp(inlineAsmJmpBuf) != 0) {
        while (tk != TK_EOL && tk != endToken && tk != '}' && tk != 0)
            tk = CPrepTokenizer_GetNextToken();
        if (tk == ';' || tk == TK_EOL)
            tk = CPrepTokenizer_GetNextToken();
        return;
    }
    InlineAsmPPC_Init(parseOption);
    if (setjmp(inlineAsmJmpBuf) != 0) {
        while (tk != ';' && tk != TK_EOL && tk != endToken && tk != '}' && tk != 0)
            tk = CPrepTokenizer_GetNextToken();
        if (tk == ';' || tk == TK_EOL)
            tk = CPrepTokenizer_GetNextToken();
    }
    while (tk != 0 && tk != endToken) {
        data_00587f18 = 0;
        if (tk == '.') {
            InlineAsmPPC_ParseDirectiveIdentifier();
        } else if (tk == TK_IDENTIFIER) {
            parse_label();
            if (tk == TK_IDENTIFIER)
                InlineAsmPPC_ParseInstruction();
        }
        if (tk == ';' || tk == TK_EOL) {
            CPrep_ResetBufferedTokenPosition();
            tk = CPrepTokenizer_GetNextToken();
        } else if (tk != endToken) {
            if (endToken == 0x29)
                CError_ReportError(ERR_RPAREN_EXPECTED);
            else
                CError_ReportError(ERR_END_LINE_EXPECTED);
        }
    }
}

void InlineAsm_ParseAsmLines(short value)
{
    parse_asm_lines(value, 1);
}

void InlineAsm_ParseAsmStatement(void)
{
    short endToken;
    if (tk == '(')
        endToken = 0x29;
    else
        endToken = 0x7d;
    data_0058850d = tk == '{';
    data_005884fd = 1;
    tk = CPrepTokenizer_GetNextToken();
    parse_asm_lines(endToken, '\0');
    data_005884fd = 0;
    data_0058850d = 0;
}

void InlineAsm_CopyAndRemapParsedAsmInstruction(Statement *owner, Statement *links, ParsedAsmInstruction **result,
                                                UInt32 *resultSize)
{
    long source = (long)owner;
    ParsedAsmInstruction *table = ((Statement *)source)->expr.asmInstruction;
    SInt32 size = table->operand_count * sizeof(EncodedOperand) + offsetof(ParsedAsmInstruction, data);
    ParsedAsmInstruction *copy = (ParsedAsmInstruction *)galloc(size);
    int entryIndex;
    EncodedOperand *entry;
    memcpy(copy, table, size);
    entryIndex = 0;
    entry = copy->data.operands;
    for (; entryIndex < copy->operand_count; ++entryIndex, ++entry) {
        switch ((unsigned char)entry->kind) {
            case 1:
                break;
            case 2:
            case 4:
                entry->target.object = (Object *)CInline_GetObjectIndex(entry->target.object);
                break;
            case 3:
                break;
            case 5:
                entry->data.value = CInline_GetStatementIndex(links, entry->data.label->target.stmt);
                break;
            case 6:
                entry->data.value = CInline_GetStatementIndex(links, entry->data.label->target.stmt);
                entry->target.object = (Object *)CInline_GetStatementIndex(links, entry->target.label->target.stmt);
                break;
        }
    }
    *result = copy;
    *resultSize = size;
}

void InlineAsm_CopyInstructionAndResolveOperands(Statement *output, struct CLabel **references, char useTable,
                                                 ParsedAsmInstruction *source, SInt32 size)
{
    ParsedAsmInstruction *instruction;
    SInt32 index;
    EncodedOperand *operand;

    instruction = galloc(size);
    memcpy(instruction, source, size);
    for (index = 0, operand = instruction->data.operands; index < instruction->operand_count; index++, operand++) {
        unsigned char kind = operand->kind;
        switch (kind) {
            case 1:
                break;
            case 2:
            case 4:
                operand->target.object = CInline_GetObjectByIndex((UInt32)operand->target.object, useTable);
                break;
            case 3:
                break;
            case 5: {
                SInt16 labelIndex = operand->data.value;
                operand->data.label = references[labelIndex];
                break;
            }
            case 6: {
                SInt16 labelIndex = operand->data.value;
                operand->data.label = references[labelIndex];
                operand->target.label = references[(SInt16)operand->target.label];
                break;
            }
        }
    }
    output->expr.asmInstruction = instruction;
}

void InlineAsm_RecordObjectUses(Statement *record)
{
    ParsedAsmInstruction *list;
    int index;
    EncodedOperand *operand;

    list = record->expr.asmInstruction;
    index = 0;
    operand = list->data.operands;
    while (index < list->operand_count) {
        switch ((unsigned char)operand->kind) {
            case 2:
                if (operand->target.object != NULL) {
                    COptimizer_RecordObjectUse(operand->target.object, 0);
                }
                break;
            case 4:
                COptimizer_RecordObjectUse(operand->target.object, 1);
                break;
        }
        index = index + 1;
        operand = operand + 1;
    }
}

CLabel *InlineAsm_FindOperandLabel(Statement *object)
{
    ParsedAsmInstruction *table;
    EncodedOperand *entry;
    int index = 0;

    table = object->expr.asmInstruction;
    entry = table->data.operands;
    for (; index < table->operand_count; index++, ++entry) {
        if (entry->kind == 5)
            return (CLabel *)entry->data.label;
        if (entry->kind == 6)
            return (CLabel *)entry->data.label;
    }
    return NULL;
}

void *InlineAsm_GetOperandLabel(Statement *owner)
{
    EncodedOperand *operand;
    ParsedAsmInstruction *instruction;
    int index;

    index = 0;
    instruction = owner->expr.asmInstruction;
    operand = instruction->data.operands;
    while (index < instruction->operand_count) {
        if (operand->kind == 6)
            return operand->target.label;
        index++;
        operand++;
    }
    return NULL;
}

Object *InlineAsm_GetObjectByIndex(ParsedAsmInstruction *t, SInt32 index, SInt32 *offset)
{
    char *base;
    EncodedOperand *e;
    int i;
    int n;

    base = (char *)t;
    for (i = 0, n = 0, e = t->data.operands; i < t->operand_count; i++, e++) {
        if (e->kind == 3 && n++ == index) {
            *offset = (char *)&e->target.object - base;
            return (Object *)e->target.object;
        }
    }
    return NULL;
}

char *format_inlineasm_instruction(ENode *info)
{
    char buffer[1024];
    ENode *node = info;
    struct ParsedAsmInstruction *instruction =
        (struct ParsedAsmInstruction *)((ENode *)(void *)node)->data.inlineasm.info;
    EncodedOperand *operand;
    SInt32 i;

    strcpy(inlineasm_instruction_buffer, "\"");
    strcat(inlineasm_instruction_buffer, InlineAsmPPC_GetOpcodeMnemonic(instruction));
    strcat(inlineasm_instruction_buffer, "\"");

    for (i = 0, operand = instruction->data.operands; i < instruction->operand_count; i++, operand++) {
        switch ((UInt8)operand->kind) {
            case 1:
                sprintf(buffer, " imm(%ld)", operand->data.value);
                break;
            case 2:
                if (operand->target.object)
                    sprintf(buffer, " reg(%s)", operand->target.object->name->name);
                else
                    sprintf(buffer, " reg(%d)", operand->data.value);
                break;
            case 3:
            case 4:
                if (operand->data.value > 0)
                    sprintf(buffer, " obj(%s+%ld)", operand->target.object->name->name, operand->data.value);
                else if (operand->data.value < 0)
                    sprintf(buffer, " obj(%s-%ld)", operand->target.object->name->name, -operand->data.value);
                else
                    sprintf(buffer, " obj(%s)", operand->target.object->name->name);
                break;
            case 5:
                sprintf(buffer, " lab(%s)", operand->data.label->uniquename->name);
                break;
            case 6: {
                SInt32 offset;
                if (!operand->negative)
                    offset = 0;
                else
                    offset = operand->modifier.value;
                sprintf(buffer, " labdiff(%s-%s%c%d)", operand->data.label->uniquename->name,
                        operand->target.label->uniquename->name, (operand->negative == 1) ? '-' : '+', offset);
                break;
            }
        }
        strcat(inlineasm_instruction_buffer, buffer);
    }
    return inlineasm_instruction_buffer;
}
