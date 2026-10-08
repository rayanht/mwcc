#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/libimp-eabi-ppc.h"
#include "driver/PrefPanels.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/ELF_Endian.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/LibImportMessages.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/StringUtils.h"
#include <string.h>
#include <stdio.h>

jmp_buf file_input_jmpbuf;
SInt32 section_size_total;
SInt32 accumulated_section_size;
char data_0058770c[588];
SInt32 accumulated_section_sizes;
char data_00588517;

typedef char **Handle;

static struct LibImportCU libimp_cu;
static UInt8 nonNativeByteOrder;

/* An ELF file's first four bytes. */
static unsigned int data_0054c490 = 0x464C457F;

void format_message_and_longjmp()
{
    CompilerTools_FormatMessageAndLongjmp(0xd, -0x6c);
}

#pragma optimization_level 2
void fn_0041e970(char *name, void *argument)
{
    CWPluginsPrivate_InvokeMessageCallback(libimp_cu.context, NULL, name, (char *)argument, 2, 0);
}
#pragma optimization_level reset

#pragma optimization_level 2
void fn_0041e990(char *name, void *argument)
{
    CWPluginsPrivate_InvokeMessageCallback(libimp_cu.context, NULL, name, argument, 1, 0);
}
#pragma optimization_level reset

#pragma optimization_level 2
UInt8 fn_0041e9b0(char *data)
{
    unsigned int reference = data_0054c490;
    int comparison = strncmp(data, (char *)&reference, sizeof(reference));
    if (comparison == 0) {
        return 1;
    }
    return 0;
}
#pragma optimization_level reset

#pragma scheduling off
#pragma sym on
char **load_file_data_and_set_archive_signature(CWFileSpec *name, SInt32 *out1, SInt32 *out2, SInt32 *out3)
{
    short err;
    int length;
    StorageHandle *contents;
    FileInputNode *rec;
    char buf[256];

    nonNativeByteOrder = (!copts.littleendian) != 0;
    limited_diagnostic_count = data_00588228 = 0;

    err = 0;
    rec = read_file_into_input_nodes(name, &err);
    if (err != 0 || rec == NULL) {
        CompilerTools_ReportLimitedDiagnostic(0x1d, name->name);
        if (rec != NULL)
            clear_file_input_data_handles(rec);
        return NULL;
    }
    if (rec->kind == 0) {
        CompilerTools_ReportLimitedDiagnostic(0x1e, name->name);
        clear_file_input_data_handles(rec);
        return NULL;
    }
    accumulated_section_size = 0;
    accumulated_section_sizes = 0;
    section_size_total = 0;
    p2cstrcpy(buf, name->name);
    if (initheaps(format_message_and_longjmp) != 0) {
        format_message_and_longjmp();
        clear_file_input_data_handles(rec);
        return NULL;
    }
    err = dispatch_file_input_by_kind(rec, buf);
    releasegheap();
    if (err != 0) {
        clear_file_input_data_handles(rec);
        return NULL;
    }
    if (!COS_ResizeHandle(contents = contents = (StorageHandle *)rec->dataHandle, length = 8)) {
        CompilerTools_ReportLimitedDiagnostic(0x1d, name->name);
        clear_file_input_data_handles(rec);
        return NULL;
    }
    if (rec->kind == 1) {
        strncpy(*rec->dataHandle, "!<arch>\n", 8);
    }
    *out1 = accumulated_section_size;
    *out3 = accumulated_section_sizes;
    *out2 = section_size_total;
    clear_file_input_data_handles(rec->next);
    return rec->dataHandle;
}
#pragma scheduling reset
#pragma sym reset

