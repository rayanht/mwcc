#define CERROR_FILE "Switch.c"
#include "compiler/common.h"
#include "compiler/Switch.h"
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
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/TOC.h"
#include "driver/Files.h"
#include "compiler/ENode.h"
static CInt64 data_005608f8 = {0, 3};

static struct SwitchCase **data_00581150;
static struct CaseRange *case_ranges;
static SInt32 switch_case_count;
static SInt32 case_range_count;
static CInt64 data_00581160;
static CInt64 data_00581168;
static CInt64 switchtable_base;
static SInt16 data_00581178;
static SInt16 switchRegHi;
static struct Type *switch_expr_type;
static struct PCodeLabel *default_case_label;
static CInt64 switchtable_max;

static inline SInt16 NextGPR(void)
{
    return gUsedVirtualRegistersGPR++;
}

static inline void SetSignedCaseValue(CInt64 *value, SInt32 low)
{
    value->lo = low;
    value->hi = low < 0 ? -1 : 0;
}

int compare_switch_case_min(const void *a, const void *b)
{
    if (CInt64_Less((*(SwitchCase *const *)a)->min, (*(SwitchCase *const *)b)->min))
        return -1;
    if (CInt64_Greater((*(SwitchCase *const *)a)->min, (*(SwitchCase *const *)b)->min))
        return 1;
    return 0;
}

void build_case_ranges(Type *type, SwitchCase *list, CLabel *defaultCase)
{
    SwitchCase **p;
    CaseRange *range;
    SInt32 i;
    if (type->size == 8) {
        data_00581160.lo = 0;
        data_00581160.hi = 0x80000000;
        data_00581168.lo = 0xffffffff;
        data_00581168.hi = 0x7fffffff;
    } else if (type->size == 4) {
        data_00581160.lo = 0x80000000;
        data_00581160.hi = 0xffffffff;
        data_00581168.lo = 0x7fffffff;
        data_00581168.hi = 0;
    } else if (Type_IsUnsigned(type)) {
        data_00581160.hi = 0;
        data_00581160.lo = 0;
        data_00581168.hi = 0;
        data_00581168.lo = 0xffff;
    } else {
        data_00581160.lo = 0xffff8000;
        data_00581160.hi = 0xffffffff;
        data_00581168.lo = 0x7fff;
        data_00581168.hi = 0;
    }
    p = CompilerTools_AllocatePool(switch_case_count * sizeof(SwitchCase *));
    data_00581150 = p;
    if (list != NULL) {
        do {
            *p++ = (SwitchCase *)list;
            list = list->next;
        } while (list != NULL);
    }
    case_ranges = CompilerTools_AllocatePool((switch_case_count * 2 + 2) * sizeof(CaseRange));
    if (type->size < 8) {
        for (i = 0; i < switch_case_count; i++) {
            SetSignedCaseValue(&data_00581150[i]->min, data_00581150[i]->min.lo);
        }
    }
    qsort(data_00581150, switch_case_count, sizeof(SwitchCase *), compare_switch_case_min);
    range = case_ranges;
    range->base = data_00581160;
    range->width = CInt64_Sub(data_00581168, data_00581160);
    i = 0;
    range->info = defaultCase->pclabel;
    for (; i < switch_case_count; i++) {
        SwitchCase *entry = data_00581150[i];
        if (!CInt64_GreaterEqual(entry->min, data_00581160))
            continue;
        if (!CInt64_LessEqual(entry->min, data_00581168))
            continue;
        if (CInt64_Equal(range->base, data_00581160)) {
            switchtable_base = entry->min;
        }
        switchtable_max = CInt64_Sub(entry->min, switchtable_base);
        if (CInt64_Greater(entry->min, range->base)) {
            range->width = CInt64_Sub(CInt64_Sub(entry->min, range->base), cint64_one);
            range++;
            range->base = entry->min;
        } else if (CInt64_Greater(range->base, data_00581160) && entry->label->pclabel == (range - 1)->info) {
            (range - 1)->width = CInt64_Add((range - 1)->width, cint64_one);
            if (CInt64_Equal(range->width, cint64_zero)) {
                range--;
            } else {
                range->base = CInt64_Add(range->base, cint64_one);
                range->width = CInt64_Sub(range->width, cint64_one);
            }
            continue;
        }
        range->width = cint64_zero;
        range->info = entry->label->pclabel;
        if (CInt64_Less(entry->min, data_00581168)) {
            range++;
            range->base = CInt64_Add(entry->min, cint64_one);
            range->width = CInt64_Sub(data_00581168, range->base);
            range->info = defaultCase->pclabel;
        }
    }
    case_range_count = range - case_ranges;
}

