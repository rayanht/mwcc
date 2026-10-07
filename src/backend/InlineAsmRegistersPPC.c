#include "compiler/common.h"
#include "compiler/InlineAsmRegistersPPC.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CBrowse.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/FunctionCalls.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/InlineAsmRegisters.h"

#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#pragma pool_strings on

/* The registers: name, kind and number. */
static struct RegistrationEntry registration_entries[] = {
    {"r0", 0, 0},     {"r1", 0, 1},     {"r2", 0, 2},     {"r3", 0, 3},     {"r4", 0, 4},     {"r5", 0, 5},
    {"r6", 0, 6},     {"r7", 0, 7},     {"r8", 0, 8},     {"r9", 0, 9},     {"r10", 0, 10},   {"r11", 0, 11},
    {"r12", 0, 12},   {"r13", 0, 13},   {"r14", 0, 14},   {"r15", 0, 15},   {"r16", 0, 16},   {"r17", 0, 17},
    {"r18", 0, 18},   {"r19", 0, 19},   {"r20", 0, 20},   {"r21", 0, 21},   {"r22", 0, 22},   {"r23", 0, 23},
    {"r24", 0, 24},   {"r25", 0, 25},   {"r26", 0, 26},   {"r27", 0, 27},   {"r28", 0, 28},   {"r29", 0, 29},
    {"r30", 0, 30},   {"r31", 0, 31},   {"gpr0", 0, 0},   {"gpr1", 0, 1},   {"gpr2", 0, 2},   {"gpr3", 0, 3},
    {"gpr4", 0, 4},   {"gpr5", 0, 5},   {"gpr6", 0, 6},   {"gpr7", 0, 7},   {"gpr8", 0, 8},   {"gpr9", 0, 9},
    {"gpr10", 0, 10}, {"gpr11", 0, 11}, {"gpr12", 0, 12}, {"gpr13", 0, 13}, {"gpr14", 0, 14}, {"gpr15", 0, 15},
    {"gpr16", 0, 16}, {"gpr17", 0, 17}, {"gpr18", 0, 18}, {"gpr19", 0, 19}, {"gpr20", 0, 20}, {"gpr21", 0, 21},
    {"gpr22", 0, 22}, {"gpr23", 0, 23}, {"gpr24", 0, 24}, {"gpr25", 0, 25}, {"gpr26", 0, 26}, {"gpr27", 0, 27},
    {"gpr28", 0, 28}, {"gpr29", 0, 29}, {"gpr30", 0, 30}, {"gpr31", 0, 31}, {"rtoc", 0, 2},   {"RTOC", 0, 2},
    {"sp", 0, 1},     {"SP", 0, 1},     {"rsp", 0, 1},    {"RSP", 0, 1},    {"f0", 1, 0},     {"f1", 1, 1},
    {"f2", 1, 2},     {"f3", 1, 3},     {"f4", 1, 4},     {"f5", 1, 5},     {"f6", 1, 6},     {"f7", 1, 7},
    {"f8", 1, 8},     {"f9", 1, 9},     {"f10", 1, 10},   {"f11", 1, 11},   {"f12", 1, 12},   {"f13", 1, 13},
    {"f14", 1, 14},   {"f15", 1, 15},   {"f16", 1, 16},   {"f17", 1, 17},   {"f18", 1, 18},   {"f19", 1, 19},
    {"f20", 1, 20},   {"f21", 1, 21},   {"f22", 1, 22},   {"f23", 1, 23},   {"f24", 1, 24},   {"f25", 1, 25},
    {"f26", 1, 26},   {"f27", 1, 27},   {"f28", 1, 28},   {"f29", 1, 29},   {"f30", 1, 30},   {"f31", 1, 31},
    {"fp0", 1, 0},    {"fp1", 1, 1},    {"fp2", 1, 2},    {"fp3", 1, 3},    {"fp4", 1, 4},    {"fp5", 1, 5},
    {"fp6", 1, 6},    {"fp7", 1, 7},    {"fp8", 1, 8},    {"fp9", 1, 9},    {"fp10", 1, 10},  {"fp11", 1, 11},
    {"fp12", 1, 12},  {"fp13", 1, 13},  {"fp14", 1, 14},  {"fp15", 1, 15},  {"fp16", 1, 16},  {"fp17", 1, 17},
    {"fp18", 1, 18},  {"fp19", 1, 19},  {"fp20", 1, 20},  {"fp21", 1, 21},  {"fp22", 1, 22},  {"fp23", 1, 23},
    {"fp24", 1, 24},  {"fp25", 1, 25},  {"fp26", 1, 26},  {"fp27", 1, 27},  {"fp28", 1, 28},  {"fp29", 1, 29},
    {"fp30", 1, 30},  {"fp31", 1, 31},  {"v0", 9, 0},     {"v1", 9, 1},     {"v2", 9, 2},     {"v3", 9, 3},
    {"v4", 9, 4},     {"v5", 9, 5},     {"v6", 9, 6},     {"v7", 9, 7},     {"v8", 9, 8},     {"v9", 9, 9},
    {"v10", 9, 10},   {"v11", 9, 11},   {"v12", 9, 12},   {"v13", 9, 13},   {"v14", 9, 14},   {"v15", 9, 15},
    {"v16", 9, 16},   {"v17", 9, 17},   {"v18", 9, 18},   {"v19", 9, 19},   {"v20", 9, 20},   {"v21", 9, 21},
    {"v22", 9, 22},   {"v23", 9, 23},   {"v24", 9, 24},   {"v25", 9, 25},   {"v26", 9, 26},   {"v27", 9, 27},
    {"v28", 9, 28},   {"v29", 9, 29},   {"v30", 9, 30},   {"v31", 9, 31},   {"vr0", 9, 0},    {"vr1", 9, 1},
    {"vr2", 9, 2},    {"vr3", 9, 3},    {"vr4", 9, 4},    {"vr5", 9, 5},    {"vr6", 9, 6},    {"vr7", 9, 7},
    {"vr8", 9, 8},    {"vr9", 9, 9},    {"vr10", 9, 10},  {"vr11", 9, 11},  {"vr12", 9, 12},  {"vr13", 9, 13},
    {"vr14", 9, 14},  {"vr15", 9, 15},  {"vr16", 9, 16},  {"vr17", 9, 17},  {"vr18", 9, 18},  {"vr19", 9, 19},
    {"vr20", 9, 20},  {"vr21", 9, 21},  {"vr22", 9, 22},  {"vr23", 9, 23},  {"vr24", 9, 24},  {"vr25", 9, 25},
    {"vr26", 9, 26},  {"vr27", 9, 27},  {"vr28", 9, 28},  {"vr29", 9, 29},  {"vr30", 9, 30},  {"vr31", 9, 31},
    {"cr0", 3, 0},    {"cr1", 3, 1},    {"cr2", 3, 2},    {"cr3", 3, 3},    {"cr4", 3, 4},    {"cr5", 3, 5},
    {"cr6", 3, 6},    {"cr7", 3, 7},    {"crf0", 3, 0},   {"crf1", 3, 1},   {"crf2", 3, 2},   {"crf3", 3, 3},
    {"crf4", 3, 4},   {"crf5", 3, 5},   {"crf6", 3, 6},   {"crf7", 3, 7},   {"lt", 8, 0},     {"gt", 8, 1},
    {"eq", 8, 2},     {"so", 8, 3},     {"un", 8, 3},     {"LT", 8, 0},     {"GT", 8, 1},     {"EQ", 8, 2},
    {"SO", 8, 3},     {"UN", 8, 3},     {NULL, 0, 0},
};

