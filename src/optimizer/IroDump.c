#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/IroDump.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
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
#include <stdio.h>

Object *data_005875b8;
int INT_005882b8;

static struct _FILE *iro_dump_output;

/* The name of each ENode type. */
static char *enode_type_names[] = {
    "EPOSTINC",
    "EPOSTDEC",
    "EPREINC",
    "EPREDEC",
    "EINDIRECT",
    "EMONMIN",
    "EBINNOT",
    "ELOGNOT",
    "EFORCELOAD",
    "EMUL",
    "EMULV",
    "EDIV",
    "EMODULO",
    "EADDV",
    "ESUBV",
    "EADD",
    "ESUB",
    "ESHL",
    "ESHR",
    "ELESS",
    "EGREATER",
    "ELESSEQU",
    "EGREATEREQU",
    "EEQU",
    "ENOTEQU",
    "EAND",
    "EXOR",
    "EOR",
    "ELAND",
    "ELOR",
    "EASS",
    "EMULASS",
    "EDIVASS",
    "EMODASS",
    "EADDASS",
    "ESUBASS",
    "ESHLASS",
    "ESHRASS",
    "EANDASS",
    "EXORASS",
    "EORASS",
    "ECOMMA",
    "EPMODULO",
    "EROTL",
    "EROTR",
    "EBCLR",
    "EBTST",
    "EBSET",
    "ETYPCON",
    "EBITFIELD",
    "EINTCONST",
    "EFLOATCONST",
    "ESTRINGCONST",
    "ECOND",
    "EFUNCCALL",
    "EFUNCCALLP",
    "EOBJREF",
    "EQUALNAME",
    "EMFPOINTER",
    "ENULLCHECK",
    "EPRECOMP",
    "ETEMP",
    "EARGOBJ",
    "ELOCOBJ",
    "ETEMPX",
    "ELABEL",
    "ESETCONST",
    "ENEWEXCEPTION",
    "ENEWEXCEPTIONARRAY",
    "EOBJLIST",
    "EMEMBER",
    "EINSTRUCTION",
    "EDEFINE",
    "EREUSE",
    "EASSBLK",
    "EVECTOR128CONST",
};

static inline void dump_separator(void)
{
    fprintf(iro_dump_output, "\n");
}

static inline void *dump_output_handle(void)
{
    return iro_dump_output;
}

static inline void printBitIndex(const char *format, int bitIndex)
{
    fprintf(iro_dump_output, (const char *)format, bitIndex);
}

static inline void printBitSetText(const void *text)
{
    fprintf(iro_dump_output, (const char *)text);
}

