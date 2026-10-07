#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLWriteObjectFile.h"
#include "driver/AssertionFailure.h"
#include "driver/CLDependencies.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/CLTarg.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/MsDos.h"
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

UInt32 CLWriteObjectFile_WriteObjectFile(struct DropinFileRecord *self, unsigned int option1, unsigned int option2)
{
    CWFileSpec sourceFile;
    CWFileSpec objectFile;
    char *result;
    UInt8 success;

    unsigned int ready = self->objectData != 0 && self->selectedPlugin != NULL;
    if (!ready)
        CLIO_ReportAssertionFailure("file->objectdata && file->compiler", "CLWriteObjectFile.c", 0x16);
    OS_OSSpec_To_FSSpec(&self->outputPath, &objectFile);
    OS_OSSpec_To_FSSpec(&self->inputPath, &sourceFile);
    if (optsCmdLine.verbose != 0) {
        unsigned char *message = (self->temporaryOutputMask & 2) ? (unsigned char *)"temporary " : (unsigned char *)"";
        result = OS_SpecToStringRelative(&self->outputPath, NULL, data_005880e0, 0x104);
        CLErrors_ForwardMessage(0x10, message, result);
    }
    success = CLPlugins_WriteObjectFile(self->selectedPlugin, &sourceFile, &objectFile, option1, option2,
                                        (int)self->objectData);
    if (!success)
        return 0;
    return 1;
}
