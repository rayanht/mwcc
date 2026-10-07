#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation821.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LiveVariables.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"

#include <string.h>

/* The instruction in each of the six execution stages and the cycles it has left there: six statics in a row,
   which the code also indexes from the first. */
static struct StageSlot data_00583018;
static struct StageSlot data_00583020;
static struct StageSlot data_00583028;
static struct StageSlot data_00583030;
static struct StageSlot data_00583038;
static struct StageSlot data_00583040;
static int data_00583048;
static UInt32 data_0058304c;
static UInt32 data_00583050;
static unsigned int record_enqueue_index;
static QueueSlot queue_slots[6];

/* 0x5842e1, byte access */

/* PCodeBlock: the object a liveness entry is indexed by; its block number
 * lives at 0x1c. */

/* Per-block liveness record: four bit vectors. */

/* TYPESTRUCT record with the byte classification field at 0x0e. */
int get_instruction_opcode_table_entry(PCodeInstruction *instruction)
{
    return DAT_00578e55[instruction->opcode * 6];
}

void fn_00530660(void)
{
    SInt32 i;
    i = 0;
    do {
        if ((&data_00583018)[i].instruction != NULL && (&data_00583018)[i].count != 0)
            (&data_00583018)[i].count--;
        i++;
    } while (i < 6);
    if (data_0058304c > 0 && queue_slots[data_00583050].flag != 0) {
        queue_slots[data_00583050].obj = NULL;
        data_0058304c--;
        data_00583048++;
        data_00583050 = (data_00583050 + 1) % 6;
        if (data_0058304c > 0 && queue_slots[data_00583050].flag != 0) {
            queue_slots[data_00583050].obj = NULL;
            data_0058304c--;
            data_00583048++;
            data_00583050 = (1 + data_00583050) % 6;
        }
    }
    if (data_00583020.instruction != NULL && data_00583020.count == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583020.instruction;
        for (i = 0; i < 6 && queue_slots[i].obj != key; i++)
            ;
        queue_slots[i].flag = 1;
        data_00583020.instruction = NULL;
    }
    if (data_00583038.instruction != NULL && data_00583038.count == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583038.instruction;
        for (i = 0; i < 6 && queue_slots[i].obj != key; i++)
            ;
        queue_slots[i].flag = 1;
        data_00583038.instruction = NULL;
    }
    if (data_00583018.instruction != NULL && data_00583018.count == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583018.instruction;
        for (i = 0; i < 6 && queue_slots[i].obj != key; i++)
            ;
        queue_slots[i].flag = 1;
        data_00583018.instruction = NULL;
    }
    if (data_00583030.instruction != NULL && data_00583030.count == 0 && data_00583038.instruction == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = data_00578e53[(object = data_00583030.instruction)->opcode * 6];
        data_00583038.instruction = object;
        data_00583038.count = count;
        data_00583030.instruction = NULL;
    }
}

static inline void EnqueueRecord(IndexedRecord *record)
{
    queue_slots[record_enqueue_index].obj = (struct PCodeInstruction *)record;
    queue_slots[record_enqueue_index].flag = 0;
    record_enqueue_index = (record_enqueue_index + 1) % 6;
}

void fn_00530830(IndexedRecord *record)
{
    int slot;
    int tableOffset;

    tableOffset = record->index * 6;
    slot = DAT_00578e50[tableOffset];

    data_0058304c = data_0058304c + 1;
    data_00583048 = data_00583048 - 1;
    EnqueueRecord(record);
    (&data_00583018)[slot].instruction = (struct PCodeInstruction *)record;
    (&data_00583018)[slot].count = DAT_00578e52[tableOffset];
}

int fn_005308b0(struct PCodeInstruction *pcode)
{
    struct PCodeInstruction *other;
    if (data_00583048 == 0)
        return 0;
    if ((&data_00583018)[DAT_00578e50[pcode->opcode * 6]].instruction != NULL)
        return 0;
    if ((pcode->flags & fIsWrite) != 0) {
        other = data_00583038.instruction;
        if (other != NULL && (other->flags & fIsWrite) != 0)
            return 0;
    }
    return 1;
}

void reset_spill_state(void)
{
    data_00583018.instruction = NULL;
    data_00583020.instruction = NULL;
    data_00583028.instruction = NULL;
    data_00583030.instruction = NULL;
    data_00583038.instruction = NULL;
    data_00583040.instruction = NULL;
    data_00583048 = 6;
    data_0058304c = 0;
    data_00583050 = 0;
    record_enqueue_index = 0;
    queue_slots[0].obj = NULL;
    queue_slots[1].obj = NULL;
    queue_slots[2].obj = NULL;
    queue_slots[3].obj = NULL;
    queue_slots[4].obj = NULL;
    queue_slots[5].obj = NULL;
}

int get_instruction_cost(PCodeInstruction *instruction)
{
    int cost = instruction_costs[instruction->opcode * 6];

    if (instruction->flags & fRecordBit) {
        cost += 2;
    }
    if (instruction->opcode == PC_LMW || instruction->opcode == PC_STMW) {
        cost += instruction->operand_count - 2;
    }
    return cost;
}