void dump_linear_node(IROLinear *node)
{
    int index;
    ENode *operand;
    Type *type;
    VarRecord *resolvedType;
    char buffer[64];
    ENode *integer;
    CLabel *label;

    if (INT_005882b8 == 0)
        return;

    fprintf(iro_dump_output, "%4d: ", node->index);
    switch (node->type) {
        case IROLinearNop:
            fprintf(iro_dump_output, "Nop");
            break;
        case IROLinearOperand:
            fprintf(iro_dump_output, "Operand ");
            operand = node->u.node;
            if (INT_005882b8 != 0) {
                switch (operand->type) {
                    case '8':
                        fprintf(iro_dump_output, "%s", operand->data.objref->name->name);
                        break;
                    case '2':
                        integer = operand;
                        CInt64_PrintDec(buffer, integer->data.intval);
                        fprintf(iro_dump_output, "%s", buffer);
                        break;
                    case '3':
                        fprintf(iro_dump_output, "%g", operand->data.floatval.data.value);
                        break;
                    case 'J':
                        fprintf(iro_dump_output, "%.8lX%.8lX%.8lX%.8lX", operand->data.vector128val.ul[0],
                                operand->data.vector128val.ul[1], operand->data.vector128val.ul[2],
                                operand->data.vector128val.ul[3]);
                        break;
                }
            }
            break;
        case IROLinearOp1Arg:
            fprintf(iro_dump_output, "%s %d", enode_type_names[node->nodetype],
                    ((IROLinear *)node->u.diadic.left)->index);
            break;
        case IROLinearOp2Arg:
            fprintf(iro_dump_output, "%s %d %d", enode_type_names[node->nodetype],
                    ((IROLinear *)node->u.diadic.left)->index, node->u.diadic.right->index);
            break;
        case IROLinearGoto:
            fprintf(iro_dump_output, "Goto %s", ((CLabel *)node->u.label)->name->name);
            break;
        case IROLinearIf:
            fprintf(iro_dump_output, "If %d %s", node->u.diadic.right->index, ((CLabel *)node->u.label)->name->name);
            break;
        case IROLinearIfNot:
            fprintf(iro_dump_output, "IfNot %d %s", node->u.diadic.right->index, ((CLabel *)node->u.label)->name->name);
            break;
        case IROLinearReturn:
            fprintf(iro_dump_output, "Return ");
            if (node->u.diadic.left != NULL) {
                fprintf(iro_dump_output, "%d", ((IROLinear *)node->u.diadic.left)->index);
            }
            break;
        case IROLinearLabel:
            fprintf(iro_dump_output, "Label %s", ((CLabel *)node->u.label)->name->name);
            break;
        case IROLinearSwitch:
            fprintf(iro_dump_output, "Switch %d", node->u.diadic.right->index);
            break;
        case IROLinearFunccall:
            fprintf(iro_dump_output, "Funccall %d(", node->u.funccall.callee->index);
            for (index = 0; index < node->u.funccall.argCount; ++index) {
                fprintf(iro_dump_output, "%d", node->u.funccall.args[index]->index);
                if (index < node->u.funccall.argCount - 1) {
                    fprintf(iro_dump_output, ",");
                }
            }
            fprintf(iro_dump_output, ")");
            break;
        case IROLinearBeginCatch:
            fprintf(iro_dump_output, "BeginCatch %d", ((IROLinear *)node->u.diadic.left)->index);
            break;
        case IROLinearEndCatch:
            fprintf(iro_dump_output, "EndCatch %d", ((IROLinear *)node->u.diadic.left)->index);
            break;
        case IROLinearEndCatchDtor:
            fprintf(iro_dump_output, "EndCatchDtor %d", ((IROLinear *)node->u.diadic.left)->index);
            break;
        case IROLinearEnd:
            fprintf(iro_dump_output, "End");
            break;
    }
    if ((node->flags & IROLF_Assigned) != 0) {
        fprintf(iro_dump_output, " <assigned>");
    }
    if ((node->flags & IROLF_Used) != 0) {
        fprintf(iro_dump_output, " <used>");
    }
    if ((node->flags & IROLF_Ind) != 0) {
        fprintf(iro_dump_output, " <ind>");
    }
    if ((node->flags & IROLF_Subs) != 0) {
        fprintf(iro_dump_output, " <subs>");
    }
    if ((node->flags & IROLF_LoopInvariant) != 0) {
        fprintf(iro_dump_output, " <loop invariant>");
    }
    if ((node->flags & IROLF_BeginLoop) != 0) {
        fprintf(iro_dump_output, " <begin loop>");
    }
    if ((node->flags & IROLF_EndLoop) != 0) {
        fprintf(iro_dump_output, " <end loop>");
    }
    if ((node->flags & IROLF_Ris) != 0) {
        fprintf(iro_dump_output, " <ris>");
    }
    if ((node->flags & IROLF_Immind) != 0) {
        fprintf(iro_dump_output, " <immind>");
    }
    if ((node->flags & IROLF_Reffed) != 0) {
        fprintf(iro_dump_output, " <reffed>");
    }
    if ((node->flags & IROLF_VecOp) != 0) {
        fprintf(iro_dump_output, " <vec op>");
    }
    if ((node->flags & IROLF_VecOpBase) != 0) {
        fprintf(iro_dump_output, " <vec op_base>");
    }
    if ((node->flags & IROLF_CounterLoop) != 0) {
        fprintf(iro_dump_output, " <counter loop>");
    }
    if ((type = (Type *)node->rtype) != NULL && CParser_IsVolatile(type, node->nodeflags & 3)) {
        fprintf(iro_dump_output, " <volatile>");
    }
    if (node->type == IROLinearOperand) {
        ENode *constant;
        if ((constant = node->u.node)->type == '8') {
            resolvedType = fn_0044ba70(constant->data.objref, 0, 1);
            if (resolvedType != NULL && (char)is_volatile_object(resolvedType->object)) {
                fprintf(iro_dump_output, " <volatile obj>");
            }
        }
    }
    fprintf(iro_dump_output, "\n");
}

