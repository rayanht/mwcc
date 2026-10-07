#define CERROR_FILE "Coloring.c"
#include "compiler/common.h"
#include "compiler/Coloring.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeListing.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Scheduler.h"
#include "compiler/SpillCode.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"

static short gColoringRegisterCount;

/* Interference graph node, recovered from the field offsets used in
 * Coloring_SetupGPRs: the object pointer sits at 0x04, the register number
 * (a word) at 0x10 and a byte of flags at 0x12. */

/* Register record returned by the binding lookup: word register number at
 * 0x24, second word register number at 0x26, byte flag at 0x28. */

static int Coloring_ObjectBelongsToClass(VarInfo *info, int reg_class);
static int Coloring_IsPairedGPRObject(Object *object);

static void Coloring_BindObjects(ObjectList *item, int reg_class);
static void Coloring_DecrementNeighbors(InterferenceNode *node);

void Coloring_SetupGPRs(void)
{
    UInt32 reg;
    ObjectList *entry;
    Object *object;
    VarInfo *info;

    for (reg = 0; reg < 32; ++reg)
        gInterferenceGraph[reg]->physical_register = reg;

    for (entry = arguments; entry != NULL; entry = entry->next) {
        object = entry->object;
        info = Registers_GetInfo(object);
        if (info->reg != 0 && (info->is_fpr == 0 || copts.operandsDebug != 0)) {
            gInterferenceGraph[info->reg]->object = object;
            if (((object->type->type == TYPEINT || object->type->type == TYPEENUM) && object->type->size == 8) ||
                (copts.operandsDebug != 0 && object->type->type == TYPEFLOAT && object->type->size != 4)) {
                gInterferenceGraph[info->reg]->flags |= 0x20;
                gInterferenceGraph[info->regHi]->flags |= 0x10;
                gInterferenceGraph[info->regHi]->object = object;
            }
        }
    }

    for (entry = locals; entry != NULL; entry = entry->next) {
        object = entry->object;
        info = Registers_GetInfo(object);
        if (info->reg != 0 && (info->is_fpr == 0 || copts.operandsDebug != 0)) {
            gInterferenceGraph[info->reg]->object = object;
            if (((object->type->type == TYPEINT || object->type->type == TYPEENUM) && object->type->size == 8) ||
                (copts.operandsDebug != 0 && object->type->type == TYPEFLOAT && object->type->size != 4)) {
                gInterferenceGraph[info->reg]->flags |= 0x20;
                gInterferenceGraph[info->regHi]->flags |= 0x10;
                gInterferenceGraph[info->regHi]->object = object;
            }
        }
    }
}

void Coloring_SetupFPRs(void)
{
    ObjectList *item;
    unsigned int reg;

    if (copts.operandsDebug) {
        CError_FATAL(132);
    }
    for (reg = 0; reg < 32; reg++) {
        gInterferenceGraph[reg]->physical_register = (short)reg;
    }
    for (item = arguments; item != NULL; item = item->next) {
        Object *object;
        VarInfo *info;

        object = item->object;
        info = Registers_GetInfo(object);
        if (info->reg != 0 && info->is_fpr) {
            gInterferenceGraph[info->reg]->object = object;
        }
    }
    for (item = locals; item != NULL; item = item->next) {
        Object *object;
        VarInfo *info;

        object = item->object;
        info = Registers_GetInfo(object);
        if (info->reg != 0 && info->is_fpr) {
            gInterferenceGraph[info->reg]->object = object;
        }
    }
}

void Coloring_SetupVRs(void)
{
    UInt32 i;
    ObjectList *list;

    for (i = 0; i < 0x20; i++)
        gInterferenceGraph[i]->physical_register = (SInt16)i;

    for (list = arguments; list != NULL; list = list->next) {
        Object *obj = list->object;
        VarInfo *info = Registers_GetInfo(obj);
        if (info->reg != 0 && info->is_vector != 0)
            gInterferenceGraph[info->reg]->object = obj;
    }

    for (list = locals; list != NULL; list = list->next) {
        Object *obj = list->object;
        VarInfo *info = Registers_GetInfo(obj);
        if (info->reg != 0 && info->is_vector != 0)
            gInterferenceGraph[info->reg]->object = obj;
    }
}

