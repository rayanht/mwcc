#include "compiler/common.h"
#include "driver/CLOverlays.h"
#include "driver/AssertionFailure.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/MemUtils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OS_ASSERT(line, cond)                                                                                          \
    if (!(cond))                                                                                                       \
    CLIO_ReportAssertionFailure(#cond, "CLOverlays.c", line)

Boolean CLOverlays_Init(Overlays *this)
{
    CLOverlayEntry *grp;
    struct OverlayAllocation *ovl;
    CLOverlayValues addr;
    unsigned int idx;

    OS_ASSERT(24, this);
    this->groups = NULL;
    this->lastgrp = NULL;
    this->numgrps = 0;
    addr.first = addr.second = 0;
    grp = CLOverlays_CreateOverlayEntry("main_application", addr);
    if (!grp)
        return 0;
    ovl = CLOverlays_CreateOverlayAllocation("MAIN");
    if (!ovl)
        return 0;
    CLOverlays_AppendOverlay(grp, ovl, &idx);
    OS_ASSERT(42, idx==0);
    CLOverlays_AppendGroup(this, grp, &idx);
    OS_ASSERT(45, idx==0);
    return 1;
}

unsigned char CLOverlays_FreeGroups(Overlays *list)
{
    struct CLOverlayEntry *entry;
    struct CLOverlayEntry *next;

    if (!list)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 54U);
    entry = list->groups;
    while (entry) {
        next = entry->next;
        free_overlay_allocations(entry);
        free(entry);
        entry = next;
    }
    list->groups = NULL;
    return 1;
}

unsigned char CLOverlays_AppendGroup(Overlays *list, CLOverlayEntry *entry, unsigned int *index)
{
    if (!list)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 70U);
    if (!entry)
        CLIO_ReportAssertionFailure("grp", "CLOverlays.c", 71U);
    if (!list->groups)
        list->groups = entry;
    else
        list->lastgrp->next = entry;
    list->lastgrp = entry;
    if (index)
        *index = list->numgrps;
    list->numgrps += 1U;
    return 1;
}

struct CLOverlayEntry *CLOverlays_GetGroupByIndex(struct Overlays *list, int index)
{
    struct CLOverlayEntry *record;
    int count;

    count = 0;
    if (!list)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 93);

    record = list->groups;
    while (record && count < index) {
        ++count;
        record = record->next;
    }
    if (count == index)
        return record;
    return NULL;
}

unsigned int CLOverlays_CountGroups(struct Overlays *list)
{
    struct CLOverlayEntry *record;
    unsigned int count;
    count = 0U;
    if (!list)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 112U);
    record = list->groups;
    if (record) {
        do {
            record = record->next;
            count += 1U;
        } while (record);
    }
    return count;
}

struct OverlayAllocation *CLOverlays_GetAllocationByGroupIndex(Overlays *overlay, int name, int value)
{
    struct CLOverlayEntry *entry;
    if (overlay == 0U)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 144U);
    entry = CLOverlays_GetGroupByIndex(overlay, name);
    if (entry != 0U)
        return CLOverlays_GetOverlayAtIndex(entry, value);
    return 0U;
}

unsigned int CLOverlays_GetAllocationValueByGroupIndex(Overlays *overlay, unsigned int groupName,
                                                       unsigned int groupIndex, unsigned int valueIndex)
{
    struct OverlayAllocation *allocation;
    if (!overlay)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 160U);
    allocation = CLOverlays_GetAllocationByGroupIndex(overlay, groupName, groupIndex);
    if (allocation)
        return get_allocation_value(allocation, valueIndex);
    return 4294967295U;
}

CLOverlayEntry *CLOverlays_CreateOverlayEntry(const char *name, CLOverlayValues overlayValues)
{
    CLOverlayEntry *overlay;
    if (!name)
        CLIO_ReportAssertionFailure("name", "CLOverlays.c", 175U);
    overlay = xmalloc(NULL, 280U);
    if (overlay) {
        strncpy(overlay->name, name, 256U);
        overlay->name[255] = 0;
        overlay->values = overlayValues;
        overlay->lastOverlay = NULL;
        overlay->firstOverlay = overlay->lastOverlay;
        overlay->overlayCount = 0U;
        overlay->next = NULL;
    } else {
        CLErrors_ReportInternalError("CLOverlays.c", 188, "Could not allocate %s", "overlay group");
    }
    return overlay;
}

void free_overlay_allocations(CLOverlayEntry *list)
{
    struct OverlayAllocation *p;
    struct OverlayAllocation *next;
    if (list == NULL) {
        CLIO_ReportAssertionFailure("grp", "CLOverlays.c", 0xc5);
    }
    p = list->firstOverlay;
    while (p) {
        next = p->next;
        free_overlay_values(p);
        free(p);
        p = next;
    }
    list->firstOverlay = NULL;
}

char CLOverlays_AppendOverlay(CLOverlayEntry *self, struct OverlayAllocation *overlay, unsigned int *index)
{
    if (self == NULL) {
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 211U);
    }
    if (overlay == NULL) {
        CLIO_ReportAssertionFailure("oly", "CLOverlays.c", 212U);
    }
    if (self->lastOverlay == NULL) {
        self->firstOverlay = overlay;
    } else {
        self->lastOverlay->next = overlay;
    }
    self->lastOverlay = overlay;
    if (index != NULL) {
        *index = self->overlayCount;
    }
    self->overlayCount++;
    return 1;
}

