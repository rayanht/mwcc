#ifndef COMPILER_IROUSEDEF_H
#define COMPILER_IROUSEDEF_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A definition of a variable (create_def_record, 0x1a bytes; chained from def_list). */
#pragma options align = mac68k
struct IRODef {
    int index;
    struct IROLinear *linear;
    VarRecord *var;
    struct IRODef *globalnext;
    struct IRODef *varnext;
    UInt16 useCount;
    UInt8 global;
    UInt8 noregister;
    UInt8 definite;
};
#pragma options align = reset
/* A use of a variable (build_use_def_records, 0x1e bytes; chained from allocated_uses). */
#pragma options align = mac68k
struct IROUse {
    int index;
    struct IRONode *node;
    struct IROLinear *linear;
    VarRecord *var;
    struct IROUse *globalnext;
    struct IROUse *varnext;
    struct BitVector *reachingDefs;
    UInt16 reachingDefCount;
};
#pragma options align = reset
extern void fn_00459420(void);
extern void split_variable_range(VarRecord *entry);
extern void visit_connected_defs_and_uses(IRODef *p);
extern IROLinear *fn_00459940(IROLinear *node);
extern SInt32 IRO_UseDef(UInt8 eliminateUnused, UInt8 simplifyUses);
extern Boolean propagate_inc_dec(void);
extern void add_constant_to_next_use(IROLinear *expression, CInt64 value, Type *type);
extern CInt64 get_update_delta(IROLinear *node);
extern void mark_var_used_at_call(Object *obj);
extern IROLinear *find_type_one_linear(IROLinear *e);
extern void build_use_def_records(void);
extern void create_def_record(VarRecord *var, struct IROLinear *linear, unsigned char definite);
extern struct BitVector *data_0058711c;
extern struct BitVector *data_00587174;
extern struct BitVector *data_00587f70;
extern struct BitVector *connected_defs_and_uses_bits;
extern void fn_0045ac60(IROLinear *p, int flag);

#ifdef __cplusplus
}
#endif

#endif
