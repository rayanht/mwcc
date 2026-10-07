#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation601.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/PCode.h"
#include "compiler/Scheduler.h"

struct MachineInfo machine601 = {
    2,
    0,
    (SInt32 (*)(void *))get_latency,
    clear_instruction_and_globals,
    (SInt32 (*)(void *))is_execution_unit_available,
    (void (*)(void *))set_execution_unit_instruction,
    advance_instruction_pipeline,
    (SInt32 (*)(void *))is_execution_unit_seven,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline. */
static MachineOpcodeInfo data_00578340[466] = {
    {5, 0, {0, 0, 0, 1}},   /* PC_B */
    {5, 0, {0, 0, 0, 1}},   /* PC_BL */
    {5, 0, {0, 0, 0, 1}},   /* PC_BC */
    {5, 0, {0, 0, 0, 1}},   /* PC_BCLR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BCCTR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BT */
    {5, 0, {0, 0, 0, 1}},   /* PC_BTLR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BTCTR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BF */
    {5, 0, {0, 0, 0, 1}},   /* PC_BFLR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BFCTR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDNZ */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDNZT */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDNZF */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDZ */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDZT */
    {5, 0, {0, 0, 0, 1}},   /* PC_BDZF */
    {5, 0, {0, 0, 0, 1}},   /* PC_BLR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BCTR */
    {5, 0, {0, 0, 0, 1}},   /* PC_BCTRL */
    {5, 0, {0, 0, 0, 1}},   /* PC_BLRL */
    {0, 2, {1, 0, 0, 0}},   /* PC_LBZ */
    {0, 2, {1, 0, 0, 0}},   /* PC_LBZU */
    {0, 2, {1, 0, 0, 0}},   /* PC_LBZX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LBZUX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHZ */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHZU */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHZX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHZUX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHA */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHAU */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHAX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHAUX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LHBRX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LWZ */
    {0, 2, {1, 0, 0, 0}},   /* PC_LWZU */
    {0, 2, {1, 0, 0, 0}},   /* PC_LWZX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LWZUX */
    {0, 2, {1, 0, 0, 0}},   /* PC_LWBRX */
    {0, 1, {1, 0, 0, 0}},   /* PC_LMW */
    {0, 1, {1, 0, 0, 0}},   /* PC_STB */
    {0, 1, {1, 0, 0, 0}},   /* PC_STBU */
    {0, 1, {1, 0, 0, 0}},   /* PC_STBX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STBUX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STH */
    {0, 1, {1, 0, 0, 0}},   /* PC_STHU */
    {0, 1, {1, 0, 0, 0}},   /* PC_STHX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STHUX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STHBRX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STW */
    {0, 1, {1, 0, 0, 0}},   /* PC_STWU */
    {0, 1, {1, 0, 0, 0}},   /* PC_STWX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STWUX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STWBRX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STMW */
    {0, 2, {1, 0, 0, 0}},   /* PC_DCBF */
    {0, 2, {1, 0, 0, 0}},   /* PC_DCBST */
    {0, 2, {1, 0, 0, 0}},   /* PC_DCBT */
    {0, 2, {1, 0, 0, 0}},   /* PC_DCBTST */
    {0, 2, {1, 0, 0, 0}},   /* PC_DCBZ */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADD */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDC */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDE */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDI */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDIC */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDICR */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDME */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDZE */
    {0, 36, {36, 0, 0, 0}}, /* PC_DIVW */
    {0, 36, {36, 0, 0, 0}}, /* PC_DIVWU */
    {0, 5, {5, 0, 0, 0}},   /* PC_MULHW */
    {0, 5, {5, 0, 0, 0}},   /* PC_MULHWU */
    {0, 5, {5, 0, 0, 0}},   /* PC_MULLI */
    {0, 5, {5, 0, 0, 0}},   /* PC_MULLW */
    {0, 1, {1, 0, 0, 0}},   /* PC_NEG */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBF */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBFC */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBFE */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBFIC */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBFME */
    {0, 1, {1, 0, 0, 0}},   /* PC_SUBFZE */
    {0, 3, {1, 0, 0, 0}},   /* PC_CMPI */
    {0, 3, {1, 0, 0, 0}},   /* PC_CMP */
    {0, 3, {1, 0, 0, 0}},   /* PC_CMPLI */
    {0, 3, {1, 0, 0, 0}},   /* PC_CMPL */
    {0, 1, {1, 0, 0, 0}},   /* PC_ANDI */
    {0, 1, {1, 0, 0, 0}},   /* PC_ANDIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_ORI */
    {0, 1, {1, 0, 0, 0}},   /* PC_ORIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_XORI */
    {0, 1, {1, 0, 0, 0}},   /* PC_XORIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_AND */
    {0, 1, {1, 0, 0, 0}},   /* PC_OR */
    {0, 1, {1, 0, 0, 0}},   /* PC_XOR */
    {0, 1, {1, 0, 0, 0}},   /* PC_NAND */
    {0, 1, {1, 0, 0, 0}},   /* PC_NOR */
    {0, 1, {1, 0, 0, 0}},   /* PC_EQV */
    {0, 1, {1, 0, 0, 0}},   /* PC_ANDC */
    {0, 1, {1, 0, 0, 0}},   /* PC_ORC */
    {0, 1, {1, 0, 0, 0}},   /* PC_EXTSB */
    {0, 1, {1, 0, 0, 0}},   /* PC_EXTSH */
    {0, 1, {1, 0, 0, 0}},   /* PC_CNTLZW */
    {0, 1, {1, 0, 0, 0}},   /* PC_RLWINM */
    {0, 1, {1, 0, 0, 0}},   /* PC_RLWNM */
    {0, 1, {1, 0, 0, 0}},   /* PC_RLWIMI */
    {0, 1, {1, 0, 0, 0}},   /* PC_SLW */
    {0, 1, {1, 0, 0, 0}},   /* PC_SRW */
    {0, 1, {1, 0, 0, 0}},   /* PC_SRAWI */
    {0, 1, {1, 0, 0, 0}},   /* PC_SRAW */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRAND */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRANDC */
    {0, 1, {1, 0, 0, 0}},   /* PC_CREQV */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRNAND */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRNOR */
    {0, 1, {1, 0, 0, 0}},   /* PC_CROR */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRORC */
    {0, 1, {1, 0, 0, 0}},   /* PC_CRXOR */
    {0, 1, {1, 0, 0, 0}},   /* PC_MCRF */
    {0, 4, {1, 0, 0, 0}},   /* PC_MTXER */
    {0, 4, {1, 0, 0, 0}},   /* PC_MTCTR */
    {0, 4, {1, 0, 0, 0}},   /* PC_MTLR */
    {0, 2, {1, 0, 0, 0}},   /* PC_MTCRF */
    {0, 1, {0, 0, 0, 0}},   /* PC_MTMSR */
    {0, 1, {0, 0, 0, 0}},   /* PC_MTSPR */
    {0, 1, {0, 0, 0, 0}},   /* PC_MFMSR */
    {0, 1, {0, 0, 0, 0}},   /* PC_MFSPR */
    {0, 1, {1, 0, 0, 0}},   /* PC_MFXER */
    {0, 1, {1, 0, 0, 0}},   /* PC_MFCTR */
    {0, 1, {1, 0, 0, 0}},   /* PC_MFLR */
    {0, 1, {1, 0, 0, 0}},   /* PC_MFCR */
    {1, 4, {1, 1, 1, 1}},   /* PC_MFFS */
    {1, 4, {1, 1, 1, 1}},   /* PC_MTFSF */
    {7, 1, {1, 0, 0, 1}},   /* PC_EIEIO */
    {7, 1, {1, 0, 0, 1}},   /* PC_ISYNC */
    {7, 1, {1, 0, 0, 1}},   /* PC_SYNC */
    {7, 0, {0, 0, 0, 1}},   /* PC_RFI */
    {0, 1, {1, 0, 0, 0}},   /* PC_LI */
    {0, 1, {1, 0, 0, 0}},   /* PC_LIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_MR */
    {0, 1, {1, 0, 0, 0}},   /* PC_NOP */
    {0, 1, {1, 0, 0, 0}},   /* PC_NOT */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFS */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFSU */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFSX */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFSUX */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFD */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFDU */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFDX */
    {0, 3, {1, 0, 0, 0}},   /* PC_LFDUX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFS */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFSU */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFSX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFSUX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFD */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFDU */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFDX */
    {0, 1, {1, 0, 0, 0}},   /* PC_STFDUX */
    {1, 4, {1, 1, 1, 1}},   /* PC_FMR */
    {1, 4, {1, 1, 1, 1}},   /* PC_FABS */
    {1, 4, {1, 1, 1, 1}},   /* PC_FNEG */
    {1, 4, {1, 1, 1, 1}},   /* PC_FNABS */
    {1, 4, {1, 1, 1, 1}},   /* PC_FADD */
    {1, 4, {1, 1, 1, 1}},   /* PC_FADDS */
    {1, 4, {1, 1, 1, 1}},   /* PC_FSUB */
    {1, 4, {1, 1, 1, 1}},   /* PC_FSUBS */
    {1, 5, {1, 1, 2, 1}},   /* PC_FMUL */
    {1, 4, {1, 1, 1, 1}},   /* PC_FMULS */
    {1, 31, {1, 1, 28, 1}}, /* PC_FDIV */
    {1, 17, {1, 1, 14, 1}}, /* PC_FDIVS */
    {1, 5, {1, 1, 2, 1}},   /* PC_FMADD */
    {1, 4, {1, 1, 1, 1}},   /* PC_FMADDS */
    {1, 5, {1, 1, 2, 1}},   /* PC_FMSUB */
    {1, 4, {1, 1, 1, 1}},   /* PC_FMSUBS */
    {1, 5, {1, 1, 2, 1}},   /* PC_FNMADD */
    {1, 4, {1, 1, 1, 1}},   /* PC_FNMADDS */
    {1, 5, {1, 1, 2, 1}},   /* PC_FNMSUB */
    {1, 4, {1, 1, 1, 1}},   /* PC_FNMSUBS */
    {1, 4, {1, 1, 1, 1}},   /* PC_FRES */
    {1, 4, {1, 1, 1, 1}},   /* PC_FRSQRTE */
    {1, 4, {1, 1, 1, 1}},   /* PC_FSEL */
    {1, 4, {1, 1, 1, 1}},   /* PC_FRSP */
    {1, 4, {1, 1, 1, 1}},   /* PC_FCTIW */
    {1, 4, {1, 1, 1, 1}},   /* PC_FCTIWZ */
    {1, 6, {1, 1, 1, 1}},   /* PC_FCMPU */
    {1, 6, {1, 1, 1, 1}},   /* PC_FCMPO */
    {0, 0, {0, 0, 0, 0}},   /* PC_LWARX */
    {0, 0, {0, 0, 0, 0}},   /* PC_LSWI */
    {0, 0, {0, 0, 0, 0}},   /* PC_LSWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STFIWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STSWI */
    {0, 0, {0, 0, 0, 0}},   /* PC_STSWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STWCX */
    {0, 0, {0, 0, 0, 0}},   /* PC_ECIWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_ECOWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_DCBI */
    {0, 0, {0, 0, 0, 0}},   /* PC_ICBI */
    {0, 0, {0, 0, 0, 0}},   /* PC_MCRFS */
    {0, 0, {0, 0, 0, 0}},   /* PC_MCRXR */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFTB */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFSR */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTSR */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFSRIN */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTSRIN */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTFSB0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTFSB1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTFSFI */
    {7, 0, {0, 0, 0, 0}},   /* PC_SC */
    {0, 0, {0, 0, 0, 0}},   /* PC_FSQRT */
    {0, 0, {0, 0, 0, 0}},   /* PC_FSQRTS */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBIA */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBIE */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBLD */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBLI */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBSYNC */
    {7, 0, {0, 0, 0, 0}},   /* PC_TW */
    {7, 0, {0, 0, 0, 0}},   /* PC_TRAP */
    {7, 0, {0, 0, 0, 0}},   /* PC_TWI */
    {7, 0, {0, 0, 0, 0}},   /* PC_OPWORD */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFROM */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSA */
    {0, 0, {0, 0, 0, 0}},   /* PC_ESA */
    {0, 0, {0, 0, 0, 0}},   /* PC_DCCCI */
    {0, 0, {0, 0, 0, 0}},   /* PC_DCREAD */
    {0, 0, {0, 0, 0, 0}},   /* PC_ICBT */
    {0, 0, {0, 0, 0, 0}},   /* PC_ICCCI */
    {0, 0, {0, 0, 0, 0}},   /* PC_ICREAD */
    {0, 0, {0, 0, 0, 0}},   /* PC_RFCI */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBRE */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBSX */
    {0, 0, {0, 0, 0, 0}},   /* PC_TLBWE */
    {0, 0, {0, 0, 0, 0}},   /* PC_WRTEE */
    {0, 0, {0, 0, 0, 0}},   /* PC_WRTEEI */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFDCR */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTDCR */
    {8, 0, {0, 0, 0, 0}},   /* PC_DCBA */
    {8, 0, {0, 0, 0, 0}},   /* PC_DSS */
    {8, 0, {0, 0, 0, 0}},   /* PC_DSSALL */
    {8, 0, {0, 0, 0, 0}},   /* PC_DST */
    {8, 0, {0, 0, 0, 0}},   /* PC_DSTT */
    {8, 0, {0, 0, 0, 0}},   /* PC_DSTST */
    {8, 0, {0, 0, 0, 0}},   /* PC_DSTSTT */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVEBX */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVEHX */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVEWX */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVSL */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVSR */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVX */
    {8, 0, {0, 0, 0, 0}},   /* PC_LVXL */
    {8, 0, {0, 0, 0, 0}},   /* PC_STVEBX */
    {8, 0, {0, 0, 0, 0}},   /* PC_STVEHX */
    {8, 0, {0, 0, 0, 0}},   /* PC_STVEWX */
    {8, 0, {0, 0, 0, 0}},   /* PC_STVX */
    {8, 0, {0, 0, 0, 0}},   /* PC_STVXL */
    {8, 0, {0, 0, 0, 0}},   /* PC_MFVSCR */
    {8, 0, {0, 0, 0, 0}},   /* PC_MTVSCR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDCUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDSBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDSHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDSWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUBM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUHM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUWM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VADDUWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAND */
    {8, 0, {0, 0, 0, 0}},   /* PC_VANDC */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGSW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VAVGUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCFSX */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCFUX */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPBFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPEQFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGEFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCTSXS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VCTUXS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VEXPTEFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VLOGEFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXSW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMAXUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINSW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMINUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGHB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGHH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGHW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGLB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGLH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRGLW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULESB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULESH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULEUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULEUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULOSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULOSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULOUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMULOUH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VNOR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VOR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKPX */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKSHSS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKSHUS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKSWSS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKSWUS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKUHUM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKUHUS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKUWUM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPKUWUS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VREFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRFIM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRFIN */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRFIP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRFIZ */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRLB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRLH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRLW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VRSQRTEFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSL */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSLB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSLH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSLO */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSLW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTISB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTISH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSPLTISW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRAB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRAH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRAW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRO */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSRW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBCUW */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBSBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBSHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBSWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUBM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUHM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUWM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUBUWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUMSWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUM2SWS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUM4SBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUM4SHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSUM4UBS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKHPX */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKHSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKHSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKLPX */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKLSB */
    {8, 0, {0, 0, 0, 0}},   /* PC_VUPKLSH */
    {8, 0, {0, 0, 0, 0}},   /* PC_VXOR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMADDFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMHADDSHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMHRADDSHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMLADDUHM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMMBM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMSHM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMSHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMUBM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMUHM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMSUMUHS */
    {8, 0, {0, 0, 0, 0}},   /* PC_VNMSUBFP */
    {8, 0, {0, 0, 0, 0}},   /* PC_VPERM */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSEL */
    {8, 0, {0, 0, 0, 0}},   /* PC_VSLDOI */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMR */
    {8, 0, {0, 0, 0, 0}},   /* PC_VMRP */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_L */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_LU */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_LX */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_LUX */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_ST */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_STU */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_STX */
    {8, 0, {0, 0, 0, 0}},   /* PC_PSQ_STUX */
    {8, 0, {0, 0, 0, 0}},   /* PC_DCBZ_L */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_ADD */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_SUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MUL */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_DIV */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MADD */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MSUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_NMADD */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_NMSUB */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_RES */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_SEL */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_ABS */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_NABS */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_NEG */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MR */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_RSQRTE */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_CMPU0 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_CMPO0 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_CMPU1 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_CMPO1 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE00 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE01 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE10 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE11 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_SUM0 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_SUM1 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MULS0 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MULS1 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MADDS0 */
    {8, 0, {0, 0, 0, 0}},   /* PC_PS_MADDS1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLE */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLEQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLIQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLLIQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLLQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SLQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRAIQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRAQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRE */
    {0, 0, {0, 0, 0, 0}},   /* PC_SREA */
    {0, 0, {0, 0, 0, 0}},   /* PC_SREQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRIQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRLIQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRLQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_SRQ */
    {0, 0, {0, 0, 0, 0}},   /* PC_MASKG */
    {0, 0, {0, 0, 0, 0}},   /* PC_MASKIR */
    {0, 0, {0, 0, 0, 0}},   /* PC_LSCBX */
    {0, 0, {0, 0, 0, 0}},   /* PC_DIV */
    {0, 0, {0, 0, 0, 0}},   /* PC_DIVS */
    {0, 0, {0, 0, 0, 0}},   /* PC_DOZ */
    {0, 0, {0, 0, 0, 0}},   /* PC_MUL */
    {0, 0, {0, 0, 0, 0}},   /* PC_NABS */
    {0, 0, {0, 0, 0, 0}},   /* PC_ABS */
    {0, 0, {0, 0, 0, 0}},   /* PC_CLCS */
    {0, 0, {0, 0, 0, 0}},   /* PC_DOZI */
    {0, 0, {0, 0, 0, 0}},   /* PC_RLMI */
    {0, 0, {0, 0, 0, 0}},   /* PC_RRIB */
};

/* The instruction in each of the pipeline's six stages and the cycles it has left there: six statics in a row,
   which the code also indexes from the first. */
static PipelineStage data_00582fe8;
static PipelineStage data_00582ff0;
static PipelineStage data_00582ff8;
static PipelineStage data_00583000;
static PipelineStage data_00583008;
static PipelineStage data_00583010;

SInt32 get_latency(PCodeInstruction *p)
{
    SInt32 n = data_00578340[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}

void clear_instruction_and_globals(void)
{
    data_00582fe8.instr = NULL;
    data_00582ff0.instr = NULL;
    data_00582ff8.instr = NULL;
    data_00583000.instr = NULL;
    data_00583008.instr = NULL;
    data_00583010.instr = NULL;
}

int is_execution_unit_available(PCodeInstruction *instruction)
{
    unsigned int kind;
    PipelineStage *counts;

    kind = data_00578340[instruction->opcode].executionUnit;
    if (kind == 7)
        kind = 0;
    counts = &data_00582fe8;
    if (counts[kind].instr != NULL)
        return 0;
    else
        return 1;
}

void set_execution_unit_instruction(PCodeInstruction *instruction)
{
    unsigned int entry;
    int offset;
    int value;

    offset = instruction->opcode;
    entry = data_00578340[offset].executionUnit;
    value = data_00578340[offset].stageCycles[0];
    if (entry == 7) {
        entry = 0;
    }
    (&data_00582fe8)[entry].instr = instruction;
    (&data_00582fe8)[entry].remaining = value;
}

void advance_instruction_pipeline(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if ((&data_00582fe8)[i].instr != NULL && (&data_00582fe8)[i].remaining != 0) {
            (&data_00582fe8)[i].remaining--;
        }
    }

    if (data_00582fe8.instr != NULL && data_00582fe8.remaining == 0) {
        data_00582fe8.instr = NULL;
    }
    if (data_00583008.instr != NULL && data_00583008.remaining == 0) {
        data_00583008.instr = NULL;
    }
    if (data_00583010.instr != NULL && data_00583010.remaining == 0) {
        data_00583010.instr = NULL;
    }
    if (data_00583000.instr != NULL && data_00583000.remaining == 0 && data_00583008.instr == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = data_00578340[(instruction = data_00583000.instr)->opcode].stageCycles[3];
        data_00583008.instr = instruction;
        data_00583008.remaining = v;
        data_00583000.instr = NULL;
    }
    if (data_00582ff8.instr != NULL && data_00582ff8.remaining == 0 && data_00583000.instr == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = data_00578340[(instruction = data_00582ff8.instr)->opcode].stageCycles[2];
        data_00583000.instr = instruction;
        data_00583000.remaining = v;
        data_00582ff8.instr = NULL;
    }
    if (data_00582ff0.instr != NULL && data_00582ff0.remaining == 0 && data_00582ff8.instr == NULL) {
        SInt32 v;
        PCodeInstruction *instruction;
        v = data_00578340[(instruction = data_00582ff0.instr)->opcode].stageCycles[1];
        data_00582ff8.instr = instruction;
        data_00582ff8.remaining = v;
        data_00582ff0.instr = NULL;
    }
}

Boolean is_execution_unit_seven(int instruction)
{
    return (int)data_00578340[((PCodeInstruction *)instruction)->opcode].executionUnit == 7;
}
