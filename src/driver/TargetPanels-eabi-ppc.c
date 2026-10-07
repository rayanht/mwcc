#include "compiler/common.h"
#include "driver/TargetPanels-eabi-ppc.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>

static SInt16 data_0057f6a8;

void fn_0042c0c0(void)
{
    return;
}

static __inline void SetProcessor(short processor)
{
    copts.processor = processor;
}

static __inline void LoadPreferenceResource(const void *name, void ***resource)
{
    DropInCompilerLinkerPrivate_CallArgumentValue(compiler_plugin_cu.context, (const char *)name, resource);
}

static __inline struct CompilerOptions ReadCompilerOptions(void **resource)
{
    return *(struct CompilerOptions *)*resource;
}

static __inline CompilerSettings ReadCompilerSettings(void **resource)
{
    return *(CompilerSettings *)*resource;
}

static __inline ExtendedCompilerSettings ReadExtendedCompilerSettings(void **resource)
{
    return *(ExtendedCompilerSettings *)*resource;
}

static inline void SetProcessorModel(SInt8 model)
{
    copts.processorModel = model;
}

static inline void SetCodeAlignment(UInt8 alignment)
{
    copts.codeAlignment = alignment;
}

static inline void SetFloatingPointSupport(Boolean enabled)
{
    if (enabled)
        copts.instructionSchedulingMode = 2;
    else
        copts.instructionSchedulingMode = 0;
}

static inline void SetTargetFeatureEnabled(Boolean enabled)
{
    if (enabled)
        copts.altivecVrsave = 1;
    else
        copts.altivecVrsave = 0;
}

