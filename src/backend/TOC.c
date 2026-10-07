#define CERROR_FILE "TOC.c"
#include "compiler/common.h"
#include "compiler/TOC.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include <string.h>
#include "compiler/ENode.h"
#include "compiler/Objects.h"

#include <stdio.h>

static CInt64 data_0055e598 = {0, 0};
static struct TOCReferenceEntry *toc_references;

/* The vector lvsl gives for each shift: constants equal to one need no load. */
char vector128_patterns[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
};

/* The vector lvsr gives for each shift. */
MWVector128 alternate_vector_patterns[16] = {
    {{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}},
    {{0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E}},
    {{0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D}},
    {{0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C}},
    {{0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}},
    {{0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A}},
    {{0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19}},
    {{0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18}},
    {{0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17}},
    {{0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16}},
    {{0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15}},
    {{0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14}},
    {{0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13}},
    {{0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12}},
    {{0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11}},
    {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}},
};

Object *TOC_CreateSinitObject(void)
{
    UInt8 buf[256];
    char name[100];
    Object *func;
    TypeFunc *ftype;
    char *p;

    CompilerTools_GetPFileFields(&((CPrepCU *)cprep_cu)->mainFile, NULL, NULL, buf);
    sprintf(name, "__sinit_%*.*s", -buf[0], buf[0], (char *)&buf[1]);
    p = name + 1;
    while (*p != 0) {
        if (*p == '.')
            *p = '_';
        p++;
    }
    ftype = (TypeFunc *)galloc(sizeof(TypeFunc));
    memclrw(ftype, sizeof(*ftype));
    ftype->type = TYPEFUNC;
    ftype->functype = &stvoid;
    ftype->args = NULL;
    ftype->flags = FUNC_DEFINED;
    func = (Object *)galloc(sizeof(Object));
    memclrw(func, sizeof(*func));
    func->otype = OT_OBJECT;
    func->type = (Type *)ftype;
    func->name = GetHashNameNode(name);
    func->sclass = TK_STATIC;
    func->datatype = DFUNC;
    ObjGen_PPC_EABI_SetObjectSection(func, 4, 1);
    BE_symbol_GetOrCreateFunctionObjectSymbol(func);
    return func;
}

static inline char TOC_RegisterClass(const EncodedOperand *operand)
{
    return operand->modifier.reg.register_class;
}

void fn_0049d420(Statement *statements)
{
    Statement *statement;
    int special;
    int classOrAggregate;
    int arrayOrStruct;
    int memberPointer;
    ParsedAsmInstruction *instruction;
    ENode *expression;
    EncodedOperand *operand;
    int operandIndex;

    toc_name_entries = NULL;
    data_00588521 = 1;
    data_00588500 = 0;
    data_0058852d = 0;
    gInitialObjectList_005882ac = NULL;
    statement = statements->next;
    if (statement != NULL) {
        do {
            unsigned short statementNumber = statement->value;
            current_statement_number = statementNumber;
            if ((statement->flags & 1) != 0) {
                gRunLevel2Pipeline = 1;
                data_0058852d = 1;
            }
            switch (statement->type) {
                case ST_EXPRESSION:
                    TOC_0049d710(statement->expr.expression, NULL, 1);
                    if ((expression = statement->expr.expression)->type != ETYPCON ||
                        expression->rtype->type != TYPEVOID) {
                        break;
                    }
                    statement->expr.expression = expression->data.monadic;
                    break;
                case ST_GOTOEXPR:
                    TOC_0049d710(statement->expr.expression, NULL, 0);
                    break;
                case ST_IFGOTO:
                case ST_IFNGOTO:
                    if (fn_0049f630(statement->expr.expression) == 0) {
                        statement->expr.expression = create_diadic_node_with_constant(statement->expr.expression);
                    }
                    TOC_0049d710(statement->expr.expression, NULL, 0);
                    break;
                case ST_RETURN:
                    if (statement->expr.expression == NULL) {
                        continue;
                    }
                    special = 1;
                    classOrAggregate = 1;
                    arrayOrStruct = 1;
                    if (statement->expr.expression->rtype->type != TYPEARRAY &&
                        statement->expr.expression->rtype->type != TYPESTRUCT) {
                        arrayOrStruct = 0;
                    }
                    if (arrayOrStruct == 0 && statement->expr.expression->rtype->type != TYPECLASS) {
                        classOrAggregate = 0;
                    }
                    if (classOrAggregate == 0) {
                        memberPointer = 0;
                        if (statement->expr.expression->rtype->type == TYPEMEMBERPOINTER &&
                            statement->expr.expression->rtype->size == 12) {
                            memberPointer = 1;
                        }
                        if (memberPointer == 0) {
                            special = 0;
                        }
                    }
                    TOC_0049d710(statement->expr.expression, NULL, special);
                    break;
                case ST_SWITCH:
                    data_00588500 = 1;
                    TOC_0049d710(statement->expr.expression, NULL, 0);
                    break;
                case ST_ENDCATCHDTOR:
                    data_00588521 = 0;
                    break;
                case ST_ASM:
                    operandIndex = 0;
                    instruction = statement->expr.asmInstruction;
                    operand = instruction->data.operands;
                    while (operandIndex < instruction->operand_count) {
                        if (operand->kind == 2 && operand->target.object == NULL) {
                            if (TOC_RegisterClass(operand) == 0) {
                                Registers_BindGPR(operand->target.object, operand->data.value);
                            } else if (TOC_RegisterClass(operand) == 1) {
                                Registers_BindFPR(operand->target.object, operand->data.value);
                            } else if (TOC_RegisterClass(operand) == 9) {
                                Registers_BindVR(operand->target.object, operand->data.value);
                            }
                        }
                        operandIndex++;
                        operand++;
                    }
                    if ((instruction->specialFlags & 2) != 0) {
                        data_00588521 = 0;
                    }
            }
            add_exception_initial_objects(statement->dobjstack);
        } while ((statement = statement->next) != NULL);
    }
    rewrite_indirect_toc_references();
}

static inline void freeFirstOperand(CException *node)
{
    add_initial_object(node->data.slots[0]);
}

static inline void freeOperandPair(CException *node)
{
    freeFirstOperand(node);
    add_initial_object(node->data.slots[1]);
}