#pragma scheduling off
char initialize_plugin_context(CWPluginPrivateContext *handle)
{
    libimp_cu.context = handle;
    memset(&libimp_cu.objectData, 0, sizeof(unsigned int[12]));
    if (CPrep_GetFileIndex(handle, (unsigned int *)&libimp_cu.mainFileNumber) != 0)
        return 0;
    if (CPrep_GetContextPayload(handle, &libimp_cu.mainFile) != 0)
        return 0;
    if (CPrep_GetSetting(handle, &libimp_cu.filesyminfo) != 0)
        return 0;
    return 1;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int fn_0041ec20(void)
{
    unsigned int value;
    if ((value = libimp_cu.objectBuffer) != 0U) {
        value = fn_0041bcb0(libimp_cu.context, (struct StorageHandle *)value, (long *)&libimp_cu.objectData);
        if (value != 0U)
            return value;
    }
    value =
        CPrep_CallCompilerCallbackWithValue(libimp_cu.context, libimp_cu.mainFileNumber, (long *)&libimp_cu.objectData);
    if (value != 0U)
        return value;
    return value;
}
#pragma scheduling reset

#pragma optimization_level 2
#pragma sym on
int __stdcall fn_0041ec70(CWPluginPrivateContext *context)
{
    int result;
    char **status;
    long mode;
    PProject block60;
    struct ConfigurationBlock116 block116;
    PProject **reference60;
    struct ConfigurationBlock116 **reference116;

    result = 0;
    CWPluginsPrivate_GetRequest(context, &mode);
    switch (mode) {
        case -2:
            result = 0;
            break;
        case -1:
            result = 0;
            break;
        case 0:
            memset(&copts, 0, 240);
            memset(data_0058770c, 0, 1364);
            reference60 = NULL;
            DropInCompilerLinkerPrivate_CallArgumentValue(context, "PPC EABI Project", &reference60);
            if (reference60 != NULL) {
                block60 = **reference60;
            }
            copts.littleendian = !block60.bigendian;
            reference116 = NULL;
            DropInCompilerLinkerPrivate_CallArgumentValue(context, "PPC EABI Linker", &reference116);
            if (reference116 != NULL) {
                block116 = **reference116;
            }
            data_00587958 = block116.flag_05;
            data_00588517 = 0;
            initialize_plugin_context(context);
            libimp_cu.browseBuffer = 0;
            libimp_cu.lineCount = 0;
            status = load_file_data_and_set_archive_signature(&libimp_cu.mainFile, &libimp_cu.codeSize,
                                                              &libimp_cu.udataSize, &libimp_cu.idataSize);
            libimp_cu.objectBuffer = (unsigned int)status;
            if (status != NULL) {
                result = fn_0041ec20();
            } else {
                result = 2;
            }
            break;
    }
    result = CWPluginsPrivate_ReturnArgument(context, (short)result);
    return result;
}
#pragma optimization_level reset
#pragma sym reset

#pragma optimization_level 2

void fn_0041ede0(char *value)
{
    CWPluginsPrivate_CallCallback9(libimp_cu.context, value, NULL, NULL, 0);
}

#pragma optimization_level reset
#pragma optimization_level 2
void fn_0041ee00(void *argument, short request)
{
    CWPluginsPrivate_InvokeMessageCallback((struct DispatchObject_0041b830 *)libimp_cu.context, NULL, argument, NULL, 2,
                                           request);
}
#pragma optimization_level reset

/* The ticks before the importer next lets the IDE break in. */
static SInt32 data_0054c4d0 = 0;

void check_ticks_and_longjmp(void)
{
    if (CompilerTools_GetTicks() > data_0054c4d0) {
        if (fn_0041b910(libimp_cu.context) == 1U)
            longjmp(file_input_jmpbuf, 1U);
        data_0054c4d0 = CompilerTools_GetTicks() + 15U;
    }
}
#pragma optimization_level 2

static char data_0054c4d4[] = "mw";

unsigned char classify_file_header(CWFileSpec *input)
{
    char hasValue;
    short handle;
    SInt32 size;
    struct ReadValue value;
    size = 8;
    hasValue = 0;
    if (Files_OpenFile(input, 1, &handle) == 0) {
        if (Files_Read(handle, &size, &value) == 0 && size == 8) {
            hasValue = 1;
        }
        Files_Close(handle);
    }
    if (hasValue != 0) {
        if (fn_0041e9b0(value.data) != 0) {
            return 4;
        }
        if (strncmp(value.data, "!<arch>\n", 8) == 0) {
            return 1;
        }
        if (value.word0 == data_0054c4d4[0] && value.word1 == data_0054c4d4[1]) {
            return 0;
        }
    }
    return 0;
}

#pragma optimization_level reset
#pragma optimization_level 2
#pragma sym on
char **read_file_into_buffer(CWFileSpec *file, short *error, int extraBytes)
{
    char **buffer;
    short fileRef;
    SInt32 fileSize;
    SInt32 bytesRead;
    *error = Files_OpenFile(file, 1, &fileRef);
    if (*error == 0) {
        *error = Files_GetSize(fileRef, &fileSize);
        if (*error != 0 || fileSize == 0) {
            Files_Close(fileRef);
            return NULL;
        }
        buffer = COS_NewOSHandle(fileSize + extraBytes);
        if (buffer == NULL) {
            buffer = COS_NewHandle(fileSize + extraBytes);
            if (buffer == NULL) {
                Files_Close(fileRef);
                *error = -108;
                return NULL;
            }
        }
        COS_LockHandle(buffer);
        bytesRead = fileSize;
        *error = Files_Read(fileRef, &bytesRead, *buffer);
        if (*error != 0 || bytesRead != fileSize) {
            Memory_FreeHandle((StorageHandle *)buffer);
            Files_Close(fileRef);
            return NULL;
        }
        if (extraBytes != 0) {
            (*buffer)[fileSize] = 0;
        }
        COS_UnlockHandle(buffer);
        Files_Close(fileRef);
        return buffer;
    }
    return NULL;
}
#pragma optimization_level reset
#pragma sym reset

#pragma optimization_level 2
FileInputNode *read_file_into_input_nodes(CWFileSpec *name, SInt16 *err)
{
    FileInputNode *member;
    unsigned char type;
    FileInputNode *head;
    FileInputNode *last;
    char **data;
    char *header;
    SInt32 size;
    SInt16 handle;
    SInt32 count;
    SInt32 length;
    char buffer[8];
    SInt32 position;

    type = classify_file_header(name);
    if (type == 4) {
        member = (FileInputNode *)galloc(sizeof(FileInputNode));
        memset(member, 0, sizeof(FileInputNode));
        member->dataHandle = read_file_into_buffer(name, err, 0);
        member->kind = type;
        return member;
    }
    if (type == 1) {
        *err = Files_OpenFile(name, 1, &handle);
        if (*err != 0)
            return NULL;
        *err = Files_GetSize(handle, &size);
        if (*err != 0 || size == 0) {
            Files_Close(handle);
            return NULL;
        }
        head = last = NULL;
        count = sizeof(buffer);
        *err = Files_Read(handle, &count, buffer);
        if (*err != 0 || count != sizeof(buffer)) {
            Files_Close(handle);
            return NULL;
        }
        position = sizeof(buffer);
        while (position < size) {
            member = (FileInputNode *)galloc(sizeof(FileInputNode));
            if (member == NULL) {
                Files_Close(handle);
                *err = -0x6c;
                for (member = head; member; member = member->next)
                    if (member->dataHandle)
                        Memory_FreeHandle((StorageHandle *)member->dataHandle);
                return NULL;
            }
            if (head == NULL)
                head = member;
            memset(member, 0, sizeof(FileInputNode));
            member->kind = 1;
            count = 0x3c;
            member->archiveMemberHeader = galloc(0x3c);
            if (member->archiveMemberHeader == NULL) {
                Files_Close(handle);
                for (member = head; member; member = member->next)
                    if (member->dataHandle)
                        Memory_FreeHandle((StorageHandle *)member->dataHandle);
                return NULL;
            }
            *err = Files_Read(handle, &count, member->archiveMemberHeader);
            if (*err != 0 || count != 0x3c) {
                Files_Close(handle);
                for (member = head; member; member = member->next)
                    if (member->dataHandle)
                        Memory_FreeHandle((StorageHandle *)member->dataHandle);
                return NULL;
            }
            header = member->archiveMemberHeader;
            position += 0x3c;
            sscanf(header + 0x30, "%ld", &length);
            data = COS_NewOSHandle(length);
            if (data == NULL) {
                data = COS_NewHandle(length);
                if (data == NULL) {
                    Files_Close(handle);
                    *err = -0x6c;
                    for (member = head; member; member = member->next)
                        if (member->dataHandle)
                            Memory_FreeHandle((StorageHandle *)member->dataHandle);
                    return NULL;
                }
            }
            member->dataHandle = data;
            COS_LockHandle(member->dataHandle);
            count = length;
            *err = Files_Read(handle, &count, *member->dataHandle);
            if (*err != 0 || count != length) {
                Files_Close(handle);
                for (member = head; member; member = member->next)
                    if (member->dataHandle)
                        Memory_FreeHandle((StorageHandle *)member->dataHandle);
                return NULL;
            }
            position += length;
            if ((position & 1) != 0 && position < size) {
                count = 1;
                *err = Files_Read(handle, &count, buffer);
                if (*err != 0 || count != 1) {
                    Files_Close(handle);
                    for (member = head; member; member = member->next)
                        if (member->dataHandle)
                            Memory_FreeHandle((StorageHandle *)member->dataHandle);
                    return NULL;
                }
                position++;
            }
            COS_UnlockHandle(member->dataHandle);
            if (last != NULL)
                last->next = member;
            last = member;
        }
        Files_Close(handle);
        return head;
    }
    return NULL;
}
#pragma optimization_level reset
#pragma optimization_level 2

void clear_file_input_data_handles(FileInputNode *args)
{
    while (args != NULL) {
        if (args->dataHandle != NULL)
            Memory_FreeHandle((StorageHandle *)args->dataHandle);
        args->dataHandle = NULL;
        args = args->next;
    }
}

static inline void ReportModeError(unsigned int error, char *location, const char *name)
{
    CompilerTools_ReportDiagnostic(error, location, name);
}

static inline void ReportInvalidMode(unsigned int error, const void *modeName, char *location, const char *name)
{
    CompilerTools_ReportDiagnostic(error, modeName, location, name);
}

static inline void ReportInvalidKind(unsigned int error, char *subject, const void *prefix, char *location,
                                     const void *suffix, const void *message)
{
    CompilerTools_ReportDiagnostic(error, subject, prefix, location, suffix, message);
}

static inline void ReportUnsupportedKind(char *location, char *name)
{
    ReportInvalidKind(54U, *name ? name : location, *name ? " of archive '" : "", *name ? location : "",
                      *name ? "'" : "", (unsigned char *)"PowerPC");
}

#pragma optimization_level reset
#pragma scheduling off
unsigned char fn_0041f430(Elf32Header *record, char *location, char *name)
{
    if (record->dataEncoding == 1U) {
        if (copts.littleendian == 0U) {
            ReportModeError(48U, location, name);
            return 0U;
        }
    } else if (record->dataEncoding == 2U) {
        if (copts.littleendian != 0U) {
            ReportModeError(48U, location, name);
            return 0U;
        }
    } else {
        ReportInvalidMode(46U, (unsigned char *)"EI_DATA", location, name);
        return 0U;
    }

    if (record->elfClass == 1U) {
        if (data_00588517 != 0) {
            ReportModeError(47U, location, name);
            return 0U;
        }
    } else if (record->elfClass == 2U) {
        if (data_00588517 == 0) {
            ReportModeError(47U, location, name);
            return 0U;
        }
    } else {
        ReportInvalidMode(46U, (signed char *)"EI_CLASS", location, name);
        return 0U;
    }

    if (record->machine != 0x14U && record->machine != 0x11U) {
        ReportUnsupportedKind(location, name);
        return 0U;
    }
    return 1U;
}
#pragma scheduling reset

#pragma optimization_level 2
void accumulate_section_sizes(FileInputNode *input, char *option1, char *option2)
{
    Elf32Header *header;
    Elf32Section *section;
    int section_index;
    char *image;
    COS_LockHandleHi(input->dataHandle);
    image = (char *)*input->dataHandle;
    check_ticks_and_longjmp();
    header = (Elf32Header *)image;
    if (nonNativeByteOrder != 0) {
        ELF_Endian_SwapElf32Header(header);
    }
    if (fn_0041f430(header, option1, option2) != 0) {
        for (section_index = 0; section_index < header->section_header_count; ++section_index) {
            section = (Elf32Section *)(image + header->section_header_offset + section_index * sizeof(Elf32Section));
            if (nonNativeByteOrder != 0) {
                ELF_Endian_SwapElf32Section(section);
            }
            if ((section->flags & 4) != 0) {
                accumulated_section_size += section->size;
            } else if ((section->flags & 2) != 0) {
                if (section->type == 1) {
                    accumulated_section_sizes += section->size;
                } else if (section->type == 8) {
                    section_size_total += section->size;
                }
            }
        }
    }
    COS_UnlockHandle(input->dataHandle);
}
#pragma optimization_level reset

#pragma optimization_level 2
char *resolve_archive_member_name(char **baseOffset, char *text)
{
    char *base;
    int index;
    index = 0;
    base = NULL;

    if (baseOffset != NULL)
        base = *baseOffset;

    if (base == NULL && text[0] == '/') {
        ++index;
        if (text[index] == '/')
            ++index;
        if (text[index] == ' ' || text[index] == 0) {
            text[index] = 0;
            return text;
        }
    }

    if (text[0] == '/' && base != NULL && (__ctype_map[(unsigned char)text[1]] & 16) != 0) {
        sscanf(text + 1, "%ld", &index);
        return base + index;
    }

    for (index = 15; index != 0; --index) {
        if (text[index] == '/' || text[index] == 0) {
            text[index] = 0;
            return text;
        }
    }
    text[15] = 0;
    return text;
}
#pragma optimization_level reset

#pragma optimization_level 2
void terminate_slash_newline_sequences(char **buffer, int length)
{
    char *text = *buffer;
    int index;
    for (index = length - 1; index > 1; --index) {
        if (text[index] == '\n' && text[index - 1] == '/')
            text[index - 1] = 0;
    }
}
#pragma optimization_level reset

#pragma optimization_level 2
void classify_archive_members(FileInputNode *list, char *arg)
{
    Handle longNames;
    char *name;
    Handle data;
    FileInputNode *node;
    char *header;

    longNames = NULL;
    for (node = list; node; node = node->next) {
        header = node->archiveMemberHeader;
        data = node->dataHandle;
        name = resolve_archive_member_name(longNames, header);

        if (!Memory_GetHandleSize((struct StorageHandle *)data)) {
            CompilerTools_ReportDiagnostic(0x7e, arg, name);
            continue;
        }
        if (!memcmp(name, "//", 3)) {
            longNames = data;
            COS_LockHandleHi(data);
            terminate_slash_newline_sequences(data, Memory_GetHandleSize((struct StorageHandle *)data));
            node->kind = 3;
        }
        if (!fn_0041e9b0(*data))
            continue;
        node->kind = 2;
        accumulate_section_sizes(node, arg, name);
    }
    if (longNames)
        COS_UnlockHandle(longNames);
}
#pragma optimization_level reset
#pragma optimization_level 2

/* Record header with a four-byte prefix and a kind byte. */
short dispatch_file_input_by_kind(FileInputNode *record, char *buffer)
{
    short result = _Setjmp(file_input_jmpbuf);

    if (result == 0) {
        if (record->kind == 1) {
            classify_archive_members(record, buffer);
        } else if (record->kind == 4) {
            accumulate_section_sizes(record, buffer, "");
        } else {
            CompilerTools_ReportLimitedDiagnostic(0x39, buffer);
            CompilerTools_FormatMessageAndLongjmp(0xd, 2);
        }
    } else {
        CompilerTools_DispatchMessageBufferByMode(result);
    }
    return result;
}
#pragma optimization_level reset
