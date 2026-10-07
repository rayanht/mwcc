#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation604.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/DWARF.h"
#include "compiler/MachineSimulation601.h"
#include "compiler/MachineSimulation603.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"

/* The instruction in each of the nine execution stages and the cycles it has left there: nine statics in a row,
   which the code also indexes from the first. */
static InstructionValueEntry execution_unit_instructions;
static InstructionValueEntry data_00582d98;
static InstructionValueEntry data_00582da0;
static InstructionValueEntry data_00582da8;
static InstructionValueEntry data_00582db0;
static InstructionValueEntry data_00582db8;
static InstructionValueEntry data_00582dc0;
static InstructionValueEntry data_00582dc8;
static InstructionValueEntry data_00582dd0;
static PCodeInstruction *data_00582dd8;
static PCodeInstruction *data_00582ddc;
static int data_00582de0;
static SInt32 DAT_00582de4;
static SInt32 instruction_retire_index;
static unsigned int next_instruction_slot;
static InstructionValueEntry instruction_ring[16];

int get_opcode_table_value(PCodeInstruction *instruction)
{
    return opcode_table_values[instruction->opcode * 6];
}

void advance_instruction_stages_and_retire(void)
{
    SInt32 count;
    SInt32 slotIndex;
    SInt32 retiredCount;

    data_00582dd8 = NULL;
    data_00582ddc = NULL;

    slotIndex = 0;
    do {
        if ((&execution_unit_instructions)[slotIndex].instruction != NULL &&
            (&execution_unit_instructions)[slotIndex].value != 0)
            (&execution_unit_instructions)[slotIndex].value--;
        slotIndex++;
    } while (slotIndex < 9);

    retiredCount = 0;
    do {
        if (DAT_00582de4 == 0)
            break;
        if (instruction_ring[instruction_retire_index].value == 0)
            break;
        instruction_ring[instruction_retire_index].instruction = NULL;
        DAT_00582de4--;
        data_00582de0++;
        instruction_retire_index = (instruction_retire_index + 1) & 0xF;
        retiredCount++;
    } while (retiredCount < 5);

    if (execution_unit_instructions.instruction != NULL && execution_unit_instructions.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = execution_unit_instructions.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        execution_unit_instructions.instruction = NULL;
        data_00582dd8 = object;
    }

    if (data_00582d98.instruction != NULL && data_00582d98.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582d98.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        data_00582d98.instruction = NULL;
        data_00582ddc = object;
    }

    if (data_00582da0.instruction != NULL && data_00582da0.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582da0.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        data_00582da0.instruction = NULL;
    }

    if (data_00582dc8.instruction != NULL && data_00582dc8.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582dc8.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        data_00582dc8.instruction = NULL;
    }

    if (data_00582db8.instruction != NULL && data_00582db8.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582db8.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        data_00582db8.instruction = NULL;
    }

    if (data_00582dd0.instruction != NULL && data_00582dd0.value == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582dd0.instruction;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instruction != object)
            ringIndex++;
        instruction_ring[ringIndex].value = 1;
        data_00582dd0.instruction = NULL;
    }

    {
        SInt32 ringIndex;
        PCodeInstruction *object;
        if ((object = data_00582da8.instruction) != NULL && data_00582da8.value == 0 &&
            (object->opcode == 0xa8 || object->opcode == 0xa9)) {
            PCodeInstruction *slotObject;
            slotObject = data_00582da8.instruction;
            ringIndex = 0;
            while (ringIndex < 16 && instruction_ring[ringIndex].instruction != slotObject)
                ringIndex++;
            instruction_ring[ringIndex].value = 1;
            data_00582da8.instruction = NULL;
        }
    }

    if (data_00582db0.instruction != NULL && data_00582db0.value == 0 && data_00582db8.instruction == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = opcode_sclass_info[(object = data_00582db0.instruction)->opcode].thirdStageCycles;
        data_00582db8.instruction = object;
        data_00582db8.value = count;
        data_00582db0.instruction = NULL;
    }

    if (data_00582da8.instruction != NULL && data_00582da8.value == 0 && data_00582db0.instruction == NULL) {
        PCodeInstruction *object;
        count = opcode_sclass_info[(object = data_00582da8.instruction)->opcode].secondStageCycles;
        data_00582db0.instruction = object;
        data_00582db0.value = count;
        data_00582da8.instruction = NULL;
    }

    if (data_00582dc0.instruction != NULL && data_00582dc0.value == 0 && data_00582dc8.instruction == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = opcode_sclass_info[(object = data_00582dc0.instruction)->opcode].secondStageCycles;
        data_00582dc8.instruction = object;
        data_00582dc8.value = count;
        data_00582dc0.instruction = NULL;
    }
}