void add_exception_initial_objects(CException *node)
{
    for (; node != NULL; node = node->next) {
        switch (node->kind) {
            case 1:
                add_initial_object(node->data.slots[0]);
                break;
            case 2:
                add_initial_object(node->data.slots[0]);
                add_initial_object(node->data.slots[1]);
                break;
            case 3:
                add_initial_object(node->data.slots[0]);
                break;
            case 4:
                add_initial_object(node->data.slots[0]);
                break;
            case 5:
                add_initial_object(node->data.slots[0]);
                break;
            case 17:
                add_initial_object(node->data.slots[0]);
                break;
            case 6:
                add_initial_object(node->data.slots[0]);
                add_initial_object(node->data.slots[1]);
                add_initial_object(node->data.slots[3]);
                break;
            case 7:
                add_initial_object(node->data.slots[0]);
                break;
            case 8:
                add_initial_object(node->data.slots[0]);
                add_initial_object(node->data.slots[1]);
                break;
            case 9:
                add_initial_object(node->data.slots[0]);
                break;
            case 10:
            case 11:
                add_initial_object(node->data.slots[0]);
                break;
            case 12:
                add_initial_object(node->data.slots[0]);
                add_initial_object(node->data.slots[2]);
                break;
            case 13:
                add_initial_object(node->data.slots[0]);
                add_initial_object(node->data.slots[1]);
                break;
            case 14:
                add_initial_object(node->data.slots[0]);
                break;
        }
    }
}

/* The flag is a field of the option block at 0x584220: its two reads in one
 * condition share a CSE temporary, which a plain global never does. */

static inline int TOC_IsSetjmp(ENode *fr)
{
    if (fr->type == EOBJREF) {
        if (memcmp(COptimizer_GetFunctionObject(fr->data.objref)->name, "__setjmp", 9) == 0)
            return 1;
        if (memcmp(COptimizer_GetFunctionObject(fr->data.objref)->name, "__vec_setjmp", 13) == 0)
            return 1;
    }
    return 0;
}

