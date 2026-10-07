#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
#include "compiler/win32.h"
#include "driver/CLFileOps.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/StringUtils.h"
#include <string.h>

/* Data of the original file that none of its linked code uses. */
static char lbl_0054C5A0 = 5;

#pragma optimization_level 2

int TargetOptimizer_ppc_eabi_SetOption(short option, char enabled)
{
    switch (option) {
        case 20581:
            data_00537a60 = enabled;
            break;
        case 19831:
            data_00537a67 = enabled;
            break;
        case 21358:
            data_00537a63 = 0;
            break;
        case 21352:
            data_00537a63 = enabled;
            break;
        default:
            return 0;
    }
    return 1;
}

#pragma optimization_level reset

#pragma scheduling off

unsigned int TargetOptimizer_ppc_eabi_ReportScheduling(struct StorageHandle *argument)
{
    int setting;
    unsigned char *message;

    if (data_00537a63 == 0)
        HPrintF(argument, "\t- no instruction scheduling\n");
    else {
        if ((setting = data_00537a68) == 20)
            message = (unsigned char *)"generic PPC";
        else if (setting == 0)
            message = (unsigned char *)"401";
        else if (setting == 1)
            message = (unsigned char *)"403";
        else if (setting == 2)
            message = (unsigned char *)"505";
        else if (setting == 3)
            message = (unsigned char *)"509";
        else if (setting == 4)
            message = (unsigned char *)"555";
        else if (setting == 5)
            message = (unsigned char *)"601";
        else if (setting == 6)
            message = (unsigned char *)"602";
        else if (setting == 7)
            message = (unsigned char *)"603";
        else if (setting == 8)
            message = (unsigned char *)"603e";
        else if (setting == 9)
            message = (unsigned char *)"604";
        else if (setting == 10)
            message = (unsigned char *)"604e";
        else if (setting == 11)
            message = (unsigned char *)"740";
        else if (setting == 12)
            message = (unsigned char *)"750";
        else if (setting == 13)
            message = (unsigned char *)"801";
        else if (setting == 14)
            message = (unsigned char *)"821";
        else if (setting == 15)
            message = (unsigned char *)"823";
        else if (setting == 16)
            message = (unsigned char *)"850";
        else if (setting == 19)
            message = (unsigned char *)"8260";
        else if (setting == 17)
            message = (unsigned char *)(signed char *)"860";
        else
            message = (unsigned char *)"???";

        HPrintF(argument, "\t- schedule for %s\n", message);
    }
}

#pragma scheduling reset

void fn_00420700(void)
{
}

unsigned int fn_00420710(OperationRecord *context)
{
    SInt32 operation;
    SInt32 value;
    HGLOBAL handle;
    DWORD error;

    context->loaded = 0;

    error = OS_Open(&context->fileSpec, 0U, &operation);
    if (!error) {
        do {
            error = OS_GetSize(operation, &value);
            if (error)
                break;
            error = OS_ResizeHandle(&context->buffer, value);
            if (error)
                break;

            handle = MsDos_GetValidMemBufferPtr(&context->buffer);
            error = OS_Read(operation, handle, &value);
            if (!error) {
                context->loaded = 1;
                context->dirty = 0;
            }
            fn_004129c0(&context->buffer);
        } while (0);

        OS_Close(operation);
    }
    return error;
}

DWORD write_file_buffer(struct OperationRecord *file)
{
    DWORD error;
    HGLOBAL contents;
    SInt32 handle;
    DWORD size;

    if ((file->loaded == 0) && (file->dirty == 0)) {
        return 0;
    }
    OS_Delete(&file->fileSpec);
    error = OS_Create(&file->fileSpec, &data_0054b770);
    if (error == 0) {
        error = OS_Open(&file->fileSpec, 2, &handle);
        if (error == 0) {
            error = OS_GetHandleSize(&file->buffer, &size);
            if (error == 0) {
                contents = MsDos_GetValidMemBufferPtr(&file->buffer);
                error = OS_Write(handle, contents, &size);
                if (error == 0) {
                    file->dirty = 0;
                }
                fn_004129c0(&file->buffer);
                OS_Close(handle);
            }
        }
    }
    return error;
}

unsigned int __stdcall TargetOptimizer_ppc_eabi_InitOperationRecord(OSSpec *source, MemBuffer *argument,
                                                                    unsigned char flag, OperationRecord *state)
{
    DWORD result;
    if (flag == 0U && argument != 0U) {
        return 12U;
    }
    state->fileSpec = *source;
    state->writeBack = flag;
    if (argument == 0U) {
        result = OS_NewHandle(0U, &state->buffer);
        if (result != 0U) {
            return result;
        }
        {
            OperationRecord *context = state;
            result = fn_00420710(context);
        }
    } else {
        result = CLFileOps_CopyMemBuffer(argument, &state->buffer);
        if (result != 0U) {
            return result;
        }
        state->dirty = 1U;
        state->loaded = 1U;
    }
    return result;
}

int __stdcall TargetOptimizer_ppc_eabi_GetMemBufferPtrAndSize(unsigned char *state, char **firstResult,
                                                              int *secondResult)
{
    HGLOBAL result;
    MemBuffer *buffer = (MemBuffer *)(state + 324);
    *secondResult = 0;
    if (!OS_ValidHandle(buffer)) {
        return 8;
    }
    result = MsDos_GetValidMemBufferPtr((MemBuffer *)(state + 324));
    *firstResult = result;
    {
        MemBuffer *sizeBuffer = (MemBuffer *)(state + 324);
        OS_GetHandleSize(sizeBuffer, (DWORD *)secondResult);
    }
    return 0;
}

unsigned int __stdcall TargetOptimizer_ppc_eabi_UnloadOperationRecord(struct OperationRecord *record)
{
    DWORD result;
    if (record->writeBack != 0U && record->dirty != 0U) {
        result = write_file_buffer(record);
        if (result != 0U)
            return result;
    }
    if (!OS_ValidHandle(&record->buffer))
        return 8U;
    result = OS_FreeHandle(&record->buffer);
    if (result != 0U)
        return result;
    record->loaded = 0U;
    return 0U;
}
