#ifndef COMPILER_CIRTRANSFORM_H
#define COMPILER_CIRTRANSFORM_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CIRTypeName {
    struct Object *object; /* 0x00: get_or_create_type_name_object caches the function object */
    SInt16 unused;         /* 0x04: CIRTransform.c never accesses this space between object and name */
    char name[64];         /* 0x06: get_or_create_type_name_object passes the spelling to GetHashNameNode */
};
extern ENode *simplify_unused_enode_values(ENode *e, UInt8 flag);
extern ENode *expand_compound_assignment(ENode *expr);
extern Object *get_or_create_type_name_object(CIRTypeName *entry, Type *ty);

#ifdef __cplusplus
}
#endif

#endif