void TOC_0049d710(ENode *node, Type *targetType, int ignored)
{
    UInt8 kind;
    ENode *expr;
    ENode *operand;
    ENodeList *args;
    ENode *left;

    node->ignored = ignored;
    switch (kind = node->type) {
        case EINTCONST:
            node->hascall = 0;
            break;
        case EFLOATCONST: {
            ENode *ref;
            Object *object;
            Object *entry;
            if (copts.operandsDebug) {
                node->hascall = 0;
                break;
            }
            object = TOC_GetFloatObject(node->rtype, &node->data.floatval);
            data_00588500 = 1;
            if (PCodeUtilities_Require(object)) {
                ref = CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(ref, sizeof(ENode));
                ref->type = EOBJREF;
                ref->cost = 0;
                ref->data.objref = object;
                ref->rtype = CDecl_NewPointerType(node->rtype);
                node->type = EINDIRECT;
                node->cost = 1;
                node->data.diadic.left = ref;
                node->hascall = 0;
                break;
            }
            if (copts.reuseSectionSymbols && (entry = BE_symbol_GetFunctionSymbolLinkData(object)) != NULL) {
                add_toc_reference(entry, object, node, 1);
            } else {
                ref = CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(ref, sizeof(ENode));
                ref->type = EOBJREF;
                ref->cost = 0;
                ref->data.objref = object;
                ref->rtype = node->rtype;
                node->type = EINDIRECT;
                node->cost = 1;
                node->data.diadic.left = ref;
                CError_ASSERT(648, object->extraQualifiers != 0);
                BE_symbol_GetOrCreateFunctionObjectSymbol(object);
            }
            node->hascall = 0;
            break;
        }
        case EASSBLK:
            if (!is_small_splat_or_table_vector((long)&node->data, node->rtype)) {
                data_00588500 = 1;
                replace_vector_constant_with_objectref(node);
                TOC_0049d710(node, NULL, 0);
            }
            break;
        case ESTRINGCONST:
            data_00588500 = 1;
            CInit_RewriteString(node, 1);
            TOC_0049d710(node, NULL, 0);
            break;
        case EOBJREF: {
            Object *object = node->data.objref;
            if (object->datatype == DALIAS)
                CError_FATAL(1998);
            DWARF_AddPendingObject(object);
            if (object->datatype == DFUNC || object->datatype == DVFUNC)
                data_00588500 = 1;
            if (object->datatype == DDATA) {
                if (!PCodeUtilities_Require(object)) {
                    if (copts.reuseSectionSymbols && object->datatype == DDATA) {
                        Object *entry = BE_symbol_GetFunctionSymbolLinkData(object);
                        if (entry) {
                            add_toc_reference(entry, object, node, 0);
                            data_00588500 = 1;
                            node->hascall = 0;
                            break;
                        }
                    }
                    data_00588500 = 1;
                    CError_ASSERT(648, object->extraQualifiers != 0);
                    BE_symbol_GetOrCreateFunctionObjectSymbol(object);
                    node->hascall = 0;
                    break;
                }
            }
            object->datatype == DABSOLUTE;
            node->hascall = 0;
            break;
        }
        case ECOND:
            if (!Operands_IsLogicalExpression(node->data.diadic.left)) {
                if (!fn_0049f630(node->data.diadic.left)) {
                    node->data.diadic.left = create_diadic_node_with_constant(node->data.diadic.left);
                }
            }
            TOC_0049d710(node->data.diadic.left, NULL, 0);
            TOC_0049d710(node->data.diadic.right, NULL, ignored);
            TOC_0049d710(node->data.cond.expr2, NULL, ignored);
            node->hascall =
                node->data.diadic.left->hascall | node->data.diadic.right->hascall | node->data.cond.expr2->hascall;
            break;
        case EFUNCCALL:
        case EFUNCCALLP:
            if (Intrinsics_IsMonadicObjrefTypeFuncFlag200Set(node)) {
                node->hascall = 0;
                if ((UInt16)node->data.funccall.funcref->data.objref->u.func.u == 8) {
                    data_0058852d = 1;
                } else if ((UInt16)node->data.funccall.funcref->data.objref->u.func.u == 0x23) {
                    if (node->data.funccall.args->next->node->type == ESTRINGCONST) {
                        fn_0049ebb0(node);
                    } else {
                        data_00588521 = 0;
                        node->hascall = 1;
                    }
                } else if ((UInt16)node->data.funccall.funcref->data.objref->u.func.u == 0x24) {
                    ENode *argument = node->data.funccall.args->next->next->node;
                    if (argument->type != EINTCONST) {
                        data_00588521 = 0;
                        node->hascall = 1;
                    }
                }
            } else {
                data_00588521 = 0;
                node->hascall = 1;
            }
            {
                ENode *function;
                if (TOC_IsSetjmp(node->data.funccall.funcref)) {
                    data_00588224 |= 1;
                    if (copts.f08)
                        data_00588224 |= 2;
                }
                function = node->data.funccall.funcref;
                if (function->type == EINDIRECT && function->rtype->type == TYPEFUNC) {
                    *function = *function->data.diadic.left;
                }
                if (node->data.funccall.funcref->type == EOBJREF) {
                    node->data.funccall.funcref->hascall = 0;
                    if (node->data.funccall.funcref->data.objref->datatype == DVFUNC &&
                        (node->data.funccall.funcref->flags & ENODE_FLAG_80)) {
                        Object *object = galloc(sizeof(Object));
                        *object = *node->data.funccall.funcref->data.objref;
                        object->datatype = DFUNC;
                        node->data.funccall.funcref->data.objref = object;
                    }
                } else {
                    TOC_0049d710(node->data.funccall.funcref, NULL, 0);
                }
                for (args = node->data.funccall.args; args; args = args->next) {
                    TOC_0049d710(args->node, NULL, 0);
                }
                if (Intrinsics_IsMonadicObjrefTypeFuncFlag200Set(node)) {
                    for (args = node->data.funccall.args; args; args = args->next) {
                        node->hascall |= args->node->hascall;
                    }
                }
                if (Type_RequiresMemoryReturn(node->data.funccall.functype->functype)) {
                    ENode *result = node->data.funccall.args->node;
                    ENode *call = CompilerTools_AllocatePool(sizeof(ENode));
                    memclrw(call, sizeof(ENode));
                    *call = *node;
                    node->type = ECOMMA;
                    node->data.diadic.left = call;
                    node->data.diadic.right = result;
                }
            }
            break;
        case ECOMMA:
            TOC_0049d710(node->data.diadic.left, NULL, 1);
            TOC_0049d710(node->data.diadic.right, NULL, ignored);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            break;
        case ELAND:
        case ELOR:
            if (!Operands_IsLogicalExpression(node->data.diadic.left)) {
                if (!fn_0049f630(node->data.diadic.left)) {
                    node->data.diadic.left = create_diadic_node_with_constant(node->data.diadic.left);
                }
            }
            if (!Operands_IsLogicalExpression(node->data.diadic.right)) {
                if (!fn_0049f630(node->data.diadic.right)) {
                    node->data.diadic.right = create_diadic_node_with_constant(node->data.diadic.right);
                }
            }
            TOC_0049d710(node->data.diadic.left, NULL, 0);
            TOC_0049d710(node->data.diadic.right, NULL, 0);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            break;
        case EDIVASS:
            if (node->rtype->type != TYPEFLOAT) {
                if (node->data.diadic.right->rtype->type == TYPEFLOAT)
                    data_00588500 = 1;
            }
            rewrite_compound_assignment(node, EDIV);
            goto assignment;
        case EMULASS:
            if (node->rtype->type != TYPEFLOAT) {
                if (node->data.diadic.right->rtype->type == TYPEFLOAT)
                    data_00588500 = 1;
            }
            rewrite_compound_assignment(node, EMUL);
            goto assignment;
        case EADDASS:
            if (node->rtype->type != TYPEFLOAT) {
                if (node->data.diadic.right->rtype->type == TYPEFLOAT)
                    data_00588500 = 1;
            }
            rewrite_compound_assignment(node, EADD);
            goto assignment;
        case ESUBASS:
            if (node->rtype->type != TYPEFLOAT) {
                if (node->data.diadic.right->rtype->type == TYPEFLOAT)
                    data_00588500 = 1;
            }
            rewrite_compound_assignment(node, ESUB);
            goto assignment;
        case EMODASS:
            if (node->rtype->type != TYPEFLOAT) {
                if (node->data.diadic.right->rtype->type == TYPEFLOAT)
                    data_00588500 = 1;
            }
            rewrite_compound_assignment(node, EMODULO);
            goto assignment;
        case ESHLASS:
            rewrite_compound_assignment(node, ESHL);
            goto assignment;
        case ESHRASS:
            rewrite_compound_assignment(node, ESHR);
            goto assignment;
        case EANDASS:
            rewrite_compound_assignment(node, EAND);
            goto assignment;
        case EXORASS:
            rewrite_compound_assignment(node, EXOR);
            goto assignment;
        case EORASS:
            rewrite_compound_assignment(node, EOR);
            goto assignment;
        case EASS: {
            if ((operand = node->data.diadic.left)->type == EINDIRECT)
                node->data.diadic.left = operand->data.diadic.left;
        }
        assignment:
            TOC_0049d710(node->data.diadic.left, NULL, 0);
            TOC_0049d710(node->data.diadic.right, NULL, 0);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            break;
        case EEQU:
        case ENOTEQU:
            if (node->data.diadic.right->type == EINTCONST && node->data.diadic.right->data.intval.lo == 0 &&
                node->data.diadic.right->data.intval.hi == 0) {
                ENode *operand = node->data.diadic.left;
                while (operand->type == EFORCELOAD || operand->type == ETYPCON) {
                    if (!(operand->rtype->type == TYPEINT || operand->rtype->type == TYPEENUM ||
                          operand->rtype->type == TYPEPOINTER ||
                          (operand->rtype->type == TYPEMEMBERPOINTER && operand->rtype->size == 4)))
                        break;
                    operand = operand->data.diadic.left;
                }
                if (operand->type == ELOGNOT) {
                    if (operand->data.diadic.left->rtype->type == TYPEINT ||
                        operand->data.diadic.left->rtype->type == TYPEENUM ||
                        operand->data.diadic.left->rtype->type == TYPEPOINTER ||
                        (operand->data.diadic.left->rtype->type == TYPEMEMBERPOINTER &&
                         operand->data.diadic.left->rtype->size == 4)) {
                        if (kind == EEQU)
                            node->type = ENOTEQU;
                        else
                            node->type = EEQU;
                        node->data.diadic.left = operand->data.diadic.left;
                        TOC_0049d710(node, NULL, 0);
                        break;
                    }
                }
                if (operand->type == EEQU) {
                    if (kind == EEQU)
                        operand->type = ENOTEQU;
                    *node = *operand;
                    TOC_0049d710(node, NULL, 0);
                    break;
                }
                if (operand->type == ENOTEQU) {
                    if (kind == EEQU)
                        operand->type = EEQU;
                    *node = *operand;
                    TOC_0049d710(node, NULL, 0);
                    break;
                }
            }
            /* fallthrough */
        case EDIV:
            if (kind == EDIV && node->data.diadic.right->type == EFLOATCONST) {
                ENode *right = node->data.diadic.right;
                if (CMach_FloatIsPowerOf2(right->data.floatval)) {
                    node->type = EMUL;
                    node->data.diadic.right->data.floatval =
                        CMach_FloatReciprocal(node->data.diadic.right->data.floatval);
                }
            }
            /* fallthrough */
        case EMODULO:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
            TOC_0049d710(node->data.diadic.left, NULL, 0);
            TOC_0049d710(node->data.diadic.right, NULL, 0);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8 &&
                (node->type == EDIV || node->type == EMODULO || (UInt8)(node->type - ESHL) <= 1)) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            if (copts.operandsDebug && node->rtype->type == TYPEFLOAT && (node->type == EDIV || node->type == EMUL)) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            if ((copts.operandsDebug && node->data.diadic.right->rtype->type == TYPEFLOAT) ||
                (copts.operandsDebug && node->data.diadic.left->rtype->type == TYPEFLOAT)) {
                if (node->type == ELESS || (UInt8)(node->type - EGREATER) <= 4) {
                    node->hascall = 1;
                    data_00588521 = 0;
                }
            }
            break;
        case ESUB:
            if (node->data.diadic.right->type == EINTCONST) {
                node->type = EADD;
                node->data.diadic.right->data.intval = CInt64_Inv(node->data.diadic.right->data.intval);
            }
            /* fallthrough */
        case EMUL:
        case EADD:
        case EAND:
        case EXOR:
        case EOR:
            TOC_0049d710(node->data.diadic.left, targetType, 0);
            TOC_0049d710(node->data.diadic.right, targetType, 0);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            if (targetType) {
                expr = node->data.diadic.left;
                if (expr->type == ETYPCON && (expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) &&
                    (expr->data.diadic.left->rtype->type == TYPEINT ||
                     expr->data.diadic.left->rtype->type == TYPEENUM) &&
                    expr->data.diadic.left->rtype->size >= targetType->size &&
                    !((expr->data.diadic.left->rtype->type == TYPEINT ||
                       expr->data.diadic.left->rtype->type == TYPEENUM) &&
                      expr->data.diadic.left->rtype->size == 8))
                    node->data.diadic.left = expr->data.diadic.left;
                expr = node->data.diadic.right;
                if (expr->type == ETYPCON && (expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) &&
                    (expr->data.diadic.left->rtype->type == TYPEINT ||
                     expr->data.diadic.left->rtype->type == TYPEENUM) &&
                    expr->data.diadic.left->rtype->size >= targetType->size &&
                    !((expr->data.diadic.left->rtype->type == TYPEINT ||
                       expr->data.diadic.left->rtype->type == TYPEENUM) &&
                      expr->data.diadic.left->rtype->size == 8))
                    node->data.diadic.right = expr->data.diadic.left;
                node->rtype = targetType;
            }
            if (copts.operandsDebug && node->rtype->type == TYPEFLOAT &&
                (node->type == EMUL || (UInt8)(node->type - EADD) <= 1)) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            break;
        case ETYPCON: {
            left = node->data.diadic.left;
            if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size < 4 &&
                (left->rtype->type == TYPEINT || left->rtype->type == TYPEENUM) &&
                !((left->rtype->type == TYPEINT || left->rtype->type == TYPEENUM) && left->rtype->size == 8)) {
                TOC_0049d710(left, node->rtype, 0);
            } else {
                TOC_0049d710(left, NULL, node->rtype->type == TYPEVOID);
            }
            node->hascall = left->hascall;
            if ((left->rtype->type == TYPEINT || left->rtype->type == TYPEENUM) && node->rtype->type == TYPEFLOAT)
                data_00588500 = 1;
            if (((left->rtype->type == TYPEINT || left->rtype->type == TYPEENUM) && left->rtype->size == 8 &&
                 node->rtype->type == TYPEFLOAT) ||
                ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8 &&
                 left->rtype->type == TYPEFLOAT)) {
                data_00588500 = 1;
                node->hascall = 1;
                data_00588521 = 0;
            }
            if (left->rtype->type == TYPEFLOAT) {
                if (Type_IsUnsigned(node->rtype) && node->rtype->size == 4) {
                    node->hascall = 1;
                    data_00588521 = 0;
                } else {
                    data_00588500 = 1;
                }
            }
            if ((copts.operandsDebug && left->rtype->type == TYPEFLOAT) ||
                (copts.operandsDebug && node->rtype->type == TYPEFLOAT)) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            if (node->rtype->type == TYPESTRUCT && (SInt32)((TypeStruct *)node->rtype)->stype >= 4 &&
                (SInt32)((TypeStruct *)node->rtype)->stype <= 14) {
                if (!(left->rtype->type == TYPESTRUCT && (SInt32)((TypeStruct *)left->rtype)->stype >= 4 &&
                      (SInt32)((TypeStruct *)left->rtype)->stype <= 14))
                    PPCError_ReportError(0x72);
            }
            break;
        }
        case EPOSTINC:
            if (!node->ignored) {
                if (node->data.diadic.left->rtype->type == TYPEFLOAT) {
                    if (Type_IsUnsigned(node->rtype)) {
                        data_00588500 = 1;
                    }
                }
                TOC_0049d710(node->data.diadic.left, NULL, 0);
                node->hascall = node->data.diadic.left->hascall;
                if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
                    node->hascall = 1;
                    data_00588521 = 0;
                }
                break;
            }
            node->type = EPREINC;
            /* fallthrough */
        case EPREINC:
            if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            expandpreincdec(node);
            rewrite_compound_assignment(node, EADD);
            goto assignment;
        case EPOSTDEC:
            if (!node->ignored) {
                if (node->data.diadic.left->rtype->type == TYPEFLOAT) {
                    if (Type_IsUnsigned(node->rtype)) {
                        data_00588500 = 1;
                    }
                }
                TOC_0049d710(node->data.diadic.left, NULL, 0);
                node->hascall = node->data.diadic.left->hascall;
                if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
                    node->hascall = 1;
                    data_00588521 = 0;
                }
                break;
            }
            node->type = EPREDEC;
            /* fallthrough */
        case EPREDEC:
            if (copts.operandsDebug && node->rtype->type == TYPEFLOAT) {
                node->hascall = 1;
                data_00588521 = 0;
            }
            expandpreincdec(node);
            rewrite_compound_assignment(node, ESUB);
            goto assignment;
        case ELOGNOT:
            if (!Operands_IsLogicalExpression(node->data.diadic.left)) {
                if (!fn_0049f630(node->data.diadic.left)) {
                    node->data.diadic.left = create_diadic_node_with_constant(node->data.diadic.left);
                }
            }
            /* fallthrough */
        case EMONMIN:
        case EBINNOT:
            left = node->data.diadic.left;
            TOC_0049d710(left, targetType, 0);
            node->hascall = left->hascall;
            if (targetType) {
                expr = node->data.diadic.left;
                if (expr->type == ETYPCON && (expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) &&
                    (expr->data.diadic.left->rtype->type == TYPEINT ||
                     expr->data.diadic.left->rtype->type == TYPEENUM) &&
                    expr->data.diadic.left->rtype->size >= targetType->size) {
                    node->data.diadic.left = expr->data.diadic.left;
                    node->rtype = targetType;
                }
            }
            break;
        case EINDIRECT:
        case EFORCELOAD:
        case EBITFIELD:
            operand = node->data.diadic.left;
            TOC_0049d710(operand, NULL, 0);
            node->hascall = operand->hascall;
            break;
        case EINSTRUCTION:
            operand = node->data.diadic.left;
            TOC_0049d710(operand, NULL, 0);
            node->hascall = operand->hascall;
            break;
        case EDEFINE:
            operand = node->data.diadic.left;
            node->hascall = operand->hascall;
            break;
        case EMFPOINTER:
            TOC_0049d710(node->data.diadic.left, NULL, 0);
            TOC_0049d710(node->data.diadic.right, NULL, 0);
            node->hascall = node->data.diadic.left->hascall | node->data.diadic.right->hascall;
            break;
        case ENULLCHECK:
            node->hascall = 0;
            break;
        case ELOCOBJ: {
            Object *object = get_or_create_label_object(node->data.labelAddress);
            ENode *ref = CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(ref, sizeof(ENode));
            ref->type = EOBJREF;
            ref->cost = 0;
            ref->data.objref = object;
            ref->rtype = CDecl_NewPointerType(object->type);
            node->type = EINDIRECT;
            node->cost = 1;
            node->data.diadic.left = ref;
            node->hascall = 0;
            break;
        }
        default:
            break;
    }
}

