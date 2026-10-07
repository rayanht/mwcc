#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation750.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"

struct MachineInfo machine750 = {
    2,
    1,
    (SInt32 (*)(void *))fn_0052f330,
    reset_simulation_pipeline,
    (SInt32 (*)(void *))fn_0052f120,
    (void (*)(void *))record_instruction_kind,
    advance_simulation_pipeline,
    (SInt32 (*)(void *))get_opcode_table_first_entry,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline; the table ends at PC_VMINFP,
   short of the last AltiVec opcodes. */
static MachineOpcodeInfo data_00576f28[302] = {
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
    {3, 2, {1, 1, 0, 0}},   /* PC_LBZ */
    {3, 2, {1, 1, 0, 0}},   /* PC_LBZU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LBZX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LBZUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHZ */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHZU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHZX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHZUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHA */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHAU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHAX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHAUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LHBRX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LWZ */
    {3, 2, {1, 1, 0, 0}},   /* PC_LWZU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LWZX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LWZUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LWBRX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LMW */
    {3, 2, {1, 1, 0, 0}},   /* PC_STB */
    {3, 2, {1, 1, 0, 0}},   /* PC_STBU */
    {3, 2, {1, 1, 0, 0}},   /* PC_STBX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STBUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STH */
    {3, 2, {1, 1, 0, 0}},   /* PC_STHU */
    {3, 2, {1, 1, 0, 0}},   /* PC_STHX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STHUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STHBRX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STW */
    {3, 2, {1, 1, 0, 0}},   /* PC_STWU */
    {3, 2, {1, 1, 0, 0}},   /* PC_STWX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STWUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STWBRX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STMW */
    {3, 3, {1, 2, 0, 0}},   /* PC_DCBF */
    {3, 3, {1, 2, 0, 0}},   /* PC_DCBST */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBT */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBTST */
    {3, 3, {1, 2, 0, 0}},   /* PC_DCBZ */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADD */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDC */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDE */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDI */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDIC */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDICR */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDIS */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDME */
    {2, 1, {1, 0, 0, 0}},   /* PC_ADDZE */
    {1, 19, {19, 0, 0, 0}}, /* PC_DIVW */
    {1, 19, {19, 0, 0, 0}}, /* PC_DIVWU */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULHW */
    {1, 6, {5, 0, 0, 0}},   /* PC_MULHWU */
    {1, 3, {3, 0, 0, 0}},   /* PC_MULLI */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULLW */
    {2, 1, {1, 0, 0, 0}},   /* PC_NEG */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBF */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBFC */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBFE */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBFIC */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBFME */
    {2, 1, {1, 0, 0, 0}},   /* PC_SUBFZE */
    {2, 3, {1, 0, 0, 0}},   /* PC_CMPI */
    {2, 3, {1, 0, 0, 0}},   /* PC_CMP */
    {2, 3, {1, 0, 0, 0}},   /* PC_CMPLI */
    {2, 3, {1, 0, 0, 0}},   /* PC_CMPL */
    {2, 1, {1, 0, 0, 0}},   /* PC_ANDI */
    {2, 1, {1, 0, 0, 0}},   /* PC_ANDIS */
    {2, 1, {1, 0, 0, 0}},   /* PC_ORI */
    {2, 1, {1, 0, 0, 0}},   /* PC_ORIS */
    {2, 1, {1, 0, 0, 0}},   /* PC_XORI */
    {2, 1, {1, 0, 0, 0}},   /* PC_XORIS */
    {2, 1, {1, 0, 0, 0}},   /* PC_AND */
    {2, 1, {1, 0, 0, 0}},   /* PC_OR */
    {2, 1, {1, 0, 0, 0}},   /* PC_XOR */
    {2, 1, {1, 0, 0, 0}},   /* PC_NAND */
    {2, 1, {1, 0, 0, 0}},   /* PC_NOR */
    {2, 1, {1, 0, 0, 0}},   /* PC_EQV */
    {2, 1, {1, 0, 0, 0}},   /* PC_ANDC */
    {2, 1, {1, 0, 0, 0}},   /* PC_ORC */
    {2, 1, {1, 0, 0, 0}},   /* PC_EXTSB */
    {2, 1, {1, 0, 0, 0}},   /* PC_EXTSH */
    {2, 1, {1, 0, 0, 0}},   /* PC_CNTLZW */
    {2, 1, {1, 0, 0, 0}},   /* PC_RLWINM */
    {2, 1, {1, 0, 0, 0}},   /* PC_RLWNM */
    {2, 1, {1, 0, 0, 0}},   /* PC_RLWIMI */
    {2, 1, {1, 0, 0, 0}},   /* PC_SLW */
    {2, 1, {1, 0, 0, 0}},   /* PC_SRW */
    {2, 1, {1, 0, 0, 0}},   /* PC_SRAWI */
    {2, 1, {1, 0, 0, 0}},   /* PC_SRAW */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRAND */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRANDC */
    {8, 1, {1, 0, 0, 1}},   /* PC_CREQV */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRNAND */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRNOR */
    {8, 1, {1, 0, 0, 1}},   /* PC_CROR */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRORC */
    {8, 1, {1, 0, 0, 1}},   /* PC_CRXOR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MCRF */
    {8, 2, {2, 0, 0, 1}},   /* PC_MTXER */
    {8, 2, {2, 0, 0, 1}},   /* PC_MTCTR */
    {8, 2, {2, 0, 0, 1}},   /* PC_MTLR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MTCRF */
    {8, 1, {1, 0, 0, 0}},   /* PC_MTMSR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MTSPR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFMSR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFSPR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFXER */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFCTR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFLR */
    {8, 1, {1, 0, 0, 1}},   /* PC_MFCR */
    {5, 3, {1, 1, 1, 0}},   /* PC_MFFS */
    {5, 3, {1, 1, 1, 0}},   /* PC_MTFSF */
    {8, 1, {1, 0, 0, 1}},   /* PC_EIEIO */
    {8, 2, {2, 0, 0, 1}},   /* PC_ISYNC */
    {8, 3, {3, 0, 0, 1}},   /* PC_SYNC */
    {8, 1, {1, 0, 0, 1}},   /* PC_RFI */
    {2, 1, {1, 0, 0, 0}},   /* PC_LI */
    {2, 1, {1, 0, 0, 0}},   /* PC_LIS */
    {2, 1, {1, 0, 0, 0}},   /* PC_MR */
    {2, 1, {1, 0, 0, 0}},   /* PC_NOP */
    {2, 1, {1, 0, 0, 0}},   /* PC_NOT */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFS */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFSU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFSX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFSUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFD */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFDU */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFDX */
    {3, 2, {1, 1, 0, 0}},   /* PC_LFDUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFS */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFSU */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFSX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFSUX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFD */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFDU */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFDX */
    {3, 2, {1, 1, 0, 0}},   /* PC_STFDUX */
    {5, 3, {1, 1, 1, 0}},   /* PC_FMR */
    {5, 3, {1, 1, 1, 0}},   /* PC_FABS */
    {5, 3, {1, 1, 1, 0}},   /* PC_FNEG */
    {5, 3, {1, 1, 1, 0}},   /* PC_FNABS */
    {5, 3, {1, 1, 1, 0}},   /* PC_FADD */
    {5, 3, {1, 1, 1, 0}},   /* PC_FADDS */
    {5, 3, {1, 1, 1, 0}},   /* PC_FSUB */
    {5, 3, {1, 1, 1, 0}},   /* PC_FSUBS */
    {5, 4, {2, 1, 1, 0}},   /* PC_FMUL */
    {5, 3, {1, 1, 1, 0}},   /* PC_FMULS */
    {5, 31, {31, 0, 0, 0}}, /* PC_FDIV */
    {5, 17, {17, 0, 0, 0}}, /* PC_FDIVS */
    {5, 4, {2, 1, 1, 0}},   /* PC_FMADD */
    {5, 3, {1, 1, 1, 0}},   /* PC_FMADDS */
    {5, 4, {2, 1, 1, 0}},   /* PC_FMSUB */
    {5, 3, {1, 1, 1, 0}},   /* PC_FMSUBS */
    {5, 4, {2, 1, 1, 0}},   /* PC_FNMADD */
    {5, 3, {1, 1, 1, 0}},   /* PC_FNMADDS */
    {5, 4, {2, 1, 1, 0}},   /* PC_FNMSUB */
    {5, 3, {1, 1, 1, 0}},   /* PC_FNMSUBS */
    {5, 10, {10, 0, 0, 0}}, /* PC_FRES */
    {5, 3, {1, 1, 1, 0}},   /* PC_FRSQRTE */
    {5, 3, {1, 1, 1, 0}},   /* PC_FSEL */
    {5, 3, {1, 1, 1, 0}},   /* PC_FRSP */
    {5, 3, {1, 1, 1, 0}},   /* PC_FCTIW */
    {5, 3, {1, 1, 1, 0}},   /* PC_FCTIWZ */
    {5, 3, {1, 1, 1, 0}},   /* PC_FCMPU */
    {5, 3, {1, 1, 1, 0}},   /* PC_FCMPO */
    {3, 1, {1, 0, 0, 0}},   /* PC_LWARX */
    {3, 1, {1, 0, 0, 0}},   /* PC_LSWI */
    {3, 1, {1, 0, 0, 0}},   /* PC_LSWX */
    {3, 1, {1, 0, 0, 0}},   /* PC_STFIWX */
    {3, 1, {1, 0, 0, 0}},   /* PC_STSWI */
    {3, 1, {1, 0, 0, 0}},   /* PC_STSWX */
    {3, 1, {1, 0, 0, 0}},   /* PC_STWCX */
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
    {5, 1, {1, 0, 0, 0}},   /* PC_FSQRT */
    {5, 1, {1, 0, 0, 0}},   /* PC_FSQRTS */
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
};

/* The instruction in each of the nine execution stages and the cycles it has left there: nine statics in a row,
   which the code also indexes from the first. */
static PipelineStage data_00582e70;
static PipelineStage data_00582e78;
static PipelineStage data_00582e80;
static PipelineStage data_00582e88;
static PipelineStage data_00582e90;
static PipelineStage data_00582e98;
static PipelineStage data_00582ea0;
static PipelineStage data_00582ea8;
static PipelineStage data_00582eb0;
static struct PCodeInstruction *data_00582eb8;
static struct PCodeInstruction *data_00582ebc;
static int data_00582ec0;
static SInt32 pending_opt_args;
static UInt32 simulation_pipeline_index;
static UInt32 opt_arg_index;
static CompletionEntry data_00582ed0[6];

static inline void record_opt_arg(PCodeInstruction *p)
{
    pending_opt_args++;
    data_00582ec0--;
    data_00582ed0[opt_arg_index].instr = p;
    data_00582ed0[opt_arg_index].completed = 0;
    opt_arg_index = (opt_arg_index + 1) % 6;
}

int fn_0052f330(PCodeInstruction *instruction)
{
    int count;
    unsigned int flags;

    flags = instruction->flags;
    count = data_00576f28[instruction->opcode].latency;
    if ((flags & PCodeInstruction_CloneExtraOperandExcluded) != 0) {
        count += 2;
    }
    if ((instruction->opcode == PC_LMW) || (instruction->opcode == PC_STMW)) {
        count += instruction->operand_count - 2;
    }
    return count;
}

void reset_simulation_pipeline(void)
{
    int stage;

    for (stage = 0; stage < 9; ++stage) {
        (&data_00582e70)[stage].instr = NULL;
    }
    data_00582ec0 = 6;
    pending_opt_args = 0;
    simulation_pipeline_index = 0;
    opt_arg_index = 0;
    data_00582ed0[0].instr = NULL;
    data_00582ed0[1].instr = NULL;
    data_00582ed0[2].instr = NULL;
    data_00582ed0[3].instr = NULL;
    data_00582ed0[4].instr = NULL;
    data_00582ed0[5].instr = NULL;
    data_00582eb8 = NULL;
    data_00582ebc = NULL;
}

int fn_0052f120(struct PCodeInstruction *instruction)
{
    int kind;
    PCodeInstruction *register1;
    unsigned int register1Absent;
    int register2Absent;
    int firstAbsent;
    PCodeInstruction *register2;
    struct PCodeInstruction *other;
    if (data_00582ec0 == 0)
        return 0;
    kind = data_00576f28[instruction->opcode].executionUnit;
    if (kind == 2) {
        firstAbsent = 0;
        if ((register1 = data_00582e78.instr) == NULL)
            firstAbsent = 1;
        register1Absent = firstAbsent;
        register2Absent = 0;
        if ((register2 = data_00582e80.instr) == NULL)
            register2Absent = 1;
        if (firstAbsent == 0 && register2Absent == 0)
            return 0;
        if (register1Absent != 0 && register2Absent != 0)
            return 1;
        if (register1Absent != 0)
            register1 = register2;
        if (Scheduler_ReturnZero(instruction, register1, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(instruction, data_00582eb8, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(instruction, data_00582ebc, 0) != 0)
            return 0;
    } else if ((&data_00582e70)[kind].instr != NULL)
        return 0;
    if ((instruction->flags & PCodeInstruction_ImplicitDefinition) != 0 && (other = data_00582e90.instr) != NULL &&
        (other->flags & PCodeInstruction_ImplicitDefinition) != 0)
        return 0;
    return 1;
}

void record_instruction_kind(PCodeInstruction *p)
{
    SInt32 kind, value;

    kind = data_00576f28[p->opcode].executionUnit;
    value = data_00576f28[p->opcode].stageCycles[0];
    record_opt_arg(p);
    if (kind == 2 && data_00582e78.instr == NULL)
        kind = 1;
    (&data_00582e70)[kind].instr = p;
    (&data_00582e70)[kind].remaining = value;
}

void advance_simulation_pipeline(void)
{
    SInt32 i;
    SInt32 v;

    data_00582eb8 = NULL;
    data_00582ebc = NULL;
    i = 0;
    do {
        if ((&data_00582e70)[i].instr != NULL && (&data_00582e70)[i].remaining != 0)
            (&data_00582e70)[i].remaining--;
        i++;
    } while (i < 9);
    if (pending_opt_args != 0 && data_00582ed0[simulation_pipeline_index].completed != 0) {
        data_00582ed0[simulation_pipeline_index].instr = NULL;
        pending_opt_args--;
        data_00582ec0++;
        simulation_pipeline_index = (simulation_pipeline_index + 1) % 6;
        if (pending_opt_args != 0 && data_00582ed0[simulation_pipeline_index].completed != 0) {
            data_00582ed0[simulation_pipeline_index].instr = NULL;
            pending_opt_args--;
            data_00582ec0++;
            simulation_pipeline_index = (simulation_pipeline_index + 1) % 6;
        }
    }
    if (data_00582e78.instr != NULL && data_00582e78.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e78.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582e78.instr = NULL;
        data_00582eb8 = q;
    }
    if (data_00582e90.instr != NULL && data_00582e90.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e90.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582e90.instr = NULL;
    }
    if (data_00582ea8.instr != NULL && data_00582ea8.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582ea8.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582ea8.instr = NULL;
    }
    if (data_00582eb0.instr != NULL && data_00582eb0.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582eb0.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582eb0.instr = NULL;
    }
    if (data_00582e70.instr != NULL && data_00582e70.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e70.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582e70.instr = NULL;
    }
    if (data_00582e80.instr != NULL && data_00582e80.remaining == 0) {
        SInt32 j;
        PCodeInstruction *q;
        q = data_00582e80.instr;
        for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
            ;
        data_00582ed0[j].completed = 1;
        data_00582e80.instr = NULL;
        data_00582ebc = q;
    }
    {
        PCodeInstruction *p;
        SInt32 j;
        PCodeInstruction *q;
        SInt16 tag;
        if ((p = data_00582e98.instr) != NULL && data_00582e98.remaining == 0 &&
            ((tag = p->opcode) == 0xa8 || tag == 0xa9)) {
            q = data_00582e98.instr;
            for (j = 0; j < 6 && data_00582ed0[j].instr != q; j++)
                ;
            data_00582ed0[j].completed = 1;
            data_00582e98.instr = NULL;
        }
    }
    if (data_00582ea0.instr != NULL && data_00582ea0.remaining == 0 && data_00582ea8.instr == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582ea0.instr)->opcode].stageCycles[2];
        data_00582ea8.instr = q;
        data_00582ea8.remaining = v;
        data_00582ea0.instr = NULL;
    }
    if (data_00582e98.instr != NULL && data_00582e98.remaining == 0 && data_00582ea0.instr == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582e98.instr)->opcode].stageCycles[1];
        data_00582ea0.instr = q;
        data_00582ea0.remaining = v;
        data_00582e98.instr = NULL;
    }
    if (data_00582e88.instr != NULL && data_00582e88.remaining == 0 && data_00582e90.instr == NULL) {
        SInt32 v;
        PCodeInstruction *q;
        v = data_00576f28[(q = data_00582e88.instr)->opcode].stageCycles[1];
        data_00582e90.instr = q;
        data_00582e90.remaining = v;
        data_00582e88.instr = NULL;
    }
}

int get_opcode_table_first_entry(PCodeInstruction *instruction)
{
    return data_00576f28[instruction->opcode].stageCycles[3];
}
