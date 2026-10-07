#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation7400.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/DWARF.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"
#include <string.h>

/* The instruction in each of the eighteen execution stages and the cycles it has left there: eighteen statics in a
   row, which the code also indexes from the first. */
static CountedSlot pipeline_slots;
static CountedSlot data_00582f08;
static CountedSlot data_00582f10;
static CountedSlot data_00582f18;
static CountedSlot data_00582f20;
static CountedSlot data_00582f28;
static CountedSlot data_00582f30;
static CountedSlot data_00582f38;
static CountedSlot data_00582f40;
static CountedSlot data_00582f48;
static CountedSlot data_00582f50;
static CountedSlot data_00582f58;
static CountedSlot data_00582f60;
static CountedSlot data_00582f68;
static CountedSlot data_00582f70;
static CountedSlot data_00582f78;
static CountedSlot data_00582f80;
static CountedSlot data_00582f88;
static PCodeInstruction *pipelineCompletedInstruction;
static PCodeInstruction *pipeline_completed_instruction;
static int DAT_00582f98;
static int queued_instruction_count;
static unsigned int pipeline_index;
static SInt32 simulationWriteIndex;
static CountedSlot instruction_queue[8];

int fn_0052f370(PCodeInstruction *instruction)
{
    return DAT_00577660[instruction->opcode].kind == '\n';
}

int lookup_instruction_opcode_entry(PCodeInstruction *instruction)
{
    return DAT_00577660[instruction->opcode].opcodeEntryValue;
}

static void Advance(PCodeInstruction *o, CountedSlot *next, const unsigned char *tab)
{
    int v;
    PCodeInstruction *saved = o;
    v = (int)(char)tab[o->opcode * 7];
    next->instruction = saved;
    next->status = v;
}

