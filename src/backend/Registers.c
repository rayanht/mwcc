#define CERROR_FILE "Registers.c"
#include "compiler/common.h"
#include "compiler/Registers.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/COptimizer.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/CopyPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
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
#include "compiler/Scheduler.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include <string.h>

static UInt8 gUsedPhysicalGPR[32];
static unsigned char gUsedPhysicalFPR[32];
static UInt8 gUsedPhysicalVR[32];
static short gSaveSpan;
static unsigned char gSavedUsedPhysicalRegisters[32];
static char lbl_00581392[14];

static void Registers_RestoreClassState(unsigned char *used, short *save_span)
{
    *save_span = gSaveSpan;
    memcpy(used, gSavedUsedPhysicalRegisters, 32);
}

static void Registers_SaveClassState(const unsigned char *used, short save_span)
{
    gSaveSpan = save_span;
    memcpy(gSavedUsedPhysicalRegisters, used, 32);
}

static void Registers_Require(int condition, int line)
{
    if (!condition) {
        CError_Internal("Registers.c", line);
    }
}

static void Registers_RecordPhysicalUse(unsigned char *used, short reg, int scan_floor, int saved_floor,
                                        short *save_span, short *available_saved)
{
    int first_used;
    int free_count;
    int scan;

    if (used[reg] != 0) {
        return;
    }

    used[reg] = 1;
    first_used = 32;
    free_count = 0;
    for (scan = 31; scan >= scan_floor; scan--) {
        if (used[scan] == 1) {
            first_used = scan;
        }
        if (scan > saved_floor && used[scan] == 0) {
            free_count++;
        }
    }
    *save_span = (short)(32 - first_used);
    *available_saved = (short)free_count;
}

static int Registers_ObjectUsesFPRs(Object *object)
{
    return copts.operandsDebug && object->type->type == TYPEFLOAT;
}

static short Registers_FindFree(unsigned char *used, int first, void (*bind)(Object *, short))
{
    int reg;

    for (reg = 31; reg >= first; reg--) {
        if (used[reg] == 0) {
            bind(NULL, (short)reg);
            return (short)reg;
        }
    }
    return -1;
}

static unsigned int Registers_BuildColorMask(const unsigned char *used, int last_register)
{
    unsigned int mask;
    int reg;

    mask = 0;
    for (reg = 0; reg <= last_register; reg++) {
        if (used[reg] == 0) {
            mask |= 1U << reg;
        }
    }
    return mask;
}

static int Registers_CountFree(const unsigned char *used)
{
    int count;
    int reg;

    count = 0;
    for (reg = 0; reg < 32; reg++) {
        if (used[reg] == 0) {
            count++;
        }
    }
    return count;
}

static void Registers_MarkFPRUsed(SInt16 regnum)
{
    SInt32 avail;
    SInt32 start;
    SInt32 i;

    start = 0x20;
    avail = 0;
    gUsedPhysicalFPR[regnum] = 1;
    i = 0x1f;
    do {
        if (gUsedPhysicalFPR[i] == 1) {
            start = i;
        }
        if (i > 0x11 && gUsedPhysicalFPR[i] == 0) {
            avail++;
        }
    } while (--i >= 0xe);
    gFPRSaveSpan = 0x20 - start;
    gAvailableSavedFPRs = avail;
}

static SInt16 Registers_FindFreeGPR(void)
{
    SInt32 i = 31;
    do {
        if (gUsedPhysicalGPR[i] == 0) {
            Registers_BindGPR(NULL, i);
            return (SInt16)i;
        }
        i--;
    } while (i >= 14);
    return -1;
}

static VarInfo *Registers_NewInfo(void)
{
    VarInfo *p = (VarInfo *)galloc(sizeof(VarInfo));
    memclrw(p, sizeof(VarInfo));
    return p;
}