void emit_case_range_binary_search(int a, int b)
{
    int mid;
    int range;
    int i;
    int reg;
    PCodeLabel *label;
    SInt32 value;
    CaseRange *p;

    range = b - a;
    CError_ASSERT(175, switch_expr_type->size <= 4);

    mid = a + (range >> 1) + 1;
    p = case_ranges + mid;

    if (CInt64_Equal(case_ranges[mid - 1].width, cint64_zero)) {
        if ((range & 1) == 0 || (CInt64_NotEqual(p[0].width, cint64_zero) && range > 1)) {
            --p;
            --mid;
        }
    }

    i = mid - 1;

    if (switch_expr_type->size < 4 && Type_IsUnsigned(switch_expr_type)) {
        PCodeUtilities_EmitInstruction(PC_CMPLI, 0, data_00581178, p->base.lo);
    } else {
        value = p->base.lo;
        if (value == (SInt16)value) {
            PCodeUtilities_EmitInstruction(PC_CMPI, 0, data_00581178, value);
        } else {
            reg = NextGPR();
            PCodeUtilities_LoadImmediate(reg, value);
            PCodeUtilities_EmitInstruction(PC_CMP, 0, data_00581178, reg);
        }
    }

    if (CInt64_Equal(p->width, cint64_zero) && mid < b) {
        PCodeUtilities_EmitConditionBranch(0, 0x17, 1, p->info);
        ++mid;
    }

    if (mid == b) {
        if (a == i) {
            if (case_ranges[a].info == case_ranges[b].info)
                PCodeUtilities_EmitBranch(case_ranges[a].info);
            else {
                PCodeUtilities_EmitConditionBranch(0, 0x16, 1, case_ranges[b].info);
                PCodeUtilities_EmitBranch(case_ranges[a].info);
            }
        } else {
            PCodeUtilities_EmitConditionBranch(0, 0x16, 1, case_ranges[b].info);
            emit_case_range_binary_search(a, i);
        }
    } else if (a == i) {
        PCodeUtilities_EmitConditionBranch(0, 0x13, 1, case_ranges[a].info);
        emit_case_range_binary_search(mid, b);
    } else {
        label = PCode_NewLabel();
        PCodeUtilities_EmitConditionBranch(0, 0x16, 1, label);
        emit_case_range_binary_search(a, i);
        PCodeUtilities_ResolveLabel(label);
        emit_case_range_binary_search(mid, b);
    }
}