static inline long third_word(long p)
{
    long x = *(int *)p;
    return x;
}

#pragma opt_strength_reduction off

unsigned char is_small_splat_or_table_vector(long value, Type *type)
{
    short *halves;
    char byte;
    char bytes_match;
    int i;
    char *bytes;
    short half;
    char halves_match;
    int *words;
    MWVector128 *pattern;
    int k;
    char words_match;
    MWVector128 *alternate;
    int word1;
    int word3;
    int j;
    long word0;
    if (type->type == TYPESTRUCT && (int)((TypeStruct *)type)[0].stype >= 4 &&
        (int)((TypeStruct *)type)[0].stype <= 14) {
        bytes = (char *)value;
        byte = bytes[0];
        bytes_match = 1;
        for (i = 1; bytes_match && i < 16; i++)
            bytes_match = byte == bytes[i];
        if (bytes_match && byte < 16 && byte > -17)
            return 1;
        halves = (short *)value;
        half = halves[0];
        halves_match = 1;
        for (i = 1; halves_match && i < 8; i++)
            halves_match = half == halves[i];
        if (halves_match && half < 16 && half > -17)
            return 1;
        words = (int *)value;
        word0 = third_word(value);
        words_match = 1;
        for (j = 1; words_match && j < 4; j++)
            words_match = word0 == words[j];
        if (words_match && word0 < 16 && word0 > -17)
            return 1;
        k = 0, word1 = (word0 = (pattern = (MWVector128 *)value)->longElements[0], pattern->longElements[1]),
        value = pattern->longElements[2], word3 = pattern->longElements[3];
        do {
            pattern = (MWVector128 *)(vector128_patterns + k * 16);
            alternate = &alternate_vector_patterns[k];
            if (word0 == pattern->longElements[0] && word1 == pattern->longElements[1] &&
                value == pattern->longElements[2] && word3 == pattern->longElements[3])
                return 1;
            if (word0 == alternate->longElements[0] && word1 == alternate->longElements[1] &&
                value == alternate->longElements[2] && word3 == alternate->longElements[3])
                return 1;
            k++;
        } while (k < 16);
    }
    (void)pattern;
    (void)word0;
    return 0;
}

