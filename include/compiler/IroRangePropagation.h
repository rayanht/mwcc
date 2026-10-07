#ifndef COMPILER_IRORANGEPROPAGATION_H
#define COMPILER_IRORANGEPROPAGATION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ERangeVar {
    struct Object *object; /* 0x00: IroRangePropagation_PropagateRangeInLinear passes objref to FindVar and AddVar */
    struct ERange
        *range; /* 0x04: AddVar stores the object's range; IroRangePropagation_PropagateRangeInLinear updates it */
    struct ERangeVar *next; /* 0x08: AddVar links range_vars; FindVar traverses it */
};
#pragma options align = reset
extern int IroRangePropagation_PropagateRangeInLinear(struct IROLinear *nd);
extern int initialize_node_range(IROLinear *record);
extern void check_range_for_type(ERange *p, Type *type);
extern CInt64 signed_char_max;
extern CInt64 type_range_minimum;
extern CInt64 data_005539c8;
extern CInt64 int16_max;
extern CInt64 data_005539d8;
extern CInt64 data_005539e0;
extern CInt64 int32_max;
extern CInt64 type_range_lower_bound;
extern CInt64 data_00553a10;
extern CInt64 range_int32_max;
extern CInt64 type_range_min;
extern CInt64 data_00553a28;
extern struct ERangeVar *first_range_var;

#ifdef __cplusplus
}
#endif

#endif
