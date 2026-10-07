#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation601.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/DWARF.h"
#include "compiler/MachineSimulation603.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include <string.h>

/* The instruction in each of the pipeline's six stages and the cycles it has left there: six statics in a row,
   which the code also indexes from the first. */
static struct InstructionCountdown data_00582fe8;
static struct InstructionCountdown data_00582ff0;
static struct InstructionCountdown data_00582ff8;
static struct InstructionCountdown data_00583000;
static struct InstructionCountdown data_00583008;
static struct InstructionCountdown data_00583010;

Boolean is_execution_unit_seven(int instruction)
{
    return (int)data_00578340[((PCodeInstruction *)instruction)->opcode].executionUnit == 7;
}

void advance_instruction_pipeline(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if ((&data_00582fe8)[i].instruction != NULL && (&data_00582fe8)[i].count != 0) {
            (&data_00582fe8)[i].count--;
        }
    }

    if (data_00582fe8.instruction != NULL && data_00582fe8.count == 0) {
        data_00582fe8.instruction = NULL;
    }
    if (data_00583008.instruction != NULL && data_00583008.count == 0) {
        data_00583008.instruction = NULL;
    }
    if (data_00583010.instruction != NULL && data_00583010.count == 0) {
        data_00583010.instruction = NULL;
    }
    if (data_00583000.instruction != NULL && data_00583000.count == 0 && data_00583008.instruction == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = sclass_pipeline_table[(instruction = data_00583000.instruction)->opcode * 6 + 3];
        data_00583008.instruction = instruction;
        data_00583008.count = v;
        data_00583000.instruction = NULL;
    }
    if (data_00582ff8.instruction != NULL && data_00582ff8.count == 0 && data_00583000.instruction == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = sclass_pipeline_table[(instruction = data_00582ff8.instruction)->opcode * 6 + 2];
        data_00583000.instruction = instruction;
        data_00583000.count = v;
        data_00582ff8.instruction = NULL;
    }
    if (data_00582ff0.instruction != NULL && data_00582ff0.count == 0 && data_00582ff8.instruction == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = sclass_pipeline_table[(instruction = data_00582ff0.instruction)->opcode * 6 + 1];
        data_00582ff8.instruction = instruction;
        data_00582ff8.count = v;
        data_00582ff0.instruction = NULL;
    }
}

void set_execution_unit_instruction(PCodeInstruction *instruction)
{
    unsigned int entry;
    int offset;
    int value;

    offset = instruction->opcode;
    entry = data_00578340[offset].executionUnit;
    value = sclass_pipeline_table[offset * 6];
    if (entry == 7) {
        entry = 0;
    }
    (&data_00582fe8)[entry].instruction = instruction;
    (&data_00582fe8)[entry].count = value;
}

int is_execution_unit_available(PCodeInstruction *instruction)
{
    unsigned int kind;
    struct InstructionCountdown *counts;

    kind = data_00578340[instruction->opcode].executionUnit;
    if (kind == 7)
        kind = 0;
    counts = &data_00582fe8;
    if (counts[kind].instruction != NULL)
        return 0;
    else
        return 1;
}

void clear_instruction_and_globals(void)
{
    data_00582fe8.instruction = NULL;
    data_00582ff0.instruction = NULL;
    data_00582ff8.instruction = NULL;
    data_00583000.instruction = NULL;
    data_00583008.instruction = NULL;
    data_00583010.instruction = NULL;
}

SInt32 get_latency(PCodeInstruction *p)
{
    SInt32 n = data_00578340[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}
