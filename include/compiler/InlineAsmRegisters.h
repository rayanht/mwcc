#ifndef COMPILER_INLINEASMREGISTERS_H
#define COMPILER_INLINEASMREGISTERS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RegisterBinding {
    struct RegisterBinding *next;
    unsigned int key;
    unsigned short attribute1;
    unsigned short registerNumber;
    struct Object *object;
};

extern void *find_register_binding_key(unsigned int *key);
extern void CTemplateNew_InsertRegisterBinding(const char *key, unsigned int attribute1, short registerNumber,
                                               Object *object);
extern void CTemplateNew_ClearGlobalArray(void);

#ifdef __cplusplus
}
#endif

#endif