#pragma opt_strength_reduction reset

void expandpreincdec(ENode *node)
{
    ENode *constant;
    Type *rtype;
    ENode *operand;

    operand = node->data.monadic;
    rtype = node->rtype;
    constant = CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(constant, sizeof(ENode));
    if (rtype->type == TYPEFLOAT) {
        constant->type = EFLOATCONST;
        constant->cost = 0;
        constant->rtype = rtype;
        constant->data.floatval = float_one;
    } else if (rtype->type == TYPEPOINTER) {
        constant->type = EINTCONST;
        constant->cost = 0;
        constant->rtype = (Type *)&stunsignedlong;
        constant->data.intval.hi = 0;
        constant->data.intval.lo = TPTR_TARGET(rtype)->size;
    } else {
        constant->type = EINTCONST;
        constant->cost = 0;
        constant->rtype = rtype;
        constant->data.intval.hi = 0;
        constant->data.intval.lo = 1;
    }
    node->type = (node->type == EPREDEC) ? ESUBASS : EADDASS;
    node->data.diadic.left = operand;
    node->data.diadic.right = constant;
}

void rewrite_compound_assignment(ENode *expr, unsigned char opcode)
{
    ENode *left;
    ENode *right;
    Type *leftType;
    Type *rightType;
    ENode *operation;
    ENode *reference;

    left = expr->data.diadic.left->data.diadic.left;
    right = expr->data.diadic.right;
    leftType = expr->data.diadic.left->rtype;
    rightType = expr->data.diadic.right->rtype;

    operation = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(operation, sizeof(ENode));
    operation->type = opcode;
    operation->rtype = leftType;
    operation->data.diadic.left = expr->data.diadic.left;
    operation->data.diadic.right = right;

    expr->type = EASS;
    expr->data.diadic.left = left;
    expr->data.diadic.right = operation;

    if (left->type != EOBJREF) {
        ENode *temporary;
        temporary = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(temporary, sizeof(ENode));
        temporary->type = EINSTRUCTION;
        temporary->rtype = leftType;
        reference = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(reference, sizeof(ENode));
        reference->type = EDEFINE;
        reference->rtype = leftType;
        reference->data.diadic.left = temporary;
        if (left->type != EBITFIELD) {
            temporary->data.diadic.left = expr->data.diadic.left;
            expr->data.diadic.left = temporary;
            operation->data.diadic.left->data.diadic.left = reference;
        } else {
            ENode *copy;
            temporary->data.diadic.left = left->data.diadic.left;
            left->data.diadic.left = temporary;
            copy = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            *copy = *left;
            copy->data.diadic.left = reference;
            operation->data.diadic.left->data.diadic.left = copy;
        }
    }

    if (right->type == EINTCONST) {
        if (leftType->type == TYPEINT || leftType->type == TYPEENUM || leftType->type == TYPEPOINTER ||
            (leftType->type == TYPEMEMBERPOINTER && leftType->size == 4)) {
            right->rtype = leftType;
            rightType = leftType;
        }
    }

    switch (opcode) {
        case EADD:
        case ESUB:
            if (leftType->type == TYPEPOINTER)
                break;
            if (right->type == EINTCONST &&
                (leftType->type == TYPEINT || leftType->type == TYPEENUM || leftType->type == TYPEPOINTER ||
                 (leftType->type == TYPEMEMBERPOINTER && leftType->size == 4)))
                break;
            /* fallthrough */
        case EAND:
        case EXOR:
        case EOR:
            if (leftType == rightType)
                break;
            /* fallthrough */
        case EMUL:
        case EDIV:
        case EMODULO: {
            ENode *conversion;
            ENode *operandConversion;
            Type *commonType;
            commonType = select_common_arithmetic_type(leftType, rightType);
            if (leftType != commonType) {
                conversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(conversion, sizeof(ENode));
                conversion->type = ETYPCON;
                conversion->rtype = leftType;
                conversion->data.diadic.left = expr->data.diadic.right;
                expr->data.diadic.right = conversion;
                operandConversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(operandConversion, sizeof(ENode));
                operandConversion->type = ETYPCON;
                operandConversion->rtype = commonType;
                operandConversion->data.diadic.left = operation->data.diadic.left;
                operation->data.diadic.left = operandConversion;
            }
            if (rightType != commonType) {
                conversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(conversion, sizeof(ENode));
                conversion->type = ETYPCON;
                conversion->rtype = commonType;
                conversion->data.diadic.left = operation->data.diadic.right;
                operation->data.diadic.right = conversion;
            }
            operation->rtype = commonType;
            (void)conversion;
        } break;
        case ESHL:
        case ESHR: {
            Type *promotedLeft;
            Type *promotedRight;
            ENode *conversion;
            promotedLeft = leftType;
            if (leftType->type == TYPEENUM)
                promotedLeft = ((TypeEnum *)leftType)->enumtype;
            if (((TypeIntegral *)promotedLeft)->integral <= stsignedint.integral)
                promotedLeft = (Type *)&stsignedint;
            promotedRight = rightType;
            if (rightType->type == TYPEENUM)
                promotedRight = ((TypeEnum *)rightType)->enumtype;
            if (((TypeIntegral *)promotedRight)->integral <= stsignedint.integral)
                promotedRight = (Type *)&stsignedint;
            if (leftType != promotedLeft) {
                conversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(conversion, sizeof(ENode));
                conversion->type = ETYPCON;
                conversion->rtype = leftType;
                conversion->data.diadic.left = expr->data.diadic.right;
                expr->data.diadic.right = conversion;
                conversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(conversion, sizeof(ENode));
                conversion->type = ETYPCON;
                conversion->rtype = promotedLeft;
                conversion->data.diadic.left = operation->data.diadic.left;
                operation->data.diadic.left = conversion;
            }
            if (rightType != promotedRight) {
                conversion = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(conversion, sizeof(ENode));
                conversion->type = ETYPCON;
                conversion->rtype = promotedRight;
                conversion->data.diadic.left = operation->data.diadic.right;
                operation->data.diadic.right = conversion;
            }
            operation->rtype = promotedLeft;
        } break;
        default:
            break;
    }
}

