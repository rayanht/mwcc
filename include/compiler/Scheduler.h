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
/* One opcode's entry in a machine model's table: its class there, and its length. */
struct MachineOpcodeInfo {
    UInt8 executionUnit;
    SInt8 latency;
    SInt8 stageCycles[4];
};
#pragma options align = mac68k
struct Checker {
    SInt32 count; /* 0x00: schedule_block limits issue slots */
    SInt32
        omitRegisterAntiDependencyLatency; /* 0x04: fn_004cd7c0 tests this flag to suppress register anti-dependency latency */
    SInt32 (*getLatency)(void *);          /* 0x08: Scheduler.c initializes node height */
    void (*beginScheduling)(void);         /* 0x0c: schedule_block initializes simulation */
    SInt32 (*check)(void *);               /* 0x10: select_ready_coloring_node tests issue eligibility */
    void (*issueInstruction)(void *); /* 0x14: schedule_block issues selected instruction */
    void (*advanceCycle)(void);       /* 0x18: schedule_block advances simulation */
    SInt32 (*checkLate)(void *);      /* 0x1c: Scheduler.c tests late instruction eligibility */
};
#pragma options align = reset
#pragma options align = mac68k
struct DependencyEntry {
    struct DependencyEntry *next; /* 0x00: fn_004cd7c0 links and traverses register owner lists */
    struct CColoringNode *owner;  /* 0x04: fn_004cd7c0 adds dependencies on this owner */
    struct Object *object;        /* 0x08: fn_004cd7c0 initializes register entries to NULL */
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
extern void *data_00581b7c;
extern struct CColoringNode *select_ready_coloring_node(struct CColoringNode *list, UInt16 id);
extern void build_sched_dependencies(CColoringNode *list, CColoringNode *blk);
extern void add_memory_dependencies(CColoringNode *owner, int mode, int arg3);
extern void check_and_add_spill_node(CColoringNode *ownerObject, Object *object, SInt16 checkAllSpills);
extern void add_dependency(CColoringNode *owner, CColoringNode *dependency, Boolean hasLatency);
extern void init_register_owner_lists(void);
extern void fn_004cd650(void *object, Object *key, SInt16 useList74);
extern void fn_004cd7c0(int kind, CColoringNode *value, struct DependencyEntry **firstList,
                        struct DependencyEntry **secondList, int useSecondList);
extern int DAT_00581b1c;
extern int DAT_00581b20;
extern int DAT_00581b28;
extern int DAT_00581b2c;
extern int DAT_00581b34;
extern int DAT_00581b38;
extern int DAT_00581b3c;
extern int DAT_00581b40;
extern int DAT_00581b44;
extern int DAT_00581b48;
extern int DAT_00581b4c;
extern int DAT_00581b54;
extern int DAT_00581b58;
extern int DAT_00581b5c;
extern int DAT_00581b60;
extern int DAT_00581b64;
extern int DAT_00581b68;
extern int DAT_00581b6c;
extern struct DependencyEntry *memory_dependency_list;
extern struct DependencyEntry *dependency_entry_list;
extern struct DependencyEntry **data_00581b00;
extern struct DependencyEntry **gpr_owner_lists;
extern struct DependencyEntry **data_00581b08;
extern struct DependencyEntry **fpr_owner_lists;
extern struct DependencyEntry **virtual_register_owner_lists;
extern struct DependencyEntry **register_owner_lists;
extern struct DependencyEntry *data_00581b18[3];
extern struct DependencyEntry *data_00581b24[3];
extern struct DependencyEntry *data_00581b30[8];
extern struct DependencyEntry *data_00581b50[8];
extern struct SchedEntry *sched_entry_list;
extern UInt16 max_height;
extern SInt32 gVirtualRegistersActive;
extern void Scheduler_Schedule(char force);
extern struct Checker machine603;
extern struct Checker machine603e;
extern struct Checker machine604;
extern struct Checker machine750;
extern struct Checker machine7400;
extern struct Checker machine601;
extern struct Checker machine821;
extern struct Checker *data_00581b80;
extern int Scheduler_ReturnZero(PCodeInstruction *list, PCodeInstruction *ref, char c);
struct CColoringNode;

#ifdef __cplusplus
}
#endif

#endif
