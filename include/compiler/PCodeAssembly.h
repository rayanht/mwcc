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
    struct PCodeAssemblyEntry *next;
    struct Object *object;
    struct PCodeBlock *block;
};
extern int PCodeAssembly_EmitFunction(Object *object, struct PCodeAssemblyEntry *entries);
extern int optimize_branches(int arg);

#ifdef __cplusplus
}
#endif

#endif
