#include "compiler/common.h"
#include "compiler/InlineAsmRegisters.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"

#include <string.h>

static struct RegisterBinding *register_binding_hash[64];

void CTemplateNew_ClearGlobalArray(void)
{
    SInt32 index;
    for (index = 0; index < 64; index++)
        register_binding_hash[index] = NULL;
}

void CTemplateNew_InsertRegisterBinding(const char *key, unsigned int attribute1, short registerNumber, Object *object)
{
    struct RegisterBinding *entry;
    struct RegisterBinding **bucket;

    bucket = &register_binding_hash[CHash(key) & 63];
    entry = (struct RegisterBinding *)lalloc(sizeof(*entry));
    entry->key = (unsigned int)key;
    entry->attribute1 = attribute1;
    entry->registerNumber = registerNumber;
    entry->object = object;
    entry->next = *bucket;
    *bucket = entry;
}

void *find_register_binding_key(unsigned int *key)
{
    struct RegisterBinding *binding = register_binding_hash[CHash((const char *)key) & 0x3f];
    while (binding) {
        unsigned int *bindingKey = &binding->key;
        if (strcmp((const char *)binding->key, (const char *)key) == 0)
            return bindingKey;
        binding = binding->next;
    }
    return NULL;
}
