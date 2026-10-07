#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation821.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LiveVariables.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"

#include "compiler/Scheduler.h"
#include <string.h>

struct MachineInfo machine821 = {
    1,
    0,
    (SInt32 (*)(void *))get_instruction_cost,
    reset_spill_state,
    (SInt32 (*)(void *))fn_005308b0,
    (void (*)(void *))fn_00530830,
    fn_00530660,
    (SInt32 (*)(void *))get_instruction_opcode_table_entry,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline; the table ends at PC_VMINFP,
   short of the last AltiVec opcodes. */
static MachineOpcodeInfo data_00578e50[302] = {
    {0, 0, {0, 0, 0, 0}},   /* PC_B */
    {0, 0, {0, 0, 0, 0}},   /* PC_BL */
    {0, 0, {0, 0, 0, 0}},   /* PC_BC */
    {0, 0, {0, 0, 0, 0}},   /* PC_BCLR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BCCTR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BT */
    {0, 0, {0, 0, 0, 0}},   /* PC_BTLR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BTCTR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BF */
    {0, 0, {0, 0, 0, 0}},   /* PC_BFLR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BFCTR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDNZ */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDNZT */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDNZF */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDZ */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDZT */
    {0, 0, {0, 0, 0, 0}},   /* PC_BDZF */
    {0, 0, {0, 0, 0, 0}},   /* PC_BLR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BCTR */
    {0, 0, {0, 0, 0, 0}},   /* PC_BCTRL */
    {0, 0, {0, 0, 0, 0}},   /* PC_BLRL */
    {0, 0, {0, 0, 0, 0}},   /* PC_LBZ */
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
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBF */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBST */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBT */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBTST */
    {3, 2, {1, 1, 0, 0}},   /* PC_DCBZ */
    {3, 2, {1, 1, 0, 0}},   /* PC_ADD */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDC */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDE */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDI */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDIC */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDICR */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDIS */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDME */
    {1, 1, {1, 0, 0, 0}},   /* PC_ADDZE */
    {1, 1, {1, 0, 0, 0}},   /* PC_DIVW */
    {1, 37, {37, 0, 0, 0}}, /* PC_DIVWU */
    {1, 37, {37, 0, 0, 0}}, /* PC_MULHW */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULHWU */
    {1, 5, {5, 0, 0, 0}},   /* PC_MULLI */
    {1, 3, {3, 0, 0, 0}},   /* PC_MULLW */
    {1, 5, {5, 0, 0, 0}},   /* PC_NEG */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBF */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFC */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFE */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFIC */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFME */
    {1, 1, {1, 0, 0, 0}},   /* PC_SUBFZE */
    {1, 1, {1, 0, 0, 0}},   /* PC_CMPI */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMP */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMPLI */
    {1, 3, {1, 0, 0, 0}},   /* PC_CMPL */
    {1, 3, {1, 0, 0, 0}},   /* PC_ANDI */
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
    {1, 1, {1, 0, 0, 0}},   /* PC_CRAND */
    {5, 1, {1, 0, 0, 0}},   /* PC_CRANDC */
    {5, 1, {1, 0, 0, 0}},   /* PC_CREQV */
    {5, 1, {1, 0, 0, 0}},   /* PC_CRNAND */
    {5, 1, {1, 0, 0, 0}},   /* PC_CRNOR */
    {5, 1, {1, 0, 0, 0}},   /* PC_CROR */
    {5, 1, {1, 0, 0, 0}},   /* PC_CRORC */
    {5, 1, {1, 0, 0, 0}},   /* PC_CRXOR */
    {5, 1, {1, 0, 0, 0}},   /* PC_MCRF */
    {5, 1, {1, 0, 0, 0}},   /* PC_MTXER */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTCTR */
    {0, 2, {2, 0, 0, 0}},   /* PC_MTLR */
    {0, 2, {2, 0, 0, 0}},   /* PC_MTCRF */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTMSR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTSPR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFMSR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFSPR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFXER */
    {7, 3, {1, 1, 1, 0}},   /* PC_MFCTR */
    {7, 3, {1, 1, 1, 0}},   /* PC_MFLR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFCR */
    {1, 1, {1, 0, 0, 0}},   /* PC_MFFS */
    {1, 1, {1, 0, 0, 0}},   /* PC_MTFSF */
    {1, 1, {1, 0, 0, 0}},   /* PC_EIEIO */
    {1, 1, {1, 0, 0, 0}},   /* PC_ISYNC */
    {1, 1, {1, 0, 0, 0}},   /* PC_SYNC */
    {1, 1, {1, 0, 0, 0}},   /* PC_RFI */
    {1, 1, {1, 0, 0, 0}},   /* PC_LI */
    {3, 2, {1, 1, 0, 0}},   /* PC_LIS */
    {3, 2, {1, 1, 0, 0}},   /* PC_MR */
    {3, 2, {1, 1, 0, 0}},   /* PC_NOP */
    {3, 2, {1, 1, 0, 0}},   /* PC_NOT */
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
    {7, 3, {1, 1, 1, 0}},   /* PC_STFD */
    {7, 3, {1, 1, 1, 0}},   /* PC_STFDU */
    {7, 3, {1, 1, 1, 0}},   /* PC_STFDX */
    {7, 3, {1, 1, 1, 0}},   /* PC_STFDUX */
    {7, 3, {1, 1, 1, 0}},   /* PC_FMR */
    {7, 3, {1, 1, 1, 0}},   /* PC_FABS */
    {7, 3, {1, 1, 1, 0}},   /* PC_FNEG */
    {7, 3, {1, 1, 1, 0}},   /* PC_FNABS */
    {7, 4, {2, 1, 1, 0}},   /* PC_FADD */
    {7, 3, {1, 1, 1, 0}},   /* PC_FADDS */
    {7, 33, {33, 0, 0, 0}}, /* PC_FSUB */
    {7, 18, {18, 0, 0, 0}}, /* PC_FSUBS */
    {7, 4, {2, 1, 1, 0}},   /* PC_FMUL */
    {7, 3, {1, 1, 1, 0}},   /* PC_FMULS */
    {7, 4, {2, 1, 1, 0}},   /* PC_FDIV */
    {7, 3, {1, 1, 1, 0}},   /* PC_FDIVS */
    {7, 4, {2, 1, 1, 0}},   /* PC_FMADD */
    {7, 3, {1, 1, 1, 0}},   /* PC_FMADDS */
    {7, 4, {2, 1, 1, 0}},   /* PC_FMSUB */
    {7, 3, {1, 1, 1, 0}},   /* PC_FMSUBS */
    {7, 18, {18, 0, 0, 0}}, /* PC_FNMADD */
    {7, 3, {1, 1, 1, 0}},   /* PC_FNMADDS */
    {7, 3, {1, 1, 1, 0}},   /* PC_FNMSUB */
    {7, 3, {1, 1, 1, 0}},   /* PC_FNMSUBS */
    {7, 3, {1, 1, 1, 0}},   /* PC_FRES */
    {7, 3, {1, 1, 1, 0}},   /* PC_FRSQRTE */
    {7, 5, {1, 1, 1, 0}},   /* PC_FSEL */
    {7, 5, {1, 1, 1, 0}},   /* PC_FRSP */
    {3, 1, {0, 0, 0, 0}},   /* PC_FCTIW */
    {3, 1, {0, 0, 0, 0}},   /* PC_FCTIWZ */
    {3, 1, {0, 0, 0, 0}},   /* PC_FCMPU */
    {3, 1, {0, 0, 0, 0}},   /* PC_FCMPO */
    {3, 1, {0, 0, 0, 0}},   /* PC_LWARX */
    {3, 1, {0, 0, 0, 0}},   /* PC_LSWI */
    {3, 1, {0, 0, 0, 0}},   /* PC_LSWX */
    {1, 1, {0, 0, 0, 0}},   /* PC_STFIWX */
    {1, 1, {0, 0, 0, 0}},   /* PC_STSWI */
    {1, 1, {0, 0, 0, 0}},   /* PC_STSWX */
    {1, 1, {0, 0, 0, 0}},   /* PC_STWCX */
    {1, 1, {0, 0, 0, 0}},   /* PC_ECIWX */
    {1, 1, {0, 0, 0, 0}},   /* PC_ECOWX */
    {1, 1, {0, 0, 0, 0}},   /* PC_DCBI */
    {1, 1, {0, 0, 0, 0}},   /* PC_ICBI */
    {1, 1, {0, 0, 0, 0}},   /* PC_MCRFS */
    {1, 1, {0, 0, 0, 0}},   /* PC_MCRXR */
    {1, 1, {0, 0, 0, 0}},   /* PC_MFTB */
    {1, 1, {0, 0, 0, 0}},   /* PC_MFSR */
    {1, 1, {0, 0, 0, 0}},   /* PC_MTSR */
    {1, 1, {0, 0, 0, 0}},   /* PC_MFSRIN */
    {1, 1, {0, 0, 0, 0}},   /* PC_MTSRIN */
    {1, 1, {0, 0, 0, 0}},   /* PC_MTFSB0 */
    {1, 1, {0, 0, 0, 0}},   /* PC_MTFSB1 */
    {1, 1, {0, 0, 0, 0}},   /* PC_MTFSFI */
    {1, 1, {0, 0, 0, 1}},   /* PC_SC */
    {1, 1, {0, 0, 0, 1}},   /* PC_FSQRT */
    {1, 1, {0, 0, 0, 0}},   /* PC_FSQRTS */
    {1, 1, {0, 0, 0, 0}},   /* PC_TLBIA */
    {1, 1, {0, 0, 0, 0}},   /* PC_TLBIE */
    {1, 1, {0, 0, 0, 0}},   /* PC_TLBLD */
    {1, 1, {0, 0, 0, 0}},   /* PC_TLBLI */
    {1, 1, {0, 0, 0, 0}},   /* PC_TLBSYNC */
    {1, 1, {0, 0, 0, 0}},   /* PC_TW */
    {1, 1, {0, 0, 0, 1}},   /* PC_TRAP */
    {1, 1, {0, 0, 0, 1}},   /* PC_TWI */
    {1, 1, {0, 0, 0, 1}},   /* PC_OPWORD */
    {1, 1, {0, 0, 0, 1}},   /* PC_MFROM */
    {1, 1, {0, 0, 0, 0}},   /* PC_DSA */
    {1, 1, {0, 0, 0, 0}},   /* PC_ESA */
    {1, 1, {0, 0, 0, 0}},   /* PC_DCCCI */
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
    {1, 0, {0, 0, 0, 0}},   /* PC_DSS */
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

/* The instruction in each of the six execution stages and the cycles it has left there: six statics in a row,
   which the code also indexes from the first. */
static PipelineStage data_00583018;
static PipelineStage data_00583020;
static PipelineStage data_00583028;
static PipelineStage data_00583030;
static PipelineStage data_00583038;
static PipelineStage data_00583040;
static int data_00583048;
static UInt32 data_0058304c;
static UInt32 data_00583050;
static unsigned int record_enqueue_index;
static CompletionEntry queue_slots[6];

/* PCodeBlock: the object a liveness entry is indexed by; its block number
 * lives at 0x1c. */

/* TYPESTRUCT record with the byte classification field at 0x0e. */

static inline void EnqueueRecord(PCodeInstruction *instr)
{
    queue_slots[record_enqueue_index].instr = instr;
    queue_slots[record_enqueue_index].completed = 0;
    record_enqueue_index = (record_enqueue_index + 1) % 6;
}

int get_instruction_cost(PCodeInstruction *instruction)
{
    int cost = data_00578e50[instruction->opcode].latency;

    if (instruction->flags & fRecordBit) {
        cost += 2;
    }
    if (instruction->opcode == PC_LMW || instruction->opcode == PC_STMW) {
        cost += instruction->operand_count - 2;
    }
    return cost;
}

void reset_spill_state(void)
{
    data_00583018.instr = NULL;
    data_00583020.instr = NULL;
    data_00583028.instr = NULL;
    data_00583030.instr = NULL;
    data_00583038.instr = NULL;
    data_00583040.instr = NULL;
    data_00583048 = 6;
    data_0058304c = 0;
    data_00583050 = 0;
    record_enqueue_index = 0;
    queue_slots[0].instr = NULL;
    queue_slots[1].instr = NULL;
    queue_slots[2].instr = NULL;
    queue_slots[3].instr = NULL;
    queue_slots[4].instr = NULL;
    queue_slots[5].instr = NULL;
}

int fn_005308b0(struct PCodeInstruction *pcode)
{
    struct PCodeInstruction *other;
    if (data_00583048 == 0)
        return 0;
    if ((&data_00583018)[data_00578e50[pcode->opcode].executionUnit].instr != NULL)
        return 0;
    if ((pcode->flags & fIsWrite) != 0) {
        other = data_00583038.instr;
        if (other != NULL && (other->flags & fIsWrite) != 0)
            return 0;
    }
    return 1;
}

void fn_00530830(PCodeInstruction *instr)
{
    int slot;
    int tableOffset;

    tableOffset = instr->opcode;
    slot = data_00578e50[tableOffset].executionUnit;

    data_0058304c = data_0058304c + 1;
    data_00583048 = data_00583048 - 1;
    EnqueueRecord(instr);
    (&data_00583018)[slot].instr = instr;
    (&data_00583018)[slot].remaining = data_00578e50[tableOffset].stageCycles[0];
}

void fn_00530660(void)
{
    SInt32 i;
    i = 0;
    do {
        if ((&data_00583018)[i].instr != NULL && (&data_00583018)[i].remaining != 0)
            (&data_00583018)[i].remaining--;
        i++;
    } while (i < 6);
    if (data_0058304c > 0 && queue_slots[data_00583050].completed != 0) {
        queue_slots[data_00583050].instr = NULL;
        data_0058304c--;
        data_00583048++;
        data_00583050 = (data_00583050 + 1) % 6;
        if (data_0058304c > 0 && queue_slots[data_00583050].completed != 0) {
            queue_slots[data_00583050].instr = NULL;
            data_0058304c--;
            data_00583048++;
            data_00583050 = (1 + data_00583050) % 6;
        }
    }
    if (data_00583020.instr != NULL && data_00583020.remaining == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583020.instr;
        for (i = 0; i < 6 && queue_slots[i].instr != key; i++)
            ;
        queue_slots[i].completed = 1;
        data_00583020.instr = NULL;
    }
    if (data_00583038.instr != NULL && data_00583038.remaining == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583038.instr;
        for (i = 0; i < 6 && queue_slots[i].instr != key; i++)
            ;
        queue_slots[i].completed = 1;
        data_00583038.instr = NULL;
    }
    if (data_00583018.instr != NULL && data_00583018.remaining == 0) {
        SInt32 i;
        struct PCodeInstruction *key = data_00583018.instr;
        for (i = 0; i < 6 && queue_slots[i].instr != key; i++)
            ;
        queue_slots[i].completed = 1;
        data_00583018.instr = NULL;
    }
    if (data_00583030.instr != NULL && data_00583030.remaining == 0 && data_00583038.instr == NULL) {
        SInt32 count;
        PCodeInstruction *object;
        count = data_00578e50[(object = data_00583030.instr)->opcode].stageCycles[1];
        data_00583038.instr = object;
        data_00583038.remaining = count;
        data_00583030.instr = NULL;
    }
}

int get_instruction_opcode_table_entry(PCodeInstruction *instruction)
{
    return data_00578e50[instruction->opcode].stageCycles[3];
}
