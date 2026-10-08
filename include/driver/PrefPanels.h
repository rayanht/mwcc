#ifndef DRIVER_PREFPANELS_H
#define DRIVER_PREFPANELS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The preference panels: the command-line tool's ("CmdLine Panel", "CmdLine Environment", "CmdLine Compiler Panel",
 * "CmdLine Linker Panel"), the C/C++ compiler's ("C/C++ Compiler", "C/C++ Warnings", "EPPC Global Optimizer") and the
 * PowerPC EABI tools' ("PPC EABI CodeGen", "PPC EABI Linker", "PPC EABI Disassembler", "PPC EABI Project",
 * "CmdLine Extras EPPC"). */
#pragma options align = mac68k
struct PCmdLine {
    UInt16 version;
    SInt16 state;
    SInt16 stages;
    SInt16 toDisk;
    SInt16 outNameOwner;
    SInt8 dryRun;
    unsigned char debugInfo;
    short verbose;
    char showLines;
    SInt8 timeWorking;
    char noWarnings;
    char warningsAreErrors;
    short maxErrors;
    short maxWarnings;
    short msgStyle;
    SInt8 noWrapOutput;
    char stderr2stdout;
    char noCmdLineWarnings;
};
struct PCmdLineEnvir {
    UInt16 version;
    SInt16 cols;
    short rows;
    char underIDE;
};
struct PCmdLineCompiler {
    UInt16 version;
    char noSysPath;
    char noFail;
    SInt16 includeSearch;
    char linkerName[64];
    char objFileExt[15];
    char browseFileExt[15];
    char ppFileExt[15];
    char disFileExt[15];
    char depFileExt[15];
    char pchFileExt[15];
    SInt32 objFileCreator;
    SInt32 objFileType;
    int browseFileCreator;
    int browseFileType;
    UInt32 ppFileCreator;
    UInt32 ppFileType;
    UInt32 disFileCreator;
    UInt32 disFileType;
    UInt32 depFileCreator;
    UInt32 depFileType;
    UInt8 compileIgnored;
    char relPathInOutputDir;
    char browserEnabled;
    char depsOnlyUserFiles;
    char outMakefile[256];
    UInt8 forcePrecompile;
    char ignoreMissingFiles;
    UInt8 printHeaderNames;
    SInt8 sbmState;
    char sbmPath[256];
    Boolean canonicalIncludes;
    Boolean keepObjects;
};
struct PCmdLineLinker {
    UInt16 version;
    SInt8 callPreLinker;
    SInt8 callPostLinker;
    SInt8 keepLinkerOutput;
    SInt8 callLinker;
};
struct PFrontEndC {
    short version;
    Boolean cplusplus;
    Boolean checkprotos;
    Boolean arm;
    Boolean trigraphs;
    Boolean onlystdkeywords;
    Boolean enumsalwaysint;
    Boolean mpwpointerstyle;
    unsigned char prefixname[32];
    Boolean ansistrict;
    Boolean mpwcnewline;
    Boolean wchar_type;
    Boolean enableexceptions;
    Boolean dontreusestrings;
    Boolean poolstrings;
    Boolean dontinline;
    Boolean useRTTI;
    Boolean multibyteaware;
    Boolean unsignedchars;
    Boolean autoinline;
    Boolean booltruefalse;
    Boolean direct_to_som;
    Boolean som_env_check;
    Boolean alwaysinline;
    short inlinelevel;
    Boolean ecplusplus;
    Boolean objective_c;
    Boolean defer_codegen;
};
struct PWarningC {
    short version;
    Boolean warn_illpragma;
    Boolean warn_emptydecl;
    Boolean warn_possunwant;
    Boolean warn_unusedvar;
    Boolean warn_unusedarg;
    Boolean warn_extracomma;
    Boolean pedantic;
    Boolean warningerrors;
    Boolean warn_hidevirtual;
    Boolean warn_implicitconv;
    Boolean warn_notinlined;
    Boolean warn_structclass;
};
struct PGlobalOptimizer {
    short version;
    Boolean optimizationlevel;
    char optfor;
    UInt8 reserved[8];
};
struct PBackEnd {
    short version;
    UInt8 structalignment;
    UInt8 readonlystrings;
    UInt8 pooldata;
    UInt8 unk05;
    UInt8 profiler;
    UInt8 unk07;
    UInt8 peephole;
    UInt8 unk09;
    UInt8 unk0a;
    UInt8 schedule;
    UInt8 unk0c;
    UInt8 common;
    UInt8 fpmode;
    UInt8 use_lmw_stmw;
    short processor;
    UInt8 funcalign;
    UInt8 fp_contract;
    UInt8 altivec;
    UInt8 vrsave;
    UInt8 unk16[6];
};
struct PLinker {
    short version;
    char generatesyminfo;
    char fullpaths;
    UInt8 generatemap;
    UInt8 unk05;
    UInt8 generatesrec;
    UInt8 listunused;
    UInt8 uselcf;
    UInt8 hascodeaddr;
    UInt8 hasdataaddr;
    UInt8 hassdataaddr;
    UInt8 unk0c;
    UInt8 hasstackaddr;
    UInt8 hasheapaddr;
    UInt8 hasromaddr;
    UInt32 codeaddr;
    UInt32 dataaddr;
    UInt32 sdataaddr;
    UInt32 sdata2addr;
    UInt32 stackaddr;
    UInt32 rambuffer;
    UInt32 romaddr;
    short sreclength;
    UInt8 sreceol;
    UInt8 unk2f;
    char mainname[64];
    UInt32 heapaddr;
};
struct PDisassembler {
    short version;
    UInt8 showdetail;
    UInt8 showdebug;
    UInt8 relocate;
    UInt8 unk05;
    UInt8 showcode;
    UInt8 extended;
    UInt8 nobinary;
    UInt8 showdata;
    UInt8 showxtables;
    UInt8 showheaders;
    UInt8 showtables;
    UInt8 unk0d;
};
struct PProject {
    short version;
    UInt8 unk02[34];
    UInt32 heapsize;
    UInt32 stacksize;
    char bigendian;
    UInt8 unk2d;
    short sdatathreshold;
    short sdata2threshold;
    short codemodel;
    UInt8 unk34[3];
    UInt8 unk37;
    UInt8 strip;
    UInt8 optpartial;
    UInt8 resolvedpartial;
    UInt8 unk3b;
};
struct PCLTExtras {
    short version;
    char mapfilename[256];
    char srecfilename[256];
};
#pragma options align = reset

#ifdef __cplusplus
}
#endif

#endif
