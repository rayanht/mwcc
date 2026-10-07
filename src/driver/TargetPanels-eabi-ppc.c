#include "compiler/common.h"
#include "driver/TargetPanels-eabi-ppc.h"
#include "driver/PrefPanels.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>

static SInt16 data_0057f6a8;

static __inline void SetProcessor(short processor)
{
    copts.processor = processor;
}

static __inline void LoadPreferenceResource(const void *name, void ***resource)
{
    DropInCompilerLinkerPrivate_CallArgumentValue(compiler_plugin_cu.context, (const char *)name, resource);
}

static __inline PBackEnd ReadCodeGenPrefs(void **resource)
{
    return *(PBackEnd *)*resource;
}

static __inline PProject ReadProjectPrefs(void **resource)
{
    return *(PProject *)*resource;
}

static __inline PLinker ReadLinkerPrefs(void **resource)
{
    return *(PLinker *)*resource;
}

static inline void SetProcessorModel(SInt8 model)
{
    copts.processorModel = model;
}

static inline void SetCodeAlignment(UInt8 alignment)
{
    copts.codeAlignment = alignment;
}

static inline void SetScheduling(Boolean enabled)
{
    if (enabled)
        copts.instructionSchedulingMode = 2;
    else
        copts.instructionSchedulingMode = 0;
}

static inline void SetVRSave(Boolean enabled)
{
    if (enabled)
        copts.altivec_vrsave = 1;
    else
        copts.altivec_vrsave = 0;
}

void fn_0042c910(void)
{
    return;
}

void TargetPanels_eabi_ppc_LoadCompilerOptions(void)
{
    PBackEnd options;
    PProject settings;
    PLinker extendedSettings;
    void **resource;

    LoadPreferenceResource((unsigned char *)"PPC EABI CodeGen", &resource);
    options = ReadCodeGenPrefs(resource);
    data_0057f6a8 = options.version;
    LoadPreferenceResource((unsigned char *)"PPC EABI Project", &resource);
    settings = ReadProjectPrefs(resource);
    LoadPreferenceResource((unsigned char *)"PPC EABI Linker", &resource);
    extendedSettings = ReadLinkerPrefs(resource);

    cprep_cu = (UInt8 *)&compiler_plugin_cu;
    SetCodeAlignment(4);
    copts.altivec_model = 0;
    SetVRSave(1);
    copts.catssupport = 1;
    copts.littleendian = !settings.bigendian;
    copts.readonly_strings = options.readonlystrings;
    SetScheduling(1);

    switch (options.version) {
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

    SetScheduling(options.schedule);

    copts.peephole = options.peephole;
    copts.structalignment = options.structalignment;
    SetCodeAlignment(4 << options.funcalign);
    copts.usedatapool = options.pooldata;
    copts.constsmalldatathreshold = settings.sdata2threshold;
    copts.nonconstsmalldatathreshold = settings.sdatathreshold;
    copts.f27 = settings.unk37;
    copts.f1e = 0;
    copts.fd3 = 0;
    copts.fd5 = 1;
    copts.fd4 = 1;
    copts.commonblocks = options.common;
    copts.fd6 = extendedSettings.fullpaths;
    copts.use_lmw_stmw = options.use_lmw_stmw;
    copts.rel109_offset = 2;
    copts.debugEnabled = 0;
    copts.operandsDebug = 0;
    copts.fp_contract = 0;
    if (options.fpmode != 0) {
        copts.debugEnabled = 1;
        if (options.fpmode == 1)
            copts.operandsDebug = 1;
        else
            copts.fp_contract = options.fp_contract;
    }
    copts.altivec_model = options.altivec;
    SetVRSave(options.vrsave);
    copts.debug_listing = 0;
    copts.unroll_speculative = 1;
    copts.unroll_instr_limit = 100;
    copts.unroll_factor_limit = 10;
}

void fn_0042c0c0(void)
{
    return;
}
