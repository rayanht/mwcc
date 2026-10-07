#ifndef COMPILER_SCHEDULER_H
#define COMPILER_SCHEDULER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct CColoringList {
    struct CColoringList *next;
    struct CColoringNode *owner;
    UInt16 value;
};
#pragma options align = reset
#pragma options align = mac68k
struct CColoringNode {
    struct CColoringNode *prev;
    struct CColoringNode *next;
    struct CColoringList *conflicts;
    struct PCodeInstruction *obj;
    UInt16 latency;
    UInt16 earliestCycle;
    UInt16 latestCycle;
    UInt16 height;
    UInt16 flag;
};
#pragma options align = reset
/* An instruction in one of a machine model's pipeline stages and the cycles it has left there. */
struct PipelineStage {
    struct PCodeInstruction *instr;
    SInt32 remaining;
};
/* An instruction in a machine model's completion queue, until it retires. */
struct CompletionEntry {
    struct PCodeInstruction *instr;
    SInt32 completed;
};
/* One opcode's entry in a machine model's table: its class there, and its length. */
struct MachineOpcodeInfo {
    UInt8 executionUnit;
    SInt8 latency;
    SInt8 stageCycles[4];
};
#pragma options align = mac68k
struct MachineInfo {
    SInt32 count;
    SInt32 omitRegisterAntiDependencyLatency;
    SInt32 (*getLatency)(void *);
    void (*beginScheduling)(void);
    SInt32 (*check)(void *);
    void (*issueInstruction)(void *);
    void (*advanceCycle)(void);
    SInt32 (*checkLate)(void *);
};
#pragma options align = reset
#pragma options align = mac68k
struct DependencyEntry {
    struct DependencyEntry *next;
    struct CColoringNode *owner;
    struct Object *object;
};
#pragma options align = reset

#pragma pack(push, 1)
struct ReferenceFlags {
    UInt8 reserved[0x16];
    UInt32 flags;
};
#pragma pack(pop)
#pragma options align = mac68k
struct SchedEntry {
    struct SchedEntry *next;
    struct CColoringNode *blk;
    SInt32 zero;
};
#pragma options align = reset
extern void schedule_block(PCodeBlock *function);
extern struct CColoringNode *select_ready_coloring_node(struct CColoringNode *list, UInt16 id);
extern void build_sched_dependencies(CColoringNode *list, CColoringNode *blk);
extern void add_memory_dependencies(CColoringNode *owner, int mode, int arg3);
extern void check_and_add_spill_node(CColoringNode *ownerObject, Object *object, SInt16 checkAllSpills);
extern void add_dependency(CColoringNode *owner, CColoringNode *dependency, Boolean hasLatency);
extern void init_register_owner_lists(void);
extern void fn_004cd650(void *object, Object *key, SInt16 useList74);
extern void fn_004cd7c0(int kind, CColoringNode *value, struct DependencyEntry **firstList,
                        struct DependencyEntry **secondList, int useSecondList);
extern SInt32 gVirtualRegistersActive;
extern void Scheduler_Schedule(char force);
extern struct MachineInfo machine603;
extern struct MachineInfo machine603e;
extern struct MachineInfo machine604;
extern struct MachineInfo machine750;
extern struct MachineInfo machine7400;
extern struct MachineInfo machine601;
extern struct MachineInfo machine821;
extern int Scheduler_ReturnZero(PCodeInstruction *list, PCodeInstruction *ref, char c);
struct CColoringNode;

#ifdef __cplusplus
}
#endif

#endif
