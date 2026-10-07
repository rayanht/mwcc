#ifndef COMPILER_SWITCH_H
#define COMPILER_SWITCH_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A run of switch case values and the label they branch to (case_ranges; Switch). */
struct CaseRange {
    CInt64 base;
    CInt64 width;
    struct PCodeLabel *info;
};
#pragma options align = mac68k
struct Scratch {
    int value;
};
#pragma options align = reset
#pragma options align = mac68k
struct SwitchCase {
    struct SwitchCase *next; /* 0x00: reconstruct_switch_info links case nodes */
    struct CLabel *label;    /* 0x04: Switch_GenerateSwitch emits case labels */
    CInt64 min;              /* 0x08: reconstruct_switch_info copies caseValue */
};
#pragma options align = reset
extern void Switch_GenerateSwitch(ENode *expression, struct SwitchInfo *cases);
extern void generate_switchtable_dispatch(ENode *node, struct SwitchInfo *cases);
extern Object *create_switchtable(void);
extern void emit_case_ranges(ENode *expr);
extern void emit_case_range_binary_tree(unsigned int firstCase, int lastCase);
extern void emit_case_range_binary_search(int a, int b);
extern int compare_switch_case_min(const void *a, const void *b);
extern void build_case_ranges(Type *type, SwitchCase *list, CLabel *defaultCase);

#ifdef __cplusplus
}
#endif

#endif