void Registers_InitRegisterState(void)
{
    SInt16 i;
    gUsedVirtualRegistersGPR = 0x20;
    gUsedVirtualRegistersFPR = 0x20;
    gUsedVirtualRegistersVR = 0x20;
    gGPRSaveSpan = 0;
    gFPRSaveSpan = 0;
    data_005883ee = 0;
    gVRSaveSpan = 0;
    gAvailableSavedGPRs = 0x10;
    gAvailableSavedFPRs = 0xe;
    gAvailableSavedVRs = 0xc;
    for (i = 0; i <= 0x1f; i++) {
        gUsedPhysicalGPR[i] = 0;
        gUsedPhysicalFPR[i] = 0;
        gUsedPhysicalVR[i] = 0;
    }
    gUsedPhysicalGPR[1] = 2;
    gUsedPhysicalGPR[2] = 2;
    Registers_BindGPR(NULL, 0xd);
    gUsedPhysicalGPR[13] = 2;
    {
        SInt32 usevrs = 0;
        if (copts.deleteDeadInstructions > 0) {
            SInt32 cond = !data_00588224;
            if (cond)
                usevrs = 1;
        }
        gUseVirtualRegisterNumbers_00587f00 = usevrs;
    }
    gVirtualRegistersActive = 1;
}

void Registers_SetupStackBaseReg(void)
{
    if (data_0058852d != 0U) {
        Registers_BindGPR(NULL, 31);
        stack_base_reg = 31U;
    } else {
        stack_base_reg = 1U;
    }
}

void Registers_AllocateGPR(Object *obj)
{
    VarInfo *info;
    SInt16 reg;
    SInt32 i;

    switch (obj->datatype) {
        case DDATA:
            if (obj->u.data.info == NULL) {
                VarInfo *v = (VarInfo *)galloc(sizeof(VarInfo));
                memclrw(v, sizeof(VarInfo));
                obj->u.data.info = v;
            }
            info = obj->u.data.info;
            break;
        case DLOCAL:
            CError_ASSERT(745, obj->u.var.info);
            info = obj->u.var.info;
            break;
        case DABSOLUTE:
            if (obj->u.data.info == NULL) {
                info = (VarInfo *)galloc(sizeof(VarInfo));
                memclrw(info, sizeof(VarInfo));
                obj->u.data.info = info;
            }
            info = obj->u.data.info;
            break;
        default:
            CError_FATAL(758);
            info = NULL;
            break;
    }

    if (gUseVirtualRegisterNumbers_00587f00 != 0) {
        reg = gUsedVirtualRegistersGPR++;
    } else {
        i = 0x1f;
        for (;;) {
            if (gUsedPhysicalGPR[i] == 0) {
                Registers_BindGPR(NULL, i);
                break;
            }
            if (--i < 0xe) {
                i = -1;
                break;
            }
        }
        reg = (SInt16)i;
    }

    info->is_fpr = 0;
    info->is_vector = 0;
    if (copts.operandsDebug != 0 && obj->type->type == TYPEFLOAT)
        info->is_fpr = 1;
    if (reg > 0)
        info->reg = reg;
}

void Registers_AllocateGPRPair(Object *obj)
{
    SInt16 reg1;
    SInt16 reg2;
    VarInfo *info;

    switch (obj->datatype) {
        case DDATA:
            if (obj->u.data.info == NULL)
                obj->u.data.info = Registers_NewInfo();
            info = obj->u.data.info;
            break;
        case DLOCAL:
            CError_ASSERT(745, obj->u.var.info != NULL);
            info = obj->u.var.info;
            break;
        case DABSOLUTE:
            if (obj->u.data.info == NULL)
                obj->u.data.info = Registers_NewInfo();
            info = obj->u.data.info;
            break;
        default:
            CError_FATAL(758);
            info = NULL;
            break;
    }

    if (gUseVirtualRegisterNumbers_00587f00) {
        reg1 = gUsedVirtualRegistersGPR++;
        reg2 = gUsedVirtualRegistersGPR++;
    } else {
        CError_ASSERT(185, gAvailableSavedGPRs >= 2);
        reg1 = Registers_FindFreeGPR();
        reg2 = Registers_FindFreeGPR();
    }

    info->is_fpr = 0;
    info->is_vector = 0;
    if (copts.operandsDebug && obj->type->type == TYPEFLOAT)
        info->is_fpr = 1;
    if (reg1 > 0 && reg2 > 0) {
        info->reg = reg1;
        info->regHi = reg2;
    }
}