/* The special registers: name, number and processors. */
static struct RegistrationTableEntry registration_table[] = {
    {"xer", 1, 0, 0xFFFFF},
    {"lr", 8, 0, 0xFFFFF},
    {"ctr", 9, 0, 0xFFFFF},
    {"mq", 0, 0, 0x1},
    {"rtcu", 4, 0, 0x1},
    {"rtcl", 5, 0, 0x1},
    {"dsisr", 18, 0, 0xFF83F},
    {"dar", 19, 0, 0xFF83F},
    {"dec", 22, 0, 0xFF83F},
    {"sdr1", 25, 0, 0xFE00F},
    {"srr0", 26, 0, 0xFFFFF},
    {"srr1", 27, 0, 0xFFFFF},
    {"eie", 80, 0, 0x1830},
    {"eid", 81, 0, 0x1830},
    {"nri", 82, 0, 0x1830},
    {"cmpa", 144, 0, 0x1830},
    {"cmpb", 145, 0, 0x1830},
    {"cmpc", 146, 0, 0x1830},
    {"cmpd", 147, 0, 0x1830},
    {"icr", 148, 0, 0x30},
    {"ecr", 148, 0, 0x1800},
    {"der", 149, 0, 0x1830},
    {"counta", 150, 0, 0x1830},
    {"countb", 151, 0, 0x1830},
    {"cmpe", 152, 0, 0x1830},
    {"cmpf", 153, 0, 0x1830},
    {"cmpg", 154, 0, 0x1830},
    {"cmph", 155, 0, 0x1830},
    {"lctrl1", 156, 0, 0x1830},
    {"lctrl2", 157, 0, 0x1830},
    {"ictrl", 158, 0, 0x1830},
    {"bar", 159, 0, 0x1830},
    {"vrsave", 256, 0, 0x40000000},
    {"sprg0", 272, 0, 0xFFFFF},
    {"sprg1", 273, 0, 0xFFFFF},
    {"sprg2", 274, 0, 0xFFFFF},
    {"sprg3", 275, 0, 0xFFFFF},
    {"ear", 282, 0, 0xFE7CF},
    {"tbl", 284, 0, 0xFF83F},
    {"tbu", 285, 0, 0xFF83F},
    {"tbl_write", 284, 0, 0xFF83F},
    {"tbu_write", 285, 0, 0xFF83F},
    {"pvr", 287, 0, 0xFFFFF},
    {"ibat0u", 528, 0, 0xFE7CF},
    {"mi_gra", 528, 0, 0x1000},
    {"ibat0l", 529, 0, 0xFE7CF},
    {"ibat1u", 530, 0, 0xFE7CF},
    {"ibat1l", 531, 0, 0xFE7CF},
    {"ibat2u", 532, 0, 0xFE7CF},
    {"ibat2l", 533, 0, 0xFE7CF},
    {"ibat3u", 534, 0, 0xFE7CF},
    {"ibat3l", 535, 0, 0xFE7CF},
    {"dbat0u", 536, 0, 0xFE7CE},
    {"l2u_gra", 536, 0, 0x1000},
    {"dbat0l", 537, 0, 0xFE7CE},
    {"dbat1u", 538, 0, 0xFE7CE},
    {"dbat1l", 539, 0, 0xFE7CE},
    {"dbat2u", 540, 0, 0xFE7CE},
    {"dbat2l", 541, 0, 0xFE7CE},
    {"dbat3u", 542, 0, 0xFE7CE},
    {"dbat3l", 543, 0, 0xFE7CE},
    {"ic_cst", 560, 0, 0x30},
    {"iccst", 560, 0, 0x800},
    {"bbcmcr", 560, 0, 0x1000},
    {"ic_adr", 561, 0, 0x30},
    {"icadr", 561, 0, 0x800},
    {"ic_dat", 562, 0, 0x30},
    {"icdat", 562, 0, 0x800},
    {"dc_cst", 568, 0, 0x30},
    {"l2u_mcr", 568, 0, 0x1000},
    {"dc_adr", 569, 0, 0x30},
    {"dc_dat", 570, 0, 0x30},
    {"dpdr", 630, 0, 0x1830},
    {"dpir", 631, 0, 0x30},
    {"immr", 638, 0, 0x30},
    {"mi_ctr", 784, 0, 0x30},
    {"mi_rba0", 784, 0, 0x1000},
    {"mi_rba1", 785, 0, 0x1000},
    {"mi_rba2", 786, 0, 0x1000},
    {"mi_ap", 786, 0, 0x30},
    {"mi_epn", 787, 0, 0x30},
    {"mi_rba3", 787, 0, 0x1000},
    {"mi_twc", 789, 0, 0x30},
    {"mi_l1dl2p", 789, 0, 0x30},
    {"mi_rpn", 790, 0, 0x30},
    {"md_ctr", 792, 0, 0x30},
    {"l2u_rba0", 792, 0, 0x1000},
    {"l2u_rba1", 793, 0, 0x1000},
    {"m_casid", 793, 0, 0x30},
    {"md_ap", 794, 0, 0x30},
    {"l2u_rba2", 794, 0, 0x1000},
    {"l2u_rba3", 795, 0, 0x1000},
    {"md_epn", 795, 0, 0x30},
    {"m_twb", 796, 0, 0x30},
    {"md_l1p", 796, 0, 0x30},
    {"md_twc", 797, 0, 0x30},
    {"md_l1dl2p", 797, 0, 0x30},
    {"md_rpn", 798, 0, 0x30},
    {"m_tw", 799, 0, 0x30},
    {"m_save", 799, 0, 0x30},
    {"mi_dbcam", 816, 0, 0x10},
    {"mi_cam", 816, 0, 0x20},
    {"mi_ra0", 816, 0, 0x1000},
    {"mi_ra1", 817, 0, 0x1000},
    {"mi_dbram0", 817, 0, 0x10},
    {"mi_ram0", 817, 0, 0x20},
    {"mi_dbram1", 818, 0, 0x10},
    {"mi_ram1", 818, 0, 0x20},
    {"mi_ra2", 818, 0, 0x1000},
    {"mi_ra3", 819, 0, 0x1000},
    {"md_dbcam", 824, 0, 0x10},
    {"md_cam", 824, 0, 0x20},
    {"l2u_ra0", 824, 0, 0x1000},
    {"l2u_ra1", 825, 0, 0x1000},
    {"md_dbram0", 825, 0, 0x10},
    {"md_ram0", 825, 0, 0x20},
    {"md_dbram1", 826, 0, 0x10},
    {"md_ram1", 826, 0, 0x20},
    {"l2u_ra2", 826, 0, 0x1000},
    {"l2u_ra3", 827, 0, 0x1000},
    {"gqr0", 912, 0, 0x20000000},
    {"gqr1", 913, 0, 0x20000000},
    {"gqr2", 914, 0, 0x20000000},
    {"gqr3", 915, 0, 0x20000000},
    {"gqr4", 916, 0, 0x20000000},
    {"gqr5", 917, 0, 0x20000000},
    {"gqr6", 918, 0, 0x20000000},
    {"gqr7", 919, 0, 0x20000000},
    {"hid_g", 920, 0, 0x20000000},
    {"wpar", 921, 0, 0x20000000},
    {"dma_u", 922, 0, 0x20000000},
    {"dma_l", 923, 0, 0x20000000},
    {"ummcr0", 936, 0, 0x6000},
    {"upmc1", 937, 0, 0x6000},
    {"upmc2", 938, 0, 0x6000},
    {"usia", 939, 0, 0x6000},
    {"ummcr1", 940, 0, 0x6000},
    {"upmc3", 941, 0, 0x6000},
    {"upmc4", 942, 0, 0x6000},
    {"zpr", 944, 0, 0x200},
    {"pid", 945, 0, 0x200},
    {"mmcr0", 952, 0, 0x6008},
    {"pmc1", 953, 0, 0x6008},
    {"sgr", 953, 0, 0x240},
    {"pmc2", 954, 0, 0x6008},
    {"dcwr", 954, 0, 0x240},
    {"sia", 955, 0, 0x6008},
    {"sler", 955, 0, 0x40},
    {"mmcr1", 956, 0, 0x6000},
    {"pmc3", 957, 0, 0x6000},
    {"pmc4", 958, 0, 0x6000},
    {"sda", 959, 0, 0x8},
    {"tbhu", 972, 0, 0x240},
    {"tblu", 973, 0, 0x240},
    {"dmiss", 976, 0, 0x8006},
    {"dcmp", 977, 0, 0x8006},
    {"hash1", 978, 0, 0x8006},
    {"hash2", 979, 0, 0x8006},
    {"icdbdr", 979, 0, 0x7C0},
    {"imiss", 980, 0, 0x8006},
    {"esr", 980, 0, 0x7C0},
    {"icmp", 981, 0, 0x8006},
    {"dear", 981, 0, 0x7C0},
    {"rpa", 982, 0, 0x8006},
    {"evpr", 982, 0, 0x7C0},
    {"cdbcr", 983, 0, 0x7C0},
    {"tsr", 984, 0, 0x7C0},
    {"tcr", 984, 0, 0x2},
    {"tcr", 986, 0, 0x7C0},
    {"ibr", 986, 0, 0x2},
    {"pit", 987, 0, 0x7C0},
    {"esasrr", 987, 0, 0x2},
    {"tbhi", 988, 0, 0x7C0},
    {"tblo", 989, 0, 0x7C0},
    {"srr2", 990, 0, 0x7C0},
    {"sebr", 990, 0, 0x2},
    {"srr3", 991, 0, 0x7C0},
    {"ser", 991, 0, 0x2},
    {"hid0", 1008, 0, 0xE00F},
    {"dbsr", 1008, 0, 0x80},
    {"hid1", 1009, 0, 0xE007},
    {"hid2", 1010, 0, 0x1},
    {"iabr", 1010, 0, 0xE00F},
    {"dbcr", 1010, 0, 0x7C0},
    {"hid2", 1011, 0, 0x8000},
    {"iac1", 1012, 0, 0x80},
    {"iac", 1012, 0, 0x40},
    {"dabr", 1013, 0, 0x6009},
    {"iac2", 1013, 0, 0x80},
    {"hid5", 1013, 0, 0x1},
    {"dac1", 1014, 0, 0x80},
    {"dac", 1014, 0, 0x40},
    {"dac2", 1015, 0, 0x80},
    {"l2cr", 1017, 0, 0x6000},
    {"dccr", 1018, 0, 0x7C0},
    {"iccr", 1019, 0, 0x7C0},
    {"ictc", 1019, 0, 0x6000},
    {"pbl1", 1020, 0, 0x80},
    {"thrm1", 1020, 0, 0x6000},
    {"pbu1", 1021, 0, 0x80},
    {"thrm2", 1021, 0, 0x6000},
    {"fpecr", 1022, 0, 0x1800},
    {"pbl2", 1022, 0, 0x80},
    {"thrm3", 1022, 0, 0x6000},
    {"pir", 1023, 0, 0x8},
    {"hid15", 1023, 0, 0x1},
    {"pbu2", 1023, 0, 0x80},
    {NULL, 0, 0, 0x0},
};