InterferenceNode *Coloring_SimplifyGraph(int allocation, int register_count, int node_count)
{
    InterferenceNode *remaining;
    InterferenceNode *stack;
    InterferenceNode *node;
    unsigned int neighbor_index;
    InterferenceNode *candidate;
    unsigned int spill_neighbor_index;
    int changed;
    unsigned int node_index;
    InterferenceNode *next_node;
    unsigned int next_neighbor_index;
    unsigned int scan_index;
    int removed;
    int degree;
    stack = NULL;
    do {
        remaining = NULL;
        removed = 0;
        for (scan_index = 32; scan_index < node_count; ++scan_index) {
            node = gInterferenceGraph[scan_index];
            if ((node->flags & 6) == 0) {
                if (node->degree < register_count) {
                    for (neighbor_index = 0; neighbor_index < node->neighbor_count; ++neighbor_index) {
                        gInterferenceGraph[node->neighbors[neighbor_index]]->degree -= 1;
                    }
                    node->flags |= 2;
                    node->next = stack;
                    stack = node;
                    removed = 1;
                } else {
                    node->next = remaining;
                    remaining = node;
                }
            }
        }
    } while (removed != 0);
    if (remaining != NULL) {
        SpillCode_ComputeSpillCosts(allocation);
    }
    while (remaining != NULL) {
        float maxcost = 3.40282347e+38f;
        double best_cost;
        double candidate_cost;
        double cost;
        node = remaining;
        best_cost = 0.0;
        candidate_cost = 0.0;
        if (remaining->virtual_register >= gColoringRegisterCount) {
            cost = maxcost;
        } else {
            degree = remaining->degree;
            cost = (double)remaining->spill_cost / (double)degree;
        }
        candidate = remaining->next;
        best_cost = cost;
        if (candidate != NULL) {
            do {
                if (candidate->virtual_register >= gColoringRegisterCount) {
                    cost = maxcost;
                } else {
                    degree = candidate->degree;
                    cost = (double)candidate->spill_cost / (double)degree;
                }
                candidate_cost = cost;
                if (candidate_cost < best_cost) {
                    node = candidate;
                    best_cost = candidate_cost;
                }
                candidate = candidate->next;
            } while (candidate != NULL);
        }
        for (spill_neighbor_index = 0; spill_neighbor_index < node->neighbor_count; ++spill_neighbor_index) {
            gInterferenceGraph[node->neighbors[spill_neighbor_index]]->degree -= 1;
        }
        node->flags |= 2;
        node->next = stack;
        stack = node;
        do {
            remaining = NULL;
            changed = 0;
            for (node_index = 32; node_index < node_count; ++node_index) {
                next_node = gInterferenceGraph[node_index];
                if ((next_node->flags & 6) == 0) {
                    if (next_node->degree < register_count) {
                        for (next_neighbor_index = 0; next_neighbor_index < next_node->neighbor_count;
                             ++next_neighbor_index) {
                            gInterferenceGraph[next_node->neighbors[next_neighbor_index]]->degree -= 1;
                        }
                        next_node->flags |= 2;
                        changed = 1;
                        next_node->next = stack;
                        stack = next_node;
                    } else {
                        next_node->next = remaining;
                        remaining = next_node;
                    }
                }
            }
        } while (changed != 0);
    }
    return stack;
}

int Coloring_SelectColors(int register_class, InterferenceNode *node)
{
    unsigned int initial_colors;
    int colors;
    int available_colors;
    int neighbor_index;
    short *neighbors;
    InterferenceNode *neighbor;
    int color;
    int additional_color;
    int success = 1;
    if (register_class == 0) {
        Coloring_ResetGPRColors();
    } else if (register_class == 1) {
        Coloring_ResetFPRColors();
    } else {
        Coloring_ResetVRColors();
    }
    if (register_class == 0) {
        initial_colors = Coloring_GPRColorMask();
    } else if (register_class == 1) {
        initial_colors = Coloring_FPRColorMask();
    } else {
        initial_colors = Coloring_VRColorMask();
    }
    colors = initial_colors;
    while (node != NULL) {
        available_colors = colors;
        neighbors = node->neighbors;
        neighbor_index = 0;
        while (neighbor_index < node->neighbor_count) {
            neighbor = gInterferenceGraph[*neighbors++];
            if (neighbor->physical_register != -1 && neighbor->physical_register < 32) {
                available_colors &= ~(1 << neighbor->physical_register);
            }
            neighbor_index++;
        }
        if (available_colors != 0) {
            color = 0;
            do {
                if ((1 << color & available_colors) != 0) {
                    node->physical_register = color;
                    break;
                }
                color++;
            } while (color < 32);
        } else {
            if (register_class == 0) {
                additional_color = Coloring_ClaimGPRColor();
            } else if (register_class == 1) {
                additional_color = Coloring_ClaimFPRColor();
            } else {
                additional_color = Coloring_ClaimVRColor();
            }
            if (additional_color != -1) {
                node->physical_register = additional_color;
                colors |= 1U << node->physical_register;
            } else {
                node->flags |= 1;
                success = 0;
            }
        }
        node = node->next;
    }
    return success;
}

