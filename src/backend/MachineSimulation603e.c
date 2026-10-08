#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation603e.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/PCode.h"
#include "compiler/Scheduler.h"

struct MachineInfo machine603e = {
    2,
    1,
    (SInt32 (*)(void *))fn_0052e640,
    fn_0052e590,
    (SInt32 (*)(void *))is_instruction_issuable,
    (void (*)(void *))fn_0052e450,
    fn_0052e110,
    (SInt32 (*)(void *))get_instruction_opcode_table_value,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline. */
static MachineOpcodeInfo machineOpcodeInfo[466] = {
    {0, 0, {0, 0, 0, 1}},   /* PC_B */
    {0, 0, {0, 0, 0, 1}},   /* PC_BL */
    {0, 0, {0, 0, 0, 1}},   /* PC_BC */
    {0, 0, {0, 0, 0, 1}},   /* PC_BCLR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BCCTR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BT */
    {0, 0, {0, 0, 0, 1}},   /* PC_BTLR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BTCTR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BF */
    {0, 0, {0, 0, 0, 1}},   /* PC_BFLR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BFCTR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDNZ */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDNZT */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDNZF */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDZ */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDZT */
    {0, 0, {0, 0, 0, 1}},   /* PC_BDZF */
    {0, 0, {0, 0, 0, 1}},   /* PC_BLR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BCTR */
    {0, 0, {0, 0, 0, 1}},   /* PC_BCTRL */
    {0, 0, {0, 0, 0, 1}},   /* PC_BLRL */
    {2, 2, {1, 1, 0, 0}},   /* PC_LBZ */
    {2, 2, {1, 1, 0, 0}},   /* PC_LBZU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LBZX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LBZUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHZ */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHZU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHZX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHZUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHA */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHAU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHAX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHAUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LHBRX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LWZ */
    {2, 2, {1, 1, 0, 0}},   /* PC_LWZU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LWZX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LWZUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LWBRX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LMW */
    {2, 2, {1, 1, 0, 0}},   /* PC_STB */
    {2, 2, {1, 1, 0, 0}},   /* PC_STBU */
    {2, 2, {1, 1, 0, 0}},   /* PC_STBX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STBUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STH */
    {2, 2, {1, 1, 0, 0}},   /* PC_STHU */
    {2, 2, {1, 1, 0, 0}},   /* PC_STHX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STHUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STHBRX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STW */
    {2, 2, {1, 1, 0, 0}},   /* PC_STWU */
    {2, 2, {1, 1, 0, 0}},   /* PC_STWX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STWUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STWBRX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STMW */
    {2, 2, {1, 1, 0, 0}},   /* PC_DCBF */
    {2, 2, {1, 1, 0, 0}},   /* PC_DCBST */
    {2, 2, {1, 1, 0, 0}},   /* PC_DCBT */
    {2, 2, {1, 1, 0, 0}},   /* PC_DCBTST */
    {2, 2, {1, 1, 0, 0}},   /* PC_DCBZ */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADD */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDC */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDE */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDI */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDIC */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDICR */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDME */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDZE */
    {1, 37, {37, 0, 0, 0}}, /* PC_DIVW */
    {1, 37, {37, 0, 0, 0}}, /* PC_DIVWU */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULHW */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULHWU */
    {1, 3, {3, 0, 0, 0}},   /* PC_MULLI */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULLW */
    {1, 1, {1, 0, 0, 0}},   /* PC_NEG */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBF */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFC */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFE */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFIC */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFME */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFZE */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMPI */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMP */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMPLI */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMPL */
    {1, 1, {1, 0, 0, 0}},   /* PC_ANDI */
    {1, 1, {1, 0, 0, 0}},   /* PC_ANDIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_ORI */
    {1, 1, {1, 0, 0, 0}},   /* PC_ORIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_XORI */
    {1, 1, {1, 0, 0, 0}},   /* PC_XORIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_AND */
    {1, 1, {1, 0, 0, 0}},   /* PC_OR */
    {1, 1, {1, 0, 0, 0}},   /* PC_XOR */
    {1, 1, {1, 0, 0, 0}},   /* PC_NAND */
    {1, 1, {1, 0, 0, 0}},   /* PC_NOR */
    {1, 1, {1, 0, 0, 0}},   /* PC_EQV */
    {1, 1, {1, 0, 0, 0}},   /* PC_ANDC */
    {1, 1, {1, 0, 0, 0}},   /* PC_ORC */
    {1, 1, {1, 0, 0, 0}},   /* PC_EXTSB */
    {1, 1, {1, 0, 0, 0}},   /* PC_EXTSH */
    {1, 1, {1, 0, 0, 0}},   /* PC_CNTLZW */
    {1, 1, {1, 0, 0, 0}},   /* PC_RLWINM */
    {1, 1, {1, 0, 0, 0}},   /* PC_RLWNM */
    {1, 1, {1, 0, 0, 0}},   /* PC_RLWIMI */
    {1, 1, {1, 0, 0, 0}},   /* PC_SLW */
    {1, 1, {1, 0, 0, 0}},   /* PC_SRW */
    {1, 1, {1, 0, 0, 0}},   /* PC_SRAWI */
    {1, 1, {1, 0, 0, 0}},   /* PC_SRAW */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRAND */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRANDC */
    {7, 1, {1, 0, 0, 0}},   /* PC_CREQV */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRNAND */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRNOR */
    {7, 1, {1, 0, 0, 0}},   /* PC_CROR */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRORC */
    {7, 1, {1, 0, 0, 0}},   /* PC_CRXOR */
    {7, 1, {1, 0, 0, 0}},   /* PC_MCRF */
    {7, 2, {2, 0, 0, 0}},   /* PC_MTXER */
    {7, 2, {2, 0, 0, 0}},   /* PC_MTCTR */
    {7, 2, {2, 0, 0, 0}},   /* PC_MTLR */
    {7, 1, {1, 0, 0, 0}},   /* PC_MTCRF */
    {7, 1, {1, 0, 0, 1}},   /* PC_MTMSR */
    {7, 1, {1, 0, 0, 1}},   /* PC_MTSPR */
    {7, 1, {1, 0, 0, 1}},   /* PC_MFMSR */
    {7, 1, {1, 0, 0, 1}},   /* PC_MFSPR */
    {7, 1, {1, 0, 0, 0}},   /* PC_MFXER */
    {7, 1, {1, 0, 0, 0}},   /* PC_MFCTR */
    {7, 1, {1, 0, 0, 0}},   /* PC_MFLR */
    {7, 1, {1, 0, 0, 0}},   /* PC_MFCR */
    {4, 3, {1, 1, 1, 0}},   /* PC_MFFS */
    {4, 3, {1, 1, 1, 0}},   /* PC_MTFSF */
    {7, 1, {1, 0, 0, 1}},   /* PC_EIEIO */
    {7, 1, {1, 0, 0, 1}},   /* PC_ISYNC */
    {7, 1, {1, 0, 0, 1}},   /* PC_SYNC */
    {7, 1, {1, 0, 0, 1}},   /* PC_RFI */
    {1, 1, {1, 0, 0, 0}},   /* PC_LI */
    {1, 1, {1, 0, 0, 0}},   /* PC_LIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_MR */
    {1, 1, {1, 0, 0, 0}},   /* PC_NOP */
    {1, 1, {1, 0, 0, 0}},   /* PC_NOT */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFS */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFSU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFSX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFSUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFD */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFDU */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFDX */
    {2, 2, {1, 1, 0, 0}},   /* PC_LFDUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFS */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFSU */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFSX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFSUX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFD */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFDU */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFDX */
    {2, 2, {1, 1, 0, 0}},   /* PC_STFDUX */
    {4, 3, {1, 1, 1, 0}},   /* PC_FMR */
    {4, 3, {1, 1, 1, 0}},   /* PC_FABS */
    {4, 3, {1, 1, 1, 0}},   /* PC_FNEG */
    {4, 3, {1, 1, 1, 0}},   /* PC_FNABS */
    {4, 3, {1, 1, 1, 0}},   /* PC_FADD */
    {4, 3, {1, 1, 1, 0}},   /* PC_FADDS */
    {4, 3, {1, 1, 1, 0}},   /* PC_FSUB */
    {4, 3, {1, 1, 1, 0}},   /* PC_FSUBS */
    {4, 4, {2, 1, 1, 0}},   /* PC_FMUL */
    {4, 3, {1, 1, 1, 0}},   /* PC_FMULS */
    {4, 33, {33, 0, 0, 0}}, /* PC_FDIV */
    {4, 18, {18, 0, 0, 0}}, /* PC_FDIVS */
    {4, 4, {2, 1, 1, 0}},   /* PC_FMADD */
    {4, 3, {1, 1, 1, 0}},   /* PC_FMADDS */
    {4, 4, {2, 1, 1, 0}},   /* PC_FMSUB */
    {4, 3, {1, 1, 1, 0}},   /* PC_FMSUBS */
    {4, 4, {2, 1, 1, 0}},   /* PC_FNMADD */
    {4, 3, {1, 1, 1, 0}},   /* PC_FNMADDS */
    {4, 4, {2, 1, 1, 0}},   /* PC_FNMSUB */
    {4, 3, {1, 1, 1, 0}},   /* PC_FNMSUBS */
    {4, 18, {18, 0, 0, 0}}, /* PC_FRES */
    {4, 3, {1, 1, 1, 0}},   /* PC_FRSQRTE */
    {4, 3, {1, 1, 1, 0}},   /* PC_FSEL */
    {4, 3, {1, 1, 1, 0}},   /* PC_FRSP */
    {4, 3, {1, 1, 1, 0}},   /* PC_FCTIW */
    {4, 3, {1, 1, 1, 0}},   /* PC_FCTIWZ */
    {4, 5, {1, 1, 1, 0}},   /* PC_FCMPU */
    {4, 5, {1, 1, 1, 0}},   /* PC_FCMPO */
    {2, 1, {1, 0, 0, 0}},   /* PC_LWARX */
    {2, 1, {1, 0, 0, 0}},   /* PC_LSWI */
    {2, 1, {1, 0, 0, 0}},   /* PC_LSWX */
    {2, 1, {1, 0, 0, 0}},   /* PC_STFIWX */
    {2, 1, {1, 0, 0, 0}},   /* PC_STSWI */
    {2, 1, {1, 0, 0, 0}},   /* PC_STSWX */
    {2, 1, {1, 0, 0, 0}},   /* PC_STWCX */
    {1, 1, {1, 0, 0, 1}},   /* PC_ECIWX */
    {1, 1, {1, 0, 0, 1}},   /* PC_ECOWX */
    {1, 1, {1, 0, 0, 0}},   /* PC_DCBI */
    {1, 1, {1, 0, 0, 0}},   /* PC_ICBI */
    {1, 1, {1, 0, 0, 0}},   /* PC_MCRFS */
    {1, 1, {1, 0, 0, 0}},   /* PC_MCRXR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFTB */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFSR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTSR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFSRIN */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTSRIN */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTFSB0 */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTFSB1 */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTFSFI */
    {1, 1, {1, 0, 0, 1}},   /* PC_SC */
    {4, 1, {1, 0, 0, 0}},   /* PC_FSQRT */
    {4, 1, {1, 0, 0, 0}},   /* PC_FSQRTS */
    {1, 1, {1, 0, 0, 0}},   /* PC_TLBIA */
    {1, 1, {1, 0, 0, 0}},   /* PC_TLBIE */
    {1, 1, {1, 0, 0, 0}},   /* PC_TLBLD */
    {1, 1, {1, 0, 0, 0}},   /* PC_TLBLI */
    {1, 1, {1, 0, 0, 0}},   /* PC_TLBSYNC */
    {1, 1, {1, 0, 0, 1}},   /* PC_TW */
    {1, 1, {1, 0, 0, 1}},   /* PC_TRAP */
    {1, 1, {1, 0, 0, 1}},   /* PC_TWI */
    {1, 1, {1, 0, 0, 1}},   /* PC_OPWORD */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFROM */
    {1, 1, {1, 0, 0, 1}},   /* PC_DSA */
    {1, 1, {1, 0, 0, 1}},   /* PC_ESA */
    {1, 0, {0, 0, 0, 0}},   /* PC_DCCCI */
    {1, 0, {0, 0, 0, 0}},   /* PC_DCREAD */
    {1, 0, {0, 0, 0, 0}},   /* PC_ICBT */
    {1, 0, {0, 0, 0, 0}},   /* PC_ICCCI */
    {1, 0, {0, 0, 0, 0}},   /* PC_ICREAD */
    {1, 0, {0, 0, 0, 0}},   /* PC_RFCI */
    {1, 0, {0, 0, 0, 0}},   /* PC_TLBRE */
    {1, 0, {0, 0, 0, 0}},   /* PC_TLBSX */
    {1, 0, {0, 0, 0, 0}},   /* PC_TLBWE */
    {1, 0, {0, 0, 0, 0}},   /* PC_WRTEE */
    {1, 0, {0, 0, 0, 0}},   /* PC_WRTEEI */
    {1, 0, {0, 0, 0, 0}},   /* PC_MFDCR */
    {1, 0, {0, 0, 0, 0}},   /* PC_MTDCR */
    {1, 0, {0, 0, 0, 0}},   /* PC_DCBA */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSS */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSSALL */
    {0, 0, {0, 0, 0, 0}},   /* PC_DST */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSTT */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSTST */
    {0, 0, {0, 0, 0, 0}},   /* PC_DSTSTT */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVEBX */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVEHX */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVEWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVSL */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVSR */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVX */
    {0, 0, {0, 0, 0, 0}},   /* PC_LVXL */
    {0, 0, {0, 0, 0, 0}},   /* PC_STVEBX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STVEHX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STVEWX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STVX */
    {0, 0, {0, 0, 0, 0}},   /* PC_STVXL */
    {0, 0, {0, 0, 0, 0}},   /* PC_MFVSCR */
    {0, 0, {0, 0, 0, 0}},   /* PC_MTVSCR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDCUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDSBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDSHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDSWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUBM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUHM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUWM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VADDUWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAND */
    {0, 0, {0, 0, 0, 0}},   /* PC_VANDC */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGSW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VAVGUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCFSX */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCFUX */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPBFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPEQFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPEQUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGEFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTSW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCMPGTUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCTSXS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VCTUXS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VEXPTEFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VLOGEFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXSW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMAXUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINSW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMINUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGHB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGHH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGHW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGLB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGLH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRGLW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULESB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULESH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULEUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULEUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULOSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULOSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULOUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMULOUH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VNOR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VOR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKPX */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKSHSS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKSHUS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKSWSS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKSWUS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKUHUM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKUHUS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKUWUM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPKUWUS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VREFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRFIM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRFIN */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRFIP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRFIZ */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRLB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRLH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRLW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VRSQRTEFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSL */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSLB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSLH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSLO */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSLW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTISB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTISH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSPLTISW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRAB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRAH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRAW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRO */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSRW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBCUW */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBSBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBSHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBSWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUBM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUHM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUWM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUBUWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUMSWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUM2SWS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUM4SBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUM4SHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSUM4UBS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKHPX */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKHSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKHSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKLPX */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKLSB */
    {0, 0, {0, 0, 0, 0}},   /* PC_VUPKLSH */
    {0, 0, {0, 0, 0, 0}},   /* PC_VXOR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMADDFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMHADDSHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMHRADDSHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMLADDUHM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMMBM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMSHM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMSHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMUBM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMUHM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMSUMUHS */
    {0, 0, {0, 0, 0, 0}},   /* PC_VNMSUBFP */
    {0, 0, {0, 0, 0, 0}},   /* PC_VPERM */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSEL */
    {0, 0, {0, 0, 0, 0}},   /* PC_VSLDOI */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMR */
    {0, 0, {0, 0, 0, 0}},   /* PC_VMRP */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_L */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_LU */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_LX */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_LUX */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_ST */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_STU */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_STX */
    {0, 0, {0, 0, 0, 0}},   /* PC_PSQ_STUX */
    {0, 0, {0, 0, 0, 0}},   /* PC_DCBZ_L */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_ADD */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_SUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MUL */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_DIV */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MADD */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MSUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_NMADD */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_NMSUB */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_RES */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_SEL */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_ABS */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_NABS */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_NEG */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MR */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_RSQRTE */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_CMPU0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_CMPO0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_CMPU1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_CMPO1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE00 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE01 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE10 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MERGE11 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_SUM0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_SUM1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MULS0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MULS1 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MADDS0 */
    {0, 0, {0, 0, 0, 0}},   /* PC_PS_MADDS1 */
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

/* The instruction in each of the eight execution stages and the cycles it has left there: eight statics in a row,
   which the code also indexes from the first. */
static PipelineStage instruction_timing_slots;
static PipelineStage data_00582d20;
static PipelineStage data_00582d28;
static PipelineStage data_00582d30;
static PipelineStage data_00582d38;
static PipelineStage queuedInstruction;
static PipelineStage data_00582d48;
static PipelineStage data_00582d50;
static int data_00582d58;
static int data_00582d5c;
static unsigned int data_00582d60;
static unsigned int data_00582d64;
static CompletionEntry instruction_completion_entries[5];

SInt32 fn_0052e640(PCodeInstruction *p)
{
    SInt32 n = machineOpcodeInfo[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}

void fn_0052e590(void)
{
    instruction_timing_slots.instr = NULL;
    data_00582d20.instr = NULL;
    data_00582d28.instr = NULL;
    data_00582d30.instr = NULL;
    data_00582d38.instr = NULL;
    queuedInstruction.instr = NULL;
    data_00582d48.instr = NULL;
    data_00582d50.instr = NULL;
    data_00582d58 = 5;
    data_00582d5c = 0;
    data_00582d60 = 0;
    data_00582d64 = 0;
    instruction_completion_entries[0].instr = NULL;
    instruction_completion_entries[1].instr = NULL;
    instruction_completion_entries[2].instr = NULL;
    instruction_completion_entries[3].instr = NULL;
    instruction_completion_entries[4].instr = NULL;
}

int is_instruction_issuable(PCodeInstruction *instr)
{
    UInt32 unit;
    PCodeInstruction *list;
    PCodeInstruction *ref;

    if (!data_00582d58)
        return 0;
    unit = machineOpcodeInfo[instr->opcode].executionUnit;
    if ((&instruction_timing_slots)[unit].instr) {
        if (unit == 1) {
            switch (instr->opcode) {
                case PC_ADD:
                case PC_ADDC:
                case PC_ADDI:
                case PC_ADDIS:
                case PC_CMPI:
                case PC_CMP:
                case PC_CMPLI:
                case PC_CMPL:
                    list = instr;
                    ref = data_00582d20.instr;
                    if (Scheduler_ReturnZero(list, ref, 0))
                        return 0;
                    if (!data_00582d50.instr)
                        return 1;
                    break;
            }
        }
        return 0;
    }
    if ((instr->flags & fIsWrite) && (&instruction_timing_slots)[3].instr &&
        ((&instruction_timing_slots)[3].instr->flags & fIsWrite))
        return 0;
    return 1;
}

void fn_0052e450(struct PCodeInstruction *instruction)
{
    unsigned int kind;
    int opcodeIndex;
    int opcodeValue;

    opcodeIndex = instruction->opcode;
    kind = machineOpcodeInfo[opcodeIndex].executionUnit;
    opcodeValue = machineOpcodeInfo[opcodeIndex].stageCycles[0];
    if ((kind == 1) && (data_00582d20.instr != NULL)) {
        kind = 7;
    }
    data_00582d5c++;
    data_00582d58--;
    instruction_completion_entries[data_00582d64].instr = instruction,
    instruction_completion_entries[data_00582d64].completed = 0;
    data_00582d64 = (data_00582d64 + 1) % 5;
    (&instruction_timing_slots)[kind].instr = instruction;
    (&instruction_timing_slots)[kind].remaining = opcodeValue;
}

void fn_0052e110(void)
{
    int slot;
    int index20;
    int index30;
    int index48;
    int index50;
    int index18;
    struct PCodeInstruction *instruction;
    int index38;
    short opcode;
    struct PCodeInstruction *pending38;
    int cycles28;
    struct PCodeInstruction *pending48;
    struct PCodeInstruction *pending20;
    struct PCodeInstruction *pending50;
    struct PCodeInstruction *pending30;
    int cycles40;
    int cycles38;
    struct PCodeInstruction *queued;
    struct PCodeInstruction *queued28;
    slot = 0;
    do {
        if ((&instruction_timing_slots)[slot].instr != NULL && (&instruction_timing_slots)[slot].remaining != 0)
            (&instruction_timing_slots)[slot].remaining -= 1;
        slot++;
    } while (slot < 8);
    if (data_00582d5c != 0 && instruction_completion_entries[data_00582d60].completed != 0) {
        instruction_completion_entries[data_00582d60].instr = NULL;
        data_00582d5c -= 1;
        data_00582d58 += 1;
        data_00582d60 = (data_00582d60 + 1) % 5;
        if (data_00582d5c != 0 && instruction_completion_entries[data_00582d60].completed != 0) {
            instruction_completion_entries[data_00582d60].instr = NULL;
            data_00582d5c -= 1;
            data_00582d58 += 1;
            data_00582d60 = (data_00582d60 + 1) % 5;
        }
    }
    if (data_00582d20.instr != NULL && data_00582d20.remaining == 0) {
        pending20 = data_00582d20.instr;
        index20 = 0;
        while (index20 < 5 && instruction_completion_entries[index20].instr != pending20) {
            index20 = index20 + 1;
        }
        instruction_completion_entries[index20].completed = 1;
        data_00582d20.instr = NULL;
    }
    if (data_00582d30.instr != NULL && data_00582d30.remaining == 0) {
        pending30 = data_00582d30.instr;
        index30 = 0;
        while (index30 < 5 && instruction_completion_entries[index30].instr != pending30) {
            index30 = index30 + 1;
        }
        instruction_completion_entries[index30].completed = 1;
        data_00582d30.instr = NULL;
    }
    if (data_00582d48.instr != NULL && data_00582d48.remaining == 0) {
        pending48 = data_00582d48.instr;
        index48 = 0;
        while (index48 < 5 && instruction_completion_entries[index48].instr != pending48) {
            index48 = index48 + 1;
        }
        instruction_completion_entries[index48].completed = 1;
        data_00582d48.instr = NULL;
    }
    if (data_00582d50.instr != NULL && data_00582d50.remaining == 0) {
        pending50 = data_00582d50.instr;
        index50 = 0;
        while (index50 < 5 && instruction_completion_entries[index50].instr != pending50) {
            index50 = index50 + 1;
        }
        instruction_completion_entries[index50].completed = 1;
        data_00582d50.instr = NULL;
    }
    if (instruction_timing_slots.instr != NULL && instruction_timing_slots.remaining == 0) {
        struct PCodeInstruction *pending18 = instruction_timing_slots.instr;
        index18 = 0;
        while (index18 < 5 && instruction_completion_entries[index18].instr != pending18)
            index18++;
        instruction_completion_entries[index18].completed = 1;
        instruction_timing_slots.instr = NULL;
    }
    instruction = data_00582d38.instr;
    if (instruction != NULL && data_00582d38.remaining == 0 &&
        ((opcode = instruction->opcode) == 168 || opcode == 169)) {
        pending38 = data_00582d38.instr;
        index38 = 0;
        while (index38 < 5 && instruction_completion_entries[index38].instr != pending38) {
            index38 = index38 + 1;
        }
        instruction_completion_entries[index38].completed = 1;
        data_00582d38.instr = NULL;
    }
    if (queuedInstruction.instr != NULL && queuedInstruction.remaining == 0 && data_00582d48.instr == NULL) {
        cycles40 = machineOpcodeInfo[(queued = queuedInstruction.instr)->opcode].stageCycles[2];
        data_00582d48.instr = queued;
        data_00582d48.remaining = cycles40;
        queuedInstruction.instr = NULL;
    }
    if (data_00582d38.instr != NULL && data_00582d38.remaining == 0 && queuedInstruction.instr == NULL) {
        cycles38 = machineOpcodeInfo[(queued = data_00582d38.instr)->opcode].stageCycles[1];
        queuedInstruction.instr = queued;
        queuedInstruction.remaining = cycles38;
        data_00582d38.instr = NULL;
    }
    if (data_00582d28.instr != NULL && data_00582d28.remaining == 0 && data_00582d30.instr == NULL) {
        cycles28 = machineOpcodeInfo[(queued28 = data_00582d28.instr)->opcode].stageCycles[1];
        data_00582d30.instr = queued28;
        data_00582d30.remaining = cycles28;
        data_00582d28.instr = NULL;
    }
}

int get_instruction_opcode_table_value(PCodeInstruction *instruction)
{
    return machineOpcodeInfo[instruction->opcode].stageCycles[3];
}
