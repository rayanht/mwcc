#ifndef COMPILER_LOADDELETION_H
#define COMPILER_LOADDELETION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct E {
    struct PCodeInstruction *inst;
    int flag;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct VectorArrayUse {
    struct VectorArrayUse *next;
    int instructionIndex;
};
#pragma pack(pop)
extern void LoadDeletion_BuildLoadLivenessSets(void);
extern void LoadDeletion_InitializeLoadLivenessRecordCounts(void);
extern void LoadDeletion_RecordImmediateLoadLiveness(void);
extern int *block_record_counts;
extern struct E *immediateLoadLiveness;
extern SInt32 *load_liveness_record_start;
extern int data_0058820c;

#ifdef __cplusplus
}
#endif

#endif
