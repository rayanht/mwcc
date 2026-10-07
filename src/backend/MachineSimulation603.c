#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation603.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CExpr2.h"
#include "compiler/CTemplateNew.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/MachineSimulation601.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include <string.h>

/* The instruction in each of the eight execution stages and the cycles it has left there: eight statics in a row,
   which the code also indexes from the first. */
static struct PipelineStage data_00582ca0;
static struct PipelineStage data_00582ca8;
static struct PipelineStage data_00582cb0;
static struct PipelineStage data_00582cb8;
static struct PipelineStage data_00582cc0;
static struct PipelineStage data_00582cc8;
static struct PipelineStage data_00582cd0;
static struct PipelineStage data_00582cd8;
static int data_00582ce0;
static SInt32 pending_instruction_count;
static UInt32 pending_instruction_retire_index;
static unsigned int execution_unit_entry_index;
static struct {
    short *entry;
    int value;
} data_00582cf0[5];

int get_opcode_table_entry(PCodeInstruction *instruction)
{
    return opcode_table[instruction->opcode * 6];
}

static void RemovePending(const void *obj)
{
    SInt32 i;

    for (i = 0; i < 5 && data_00582cf0[i].entry != obj; i++)
        ;
    data_00582cf0[i].value = 1;
}

void retire_and_advance_pending_instructions(void)
{
    Object *obj0;
    SInt32 i;
    PCodeInstruction *o;

    i = 0;
    do {
        if ((&data_00582ca0)[i].instruction != NULL && (&data_00582ca0)[i].value != 0)
            --(&data_00582ca0)[i].value;
        i = i + 1;
    } while (i < 8);

    if (pending_instruction_count != 0 && data_00582cf0[pending_instruction_retire_index].value != 0) {
        data_00582cf0[pending_instruction_retire_index].entry = NULL;
        pending_instruction_count = pending_instruction_count - 1;
        ++data_00582ce0;
        pending_instruction_retire_index = (pending_instruction_retire_index + 1U) % 5U;
        if (pending_instruction_count != 0 && data_00582cf0[pending_instruction_retire_index].value != 0) {
            data_00582cf0[pending_instruction_retire_index].entry = NULL;
            pending_instruction_count = pending_instruction_count - 1;
            data_00582ce0 = data_00582ce0 + 1;
            pending_instruction_retire_index = (pending_instruction_retire_index + 1U) % 5U;
        }
    }

    if (data_00582ca8.instruction != NULL && data_00582ca8.value == 0) {
        RemovePending(data_00582ca8.instruction);
        data_00582ca8.instruction = NULL;
    }

    if (data_00582cb8.instruction != NULL && data_00582cb8.value == 0) {
        RemovePending(data_00582cb8.instruction);
        data_00582cb8.instruction = NULL;
    }

    if (data_00582cd0.instruction != NULL && data_00582cd0.value == 0) {
        RemovePending(data_00582cd0.instruction);
        data_00582cd0.instruction = NULL;
    }

    if (data_00582cd8.instruction != NULL && data_00582cd8.value == 0) {
        RemovePending(data_00582cd8.instruction);
        data_00582cd8.instruction = NULL;
    }

    if (data_00582ca0.instruction != NULL && data_00582ca0.value == 0) {
        RemovePending(data_00582ca0.instruction);
        data_00582ca0.instruction = NULL;
    }

    if ((o = data_00582cc0.instruction) != NULL && data_00582cc0.value == 0 &&
        (o->opcode == PC_FDIV || o->opcode == PC_FDIVS)) {
        RemovePending(data_00582cc0.instruction);
        data_00582cc0.instruction = NULL;
    }

    if (data_00582cc8.instruction != NULL && data_00582cc8.value == 0 && data_00582cd0.instruction == NULL) {
        SInt32 v = 0;
        v = opcode_simulation_table[(o = data_00582cc8.instruction)->opcode * 6 + 2];
        data_00582cd0.instruction = o;
        data_00582cd0.value = v;
        data_00582cc8.instruction = NULL;
    }

    if (data_00582cc0.instruction != NULL && data_00582cc0.value == 0 && data_00582cc8.instruction == NULL) {
        SInt32 v = 0;
        v = opcode_simulation_table[(o = data_00582cc0.instruction)->opcode * 6 + 1];
        data_00582cc8.instruction = o;
        data_00582cc8.value = v;
        data_00582cc0.instruction = NULL;
    }

    if (data_00582cb0.instruction != NULL && data_00582cb0.value == 0 && data_00582cb8.instruction == NULL) {
        SInt32 v = 0;
        v = opcode_simulation_table[(o = data_00582cb0.instruction)->opcode * 6 + 1];
        data_00582cb8.instruction = o;
        data_00582cb8.value = v;
        data_00582cb0.instruction = NULL;
    }
}

void assign_entry_to_execution_unit(short *entry)
{
    int category;
    int tableIndex;

    tableIndex = entry[10];
    category = machine_opcode_info[tableIndex].executionUnit;
    ++pending_instruction_count;
    --data_00582ce0;
    data_00582cf0[execution_unit_entry_index].value = (data_00582cf0[execution_unit_entry_index].entry = entry, 0);
    execution_unit_entry_index = (execution_unit_entry_index + 1) % 5;
    (&data_00582ca0)[category].instruction = (PCodeInstruction *)entry;
    (&data_00582ca0)[category].value = opcode_simulation_table[tableIndex * 6];
}

int fn_0052dfa0(PCodeInstruction *instruction)
{
    PCodeInstruction *previousInstruction;
    if (data_00582ce0 == 0) {
        return 0;
    }
    if ((&data_00582ca0)[machine_opcode_info[instruction->opcode].executionUnit].instruction != NULL) {
        return 0;
    }
    if ((instruction->flags & PCodeInstruction_ImplicitDefinition) != 0) {
        previousInstruction = data_00582cb8.instruction;
        if (previousInstruction != NULL && (previousInstruction->flags & PCodeInstruction_ImplicitDefinition) != 0) {
            return 0;
        }
    }
    return 1;
}

void fn_0052e000(void)
{
    data_00582ca0.instruction = NULL;
    data_00582ca8.instruction = NULL;
    data_00582cb0.instruction = NULL;
    data_00582cb8.instruction = NULL;
    data_00582cc0.instruction = NULL;
    data_00582cc8.instruction = NULL;
    data_00582cd0.instruction = NULL;
    data_00582cd8.instruction = NULL;
    data_00582ce0 = 5;
    pending_instruction_count = 0;
    pending_instruction_retire_index = 0;
    execution_unit_entry_index = 0;
    data_00582cf0[0].entry = NULL;
    data_00582cf0[1].entry = NULL;
    data_00582cf0[2].entry = NULL;
    data_00582cf0[3].entry = NULL;
    data_00582cf0[4].entry = NULL;
}

SInt32 get_adjusted_latency(PCodeInstruction *p)
{
    SInt32 n = machine_opcode_info[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}