/* The processors each special register number exists on. */
static unsigned int inline_asm_register_masks[1024] = {
    0x1,        0xFFFFF,    0x0,        0x0,        0x1,        0x1,        0x0,        0x0,        0xFFFFF,
    0xFFFFF,    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0xFF83F,    0xFF83F,    0x0,        0x0,        0xFF83F,    0x0,        0x0,        0xFE00F,    0xFFFFF,
    0xFFFFF,    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x1830,
    0x1830,     0x1830,     0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,
    0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x1830,     0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x40000000, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xFF83F,    0xFF83F,
    0x0,        0x0,        0xFFFFF,    0xFFFFF,    0xFFFFF,    0xFFFFF,    0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0xFE7CF,    0x0,        0xFF83F,    0xFF83F,    0x0,        0xFFFFF,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xFF7CF,    0xFE7CF,    0xFE7CF,
    0xFE7CF,    0xFE7CF,    0xFE7CF,    0xFE7CF,    0xFE7CF,    0xFF7CE,    0xFE7CE,    0xFE7CE,    0xFE7CE,
    0xFE7CE,    0xFE7CE,    0xFE7CE,    0xFE7CE,    0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x1830,     0x1830,     0x1830,     0x0,        0x0,        0x0,        0x0,
    0x0,        0x1030,     0x30,       0x30,       0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x1830,     0x30,       0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x30,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x1030,     0x1000,     0x1030,     0x1030,     0x0,        0x30,       0x30,       0x0,
    0x1030,     0x1030,     0x1030,     0x1030,     0x30,       0x30,       0x30,       0x30,       0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x1030,     0x1030,     0x1030,
    0x1000,     0x0,        0x0,        0x0,        0x0,        0x1030,     0x1030,     0x1030,     0x1000,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000,
    0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x6000,     0x6000,     0x6000,     0x6000,     0x6000,     0x6000,     0x6000,     0x0,        0x200,
    0x200,      0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x6008,     0x6248,
    0x6248,     0x6048,     0x6000,     0x6000,     0x6000,     0x8,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x240,      0x240,      0x0,        0x0,        0x8006,     0x8006,     0x8006,     0x87C6,     0x87C6,
    0x87C6,     0x87C6,     0x7C0,      0x7C2,      0x0,        0x7C2,      0x7C2,      0x7C0,      0x7C0,
    0x7C2,      0x7C2,      0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0xE08F,     0xE007,     0xE7CF,     0x8000,     0xC0,       0x6089,     0xC0,       0x80,       0x0,
    0x6000,     0x7C0,      0x67C0,     0x6080,     0x6080,     0x7880,     0x6089,
};

