#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation604.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/DWARF.h"
#include "compiler/MachineSimulation601.h"
#include "compiler/MachineSimulation603.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"

struct MachineInfo machine604 = {
    4,
    1,
    (SInt32 (*)(void *))get_size_rec_latency,
    fn_0052eb60,
    (SInt32 (*)(void *))can_issue_instruction,
    (void (*)(void *))assign_instruction_to_execution_unit,
    advance_instruction_stages_and_retire,
    (SInt32 (*)(void *))get_opcode_table_value,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline. */
static MachineOpcodeInfo machineOpcodeInfo604[466] = {
    {8, 0, {0, 0, 0, 1}},   /* PC_B */
    {8, 0, {0, 0, 0, 1}},   /* PC_BL */
    {8, 0, {0, 0, 0, 1}},   /* PC_BC */
    {8, 0, {0, 0, 0, 1}},   /* PC_BCLR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BCCTR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BT */
    {8, 0, {0, 0, 0, 1}},   /* PC_BTLR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BTCTR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BF */
    {8, 0, {0, 0, 0, 1}},   /* PC_BFLR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BFCTR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDNZ */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDNZT */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDNZF */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDZ */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDZT */
    {8, 0, {0, 0, 0, 1}},   /* PC_BDZF */
    {8, 0, {0, 0, 0, 1}},   /* PC_BLR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BCTR */
    {8, 0, {0, 0, 0, 1}},   /* PC_BCTRL */
    {8, 0, {0, 0, 0, 1}},   /* PC_BLRL */
    {6, 2, {1, 1, 0, 0}},   /* PC_LBZ */
    {6, 2, {1, 1, 0, 0}},   /* PC_LBZU */
    {6, 2, {1, 1, 0, 0}},   /* PC_LBZX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LBZUX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHZ */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHZU */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHZX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHZUX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHA */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHAU */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHAX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHAUX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LHBRX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LWZ */
    {6, 2, {1, 1, 0, 0}},   /* PC_LWZU */
    {6, 2, {1, 1, 0, 0}},   /* PC_LWZX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LWZUX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LWBRX */
    {6, 2, {1, 1, 0, 0}},   /* PC_LMW */
    {6, 3, {1, 1, 0, 0}},   /* PC_STB */
    {6, 3, {1, 1, 0, 0}},   /* PC_STBU */
    {6, 3, {1, 1, 0, 0}},   /* PC_STBX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STBUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STH */
    {6, 3, {1, 1, 0, 0}},   /* PC_STHU */
    {6, 3, {1, 1, 0, 0}},   /* PC_STHX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STHUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STHBRX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STW */
    {6, 3, {1, 1, 0, 0}},   /* PC_STWU */
    {6, 3, {1, 1, 0, 0}},   /* PC_STWX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STWUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STWBRX */
    {6, 2, {1, 1, 0, 0}},   /* PC_STMW */
    {6, 2, {1, 1, 0, 0}},   /* PC_DCBF */
    {6, 2, {1, 1, 0, 0}},   /* PC_DCBST */
    {6, 2, {1, 1, 0, 0}},   /* PC_DCBT */
    {6, 2, {1, 1, 0, 0}},   /* PC_DCBTST */
    {6, 2, {1, 1, 0, 0}},   /* PC_DCBZ */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADD */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDC */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDE */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDI */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDIC */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDICR */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDME */
    {0, 1, {1, 0, 0, 0}},   /* PC_ADDZE */
    {2, 20, {20, 0, 0, 0}}, /* PC_DIVW */
    {2, 20, {20, 0, 0, 0}}, /* PC_DIVWU */
    {2, 4, {4, 0, 0, 0}},   /* PC_MULHW */
    {2, 4, {4, 0, 0, 0}},   /* PC_MULHWU */
    {2, 3, {3, 0, 0, 0}},   /* PC_MULLI */
    {2, 4, {4, 0, 0, 0}},   /* PC_MULLW */
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
    {8, 1, {1, 0, 0, 0}},   /* PC_CRAND */
    {8, 1, {1, 0, 0, 0}},   /* PC_CRANDC */
    {8, 1, {1, 0, 0, 0}},   /* PC_CREQV */
    {8, 1, {1, 0, 0, 0}},   /* PC_CRNAND */
    {8, 1, {1, 0, 0, 0}},   /* PC_CRNOR */
    {8, 1, {1, 0, 0, 0}},   /* PC_CROR */
    {8, 1, {1, 0, 0, 0}},   /* PC_CRORC */
    {8, 1, {1, 0, 0, 0}},   /* PC_CRXOR */
    {8, 1, {1, 0, 0, 0}},   /* PC_MCRF */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTXER */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTCTR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTLR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTCRF */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTMSR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTSPR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFMSR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFSPR */
    {2, 3, {3, 0, 0, 0}},   /* PC_MFXER */
    {2, 3, {3, 0, 0, 0}},   /* PC_MFCTR */
    {2, 3, {3, 0, 0, 0}},   /* PC_MFLR */
    {2, 3, {3, 0, 0, 0}},   /* PC_MFCR */
    {3, 3, {1, 1, 1, 0}},   /* PC_MFFS */
    {3, 3, {1, 1, 1, 0}},   /* PC_MTFSF */
    {6, 1, {0, 0, 0, 1}},   /* PC_EIEIO */
    {6, 1, {0, 0, 0, 1}},   /* PC_ISYNC */
    {6, 1, {0, 0, 0, 1}},   /* PC_SYNC */
    {6, 1, {1, 0, 0, 1}},   /* PC_RFI */
    {0, 1, {1, 0, 0, 0}},   /* PC_LI */
    {0, 1, {1, 0, 0, 0}},   /* PC_LIS */
    {0, 1, {1, 0, 0, 0}},   /* PC_MR */
    {0, 1, {1, 0, 0, 0}},   /* PC_NOP */
    {0, 1, {1, 0, 0, 0}},   /* PC_NOT */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFS */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFSU */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFSX */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFSUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFD */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFDU */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFDX */
    {6, 3, {1, 1, 0, 0}},   /* PC_LFDUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFS */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFSU */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFSX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFSUX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFD */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFDU */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFDX */
    {6, 3, {1, 1, 0, 0}},   /* PC_STFDUX */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMR */
    {3, 3, {1, 1, 1, 0}},   /* PC_FABS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNEG */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNABS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FADD */
    {3, 3, {1, 1, 1, 0}},   /* PC_FADDS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FSUB */
    {3, 3, {1, 1, 1, 0}},   /* PC_FSUBS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMUL */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMULS */
    {3, 32, {32, 0, 0, 0}}, /* PC_FDIV */
    {3, 18, {18, 0, 0, 0}}, /* PC_FDIVS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMADD */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMADDS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMSUB */
    {3, 3, {1, 1, 1, 0}},   /* PC_FMSUBS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNMADD */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNMADDS */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNMSUB */
    {3, 3, {1, 1, 1, 0}},   /* PC_FNMSUBS */
    {3, 18, {18, 0, 0, 0}}, /* PC_FRES */
    {3, 3, {1, 1, 1, 0}},   /* PC_FRSQRTE */
    {3, 3, {1, 1, 1, 0}},   /* PC_FSEL */
    {3, 3, {1, 1, 1, 0}},   /* PC_FRSP */
    {3, 3, {1, 1, 1, 0}},   /* PC_FCTIW */
    {3, 3, {1, 1, 1, 0}},   /* PC_FCTIWZ */
    {3, 5, {1, 1, 1, 0}},   /* PC_FCMPU */
    {3, 5, {1, 1, 1, 0}},   /* PC_FCMPO */
    {6, 1, {1, 0, 0, 0}},   /* PC_LWARX */
    {6, 1, {1, 0, 0, 0}},   /* PC_LSWI */
    {6, 1, {1, 0, 0, 0}},   /* PC_LSWX */
    {6, 1, {1, 0, 0, 0}},   /* PC_STFIWX */
    {6, 1, {1, 0, 0, 0}},   /* PC_STSWI */
    {6, 1, {1, 0, 0, 0}},   /* PC_STSWX */
    {6, 1, {1, 0, 0, 0}},   /* PC_STWCX */
    {2, 1, {1, 0, 0, 1}},   /* PC_ECIWX */
    {2, 1, {1, 0, 0, 1}},   /* PC_ECOWX */
    {2, 1, {1, 0, 0, 0}},   /* PC_DCBI */
    {2, 1, {1, 0, 0, 0}},   /* PC_ICBI */
    {2, 1, {1, 0, 0, 0}},   /* PC_MCRFS */
    {2, 1, {1, 0, 0, 0}},   /* PC_MCRXR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFTB */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFSR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTSR */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFSRIN */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTSRIN */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTFSB0 */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTFSB1 */
    {2, 1, {1, 0, 0, 0}},   /* PC_MTFSFI */
    {2, 1, {1, 0, 0, 1}},   /* PC_SC */
    {3, 1, {1, 0, 0, 0}},   /* PC_FSQRT */
    {3, 1, {1, 0, 0, 0}},   /* PC_FSQRTS */
    {2, 1, {1, 0, 0, 0}},   /* PC_TLBIA */
    {2, 1, {1, 0, 0, 0}},   /* PC_TLBIE */
    {2, 1, {1, 0, 0, 0}},   /* PC_TLBLD */
    {2, 1, {1, 0, 0, 0}},   /* PC_TLBLI */
    {2, 1, {1, 0, 0, 0}},   /* PC_TLBSYNC */
    {2, 1, {1, 0, 0, 1}},   /* PC_TW */
    {2, 1, {1, 0, 0, 1}},   /* PC_TRAP */
    {2, 1, {1, 0, 0, 1}},   /* PC_TWI */
    {2, 1, {1, 0, 0, 1}},   /* PC_OPWORD */
    {2, 1, {1, 0, 0, 0}},   /* PC_MFROM */
    {2, 1, {1, 0, 0, 1}},   /* PC_DSA */
    {2, 1, {1, 0, 0, 1}},   /* PC_ESA */
    {2, 0, {0, 0, 0, 0}},   /* PC_DCCCI */
    {2, 0, {0, 0, 0, 0}},   /* PC_DCREAD */
    {2, 0, {0, 0, 0, 0}},   /* PC_ICBT */
    {2, 0, {0, 0, 0, 0}},   /* PC_ICCCI */
    {2, 0, {0, 0, 0, 0}},   /* PC_ICREAD */
    {2, 0, {0, 0, 0, 0}},   /* PC_RFCI */
    {2, 0, {0, 0, 0, 0}},   /* PC_TLBRE */
    {2, 0, {0, 0, 0, 0}},   /* PC_TLBSX */
    {2, 0, {0, 0, 0, 0}},   /* PC_TLBWE */
    {2, 0, {0, 0, 0, 0}},   /* PC_WRTEE */
    {2, 0, {0, 0, 0, 0}},   /* PC_WRTEEI */
    {2, 0, {0, 0, 0, 0}},   /* PC_MFDCR */
    {2, 0, {0, 0, 0, 0}},   /* PC_MTDCR */
    {2, 0, {0, 0, 0, 0}},   /* PC_DCBA */
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

/* The instruction in each of the nine execution stages and the cycles it has left there: nine statics in a row,
   which the code also indexes from the first. */
static PipelineStage execution_unit_instructions;
static PipelineStage data_00582d98;
static PipelineStage data_00582da0;
static PipelineStage data_00582da8;
static PipelineStage data_00582db0;
static PipelineStage data_00582db8;
static PipelineStage data_00582dc0;
static PipelineStage data_00582dc8;
static PipelineStage data_00582dd0;
static PCodeInstruction *data_00582dd8;
static PCodeInstruction *data_00582ddc;
static int data_00582de0;
static SInt32 data_00582de4;
static SInt32 instruction_retire_index;
static unsigned int next_instruction_slot;
static CompletionEntry instruction_ring[16];

static void ZeroStages(PipelineStage *stages, SInt32 n)
{
    SInt32 i;
    for (i = 0; i < n; i++)
        stages[i].instr = NULL;
}

static void ZeroEntries(CompletionEntry *entries, SInt32 n)
{
    SInt32 i;
    for (i = 0; i < n; i++)
        entries[i].instr = NULL;
}

SInt32 get_size_rec_latency(PCodeInstruction *p)
{
    SInt32 n = machineOpcodeInfo604[p->opcode].latency;
    if (p->flags & fRecordBit) {
        n += 2;
    }
    if (p->opcode == 0x27 || p->opcode == 0x36) {
        n += p->operand_count - 2;
    }
    return n;
}

void fn_0052eb60(void)
{
    ZeroStages(&execution_unit_instructions, 9);
    data_00582de0 = 0x10;
    data_00582de4 = 0;
    instruction_retire_index = 0;
    next_instruction_slot = 0;
    ZeroEntries(instruction_ring, 16);
    data_00582dd8 = NULL;
    data_00582ddc = NULL;
}

int can_issue_instruction(struct PCodeInstruction *instruction)
{
    unsigned int category;
    PCodeInstruction *candidate;
    PCodeInstruction *ref;
    int firstMissing;
    int secondMissing;
    int noFirst;
    int enabled;
    category = machineOpcodeInfo604[(int)instruction->opcode].executionUnit;
    enabled = data_00582de0;
    if (enabled == 0)
        return 0;
    if (category == 0) {
        firstMissing = noFirst = !execution_unit_instructions.instr;
        secondMissing = 0;
        if ((candidate = data_00582d98.instr) == NULL)
            secondMissing = 1;
        if (noFirst == 0 && secondMissing == 0)
            return 0;
        if (firstMissing != 0 && secondMissing != 0)
            return 1;
        if (firstMissing == 0)
            candidate = execution_unit_instructions.instr;
        if (Scheduler_ReturnZero(instruction, candidate, 0) != 0)
            return 0;
        ref = data_00582dd8;
        if (Scheduler_ReturnZero(instruction, ref, 0) != 0)
            return 0;
        ref = data_00582ddc;
        if (Scheduler_ReturnZero(instruction, ref, 0) != 0)
            return 0;
    } else if ((&execution_unit_instructions)[category].instr != NULL) {
        return 0;
    }
    return 1;
}

void assign_instruction_to_execution_unit(struct PCodeInstruction *instruction)
{
    unsigned int index;
    int value;
    unsigned int opcode = instruction->opcode;
    index = machineOpcodeInfo604[opcode].executionUnit;
    value = machineOpcodeInfo604[opcode].stageCycles[0];
    if ((index == 0) && (execution_unit_instructions.instr != NULL)) {
        index = 1;
    }
    data_00582de4 = 1 + data_00582de4;
    data_00582de0 = data_00582de0 + -1;
    instruction_ring[next_instruction_slot].instr = instruction;
    instruction_ring[next_instruction_slot].completed = 0;
    next_instruction_slot = next_instruction_slot + 1 & 0xf;
    (&execution_unit_instructions)[index].instr = instruction;
    (&execution_unit_instructions)[index].remaining = value;
}

void advance_instruction_stages_and_retire(void)
{
    SInt32 count;
    SInt32 slotIndex;
    SInt32 retiredCount;

    data_00582dd8 = NULL;
    data_00582ddc = NULL;

    slotIndex = 0;
    do {
        if ((&execution_unit_instructions)[slotIndex].instr != NULL &&
            (&execution_unit_instructions)[slotIndex].remaining != 0)
            (&execution_unit_instructions)[slotIndex].remaining--;
        slotIndex++;
    } while (slotIndex < 9);

    retiredCount = 0;
    do {
        if (data_00582de4 == 0)
            break;
        if (instruction_ring[instruction_retire_index].completed == 0)
            break;
        instruction_ring[instruction_retire_index].instr = NULL;
        data_00582de4--;
        data_00582de0++;
        instruction_retire_index = (instruction_retire_index + 1) & 0xF;
        retiredCount++;
    } while (retiredCount < 5);

    if (execution_unit_instructions.instr != NULL && execution_unit_instructions.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = execution_unit_instructions.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        execution_unit_instructions.instr = NULL;
        data_00582dd8 = object;
    }

    if (data_00582d98.instr != NULL && data_00582d98.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582d98.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        data_00582d98.instr = NULL;
        data_00582ddc = object;
    }

    if (data_00582da0.instr != NULL && data_00582da0.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582da0.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        data_00582da0.instr = NULL;
    }

    if (data_00582dc8.instr != NULL && data_00582dc8.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582dc8.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        data_00582dc8.instr = NULL;
    }

    if (data_00582db8.instr != NULL && data_00582db8.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582db8.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        data_00582db8.instr = NULL;
    }

    if (data_00582dd0.instr != NULL && data_00582dd0.remaining == 0) {
        SInt32 ringIndex;
        PCodeInstruction *object;
        object = data_00582dd0.instr;
        ringIndex = 0;
        while (ringIndex < 16 && instruction_ring[ringIndex].instr != object)
            ringIndex++;
        instruction_ring[ringIndex].completed = 1;
        data_00582dd0.instr = NULL;
    }

    {
        SInt32 ringIndex;
        PCodeInstruction *object;
        if ((object = data_00582da8.instr) != NULL && data_00582da8.remaining == 0 &&
            (object->opcode == 0xa8 || object->opcode == 0xa9)) {
            PCodeInstruction *slotObject;
            slotObject = data_00582da8.instr;
            ringIndex = 0;
            while (ringIndex < 16 && instruction_ring[ringIndex].instr != slotObject)
                ringIndex++;
            instruction_ring[ringIndex].completed = 1;
            data_00582da8.instr = NULL;
        }
    }

    if (data_00582db0.instr != NULL && data_00582db0.remaining == 0 && data_00582db8.instr == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = machineOpcodeInfo604[(object = data_00582db0.instr)->opcode].stageCycles[2];
        data_00582db8.instr = object;
        data_00582db8.remaining = count;
        data_00582db0.instr = NULL;
    }

    if (data_00582da8.instr != NULL && data_00582da8.remaining == 0 && data_00582db0.instr == NULL) {
        PCodeInstruction *object;
        count = machineOpcodeInfo604[(object = data_00582da8.instr)->opcode].stageCycles[1];
        data_00582db0.instr = object;
        data_00582db0.remaining = count;
        data_00582da8.instr = NULL;
    }

    if (data_00582dc0.instr != NULL && data_00582dc0.remaining == 0 && data_00582dc8.instr == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = machineOpcodeInfo604[(object = data_00582dc0.instr)->opcode].stageCycles[1];
        data_00582dc8.instr = object;
        data_00582dc8.remaining = count;
        data_00582dc0.instr = NULL;
    }
}

int get_opcode_table_value(PCodeInstruction *instruction)
{
    return machineOpcodeInfo604[instruction->opcode].stageCycles[3];
}