void emit_case_range_binary_tree(unsigned int firstCase, int lastCase)
{
    CaseRange *pivot;
    int span = lastCase - firstCase;
    int leftLast;
    int rightFirst = ((span >> 1) + firstCase) + 1;
    SInt16 lowReg, highReg, compareHighReg, pivotHighReg;

    pivot = &case_ranges[rightFirst];
    if (CInt64_Equal(case_ranges[rightFirst - 1].width, cint64_zero)) {
        if ((span & 1) == 0 || (CInt64_NotEqual(pivot->width, cint64_zero) && span > 1)) {
            pivot--;
            rightFirst--;
        }
    }
    leftLast = rightFirst - 1;
    if (CInt64_Equal(pivot->width, cint64_zero) && rightFirst < lastCase) {
        lowReg = gUsedVirtualRegistersGPR++;
        highReg = gUsedVirtualRegistersGPR++;
        PCodeUtilities_LoadImmediate(lowReg, pivot->base.lo);
        PCodeUtilities_LoadImmediate(highReg, pivot->base.hi);
        PCodeUtilities_EmitInstruction(PC_XOR, lowReg, data_00581178, lowReg);
        PCodeUtilities_EmitInstruction(PC_XOR, highReg, switchRegHi, highReg);
        PCodeUtilities_EmitInstruction(PC_OR, highReg, lowReg, highReg);
        PCodeUtilities_EmitInstruction(PC_CMPI, 0, highReg, 0);
        PCodeUtilities_EmitConditionBranch(0, 0x17, 1, pivot->info);
        rightFirst++;
    }
    if (rightFirst == lastCase) {
        if (firstCase == leftLast) {
            if (case_ranges[firstCase].info == case_ranges[lastCase].info) {
                PCodeUtilities_EmitBranch(case_ranges[firstCase].info);
            } else {
                lowReg = gUsedVirtualRegistersGPR++;
                highReg = gUsedVirtualRegistersGPR++;
                compareHighReg = gUsedVirtualRegistersGPR++;
                pivotHighReg = gUsedVirtualRegistersGPR++;
                PCodeUtilities_LoadImmediate(lowReg, pivot->base.lo);
                PCodeUtilities_LoadImmediate(highReg, pivot->base.hi);
                if (TYPE_INTEGRAL(switch_expr_type)->integral != 12 &&
                    TYPE_INTEGRAL(switch_expr_type)->integral != 12) {
                    PCodeUtilities_EmitInstruction(PC_XORIS, compareHighReg, switchRegHi, 0x8000);
                    PCodeUtilities_EmitInstruction(PC_XORIS, pivotHighReg, highReg, 0x8000);
                } else {
                    pivotHighReg = highReg;
                    compareHighReg = switchRegHi;
                }
                PCodeUtilities_EmitInstruction(PC_SUBFC, lowReg, lowReg, data_00581178);
                PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, pivotHighReg, compareHighReg);
                PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, lowReg, lowReg);
                PCodeUtilities_EmitInstruction(PC_NEG, highReg, highReg);
                PCodeUtilities_EmitInstruction(PC_CMPI, 0, highReg, 0);
                PCodeUtilities_EmitConditionBranch(0, 0x17, 1, case_ranges[lastCase].info);
                PCodeUtilities_EmitBranch(case_ranges[firstCase].info);
            }
        } else {
            lowReg = gUsedVirtualRegistersGPR++;
            highReg = gUsedVirtualRegistersGPR++;
            compareHighReg = gUsedVirtualRegistersGPR++;
            pivotHighReg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_LoadImmediate(lowReg, pivot->base.lo);
            PCodeUtilities_LoadImmediate(highReg, pivot->base.hi);
            if (TYPE_INTEGRAL(switch_expr_type)->integral != 12 && TYPE_INTEGRAL(switch_expr_type)->integral != 12) {
                PCodeUtilities_EmitInstruction(PC_XORIS, compareHighReg, switchRegHi, 0x8000);
                PCodeUtilities_EmitInstruction(PC_XORIS, pivotHighReg, highReg, 0x8000);
            } else {
                compareHighReg = switchRegHi;
                pivotHighReg = highReg;
            }
            PCodeUtilities_EmitInstruction(PC_SUBFC, lowReg, lowReg, data_00581178);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, pivotHighReg, compareHighReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, lowReg, lowReg);
            PCodeUtilities_EmitInstruction(PC_NEG, highReg, highReg);
            PCodeUtilities_EmitInstruction(PC_CMPI, 0, highReg, 0);
            PCodeUtilities_EmitConditionBranch(0, 0x17, 1, case_ranges[lastCase].info);
            emit_case_range_binary_tree(firstCase, leftLast);
        }
    } else {
        if (firstCase == leftLast) {
            lowReg = gUsedVirtualRegistersGPR++;
            highReg = gUsedVirtualRegistersGPR++;
            compareHighReg = gUsedVirtualRegistersGPR++;
            pivotHighReg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_LoadImmediate(lowReg, pivot->base.lo);
            PCodeUtilities_LoadImmediate(highReg, pivot->base.hi);
            if (TYPE_INTEGRAL(switch_expr_type)->integral != 12 && TYPE_INTEGRAL(switch_expr_type)->integral != 12) {
                PCodeUtilities_EmitInstruction(PC_XORIS, compareHighReg, switchRegHi, 0x8000);
                PCodeUtilities_EmitInstruction(PC_XORIS, pivotHighReg, highReg, 0x8000);
            } else {
                compareHighReg = switchRegHi;
                pivotHighReg = highReg;
            }
            PCodeUtilities_EmitInstruction(PC_SUBFC, lowReg, data_00581178, lowReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, compareHighReg, pivotHighReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, lowReg, lowReg);
            PCodeUtilities_EmitInstruction(PC_NEG, highReg, highReg);
            PCodeUtilities_EmitInstruction(PC_CMPI, 0, highReg, 0);
            PCodeUtilities_EmitConditionBranch(0, 0x18, 1, case_ranges[firstCase].info);
            emit_case_range_binary_tree(rightFirst, lastCase);
        } else {
            PCodeLabel *rightLabel;
            lowReg = gUsedVirtualRegistersGPR++;
            highReg = gUsedVirtualRegistersGPR++;
            compareHighReg = gUsedVirtualRegistersGPR++;
            pivotHighReg = gUsedVirtualRegistersGPR++;
            PCodeUtilities_LoadImmediate(lowReg, pivot->base.lo);
            PCodeUtilities_LoadImmediate(highReg, pivot->base.hi);
            if (TYPE_INTEGRAL(switch_expr_type)->integral != 12 && TYPE_INTEGRAL(switch_expr_type)->integral != 12) {
                PCodeUtilities_EmitInstruction(PC_XORIS, compareHighReg, switchRegHi, 0x8000);
                PCodeUtilities_EmitInstruction(PC_XORIS, pivotHighReg, highReg, 0x8000);
            } else {
                compareHighReg = switchRegHi;
                pivotHighReg = highReg;
            }
            PCodeUtilities_EmitInstruction(PC_SUBFC, lowReg, lowReg, data_00581178);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, pivotHighReg, compareHighReg);
            PCodeUtilities_EmitInstruction(PC_SUBFE, highReg, lowReg, lowReg);
            PCodeUtilities_EmitInstruction(PC_NEG, highReg, highReg);
            PCodeUtilities_EmitInstruction(PC_CMPI, 0, highReg, 0);
            rightLabel = PCode_NewLabel();
            PCodeUtilities_EmitConditionBranch(0, 0x17, 1, rightLabel);
            emit_case_range_binary_tree(firstCase, leftLast);
            PCodeUtilities_ResolveLabel(rightLabel);
            emit_case_range_binary_tree(rightFirst, lastCase);
        }
    }
}