static struct SecondaryRegistrationEntry secondary_registration_table[] = {
    {64, "exisr", 128},   {66, "exier", 128},   {112, "brh0", 1024},  {113, "brh1", 1024},  {114, "brh2", 1024},
    {115, "brh3", 1024},  {116, "brh4", 1024},  {117, "brh5", 1024},  {118, "brh6", 1024},  {119, "brh7", 1024},
    {128, "br0", 128},    {128, "brcr0", 64},   {129, "br1", 128},    {129, "brcr1", 64},   {130, "br2", 128},
    {130, "brcr2", 64},   {131, "br3", 128},    {131, "brcr3", 64},   {132, "br4", 128},    {132, "brcr4", 64},
    {133, "br5", 128},    {133, "brcr5", 64},   {134, "br6", 128},    {134, "brcr6", 64},   {135, "br7", 128},
    {135, "brcr7", 64},   {144, "bear", 1984},  {145, "besr", 128},   {145, "besr0", 64},   {160, "iocr", 1984},
    {161, "pmcr0", 64},   {192, "dmacr0", 128}, {193, "dmact0", 128}, {194, "dmada0", 128}, {195, "dmasa0", 128},
    {196, "dmacc0", 128}, {200, "dmacr1", 128}, {201, "dmact1", 128}, {202, "dmada1", 128}, {203, "dmasa1", 128},
    {204, "dmacc1", 128}, {208, "dmacr2", 768}, {209, "dmact2", 768}, {210, "dmada2", 768}, {211, "dmasa2", 768},
    {212, "dmacc2", 768}, {216, "dmacr3", 768}, {217, "dmact3", 768}, {218, "dmada3", 768}, {219, "dmasa3", 768},
    {220, "dmacc3", 768}, {224, "dmasr", 128},  {0, NULL, 0},
};

