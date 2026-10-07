#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/cc-eabi-ppc.h"
#include "driver/PrefPanels.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/IrOptimizer.h"
#include "driver/CLPluginRequests.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include "driver/TargetPanels-eabi-ppc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

Boolean DAT_0054c3d0 = 1;

#pragma options align = mac68k
static struct {
    SInt16 version;
    const char *helpFile;
} lbl_0054c3e4 = {1, "CCompiler.hlp"};
#pragma options align = reset

static PFrontEndC frontEndPrefs;
static char data_0057f486;

void cc_eabi_ppc_ReportCompilingFunction(char *name)
{
    char buf[96];
    sprintf(buf, "Compiling function:\t%.64s", name);
    fn_0041b8d0(compiler_plugin_cu.context, buf, "");
}

int __stdcall dispatch_compiler_plugin_request(CWPluginPrivateContext *input)
{
    long firstValue;
    short result;
    int status;
    UInt8 prepResult;
    long secondValue;
    int value;
    long classification;
    struct ValuePairState output;
    status = 0;
    CWPluginsPrivate_GetRequest(input, &classification);
    switch (classification) {
        case -2:
            compiler_plugin_cu.context = input;
            data_00588258 = 0;
            fn_0042c920();
            break;
        case -1:
            compiler_plugin_cu.context = input;
            fn_0042c910();
            if (data_00588258 != 0 && data_0057f486 != 0) {
                fn_0041bc50(compiler_plugin_cu.context, data_00588258);
            }
            data_0057f486 = 0;
            data_00588258 = 0;
            break;
        case 0:
            initialize_compiler_plugin_cu((unsigned int)input);
            if (data_00588258 == 0) {
                CWPluginsPrivate_InvokeCallback40(compiler_plugin_cu.context, "Win32_Plugins_PPC_Nintendo", "1", 0, 0,
                                                  &data_00588258);
                data_0057f486 = 1;
            }
            if (data_00588258 != 0) {
                initialize_copts(&compiler_plugin_cu);
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                TargetPanels_eabi_ppc_LoadCompilerOptions();
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                CodeGen_SetIROptimizationEnabled();
                prepResult = CPrep_Compile(&compiler_plugin_cu);
                status = prepResult;
                if (status != 0) {
                    if ((firstValue = compiler_plugin_cu.objectBuffer) != 0) {
                        fn_0041bcb0(compiler_plugin_cu.context, (struct StorageHandle *)firstValue,
                                    &compiler_plugin_cu.objectData);
                    }
                    if ((secondValue = compiler_plugin_cu.browseBuffer) != 0) {
                        fn_0041bcb0(compiler_plugin_cu.context, (struct StorageHandle *)secondValue,
                                    &compiler_plugin_cu.browseData);
                    }
                    if (compiler_plugin_cu.preprocessOnly != 0) {
                        if (compiler_plugin_cu.objectData != 0) {
                            memset(&output, 0, 10);
                            value = compiler_plugin_cu.objectData;
                            compiler_plugin_cu.objectData = 0;
                            output.secondValue = value;
                            result = CWPluginsPrivate_CallValuePairCallback(compiler_plugin_cu.context, &output);
                        }
                    } else {
                        result = CPrep_CallCompilerCallbackWithValue(compiler_plugin_cu.context,
                                                                     compiler_plugin_cu.mainFileNumber,
                                                                     &compiler_plugin_cu.objectData);
                    }
                    status = result;
                } else {
                    status = 2;
                }
            } else {
                status = 10;
            }
    }
    return CWPluginsPrivate_ReturnArgument(input, status);
}

