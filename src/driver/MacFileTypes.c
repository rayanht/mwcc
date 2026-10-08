#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/MacFileTypes.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/MacSpecs.h"
/* A file-type list node: 8 bytes, payload at 0x00 and link at 0x04. */

#include <stdlib.h>
#include <string.h>

int(__stdcall *data_00587e70)();

static struct MacFileTypeNode *defaultlist = NULL;
static struct MacFileTypeNode **mac_file_types = &defaultlist;

void __stdcall MacFileTypes_AppendTable(struct MacFileTypeNode **list, SInt32 value)
{
    struct MacFileTypeNode **pp;

    if (list == NULL) {
        list = mac_file_types;
    }
    pp = list;
    while (*pp != NULL) {
        pp = &(*pp)->next;
    }
    *pp = (struct MacFileTypeNode *)malloc(8);
    if (*pp == NULL) {
        CLIO_ReportAssertionFailure("*scan != NULL", "MacFileTypes.c", 0x2b);
    }
    (*pp)->table = (struct OpcodeDescriptorTable *)value;
    (*pp)->next = NULL;
}

void __stdcall fn_00421a80(int value, unsigned int *result)
{
    MacFileTypeNode *group;
    struct OpcodeDescriptorTable *records;
    int index;
    group = *mac_file_types;
    while (group != NULL) {
        index = 0;
        records = group->table;
        while (index < records->count) {
            if (value == records->patterns[index].result) {
                memset(result, 0, sizeof(*result));
                return;
            }
            index++;
        }
        group = group->next;
    }
    *result = data_0054b770;
}

void __stdcall fn_00421af0(OSSpec *value, DWORD input)
{
    unsigned int result;

    fn_00421a80(input, &result);
    OS_SetFileType(value, &result);
}

unsigned char __stdcall MacFileTypes_MatchBytes(void *bytes, int length, UInt32 *mnemonic)
{
    struct OpcodeDescriptorTable *table;
    int comparison;
    int index;
    MacFileTypeNode *link;
    MacFileTypeNode **head;
    *mnemonic = 0;
    head = mac_file_types;
    link = *head;
    while (link != NULL) {
        table = link->table;
        index = 0;
        while (index < table->count) {
            if (table->patterns[index].length <= length && table->patterns[index].bytes != NULL) {
                comparison = memcmp(bytes, table->patterns[index].bytes, table->patterns[index].length);
                if (comparison == 0) {
                    *mnemonic = table->patterns[index].result;
                    return 1;
                }
            }
            index = index + 1;
        }
        link = link->next;
    }
    return 0;
}

SInt32 __stdcall MacFileTypes_GetFileType(OSSpec *path, UInt32 *fileType)
{
    SInt32 file;
    char header[0x20];
    SInt32 size;
    SInt32 fileFlags;
    OSSpec resolvedPath;
    SInt32 resolvedFileFlags;
    SInt32 resolvedFile;
    DWORD result;

    result = OS_Open(path, 0, &file);
    if (result != 0)
        return result;

    OS_GetSize(file, &fileFlags);
    size = 0x20;
    result = OS_Read(file, header, &size);
    OS_Close(file);
    if (result == 0 && size != 0 && MacFileTypes_MatchBytes(header, size, fileType) != 0)
        return 0;

    if (data_00587e70 == NULL || data_00587e70(path, fileType) == 0) {
        if (fileFlags == 0) {
            result = MacSpecs_MakeResourceForkSpec(path, &resolvedPath, 0);
            if (result == 0) {
                result = OS_Open(&resolvedPath, 0, &resolvedFile);
                if (result == 0) {
                    OS_GetSize(resolvedFile, &resolvedFileFlags);
                    OS_Close(resolvedFile);
                    if (resolvedFileFlags != 0) {
                        *fileType = 0x72737263;
                        return 0;
                    }
                }
            }
        }
        *fileType = 0x54455854;
    }
    return 0;
}

unsigned int __stdcall fn_00421d30(OSSpec *file, SInt32 unused, SInt32 fileType)
{
    fn_00421af0(file, fileType);
}
