#ifndef COMPILER_PCODEASSEMBLY_H
#define COMPILER_PCODEASSEMBLY_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
union OutputRelocation {
    struct {
        short kind;
        struct Object *object;
        unsigned int options;
    } record;
    int flags;
};
#pragma options align = reset
#pragma options align = mac68k
struct WeirdOperand {
    SInt16 type;
    struct Object *object;
    SInt32 addend;
};
#pragma options align = reset
struct PCodeInstruction;
struct PCodeInstruction;
extern void expand_out_of_range_conditional_branches(void);
extern UInt32 encode_assembly_instruction(PCodeInstruction *instr, UInt32 offset, WeirdOperand *wop);
struct PCodeAssemblyEntry {
    struct PCodeAssemblyEntry *next; /* 0x00: PCodeAssembly_EmitFunction walks the entries */
    struct Object *object; /* 0x04: PCodeAssembly_EmitFunction passes the object to ObjGen_PPC_EABI_SetSymbolOffset */
    struct PCodeBlock *block; /* 0x08: PCodeAssembly_EmitFunction reads block->code_offset */
};
extern int PCodeAssembly_EmitFunction(Object *object, struct PCodeAssemblyEntry *entries);
extern int optimize_branches(int arg);

#ifdef __cplusplus
}
#endif

#endif