void emit_case_ranges(ENode *expr)
{
    Operand operand;

    memclrw(&operand, sizeof(operand));
    if ((expr->rtype->type == TYPEINT || expr->rtype->type == TYPEENUM) && expr->rtype->size == 8) {
        data_00560648[expr->type](expr, 0, 0, &operand);
        Operands_ForceGPRPair(&operand, expr->rtype, 0, 0);
        switch_expr_type = expr->rtype;
        data_00581178 = operand.reg;
        switchRegHi = operand.regHi;
        emit_case_range_binary_tree(0, case_range_count);
    } else {
        data_00560648[expr->type](expr, 0, 0, &operand);
        if (expr->rtype->size < 4) {
            Operands_ExtendGPR(&operand, expr->rtype, 0);
        }
        if (operand.kind != OpndType_GPR) {
            Operands_ForceGPR(&operand, expr->rtype, 0);
        }
        switch_expr_type = expr->rtype;
        data_00581178 = operand.reg;
        emit_case_range_binary_search(0, case_range_count);
    }
}

Object *create_switchtable(void)
{
    Object *obj;
    ObjectList *node;
    SInt32 *dst;
    CaseRange *p;
    CInt64 idx;

    obj = (Object *)galloc(sizeof(Object));
    node = (ObjectList *)galloc(sizeof(ObjectList));
    memclrw(obj, sizeof(Object));
    memclrw(node, sizeof(ObjectList));

    obj->otype = OT_OBJECT;
    obj->access = ACCESSPUBLIC;
    obj->datatype = DDATA;
    obj->name = CParser_GetUniqueName();
    obj->dwarfLinks.toc = NULL;
    obj->sclass = TK_STATIC;
    obj->qual = Q_CONST;
    obj->flags |= 6;
    obj->u.data.linkname = obj->name;
    obj->section = ObjGen_PPC_EABI_GetHeaderIndex(2);
    obj->type = NULL;
    fn_0049f230(obj, 0, 0);
    obj->type = (Type *)&void_ptr;
    obj->u.data.u.switchtable.data =
        (SInt32 *)CompilerTools_AllocatePool((obj->u.data.u.switchtable.size = switchtable_max.lo + 1) << 2);

    p = case_ranges;
    dst = obj->u.data.u.switchtable.data;
    idx = cint64_zero;
    while (CInt64_LessEqual(idx, switchtable_max)) {
        while (CInt64_Greater(CInt64_Add(switchtable_base, idx), CInt64_Add(p->base, p->width)))
            p++;
        *dst = (SInt32)p->info;
        idx = CInt64_Add(idx, cint64_one);
        dst++;
    }

    node->object = obj;
    node->next = switch_tables;
    switch_tables = node;
    return node->object;
}