static void Coloring_SetupClass(int reg_class)
{
    int reg;

    for (reg = 0; reg < 32; reg++) {
        gInterferenceGraph[reg]->physical_register = (short)reg;
    }
    Coloring_BindObjects(arguments, reg_class);
    Coloring_BindObjects(locals, reg_class);
}

static void Coloring_BindObjects(ObjectList *item, int reg_class)
{
    while (item != NULL) {
        Object *object;
        VarInfo *info;
        InterferenceNode *node;

        object = item->object;
        info = Registers_GetInfo(object);
        if (info->reg != 0 && Coloring_ObjectBelongsToClass(info, reg_class)) {
            node = gInterferenceGraph[info->reg];
            node->object = object;

            if (reg_class == RegClass_GPR && Coloring_IsPairedGPRObject(object)) {
                node->flags |= Interference_FirstOfPair;
                node = gInterferenceGraph[info->regHi];
                node->flags |= Interference_SecondOfPair;
                node->object = object;
            }
        }
        item = item->next;
    }
}

static int Coloring_ObjectBelongsToClass(VarInfo *info, int reg_class)
{
    if (reg_class == RegClass_VR) {
        return info->is_vector;
    }
    if (reg_class == RegClass_FPR) {
        return info->is_fpr;
    }
    return !info->is_fpr || copts.operandsDebug;
}

static int Coloring_IsPairedGPRObject(Object *object)
{
    Type *type;

    type = object->type;
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        return 1;
    }
    return copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4;
}

void Coloring_CommitAssignments(int reg_class, int register_count)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;
    InterferenceNode *node;
    VarInfo *info;
    unsigned int reg;
    int color;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            PCodeOperand *operand;
            int index;

            operand = instruction->operandData.operands;
            for (index = instruction->operand_count; index--;) {
                if (operand->kind == reg_class) {
                    operand->value.reg = gInterferenceGraph[operand->value.reg]->physical_register;
                }
                operand++;
            }

            if ((instruction->flags & fIsMove) != 0 &&
                instruction->operandData.operands[0].value.reg == instruction->operandData.operands[1].value.reg) {
                PCode_UnlinkInstruction(instruction);
            }
        }
    }

    for (reg = 32; reg < register_count; reg++) {
        node = gInterferenceGraph[reg];
        if (node->object != NULL && (node->flags & Interference_Spilled) == 0) {
            if ((node->flags & Interference_Coalesced) != 0) {
                color = node->physical_register;
                while (color >= 32) {
                    color = gInterferenceGraph[color]->physical_register;
                    if (color < 0) {
                        break;
                    }
                }
                node->physical_register = color;
            }

            if ((node->flags & Interference_SecondOfPair) != 0) {
                info = Registers_GetInfo(node->object);
                info->regHi = node->physical_register;
            } else {
                info = Registers_GetInfo(node->object);
                info->reg = node->physical_register;
            }
        }
    }
}

/* 0x563060: "VR" */
/* 0x563064: "AFTER CHECKING FOR ALTIVEC FRAME" */
/* 0x563088: "GPR" */
/* 0x56308c: "Coloring.c" */
/* 0x563098: "FPR" */