struct OverlayAllocation *CLOverlays_GetOverlayAtIndex(struct CLOverlayEntry *list, int index)
{
    struct OverlayAllocation *entry;
    int position;
    position = 0;
    if (list == NULL)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 234U);
    entry = list->firstOverlay;
    while (entry != NULL && position < index) {
        position += 1;
        entry = entry->next;
    }
    if (position == index)
        return entry;
    return NULL;
}

unsigned int CLOverlays_CountOverlays(CLOverlayEntry *record)
{
    struct OverlayAllocation *link;
    unsigned int count;

    count = 0U;
    if (record == NULL)
        CLIO_ReportAssertionFailure("this", "CLOverlays.c", 254U);

    link = record->firstOverlay;
    if (link != NULL) {
        do {
            link = link->next;
            count += 1U;
        } while (link != NULL);
    }
    return count;
}

struct OverlayAllocation *CLOverlays_CreateOverlayAllocation(const char *name)
{
    struct OverlayAllocation *overlay;
    overlay = xmalloc(NULL, 272U);
    if (overlay) {
        strncpy(overlay->name, name, 256U);
        overlay->name[255] = 0;
        overlay->values = NULL;
        overlay->word264 = 0U;
        overlay->valueCount = overlay->word264;
        overlay->next = NULL;
    } else {
        CLErrors_ReportInternalError("CLOverlays.c", 281, "Could not allocate %s", "overlay");
    }
    return overlay;
}

void free_overlay_values(struct OverlayAllocation *overlay)
{
    if (overlay == NULL)
        CLIO_ReportAssertionFailure("oly", "CLOverlays.c", 288U);
    if (overlay->values != NULL)
        free(overlay->values);
    overlay->values = NULL;
}

UInt8 CLOverlays_AppendEntry(struct OverlayAllocation *table, SInt32 entry, SInt32 *entryIndex)
{
    SInt32 count;
    SInt32 capacity;
    if (!table) {
        CLIO_ReportAssertionFailure("oly", "CLOverlays.c", 296U);
    }
    count = table->valueCount;
    capacity = table->word264;
    if (count >= capacity) {
        table->word264 += 16;
        table->values = xrealloc("overlay file list", table->values, table->word264 << 2);
    }
    table->values[table->valueCount] = entry;
    if (entryIndex) {
        *entryIndex = table->valueCount;
    }
    table->valueCount += 1;
    return 1;
}

unsigned int get_allocation_value(OverlayAllocation *table, unsigned int index)
{
    int allocationIndex;
    int valueCount;

    if (table == NULL) {
        CLIO_ReportAssertionFailure("oly", "CLOverlays.c", 314U);
    }
    allocationIndex = index;
    valueCount = table->valueCount;
    if (allocationIndex < valueCount) {
        return table->values[index];
    }
    return 0xffffffffU;
}

unsigned int CLOverlays_GetValueCount(struct OverlayAllocation *record)
{
    if (record == NULL)
        CLIO_ReportAssertionFailure("oly", "CLOverlays.c", 323U);
    return record->valueCount;
}

void CLOverlays_ConvertTimestampTo1904EpochSeconds(SInt32 timestamp, int *result)
{
    int year;
    int yearOffset;
    struct tm *calendar;

    calendar = localtime(&timestamp);
    year = calendar->tm_year;
    yearOffset = year - 4;
    *result = ((yearOffset + 3) / 4 + yearOffset * 365 - (yearOffset + 4) / 100 + (yearOffset - 296) / 400 +
               calendar->tm_yday) *
                  86400 +
              calendar->tm_hour * 3600 + calendar->tm_min * 60 + calendar->tm_sec;
}

void CLOverlays_ConvertSecondsToTimestamp(unsigned int seconds, struct PackedConversionResult *result)
{
    static long days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct tm time;
    unsigned int remaining;
    int month;
    int timestamp;

    memset(&time, 0, sizeof(time));
    remaining = seconds;
    time.tm_sec = seconds % 60;
    remaining = remaining / 60;
    time.tm_min = remaining % 60;
    remaining = remaining / 60;
    time.tm_hour = remaining % 24;
    remaining = remaining / 24;
    time.tm_yday = remaining % 365;
    remaining = remaining / 365;
    time.tm_year = remaining + 4;
    time.tm_yday -= (((remaining + 3) >> 2) - (remaining + 4) / 100 + (int)(remaining - 296) / 400);
    if (((time.tm_year % 4) != 0) && (((time.tm_year % 100) != 0) || ((time.tm_year % 400) == 0))) {
        days_in_month[1] = 28;
    } else {
        days_in_month[1] = 29;
    }
    for (month = 0; time.tm_yday >= days_in_month[month]; month = month + 1) {
        time.tm_yday -= days_in_month[month];
    }
    time.tm_mon = month;
    time.tm_mday = time.tm_yday + 1;
    timestamp = mktime(&time);
    result->value = timestamp;
    if ((4 <= month) && (month < 10)) {
        result->value += 3600;
    }
}