void Registers_AllocateFPR(Object *obj)
{
    SInt16 reg;
    SInt32 i;
    VarInfo *info;

    switch (obj->datatype) {
        case DDATA:
            if (obj->u.data.info == NULL) {
                VarInfo *p = (VarInfo *)galloc(sizeof(VarInfo));
                memclrw(p, sizeof(VarInfo));
                obj->u.data.info = p;
            }
            info = obj->u.data.info;
            break;

        case DLOCAL:
            CError_ASSERT(745, obj->u.var.info != NULL);
            info = obj->u.var.info;
            break;

        case DABSOLUTE:
            if (obj->u.data.info == NULL) {
                VarInfo *p = (VarInfo *)galloc(sizeof(VarInfo));
                memclrw(p, sizeof(VarInfo));
                obj->u.data.info = p;
            }
            info = obj->u.data.info;
            break;

        default:
            CError_FATAL(758);
            info = NULL;
            break;
    }

    if (gUseVirtualRegisterNumbers_00587f00) {
        reg = gUsedVirtualRegistersFPR++;
    } else {
        i = 31;
        for (;;) {
            if (gUsedPhysicalFPR[i] == 0) {
                Registers_BindFPR(NULL, i);
                reg = (SInt16)i;
                break;
            }
            if (--i < 14) {
                reg = -1;
                break;
            }
        }
    }

    info->is_fpr = 1;
    info->is_vector = 0;
    if (reg > 0)
        info->reg = reg;
}

void Registers_AllocateVR(Object *obj)
{
    SInt16 reg;
    SInt32 i;
    VarInfo *vr;

    switch (obj->datatype) {
        case DDATA:
            if (obj->u.data.info == NULL) {
                vr = (VarInfo *)galloc(sizeof(*vr));
                memclrw(vr, sizeof(*vr));
                obj->u.data.info = vr;
            }
            vr = obj->u.data.info;
            break;
        case DLOCAL:
            if (obj->u.var.info == NULL)
                CError_FATAL(745);
            vr = obj->u.var.info;
            break;
        case DABSOLUTE:
            if (obj->u.data.info == NULL) {
                vr = (VarInfo *)galloc(sizeof(*vr));
                memclrw(vr, sizeof(*vr));
                obj->u.data.info = vr;
            }
            vr = obj->u.data.info;
            break;
        default:
            CError_FATAL(758);
            vr = NULL;
    }

    if (gUseVirtualRegisterNumbers_00587f00 != 0) {
        reg = gUsedVirtualRegistersVR;
        gUsedVirtualRegistersVR++;
    } else {
        i = 31;
        for (;;) {
            if (gUsedPhysicalVR[i] == 0) {
                Registers_BindVR(NULL, i);
                reg = (SInt16)i;
                break;
            }
            i--;
            if (i < 20) {
                reg = -1;
                break;
            }
        }
    }
    vr->is_fpr = 0;
    vr->is_vector = 1;
    if (reg > 0)
        vr->reg = reg;
}

void Registers_BindGPR(Object *obj, SInt16 reg)
{
    SInt32 i;
    SInt32 maxreg;
    SInt32 avail;
    VarInfo *info;

    if (gUsedPhysicalGPR[reg] == 0) {
        maxreg = 32;
        avail = 0;
        gUsedPhysicalGPR[reg] = 1;
        i = 31;
        do {
            if (gUsedPhysicalGPR[i] == 1)
                maxreg = i;
            if (i > 15 && gUsedPhysicalGPR[i] == 0)
                avail++;
        } while (--i >= 14);
        gGPRSaveSpan = (SInt16)(32 - maxreg);
        gAvailableSavedGPRs = (SInt16)avail;
    }

    if (obj != NULL) {
        switch (obj->datatype) {
            case DDATA:
                if (obj->u.data.info == NULL) {
                    info = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(info, sizeof(VarInfo));
                    obj->u.data.info = info;
                }
                info = obj->u.data.info;
                break;
            case DLOCAL:
                CError_ASSERT(745, obj->u.var.info != NULL);
                info = obj->u.var.info;
                break;
            case DABSOLUTE:
                if (obj->u.data.info == NULL) {
                    info = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(info, sizeof(VarInfo));
                    obj->u.data.info = info;
                }
                info = obj->u.data.info;
                break;
            default:
                CError_FATAL(758);
                info = NULL;
        }

        info->is_fpr = 0;
        info->is_vector = 0;
        info->reg = reg;
        if (copts.operandsDebug && obj->type->type == TYPEFLOAT)
            info->is_fpr = 1;
    }
}

