#include "compiler/common.h"
#include "driver/TextUtils.h"
#include "driver/Memory.h"
#include "driver/ResourceStrings.h"
#include "driver/Resources.h"
#include "driver/StringUtils.h"
#include <stdio.h>
#include <string.h>

char *CLIO_ConvertToPascalString(char *string)
{
    unsigned int length;

    length = strlen(string);
    if (length > 255) {
        length = 255;
    }
    memmove(string + 1, string, length);
    *string = (char)length;
    return string;
}

char *__stdcall CLIO_ConvertPascalToCString(char *string)
{
    unsigned int length = (unsigned char)*string;
    memmove(string, string + 1, length);
    string[length] = '\0';
    return string;
}

void __stdcall CLIO_GetResourceString(unsigned char *output, short resourceID, short stringIndex)
{
    unsigned char *cursor;
    unsigned char *end;
    short remaining;
    char *message;
    struct StorageHandle *resource;
    short stringCount;
    unsigned char *dest;
    int resourceSize;
    unsigned char length;
    unsigned char buffer[256];

    message = ResourceStrings_GetString(resourceID, stringIndex);
    if (message != NULL) {
        strcpy((char *)buffer, message);
        CLIO_ConvertToPascalString((char *)buffer);
    } else {
        sprintf((char *)buffer, "[Resource string id=%d index=%d not found]", resourceID, stringIndex);
        CLIO_ConvertToPascalString((char *)buffer);
        resource = (struct StorageHandle *)Resources_GetHand(1398034979, resourceID);
        if (resource != NULL) {
            {
                struct StringListHeader *header = (struct StringListHeader *)resource->data;
                stringCount = header->countLow + (header->countHigh << 8);
            }
            if (stringIndex > 0 && stringIndex <= stringCount) {
                unsigned char *strings;
                resourceSize = Memory_GetHandleSize(resource);
                fn_00413a00(resource);
                strings = (unsigned char *)resource->data;
                cursor = strings + sizeof(struct StringListHeader);
                end = strings + resourceSize;
                remaining = stringIndex;
                while (cursor < end && --remaining != 0) {
                    length = *cursor;
                    cursor += length + 1;
                }
                if (cursor < strings + resourceSize) {
                    _pstrcpy(buffer, cursor);
                }
                fn_00413a50(resource);
            }
        }
    }
    cursor = buffer + 1;
    dest = output + 1;
    while (cursor <= buffer + buffer[0]) {
        if (*cursor == 0xD4) {
            *dest = '`';
        } else if (*cursor == 0xD5) {
            *dest = '\'';
        } else if (*cursor == 0xD2 || *cursor == 0xD3) {
            *dest = '"';
        } else if (*cursor == 0xC9 && dest - output < 253) {
            *dest = '.';
            dest[1] = '.';
            dest += 2;
            *dest = '.';
        } else {
            *dest = *cursor;
        }
        ++cursor;
        ++dest;
    }
    *output = dest - output - 1;
}

void __stdcall CLIO_GetResourceCString(char *output, int resourceID, int stringIndex)
{
    CLIO_GetResourceString((unsigned char *)output, resourceID, stringIndex);
    CLIO_ConvertPascalToCString(output);
}

/* The linker stripped the function that used this literal; it stays in the unit's .data. */
static void TextUtils_FormatNumber(char *buffer, SInt32 value)
{
    sprintf(buffer, "%d", value);
}
