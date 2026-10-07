#ifndef DRIVER_CLOVERLAYS_H
#define DRIVER_CLOVERLAYS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CLOverlayValues {
    unsigned int first;
    unsigned int second;
};
struct CLOverlayEntry {
    char name[256];
    CLOverlayValues values;
    struct OverlayAllocation *firstOverlay;
    struct OverlayAllocation *lastOverlay;
    unsigned int overlayCount;
    struct CLOverlayEntry *next;
};
#pragma pack(push, 1)
struct OverlayAllocation {
    char name[256];
    unsigned int *values;
    unsigned int valueCount;
    unsigned int word264;
    struct OverlayAllocation *next;
};
#pragma pack(pop)
struct Overlays {
    struct CLOverlayEntry *groups;
    struct CLOverlayEntry *lastgrp;
    SInt32 numgrps;
};
extern Boolean CLOverlays_Init(Overlays *this_);
extern struct CLOverlayEntry *CLOverlays_GetGroupByIndex(struct Overlays *list, int index);
extern unsigned int CLOverlays_CountGroups(struct Overlays *list);
extern struct OverlayAllocation *CLOverlays_GetAllocationByGroupIndex(Overlays *overlay, int name, int value);
extern unsigned int CLOverlays_GetAllocationValueByGroupIndex(Overlays *overlay, unsigned int arg1, unsigned int arg2,
                                                              unsigned int arg3);
extern struct OverlayAllocation *CLOverlays_GetOverlayAtIndex(struct CLOverlayEntry *list, int index);
extern unsigned int CLOverlays_CountOverlays(CLOverlayEntry *record);
extern struct OverlayAllocation *CLOverlays_CreateOverlayAllocation(const char *name);
extern unsigned int CLOverlays_GetValueCount(struct OverlayAllocation *record);
extern unsigned char CLOverlays_AppendGroup(Overlays *list, CLOverlayEntry *entry, unsigned int *index);
extern unsigned int get_allocation_value(OverlayAllocation *table, unsigned int index);
extern unsigned char CLOverlays_FreeGroups(Overlays *list);
extern void free_overlay_values(struct OverlayAllocation *overlay);
extern CLOverlayEntry *CLOverlays_CreateOverlayEntry(const char *name, CLOverlayValues overlayValues);
extern UInt8 CLOverlays_AppendEntry(struct OverlayAllocation *table, SInt32 entry, SInt32 *entryIndex);
extern char CLOverlays_AppendOverlay(CLOverlayEntry *self, struct OverlayAllocation *overlay, unsigned int *index);
extern void free_overlay_allocations(CLOverlayEntry *list);
extern void CLOverlays_ConvertTimestampTo1904EpochSeconds(SInt32 timestamp, int *result);
extern void CLOverlays_ConvertSecondsToTimestamp(unsigned int seconds, struct PackedConversionResult *result);
extern long days_in_month[];

#ifdef __cplusplus
}
#endif

#endif
