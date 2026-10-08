#ifndef COMPILER_INLINEASMMNEMONICSPPC_H
#define COMPILER_INLINEASMMNEMONICSPPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct AsmOperandPattern {
    char *name;
    unsigned int opcode;
    unsigned char operands[6];
    unsigned int processorMask;
    unsigned int instruction;
};
#pragma pack(pop)
struct NameLookupLink {
    struct NameLookupLink *next;
    struct AsmOperandPattern *record;
};

extern struct NameLookupLink *asmOperandPatternLookup[256];
extern AsmOperandPattern *CTemplateNew_FindAsmOperandPattern(char *name);
extern void CTemplateNew_InitAsmOperandPatternLookup(void);

#ifdef __cplusplus
}
#endif

#endif