void IroDump_PrintBitSet(char *prefix, BitVector *bitset)
{
    Boolean inRange = 0;
    Boolean firstRange = 1;
    int bitIndex;
    int rangeStart;

    if (INT_005882b8 == 0)
        return;

    printBitSetText(prefix);
    for (bitIndex = 0; bitIndex < bitset->size << 5; ++bitIndex) {
        if (((bitIndex >> 5) < bitset->size) && ((1 << bitIndex & bitset->bits[bitIndex >> 5]) != 0)) {
            if (!inRange) {
                if (!firstRange) {
                    struct _FILE *output = iro_dump_output;
                    fputc(',', output);
                }
                firstRange = 0;
                printBitIndex("%d", bitIndex);
                inRange = 1;
                rangeStart = bitIndex;
            }
        } else if (inRange) {
            inRange = 0;
            if (bitIndex != rangeStart + 1)
                printBitIndex("-%d", bitIndex - 1);
        }
    }
    if (inRange && bitIndex != rangeStart + 1)
        printBitIndex("-%d", bitIndex - 1);
    fprintf(iro_dump_output, "\n");
}

void IroDump_DumpFunction(char *value, int enabled)
{
    char *name;
    if ((unsigned char)enabled != 0U) {
        if (data_005875b8 != 0U) {
            name = data_005875b8->name->name;
        } else {
            name = "Init-code";
        }
        IroDump_Print("Dumping function %s after %s \n", name, value);
        IroDump_Print("--------------------------------------------------------------------------------\n");
        dump_flowgraph();
    }
}

void dump_flowgraph(void)
{
    SInt32 i;
    SInt32 count;
    IROLinear *p;
    UInt16 *list;
    SInt32 j;
    SInt32 npred;
    UInt16 *pred;
    IRONode *node;

    if (INT_005882b8 == 0)
        return;
    if (iro_dump_output == NULL)
        return;

    fprintf(iro_dump_output, "\nFlowgraph\n");
    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        fprintf(iro_dump_output, "Flowgraph node %d  First=%d, Last=%d\n", node->index, node->first->index,
                node->last->index);
        fprintf(iro_dump_output, "Succ = ");
        count = node->numsucc;
        list = node->succ;
        if (INT_005882b8) {
            for (i = 0; i < count; i++)
                fprintf(iro_dump_output, "%d ", list[i]);
            fprintf(iro_dump_output, "\n");
        }
        fprintf(iro_dump_output, "Pred = ");
        pred = node->pred;
        npred = node->numpred;
        if (INT_005882b8) {
            for (j = 0; j < npred; j++)
                fprintf(iro_dump_output, "%d ", pred[j]);
            fprintf(iro_dump_output, "\n");
        }
        fprintf(iro_dump_output, "MustReach = %d\n", node->mustreach);
        fprintf(iro_dump_output, "LoopDepth = %d\n", node->loopdepth);
        IroDump_PrintBitSet("Dom: ", node->dom);
        p = node->first;
        if (p != NULL) {
            for (;;) {
                dump_linear_node(p);
                if (p == node->last)
                    break;
                p = p->next;
            }
        }
        fprintf(iro_dump_output, "\n\n");
    }
    fprintf(iro_dump_output, "\n");
    fflush(iro_dump_output);
}

void IroDump_DumpNode(IRONode *node)
{
    SInt32 i;
    IROLinear *p;

    if (INT_005882b8 == 0)
        return;
    fprintf(iro_dump_output, "Flowgraph node %d  First=%d, Last=%d\n", node->index, node->first->index,
            node->last->index);
    fprintf(iro_dump_output, "Succ = ");
    for (i = 0; i < node->numsucc; i++)
        fprintf(iro_dump_output, "%d ", node->succ[i]);
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "Pred = ");
    for (i = 0; i < node->numpred; i++)
        fprintf(iro_dump_output, "%d ", node->pred[i]);
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "MustReach = %d\n", node->mustreach);
    fprintf(iro_dump_output, "LoopDepth = %d\n", node->loopdepth);
    IroDump_PrintBitSet("Dom: ", node->dom);
    for (p = node->first; p != NULL; p = p->next) {
        dump_linear_node(p);
        if (p == node->last)
            break;
    }
    fprintf(iro_dump_output, "\n\n");
}

