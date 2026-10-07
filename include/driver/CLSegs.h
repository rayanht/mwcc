#ifndef DRIVER_CLSEGS_H
#define DRIVER_CLSEGS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Segments {
    unsigned int *values;
    unsigned short capacity;
    unsigned short count;
};
struct PayloadWithValue {
    char name[32];
    unsigned short value;
};
extern Boolean allocate_access_path_value_index(Segments *table, UInt16 *index);
extern struct PayloadWithValue *CLSegs_GetValue(struct Segments *table, unsigned int index);
extern unsigned short CLSegs_GetCount(struct Segments *table);
extern Boolean CLSegs_AddValue(Segments *table, struct PayloadWithValue *value, UInt16 *index);
extern Boolean CLSegs_InitSegments(Segments *segments);
extern unsigned char CLSegs_FreeValues(Segments *array);
extern struct PayloadWithValue *CLSegs_CreatePayloadWithValue(const char *source, UInt16 value);
extern void free_if_not_null(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