static struct RegistrationHashEntry *inlineAsmRegisterHashTable[64];
static struct RegistrationHashEntry *secondary_registration_hash[64];
static char data_00582410[24];
static InlineAsmRegisterEntry inlineAsmRegisterEntry;
static InlineAsmRegisterEntry data_00582434;

void CTemplateNew_InitRegistrationHashTables(void)
{
    long index;
    long secondaryIndex;
    Object *record;
    Object *secondaryRecord;
    const char *format;
    ObjectList *list;
    ObjectList *secondaryList;
    const char *secondaryFormat;
    HashNameNode *name;
    struct RegistrationHashEntry *entry;
    int id;
    struct RegistrationHashEntry *secondaryEntry;
    struct RegistrationHashEntry **secondaryBucket;
    struct RegistrationTableEntry *table;
    struct SecondaryRegistrationEntry *secondaryTable;
    struct RegistrationHashEntry **bucket;
    struct RegistrationEntry *registration;
    int secondaryId;
    const char *secondaryName;
    const char *entryName;
    short value;
    short secondaryValue;
    char message[20];

    for (index = 0; index < 64; index++)
        inlineAsmRegisterHashTable[index] = NULL;
    for (secondaryIndex = 0; secondaryIndex < 64; secondaryIndex++)
        secondary_registration_hash[secondaryIndex] = NULL;
    for (registration = registration_entries; registration->name != NULL; registration++) {
        name = GetHashNameNode(registration->name);
        for (list = (ObjectList *)arguments; list != NULL; list = list->next) {
            record = list->object;
            if (record != NULL && record->name == name) {
                switch (registration->kind) {
                    case 0:
                        format = "r";
                        break;
                    case 1:
                        format = "f";
                        break;
                    case 9:
                        format = "v";
                        break;
                    case 2:
                        format = "S";
                        break;
                    case 3:
                        format = "cr";
                        break;
                    default:
                        format = "";
                }
                sprintf(message, "%s%d", format, registration->value);
                PPCError_ReportDiagnostic(101, record->name->name, message);
            }
        }
        for (secondaryList = locals; secondaryList != NULL; secondaryList = secondaryList->next) {
            secondaryRecord = secondaryList->object;
            if (secondaryRecord != NULL && secondaryRecord->name == name) {
                switch (registration->kind) {
                    case 0:
                        secondaryFormat = "r";
                        break;
                    case 1:
                        secondaryFormat = "f";
                        break;
                    case 9:
                        secondaryFormat = "v";
                        break;
                    case 2:
                        secondaryFormat = "S";
                        break;
                    case 3:
                        secondaryFormat = "cr";
                        break;
                    default:
                        secondaryFormat = "";
                }
                sprintf(message, "%s%d", secondaryFormat, registration->value);
                PPCError_ReportDiagnostic(100, secondaryRecord->name->name, message);
            }
        }
        CTemplateNew_InsertRegisterBinding(registration->name, registration->kind, registration->value, NULL);
    }
    table = registration_table;
    while ((entryName = table->name) != NULL) {
        id = table->id;
        value = table->value;
        bucket = &inlineAsmRegisterHashTable[CHash(entryName) & 63];
        entry = (struct RegistrationHashEntry *)CompilerTools_AllocatePool(sizeof(*entry));
        entry->id = id;
        entry->name = entryName;
        entry->kind = 2;
        table++;
        entry->value = value;
        entry->extra = 0;
        entry->next = *bucket;
        *bucket = entry;
    }
    secondaryTable = secondary_registration_table;
    while ((secondaryName = secondaryTable->name) != NULL) {
        secondaryValue = secondaryTable->value;
        secondaryId = secondaryTable->id;
        secondaryBucket = &secondary_registration_hash[CHash(secondaryName) & 63];
        secondaryEntry = (struct RegistrationHashEntry *)CompilerTools_AllocatePool(sizeof(*secondaryEntry));
        secondaryEntry->id = secondaryId;
        secondaryEntry->name = secondaryName;
        secondaryEntry->kind = 2;
        secondaryTable++;
        secondaryEntry->value = secondaryValue;
        secondaryEntry->extra = 0;
        secondaryEntry->next = *secondaryBucket;
        *secondaryBucket = secondaryEntry;
    }
}