void assign_instruction_to_execution_unit(struct PCodeInstruction *instruction)
{
    unsigned int index;
    int value;
    unsigned int opcode = instruction->opcode;
    index = machineOpcodeInfo604[opcode].executionUnit;
    value = opcode_sclass_info[opcode].initialStageCycles;
    if ((index == 0) && (execution_unit_instructions.instruction != NULL)) {
        index = 1;
    }
    DAT_00582de4 = 1 + DAT_00582de4;
    data_00582de0 = data_00582de0 + -1;
    instruction_ring[next_instruction_slot].instruction = instruction;
    instruction_ring[next_instruction_slot].value = 0;
    next_instruction_slot = next_instruction_slot + 1 & 0xf;
    (&execution_unit_instructions)[index].instruction = instruction;
    (&execution_unit_instructions)[index].value = value;
}

static void ZeroArray(InstructionValueEntry *arr, SInt32 n)
{
    SInt32 i;

    for (i = 0; i < n; i++)
        arr[i].instruction = NULL;
}

static void ZeroInstructions(InstructionValueEntry *arr, SInt32 n)
{
    SInt32 i;
    for (i = 0; i < n; i++)
        arr[i].instruction = NULL;
}

int can_issue_instruction(struct PCodeInstruction *instruction)
{
    unsigned int category;
    PCodeInstruction *candidate;
    PCodeInstruction *ref;
    int firstMissing;
    int secondMissing;
    int noFirst;
    int enabled;
    category = machineOpcodeInfo604[(int)instruction->opcode].executionUnit;
    enabled = data_00582de0;
    if (enabled == 0)
        return 0;
    if (category == 0) {
        firstMissing = noFirst = !execution_unit_instructions.instruction;
        secondMissing = 0;
        if ((candidate = data_00582d98.instruction) == NULL)
            secondMissing = 1;
        if (noFirst == 0 && secondMissing == 0)
            return 0;
        if (firstMissing != 0 && secondMissing != 0)
            return 1;
        if (firstMissing == 0)
            candidate = execution_unit_instructions.instruction;
        if (Scheduler_ReturnZero(instruction, candidate, 0) != 0)
            return 0;
        ref = data_00582dd8;
        if (Scheduler_ReturnZero(instruction, ref, 0) != 0)
            return 0;
        ref = data_00582ddc;
        if (Scheduler_ReturnZero(instruction, ref, 0) != 0)
            return 0;
    } else if ((&execution_unit_instructions)[category].instruction != NULL) {
        return 0;
    }
    return 1;
}

void fn_0052eb60(void)
{
    ZeroInstructions(&execution_unit_instructions, 9);
    data_00582de0 = 0x10;
    DAT_00582de4 = 0;
    instruction_retire_index = 0;
    next_instruction_slot = 0;
    ZeroInstructions(instruction_ring, 16);
    data_00582dd8 = NULL;
    data_00582ddc = NULL;
}

SInt32 get_size_rec_latency(PCodeInstruction *p)
{
    SInt32 n = machineOpcodeInfo604[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}
