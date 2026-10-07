#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation750.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"

/* The instruction in each of the nine execution stages and the cycles it has left there: nine statics in a row,
   which the code also indexes from the first. */
static struct PoolEntry data_00582e70;
static struct PoolEntry data_00582e78;
static struct PoolEntry data_00582e80;
static struct PoolEntry data_00582e88;
static struct PoolEntry data_00582e90;
static struct PoolEntry data_00582e98;
static struct PoolEntry data_00582ea0;
static struct PoolEntry data_00582ea8;
static struct PoolEntry data_00582eb0;
static struct PCodeInstruction *data_00582eb8;
static struct PCodeInstruction *data_00582ebc;
static int data_00582ec0;
static SInt32 pending_opt_args;
static UInt32 simulation_pipeline_index;
static UInt32 opt_arg_index;
static OptPair DAT_00582ed0[6];

int get_opcode_table_first_entry(PCodeInstruction *instruction)
{
    return opcode_table_entries[instruction->opcode * 6];
}

void advance_simulation_pipeline(void)
{
    SInt32 i;
    SInt32 v;

    data_00582eb8 = NULL;
    data_00582ebc = NULL;
    i = 0;
    do {
        if ((&data_00582e70)[i].obj != NULL && (&data_00582e70)[i].cnt != 0)
            (&data_00582e70)[i].cnt--;
        i++;
    } while (i < 9);
    if (pending_opt_args != 0 && DAT_00582ed0[simulation_pipeline_index].completed != 0) {
        DAT_00582ed0[simulation_pipeline_index].instruction = NULL;
        pending_opt_args--;
        data_00582ec0++;
        simulation_pipeline_index = (simulation_pipeline_index + 1) % 6;
        if (pending_opt_args != 0 && DAT_00582ed0[simulation_pipeline_index].completed != 0) {
            DAT_00582ed0[simulation_pipeline_index].instruction = NULL;
            pending_opt_args--;
            data_00582ec0++;
            simulation_pipeline_index = (simulation_pipeline_index + 1) % 6;
        }
    }
    if (data_00582e78.obj != NULL && data_00582e78.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e78.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582e78.obj = NULL;
        data_00582eb8 = q;
    }
    if (data_00582e90.obj != NULL && data_00582e90.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e90.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582e90.obj = NULL;
    }
    if (data_00582ea8.obj != NULL && data_00582ea8.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582ea8.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582ea8.obj = NULL;
    }
    if (data_00582eb0.obj != NULL && data_00582eb0.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582eb0.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582eb0.obj = NULL;
    }
    if (data_00582e70.obj != NULL && data_00582e70.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e70.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582e70.obj = NULL;
    }
    if (data_00582e80.obj != NULL && data_00582e80.cnt == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e80.obj;
        for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
            ;
        DAT_00582ed0[j].completed = 1;
        data_00582e80.obj = NULL;
        data_00582ebc = q;
    }
    {
        PCodeInstruction *p;
        SInt32 j;
        PCodeInstruction *q;
        SInt16 tag;
        if ((p = data_00582e98.obj) != NULL && data_00582e98.cnt == 0 && ((tag = p->opcode) == 0xa8 || tag == 0xa9)) {
            q = data_00582e98.obj;
            for (j = 0; j < 6 && DAT_00582ed0[j].instruction != q; j++)
                ;
            DAT_00582ed0[j].completed = 1;
            data_00582e98.obj = NULL;
        }
    }
    if (data_00582ea0.obj != NULL && data_00582ea0.cnt == 0 && data_00582ea8.obj == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582ea0.obj)->opcode].latency4;
        data_00582ea8.obj = q;
        data_00582ea8.cnt = v;
        data_00582ea0.obj = NULL;
    }
    if (data_00582e98.obj != NULL && data_00582e98.cnt == 0 && data_00582ea0.obj == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582e98.obj)->opcode].latency3;
        data_00582ea0.obj = q;
        data_00582ea0.cnt = v;
        data_00582e98.obj = NULL;
    }
    if (data_00582e88.obj != NULL && data_00582e88.cnt == 0 && data_00582e90.obj == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582e88.obj)->opcode].latency3;
        data_00582e90.obj = q;
        data_00582e90.cnt = v;
        data_00582e88.obj = NULL;
    }
}

static inline void record_opt_arg(PCodeInstruction *p)
{
    pending_opt_args++;
    data_00582ec0--;
    DAT_00582ed0[opt_arg_index].instruction = p;
    DAT_00582ed0[opt_arg_index].completed = 0;
    opt_arg_index = (opt_arg_index + 1) % 6;
}

void record_instruction_kind(PCodeInstruction *p)
{
    SInt32 kind, value;

    kind = DAT_00576f29[p->opcode - 1].kind;
    value = opcode_table_entries[p->opcode * 6 - 3];
    record_opt_arg(p);
    if (kind == 2 && data_00582e78.obj == NULL)
        kind = 1;
    (&data_00582e70)[kind].obj = p;
    (&data_00582e70)[kind].cnt = value;
}

int fn_0052f120(struct PCodeInstruction *instruction)
{
    int kind;
    PCodeInstruction *register1;
    unsigned int register1Absent;
    int register2Absent;
    int firstAbsent;
    PCodeInstruction *register2;
    struct PCodeInstruction *other;
    if (data_00582ec0 == 0)
        return 0;
    kind = data_00576f28[instruction->opcode].kind;
    if (kind == 2) {
        firstAbsent = 0;
        if ((register1 = data_00582e78.obj) == NULL)
            firstAbsent = 1;
        register1Absent = firstAbsent;
        register2Absent = 0;
        if ((register2 = data_00582e80.obj) == NULL)
            register2Absent = 1;
        if (firstAbsent == 0 && register2Absent == 0)
            return 0;
        if (register1Absent != 0 && register2Absent != 0)
            return 1;
        if (register1Absent != 0)
            register1 = register2;
        if (Scheduler_ReturnZero(instruction, register1, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(instruction, data_00582eb8, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(instruction, data_00582ebc, 0) != 0)
            return 0;
    } else if ((&data_00582e70)[kind].obj != NULL)
        return 0;
    if ((instruction->flags & PCodeInstruction_ImplicitDefinition) != 0 && (other = data_00582e90.obj) != NULL &&
        (other->flags & PCodeInstruction_ImplicitDefinition) != 0)
        return 0;
    return 1;
}

void reset_simulation_pipeline(void)
{
    int stage;

    for (stage = 0; stage < 9; ++stage) {
        (&data_00582e70)[stage].obj = NULL;
    }
    data_00582ec0 = 6;
    pending_opt_args = 0;
    simulation_pipeline_index = 0;
    opt_arg_index = 0;
    DAT_00582ed0[0].instruction = NULL;
    DAT_00582ed0[1].instruction = NULL;
    DAT_00582ed0[2].instruction = NULL;
    DAT_00582ed0[3].instruction = NULL;
    DAT_00582ed0[4].instruction = NULL;
    DAT_00582ed0[5].instruction = NULL;
    data_00582eb8 = NULL;
    data_00582ebc = NULL;
}

int fn_0052f330(PCodeInstruction *instruction)
{
    int count;
    unsigned int flags;

    flags = instruction->flags;
    count = DAT_00576f29[instruction->opcode].count;
    if ((flags & PCodeInstruction_CloneExtraOperandExcluded) != 0) {
        count += 2;
    }
    if ((instruction->opcode == PC_LMW) || (instruction->opcode == PC_STMW)) {
        count += instruction->operand_count - 2;
    }
    return count;
}
