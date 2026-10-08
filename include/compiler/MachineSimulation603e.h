#ifndef COMPILER_MACHINESIMULATION603E_H
#define COMPILER_MACHINESIMULATION603E_H

#include "compiler/common.h"
#include "compiler/MachineSimulation603.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void fn_0052e110(void);
extern int get_instruction_opcode_table_value(PCodeInstruction *instruction);
extern void fn_0052e590(void);
extern int is_instruction_issuable(PCodeInstruction *instr);
extern void fn_0052e450(struct PCodeInstruction *instruction);
extern SInt32 fn_0052e640(struct PCodeInstruction *p);

#ifdef __cplusplus
}
#endif

#endif