InlineAsmRegisterEntry *fn_004f06d0(char *name)
{
    char buf[0x2b];
    Boolean flag;
    SInt32 value;
    struct RegistrationHashEntry *entry;
    InlineAsmRegisterEntry *rec;
    InlineAsmRegisterEntry *found;

    if (strlen(name) < 0x28)
        CToLowercase(name, buf);
    else
        return NULL;
    found = NULL;
    for (entry = secondary_registration_hash[CHash(buf) & 0x3f]; entry != NULL; entry = entry->next) {
        rec = (InlineAsmRegisterEntry *)&entry->name;
        if (strcmp(entry->name, buf) == 0) {
            if (data_00587128 == 0xfffff) {
                UInt32 mask = data_00587128 & 0xfffff;
                if ((entry->id & mask) == mask)
                    return rec;
            } else if (entry->id & data_00587128) {
                return rec;
            }
            found = rec;
        }
    }
    if (found != NULL) {
        if (copts.warn_possunwant)
            PPCError_ReportDiagnostic(0x75, name);
        return found;
    }
    if (strncmp("dcr", buf, 3) == 0) {
        ScanDec(buf + 3, &value, &flag);
        if (flag || (UInt32)value > 0x400) {
            PPCError_ReportError(0x75, name);
            return NULL;
        }
        data_00582434.name = NULL;
        data_00582434.kind = 4;
        data_00582434.number = (SInt16)value;
        data_00582434.object = 0;
        return &data_00582434;
    }
    return NULL;
}