void initialize_copts(CPrepCU *source)
{
    union {
        PFrontEndC **frontEnd;
        PWarningC **warnings;
        PGlobalOptimizer **optimizer;
    } settingsHandle;
    unsigned char extension[256];
    PWarningC warningPrefs;
    PGlobalOptimizer optimizerPrefs;
    int dotPosition, extensionLength;

    memclrw(&copts, sizeof(copts));
    DropInCompilerLinkerPrivate_CallArgumentValue(compiler_plugin_cu.context, "C/C++ Compiler",
                                                  &settingsHandle.frontEnd);
    frontEndPrefs = **settingsHandle.frontEnd;

    copts.littleendian = 0;
    copts.cplusplus = 1;
    extension[0] = 0;
    copts.objective_c = frontEndPrefs.objective_c;

    for (dotPosition = source->mainFile.fileData.file.name[0]; source->mainFile.fileData.file.name[dotPosition] != '.';
         dotPosition--)
        ;
    if (dotPosition >= 2) {
        for (extensionLength = 0; extensionLength + dotPosition <= source->mainFile.fileData.file.name[0];
             extensionLength++)
            extension[extensionLength] = tolower(source->mainFile.fileData.file.name[extensionLength + dotPosition]);
        extension[extensionLength] = 0;
    }

    if (memcmp(extension, ".c", 3) == 0 || memcmp(extension, ".h", 3) == 0 || memcmp(extension, ".pch", 5) == 0) {
        copts.cplusplus = frontEndPrefs.cplusplus;
    } else if (memcmp(extension, ".m", 3) == 0) {
        copts.cplusplus = frontEndPrefs.cplusplus;
        copts.objective_c = 1;
    } else if (memcmp(extension, ".mm", 4) == 0 || memcmp(extension, ".M", 3) == 0) {
        copts.cplusplus = 1;
        copts.objective_c = 1;
    }

    copts.checkprotos = frontEndPrefs.checkprotos;
    copts.ARM_conform = frontEndPrefs.arm;
    copts.ARMscoping = frontEndPrefs.arm;
    copts.trigraphs = frontEndPrefs.trigraphs;
    copts.onlystdkeywords = frontEndPrefs.onlystdkeywords;
    copts.enumsalwaysint = frontEndPrefs.enumsalwaysint;
    copts.mpwc_relax = frontEndPrefs.mpwpointerstyle;
    copts.ANSIstrict = frontEndPrefs.ansistrict;
    copts.mpwc_newline = frontEndPrefs.mpwcnewline;
    copts.exceptions = frontEndPrefs.enableexceptions;
    copts.dont_reuse_strings = frontEndPrefs.dontreusestrings;
    copts.poolstrings = frontEndPrefs.poolstrings;
    copts.dontinline = frontEndPrefs.dontinline;
    copts.RTTI = frontEndPrefs.useRTTI;
    copts.f54 = (char *)frontEndPrefs.prefixname;
    copts.multibyteaware = frontEndPrefs.multibyteaware;
    copts.unsigned_char = frontEndPrefs.unsignedchars;
    copts.auto_inline = frontEndPrefs.autoinline;
    copts.direct_to_som = frontEndPrefs.direct_to_som;
    copts.SOMCheckEnvironment = frontEndPrefs.som_env_check;
    copts.booltruefalse = frontEndPrefs.booltruefalse;
    copts.alwaysinline = frontEndPrefs.alwaysinline;
    copts.inlineLimit = frontEndPrefs.inlinelevel;
    copts.wchar_type = frontEndPrefs.wchar_type;
    copts.ecplusplus = frontEndPrefs.ecplusplus;
    copts.defer_codegen = frontEndPrefs.defer_codegen;

    DropInCompilerLinkerPrivate_CallArgumentValue(compiler_plugin_cu.context, "C/C++ Warnings",
                                                  &settingsHandle.warnings);
    warningPrefs = **settingsHandle.warnings;
    copts.warn_illpragma = warningPrefs.warn_illpragma;
    copts.warn_emptydecl = warningPrefs.warn_emptydecl;
    copts.warn_possunwant = warningPrefs.warn_possunwant;
    copts.warn_unusedvar = warningPrefs.warn_unusedvar;
    copts.warn_unusedarg = warningPrefs.warn_unusedarg;
    copts.warn_extracomma = warningPrefs.warn_extracomma;
    copts.extended_errorcheck = warningPrefs.pedantic;
    copts.warningerrors = warningPrefs.warningerrors;
    copts.warn_hidevirtual = warningPrefs.warn_hidevirtual;
    copts.warn_implicitconv = warningPrefs.warn_implicitconv;
    copts.warn_notinlined = warningPrefs.warn_notinlined;
    copts.warn_structclass = warningPrefs.warn_structclass;

    DropInCompilerLinkerPrivate_CallArgumentValue(compiler_plugin_cu.context, "EPPC Global Optimizer",
                                                  &settingsHandle.optimizer);
    optimizerPrefs = **settingsHandle.optimizer;
    copts.deleteDeadInstructions = optimizerPrefs.optimizationlevel;
    copts.optimizesize = (optimizerPrefs.optfor == 1);
    copts.unrollOption = 8;
    copts.fd1 = 100;
    copts.filesyminfo = source->filesyminfo;
    copts.precompiledHeaderCreator = 0x43574945;
    copts.precompiledHeaderFileTypes[0] = 0x4d4d4348;
    copts.precompiledHeaderFileTypes[1] = 0x54455854;
}

signed char initialize_compiler_plugin_cu(unsigned int input)
{
    TgtRec readBuffer;
    CWPluginPrivateContext *context;

    memset(&compiler_plugin_cu, 0, sizeof(compiler_plugin_cu));
    compiler_plugin_cu.context = (CWPluginPrivateContext *)input;

    context = (CWPluginPrivateContext *)input;
    if (CWPluginsPrivate_GetRequest(context, &compiler_plugin_cu.pluginRequest))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CWPluginsPrivate_GetAPIVersion(context, &compiler_plugin_cu.apiVersion))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CWPluginsPrivate_GetSourceFile(context, &compiler_plugin_cu.projectFile))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CWPluginsPrivate_GetNumFiles(context, &compiler_plugin_cu.projectFileCount))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetFileIndex(context, (unsigned int *)&compiler_plugin_cu.mainFileNumber))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetContextPayload(context, &compiler_plugin_cu.mainFile))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetResultValues(context, &compiler_plugin_cu.mainFileOffset, &compiler_plugin_cu.mainFileLength))
        return 0;
    if (CPrep_GetEnabled(input, &compiler_plugin_cu.precompiling))
        return 0;
    if (CPrep_GetActive(input, &compiler_plugin_cu.active))
        return 0;
    if (CPrep_GetOperation(input, &compiler_plugin_cu.preprocessOnly))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetSetting(context, &compiler_plugin_cu.filesyminfo))
        return 0;
    if (CPrep_GetReserved15d(input, &compiler_plugin_cu.useMappedPrecompiledHeaders))
        return 0;
    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetDependencyState(context, &compiler_plugin_cu.browseOptions))
        return 0;

    compiler_plugin_cu.compiling = !compiler_plugin_cu.preprocessOnly;
    if (CPrep_GetDependencyOption(input, &compiler_plugin_cu.mainFileAttributes))
        return 0;

    context = (CWPluginPrivateContext *)input;
    if (CPrep_GetTargetSettings(context, &readBuffer))
        return 0;
    compiler_plugin_cu.platformCode1 = readBuffer.head.platformCodes[1];
    compiler_plugin_cu.platformCode0 = readBuffer.head.platformCodes[0];
    return 1;
}