void IroDump_DumpAssignments(void)
{
    IRONode *node;
    IROLinear *linear;

    if (INT_005882b8 == 0)
        return;
    fprintf(iro_dump_output, "\nAssignments\n\n");
    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        for (linear = node->first; linear != NULL; linear = linear->next) {
            if (linear->flags & IROLF_Assigned) {
                fprintf(iro_dump_output, "%5d ", linear->index);
                dump_linear_node(linear);
            }
            if (linear == node->last)
                break;
        }
    }
}

void IroDump_DumpVariables(void)
{
    VarRecord *var;

    if (INT_005882b8 == 0)
        return;
    fprintf(iro_dump_output, "\nVariables\n");
    for (var = var_records; var != NULL; var = var->next)
        fprintf(iro_dump_output, "%5d %s %s\n", var->index, var->object->name->name,
                var->noregister ? "<addressed>" : "");
}

void IroDump_DumpDataFlow(void)
{
    IRONode *node;

    if (INT_005882b8 == 0)
        return;
    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        fprintf(iro_dump_output, "Node %d\n", node->index);
        IroDump_PrintBitSet("In:   ", node->in);
        IroDump_PrintBitSet("Gen:  ", node->gen);
        IroDump_PrintBitSet("Kill: ", node->kill);
        IroDump_PrintBitSet("Out:  ", node->out);
        IroDump_PrintBitSet("AA:   ", node->copyOut);
    }
}

void IroDump_DumpExpressions(void)
{
    IROExpr *entry;

    if (INT_005882b8 == 0)
        return;

    fprintf(dump_output_handle(), "Expressions\n\n");
    for (entry = expr_list; entry != NULL; entry = entry->next) {
        fprintf(dump_output_handle(), "%4d: %d FN:%d CE:%d NS:%d ", entry->index, entry->linear->index,
                entry->node->index, entry->mayTrap, entry->hasSideEffects);
        IroDump_PrintBitSet("Depends: ", entry->depends);
        dump_separator();
    }
    dump_separator();
}

void IroDump_OpenLog(char *name)
{
    char path[256];

    strcpy(path, name);
    strcat(path, ".log");
    iro_dump_output = fopen(path, "wt");
}

void IroDump_DumpAddress(IROLinear *address, IROLinear *baseTerm, IROLinear *varTerm, IROLinear *constTerm)
{
    if (INT_005882b8 == 0)
        return;
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "Address  :\n");
    dump_linear_node(address);
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "BaseTerms:\n");
    dump_linear_node(baseTerm);
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "VarTerms:\n");
    dump_linear_node(varTerm);
    fprintf(iro_dump_output, "\n");
    fprintf(iro_dump_output, "ConstTerms:\n");
    dump_linear_node(constTerm);
    fprintf(iro_dump_output, "\n");
}

void IroDump_Print(const char *message, ...)
{
    va_list arguments;
    int argumentSize;

    if (INT_005882b8 == 0)
        return;

    argumentSize = (va_list)(&message + 1) - (va_list)&message;
    arguments = (va_list)&message + (argumentSize + 3) / 4 * 4;
    vfprintf(iro_dump_output, message, arguments);
}

Object *IroDump_GetObjRef(IROLinear *linear)
{
    if (linear->type == IROLinearOp1Arg && linear->nodetype == EINDIRECT &&
        linear->u.monadic->type == IROLinearOperand && linear->u.monadic->u.node->type == EOBJREF)
        return linear->u.monadic->u.node->data.objref;
    return NULL;
}

int fn_0044d520(IROLinear *node)
{
    if (node->type == 1U &&
        (node->u.node->type == EINTCONST || node->u.node->type == EVECTOR128CONST || node->u.node->type == EFLOATCONST))
        return 1;
    return 0;
}

SInt32 IroDump_IsPowerOfTwo(IROLinear *node, SInt32 *bit)
{
    SInt32 value;
    SInt32 mask;
    UInt32 index;
    ENode *expr;

    *bit = -1;
    if (node->type == IROLinearOperand) {
        if ((expr = node->u.node)->type == EINTCONST) {
            if (expr->data.intval.hi != 0)
                return 0;
            value = expr->data.intval.lo;
            mask = 1;
            index = 0;
            do {
                if (mask == value) {
                    *bit = index;
                    return 1;
                }
                index++;
                mask += mask;
            } while (index < 31);
        }
    }
    return 0;
}

unsigned int IroDump_IsType1NodeType50(IROLinear *linear)
{
    if (linear->type == IROLinearOperand && linear->u.node->type == 50U)
        return 1U;
    return 0U;
}
