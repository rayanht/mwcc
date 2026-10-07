#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/MemUtils.h"
#include "driver/CLBrowser.h"
#include "driver/CLErrors.h"
#include "driver/CLPlugins.h"
#include <string.h>

/* Source and destination handles for a preference data copy. */
#include <stdio.h>
#include <stdlib.h>



void *xmalloc(const char *text, unsigned int size)
{
    char message[80];
    char *memory;

    memory = malloc(size ? size : 1);
    if (memory == NULL) {
        sprintf(message, "Out of memory when allocating %d bytes%s%s", size, text ? " for " : "", text ? text : "");
        CLErrors_FatalError(message);
        longjmp(driver_jmp_buf, 1);
        return NULL;
    }
    return memory;
}

void *xcalloc(const char *pool, unsigned int size)
{
    unsigned char *memory;
    memory = xmalloc(pool, size);
    memset(memory, 0, size);
    return memory;
}

void *xrealloc(const char *detail, void *block, unsigned int size)
{
    char message[80];
    char *resizedBlock;

    resizedBlock = (char *)realloc(block, size ? size : 1U);
    if (!resizedBlock) {
        sprintf(message, "Out of memory when resizing buffer to %d bytes%s%s", size, detail ? " for " : "",
                detail ? detail : "");
        CLErrors_FatalError(message);
        longjmp(driver_jmp_buf, 1U);
        return NULL;
    }
    return resizedBlock;
}

char *xstrdup(const char *s)
{
    return strcpy(xmalloc(NULL, strlen(s) + 1), s);
}

int __stdcall MemUtils_CallPluginEntry(Plugin *entry)
{
    short result;
    result = CLPlugins_CallEntry(entry, entry->object);
    return result;
}

#pragma auto_inline reset
