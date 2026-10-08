#ifndef COMPILER_REGISTERS_H
#define COMPILER_REGISTERS_H

#include "compiler/common.h"
#include "compiler/CPrep.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct VarInfo {
    struct Object *func;
    SInt32 usage;
    struct TStreamElement deftoken;
    SInt16 varnumber;
    UInt8 noregister;
    UInt8 used;
    SInt16 reg;
    SInt16 regHi;
    UInt8 is_fpr;
    UInt8 in_param_area;
    UInt8 is_vector;
    UInt8 alignmentPadding;
};
#pragma options align = reset
extern void Registers_BindVR(Object *obj, short vr);
extern void Registers_BindFPR(Object *obj, SInt16 regnum);
extern void Registers_BindGPR(Object *obj, SInt16 reg);
extern short gAvailableSavedFPRs;
extern SInt16 gAvailableSavedVRs;
extern void Coloring_ResetVRColors(void);
extern void Coloring_ResetFPRColors(void);
extern void Coloring_ResetGPRColors(void);
extern void Registers_SetupVRs(void);
extern void Registers_SetupFPRs(void);
extern void Registers_SetupGPRs(void);
extern VarInfo *Registers_GetInfo(Object *object);
extern void Registers_CloseCoalesceWindow(void);
extern void Registers_UpdateCoalesceWindow(void);
extern void Registers_CheckpointCoalesceWindow(void);
extern void Registers_SnapshotInitialObjectRange(void);
extern void Registers_BeginCoalesceWindow(void);
extern short Coloring_ClaimVRColor(void);
extern short Coloring_ClaimFPRColor(void);
extern short Coloring_ClaimGPRColor(void);
extern unsigned int Coloring_VRColorMask(void);
extern unsigned int Coloring_FPRColorMask(void);
extern unsigned int Coloring_GPRColorMask(void);
extern int Registers_AvailableVRs(void);
extern int Registers_AvailableFPRs(void);
extern int Registers_AvailableGPRs(void);
extern void Registers_BindVR(Object *obj, short vr);
extern void Registers_BindFPR(Object *obj, SInt16 regnum);
extern void Registers_BindGPRPair(Object *obj, SInt16 reg0, SInt16 reg1);
extern void Registers_AllocateVR(Object *obj);
extern void Registers_AllocateFPR(Object *obj);
extern void Registers_AllocateGPRPair(Object *obj);
extern void Registers_AllocateGPR(Object *obj);
extern unsigned char Registers_AreNonvolatileVRUsed(void);
extern void Registers_BindGPR(Object *obj, SInt16 reg);
extern void Registers_SetupStackBaseReg(void);
extern SInt32 fn_004c15f0(void);
extern short gFPRCoalesceLast;
extern short gFPRCounterCheckpoint;
extern short gGPRCoalesceLast;
extern short gGPRCounterCheckpoint;
extern short gInitialObjectFPRLast;
extern short gInitialObjectVRLast;
extern SInt32 gUseVirtualRegisterNumbers_00587f00;
extern short gVRCoalesceLast;
extern short gVRCounterCheckpoint;
extern void Registers_InitRegisterState(void);
extern UInt32 Registers_GetOperandRegMask(PCodeBlock *list);

#ifdef __cplusplus
}
#endif

#endif