/* VarInfo layout recovered from the original: 0x2c bytes, with the
 * register fields at 0x24/0x26 and the two flags at 0x28/0x2a. */

void Registers_BindGPRPair(Object *obj, SInt16 reg0, SInt16 reg1)
{
    VarInfo *info;

    Registers_BindGPR(NULL, reg0);
    Registers_BindGPR(NULL, reg1);
    if (obj != NULL) {
        switch (obj->datatype) {
            case DDATA:
                if (obj->u.data.info == NULL) {
                    VarInfo *vi = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(vi, sizeof(VarInfo));
                    obj->u.data.info = vi;
                }
                info = obj->u.data.info;
                break;
            case DLOCAL:
                if (obj->u.var.info == NULL)
                    CError_FATAL(745);
                info = obj->u.var.info;
                break;
            case DABSOLUTE:
                if (obj->u.data.info == NULL) {
                    VarInfo *vi = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(vi, sizeof(VarInfo));
                    obj->u.data.info = vi;
                }
                info = obj->u.data.info;
                break;
            default:
                CError_FATAL(758);
                info = NULL;
                break;
        }
        info->is_fpr = 0;
        info->is_vector = 0;
        info->reg = reg0;
        info->regHi = reg1;
        if (copts.operandsDebug && obj->type->type == TYPEFLOAT)
            info->is_fpr = 1;
    }
}

void Registers_BindFPR(Object *obj, SInt16 regnum)
{
    if (gUsedPhysicalFPR[regnum] == 0) {
        Registers_MarkFPRUsed(regnum);
    }
    if (obj != NULL) {
        VarInfo *info;

        switch (obj->datatype) {
            case DDATA:
                if (obj->u.data.info == NULL) {
                    VarInfo *p;
                    p = galloc(sizeof(VarInfo));
                    memclrw(p, sizeof(VarInfo));
                    obj->u.data.info = p;
                }
                info = obj->u.data.info;
                break;
            case DLOCAL:
                if (obj->u.var.info == NULL) {
                    CError_FATAL(745);
                }
                info = obj->u.var.info;
                break;
            case DABSOLUTE:
                if (obj->u.data.info == NULL) {
                    VarInfo *p;
                    p = galloc(sizeof(VarInfo));
                    memclrw(p, sizeof(VarInfo));
                    obj->u.data.info = p;
                }
                info = obj->u.data.info;
                break;
            default:
                CError_FATAL(758);
                info = NULL;
        }

        info->is_fpr = 1;
        info->is_vector = 0;
        info->reg = regnum;
    }
}

void Registers_BindVR(Object *obj, short vr)
{
    if (gUsedPhysicalVR[vr] == 0) {
        SInt32 i;
        SInt32 span = 0x20;
        SInt32 avail = 0;

        gUsedPhysicalVR[vr] = 1;
        i = 0x1f;
        do {
            if (gUsedPhysicalVR[i] == 1)
                span = i;
            if (i > 0x13 && gUsedPhysicalVR[i] == 0)
                avail++;
            i--;
        } while (i >= 0x14);
        gVRSaveSpan = 0x20 - span;
        gAvailableSavedVRs = avail;
    }
    if (obj != NULL) {
        VarInfo *regs;

        switch (obj->datatype) {
            case DDATA:
                if (obj->u.data.info == NULL) {
                    VarInfo *p = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(p, sizeof(VarInfo));
                    obj->u.data.info = p;
                }
                regs = obj->u.data.info;
                break;
            case DLOCAL:
                CError_ASSERT(745, obj->u.var.info != NULL);
                regs = obj->u.var.info;
                break;
            case DABSOLUTE:
                if (obj->u.data.info == NULL) {
                    VarInfo *p = (VarInfo *)galloc(sizeof(VarInfo));
                    memclrw(p, sizeof(VarInfo));
                    obj->u.data.info = p;
                }
                regs = obj->u.data.info;
                break;
            default:
                CError_FATAL(758);
                regs = NULL;
                break;
        }

        regs->is_fpr = 0;
        regs->is_vector = 1;
        regs->reg = vr;
    }
}

