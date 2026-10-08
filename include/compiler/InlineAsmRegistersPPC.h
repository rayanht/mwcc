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
    const char *name;
    short value;
    short alignmentPadding;
    int id;
};
struct SecondaryRegistrationEntry {
    int id;
    const char *name;
    short value;
};
struct InlineAsmRegisterEntry {
    const char *name;
    short kind;
    short number;
    struct Object *object;
};

extern struct InlineAsmRegisterEntry *fn_004f06d0(char *name);
extern struct InlineAsmRegisterEntry *CTemplateNew_LookupInlineAsmRegister(char *name);
extern void CTemplateNew_InitRegistrationHashTables(void);
extern InlineAsmRegisterEntry *CTemplateNew_GetInlineAsmRegisterEntry(HashNameNode *name);

#ifdef __cplusplus
}
#endif

#endif
