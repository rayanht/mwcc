#include "compiler/common.h"
#include "driver/StringExtras.h"
#include <string.h>

char *__stdcall CLProj_AppendString(char *dest, char *src, int size)
{
    char *end;
    char ch;

    end = dest + strlen(dest);
    while ((ch = *src) != '\0' && (int)(end - dest + 1) < size) {
        *end = ch;
        src++;
        end++;
    }
    *end = '\0';
    return dest;
}

char *__stdcall CLProj_CopyStringBounded(char *destination, const char *source, unsigned int count, int capacity)
{
    char *output;

    for (output = destination; (count-- && *source != '\0' && (int)(output - destination + 1) < capacity);
         output = 1 + output) {
        *output = *source;
        ++source;
    }
    *output = '\0';
    return destination;
}
