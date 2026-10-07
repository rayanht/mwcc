#ifndef COMPILER_STACKFRAMEEABI_H
#define COMPILER_STACKFRAMEEABI_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct TB {
    UInt32 zero;
    UInt8 version, lang;
    unsigned char a0 : 2, a2 : 1, a3 : 5;
    unsigned char b0 : 1, b1 : 1, b2 : 1, b3 : 3, b6 : 1, b7 : 1;
    unsigned char c0 : 1, c1 : 1, c2 : 6;
    unsigned char d0 : 1, d1 : 1, d2 : 6;
    UInt16 reserved;
};
#pragma options align = reset
#pragma options align = mac68k
struct VX {
    unsigned char regs : 6, opt : 1, var : 1;
    unsigned char count : 7, other : 1;
};
#pragma options align = reset
extern void fn_004aa580(void);
extern SInt32 StackFrameEABI_GetRecordSize(Object *object);
extern void StackFrameEABI_SaveArgumentRegisters(PCodeBlock *entryBlock, PCodeBlock *fpSaveBlock,
                                                 PCodeBlock *gpSaveBlock, PCodeLabel *operand);
extern void emit_stack_frame_allocation(int arg);
extern void StackFrameEABI_ClearUnusedStackFrame(void);
extern void generate_interrupt_register_saves(void);
extern void emit_restore_special_registers(SInt16 frameRegister);
extern void save_and_update_vrsave(int a, int argb);
extern void StackFrameEABI_004aa7d0(void);
extern int emit_lwz_register_restores(short a0);
extern SInt32 StackFrameEABI_GetTypeAlignment(Type *type);
extern void *StackFrameEABI_004aabb0(UInt32 value, char *name, SInt32 *outSize, Object *func);
extern void StackFrameEABI_EmitFrameAllocation(char allocateFrame, short scratchReg, int frameSize);
extern void restore_gprs(PCodeBlock *func, Boolean a, Boolean b, SInt16 c);
extern void save_gprs(PCodeBlock *func, Boolean a, Boolean b);
extern void restore_vrs(PCodeBlock *block);
extern void emit_restore_fprs(PCodeBlock *func, Boolean flag);
extern void emit_vr_saves(PCodeBlock *block);
extern void emit_save_fprs(PCodeBlock *block, Boolean savefpr);
extern void StackFrameEABI_GeneratePrologueEpilogue(PCodeBlock *block, int arg2, int arg3);
extern void StackFrameEABI_MergePrologueEpilogue(PCodeBlock *blk, char flag);
extern SInt32 stack_frame_size;
extern SInt32 stack_frame_adjustment;
extern int stack_frame_padding;
extern SInt32 eabi_stack_frame_size;
extern SInt32 data_00587634;
extern SInt32 data_00587638;
extern int data_0058764c;
extern SInt32 special_register_save_offset;
extern SInt32 frame_alignment_padding;
extern SInt32 r12_save_offset;
extern int interrupt_register_save_mask;
extern UInt32 vrsave_mask;
extern SInt32 data_00588070;
extern int data_005880d8;
extern SInt32 data_00588204;
extern union StackFrameInterruptValue {
    struct InterruptGenerationRecord *record;
    SInt32 value;
} data_005882c0;
extern SInt16 data_005883ee;
extern UInt8 data_0058852d;
extern short gFPRSaveSpan;
extern SInt16 gGPRSaveSpan;
extern UInt8 gHasAltivecFrame;
extern short gVRSaveSpan;
extern void StackFrameEABI_AllocateObjectSlot(Object *object);
extern void StackFrameEABI_Initialize(void);
extern void StackFrameEABI_FinalizeLayout(struct PCodeBlock *function);
extern int outgoing_argument_size;
extern int data_005876a8;
extern SInt32 gStackFrameSize;
extern unsigned int data_005880cc;
extern unsigned long data_00587e40;
extern void StackFrame_CheckAltivec(void);
extern void fn_004a9c70(void);
extern int fn_004a9f70(int value);
extern unsigned int fn_004a9f90(void);
extern UInt8 data_005884ff;
extern UInt8 data_00588521;

#ifdef __cplusplus
}
#endif

#endif