static Type *Type_IntPromote(Type *t)
{
    if (t->type == TYPEENUM)
        t = TYPE_ENUM(t)->enumtype;
    if (TYPE_INTEGRAL(t)->integral <= stsignedint.integral)
        t = (Type *)&stsignedint;
    return t;
}

Type *select_common_arithmetic_type(Type *leftType, Type *rightType)
{
    Type *swapType;

    if (leftType->type == TYPEFLOAT || rightType->type == TYPEFLOAT)
        return TYPE_INTEGRAL(leftType)->integral > TYPE_INTEGRAL(rightType)->integral ? leftType : rightType;

    leftType = Type_IntPromote(leftType);
    rightType = Type_IntPromote(rightType);

    if (leftType != rightType) {
        if (TYPE_INTEGRAL(leftType)->integral < TYPE_INTEGRAL(rightType)->integral) {
            swapType = leftType;
            leftType = rightType;
            rightType = swapType;
        }
        if (leftType->size == rightType->size && !Type_IsUnsigned(leftType) && Type_IsUnsigned(rightType)) {
            if (leftType == (Type *)&stsignedlong)
                leftType = (Type *)&stunsignedlong;
            else {
                if (leftType != (Type *)&stsignedlonglong)
                    CError_FATAL(1397);
                leftType = (Type *)&stunsignedlonglong;
            }
        }
    }
    return leftType;
}

void fn_0049ebb0(ENode *expr)
{
    ENode *constantNode;
    ENodeList *list;
    SInt32 stringSize;

    constantNode = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(constantNode, sizeof(ENode));
    constantNode->type = EINTCONST;
    constantNode->cost = 0;
    constantNode->flags = 0;
    constantNode->rtype = (Type *)&stunsignedlong;
    stringSize = expr->data.funccall.args->next->node->data.string.size;
    constantNode->data.intval.lo = stringSize;
    constantNode->data.intval.hi = stringSize < 0 ? 0xffffffffU : 0U;
    list = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    memclrw(list, sizeof(ENodeList));
    list->next = NULL;
    list->node = constantNode;
    expr->data.funccall.args->next->next = list;
    expr->data.funccall.funcref->data.objref = data_00587fc0;
}

ENode *create_diadic_node_with_constant(ENode *e)
{
    ENode *n;
    ENode *c;

    n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(n, sizeof(ENode));
    c = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(c, sizeof(ENode));

    if (e->rtype->type == TYPEFLOAT) {
        c->type = EFLOATCONST;
        c->cost = 0;
        c->rtype = (e->rtype->size == 4) ? (Type *)&stfloat : (Type *)&stdouble;
        c->data.intval = data_0055e598;
    } else {
        c->type = EINTCONST;
        c->cost = 0;
        if ((e->rtype->type == TYPEINT || e->rtype->type == TYPEENUM) && e->rtype->size == 8)
            c->rtype = (Type *)&stsignedlonglong;
        else
            c->rtype = (Type *)&stsignedint;
        c->data.intval.lo = 0;
        c->data.intval.hi = 0;
    }

    n->type = ENOTEQU;
    n->cost = e->cost;
    n->rtype = (Type *)&stsignedint;
    n->data.diadic.left = e;
    n->data.diadic.right = c;
    return n;
}

void add_initial_object(void *object)
{
    ObjectList *node;

    if (object == NULL)
        return;
    if (((Object *)object)->otype != OT_OBJECT)
        return;
    {
        Object *record = (Object *)object;
        if (record->datatype != DLOCAL)
            return;
    }
    node = gInitialObjectList_005882ac;
    while (node != NULL) {
        if (node->object.value == object)
            return;
        node = node->next;
    }
    node = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
    memclrw(node, sizeof(ObjectList));
    node->object.value = object;
    node->next = gInitialObjectList_005882ac;
    gInitialObjectList_005882ac = node;
}