void TargetPanels_eabi_ppc_LoadCompilerOptions(void)
{
    struct CompilerOptions options;
    CompilerSettings settings;
    ExtendedCompilerSettings extendedSettings;
    void **resource;

    LoadPreferenceResource((unsigned char *)"PPC EABI CodeGen", &resource);
    options = ReadCompilerOptions(resource);
    data_0057f6a8 = options.formatVersion;
    LoadPreferenceResource((unsigned char *)"PPC EABI Project", &resource);
    settings = ReadCompilerSettings(resource);
    LoadPreferenceResource((unsigned char *)"PPC EABI Linker", &resource);
    extendedSettings = ReadExtendedCompilerSettings(resource);

    cprep_cu = (UInt8 *)&compiler_plugin_cu;
    SetCodeAlignment(4);
    copts.altivecModel = 0;
    SetTargetFeatureEnabled(1);
    copts.emitSerializedAssemblyFormat = 1;
    copts.nativeByteOrder = !settings.reverseByteOrder;
    copts.fb2 = options.unk03;
    SetFloatingPointSupport(1);

    switch (options.formatVersion) {
        case 6:
        case 7:
        case 8:
        case 9:
            switch (options.processor) {
                case 0:
                    SetProcessor(0);
                    break;
                case 1:
                    SetProcessor(1);
                    break;
                case 2:
                    SetProcessor(2);
                    break;
                case 3:
                    SetProcessor(3);
                    break;
                case 4:
                    SetProcessor(4);
                    break;
                case 5:
                    SetProcessor(5);
                    break;
                case 6:
                    SetProcessor(6);
                    break;
                case 7:
                    SetProcessor(7);
                    break;
                case 8:
                    SetProcessor(8);
                    break;
                case 9:
                    SetProcessor(9);
                    break;
                case 10:
                    SetProcessor(10);
                    break;
                case 11:
                    SetProcessor(11);
                    break;
                case 12:
                    SetProcessor(12);
                    break;
                case 13:
                    SetProcessor(13);
                    break;
                case 14:
                    SetProcessor(14);
                    break;
                case 15:
                    SetProcessor(15);
                    break;
                case 16:
                    SetProcessor(16);
                    break;
                case 17:
                    SetProcessor(17);
                    break;
                case 18:
                    SetProcessor(19);
                    break;
                case 19:
                default:
                    SetProcessor(20);
                    break;
            }
            /* Fall through for older option formats. */
        case 5:
            switch (options.processor) {
                case 0:
                    SetProcessor(0);
                    break;
                case 1:
                    SetProcessor(1);
                    break;
                case 2:
                    SetProcessor(2);
                    break;
                case 3:
                    SetProcessor(3);
                    break;
                case 4:
                    SetProcessor(5);
                    break;
                case 5:
                    SetProcessor(6);
                    break;
                case 6:
                    SetProcessor(7);
                    break;
                case 7:
                    SetProcessor(8);
                    break;
                case 8:
                    SetProcessor(9);
                    break;
                case 9:
                    SetProcessor(10);
                    break;
                case 10:
                    SetProcessor(11);
                    break;
                case 11:
                    SetProcessor(12);
                    break;
                case 12:
                    SetProcessor(13);
                    break;
                case 13:
                    SetProcessor(14);
                    break;
                case 14:
                    SetProcessor(15);
                    break;
                case 15:
                    SetProcessor(16);
                    break;
                case 16:
                    SetProcessor(17);
                    break;
                case 17:
                default:
                    SetProcessor(20);
                    break;
            }
            /* Fall through for older option formats. */
        case 4:
            switch (options.processor) {
                case 0:
                    SetProcessor(0);
                    break;
                case 1:
                    SetProcessor(1);
                    break;
                case 2:
                    SetProcessor(2);
                    break;
                case 3:
                    SetProcessor(3);
                    break;
                case 4:
                    SetProcessor(5);
                    break;
                case 5:
                    SetProcessor(6);
                    break;
                case 6:
                    SetProcessor(7);
                    break;
                case 7:
                    SetProcessor(8);
                    break;
                case 8:
                    SetProcessor(9);
                    break;
                case 9:
                    SetProcessor(10);
                    break;
                case 10:
                    SetProcessor(11);
                    break;
                case 11:
                    SetProcessor(12);
                    break;
                case 12:
                    SetProcessor(13);
                    break;
                case 13:
                    SetProcessor(14);
                    break;
                case 14:
                    SetProcessor(15);
                    break;
                case 15:
                    SetProcessor(17);
                    break;
                case 16:
                default:
                    SetProcessor(16);
                    break;
            }
            /* Fall through for older option formats. */
        case 3:
            switch (options.processor) {
                case 0:
                    SetProcessor(0);
                    break;
                case 1:
                    SetProcessor(1);
                    break;
                case 2:
                    SetProcessor(2);
                    break;
                case 3:
                    SetProcessor(3);
                    break;
                case 4:
                    SetProcessor(5);
                    break;
                case 5:
                    SetProcessor(6);
                    break;
                case 6:
                    SetProcessor(7);
                    break;
                case 7:
                    SetProcessor(9);
                    break;
                case 8:
                    SetProcessor(11);
                    break;
                case 9:
                    SetProcessor(12);
                    break;
                case 10:
                    SetProcessor(13);
                    break;
                case 11:
                    SetProcessor(14);
                    break;
                case 12:
                    SetProcessor(15);
                    break;
                case 13:
                    SetProcessor(17);
                    break;
                case 14:
                default:
                    SetProcessor(20);
                    break;
            }
            /* Fall through for older option formats. */
        case 1:
        case 2:
            switch (options.processor) {
                case 0:
                    SetProcessor(0);
                    break;
                case 1:
                    SetProcessor(1);
                    break;
                case 2:
                    SetProcessor(2);
                    break;
                case 3:
                    SetProcessor(3);
                    break;
                case 4:
                    SetProcessor(5);
                    break;
                case 5:
                    SetProcessor(6);
                    break;
                case 6:
                    SetProcessor(7);
                    break;
                case 7:
                    SetProcessor(9);
                    break;
                case 8:
                    SetProcessor(11);
                    break;
                case 9:
                    SetProcessor(12);
                    break;
                case 10:
                    SetProcessor(13);
                    break;
                case 11:
                    SetProcessor(14);
                    break;
                case 12:
                    SetProcessor(17);
                    break;
                case 13:
                    SetProcessor(20);
                    break;
            }
            break;
        default:
            SetProcessor(options.processor);
    }

    switch (copts.processor) {
        case 5:
            SetProcessorModel(1);
            break;
        case 7:
            SetProcessorModel(2);
            break;
        case 8:
        case 18:
        case 19:
            SetProcessorModel(5);
            break;
        case 9:
            SetProcessorModel(3);
            break;
        case 10:
            SetProcessorModel(6);
            break;
        case 11:
        case 12:
            SetProcessorModel(4);
            break;
        default:
            SetProcessorModel(8);
            break;
    }

    SetFloatingPointSupport(options.floatingPointSupportEnabled);

    copts.peepholeOptimizationEnabled = options.unk08;
    copts.structalignment = options.structAlignment;
    SetCodeAlignment(4 << options.powerOfTwoShift);
    copts.reuseSectionSymbols = options.reuseSectionSymbols;
    copts.smallBSSLimit = settings.smallBSSLimit;
    copts.smallDataLimit = settings.smallDataLimit;
    copts.f27 = settings.unk37;
    copts.f1e = 0;
    copts.fd3 = 0;
    copts.fd5 = 1;
    copts.fd4 = 1;
    copts.f20 = options.unk0d;
    copts.fd6 = extendedSettings.preferenceData[3];
    copts.useRegisterSaveHelpers = options.useRegisterSaveHelpers;
    copts.rel109Offset = 2;
    copts.debugEnabled = 0;
    copts.operandsDebug = 0;
    copts.debugOptions = 0;
    if (options.debugMode != 0) {
        copts.debugEnabled = 1;
        if (options.debugMode == 1)
            copts.operandsDebug = 1;
        else
            copts.debugOptions = options.debugOptions;
    }
    copts.altivecModel = options.builtinStructConversionsEnabled;
    SetTargetFeatureEnabled(options.targetFeatureEnabled);
    copts.cOptimizerDumpEnabled = 0;
    copts.ppcUnrollSpeculative = 1;
    copts.ppcUnrollInstructionsLimit = 100;
    copts.ppcUnrollFactorLimit = 10;
}

void fn_0042c910(void)
{
    return;
}
