#define CERROR_FILE "CInit.c"
#include "compiler/common.h"
#include "compiler/CInit.h"
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
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/FuncLevelAsmPPC.h"
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
#include "compiler/Objects.h"
#include "compiler/ENode.h"
#include "compiler/Types.h"
#include <string.h>

static struct PooledString *string_cache;
static struct NameEntry *pooled_strings;
static struct NameEntry *pooled_wstrings;
static struct InitListItem *tentative_init_list;
static struct ENodeList *data_00581ba0;
static UInt8 data_00581ba4;

enum { TYPESTRUCT_004d2700 = 4, TYPEARRAY_004d2700 = 12 };

enum InitKind { IK_ONE = 1, IK_TWO = 2 };

typedef enum { BF_FALSE, BF_TRUE } BoolFlag;

static inline UInt8 CInit_StringEmissionMode(void)
{
    return copts.readonly_strings;
}

static Boolean needs_init(TypeClass *x)
{
    return CClass_Constructor(x) || CClass_Destructor(x);
}

static inline void CInitPushSave(InitInfo *save, Object *obj)
{
    memclrw(save, sizeof(*save));
    save->obj = obj;
    save->next = cinit_state;
    cinit_state = save;
}

static inline char classify(Type *type)
{
    short needed;
    char result;
    switch ((char)type->type) {
        case TYPESTRUCT:
            result = 1;
            break;
        case TYPEARRAY:
            while (type->type == TYPEARRAY)
                type = ((TypePointer *)type)->target;
            if (type->type != TYPECLASS) {
                result = 1;
                break;
            }
        case TYPECLASS:
            needed = 1;
            if (CClass_Constructor((TypeClass *)type) == NULL && CClass_Destructor((TypeClass *)type) == NULL)
                needed = 0;
            result = !(char)needed;
            break;
        default:
            result = 0;
    }
    return result;
}

static inline void CInit_SetupSave(InitInfo *save, Object *obj, void (*emit)(ENode *),
                                   void (*destruct)(Type *, Object *, SInt32, SInt32))
{
    memclrw(save, sizeof(*save));
    save->obj = obj;
    save->next = cinit_state;
    cinit_state = save;
    save->emitObject = obj;
    save->insert_expr_cb = emit;
    save->register_object_cb = (ENode * (*)(Type *, Object *, SInt32, SInt32)) destruct;
}

#ifndef TRUE
#endif
#ifndef FALSE
#endif

static Boolean CInit_IsZero(UInt8 *p, SInt32 n)
{
    SInt32 i;
    if (copts.explicit_zero_data)
        return FALSE;
    for (i = 0; i < n; i++)
        if (p[i] != 0)
            return FALSE;
    return TRUE;
}

static Boolean CInit_IsDtorTemp(ENode *e)
{
    return e->type == EPRECOMP && e->data.temp.needs_dtor;
}

static Object *CreateTempObject(Type *type)
{
    DeclInfo s;
    Object *obj;

    memclrw(&s, sizeof(s));
    s.thetype = type;
    s.name = CParser_GetUniqueName();
    s.qual = 0;
    s.storageclass = 0x102;
    s.requireMangledName = 1;
    obj = CParser_NewObject(&s);
    obj->nspace = cscope_root;
    CodeGen_SetObjectSectionAndInterruptInfo(obj);
    emit_object(obj, NULL, NULL, obj->type->size, 0);
    return obj;
}

static Boolean IsComplexClass(Type *type)
{
    return CClass_Constructor((TypeClass *)type) || CClass_Destructor((TypeClass *)type);
}

static void InitBuffer(SInt32 size)
{
    cinit_state->bufferSize = size;
    if (size == 0)
        size = 0x200;
    else if (size & 1)
        size++;
    cinit_state->buffer = CompilerTools_AllocatePool(size);
    cinit_state->bufferUsed = size;
    memclrw(cinit_state->buffer, size);
}

static Boolean IsZeroed(UInt8 *buf, SInt32 size)
{
    SInt32 i;

    if (copts.explicit_zero_data)
        return 0;
    for (i = 0; i < size; i++) {
        if (buf[i])
            return 0;
    }
    return 1;
}

static Object *CreateObject(Type *type, SInt32 qual)
{
    DeclInfo rec;
    Object *obj;

    memclrw(&rec, sizeof(rec));
    rec.thetype = type;
    rec.name = CParser_GetUniqueName();
    rec.qual = qual;
    rec.storageclass = 0x102;
    rec.requireMangledName = 1;
    obj = CParser_NewObject(&rec);
    obj->nspace = cscope_root;
    return obj;
}

inline void InitExprWrap(Type *pt, ENode *node, Boolean flag)
{
    CInit_004d1d90(pt, node);
}

static void CInit_DefaultInit(Type *type, ENode *node)
{
    if (cinit_state->expr_cb != NULL) {
        (*cinit_state->expr_cb)(type, node, 0);
        cinit_state->expr_cb_called = 1;
    } else {
        CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
    }
}

static inline UInt8 initializer_uses_constructor_syntax(const CInit *initializer)
{
    return initializer->usesConstructorSyntax;
}

static inline UInt8 initializer_is_parenthesized(const CInit *initializer)
{
    return initializer->parenthesized;
}

static inline void initializer_set_parenthesized(CInit *initializer, UInt8 parenthesized)
{
    initializer->parenthesized = parenthesized;
}

void CInit_Init(void)
{
    data_0058757c = NULL;
    cinit_state = NULL;
    string_cache = NULL;
    pooled_strings = NULL;
    pooled_wstrings = NULL;
    tentative_init_list = NULL;
}

void write_buffer_at_offset(void *src, SInt32 offset, SInt32 size)
{
    SInt32 needed = offset + size;
    UInt8 *nb;
    struct InitInfo *buf;

    buf = cinit_state;
    if (needed > buf->bufferSize) {
        buf = cinit_state;
        buf->bufferSize = needed;
    }
    buf = cinit_state;
    if (needed > buf->bufferUsed) {
        Type *tp;
        struct InitInfo *ctx = cinit_state;
        Object *obj;
        tp = (obj = (ctx = cinit_state)->obj)->type;
        if (tp->size == 0) {
            if (needed < 8000)
                needed += 0x400;
            else
                needed += 0x4000;
        }
        if (needed & 1)
            needed++;
        nb = (UInt8 *)CompilerTools_AllocatePool(needed);
        memclrw(nb, needed);
        {
            struct InitInfo *ctx = cinit_state;
            buf = cinit_state;
            memcpy(nb, (buf = cinit_state)->buffer, (ctx = cinit_state)->bufferUsed);
        }
        buf = cinit_state;
        buf->buffer = nb;
        buf = cinit_state;
        buf->bufferUsed = needed;
    }
    if (src) {
        buf = cinit_state;
        memcpy(buf->buffer + offset, src, size);
    }
}

ENode *parse_initializer_expression(ENode *node)
{
    switch (tk) {
        case TK_INTCONST:
        case TK_FLOATCONST: {
            CInt64 savedInteger = token_integer;
            Float savedFloat;
            SInt32 savedOther;
            SInt16 token;
            savedFloat = token_float;
            savedOther = token_value_kind_or_string_length;
            token = CPrepTokenizer_GetNextTokenAndRestorePosition();

            token_integer = savedInteger;
            token_float = savedFloat;
            token_value_kind_or_string_length = savedOther;

            switch (token) {
                case ',':
                case ';':
                case '}':
                    memclrw(node, sizeof(*node));
                    switch (tk) {
                        case TK_INTCONST:
                            node->type = EINTCONST;
                            node->rtype = (Type *)atomtype();
                            node->data.intval = token_integer;
                            break;
                        case TK_FLOATCONST:
                            node->type = EFLOATCONST;
                            node->rtype = (Type *)atomtype();
                            node->data.floatval = token_float;
                            break;
                    }
                    tk = CPrepTokenizer_GetNextToken();
                    CPrep_ResetBufferedTokenPosition();
                    return node;
            }
            break;
        }
    }
    node = conv_assignment_expression();
    CPrep_ResetBufferedTokenPosition();
    return node;
}