void generate_switchtable_dispatch(ENode *node, SwitchInfo *cases)
{
    Object *obj;
    Scratch immediate;
    Operand operand;
    Operand target;
    SInt16 indexReg;
    SInt32 offset;
    SInt32 high;
    SInt16 baseReg;
    UInt16 low;
    CInt64 savedValue;
    SwitchCase *entry;
    CLabel *defaultCase;

    savedValue = data_005608f8;
    memclrw(&operand, sizeof(operand));
    memclrw(&target, sizeof(target));
    if (CInt64_Greater(switchtable_base, cint64_zero)) {
        if (CInt64_Less(switchtable_base, savedValue)) {
            switchtable_max = CInt64_Add(switchtable_max, switchtable_base);
            switchtable_base = cint64_zero;
        }
    }
    obj = create_switchtable();
    if ((node->rtype->type == TYPEINT || node->rtype->type == TYPEENUM) && node->rtype->size == 8)
        CError_FATAL(554);
    data_00560648[node->type](node, 0, 0, &operand);
    if (node->rtype->size < 4)
        Operands_ExtendGPR(&operand, node->rtype, 0);
    if (operand.kind != OpndType_GPR)
        Operands_ForceGPR(&operand, node->rtype, 0);
    indexReg = operand.reg;
    if (CInt64_NotEqual(switchtable_base, cint64_zero)) {
        indexReg = gUsedVirtualRegistersGPR++;
        immediate.value = (SInt16)-switchtable_base.lo;
        {
            Scratch *shortImmediate = &immediate;
        }
        offset = -switchtable_base.lo;
        if (offset != immediate.value) {
            high = offset >> 16;
            PCodeUtilities_EmitInstruction(PC_ADDIS, indexReg, operand.reg, 0, (SInt16)(((offset >> 15) & 1) + high));
            if ((SInt16)offset != 0)
                PCodeUtilities_EmitInstruction(PC_ADDI, indexReg, indexReg, 0, immediate);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, indexReg, operand.reg, 0, offset);
        }
    }
    low = switchtable_max.lo;
    if (switchtable_max.lo != low) {
        baseReg = gUsedVirtualRegistersGPR++;
        PCodeUtilities_LoadImmediate(baseReg, switchtable_max.lo);
        PCodeUtilities_EmitInstruction(PC_CMPL, 0, indexReg, baseReg);
    } else {
        PCodeUtilities_EmitInstruction(PC_CMPLI, 0, indexReg, switchtable_max.lo);
    }
    PCodeUtilities_EmitConditionBranch(0, 0x14, 1, default_case_label);
    {
        SInt32 targetAddress;
        if (((Object *)(targetAddress = (SInt32)obj))->dwarfLinks.toc != NULL) {
            target.kind = OpndType_IndirectSymbol;
            targetAddress = (SInt32)((Object *)targetAddress)->dwarfLinks.toc;
            target.object = (Object *)targetAddress;
        } else {
            target.kind = OpndType_Symbol;
            target.object = obj;
        }
        (void)targetAddress;
    }
    if (target.kind != OpndType_GPR) {
        baseReg = gUsedVirtualRegistersGPR++;
        Operands_ForceGPR(&target, (Type *)&void_ptr, baseReg);
    }
    if (target.kind != OpndType_GPR) {
        CError_FATAL(599);
    } else {
        if (target.reg != baseReg)
            PCodeUtilities_EmitInstruction(PC_MR, baseReg, target.reg);
    }
    if (CInt64_Equal(switchtable_base, cint64_zero)) {
        indexReg = gUsedVirtualRegistersGPR++;
        PCodeUtilities_EmitInstruction(PC_RLWINM, indexReg, operand.reg, 2, 0, 0x1d);
    } else {
        PCodeUtilities_EmitInstruction(PC_RLWINM, indexReg, indexReg, 2, 0, 0x1d);
    }
    PCodeUtilities_EmitInstruction(PC_LWZX, baseReg, baseReg, indexReg);
    for (entry = cases->cases; entry != NULL; entry = entry->next)
        PCode_AddSuccessor(gCurrentBlock, entry->label->pclabel);
    defaultCase = cases->defaultlabel;
    PCode_AddSuccessor(gCurrentBlock, defaultCase->pclabel);
    PCodeUtilities_EmitInstruction(PC_MTCTR, baseReg);
    PCodeUtilities_EmitInstructionAndCreateBlock(obj);
}