int Registers_AvailableGPRs(void)
{
    return Registers_CountFree(gUsedPhysicalGPR);
}

int Registers_AvailableFPRs(void)
{
    return Registers_CountFree(gUsedPhysicalFPR);
}

int Registers_AvailableVRs(void)
{
    return Registers_CountFree(gUsedPhysicalVR);
}

unsigned int Coloring_GPRColorMask(void)
{
    return Registers_BuildColorMask(gUsedPhysicalGPR, 12);
}

unsigned int Coloring_FPRColorMask(void)
{
    return Registers_BuildColorMask(gUsedPhysicalFPR, 13);
}

unsigned int Coloring_VRColorMask(void)
{
    return Registers_BuildColorMask(gUsedPhysicalVR, 19);
}

short Coloring_ClaimGPRColor(void)
{
    int reg;
    for (reg = 31; reg >= 14; --reg) {
        if (gUsedPhysicalGPR[reg] == 0) {
            Registers_BindGPR(NULL, reg);
            return reg;
        }
    }
    return -1;
}

short Coloring_ClaimFPRColor(void)
{
    int color;

    for (color = 31; color >= 14; --color) {
        if (gUsedPhysicalFPR[color] == 0) {
            Registers_BindFPR(NULL, color);
            return color;
        }
    }
    return -1;
}

short Coloring_ClaimVRColor(void)
{
    int color;

    for (color = 31; color >= 20; --color) {
        if (gUsedPhysicalVR[color] == 0) {
            Registers_BindVR(NULL, color);
            return color;
        }
    }
    return -1;
}

unsigned char Registers_AreNonvolatileVRUsed(void)
{
    int reg;

    for (reg = 31; reg >= 20; --reg) {
        if (gUsedPhysicalVR[reg] != 0) {
            return 1;
        }
    }
    return 0;
}

void Registers_BeginCoalesceWindow(void)
{
    gGPRCoalesceFirst = gGPRCoalesceLast = gUsedVirtualRegistersGPR;
    gFPRCoalesceFirst = gFPRCoalesceLast = gUsedVirtualRegistersFPR;
    gVRCoalesceFirst = gVRCoalesceLast = gUsedVirtualRegistersVR;
}

void Registers_SnapshotInitialObjectRange(void)
{
    gInitialObjectGPRLast = gUsedVirtualRegistersGPR - 1;
    gInitialObjectFPRLast = gUsedVirtualRegistersFPR - 1;
    gInitialObjectVRLast = gUsedVirtualRegistersVR - 1;
}

void Registers_CheckpointCoalesceWindow(void)
{
    gGPRCounterCheckpoint = gGPRCoalesceLast = gUsedVirtualRegistersGPR;
    gFPRCounterCheckpoint = gFPRCoalesceLast = gUsedVirtualRegistersFPR;
    gVRCounterCheckpoint = gVRCoalesceLast = gUsedVirtualRegistersVR;
}

void Registers_UpdateCoalesceWindow(void)
{
    if (!gUseVirtualRegisterNumbers_00587f00) {
        if (gUsedVirtualRegistersGPR > gGPRCoalesceLast) {
            gGPRCoalesceLast = gUsedVirtualRegistersGPR;
        }
        if (gUsedVirtualRegistersFPR > gFPRCoalesceLast) {
            gFPRCoalesceLast = gUsedVirtualRegistersFPR;
        }
        if (gUsedVirtualRegistersVR > gVRCoalesceLast) {
            gVRCoalesceLast = gUsedVirtualRegistersVR;
        }
        if (gUsedVirtualRegistersGPR > 0x100) {
            gUsedVirtualRegistersGPR = gGPRCounterCheckpoint;
        }
        if (gUsedVirtualRegistersFPR > 0x100) {
            gUsedVirtualRegistersFPR = gFPRCounterCheckpoint;
        }
        if (gUsedVirtualRegistersVR > 0x100) {
            gUsedVirtualRegistersVR = gVRCounterCheckpoint;
        }
    }
}