UInt8 advance_initializer_state(CInit *initializer)
{
    DeclInfo declaration;

    initializer->expr = NULL;
    if (tk == ';') {
        initializer->state = 4;
        return initializer->state;
    }
    switch (initializer->state) {
        case 0:
            if (initializer_uses_constructor_syntax(initializer) != 0) {
                if (tk == '(') {
                    tk = CPrepTokenizer_GetNextToken();
                    CParser_GetDeclSpecs(&declaration, 1);
                    if (tk == ')') {
                        tk = CPrepTokenizer_GetNextToken();
                    } else {
                        CError_ReportError(ERR_RPAREN_EXPECTED);
                    }
                    if (tk == '(') {
                        tk = CPrepTokenizer_GetNextToken();
                    } else {
                        CError_ReportError(ERR_LPAREN_EXPECTED);
                    }
                    initializer_set_parenthesized(initializer, 1);
                    initializer->state = 1;
                    return initializer->state;
                }
            } else if (tk == '{') {
                tk = CPrepTokenizer_GetNextToken();
                initializer_set_parenthesized(initializer, 0);
                initializer->state = 1;
                return initializer->state;
            }
            initializer->expr = parse_initializer_expression(&initializer->exprbuf);
            initializer->state = 2;
            return initializer->state;

        case 1:
            break;

        case 2:
        case 3:
            if (tk == ',') {
                tk = CPrepTokenizer_GetNextToken();
            } else {
                if (initializer_is_parenthesized(initializer) != 0) {
                    if (tk != ')') {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                } else {
                    if (tk != '}') {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                }
                initializer->state = 3;
                return initializer->state;
            }
            break;

        default:
            CError_FATAL(368);
    }

    switch (tk) {
        case '{':
            tk = CPrepTokenizer_GetNextToken();
            initializer->state = 1;
            return initializer->state;
        case '}':
            initializer->state = 3;
            return initializer->state;
        case '(':
            if (initializer_uses_constructor_syntax(initializer) != 0) {
                tk = CPrepTokenizer_GetNextToken();
                initializer->state = 1;
                return initializer->state;
            }
            /* fall through */
        case ')':
            if (initializer_uses_constructor_syntax(initializer) != 0 &&
                initializer_is_parenthesized(initializer) != 0) {
                initializer->state = 3;
                return initializer->state;
            }
            break;
        default:
            break;
    }
    initializer->expr = parse_initializer_expression(&initializer->exprbuf);
    initializer->state = 2;
    return initializer->state;
}

Boolean CInit_004d3ba0(Type *type)
{
    switch ((SInt8)type->type) {
        case TYPESTRUCT:
            return 1;
        case TYPEARRAY:
            while (type->type == TYPEARRAY)
                type = TPTR_TARGET(type);
            if (type->type != TYPECLASS)
                return 1;
        case TYPECLASS:
            return !(Boolean)(CClass_Constructor((TypeClass *)type) || CClass_Destructor((TypeClass *)type));
        default:
            return 0;
    }
}

Boolean CInit_004d3b20(Type *type)
{
    Boolean flag;

    switch (*(SInt8 *)type) {
        case TYPEPOINTER:
            return (TYPE_POINTER(type)->qual & Q_REFERENCE) == 0;
        case TYPEARRAY:
            while (type->type == TYPEARRAY)
                type = TYPE_POINTER(type)->target;
            if (type->type != TYPECLASS)
                return 1;
            /* fall through */
        case TYPECLASS:
            flag = CClass_Constructor((TypeClass *)type) || CClass_Destructor((TypeClass *)type);
            return !flag;
        default:
            return 1;
    }
}

void append_initializer_entry(InitializerData *ctx, Type *type, ENode *expr)
{
    SInt32 kind;
    if (ctx->owner->unknown9 != 0 ||
        (type->type == TYPESTRUCT && (kind = (SInt8)TYPE_STRUCT(type)->stype) >= 4 && kind <= 0x0e)) {
        InitializerEntry *node = (InitializerEntry *)CompilerTools_AllocatePool(sizeof(InitializerEntry));
        memclrw(node, sizeof(InitializerEntry));
        node->next = NULL;
        node->type = type;
        node->expression = expr;
        node->offset = ctx->offset + ctx->size;
        if (ctx->owner->entries != NULL) {
            InitializerEntry *tail = ctx->owner->entries;
            while (tail->next != NULL)
                tail = tail->next;
            tail->next = node;
        } else {
            ctx->owner->entries = node;
        }
    } else {
        CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
    }
}

Boolean evaluate_int_or_relocation(ENode *node, Object **pobj, CInt64 *pval)
{
    Object *obj;
    Object *lbase;
    Object *rbase;
    CInt64 lval;
    CInt64 rval;
    *pobj = NULL;
    pval->lo = 0;
    pval->hi = 0;
    for (;;) {
        switch (node->type) {
            case EINTCONST:
                *pval = node->data.intval;
                return 1;
            case EOBJREF:
                obj = node->data.objref;
                if (obj->datatype == DALIAS) {
                    CError_ASSERT(496, obj->u.alias.offset == 0);
                    obj = obj->u.alias.object;
                }
                lbase = obj;
                if (lbase->datatype == DLOCAL)
                    return 0;
                *pobj = lbase;
                return 1;
            case ESTRINGCONST:
                CInit_RewriteString(node, 0);
                break;
            case ETYPCON:
                do {
                    node = node->data.monadic;
                    if (node->rtype->size != stunsignedlong.size ||
                        (node->rtype->type != TYPEPOINTER && node->rtype->type != TYPEINT))
                        return 0;
                } while (node->type == ETYPCON);
                break;
            case EADD:
                if (!evaluate_int_or_relocation(node->data.diadic.left, &lbase, &lval))
                    return 0;
                if (!evaluate_int_or_relocation(node->data.diadic.right, &rbase, &rval))
                    return 0;
                if (lbase != NULL && rbase != NULL)
                    return 0;
                *pobj = lbase;
                *pval = CMach_CalcIntDiadic((Type *)&stunsignedlong, lval, '+', rval);
                return 1;
            case ESUB:
                if (!evaluate_int_or_relocation(node->data.diadic.left, &lbase, &lval))
                    return 0;
                if (!evaluate_int_or_relocation(node->data.diadic.right, &rbase, &rval))
                    return 0;
                if (rbase != NULL)
                    return 0;
                *pobj = lbase;
                *pval = CMach_CalcIntDiadic((Type *)&stunsignedlong, lval, '-', rval);
                return 1;
            default:
                return 0;
        }
    }
}

void initialize_pointer_or_intconst(InitializerData *ctx, ENode *node, Type *ns, UInt32 qual)
{
    OLinkList *p;
    Object *flag;
    CInt64 val;

    node = oldassignmentpromotion(node, ns, qual & Q_CV, 1);
    if (node->rtype->type == TYPEPOINTER || node->type == EINTCONST) {
        if (evaluate_int_or_relocation(node, &flag, &val)) {
            if (flag) {
                p = (OLinkList *)CompilerTools_AllocatePool(16);
                p->next = ctx->owner->relocations;
                p->obj = flag;
                p->addend = val.lo;
                p->offset = ctx->offset + ctx->size;
                ctx->owner->relocations = p;
            } else {
                CMach_InitIntMem((Type *)&stunsignedlong, val, ctx->buffer + ctx->size);
            }
        } else {
            append_initializer_entry(ctx, ns, node);
        }
    } else {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
    }
}

void initialize_int(InitializerData *stage, ENode *expr, Type *type, UInt32 qual)
{
    expr = oldassignmentpromotion(expr, type, qual & Q_CV, 1);
    if (expr->rtype->type == TYPEINT) {
        if (expr->type == EINTCONST) {
            CMach_InitIntMem(type, expr->data.intval, stage->buffer + stage->size);
        } else if (expr->type == ETYPCON && expr->data.monadic->rtype->type == TYPEPOINTER &&
                   expr->rtype->size == stunsignedlong.size && (copts.cplusplus || !copts.ANSIstrict)) {
            initialize_pointer_or_intconst(stage, expr->data.monadic, expr->data.monadic->rtype, qual);
        } else {
            append_initializer_entry(stage, type, expr);
        }
    } else {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
    }
}

void CInit_004d3620(TypeBitfield *bf, unsigned char *ptr, CInt64 value)
{
    SInt32 i;
    SInt32 n;
    SInt32 inc;

    if (copts.littleendian != 0) {
        i = bf->offset;
        inc = 1;
    } else {
        i = bf->bitlength + bf->offset - 1;
        inc = -1;
    }
    for (n = 0; n < bf->bitlength; n++, i += inc) {
        if (value.lo & 1) {
            if (copts.littleendian != 0) {
                ptr[i >> 3] |= 1 << (i & 7);
            } else {
                ptr[i >> 3] |= 0x80 >> (i & 7);
            }
        }
        value = CInt64_ShrU(value, cint64_one);
    }
}

void initialize_array_data(InitializerData *pool, CInit *iter, TypePointer *arrayType, UInt32 flags,
                           Boolean requireBraces)
{
    Boolean isWideChar;
    Boolean isChar;
    SInt32 elementSize;
    Boolean incompleteArray;
    Boolean hasBraces;
    InitializerData *block;
    UInt8 state;
    SInt32 index;
    SInt32 base;
    SInt32 arraySize;
    SInt32 stringSize;

    incompleteArray = (arrayType->size == 0);
    if ((elementSize = arrayType->target->size) == 0) {
        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        return;
    }
    isChar = arrayType->target->type == TYPEINT && arrayType->target->size == 1;
    isWideChar = arrayType->target->type == TYPEINT && arrayType->target->size == stwchar.size;

    switch (iter->state) {
        case 1:
            hasBraces = 1;
            if (advance_initializer_state(iter) == 3) {
                if (incompleteArray)
                    CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            break;
        case 2:
            hasBraces = 0;
            break;
    }
    switch (iter->state) {
        case 1:
        case 2:
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
    }

    if (iter->state == 2 && iter->expr->type == ESTRINGCONST && (isChar || isWideChar)) {
        if (iter->expr->rtype->type == TYPEPOINTER && arrayType->target->size != TPTR_TARGET(iter->expr->rtype)->size)
            CError_Warning(ERR_ILLEGAL_INITIALIZATION);
        if (incompleteArray) {
            arrayType->size = iter->expr->data.string.size;
            if (pool->capacity < (stringSize = iter->expr->data.string.size)) {
                block = (InitializerData *)CompilerTools_AllocatePool(0x26);
                memclrw(block, 0x26);
                block->owner = pool->owner;
                block->buffer = (unsigned char *)CompilerTools_AllocatePool(stringSize);
                block->offset = pool->offset + pool->size;
                block->capacity = stringSize;
                pool->next = block;
                memset(block->buffer, 0, block->capacity);
                pool = block;
            }
            memcpy(pool->buffer, iter->expr->data.string.data, iter->expr->data.string.size);
            pool->size = iter->expr->data.string.size;
        } else {
            if (iter->expr->data.string.size > arrayType->size) {
                if (copts.cplusplus != 0 || iter->expr->data.string.size - 1 > arrayType->size)
                    CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                iter->expr->data.string.size = arrayType->size;
            }
            memcpy(pool->buffer + pool->size, iter->expr->data.string.data, iter->expr->data.string.size);
        }
    } else {
        if (!hasBraces && requireBraces) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
        }
        base = pool->size;
        index = 0;
        for (;;) {
            if (incompleteArray) {
                arraySize = (index + 1) * elementSize;
                pool->size = base + arraySize - elementSize - pool->offset;
                if (arraySize > pool->offset + pool->capacity) {
                    block = (InitializerData *)CompilerTools_AllocatePool(0x26);
                    memclrw(block, 0x26);
                    block->owner = pool->owner;
                    block->buffer = (unsigned char *)CompilerTools_AllocatePool(elementSize * 16);
                    block->offset = pool->offset + pool->size;
                    block->capacity = elementSize * 16;
                    pool->next = block;
                    memset(block->buffer, 0, block->capacity);
                    pool = block;
                }
                initialize_typed_data(pool, iter, arrayType->target, flags, 0);
                arrayType->size = arraySize;
                pool->size = base + arraySize - pool->offset;
            } else {
                if (arrayType->size <= index * elementSize) {
                    index--;
                    CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                }
                pool->size = index * elementSize + base;
                initialize_typed_data(pool, iter, arrayType->target, flags, 0);
                if (!hasBraces && arrayType->size <= (index + 1) * elementSize)
                    break;
            }
            state = advance_initializer_state(iter);
            switch (state) {
                case 3:
                    if (hasBraces)
                        tk = CPrepTokenizer_GetNextToken();
                    return;
                case 1:
                case 2:
                    break;
                default:
                    CError_ReportError(ERR_RBRACE_EXPECTED);
                    return;
            }
            index++;
        }
    }
    if (hasBraces) {
        state = advance_initializer_state(iter);
        switch (state) {
            case 3:
                tk = CPrepTokenizer_GetNextToken();
                return;
            case 2:
                CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                return;
            default:
                CError_ReportError(ERR_RBRACE_EXPECTED);
                return;
        }
    }
}

void initialize_struct_data(InitializerData *ctx, CInit *ci, Type *type, UInt32 qual, Boolean allowIncomplete)
{
    int base;
    StructMember *member;
    TypePointer arrayType;
    Boolean braced;
    int i;
    int count;
    SInt32 kind;

    count = 0;
    if ((member = TYPE_STRUCT(type)->members) == NULL) {
        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        return;
    }
    switch (ci->state) {
        case 1:
            braced = 1;
            if (advance_initializer_state(ci) == 3) {
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            break;
        case 2:
            braced = 0;
            break;
    }
    switch (ci->state) {
        case 1:
        case 2:
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
    }
    if (!braced && ci->state == 2 && (allowIncomplete || ci->expr->rtype == type)) {
        ci->expr = oldassignmentpromotion(ci->expr, type, qual, 1);
        if (ci->expr->rtype->type == TYPESTRUCT)
            append_initializer_entry(ctx, type, ci->expr);
        return;
    }
    base = ctx->size;
    do {
        ctx->size = base + member->offset;
        if (member->type->size == 0) {
            if (!(allowIncomplete && member->type->type == TYPEARRAY)) {
                CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                if (member->type->type != TYPEARRAY)
                    return;
            }
            arrayType = *TYPE_POINTER(member->type);
            initialize_array_data(ctx, ci, &arrayType, member->qual, 1);
            ctx->size_adjustment = arrayType.size;
        } else {
            initialize_typed_data(ctx, ci, member->type, member->qual, 0);
        }
        count++;
        member = member->next;
        while (member != NULL && (member->qual & Q_WEAK) != 0)
            member = member->next;
        if (member == NULL || TYPE_STRUCT(type)->stype == 1) {
            if (braced) {
                switch (advance_initializer_state(ci)) {
                    case 3:
                        tk = CPrepTokenizer_GetNextToken();
                        return;
                    case 2:
                        CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                        return;
                    default:
                        CError_ReportError(ERR_RBRACE_EXPECTED);
                        return;
                }
            }
            return;
        }
        switch (advance_initializer_state(ci)) {
            case 3:
                if (braced)
                    tk = CPrepTokenizer_GetNextToken();
                kind = TYPE_STRUCT(type)->stype;
                if (kind >= 4 && kind <= 0xe) {
                    switch (kind) {
                        case 4:
                        case 5:
                        case 6: {
                            UInt8 *bytes;
                            UInt8 value;
                            if (count == 16)
                                break;
                            if (count == 1) {
                                bytes = ctx->buffer;
                                value = bytes[0];
                                for (i = 1; i < 16; i++)
                                    bytes[i] = value;
                                break;
                            }
                            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                            break;
                        }
                        case 7:
                        case 8:
                        case 9:
                        case 0xe: {
                            UInt16 *words;
                            UInt16 value;
                            if (count == 8)
                                break;
                            if (count == 1) {
                                words = (UInt16 *)ctx->buffer;
                                value = words[0];
                                for (i = 1; i < 8; i++)
                                    words[i] = value;
                                break;
                            }
                            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                            break;
                        }
                        case 0xa:
                        case 0xb:
                        case 0xc:
                        case 0xd: {
                            UInt32 *words;
                            UInt32 value;
                            if (count == 4)
                                break;
                            if (count == 1) {
                                words = (UInt32 *)ctx->buffer;
                                value = words[0];
                                for (i = 1; i < 4; i++)
                                    words[i] = value;
                                break;
                            }
                            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                            break;
                        }
                    }
                }
                return;
            case 1:
            case 2:
                continue;
            default:
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                return;
        }
    } while (1);
}

#include <string.h>

#include <string.h>

void initialize_class_initializer_data(InitializerData *dst, CInit *op, Type *type, UInt32 qual, int flag)
{
    ObjMemberVar *member;
    ENode *initialExpr;
    ObjMemberVar *next;
    enum InitKind kind;
    ENode *expr;
    unsigned char token;
    unsigned char nextToken;
    unsigned long offset;
    int baseOffset;
    char braced;
    TypePointer array;

    if (TYPE_CLASS(type)->bases || TYPE_CLASS(type)->vtable) {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
        return;
    }
    switch (op->state) {
        case IK_ONE:
            braced = 1;
            if ((char)(token = advance_initializer_state(op)) == 3) {
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            break;
        case IK_TWO:
            braced = 0;
    }
    switch (kind = (enum InitKind)op->state) {
        default:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
        case IK_ONE:
        case IK_TWO:
            if (!braced && kind == IK_TWO) {
                if ((char)flag || (initialExpr = op->expr)->rtype == type) {
                    expr = op->expr = oldassignmentpromotion(op->expr, type, qual, 1);
                    if (expr->rtype->type == TYPECLASS)
                        append_initializer_entry(dst, type, expr);
                    return;
                }
            }
            {
                ObjMemberVar *scan;
                for (scan = TYPE_CLASS(type)->ivars; scan; scan = scan->next) {
                    if (scan->access != ACCESSPUBLIC) {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                        break;
                    }
                }
            }
    }
    if ((member = TYPE_CLASS(type)->ivars) == NULL) {
        CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
        return;
    }
    baseOffset = dst->size;
    for (;;) {
        dst->size = baseOffset + member->offset;
        if (member->type->size == 0) {
            if (!(char)flag || member->type->type != TYPEARRAY) {
                CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                if (member->type->type != TYPEARRAY)
                    return;
            }
            array = *(TypePointer *)member->type;
            initialize_array_data(dst, op, &array, member->qual, 1);
            dst->size_adjustment = array.size;
        } else {
            initialize_typed_data(dst, op, member->type, member->qual, 0);
        }
        next = member;
        offset = member->offset;
        for (;;) {
            if ((next = next->next) == NULL) {
                next = NULL;
                break;
            }
            if (!next->anonunion || next->offset > member->offset)
                break;
            if (next->type->type != TYPEBITFIELD)
                continue;
            if (member->type->type == TYPEBITFIELD &&
                ((TypeBitfield *)next->type)->offset != ((TypeBitfield *)member->type)->offset)
                break;
        }
        if ((member = next) == NULL || TYPE_CLASS(type)->mode == 1 && next->offset == offset) {
            if (braced) {
                token = advance_initializer_state(op);
                switch (token) {
                    case 3:
                        tk = CPrepTokenizer_GetNextToken();
                        return;
                    case 2:
                        CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                        return;
                    default:
                        CError_ReportError(ERR_RBRACE_EXPECTED);
                        return;
                }
            }
            return;
        }
        nextToken = advance_initializer_state(op);
        switch (nextToken) {
            case 3:
                if (braced)
                    tk = CPrepTokenizer_GetNextToken();
                return;
            case 1:
            case 2:
                continue;
            default:
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                return;
        }
    }
}

int initialize_typed_data(InitializerData *data, CInit *init, Type *type, UInt32 qualifiers, int nested)
{
    Boolean braced;

    switch ((SInt8)type->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;

        case TYPEINT:
        case TYPEFLOAT:
        case TYPEENUM:
        case TYPEBITFIELD:
        case TYPEMEMBERPOINTER:
        case TYPEPOINTER:
            switch (init->state) {
                case 1:
                    braced = 1;
                    advance_initializer_state(init);
                    break;
                case 2:
                    braced = 0;
                    break;
            }
            if (init->state != 2) {
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                return;
            }
            switch ((SInt8)type->type) {
                case TYPEINT:
                    initialize_int(data, init->expr, type, qualifiers);
                    break;
                case TYPEFLOAT: {
                    ENode *expr;
                    expr = init->expr;
                    expr = oldassignmentpromotion(expr, type, qualifiers & Q_CV, 1);
                    if (expr->rtype->type == TYPEFLOAT) {
                        if (expr->type == EFLOATCONST)
                            CMach_InitFloatMem(type, expr->data.floatval, data->buffer + data->size);
                        else
                            append_initializer_entry(data, type, expr);
                    } else {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                    break;
                }
                case TYPEENUM: {
                    ENode *expr;
                    expr = init->expr;
                    expr = oldassignmentpromotion(expr, type, qualifiers & Q_CV, 1);
                    if (expr->rtype->type == TYPEENUM) {
                        if (expr->type == EINTCONST)
                            CMach_InitIntMem(TYPE_ENUM(type)->enumtype, expr->data.intval, data->buffer + data->size);
                        else
                            append_initializer_entry(data, type, expr);
                    } else {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                    break;
                }
                case TYPEPOINTER:
                    initialize_pointer_or_intconst(data, init->expr, type, qualifiers);
                    break;
                case TYPEMEMBERPOINTER: {
                    ENode *expr;
                    expr = init->expr;
                    expr = oldassignmentpromotion(expr, type, qualifiers & Q_CV, 1);
                    if (expr->type == EINTCONST)
                        CMach_InitIntMem((Type *)&stsignedlong, expr->data.intval, data->buffer + data->size);
                    else
                        append_initializer_entry(data, type, expr);
                    break;
                }
                case TYPEBITFIELD: {
                    Type *baseType = TYPE_BITFIELD(type)->bitfieldtype;
                    ENode *expr;
                    ENode *initializerExpr = init->expr;
                    if (baseType->type == TYPEENUM)
                        baseType = TYPE_ENUM(baseType)->enumtype;
                    expr = oldassignmentpromotion(initializerExpr, baseType, qualifiers & Q_CV, 1);
                    if (expr->rtype->type == TYPEINT) {
                        if (expr->type == EINTCONST)
                            CInit_004d3620(TYPE_BITFIELD(type), data->buffer + data->size, expr->data.intval);
                        else
                            append_initializer_entry(data, type, expr);
                    } else {
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                    break;
                }
                default:
                    CError_FATAL(1474);
                    break;
            }
            if (braced != 0) {
                switch (advance_initializer_state(init)) {
                    case 3:
                        tk = CPrepTokenizer_GetNextToken();
                        break;
                    case 2:
                        CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                        break;
                    default:
                        CError_ReportError(ERR_RBRACE_EXPECTED);
                        break;
                }
            }
            break;

        case TYPESTRUCT:
            initialize_struct_data(data, init, type, qualifiers, nested);
            break;
        case TYPEARRAY:
            initialize_array_data(data, init, TYPE_POINTER(type), qualifiers, nested);
            break;
        case TYPECLASS:
            initialize_class_initializer_data(data, init, type, qualifiers, nested);
            break;

        default:
            CError_FATAL(1505);
            break;
    }
}

/* Chained initialization data, with allocation state in the head block. */

void CInit_004d2700(InitializerData *data, Type *type, UInt32 qual, Boolean flag)
{
    SInt32 size;
    TypePointer *array;
    InitializerData *block;
    unsigned char *buffer;
    CInit state;

    fn_00441f10();
    memclrw(data, 38);
    data->owner = data;
    if (type->size == 0) {
        if (type->type == TYPEARRAY_004d2700) {
            data->capacity = TPTR_TARGET(type)->size << 4;
        } else {
            CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        }
    } else {
        data->capacity = type->size;
    }
    data->buffer = CompilerTools_AllocatePool(data->capacity);
    memset(data->buffer, 0, data->capacity);
    data->unknown9 = flag;
    state.state = 0;
    state.usesConstructorSyntax = 0;
    if (type->type == TYPESTRUCT_004d2700) {
        SInt32 structKind = ((TypeStruct *)type)->stype;
        if (structKind >= 4 && structKind <= 14)
            data->unknown9 = 1;
    }
    if (type->type == TYPEARRAY_004d2700) {
        array = (TypePointer *)type;
        if (array->target->type == TYPESTRUCT_004d2700) {
            SInt32 structKind = ((TypeStruct *)array->target)->stype;
            if (structKind >= 4 && structKind <= 14)
                data->unknown9 = 1;
        }
    }
    advance_initializer_state(&state);
    initialize_typed_data(data, &state, type, qual, 1);
    size = type->size + data->size_adjustment;
    if (size != 0) {
        if (data->next != NULL) {
            buffer = CompilerTools_AllocatePool(size);
            block = data;
            while (block != NULL) {
                CError_ASSERT(1577, block->offset + block->size <= size);
                memcpy(buffer + block->offset, block->buffer, block->size);
                block = block->next;
            }
            data->buffer = buffer;
        }
    } else {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
    }
    data->size = size;
    data->next = NULL;
    CompilerTools_DecrementPositiveCounter();
}

ENode *build_init_assignment(ENode *previous, ENode *base, SInt32 offset, Type *type, ENode *value)
{
    ENode *node;
    ENode *bitfield;

    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *node = *base;
    if (offset != 0) {
        node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
    }
    if (type->type == TYPEBITFIELD) {
        bitfield = makemonadicnode(node, EBITFIELD);
        bitfield->rtype = type;
        bitfield = makemonadicnode(bitfield, EINDIRECT);
        bitfield->rtype = TYPE_BITFIELD(type)->bitfieldtype;
        node = bitfield;
    } else {
        node = makemonadicnode(node, EINDIRECT);
        node->rtype = type;
    }
    node = makediadicnode(node, value, EASS);
    if (previous == NULL) {
        return node;
    }
    previous = makediadicnode(previous, node, ECOMMA);
    previous->rtype = node->rtype;
    return previous;
}

ENode *create_destructor_registration_call(Type *objectType, Object *destructor, ENode *objectAddress)
{
    ENode *call;
    Object *registrationRecord;
    Type *registrationType;
    DeclInfo declaration;

    if (copts.no_static_dtors)
        return objectAddress;

    call = CompilerTools_AllocatePool(sizeof(ENode));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->flags = 0;
    call->rtype = CDecl_NewPointerType(objectType);
    call->data.funccall.funcref = create_objectrefnode(destructor_registration_func);
    call->data.funccall.functype = (TypeFunc *)destructor_registration_func->type;
    call->data.funccall.args = CompilerTools_AllocatePool(sizeof(ENodeList));
    call->data.funccall.args->node = objectAddress;
    call->data.funccall.args->next = CompilerTools_AllocatePool(sizeof(ENodeList));
    call->data.funccall.args->next->node = create_objectrefnode(CABI_GetDestructorObject(destructor, 1));
    call->data.funccall.args->next->next = CompilerTools_AllocatePool(sizeof(ENodeList));

    registrationType = CDecl_NewStructType(void_ptr.size * 3, CMachine_GetTypeAlignment((Type *)&void_ptr));
    memclrw(&declaration, sizeof(declaration));
    declaration.thetype = registrationType;
    declaration.name = CParser_GetUniqueName();
    declaration.qual = 0;
    declaration.storageclass = 0x102;
    declaration.requireMangledName = 1;
    registrationRecord = CParser_NewObject(&declaration);
    registrationRecord->nspace = cscope_root;
    CodeGen_SetObjectSectionAndInterruptInfo(registrationRecord);
    emit_object(registrationRecord, NULL, NULL, registrationRecord->type->size, 0);
    call->data.funccall.args->next->next->node = create_objectrefnode(registrationRecord);
    call->data.funccall.args->next->next->next = NULL;
    return call;
}

Boolean initialize_class_object(Object *obj, Type *initObject, ENode *expr, SInt32 offset, Boolean parse)
{
    NameSpaceObjectList *initializer;
    Object *destructor;
    ENodeList *args;
    ENode *node;
    Boolean flag;

    initializer = CClass_Constructor((TypeClass *)initObject);
    destructor = CClass_Destructor((TypeClass *)initObject);
    if (initializer == NULL && destructor == NULL)
        return 0;
    if (parse && initializer == NULL && tk == '=') {
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x7b)
            return 0;
    }
    if (parse && tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        args = CExpr_ScanExpressionList(1);
        if (tk == ')')
            tk = CPrepTokenizer_GetNextToken();
        else
            CError_ReportError(ERR_RPAREN_EXPECTED);
    } else {
        if (expr != NULL) {
            args = CompilerTools_AllocatePool(sizeof(ENodeList));
            args->node = expr;
            args->next = NULL;
        } else
            args = NULL;
    }
    node = create_objectrefnode(obj);
    if (offset != 0) {
        node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
    }
    if (initializer != NULL) {
        flag = 1;
        if (tk == '=') {
            flag = 0;
            if (args != NULL)
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            args = CompilerTools_AllocatePool(sizeof(ENodeList));
            args->next = NULL;
            tk = CPrepTokenizer_GetNextToken();
            args->node = conv_assignment_expression();
        }
        node = CExpr_ConstructObject(initObject, node, args, 0, 1, 0, 1, flag);
        if (node->rtype->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return 1;
        }
    } else {
        if (args != NULL)
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
    }
    if (destructor != NULL) {
        node = create_destructor_registration_call(initObject, destructor, node);
    }
    if (cinit_state->useEmitCallback != 0) {
        cinit_state->init_expr_register_cb(node);
    } else {
        InitExpr_Register(node, obj);
    }
    return 1;
}

Boolean CInit_004d20e0(Type *type, ENode *initializer, SInt32 offset, char parseArguments)
{
    NameSpaceObjectList *objects;
    Object *classInfo;
    ENodeList *arguments;
    ENode *node;
    Boolean flag;
    ENode *result;
    ENode *expression;

    objects = CClass_Constructor(TYPE_CLASS(type));
    classInfo = CClass_Destructor(TYPE_CLASS(type));
    if (objects == NULL && classInfo == NULL)
        return 0;

    if (classInfo != NULL)
        CClass_CheckStaticAccess(NULL, TYPE_CLASS(type), classInfo->access);

    if (parseArguments != 0 && objects == NULL && tk == '=') {
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x7b)
            return 0;
    }

    if (parseArguments != 0 && tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        arguments = CExpr_ScanExpressionList(1);
        if (tk == ')')
            tk = CPrepTokenizer_GetNextToken();
        else
            CError_ReportError(ERR_RPAREN_EXPECTED);
    } else {
        if (initializer != NULL) {
            arguments = CompilerTools_AllocatePool(sizeof(CInit_ArgumentList));
            arguments->node = initializer;
            arguments->next = NULL;
        } else {
            arguments = NULL;
        }
    }

    if (objects != NULL) {
        flag = 1;
        if (tk == '=') {
            if (arguments != NULL)
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            arguments = CompilerTools_AllocatePool(sizeof(CInit_ArgumentList));
            arguments->next = NULL;
            tk = CPrepTokenizer_GetNextToken();
            expression = conv_assignment_expression();
            flag = 0;
            arguments->node = expression;
        }
        if (classInfo == NULL) {
            node = create_objectrefnode(cinit_state->emitObject);
            if (offset != 0)
                node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
        } else {
            node = cinit_state->register_object_cb(type, cinit_state->emitObject, offset, 0);
        }
        result = CExpr_ConstructObject(type, node, arguments, 0, 1, 0, 1, flag);
        if (result->rtype->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return 1;
        }
        node = makemonadicnode(result, EINDIRECT);
        node->rtype = TPTR_TARGET(node->rtype);
        cinit_state->insert_expr_cb(node);
    } else {
        if (arguments != NULL)
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
        if (classInfo != NULL)
            node = cinit_state->register_object_cb(type, cinit_state->emitObject, offset, 0);
        cinit_state->insert_expr_cb(node);
    }
    return 1;
}

void init_int_or_relocation(Type *type, ENode *expression)
{
    OLinkList *relocation;
    Object *object;
    CInt64 value;

    if (evaluate_int_or_relocation(expression, &object, &value)) {
        if (object != NULL) {
            relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
            relocation->next = cinit_state->list;
            relocation->obj = object;
            relocation->addend = value.lo;
            relocation->offset = cinit_state->expr_offset;
            cinit_state->list = relocation;
        } else {
            CMach_InitIntMem((Type *)&stunsignedlong, value, cinit_state->buffer + cinit_state->expr_offset);
        }
    } else {
        if (cinit_state->expr_cb != NULL) {
            cinit_state->expr_cb(type, expression, 0);
            cinit_state->expr_cb_called = 1;
        } else {
            CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
        }
    }
    cinit_state->expr_offset += 4;
}

void init_int(Type *type, ENode *node)
{
    if (node->type == EINTCONST) {
        CMach_InitIntMem(type, node->data.intval, cinit_state->buffer + cinit_state->expr_offset);
    } else if (node->type == ETYPCON && node->data.monadic->rtype->type == TYPEPOINTER &&
               node->rtype->size == stunsignedlong.size && (copts.cplusplus != 0 || copts.ANSIstrict == 0)) {
        init_int_or_relocation(node->data.monadic->rtype, node->data.monadic);
    } else {
        if (cinit_state->expr_cb != NULL) {
            cinit_state->expr_cb(type, node, 0);
            cinit_state->expr_cb_called = 1;
        } else {
            CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
        }
    }
    cinit_state->expr_offset += type->size;
}

void CInit_004d1d90(Type *type, ENode *node)
{
    switch ((SInt8)type->type) {
        case TYPEINT:
            init_int(type, node);
            return;

        case TYPEFLOAT:
            if (node->type == EFLOATCONST)
                CMach_InitFloatMem(type, node->data.floatval, cinit_state->buffer + cinit_state->expr_offset);
            else
                CInit_DefaultInit(type, node);
            cinit_state->expr_offset += type->size;
            return;

        case TYPEENUM:
            if (node->type == EINTCONST)
                CMach_InitIntMem(TYPE_ENUM(type)->enumtype, node->data.intval,
                                 cinit_state->buffer + cinit_state->expr_offset);
            else
                CInit_DefaultInit(type, node);
            cinit_state->expr_offset += type->size;
            return;

        case TYPEPOINTER:
            init_int_or_relocation(type, node);
            return;

        case TYPEMEMBERPOINTER:
            if (node->type == EINTCONST)
                CMach_InitIntMem((Type *)&stsignedlong, node->data.intval,
                                 cinit_state->buffer + cinit_state->expr_offset);
            else
                CInit_DefaultInit(type, node);
            cinit_state->expr_offset += type->size;
            return;

        case TYPESTRUCT:
        case TYPECLASS:
            CInit_DefaultInit(type, node);
            return;

        case TYPEARRAY:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;

        default:
            CError_FATAL(2012);
            return;
    }
}

/* An array initializer: a string literal for a char or wide-char array, or a braced list of element initializers;
   an array of unknown size takes its size from the initializer. */
void initialize_array_data_by_type(TypePointer *tptr, UInt32 mode, Boolean flag)
{
    SInt32 start;
    SInt8 chars;
    SInt8 wide;
    SInt32 elemSize;
    Type *element;
    Boolean braced;
    Boolean ctor;
    SInt32 i;
    void (*callback)(Type *, ENode *, Boolean);

    elemSize = start = tptr->target->size;
    element = tptr->target;
    if (!elemSize) {
        if (element->type != TYPEARRAY) {
            CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
            return;
        }
        if (!(elemSize = element->size)) {
            CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
            return;
        }
    }
    chars = element->type == TYPEINT && element->size == 1;
    wide = element->type == TYPEINT && tptr->target->size == stwchar.size;
    braced = 1;
    if (flag && !(tk == TK_STRING && (Boolean)chars) && !(tk == TK_STRING_WIDE && (Boolean)wide)) {
        if (tk != '{') {
            CError_ReportErrorAndUpdateToken(ERR_LBRACE_EXPECTED);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
    } else {
        if (tk == '{')
            tk = CPrepTokenizer_GetNextToken();
        else
            braced = 0;
    }
    if ((tk == TK_STRING && (Boolean)chars) || (tk == TK_STRING_WIDE && (Boolean)wide)) {
        if (tptr->size) {
            if (token_value_kind_or_string_length > tptr->size) {
                if (copts.cplusplus ||
                    token_value_kind_or_string_length - ((Boolean)wide ? stwchar.size : 1) > tptr->size)
                    CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                token_value_kind_or_string_length = tptr->size;
            }
            memcpy(cinit_state->buffer + cinit_state->expr_offset, string_token_data,
                   token_value_kind_or_string_length);
            cinit_state->expr_offset += tptr->size;
        } else {
            tptr->size = token_value_kind_or_string_length;
            write_buffer_at_offset(string_token_data, cinit_state->expr_offset, token_value_kind_or_string_length);
            cinit_state->expr_offset += token_value_kind_or_string_length;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (braced) {
            if (tk == ',' && copts.cplusplus)
                tk = CPrepTokenizer_GetNextToken();
            if (tk != '}')
                CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
        }
        return;
    }
    if (tptr->target->type == TYPECLASS && needs_init((TypeClass *)tptr->target))
        ctor = 1;
    else
        ctor = 0;
    start = cinit_state->expr_offset, i = 0;
    for (;;) {
        if (tk == '}') {
        finish:
            if (tptr->size) {
                if (ctor) {
                    for (; tptr->size > i * elemSize; i++) {
                        cinit_state->expr_offset = start + i * elemSize;
                        if ((callback = cinit_state->expr_cb) != NULL) {
                            callback(tptr->target, NULL, 1);
                            cinit_state->expr_cb_called = 1;
                        } else
                            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                    }
                }
            } else
                tptr->size = i * elemSize;
            start += tptr->size;
            cinit_state->expr_offset = start;
            if (braced)
                tk = CPrepTokenizer_GetNextToken();
            return;
        }
        if (tptr->size == 0) {
            cinit_state->expr_offset = start + i * elemSize;
            write_buffer_at_offset(NULL, cinit_state->expr_offset, elemSize);
            if (ctor) {
                if (cinit_state->expr_cb) {
                    cinit_state->expr_cb(tptr->target, conv_assignment_expression(), 1);
                    cinit_state->expr_cb_called = 1;
                } else
                    CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            } else
                initialize_data_by_type(tptr->target, mode, 0);
        } else {
            if (tptr->size <= i * elemSize) {
                i--;
                CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
            }
            cinit_state->expr_offset = start + i * elemSize;
            if (ctor) {
                if (cinit_state->expr_cb) {
                    cinit_state->expr_cb(tptr->target, conv_assignment_expression(), 1);
                    cinit_state->expr_cb_called = 1;
                } else
                    CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            } else
                initialize_data_by_type(tptr->target, mode, 0);
            if (!braced && tptr->size <= (i + 1) * elemSize)
                return;
        }
        if (tk != '}') {
            if (tk != ',') {
                CError_ReportErrorAndUpdateToken(ERR_DECLARATION_SYNTAX_ERROR);
                braced = 0;
                i++;
                goto finish;
            }
            tk = CPrepTokenizer_GetNextToken();
        }
        i++;
    }
}

void initialize_struct(Type *type, Boolean brace)
{
    StructMember *member;
    SInt32 baseOffset;
    Boolean hasBrace;
    SInt32 structKind;
    ENode *expression;

    if ((member = TYPE_STRUCT(type)->members) == NULL) {
        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        return;
    }
    structKind = TYPE_STRUCT(type)->stype;
    if (structKind == 1) {
        if (tk == '{') {
            tk = CPrepTokenizer_GetNextToken();
            initialize_data_by_type(member->type, member->qual, 0);
            if (tk == '}') {
                tk = CPrepTokenizer_GetNextToken();
            }
        } else {
            initialize_data_by_type(member->type, member->qual, 0);
        }
        return;
    }
    if (type->type == TYPESTRUCT && structKind >= 4 && structKind <= 0xe && tk != '{') {
        expression = oldassignmentpromotion(conv_assignment_expression(), type, 0, 1);
        InitExprWrap(type, expression, expression->type == EASSBLK);
        return;
    }
    if (tk != '{') {
        if (brace) {
            CError_ReportErrorAndUpdateToken(ERR_LBRACE_EXPECTED);
        }
        hasBrace = 0;
    } else {
        hasBrace = 1;
        tk = CPrepTokenizer_GetNextToken();
    }
    baseOffset = cinit_state->expr_offset;
    for (;;) {
        if (tk == '}')
            break;
        cinit_state->expr_offset = baseOffset + member->offset;
        if (member->type->size == 0 && member->type->type == TYPEARRAY) {
            CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
            break;
        }
        initialize_data_by_type(member->type, member->qual, 0);
        if (tk == '}')
            break;
        if (tk != ',') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            break;
        }
        do {
            member = member->next;
            if (member == NULL)
                break;
        } while (member->qual & Q_WEAK);
        if (member == NULL) {
            if (!hasBrace)
                break;
            tk = CPrepTokenizer_GetNextToken();
            if (tk == '}')
                continue;
            CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
            break;
        }
        tk = CPrepTokenizer_GetNextToken();
    }
    cinit_state->expr_offset = baseOffset + type->size;
    if (tk == '}' && hasBrace) {
        tk = CPrepTokenizer_GetNextToken();
    }
}

void initialize_class_data(Type *t, Boolean flag)
{
    ObjMemberVar *m;
    Boolean brace;
    SInt32 base;

    if (tk == '{') {
        brace = 1;
        tk = CPrepTokenizer_GetNextToken();
    } else {
        brace = 0;
    }
    if (TYPE_CLASS(t)->bases != NULL || TYPE_CLASS(t)->vtable != NULL) {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
        return;
    }
    for (m = TYPE_CLASS(t)->ivars; m != NULL; m = m->next)
        if (m->access != ACCESSPUBLIC)
            break;
    if (m == NULL && CClass_Constructor(TYPE_CLASS(t)) == NULL && (CClass_Destructor(TYPE_CLASS(t)) == NULL || brace)) {
        if ((m = TYPE_CLASS(t)->ivars) != NULL) {
            base = cinit_state->expr_offset;
            for (;;) {
                if (tk == '}')
                    break;
                if (m->type->size == 0 && m->type->type == TYPEARRAY) {
                    CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                    break;
                }
                cinit_state->expr_offset = base + m->offset;
                initialize_data_by_type(m->type, m->qual, 0);
                if (tk == '}')
                    break;
                if (tk != ',') {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
                }
                do {
                    m = m->next;
                } while (m != NULL && m->anonunion != 0);
                if (m == NULL) {
                    if (!brace)
                        break;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk != '}') {
                        CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
                        break;
                    }
                } else {
                    tk = CPrepTokenizer_GetNextToken();
                }
            }
        } else {
            if (brace && tk != '}')
                CError_ReportError(ERR_TOO_MANY_INITIALIZERS);
        }
    } else {
        if (brace)
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
        CInit_004d1d90(t, oldassignmentpromotion(conv_assignment_expression(), t, 0, 1));
    }
    cinit_state->expr_offset = base + t->size;
    if (tk == '}' && brace)
        tk = CPrepTokenizer_GetNextToken();
}

void initialize_data_by_type(Type *node, UInt32 mode, Boolean flag)
{
    ENode localA;
    ENode localB;
    ENode *ctx;
    ENode *n;
    Type *ty;
    Boolean braced;

    switch ((SInt8)node->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;

        case TYPEINT:
        case TYPEFLOAT:
        case TYPEENUM:
        case TYPEMEMBERPOINTER:
        case TYPEPOINTER:
            if (tk == '{') {
                tk = CPrepTokenizer_GetNextToken();
                ctx = parse_initializer_expression(&localA);
                n = oldassignmentpromotion(ctx, node, mode & 3, 1);
                if (tk == ',' && copts.cplusplus != 0)
                    tk = CPrepTokenizer_GetNextToken();
                if (tk != '}')
                    CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
                else
                    tk = CPrepTokenizer_GetNextToken();
            } else {
                ctx = parse_initializer_expression(&localA);
                n = oldassignmentpromotion(ctx, node, mode & 3, 1);
            }
            CInit_004d1d90(node, n);
            return;

        case TYPEBITFIELD:
            if ((braced = (tk == '{')) != 0)
                tk = CPrepTokenizer_GetNextToken();
            ctx = parse_initializer_expression(&localB);
            ty = TYPE_BITFIELD(node)->bitfieldtype;
            if (ty->type == TYPEENUM)
                ty = TYPE_ENUM(ty)->enumtype;
            n = oldassignmentpromotion(ctx, ty, 0, 1);
            if (n->type == EINTCONST)
                CInit_004d3620(TYPE_BITFIELD(node), cinit_state->buffer + cinit_state->expr_offset, n->data.intval);
            else
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
            if (braced) {
                if (tk == ',' && copts.cplusplus != 0)
                    tk = CPrepTokenizer_GetNextToken();
                if (tk != '}')
                    CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
                else
                    tk = CPrepTokenizer_GetNextToken();
            }
            return;

        case TYPEARRAY:
            initialize_array_data_by_type((TypePointer *)node, mode, flag);
            return;

        case TYPESTRUCT:
            initialize_struct(node, flag);
            return;

        case TYPECLASS:
            initialize_class_data(node, flag);
            return;

        default:
            CError_FATAL(2411);
            return;
    }
}

void initialize_object_at_offset(Type *type, ENode *initializer, Boolean is_class)
{
    ENode *node;
    SInt32 offset;

    cinit_state->hasRuntimeInitialization = 1;
    if (is_class) {
        initialize_class_object(cinit_state->obj, type, initializer, cinit_state->expr_offset, 0);
    } else {
        node = create_objectrefnode(cinit_state->obj);
        if (TYPE(node->rtype)->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
        }
        TYPE_POINTER(node->rtype)->target = type;
        offset = cinit_state->expr_offset;
        if (offset != 0)
            node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
        node = makemonadicnode(node, EINDIRECT);
        node->rtype = type;
        node = makediadicnode(node, initializer, EASS);
        if (cinit_state->useEmitCallback != 0)
            cinit_state->init_expr_register_cb(node);
        else
            InitExpr_Register(node, cinit_state->obj);
    }
}

void CInit_004d1170(Type *type, ENode *expr, Boolean flag)
{
    ENode *node;
    TypePointer *arrayType;
    SInt32 offset;

    if (flag) {
        CInit_004d20e0(type, expr, cinit_state->expr_offset, 0);
    } else {
        if (type->type == TYPEARRAY && (type->size & 1)) {
            arrayType = (TypePointer *)galloc(sizeof(TypePointer));
            *arrayType = *(TypePointer *)type;
            type = (Type *)arrayType;
            arrayType->size++;
        }
        node = create_objectrefnode(cinit_state->emitObject);
        if (node->rtype->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return;
        }
        TYPE_POINTER(node->rtype)->target = type;
        offset = cinit_state->expr_offset;
        if (offset != 0) {
            node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
        }
        node = makemonadicnode(node, EINDIRECT);
        node->rtype = type;
        node = makediadicnode(node, expr, EASS);
        if (copts.cplusplus == 0) {
            CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
        }
        cinit_state->insert_expr_cb(node);
    }
}

ENode *CInit_004d0ae0(Object *obj, Type *type, UInt32 qual, void (*contextOffset)(Type *, ENode *, Boolean),
                      Boolean flag)
{
    ENode *objectNode;
    Object *targetObject;
    ENode *result;
    ENode *initializer;
    Type *elementType;
    UInt16 qualifiers;
    Boolean inlineData;
    ENode *floatResult;
    SInt32 size;

    do {
        objectNode = NULL;
        cinit_state->expr_cb = contextOffset;
        if (tk == '(') {
            if (type->type == TYPEARRAY)
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            tk = CPrepTokenizer_GetNextToken();
            result = oldassignmentpromotion(conv_assignment_expression(), type, qual, 1);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
        } else {
            tk = CPrepTokenizer_GetNextToken();
            switch ((SInt8)type->type) {
                case TYPECLASS:
                    if (tk == '{' && CClass_Constructor((TypeClass *)type))
                        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
                case TYPESTRUCT:
                    if (tk != '{')
                        break;
                case TYPEARRAY:
                    if (obj == NULL) {
                        if (type->type == TYPEARRAY) {
                            for (elementType = type; elementType->type == TYPEARRAY;
                                 elementType = ((TypePointer *)elementType)->target)
                                ;
                            if (elementType->type == TYPECLASS && IsComplexClass(elementType)) {
                                InitBuffer(type->size);
                                cinit_state->obj = cinit_state->emitObject;
                                initialize_data_by_type(type, cinit_state->obj->qual, 1);
                                return NULL;
                            }
                            if (type->size & 1) {
                                TypePointer *arrayType = (TypePointer *)galloc(sizeof(TypePointer));
                                *arrayType = *(TypePointer *)type;
                                type = (Type *)arrayType;
                                arrayType->size++;
                            }
                        }
                        cinit_state->obj = obj = CreateObject(type, qual);
                        objectNode = create_objectnode(obj);
                        cinit_state->emitObject = obj;
                    }
                    InitBuffer(type->size);
                    initialize_data_by_type(type, obj->qual, 1);
                    if (obj->type->size != (size = cinit_state->bufferSize))
                        CError_FATAL(2567);
                    if (cinit_state->list != NULL || !IsZeroed(cinit_state->buffer, size))
                        emit_object(obj, cinit_state->buffer, cinit_state->list, size = obj->type->size, 0);
                    else
                        emit_object(obj, NULL, NULL, size = obj->type->size, 0);
                    return objectNode;
                case TYPEINT:
                case TYPEFLOAT:
                case TYPEENUM:
                case TYPEMEMBERPOINTER:
                case TYPEPOINTER:
                    break;
                default:
                    goto fatal;
            }
            if (obj)
                qualifiers = obj->qual;
            else
                qualifiers = cinit_state->emitObject->qual;
            qualifiers &= Q_CONST | Q_VOLATILE;
            if (tk == '{') {
                tk = CPrepTokenizer_GetNextToken();
                initializer = assignment_expression();
                if (tk == ',' && copts.cplusplus != 0)
                    tk = CPrepTokenizer_GetNextToken();
                if (tk != '}')
                    CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
                else
                    tk = CPrepTokenizer_GetNextToken();
            } else {
                initializer = assignment_expression();
            }
            result = oldassignmentpromotion(initializer, type, qualifiers, 1);
        }

        if (obj == NULL)
            targetObject = cinit_state->emitObject;
        else
            targetObject = obj;
        do {
            if (is_const_object(targetObject)) {
                switch ((SInt8)targetObject->type->type) {
                    case TYPEINT:
                    case TYPEENUM:
                        if (result->type != EINTCONST)
                            continue;
                        targetObject->u.data.u.intconst = result->data.intval;
                        break;
                    case TYPEFLOAT:
                        if (result->type != EFLOATCONST)
                            continue;
                        targetObject->u.data.u.string = (char *)galloc(sizeof(CInt64));
                        floatResult = result;
                        *(Float *)targetObject->u.data.u.string =
                            CMachine_RoundFloatToType(targetObject->type, floatResult->data.floatval);
                        break;
                    case TYPEPOINTER:
                        initializer = result;
                        while (initializer->type == ETYPCON)
                            initializer = initializer->data.monadic;
                        if (initializer->type != EINTCONST)
                            continue;
                        targetObject->u.data.u.intconst = initializer->data.intval;
                        break;
                    default:
                        continue;
                }
                targetObject->qual |= Q_INLINE_DATA;
                if (obj == NULL) {
                    targetObject->sclass = TK_STATIC;
                    targetObject->datatype = DDATA;
                    targetObject->u.data.linkname = CParser_AppendUniqueName(targetObject->name->name);
                } else if (targetObject->sclass != TK_STATIC || (targetObject->flags & OBJECT_FLAGS_2)) {
                    CInit_ExportConst(targetObject);
                }
                return NULL;
            }
        } while (0);
        if (obj == NULL || (flag && copts.cplusplus != 0)) {
            if (obj) {
                CanAllocObject(obj->type);
                if (obj->type->size != type->size)
                    CError_FATAL(2675);
                emit_object(obj, NULL, NULL, obj->type->size, 0);
            }
            return result;
        }
        InitBuffer(type->size);
        CInit_004d1d90(type, result);
        if (obj->type->size != cinit_state->bufferSize)
            CError_FATAL(2684);
        CanAllocObject(obj->type);
        inlineData = !cinit_state->hasRuntimeInitialization && is_const_object(targetObject);
        if (cinit_state->list != NULL || !IsZeroed(cinit_state->buffer, obj->type->size)) {
            if (inlineData)
                emit_object(obj, cinit_state->buffer, cinit_state->list, obj->type->size, 1);
            else
                emit_object(obj, cinit_state->buffer, cinit_state->list, obj->type->size, 0);
        } else {
            if (inlineData)
                emit_object(obj, NULL, NULL, obj->type->size, 1);
            else
                emit_object(obj, NULL, NULL, obj->type->size, 0);
        }
        break;
    fatal:
        CError_FATAL(2704);
    } while (0);
    return NULL;
}

void CInit_ExportConst(Object *obj)
{
    unsigned char buf[64];
    if (obj->flags & OBJECT_DEFINED)
        return;
    switch (*(SInt8 *)obj->type) {
        case TYPEINT:
            CMach_InitIntMem(obj->type, obj->u.data.u.intconst, buf);
            break;
        case TYPEENUM:
            CMach_InitIntMem(TYPE_ENUM(obj->type)->enumtype, obj->u.data.u.intconst, buf);
            break;
        case TYPEPOINTER:
            CMach_InitIntMem((Type *)&stunsignedlong, obj->u.data.u.intconst, buf);
            break;
        case TYPEFLOAT:
            CMach_InitFloatMem(obj->type, *(Float *)obj->u.data.u.string, buf);
            break;
        default:
            CError_FATAL(2735);
            break;
    }
    if (is_const_object(obj))
        emit_object(obj, buf, NULL, obj->type->size, 1);
    else
        emit_object(obj, buf, NULL, obj->type->size, 0);
}

void initialize_class_array(Object *obj, Type *type, Boolean staticInit)
{
    Object *dtor;
    SInt32 count;
    Object *ctor;
    DeclInfo declaration;
    ENode *node;
    Statement *statement;
    SInt32 i;
    SInt32 offset;
    Object *arrayDtor;
    Object *registrationObject;
    ENode *dtorRef;
    Type *registrationType;

    dtor = CClass_Destructor((TypeClass *)type);
    count = obj->type->size / type->size;
    if (CClass_Constructor((TypeClass *)type) != NULL) {
        ctor = CClass_DefaultConstructor((TypeClass *)type);
        if (ctor == NULL) {
            CError_ReportError(ERR_CLASS_NO_DEFAULT_CONSTRUCTOR);
            return;
        }
    } else {
        ctor = NULL;
    }
    if (count <= 1 || (staticInit == 0 && count <= 8)) {
        if (staticInit != 0) {
            for (i = 0; i < count; i++) {
                initialize_class_object(obj, type, NULL, i * type->size, 0);
            }
        } else {
            for (i = 0; i < count; i++) {
                offset = i * type->size;
                node = create_objectrefnode(obj);
                if (offset != 0) {
                    node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, offset), EADD);
                }
                if (ctor != NULL) {
                    node = CExpr_ConstructObject(type, node, NULL, 0, 1, 0, 1, 1);
                }
                cinit_state->insert_expr_cb(node);
                if (dtor != NULL) {
                    CExcept_RegisterDestructorObject(obj, offset, dtor, 1);
                }
            }
            if (dtor != NULL) {
                statement = CFunc_AppendStatement(4);
                statement->expr.expression = nullnode();
            }
        }
    } else {
        if (ctor != NULL) {
            if (dtor != NULL) {
                dtorRef = create_objectrefnode(CABI_GetDestructorObject(dtor, 1));
            } else {
                dtorRef = nullnode();
            }
            node = CExpr_FuncCallSix(class_array_initializer, create_objectrefnode(obj), create_objectrefnode(ctor),
                                     dtorRef, intconstnode((Type *)&stunsignedlong, type->size),
                                     intconstnode((Type *)&stunsignedlong, count), NULL);
        } else {
            node = nullnode();
        }
        if (staticInit != 0) {
            if (dtor != NULL && copts.no_static_dtors == 0) {
                arrayDtor = CParser_NewCompilerDefFunctionObject();
                arrayDtor->name = CParser_AppendUniqueName("__arraydtor");
                arrayDtor->type = (Type *)&data_0055d5e8;
                arrayDtor->sclass = TK_STATIC;
                arrayDtor->qual = Q_INLINE;
                CParser_RegisterSingleExprFunction(arrayDtor,
                                                   funccallexpr(data_0058717c, create_objectrefnode(obj),
                                                                create_objectrefnode(CABI_GetDestructorObject(dtor, 1)),
                                                                intconstnode((Type *)&stsignedlong, type->size),
                                                                intconstnode((Type *)&stsignedlong, count)));
                node = makediadicnode(node, nullnode(), ECOMMA);
                node->rtype = (Type *)&void_ptr;
                registrationType = CDecl_NewStructType(void_ptr.size * 3, CMachine_GetTypeAlignment((Type *)&void_ptr));
                memclrw(&declaration, sizeof(declaration));
                declaration.thetype = registrationType;
                declaration.name = CParser_GetUniqueName();
                declaration.qual = 0;
                declaration.storageclass = TK_STATIC;
                declaration.requireMangledName = 1;
                registrationObject = CParser_NewObject(&declaration);
                registrationObject->nspace = cscope_root;
                CodeGen_SetObjectSectionAndInterruptInfo(registrationObject);
                emit_object(registrationObject, NULL, NULL, registrationObject->type->size, 0);
                node = funccallexpr(destructor_registration_func, node, create_objectrefnode(arrayDtor),
                                    create_objectrefnode(registrationObject), NULL);
            }
            if (cinit_state->useEmitCallback != 0) {
                cinit_state->init_expr_register_cb(node);
            } else {
                InitExpr_Register(node, obj);
            }
        } else {
            statement = CFunc_AppendStatement(4);
            statement->expr.expression = node;
            if (dtor != NULL) {
                CException_RegisterMemberArray(statement, obj, dtor, count, type->size);
                statement = CFunc_AppendStatement(4);
                statement->expr.expression = nullnode();
            }
        }
    }
}

ENode *create_scopebegin_node(Type *type)
{
    ENode *node;
    node = CExpr2_NewESCOPEBEGINNode(type, 0);
    if (type->type == TYPECLASS && CClass_Destructor((TypeClass *)type))
        node->data.scopebegin.state = 1;
    return node;
}

ENode *create_temp_object_expr(Type *type, Boolean reportError)
{
    ENode *call;
    Object *cls;
    Type *tempType;
    ENode *result;
    Object *object;
    Object *argumentObject;

    object = CreateTempObject(type);
    result = create_objectrefnode(object);

    if (type->type == TYPECLASS) {
        cls = CClass_Destructor((TypeClass *)type);
        if (cls != NULL && !copts.no_static_dtors) {
            if (reportError) {
                CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            }
            call = galloc(sizeof(ENode));
            call->type = EFUNCCALL;
            call->cost = 200;
            call->flags = 0;
            call->rtype = CDecl_NewPointerType(type);
            call->data.funccall.funcref = create_objectrefnode(destructor_registration_func);
            call->data.funccall.functype = (TypeFunc *)destructor_registration_func->type;
            call->data.funccall.args = CompilerTools_AllocatePool(sizeof(ENodeList));
            call->data.funccall.args->node = result;
            call->data.funccall.args->next = CompilerTools_AllocatePool(sizeof(ENodeList));
            call->data.funccall.args->next->node = create_objectrefnode(CABI_GetDestructorObject(cls, 1));
            call->data.funccall.args->next->next = CompilerTools_AllocatePool(sizeof(ENodeList));

            tempType = CDecl_NewStructType(3 * void_ptr.size, CMachine_GetTypeAlignment((Type *)&void_ptr));
            argumentObject = CreateTempObject(tempType);

            call->data.funccall.args->next->next->node = create_objectrefnode(argumentObject);
            call->data.funccall.args->next->next->next = NULL;
            result = call;
        }
    }
    return result;
}

void emit_indirect_assignment(Type *type, ENode *expr, Boolean flag)
{
    ENode *result;
    SInt32 off;
    ENode *node;

    node = create_objectrefnode(cinit_state->obj);
    if (node->rtype->type != TYPEPOINTER) {
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
        return;
    }
    TPTR_TARGET(node->rtype) = type;
    off = cinit_state->expr_offset;
    if (off != 0) {
        node = makediadicnode(node, intconstnode((Type *)&stunsignedlong, off), EADD);
    }
    node = makemonadicnode(node, EINDIRECT);
    node->rtype = type;
    result = makediadicnode(node, expr, EASS);
    if (cinit_state->useEmitCallback != 0) {
        cinit_state->init_expr_register_cb(result);
    } else {
        InitExpr_Register(result, cinit_state->obj);
    }
}

void find_dtor_temp_arg(ENode *e)
{
    ENodeList *args;

    while (e->type == ECOMMA)
        e = e->data.diadic.right;
    if (e->rtype->type == TYPEPOINTER && TPTR_TARGET(e->rtype)->type == TYPECLASS) {
        switch (e->type) {
            case ETYPCON:
                find_dtor_temp_arg(e->data.monadic);
                return;
            case EADD:
            case ESUB:
                find_dtor_temp_arg(e->data.diadic.left);
                find_dtor_temp_arg(e->data.diadic.right);
                return;
            case EFUNCCALL:
            case EFUNCCALLP:
                if ((args = e->data.funccall.args) != NULL &&
                    (CInit_IsDtorTemp(args->node) || ((args = args->next) != NULL && CInit_IsDtorTemp(args->node)))) {
                    if (data_00581ba0 == NULL)
                        data_00581ba0 = args;
                    else
                        data_00581ba4 = 1;
                }
                return;
        }
    }
}

Boolean initialize_from_assignment(Object *obj, Boolean flag)
{
    ENode *create_scopebegin_node(Type *, Boolean);

    Type *type;
    SInt32 bufferSize;
    Object *temporary;
    ENode *initializer;

    if (tk == '=') {
        if (flag)
            data_0058757c = create_scopebegin_node;
        else
            data_0058757c = create_temp_object_expr;
        tk = CPrepTokenizer_GetNextToken();
        initializer = assignment_expression();
        initializer = oldassignmentpromotion(initializer, obj->type, obj->qual & Q_CV, 1);
        data_0058757c = NULL;
        if (flag) {
            type = obj->type;
            CError_ASSERT(3042, type->type == TYPEPOINTER);
            if (TYPE_POINTER(type)->target->type == TYPECLASS) {
                data_00581ba0 = NULL;
                data_00581ba4 = 0;
                find_dtor_temp_arg(initializer);
                if (data_00581ba0 != NULL) {
                    CError_ASSERT(3050, data_00581ba4 == 0);
                    temporary = create_temp_object(data_00581ba0->node->data.temp.type);
                    cinit_state->register_object_cb(data_00581ba0->node->data.temp.type, temporary, 0, 0);
                    data_00581ba0->node = create_objectrefnode(temporary);
                }
            }
            cinit_state->insert_expr_cb(makediadicnode(CExpr_New_EINDIRECT_Node(obj), initializer, EASS));
        } else {
            cinit_state->expr_cb = emit_indirect_assignment;
            cinit_state->bufferSize = bufferSize = obj->type->size;
            if (bufferSize == 0)
                bufferSize = 0x200;
            else if (bufferSize & 1)
                bufferSize++;
            cinit_state->buffer = CompilerTools_AllocatePool(bufferSize);
            cinit_state->bufferUsed = bufferSize;
            memclrw(cinit_state->buffer, bufferSize);
            init_int_or_relocation(obj->type, initializer);
            CError_ASSERT(3091, obj->type->size == cinit_state->bufferSize);
            if (cinit_state->list != NULL || !CInit_IsZero(cinit_state->buffer, obj->type->size)) {
                CanAllocObject(obj->type);
                (void)obj->type->size;
                emit_object(obj, cinit_state->buffer, cinit_state->list, obj->type->size, 0);
            } else {
                CanAllocObject(obj->type);
                (void)obj->type->size;
                emit_object(obj, NULL, NULL, obj->type->size, 0);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* The initialization of OBJ (or of a temporary when OBJ is NULL) of TYPE from its parsed initializer: a vector whose
   elements are one small value is that constant; otherwise an anonymous static object holds the constant part, the object
   is copied from it and the parts computed at run time are assigned after. */
#include <string.h>

ENode *CInit_AutoObject(Object *object, Type *type, UInt32 qualifiers)
{
    InitializerData initializer;
    DeclInfo declaration;
    ENode *reference, *expression, *result;
    Object *constantObject;
    InitializerEntry *entry;
    Type *constantType;
    int index;
    int structureKind;

    CInit_004d2700(&initializer, type, qualifiers, copts.cplusplus || copts.gcc_extensions || object == NULL);
    if (type->type == TYPESTRUCT && (structureKind = TYPE_STRUCT(type)->stype) >= 4 && structureKind <= 14) {
        switch (structureKind) {
            case 4:
            case 5:
            case 6: {
                signed char *elements = (signed char *)initializer.buffer;
                signed char value = elements[0];
                Boolean same = 1;
                for (index = 1; same && index < 16; index++)
                    same = value == elements[index];
                if (same && value < 16 && value > -17)
                    return intconstnode(type, value);
                break;
            }
            case 7:
            case 8:
            case 9:
            case 14: {
                int index;
                short *elements;
                short value;
                Boolean same;
                elements = (short *)initializer.buffer;
                value = elements[0];
                same = 1;
                for (index = 1; same && index < 8; index++)
                    same = value == elements[index];
                if (same && value < 16 && value > -17)
                    return intconstnode(type, value);
                break;
            }
            case 10:
            case 11:
            case 12: {
                int index;
                SInt32 *elements;
                SInt32 value;
                Boolean same;
                elements = (SInt32 *)initializer.buffer;
                value = elements[0];
                same = 1;
                for (index = 1; same && index < 4; index++)
                    same = value == elements[index];
                if (same && value < 16 && value > -17)
                    return intconstnode(type, value);
                break;
            }
        }
    }
    if (initializer.size_adjustment) {
        type = CDecl_NewStructType(type->size + initializer.size_adjustment, CMachine_GetTypeAlignment(type));
        if (object)
            object->type = type;
    }
    if (object)
        reference = create_objectrefnode(object);
    else
        reference = CExpr2_NewESCOPEBEGINNode(type, 1);
    if ((constantType = type)->type == TYPEARRAY)
        constantType = CDecl_NewStructType(type->size, CMachine_GetTypeAlignment(type));
    memclrw(&declaration, sizeof(declaration));
    declaration.thetype = constantType;
    declaration.name = CParser_GetUniqueName();
    declaration.qual = 0;
    declaration.storageclass = 0x102;
    declaration.requireMangledName = 1;
    constantObject = CParser_NewObject(&declaration);
    constantObject->nspace = cscope_root;
    emit_object(constantObject, initializer.buffer, initializer.relocations, initializer.size, 1);
    expression = makemonadicnode(reference, EINDIRECT);
    expression->rtype = constantType;
    expression = makediadicnode(expression, create_objectnode(constantObject), EASS);
    for (entry = initializer.entries; entry; entry = entry->next)
        expression = build_init_assignment(expression, reference, entry->offset, entry->type, entry->expression);
    if (!object) {
        result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        *result = *reference;
        result = makemonadicnode(result, EINDIRECT);
        result->rtype = type;
        result->flags = qualifiers & ENODE_FLAG_QUALS;
        expression = makecommaexpression(expression, result);
    }
    return expression;
}

void fn_004cfc50(Object *object)
{
    ENode *objectExpr;
    ENode *expr;
    InitializerEntry *entry;
    InitializerData initData;

    CInit_004d2700(&initData, object->type, object->qual, copts.cplusplus);
    objectExpr = create_objectrefnode(object);
    CanAllocObject(object->type);
    if (initData.entries == NULL && is_const_object(object) != 0) {
        emit_object(object, initData.buffer, initData.relocations, initData.size, 1);
    } else {
        emit_object(object, initData.buffer, initData.relocations, initData.size, 0);
    }
    if (initData.entries != NULL) {
        entry = initData.entries;
        for (expr = NULL; entry != NULL; entry = entry->next) {
            expr = build_init_assignment(expr, objectExpr, entry->offset, entry->type, entry->expression);
        }
        InitExpr_Register(expr, object);
    }
}

void CInit_InitializeAutoData(Object *obj, void (*emitInitializer)(ENode *),
                              void (*registerDestructor)(Type *, Object *, SInt32, SInt32))
{
    Type *type = obj->type;
    Type *elementType;
    Boolean simpleInitialization;
    Boolean hasConstructor;
    InitInfo save;

    switch ((SInt8)type->type) {
        case TYPESTRUCT:
            simpleInitialization = 1;
            break;
        case TYPEARRAY:
            while (type->type == TYPEARRAY)
                type = TPTR_TARGET(type);
            if (type->type != TYPECLASS) {
                simpleInitialization = 1;
                break;
            }
        case TYPECLASS:
            hasConstructor = CClass_Constructor(TYPE_CLASS(type)) || CClass_Destructor((TypeClass *)type);
            simpleInitialization = !hasConstructor;
            break;
        default:
            simpleInitialization = 0;
            break;
    }
    if (simpleInitialization) {
        if (tk == '=' || (tk == '(' && copts.cplusplus != 0)) {
            if (tk == '=' && ((tk = CPrepTokenizer_GetNextToken()) == 0x7b || obj->type->type == TYPEARRAY)) {
                emitInitializer(CInit_AutoObject(obj, obj->type, obj->qual));
                return;
            }
            {
                ENode *expression;
                expression = conv_assignment_expression();
                expression = oldassignmentpromotion(expression, obj->type, obj->qual & Q_CV, 1);
                emitInitializer(makediadicnode(CExpr_New_EINDIRECT_Node(obj), expression, 0x1e));
            }
            return;
        } else {
            if (copts.cplusplus != 0 && is_const_object(obj) != 0)
                CError_ReportError(ERR_CONST_AMPERSAND_VARIABLE_NEEDS_INITIALIZER);
            return;
        }
    }
    CInit_SetupSave(&save, obj, emitInitializer, registerDestructor);

    if (obj->type->type == TYPECLASS && CInit_004d20e0(obj->type, NULL, 0, 1) != 0) {
        cinit_state = save.next;
        return;
    }
    if ((type = obj->type)->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE) != 0 &&
        initialize_from_assignment(obj, 1) != 0) {
        cinit_state = save.next;
        return;
    }
    if (tk != '=' && (tk != '(' || copts.cplusplus == 0)) {
        if ((elementType = obj->type)->type == TYPEARRAY) {
            while (elementType->type == TYPEARRAY)
                elementType = TPTR_TARGET(elementType);
            if (elementType->type == TYPECLASS) {
                hasConstructor =
                    CClass_Constructor(TYPE_CLASS(elementType)) || CClass_Destructor((TypeClass *)elementType);
                if (hasConstructor) {
                    initialize_class_array(obj, elementType, 0);
                    cinit_state = save.next;
                    return;
                }
                fn_00476e60(TYPE_CLASS(elementType));
            }
        }
        if (obj->type->type == TYPECLASS)
            fn_00476e60(TYPE_CLASS(obj->type));
        if (((type = obj->type)->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE) != 0) ||
            is_const_object(obj) != 0) {
            if (copts.cplusplus != 0)
                CError_ReportError(ERR_CONST_AMPERSAND_VARIABLE_NEEDS_INITIALIZER);
        }
    } else {
        if (obj->type->size != 0 || obj->type->type == TYPEARRAY) {
            ENode *initializer = CInit_004d0ae0(NULL, obj->type, obj->qual, CInit_004d1170, 0);
            if (initializer != NULL)
                emitInitializer(makediadicnode(CExpr_New_EINDIRECT_Node(obj), initializer, 0x1e));
        } else {
            CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        }
    }
    if (obj->type->type == TYPECLASS && CClass_Destructor((TypeClass *)obj->type) != NULL)
        registerDestructor(obj->type, obj, 0, 0);
    cinit_state = save.next;
}

void CInit_InitializeStaticData(Object *initObject, void (*output)(ENode *))
{
    InitInfo context;
    InitializerData initializer;
    short needsConstruction;
    char classification;
    Boolean flag;

    classification = classify(initObject->type);
    if ((char)classification != 0) {
        if (tk == '=' || (tk == '(' && copts.cplusplus != 0)) {
            if (tk == '=')
                tk = CPrepTokenizer_GetNextToken();
            flag = copts.cplusplus;
            CInit_004d2700(&initializer, initObject->type, initObject->qual, flag);
            CanAllocObject(initObject->type);
            if (initializer.entries == NULL && is_const_object(initObject) != 0)
                emit_object(initObject, initializer.buffer, initializer.relocations, initializer.size, 1);
            else
                emit_object(initObject, initializer.buffer, initializer.relocations, initializer.size, 0);
            if (initializer.entries != NULL) {
                ENode *expression;
                InitializerEntry *entry;
                ENode *target;
                target = create_objectrefnode(initObject);
                entry = initializer.entries;
                expression = NULL;
                while (entry != NULL) {
                    expression =
                        build_init_assignment(expression, target, entry->offset, entry->type, entry->expression);
                    entry = entry->next;
                }
                output(expression);
            }
        } else {
            if (copts.cplusplus != 0 && is_const_object(initObject) != 0)
                CError_ReportError(ERR_CONST_AMPERSAND_VARIABLE_NEEDS_INITIALIZER);
            if (is_const_object(initObject) != 0)
                emit_object(initObject, NULL, NULL, initObject->type->size, 1);
            else
                emit_object(initObject, NULL, NULL, initObject->type->size, 0);
        }
        return;
    }
    memclrw(&context, 56);
    context.obj = initObject;
    cinit_state = (context.next = cinit_state, &context);
    context.useEmitCallback = 1;
    context.init_expr_register_cb = output;
    if (initObject->type->type == TYPECLASS && initialize_class_object(initObject, initObject->type, NULL, 0, 1) != 0) {
        CanAllocObject(initObject->type);
        emit_object(initObject, NULL, NULL, initObject->type->size, 0);
        cinit_state = context.next;
        return;
    }
    if (initObject->type->type == TYPEPOINTER && (((TypePointer *)initObject->type)->qual & Q_REFERENCE) != 0 &&
        initialize_from_assignment(initObject, 0) != 0) {
        cinit_state = context.next;
        return;
    }
    if (tk != '=' && (tk != '(' || copts.cplusplus == 0)) {
        if (CanAllocObject(initObject->type) != 0)
            emit_object(initObject, NULL, NULL, initObject->type->size, 0);
        {
            Type *arrayElement;
            if ((arrayElement = initObject->type)->type == TYPEARRAY) {
                while (arrayElement->type == TYPEARRAY)
                    arrayElement = ((TypePointer *)arrayElement)->target;
                if (arrayElement->type == TYPECLASS) {
                    needsConstruction = 1;
                    if (CClass_Constructor((TypeClass *)arrayElement) == NULL &&
                        CClass_Destructor((TypeClass *)arrayElement) == NULL)
                        needsConstruction = 0;
                    if ((char)needsConstruction != 0) {
                        initialize_class_array(initObject, arrayElement, 1);
                        cinit_state = context.next;
                        return;
                    }
                    fn_00476e60(TYPE_CLASS(arrayElement));
                }
            }
        }
        if (initObject->type->type == TYPECLASS)
            fn_00476e60(TYPE_CLASS(initObject->type));
        if ((initObject->type->type == TYPEPOINTER && (((TypePointer *)initObject->type)->qual & Q_REFERENCE) != 0) ||
            is_const_object(initObject) != 0) {
            if (copts.cplusplus != 0)
                CError_ReportError(ERR_CONST_AMPERSAND_VARIABLE_NEEDS_INITIALIZER);
        }
    } else if (initObject->type->size != 0 || initObject->type->type == TYPEARRAY) {
        ENode *expression;
        expression = CInit_004d0ae0(initObject, initObject->type, initObject->qual, initialize_object_at_offset, 1);
        if (expression != NULL)
            output(makediadicnode(CExpr_New_EINDIRECT_Node(initObject), expression, EASS));
    } else {
        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
    }
    cinit_state = context.next;
}

void CInit_InitializeData(Object *obj)
{
    Object *destructor;
    Type *ty;
    Type *a;
    TypeClass *t;
    Boolean b;
    SInt32 n;
    InitListItem *p;
    Boolean flag;
    UInt8 kind;
    struct InitInfo ctx;
    CInt64 v;

    if (tk == ':') {
        tk = CPrepTokenizer_GetNextToken();
        obj->datatype = DABSOLUTE;
        v = fn_004f0b30();
        obj->u.data.u.intconst.hi = v.lo;
        return;
    }
    if (!(tk == '=' || (tk == '(' && copts.cplusplus != 0))) {
        n = (SInt16)obj->sclass;
        if (n == 0x103)
            return;
        if (copts.cplusplus == 0 && n != 0x102) {
            p = tentative_init_list;
            while (p != NULL) {
                if (p->object == obj)
                    break;
                p = p->next;
            }
            if (p == NULL) {
                p = galloc(8);
                p->object = obj;
                p->next = tentative_init_list;
                tentative_init_list = p;
                obj->qual |= Q_TENTATIVE;
            }
            return;
        }
        if (obj->flags & OBJECT_DEFINED)
            CError_ReportError(ERR_DATA_OBJECT_REDEFINED, obj);
        obj->flags |= OBJECT_DEFINED;
        flag = 0;
        kind = (ty = obj->type)->type;
        if (kind == TYPEARRAY) {
            a = ty;
            while (a->type == TYPEARRAY)
                a = TPTR_TARGET(a);
            if (a->type == TYPECLASS) {
                b = CClass_Constructor((TypeClass *)a) || CClass_Destructor((TypeClass *)a);
                if (b) {
                    CInitPushSave(&ctx, obj);
                    initialize_class_array(obj, a, 1);
                    cinit_state = ctx.next;
                    flag = 1;
                }
                fn_00476e60(TYPE_CLASS(a));
            }
        } else if (kind == TYPECLASS) {
            t = TYPE_CLASS(ty);
            b = CClass_Constructor(t) || CClass_Destructor(t);
            if (b) {
                CInitPushSave(&ctx, obj);
                initialize_class_object(obj, obj->type, NULL, 0, 0);
                cinit_state = ctx.next;
                flag = 1;
            } else {
                fn_00476e60(TYPE_CLASS(obj->type));
            }
        }
        if (!flag && copts.cplusplus != 0 &&
            ((obj->type->type == TYPEPOINTER && (TYPE_POINTER(obj->type)->qual & Q_REFERENCE)) || is_const_object(obj)))
            CError_ReportError(ERR_CONST_AMPERSAND_VARIABLE_NEEDS_INITIALIZER);
        if (CanAllocObject(obj->type))
            emit_object(obj, NULL, NULL, obj->type->size, 0);
        return;
    }
    if (obj->flags & OBJECT_DEFINED)
        CError_ReportError(ERR_DATA_OBJECT_REDEFINED, obj);
    ty = obj->type;
    a = ty;
    switch ((SInt8)ty->type) {
        case TYPESTRUCT:
            b = 1;
            break;
        case TYPEARRAY:
            while (a->type == TYPEARRAY)
                a = TPTR_TARGET(a);
            if (a->type != TYPECLASS) {
                b = 1;
                break;
            }
        case TYPECLASS:
            b = CClass_Constructor(TYPE_CLASS(a)) || CClass_Destructor(TYPE_CLASS(a));
            b = !b;
            break;
        default:
            b = 0;
            break;
    }
    if (b) {
        if (tk == '=')
            tk = CPrepTokenizer_GetNextToken();
        else
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        fn_004cfc50(obj);
        return;
    }
    CInitPushSave(&ctx, obj);
    if ((ty = obj->type)->type == TYPECLASS && initialize_class_object(obj, ty, NULL, 0, 1)) {
        CanAllocObject(obj->type);
        emit_object(obj, NULL, NULL, obj->type->size, 0);
        cinit_state = ctx.next;
        return;
    }
    if (obj->type->type == TYPEPOINTER && (TYPE_POINTER(obj->type)->qual & Q_REFERENCE)) {
        if (initialize_from_assignment(obj, 0)) {
            cinit_state = ctx.next;
            return;
        }
    }
    if ((a = obj->type)->size == 0 && a->type != TYPEARRAY) {
        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
        cinit_state = ctx.next;
        return;
    }
    if (copts.cplusplus != 0)
        CInit_004d0ae0(obj, a, obj->qual, initialize_object_at_offset, 0);
    else
        CInit_004d0ae0(obj, a, obj->qual, NULL, 0);
    if (obj->type->type == TYPECLASS) {
        destructor = (Object *)CClass_Destructor((TypeClass *)obj->type);
        if (destructor != NULL) {
            ENode *result = create_destructor_registration_call(obj->type, destructor, create_objectrefnode(obj));
            InitExpr_Register(result, obj);
        }
    }
    cinit_state = ctx.next;
}

Object *CInit_DeclareString(const char *data, UInt32 length, UInt8 kind1, UInt8 kind2)
{
    PooledString *node;
    PooledString *cached;
    Object *object;

    if (copts.dont_reuse_strings == 0) {
        for (cached = string_cache; cached != NULL; cached = cached->next) {
            if (cached->size == length && cached->ispascal == kind1 && cached->iswide == kind2) {
                if (memcmp(cached->data, data, length) == 0)
                    return cached->obj;
            }
        }
    }

    object = CParser_NewCompilerDefDataObject();
    object->name = (HashNameNode *)CParser_GetUniqueName();
    if (kind2)
        object->type = CDecl_NewArrayType(CParser_GetWCharType(), length);
    else
        object->type = CDecl_NewArrayType(kind1 ? (Type *)&stunsignedchar : (Type *)&stchar, length);
    object->sclass = TK_STATIC;
    CScope_AddGlobalObject(object);

    if (copts.readonly_strings)
        emit_object(object, data, NULL, object->type->size, 1);
    else
        emit_object(object, data, NULL, object->type->size, 0);

    node = galloc(sizeof(PooledString));
    node->next = string_cache;
    string_cache = node;
    node->obj = object;
    node->offset = 0;
    node->size = length;
    node->ispascal = kind1;
    node->iswide = kind2;
    node->data = galloc(length);
    memcpy(node->data, data, length);
    return object;
}

NameEntry *CInit_DeclarePooledString(const char *name, SInt32 length, SInt8 unsignedChar)
{
    Object *stringObject;
    SInt32 offset;
    DeclInfo declaration;
    NameEntry *entry;

    if (copts.dont_reuse_strings == 0) {
        for (entry = pooled_strings; entry != NULL; entry = entry->next) {
            if (entry->length == length && entry->unsignedChar == unsignedChar &&
                memcmp(entry->bytes, name, length) == 0)
                return entry;
        }
    }
    if (pooled_strings != NULL) {
        stringObject = pooled_strings->object;
        offset = pooled_strings->offset + pooled_strings->length;
    } else {
        Type *arrayType;
        Type *charType;
        HashNameNode *baseName;
        Object *object;

        baseName = GetHashNameNode("@stringBase0");
        if (unsignedChar)
            charType = (Type *)&stunsignedchar;
        else
            charType = (Type *)&stchar;
        arrayType = CDecl_NewArrayType(charType, length);
        memclrw(&declaration, sizeof(declaration));
        declaration.thetype = arrayType;
        if (baseName == NULL)
            baseName = CParser_GetUniqueName();
        declaration.name = baseName;
        declaration.qual = 0;
        declaration.storageclass = 0x102;
        declaration.requireMangledName = 1;
        object = CParser_NewObject(&declaration);
        object->nspace = cscope_root;
        stringObject = object;
        ObjGen_PPC_EABI_SetObjectSection(object, 0, copts.readonly_strings);
        offset = 0;
    }
    entry = galloc(0x16);
    entry->next = pooled_strings;
    pooled_strings = entry;
    entry->object = stringObject;
    entry->offset = offset;
    entry->length = length;
    entry->unsignedChar = unsignedChar;
    entry->bytes = galloc(length);
    memcpy(entry->bytes, name, length);
    return entry;
}

NameEntry *CInit_DeclarePooledWString(char *string, UInt32 length)
{
    Type *type;
    HashNameNode *name;
    DeclInfo declaration;
    SInt32 offset;
    Object *object;
    NameEntry *node;

    if (copts.dont_reuse_strings == 0) {
        for (node = pooled_wstrings; node != NULL; node = node->next) {
            if (node->length == length && memcmp(node->bytes, string, length) == 0)
                return node;
        }
    }
    if (pooled_wstrings != NULL) {
        object = pooled_wstrings->object;
        offset = pooled_wstrings->offset + pooled_wstrings->length;
    } else {
        name = GetHashNameNode("@wstringBase0");
        type = CDecl_NewArrayType(CParser_GetWCharType(), length);
        memclrw(&declaration, sizeof(declaration));
        declaration.thetype = type;
        if (name == NULL)
            name = CParser_GetUniqueName();
        declaration.name = name;
        declaration.qual = 0;
        declaration.storageclass = 0x102;
        declaration.requireMangledName = 1;
        {
            Object *createdObject = CParser_NewObject(&declaration);
            createdObject->nspace = cscope_root;
            object = createdObject;
        }
        offset = 0;
    }
    node = galloc(0x16);
    node->next = pooled_wstrings;
    pooled_wstrings = node;
    node->object = object;
    node->offset = offset;
    node->length = length;
    node->unsignedChar = 0;
    node->bytes = galloc(length);
    memcpy(node->bytes, string, length);
    return node;
}

void CInit_RewriteString(ENode *node, SInt32 flag_arg)
{
    NameEntry *res;
    Boolean flag;

    if (cprep_cu[0xe0] == 1)
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);

    CError_ASSERT(4019, node->rtype->type == TYPEPOINTER);

    flag = (TPTR_TARGET(node->rtype)->size != 1);

    if (copts.poolstrings != 0) {
        if (flag)
            res = CInit_DeclarePooledWString(node->data.string.data, node->data.string.size);
        else
            res = CInit_DeclarePooledString(node->data.string.data, node->data.string.size, node->data.temp.needs_dtor);

        if (res->offset != 0) {
            node->type = EADD;
            node->data.diadic.right = intconstnode((Type *)&stunsignedlong, res->offset);
            node->data.diadic.left = create_objectrefnode(res->object);
            node->cost = 1;
        } else {
            node->type = EOBJREF;
            node->data.objref = res->object;
        }
    } else {
        node->type = EOBJREF;
        node->data.objref =
            CInit_DeclareString(node->data.string.data, node->data.string.size, node->data.temp.needs_dtor, flag);
    }
}

void CInit_DeclarePooledStrings(void)
{
    NameEntry *entry;
    SInt32 size;
    char *buffer;

    size = 0;
    for (entry = pooled_strings; entry != NULL; entry = entry->next)
        size += entry->length;
    if (size != 0) {
        pooled_strings->object->type = CDecl_NewArrayType((Type *)&stchar, size);
        buffer = galloc(size);
        for (entry = pooled_strings; entry != NULL; entry = entry->next)
            memcpy(buffer + entry->offset, entry->bytes, entry->length);
        if (CInit_StringEmissionMode())
            emit_object(pooled_strings->object, buffer, NULL, size, 1);
        else
            emit_object(pooled_strings->object, buffer, NULL, size, 0);
    }

    size = 0;
    for (entry = pooled_wstrings; entry != NULL; entry = entry->next)
        size += entry->length;
    if (size != 0) {
        pooled_wstrings->object->type = CDecl_NewArrayType(CParser_GetWCharType(), size);
        buffer = galloc(size);
        for (entry = pooled_wstrings; entry != NULL; entry = entry->next)
            memcpy(buffer + entry->offset, entry->bytes, entry->length);
        if (CInit_StringEmissionMode())
            emit_object(pooled_wstrings->object, buffer, NULL, size, 1);
        else
            emit_object(pooled_wstrings->object, buffer, NULL, size, 0);
    }
}

void emit_object(Object *object, const void *buffer, struct OLinkList *args, unsigned int options, Boolean useAlternate)
{
    OLinkList *argument;
    unsigned int savedQual = object->qual;

    if (cprep_cu[224] == 1U) {
        CException_AddPendingBuffer(object, buffer, args, options);
        return;
    }
    object->flags |= OBJECT_DEFINED;
    if (!func_errors) {
        for (argument = args; argument; argument = argument->next)
            CInline_0050f240(argument->obj);
        if (copts.filesyminfo)
            fn_0043f1f0(&function_fileinfo);
        if (useAlternate)
            ObjGen_PPC_EABI_EmitObject(object, buffer, args, options);
        else
            ObjGen_PPC_EABI_EmitObjectWithDebugEntry(object, buffer, args, options);
        object->qual = savedQual;
    }
}

void fn_004ceab0(Object *object, void *buffer, void *args, SInt32 size)
{
    emit_object(object, buffer, args, size, 0);
}

void fn_004cea90(Object *object, void *buffer, void *args, int size)
{
    emit_object(object, buffer, args, size, 1);
}

void CInit_DefineTentativeData(void)
{
    struct InitListItem *entry;

    entry = tentative_init_list;
    if (entry != NULL) {
        do {
            if ((entry->object->flags & 4) == 0) {
                emit_object(entry->object, NULL, NULL, entry->object->type->size, 0);
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    tentative_init_list = NULL;
}
