#include "compiler/common.h"
#include "driver/CLSegs.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/MemUtils.h"
#include <stdlib.h>
#include <string.h>

struct PayloadWithValue *CLSegs_CreatePayloadWithValue(const char *source, UInt16 value)
{
    struct PayloadWithValue *record;
    record = xmalloc(NULL, sizeof(*record));
    strncpy(record->name, source, sizeof(record->name));
    record->name[sizeof(record->name) - 1] = 0;
    record->value = value;
    return record;
}

void free_if_not_null(void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

Boolean CLSegs_InitSegments(AccessPathValueTable *segments)
{
    unsigned short segmentIndex;
    struct PayloadWithValue *segment;
    if (segments == NULL) {
        CLIO_ReportAssertionFailure("segs != NULL", "CLSegs.c", 34U);
    }
    memset(segments, 0, 8U);
    segments->values = NULL;
    segment = CLSegs_CreatePayloadWithValue("Jump Table", 40U);
    CLSegs_AddValue(segments, segment, &segmentIndex);
    if (segmentIndex != 0U) {
        CLIO_ReportAssertionFailure("idx==0", "CLSegs.c", 42U);
    }
    segment = CLSegs_CreatePayloadWithValue("Main", 65535U);
    CLSegs_AddValue(segments, segment, &segmentIndex);
    if (segmentIndex != 1U) {
        CLIO_ReportAssertionFailure("idx==1", "CLSegs.c", 47U);
    }
    return 1;
}

unsigned char CLSegs_FreeValues(AccessPathValueTable *array)
{
    unsigned short index;
    if (array == NULL)
        CLIO_ReportAssertionFailure("segs != NULL", "CLSegs.c", 55U);
    if (array->values != NULL) {
        index = 0;
        while (index < array->count) {
            struct PayloadWithValue **entries = (struct PayloadWithValue **)array->values;
            free_if_not_null(entries[index]);
            index++;
        }
        free(array->values);
    }
    array->values = NULL;
    return 1;
}

Boolean allocate_access_path_value_index(AccessPathValueTable *table, UInt16 *index)
{
    if (table == NULL) {
        CLIO_ReportAssertionFailure("segs != NULL", "CLSegs.c", 76U);
    }
    if (table->count >= table->capacity) {
        UInt16 capacity;
        unsigned int *entries;
        table->capacity += 20U;
        capacity = table->capacity;
        entries = (unsigned int *)xrealloc("segments", table->values, capacity * sizeof(*entries));
        table->values = entries;
    }
    {
        UInt16 entryIndex;
        entryIndex = table->count;
        table->count += 1U;
        *index = entryIndex;
    }
    return 1;
}

Boolean CLSegs_AddValue(AccessPathValueTable *table, struct PayloadWithValue *value, UInt16 *index)
{
    UInt16 allocated_index;

    if (allocate_access_path_value_index(table, &allocated_index)) {
        struct PayloadWithValue **entries = (struct PayloadWithValue **)table->values;
        entries[allocated_index] = value;
        *index = allocated_index;
        return 1;
    }
    return 0;
}

struct PayloadWithValue *CLSegs_GetValue(struct AccessPathValueTable *table, unsigned int index)
{
    if (table == NULL) {
        CLIO_ReportAssertionFailure("segs != NULL", "CLSegs.c", 135U);
    }
    if ((unsigned short)index < table->count) {
        struct PayloadWithValue **entries = (struct PayloadWithValue **)table->values;
        return entries[(unsigned short)index];
    }
    return 0U;
}

unsigned short CLSegs_GetCount(struct AccessPathValueTable *table)
{
    if (table == NULL) {
        CLIO_ReportAssertionFailure("segs != NULL", "CLSegs.c", 145U);
    }
    return table->count;
}
