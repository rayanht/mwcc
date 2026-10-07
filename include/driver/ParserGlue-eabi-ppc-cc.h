#ifndef DRIVER_PARSERGLUE_EABI_PPC_CC_H
#define DRIVER_PARSERGLUE_EABI_PPC_CC_H

#include "compiler/common.h"
#include "driver/PrefPanels.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Pragma {
    char *value;  /* 0x00: PragmaHasSetting reads the setting byte; ParserHelpers_cc_EmitPragmas checks PR_UNSET */
    char *pragma; /* 0x04: ParserHelpers_cc_EmitPragmas emits the pragma name and tests the table terminator */
    int flags;    /* 0x08: ParserHelpers_cc_EmitPragmas selects normal or reversed on/off settings */
};
extern unsigned int fn_00405670(void);
extern int fn_004056a0(void);
extern int fn_00405710(void);
extern char output_path;
extern struct PtrList data_005876fc[];
extern struct StorageHandle *directive_storage;
extern int fn_00405840(void);
extern PCmdLine pCmdLine;
extern PCmdLineCompiler pCmdLineCompiler;
extern PCmdLineLinker pCmdLineLinker;
extern PBackEnd pBackEnd;
extern PLinker pLinker;
extern PDisassembler pDisassembler;
extern PProject pProject;
extern PCLTExtras pCLTExtras;
extern char useDefaultIncludes;
extern char useFullPaths;
extern PFrontEndC pFrontEndC;
extern PWarningC pWarningC;
extern PGlobalOptimizer pGlobalOptimizer;

#ifdef __cplusplus
}
#endif

#endif
