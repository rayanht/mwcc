#ifndef COMPILER_IROEVAL_H
#define COMPILER_IROEVAL_H

#include "compiler/common.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct CompareCase {
    SInt32 state;
    SInt32 flag;
    struct Object *key;
    struct IRONode *node;
    struct CompareCase *link;
    CInt64 val;
};
#pragma options align = reset
#pragma options align = mac68k
union Val {
    long long ll;
    double d;
    Float f;
    struct {
        UInt32 lo;
        UInt32 hi;
    } w;
};
#pragma options align = reset
extern int IRO_ConstantFolding(void);
extern void convert_cint64_to_bitfield(CInt64 *val, Type *type, TypeBitfield *type2);
extern int IRO_EvaluateConditionals(void);
extern Type *get_unsigned_type(Type *type);
extern int fn_00454bb0(void);
extern unsigned int fn_00454c30(IRONode *first, IRONode *limit);
extern SInt32 group_adjacent_compare_cases(IRONode *first, IRONode *last);
extern int fn_00454f80(CompareCase *list, CInt64 value);
extern SInt32 mark_adjacent_compare_cases(CompareCase *p1, CompareCase *p2, CInt64 *p3);
extern int fn_00455350(IRONode *left, IRONode *right);
extern int get_matching_cond_objref(Object *expectedResult, IROLinear *cond);
extern SInt32 has_label_successor(void *id, IRONode *node);
extern int starts_with_branch_cond(IRONode *node);
extern IROLinear *find_leftmost_leaf(IROLinear *p);

#ifdef __cplusplus
}
#endif

#endif