void TOC_EnumerateObjectCodeOffsets(void *arg)
{
    struct TOCNameEntry *node;
    CLabel *cls;

    for (node = toc_name_entries; node != NULL; node = node->next) {
        cls = node->label;
        fn_00489360(node->object, cls->pclabel->target.block->code_offset, arg);
    }
}

/* Descriptor allocated for a TOC cache entry. */
/* Pointer-valued view of the TOC cache list. */

Object *get_or_create_label_object(CLabel *node)
{
    struct TOCNameEntry *entry;
    Object *object;
    struct TOCNameEntry *newEntry;
    CLabel *label;

    entry = toc_name_entries;
    label = node;
    while (entry != NULL) {
        if (entry->label == label)
            return entry->object;
        entry = entry->next;
    }

    object = galloc(sizeof(Object));
    memclrw(object, sizeof(Object));
    object->otype = OT_OBJECT;
    object->type = (Type *)&void_ptr;
    object->name = label->uniquename;
    object->dwarfLinks.toc = NULL;
    object->u.data.info = NULL;
    object->sclass = TK_STATIC;
    object->qual = Q_CONST;
    object->datatype = DDATA;
    object->flags |= OBJECT_FLAGS_2 | OBJECT_DEFINED;
    object->extraQualifiers = ObjGen_PPC_EABI_GetHeaderIndex(0x21);

    newEntry = galloc(sizeof(struct TOCNameEntry));
    memclrw(newEntry, sizeof(struct TOCNameEntry));
    newEntry->object = object;
    newEntry->label = label;
    newEntry->next = toc_name_entries;
    toc_name_entries = newEntry;
    return object;
}

void replace_vector_constant_with_objectref(ENode *node)
{
    struct MemberPointerConstant *constant;
    MWVector128 *value;
    int comparison;
    Object *object;
    int offset;
    ENode *expression;
    int lastOffset;
    MWVector128 *newValue;
    DeclInfo objectInfo;
    newValue = (MWVector128 *)galloc(sizeof(*newValue));
    CMachine_InitVectorMem(node->rtype, node->data.vector128, newValue);
    if (cprep_cu[0xe0] == 1) {
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
    }
    constant = member_pointer_constants;
    while (constant != NULL) {
        value = constant->value;
        comparison = memcmp(value, newValue, sizeof(*newValue));
        if (comparison == 0) {
            break;
        }
        constant = constant->next;
    }
    if (constant == NULL) {
        if (member_pointer_constants != NULL) {
            lastOffset = member_pointer_constants->offset;
            object = member_pointer_constants->object;
            offset = lastOffset + sizeof(*newValue);
        } else {
            memclrw(&objectInfo, sizeof(objectInfo));
            objectInfo.dtype = CDecl_NewArrayType(TYPE(&stvectorsignedlong), sizeof(*newValue));
            objectInfo.name = GetHashNameNode("@vectorBase0");
            objectInfo.qual = Q_CONST;
            objectInfo.storage = 258;
            objectInfo.requireMangledName = 1;
            if (copts.smallBSSLimit > 16) {
                objectInfo.extraQualifiers = ObjGen_PPC_EABI_GetHeaderIndex(34);
            } else {
                objectInfo.extraQualifiers = ObjGen_PPC_EABI_GetHeaderIndex(32);
            }
            object = CParser_NewObject(&objectInfo);
            offset = 0;
            object->nspace = registration_context;
        }
        constant = (struct MemberPointerConstant *)galloc(sizeof(*constant));
        constant->next = member_pointer_constants;
        member_pointer_constants = constant;
        constant->object = object;
        constant->offset = offset;
        constant->value = (MWVector128 *)galloc(sizeof(*newValue));
        memcpy(constant->value, newValue, sizeof(*newValue));
    }
    if (constant->offset != 0) {
        expression = makediadicnode(create_objectrefnode(constant->object),
                                    intconstnode((Type *)&stunsignedlong, constant->offset), 15);
    } else {
        expression = create_objectrefnode(constant->object);
    }
    node->type = EINDIRECT;
    node->cost = 1;
    node->flags |= 1;
    node->data.monadic = expression;
}

void TOC_EmitMemberPointerConstants(void)
{
    SInt32 size = 0;
    char *buffer;
    Type *type;
    struct MemberPointerConstant *node = member_pointer_constants;

    while (node != NULL) {
        node = node->next;
        size += sizeof(TOCEntry);
    }

    if (size != 0) {
        type = CDecl_NewArrayType(TYPE(&stvectorsignedlong), size);
        member_pointer_constants->object->type = type;
        buffer = (char *)galloc(size);
        for (node = member_pointer_constants; node != NULL; node = node->next) {
            memcpy(buffer + node->offset, node->value, sizeof(TOCEntry));
        }
        fn_004cea90(member_pointer_constants->object, buffer, NULL, size);
    }
}

Object *TOC_GetFloatObject(Type *type, Float *value)
{
    ObjectList *entry;
    Float *bits;
    Object *object;
    Float *stored;
    Type *object_type;
    ObjectList *new_entry;
    Float *copy;
    if (copts.debugEnabled == 0) {
        PPCError_FatalError(169);
    }
    entry = float_object_list;
    bits = (Float *)(unsigned long)value;
    while (entry != NULL) {
        object_type = entry->object.value->type;
        stored = (Float *)entry->object.value->u.data.u.string;
        if (object_type == (Type *)type && stored->data.words[0] == bits->data.words[0] &&
            stored->data.words[1] == bits->data.words[1]) {
            return entry->object.value;
        }
        entry = entry->next;
    }
    object = (Object *)galloc(54);
    memclrw(object, 54);
    object->otype = OT_OBJECT;
    object->type = (Type *)type;
    object->name = (HashNameNode *)CParser_GetUniqueName();
    object->dwarfLinks.toc = NULL;
    object->u.data.info = NULL;
    object->u.data.linkname = object->name;
    object->sclass = TK_STATIC;
    object->qual = (Q_CONST | Q_INLINE_DATA);
    object->datatype = DDATA;
    ObjGen_PPC_EABI_SetObjectSection(object, type->size, 1);
    BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    object->flags |= OBJECT_FLAGS_2;
    object->u.data.u.string = (char *)galloc(8);
    copy = (Float *)object->u.data.u.string;
    *copy = *value;
    new_entry = (ObjectList *)galloc(8);
    memclrw(new_entry, 8);
    new_entry->object.value = object;
    new_entry->next = float_object_list;
    float_object_list = new_entry;
    if (copts.operandsDebug == 0) {
        ObjGen_PPC_EABI_EmitFloatObject(object);
    }
    return object;
}