static inline Boolean CTemplateNew_ShouldWarnUnsupportedInlineAsmRegister(void)
{
    return copts.warn_possunwant;
}

struct InlineAsmRegisterEntry *CTemplateNew_LookupInlineAsmRegister(char *registerName)
{
    char lowercaseName[43];
    Boolean badRegisterNumber;
    SInt32 sprNumber;
    struct InlineAsmRegisterEntry *unsupportedRegister;
    struct RegistrationHashEntry *hashEntry;
    struct InlineAsmRegisterEntry *matchingRegister;

    void *(*lookupBinding)(unsigned int *) = find_register_binding_key;

    matchingRegister = (struct InlineAsmRegisterEntry *)lookupBinding((unsigned int *)registerName);
    if (matchingRegister)
        return matchingRegister;

    unsupportedRegister = NULL;
    if (strlen(registerName) < 40) {
        CToLowercase(registerName, lowercaseName);
    } else {
        return NULL;
    }

    for (hashEntry = inlineAsmRegisterHashTable[CHash(lowercaseName) & 0x3f]; hashEntry; hashEntry = hashEntry->next) {
        matchingRegister = (struct InlineAsmRegisterEntry *)&hashEntry->name;
        if (strcmp(hashEntry->name, lowercaseName) != 0)
            continue;
        if (data_00587128 == 0xfffff) {
            unsigned int requiredProcessors = data_00587128 & 0xfffff;
            if (requiredProcessors == (requiredProcessors & hashEntry->id))
                return matchingRegister;
        } else if (hashEntry->id & data_00587128) {
            return matchingRegister;
        }
        unsupportedRegister = matchingRegister;
    }
    if (unsupportedRegister) {
        if (CTemplateNew_ShouldWarnUnsupportedInlineAsmRegister())
            PPCError_ReportDiagnostic(117, registerName);
        return unsupportedRegister;
    }
    if (strncmp("spr", lowercaseName, 3) == 0) {
        ScanDec(lowercaseName + 3, &sprNumber, &badRegisterNumber);
        if (badRegisterNumber != 0 || sprNumber > 0x400U) {
            PPCError_ReportError(117, registerName);
            return NULL;
        }
        inlineAsmRegisterEntry.name = NULL;
        inlineAsmRegisterEntry.kind = 2;
        inlineAsmRegisterEntry.number = sprNumber;
        inlineAsmRegisterEntry.object = NULL;
        if (CTemplateNew_ShouldWarnUnsupportedInlineAsmRegister()) {
            if (data_00587128 == 0xfffff) {
                if (((data_00587128 & 0xfffff) & inline_asm_register_masks[sprNumber]) != (data_00587128 & 0xfffff))
                    PPCError_ReportDiagnostic(117, registerName);
            } else {
                if ((data_00587128 & inline_asm_register_masks[sprNumber]) == 0)
                    PPCError_ReportDiagnostic(117, registerName);
            }
        }
        return &inlineAsmRegisterEntry;
    }
    return NULL;
}

InlineAsmRegisterEntry *CTemplateNew_GetInlineAsmRegisterEntry(HashNameNode *name)
{
    Object *object;
    TypeStruct *type;
    int stype;
    InlineAsmRegisterEntry *entry;
    struct AsmOperand lookup;
    char *registerName;
    if (InlineAsm_ResolveOperandNameDefault(name, &lookup) != 0) {
        if ((object = lookup.object) != NULL && object->sclass == TK_REGISTER) {
            type = (TypeStruct *)object->type;
            registerName = name->name;
            entry = CTemplateNew_LookupInlineAsmRegister(registerName);
            if (entry != NULL && entry->object == lookup.object)
                return entry;
            if (type->type == TYPEFLOAT)
                CTemplateNew_InsertRegisterBinding(name->name, 1, 0, lookup.object);
            else if (type->type == TYPESTRUCT && (stype = type->stype) >= 4 && stype <= 14)
                CTemplateNew_InsertRegisterBinding(name->name, 9, 0, lookup.object);
            else
                CTemplateNew_InsertRegisterBinding(name->name, 0, 0, lookup.object);
        }
    }
    registerName = name->name;
    return CTemplateNew_LookupInlineAsmRegister(registerName);
}
