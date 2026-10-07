#ifndef COMPILER_INLINEASMREGISTERSPPC_H
#define COMPILER_INLINEASMREGISTERSPPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RegistrationEntry {
    const char *name;
    unsigned char kind;
    unsigned short value;
};
struct RegistrationHashEntry {
    struct RegistrationHashEntry *next;
    int id;
    const char *name;
    short kind;
    short value;
    int extra;
};
struct RegistrationTableEntry {
    const char *name;       /* 0x00: CTemplateNew_InitRegistrationHashTables reads the register name */
    short value;            /* 0x04: CTemplateNew_InitRegistrationHashTables copies the register value */
    short alignmentPadding; /* 0x06: registration_table, unused space aligning id to 0x08 */
    int id;                 /* 0x08: CTemplateNew_InitRegistrationHashTables copies the register id */
};
struct SecondaryRegistrationEntry {
    int id;
    const char *name;
    short value;
};
struct InlineAsmRegisterEntry {
    const char *name;      /* 0x00: fn_004f06d0 clears the numeric register name */
    short kind;            /* 0x04: fn_004f06d0 sets DCR kind 4; CTemplateNew_LookupInlineAsmRegister sets SPR kind 2 */
    short number;          /* 0x06: InlineAsmPPC.c reads the DCR register number */
    struct Object *object; /* 0x08: CTemplateNew_LookupInlineAsmRegister clears object for numeric registers */
};

extern struct InlineAsmRegisterEntry *fn_004f06d0(char *name);
extern struct InlineAsmRegisterEntry *CTemplateNew_LookupInlineAsmRegister(char *name);
extern void CTemplateNew_InitRegistrationHashTables(void);
extern InlineAsmRegisterEntry *CTemplateNew_GetInlineAsmRegisterEntry(HashNameNode *name);

#ifdef __cplusplus
}
#endif

#endif