void rewrite_indirect_toc_references(void)
{
    ENode *replacement;
    TOCReferenceEntry *entry;
    ENode *record;
    Object *key;

    for (entry = toc_references; entry != NULL; entry = entry->next) {
        if ((record = entry->expression) != NULL) {
            key = entry->lookupObject;
            if (entry->makeIndirect != 0) {
                replacement = CompilerTools_AllocatePool(sizeof(*replacement));
                memclrw(replacement, sizeof(*replacement));
                replacement->type = EOBJREF;
                replacement->cost = 0;
                replacement->data.objref = key;
                replacement->rtype = record->rtype;
                record->type = EINDIRECT;
                record->cost = 1;
                record->data.monadic = replacement;
                if (key->extraQualifiers == 0) {
                    CError_FATAL(648);
                }
                BE_symbol_GetOrCreateFunctionObjectSymbol(key);
            } else {
                if (key->extraQualifiers == 0) {
                    CError_FATAL(648);
                }
                BE_symbol_GetOrCreateFunctionObjectSymbol(key);
            }
        }
    }
}

Object *fn_0049f230(Object *object, SInt32 a, SInt32 b)
{
    if (object->extraQualifiers == 0)
        CError_FATAL(648);
    BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    return NULL;
}

UInt8 TOC_HasObjectReferenceWithoutExpression(Object *key)
{
    TOCReferenceEntry *entry;

    entry = toc_references;
    if (entry != NULL) {
        do {
            if (key == entry->object) {
                if (entry->expression != NULL) {
                    return '\0';
                }
                return '\x01';
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    return '\0';
}

void add_toc_reference(Object *id, Object *a, ENode *b, char c)
{
    TOCReferenceEntry *n;
    TOCReferenceEntry *first = NULL;
    TOCReferenceEntry *dup = NULL;

    n = toc_references;
    while (n != NULL) {
        if (id == n->object) {
            if (first != NULL)
                dup = n;
            else
                first = n;
        }
        n = n->next;
    }

    if (dup != NULL) {
        if (dup->expression != NULL) {
            Object *object = dup->object;
            make_objectref_offset(object, dup->lookupObject, dup->expression, dup->makeIndirect);
            dup->expression = NULL;
            object = first->object;
            make_objectref_offset(object, first->lookupObject, first->expression, first->makeIndirect);
            first->expression = NULL;
        }
        {
            Object *object = id;
            make_objectref_offset(object, a, b, c);
        }
        return;
    }
    {
        TOCReferenceEntry *nn = (TOCReferenceEntry *)galloc(18);
        memclrw(nn, 18);
        nn->object = id;
        nn->lookupObject = a;
        nn->expression = b;
        nn->makeIndirect = c;
        nn->next = toc_references;
        toc_references = nn;
    }
}

void make_objectref_offset(Object *object, Object *lookupObject, ENode *expression, Boolean makeIndirect)
{
    BE_SymNode *record;
    ENode *sum;
    ENode *objectRef;
    ENode *offset;
    SInt32 displacement;

    fn_0049f4b0(object);
    record = BE_symbol_GetOrCreateFunctionObjectSymbol(lookupObject);
    record->flags |= 0x80;
    record->sectionData.section->symbolLink->symbol->flags &= ~0x40;

    if (makeIndirect) {
        sum = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(sum, sizeof(ENode));
        sum->type = EADD;
        sum->cost = 1;
        sum->rtype = CDecl_NewPointerType(expression->rtype);
        expression->type = EINDIRECT;
        expression->cost = 1;
        expression->data.monadic = sum;

        objectRef = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(objectRef, sizeof(ENode));
        objectRef->type = EOBJREF;
        objectRef->cost = 0;
        objectRef->data.objref = object;
        objectRef->rtype = expression->rtype;

        offset = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(offset, sizeof(ENode));
        offset->type = EINTCONST;
        offset->cost = 0;
        displacement = BE_symbol_GetOffset(record);
        offset->data.intval.lo = displacement;
        offset->data.intval.hi = displacement < 0 ? -1 : 0;
        offset->rtype = (Type *)&stsignedint;

        sum->data.diadic.left = objectRef;
        sum->data.diadic.right = offset;
    } else {
        objectRef = CompilerTools_AllocatePool(sizeof(ENode));
        objectRef->type = EOBJREF;
        objectRef->cost = 0;
        objectRef->data.objref = object;
        objectRef->rtype = expression->rtype;

        offset = CompilerTools_AllocatePool(sizeof(ENode));
        offset->type = EINTCONST;
        offset->cost = 0;
        displacement = BE_symbol_GetOffset(record);
        offset->data.intval.lo = displacement;
        offset->data.intval.hi = displacement < 0 ? -1 : 0;
        offset->rtype = (Type *)&stsignedint;

        expression->type = EADD;
        expression->cost = 1;
        expression->data.diadic.left = objectRef;
        expression->data.diadic.right = offset;
    }
}

void fn_0049f4b0(Object *object)
{
    VarInfo *info;
    ObjectList *entry;
    unsigned int weight;
    ObjectList *new_entry;
    Object *current = object;

    info = Registers_GetInfo(current);
    info->used = 1;
    if (copts.uniformSpillBlockWeight != 0)
        weight = 1;
    else
        weight = current_statement_number;
    info->usage += weight;

    if ((entry = gTrailingObjectList_005876a0) != NULL) {
        do {
            if (current == entry->object.value)
                return;
            entry = entry->next;
        } while (entry != NULL);
    }

    if ((int)info->usage < 3)
        info->usage = 3;
    new_entry = (ObjectList *)galloc(sizeof(ObjectList));
    memclrw(new_entry, sizeof(ObjectList));
    new_entry->object.value = current;
    new_entry->next = gTrailingObjectList_005876a0;
    gTrailingObjectList_005876a0 = new_entry;
}

void Operands_ClearTrailingObjectInfo(void)
{
    ObjectList *list = gTrailingObjectList_005876a0;
    while (list != NULL) {
        list->object.value->u.data.info = NULL;
        list = list->next;
    }
    toc_references = NULL;
}

void fn_0049f560(void)
{
    gTrailingObjectList_005876a0 = data_00587660 = (float_object_list = NULL);
    gInitialObjectList_005882ac = NULL;
    data_00588200 = 0;
    member_pointer_constants = NULL;
    data_00588508 = 1;
    memclrw(data_005883f0, 54);
    blank_name = GetHashNameNode("TOC");
    toc_references = NULL;
}