void Registers_CloseCoalesceWindow(void)
{
    if (gUsedVirtualRegistersGPR < gGPRCoalesceLast) {
        gUsedVirtualRegistersGPR = gGPRCoalesceLast;
    } else {
        gGPRCoalesceLast = gUsedVirtualRegistersGPR;
    }
    if (gUsedVirtualRegistersFPR < gFPRCoalesceLast) {
        gUsedVirtualRegistersFPR = gFPRCoalesceLast;
    } else {
        gFPRCoalesceLast = gUsedVirtualRegistersFPR;
    }
    if (gUsedVirtualRegistersVR < gVRCoalesceLast) {
        gUsedVirtualRegistersVR = gVRCoalesceLast;
    } else {
        gVRCoalesceLast = gUsedVirtualRegistersVR;
    }
}

VarInfo *Registers_GetInfo(Object *object)
{
    VarInfo *info;

    switch (object->datatype) {
        case DDATA:
            if (!object->u.data.info) {
                info = (VarInfo *)galloc(44U);
                memclrw(info, 44U);
                object->u.data.info = info;
            }
            return object->u.data.info;
        case DLOCAL:
            if (!object->u.var.info)
                CError_FATAL(745);
            return object->u.var.info;
        case DABSOLUTE:
            if (!object->u.data.info) {
                info = (VarInfo *)galloc(44U);
                memclrw(info, 44U);
                object->u.data.info = info;
            }
            return object->u.data.info;
        default:
            CError_FATAL(758);
            return NULL;
    }
}

UInt32 Registers_GetOperandRegMask(PCodeBlock *list)
{
    UInt32 mask = 0;
    PCodeBlock *cur = list;

    while (cur != NULL) {
        PCodeInstruction *node;
        for (node = cur->instructions; node != NULL; node = node->next) {
            if (node->flags & 3) {
                SInt32 i;
                for (i = 0; i < node->operand_count; i++) {
                    if (node->operandData.operands[i].kind == PCOp_VR)
                        mask |= 1 << (31 - node->operandData.operands[i].value.reg);
                }
            }
        }
        cur = cur->next;
    }
    return mask;
}

SInt32 fn_004c15f0(void)
{
    SInt16 i;
    SInt16 n;

    if (data_00588521 == 0)
        return 1;

    n = 0x20;
    for (i = 0; i <= 0xd; i++) {
        if (gUsedPhysicalFPR[i] != 0)
            n++;
    }
    if (gUsedVirtualRegistersFPR > n)
        return 1;

    n = 0x20;
    for (i = 0; i <= 0x13; i++) {
        if (gUsedPhysicalVR[i] != 0)
            n++;
    }
    if (gUsedVirtualRegistersVR > n)
        return 1;

    for (i = 0xe; i <= 0x1f; i++) {
        if (gUsedPhysicalFPR[i] != 0)
            return 1;
    }

    for (i = 0x14; i <= 0x1f; i++) {
        if (gUsedPhysicalVR[i] != 0)
            return 1;
    }

    return 0;
}

void Registers_SetupGPRs(void)
{
    gSaveSpan = gGPRSaveSpan;
    memcpy(gSavedUsedPhysicalRegisters, gUsedPhysicalGPR, 32);
}

void Registers_SetupFPRs(void)
{
    gSaveSpan = gFPRSaveSpan;
    memcpy(gSavedUsedPhysicalRegisters, gUsedPhysicalFPR, 32);
}

void Registers_SetupVRs(void)
{
    gSaveSpan = gVRSaveSpan;
    memcpy(gSavedUsedPhysicalRegisters, gUsedPhysicalVR, 32);
}

void Coloring_ResetGPRColors(void)
{
    gGPRSaveSpan = gSaveSpan;
    memcpy(gUsedPhysicalGPR, gSavedUsedPhysicalRegisters, 32);
}

void Coloring_ResetFPRColors(void)
{
    gFPRSaveSpan = gSaveSpan;
    memcpy(gUsedPhysicalFPR, gSavedUsedPhysicalRegisters, 32);
}

void Coloring_ResetVRColors(void)
{
    gVRSaveSpan = gSaveSpan;
    memcpy(gUsedPhysicalVR, gSavedUsedPhysicalRegisters, 32);
}
