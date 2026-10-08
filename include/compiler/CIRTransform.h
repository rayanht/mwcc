#ifndef COMPILER_CIRTRANSFORM_H
#define COMPILER_CIRTRANSFORM_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CIRTypeName {
    struct Object *object;
    SInt16 unused;
    char name[64];
};
extern ENode *simplify_unused_enode_values(ENode *e, UInt8 flag);
extern ENode *expand_compound_assignment(ENode *expr);
extern Object *get_or_create_type_name_object(CIRTypeName *entry, Type *ty);

#ifdef __cplusplus
}
#endif

#endif