void Coloring_AllocateRegisters(Object *function)
{
    unsigned int spill;

    Registers_SetupVRs();
    gColoringRegisterCount = gUsedVirtualRegistersVR;
    if (gUsedVirtualRegistersVR > 0x20 && Registers_AvailableVRs() == 0) {
        PPCError_ReportError(0x66, "VR");
        return;
    }
    spill = 1;
    while (spill && gUsedVirtualRegistersVR > 0x20) {
        SpillCode_BuildInterference(function, 9, gUsedVirtualRegistersVR);
        Coloring_SetupVRs();
        spill = 0;
        if (Coloring_SelectColors(9, Coloring_SimplifyGraph(9, Registers_AvailableVRs(), gUsedVirtualRegistersVR)) == 0)
            spill = 1;
        if (spill)
            InterferenceGraph_SpillRegisters(9, gUsedVirtualRegistersVR);
        else
            Coloring_CommitAssignments(9, gUsedVirtualRegistersVR);
        CompilerTools_ResetPool();
    }
    StackFrame_CheckAltivec();
    if (copts.debug_listing && gHasAltivecFrame) {
        CodeGen_DumpPCode_004c4bd0(COptimizer_GetFunctionObject(function)->name, "AFTER CHECKING FOR ALTIVEC FRAME");
    }
    Registers_SetupGPRs();
    gColoringRegisterCount = gUsedVirtualRegistersGPR;
    if (gUsedVirtualRegistersGPR > 0x20 && Registers_AvailableGPRs() <= 0) {
        PPCError_ReportError(0x66, "GPR");
        return;
    }
    spill = 1;
    while (spill && gUsedVirtualRegistersGPR > 0x20) {
        SpillCode_BuildInterference(function, 0, gUsedVirtualRegistersGPR);
        Coloring_SetupGPRs();
        spill = 0;
        if (Coloring_SelectColors(0, Coloring_SimplifyGraph(0, Registers_AvailableGPRs(), gUsedVirtualRegistersGPR)) ==
            0)
            spill = 1;
        if (spill)
            InterferenceGraph_SpillRegisters(0, gUsedVirtualRegistersGPR);
        else
            Coloring_CommitAssignments(0, gUsedVirtualRegistersGPR);
        CompilerTools_ResetPool();
    }
    Registers_SetupFPRs();
    gColoringRegisterCount = gUsedVirtualRegistersFPR;
    if (copts.operandsDebug && gUsedVirtualRegistersFPR > 0x20)
        CError_FATAL(492);
    if (gUsedVirtualRegistersFPR > 0x20 && Registers_AvailableFPRs() <= 0) {
        PPCError_ReportError(0x66, "FPR");
        return;
    }
    spill = 1;
    while (spill && gUsedVirtualRegistersFPR > 0x20) {
        SpillCode_BuildInterference(function, 1, gUsedVirtualRegistersFPR);
        Coloring_SetupFPRs();
        spill = 0;
        if (Coloring_SelectColors(1, Coloring_SimplifyGraph(1, Registers_AvailableFPRs(), gUsedVirtualRegistersFPR)) ==
            0)
            spill = 1;
        if (spill)
            InterferenceGraph_SpillRegisters(1, gUsedVirtualRegistersFPR);
        else
            Coloring_CommitAssignments(1, gUsedVirtualRegistersFPR);
        CompilerTools_ResetPool();
    }
    gVirtualRegistersActive = 0;
}

static short Coloring_ResolveCoalescedColor(short color)
{
    while (color >= 32) {
        color = gInterferenceGraph[color]->physical_register;
    }
    return color;
}

static short Coloring_ClaimColor(int reg_class)
{
    if (reg_class == RegClass_GPR) {
        return Coloring_ClaimGPRColor();
    }
    if (reg_class == RegClass_FPR) {
        return Coloring_ClaimFPRColor();
    }
    return Coloring_ClaimVRColor();
}

static unsigned int Coloring_GetColorMask(int reg_class)
{
    if (reg_class == RegClass_GPR) {
        return Coloring_GPRColorMask();
    }
    if (reg_class == RegClass_FPR) {
        return Coloring_FPRColorMask();
    }
    return Coloring_VRColorMask();
}

static void Coloring_ResetColors(int reg_class)
{
    if (reg_class == RegClass_GPR) {
        Coloring_ResetGPRColors();
    } else if (reg_class == RegClass_FPR) {
        Coloring_ResetFPRColors();
    } else {
        Coloring_ResetVRColors();
    }
}

static int Coloring_SimplifyLowDegree(int available_colors, int register_count, InterferenceNode **stack,
                                      InterferenceNode **remaining)
{
    int changed;
    int reg;

    changed = 0;
    *remaining = NULL;
    for (reg = 32; reg < register_count; reg++) {
        InterferenceNode *node;

        node = gInterferenceGraph[reg];
        if ((node->flags & (Interference_Simplified | Interference_Coalesced)) == 0) {
            if (node->degree < available_colors) {
                Coloring_DecrementNeighbors(node);
                node->flags |= Interference_Simplified;
                node->next = *stack;
                *stack = node;
                changed = 1;
            } else {
                node->next = *remaining;
                *remaining = node;
            }
        }
    }
    return changed;
}

static void Coloring_DecrementNeighbors(InterferenceNode *node)
{
    int index;

    for (index = 0; index < node->neighbor_count; index++) {
        gInterferenceGraph[node->neighbors[index]]->degree--;
    }
}

static void Coloring_RunClass(Object *function, int reg_class, int register_count, int (*available_registers)(void),
                              void (*setup_class)(void))
{
    int retry;
    InterferenceNode *graph;

    retry = 1;
    while (retry && register_count > 32) {
        SpillCode_BuildInterference(function, reg_class, register_count);
        setup_class();
        retry = 0;
        graph = Coloring_SimplifyGraph(reg_class, available_registers(), register_count);
        if (!Coloring_SelectColors(reg_class, graph)) {
            retry = 1;
        }
        if (retry) {
            InterferenceGraph_SpillRegisters(reg_class, register_count);
        } else {
            Coloring_CommitAssignments(reg_class, register_count);
        }
        CompilerTools_ResetPool();
    }
}