void Switch_GenerateSwitch(ENode *expression, SwitchInfo *cases)
{
    SwitchCase *caseNode;
    CLabel *defaultCase;

    switch_case_count = 0;
    for (caseNode = cases->cases; caseNode != NULL; caseNode = caseNode->next) {
        if (caseNode->label->pclabel == NULL)
            caseNode->label->pclabel = PCode_NewLabel();
        switch_case_count++;
    }
    CError_ASSERT(651, switch_case_count >= 0 && switch_case_count <= 0x3333332U);
    defaultCase = cases->defaultlabel;
    if (defaultCase->pclabel == NULL) {
        PCodeLabel *id = PCode_NewLabel();
        defaultCase = cases->defaultlabel;
        defaultCase->pclabel = id;
    }
    defaultCase = cases->defaultlabel;
    default_case_label = defaultCase->pclabel;
    build_case_ranges(expression->rtype, cases->cases, cases->defaultlabel);
    if ((expression->rtype->type == TYPEINT || expression->rtype->type == TYPEENUM) && expression->rtype->size == 8)
        emit_case_ranges(expression);
    else if (case_range_count < 8 || (case_range_count * 2U) < (switchtable_max.lo >> 1) + 4)
        emit_case_ranges(expression);
    else
        generate_switchtable_dispatch(expression, cases);
}