void advance_pipeline(void)
{
    int stageIndex;

    pipelineCompletedInstruction = NULL;
    pipeline_completed_instruction = NULL;
    for (stageIndex = 0; stageIndex < 18; stageIndex++) {
        if (((&pipeline_slots)[stageIndex].instruction != NULL) && ((&pipeline_slots)[stageIndex].status != 0)) {
            --(&pipeline_slots)[stageIndex].status;
        }
    }
    if ((queued_instruction_count != 0) && (instruction_queue[pipeline_index].status != 0)) {
        instruction_queue[pipeline_index].instruction = NULL;
        --queued_instruction_count;
        ++DAT_00582f98;
        pipeline_index = (pipeline_index + 1) & 7;
        if ((queued_instruction_count != 0) && (instruction_queue[pipeline_index].status != 0)) {
            instruction_queue[pipeline_index].instruction = NULL;
            --queued_instruction_count;
            ++DAT_00582f98;
            pipeline_index = (pipeline_index + 1) & 7;
        }
    }
    if ((data_00582f08.instruction != NULL) && (data_00582f08.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f08.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f08.instruction = NULL;
        pipelineCompletedInstruction = completedInstruction;
    }
    if ((data_00582f50.instruction != NULL) && (data_00582f50.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f50.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f50.instruction = NULL;
    }
    if ((data_00582f20.instruction != NULL) && (data_00582f20.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f20.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f20.instruction = NULL;
    }
    if ((data_00582f38.instruction != NULL) && (data_00582f38.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f38.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f38.instruction = NULL;
    }
    if ((data_00582f40.instruction != NULL) && (data_00582f40.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f40.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f40.instruction = NULL;
    }
    if ((pipeline_slots.instruction != NULL) && (pipeline_slots.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = pipeline_slots.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        pipeline_slots.instruction = NULL;
    }
    if ((data_00582f48.instruction != NULL) && (data_00582f48.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f48.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f48.instruction = NULL;
    }
    if ((data_00582f68.instruction != NULL) && (data_00582f68.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f68.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f68.instruction = NULL;
    }
    if ((data_00582f88.instruction != NULL) && (data_00582f88.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f88.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f88.instruction = NULL;
    }
    if ((data_00582f10.instruction != NULL) && (data_00582f10.status == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f10.instruction;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].status = 1;
        data_00582f10.instruction = NULL;
        pipeline_completed_instruction = completedInstruction;
    }
    {
        PCodeInstruction *instruction = data_00582f28.instruction;
        if ((instruction != NULL) && (data_00582f28.status == 0) &&
            ((instruction->opcode == PC_FDIV) || (instruction->opcode == PC_FDIVS))) {
            int slotIndex;
            PCodeInstruction *completedInstruction;
            completedInstruction = data_00582f28.instruction;
            for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instruction != completedInstruction);
                 slotIndex++) {
            }
            instruction_queue[slotIndex].status = 1;
            data_00582f28.instruction = NULL;
        }
    }
    if (((data_00582f30.instruction != NULL) && (data_00582f30.status == 0)) && (data_00582f38.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f30.instruction, &data_00582f38, &DAT_00577660[0].stage3Latency);
        data_00582f30.instruction = NULL;
    }
    if (((data_00582f28.instruction != NULL) && (data_00582f28.status == 0)) && (data_00582f30.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f28.instruction, &data_00582f30, &DAT_00577660[0].stage2Latency);
        data_00582f28.instruction = NULL;
    }
    if (((data_00582f18.instruction != NULL) && (data_00582f18.status == 0)) && (data_00582f20.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f18.instruction, &data_00582f20, &DAT_00577660[0].stage2Latency);
        data_00582f18.instruction = NULL;
    }
    if (((data_00582f60.instruction != NULL) && (data_00582f60.status == 0)) && (data_00582f68.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f60.instruction, &data_00582f68, &DAT_00577660[0].stage3Latency);
        data_00582f60.instruction = NULL;
    }
    if (((data_00582f58.instruction != NULL) && (data_00582f58.status == 0)) && (data_00582f60.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f58.instruction, &data_00582f60, &DAT_00577660[0].stage2Latency);
        data_00582f58.instruction = NULL;
    }
    if (((data_00582f80.instruction != NULL) && (data_00582f80.status == 0)) && (data_00582f88.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f80.instruction, &data_00582f88, &DAT_00577660[0].stage4Latency);
        data_00582f80.instruction = NULL;
    }
    if (((data_00582f78.instruction != NULL) && (data_00582f78.status == 0)) && (data_00582f80.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f78.instruction, &data_00582f80, &DAT_00577660[0].stage3Latency);
        data_00582f78.instruction = NULL;
    }
    if (((data_00582f70.instruction != NULL) && (data_00582f70.status == 0)) && (data_00582f78.instruction == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f70.instruction, &data_00582f78, &DAT_00577660[0].stage2Latency);
        data_00582f70.instruction = NULL;
    }
}

void queue_instruction(PCodeInstruction *obj)
{ /* signature unknown: cdecl, arguments at [esp+4], [esp+8], ... */
    SInt32 t;
    SInt32 c;

    t = DAT_00577660[obj->opcode].kind;
    c = DAT_00577660[obj->opcode].cost;

    queued_instruction_count++;
    DAT_00582f98--;
    instruction_queue[simulationWriteIndex].instruction = obj;
    instruction_queue[simulationWriteIndex].status = 0;
    simulationWriteIndex = (simulationWriteIndex + 1) & 7;
    if (t == 2 && data_00582f08.instruction == NULL) {
        t = 1;
    }
    (&pipeline_slots)[t].instruction = obj;
    (&pipeline_slots)[t].status = c;
}

int can_issue_instruction_in_pipeline_slots(PCodeInstruction *node)
{
    int primaryMissing;
    PCodeInstruction *fourth;
    PCodeInstruction *first;
    unsigned firstAbsent;
    int alternateMissing;
    PCodeInstruction *second;
    int secondMissing, thirdMissing;
    PCodeInstruction *primary;
    PCodeInstruction *third;
    PCodeInstruction *other;
    int fourthMissing;
    int kind, firstMissing;

    if (DAT_00582f98 == 0)
        return 0;
    kind = DAT_00577660[node->opcode].kind;
    if (kind == 2) {
        PCodeInstruction *alternate;
        firstAbsent = firstMissing = !(first = data_00582f08.instruction);
        alternateMissing = !(alternate = data_00582f10.instruction);
        if (!firstMissing) {
            if (!alternateMissing)
                return 0;
        }
        if (firstAbsent && alternateMissing)
            return 1;
        if (firstAbsent)
            first = alternate;
        if (Scheduler_ReturnZero(node, first, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(node, pipelineCompletedInstruction, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(node, pipeline_completed_instruction, 0) != 0)
            return 0;
    } else if (kind == 14 || kind == 9 || kind == 10 || kind == 11) {
        primaryMissing = !(primary = data_00582f50.instruction);
        secondMissing = !(second = data_00582f70.instruction);
        thirdMissing = !(third = data_00582f58.instruction);
        fourthMissing = !(fourth = data_00582f48.instruction);
        if (kind == 10) {
            if (!primaryMissing)
                return 0;
            if (secondMissing) {
                if (!thirdMissing)
                    second = third;
                else if (!fourthMissing)
                    second = fourth;
                else
                    second = NULL;
            }
            if (Scheduler_ReturnZero(node, second, 9) != 0)
                return 0;
        } else {
            if (!secondMissing || !thirdMissing || !fourthMissing)
                return 0;
            if (!primaryMissing && Scheduler_ReturnZero(node, primary, 9) != 0)
                return 0;
        }
    } else if ((&pipeline_slots)[kind].instruction != NULL)
        return 0;
    if ((node->flags & fIsWrite) != 0) {
        other = data_00582f20.instruction;
        if (other != NULL && (other->flags & fIsWrite) != 0)
            return 0;
    }
    return 1;
}

void reset_pipeline_state(void)
{
    int slot;

    for (slot = 0; slot < 18; ++slot) {
        (&pipeline_slots)[slot].instruction = NULL;
    }
    DAT_00582f98 = 8;
    queued_instruction_count = 0;
    pipeline_index = 0;
    simulationWriteIndex = 0;
    instruction_queue[0].instruction = NULL;
    instruction_queue[1].instruction = NULL;
    instruction_queue[2].instruction = NULL;
    instruction_queue[3].instruction = NULL;
    instruction_queue[4].instruction = NULL;
    instruction_queue[5].instruction = NULL;
    instruction_queue[6].instruction = NULL;
    instruction_queue[7].instruction = NULL;
    pipelineCompletedInstruction = NULL;
    pipeline_completed_instruction = NULL;
}

int get_adjusted_opcode_table_value(PCodeInstruction *record)
{
    int result;

    result = (SInt8)DAT_00577660[record->opcode].baseLatency;
    if ((record->flags & fRecordBit) != 0) {
        result = result + 2;
    }
    if ((record->opcode == PC_LMW) || (record->opcode == PC_STMW)) {
        result = result + (record->operand_count - 2);
    }
    return result;
}
