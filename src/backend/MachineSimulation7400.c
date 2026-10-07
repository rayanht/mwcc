#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/MachineSimulation7400.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/DWARF.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Scheduler.h"
#include <string.h>

struct MachineInfo machine7400 = {
    2,
    1,
    (SInt32 (*)(void *))get_adjusted_opcode_table_value,
    reset_pipeline_state,
    (SInt32 (*)(void *))can_issue_instruction_in_pipeline_slots,
    (void (*)(void *))queue_instruction,
    advance_pipeline,
    (SInt32 (*)(void *))lookup_instruction_opcode_entry,
};

/* Each opcode's execution unit, latency and cycles in each stage of the pipeline. */
static struct OpcodeScheduleInfo data_00577660[466] = {
    {0, 0, 0, 0, 0, 0, 0},   /* PC_B */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BL */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BC */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BCLR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BCCTR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BT */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BTLR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BTCTR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BF */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BFLR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BFCTR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDNZ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDNZT */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDNZF */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDZ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDZT */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BDZF */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BLR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BCTR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BCTRL */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_BLRL */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LBZ */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LBZU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LBZX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LBZUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHZ */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHZU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHZX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHZUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHA */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHAU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHAX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHAUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LHBRX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LWZ */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LWZU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LWZX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LWZUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LWBRX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LMW */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STB */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STBU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STBX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STBUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STH */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STHU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STHX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STHUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STHBRX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STW */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STWU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STWX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STWUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STWBRX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STMW */
    {3, 3, 1, 2, 0, 0, 0},   /* PC_DCBF */
    {3, 3, 1, 2, 0, 0, 0},   /* PC_DCBST */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DCBT */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DCBTST */
    {3, 3, 1, 2, 0, 0, 0},   /* PC_DCBZ */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADD */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDE */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDIC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDICR */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDIS */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDME */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ADDZE */
    {1, 19, 19, 0, 0, 0, 0}, /* PC_DIVW */
    {1, 19, 19, 0, 0, 0, 0}, /* PC_DIVWU */
    {1, 5, 5, 0, 0, 0, 0},   /* PC_MULHW */
    {1, 6, 5, 0, 0, 0, 0},   /* PC_MULHWU */
    {1, 3, 3, 0, 0, 0, 0},   /* PC_MULLI */
    {1, 5, 5, 0, 0, 0, 0},   /* PC_MULLW */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_NEG */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBF */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBFC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBFE */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBFIC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBFME */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SUBFZE */
    {2, 3, 1, 0, 0, 0, 0},   /* PC_CMPI */
    {2, 3, 1, 0, 0, 0, 0},   /* PC_CMP */
    {2, 3, 1, 0, 0, 0, 0},   /* PC_CMPLI */
    {2, 3, 1, 0, 0, 0, 0},   /* PC_CMPL */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ANDI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ANDIS */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ORI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ORIS */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_XORI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_XORIS */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_AND */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_OR */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_XOR */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_NAND */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_NOR */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_EQV */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ANDC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ORC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_EXTSB */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_EXTSH */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_CNTLZW */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_RLWINM */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_RLWNM */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_RLWIMI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SLW */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SRW */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SRAWI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_SRAW */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRAND */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRANDC */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CREQV */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRNAND */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRNOR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CROR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRORC */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_CRXOR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MCRF */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTXER */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTCTR */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTLR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MTCRF */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MTMSR */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTSPR */
    {8, 1, 1, 0, 0, 0, 0},   /* PC_MFMSR */
    {8, 3, 3, 0, 0, 0, 1},   /* PC_MFSPR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MFXER */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MFCTR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MFLR */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MFCR */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_MFFS */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_MTFSF */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_EIEIO */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_ISYNC */
    {8, 3, 3, 0, 0, 0, 1},   /* PC_SYNC */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_RFI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_LI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_LIS */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_MR */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_NOP */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_NOT */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFS */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFSU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFSX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFSUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFD */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFDU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFDX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LFDUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFS */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFSU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFSX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFSUX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFD */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFDU */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFDX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFDUX */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FMR */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FABS */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FNEG */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FNABS */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FADD */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FADDS */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FSUB */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FSUBS */
    {5, 4, 2, 1, 1, 0, 0},   /* PC_FMUL */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FMULS */
    {5, 31, 31, 0, 0, 0, 0}, /* PC_FDIV */
    {5, 17, 17, 0, 0, 0, 0}, /* PC_FDIVS */
    {5, 4, 2, 1, 1, 0, 0},   /* PC_FMADD */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FMADDS */
    {5, 4, 2, 1, 1, 0, 0},   /* PC_FMSUB */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FMSUBS */
    {5, 4, 2, 1, 1, 0, 0},   /* PC_FNMADD */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FNMADDS */
    {5, 4, 2, 1, 1, 0, 0},   /* PC_FNMSUB */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FNMSUBS */
    {5, 10, 10, 0, 0, 0, 0}, /* PC_FRES */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FRSQRTE */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FSEL */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FRSP */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FCTIW */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FCTIWZ */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FCMPU */
    {5, 3, 1, 1, 1, 0, 0},   /* PC_FCMPO */
    {3, 2, 1, 1, 0, 0, 1},   /* PC_LWARX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LSWI */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LSWX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STFIWX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STSWI */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STSWX */
    {3, 2, 1, 1, 0, 0, 1},   /* PC_STWCX */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ECIWX */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ECOWX */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_DCBI */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ICBI */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MCRFS */
    {8, 1, 1, 0, 0, 0, 1},   /* PC_MCRXR */
    {8, 1, 1, 0, 0, 0, 0},   /* PC_MFTB */
    {8, 3, 3, 0, 0, 0, 0},   /* PC_MFSR */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTSR */
    {8, 3, 3, 0, 0, 0, 0},   /* PC_MFSRIN */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_MTSRIN */
    {5, 1, 1, 0, 0, 0, 0},   /* PC_MTFSB0 */
    {5, 1, 1, 0, 0, 0, 0},   /* PC_MTFSB1 */
    {5, 1, 1, 0, 0, 0, 0},   /* PC_MTFSFI */
    {8, 2, 2, 0, 0, 0, 1},   /* PC_SC */
    {5, 1, 1, 0, 0, 0, 0},   /* PC_FSQRT */
    {5, 1, 1, 0, 0, 0, 0},   /* PC_FSQRTS */
    {3, 1, 1, 0, 0, 0, 0},   /* PC_TLBIA */
    {3, 1, 1, 0, 0, 0, 0},   /* PC_TLBIE */
    {3, 1, 1, 0, 0, 0, 0},   /* PC_TLBLD */
    {3, 1, 1, 0, 0, 0, 0},   /* PC_TLBLI */
    {3, 1, 1, 0, 0, 0, 1},   /* PC_TLBSYNC */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_TW */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_TRAP */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_TWI */
    {2, 1, 1, 0, 0, 0, 1},   /* PC_OPWORD */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_MFROM */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_DSA */
    {2, 1, 1, 0, 0, 0, 0},   /* PC_ESA */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_DCCCI */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_DCREAD */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_ICBT */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_ICCCI */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_ICREAD */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_RFCI */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_TLBRE */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_TLBSX */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_TLBWE */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_WRTEE */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_WRTEEI */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_MFDCR */
    {2, 1, 0, 0, 0, 0, 0},   /* PC_MTDCR */
    {3, 3, 1, 2, 0, 0, 0},   /* PC_DCBA */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DSS */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DSSALL */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DST */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DSTT */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DSTST */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_DSTSTT */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVEBX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVEHX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVEWX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVSL */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVSR */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_LVXL */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STVEBX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STVEHX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STVEWX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STVX */
    {3, 2, 1, 1, 0, 0, 0},   /* PC_STVXL */
    {9, 1, 1, 0, 0, 0, 1},   /* PC_MFVSCR */
    {9, 1, 1, 0, 0, 0, 1},   /* PC_MTVSCR */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDCUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VADDFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDSBS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDSHS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDSWS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUBM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUBS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUHM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUHS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUWM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VADDUWS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAND */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VANDC */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGSB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGSH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGSW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGUB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VAVGUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCFSX */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCFUX */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCMPBFP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCMPEQFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPEQUB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPEQUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPEQUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCMPGEFP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCMPGTFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTSB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTSH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTSW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTUB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VCMPGTUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCTSXS */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VCTUXS */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VEXPTEFP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VLOGEFP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VMAXFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXSB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXSH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXSW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXUB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMAXUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VMINFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINSB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINSH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINSW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINUB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMINUW */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGHB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGHH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGHW */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGLB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGLH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRGLW */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULESB */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULESH */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULEUB */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULEUH */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULOSB */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULOSH */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULOUB */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMULOUH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VNOR */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VOR */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKPX */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKSHSS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKSHUS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKSWSS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKSWUS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKUHUM */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKUHUS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKUWUM */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPKUWUS */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VREFP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VRFIM */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VRFIN */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VRFIP */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VRFIZ */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VRLB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VRLH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VRLW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VRSQRTEFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSL */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSLB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSLH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSLO */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSLW */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTW */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTISB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTISH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSPLTISW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSR */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRAB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRAH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRAW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRB */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSRO */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSRW */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBCUW */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VSUBFP */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBSBS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBSHS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBSWS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUBM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUBS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUHM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUHS */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUWM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSUBUWS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VSUMSWS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VSUM2SWS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VSUM4SBS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VSUM4SHS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VSUM4UBS */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKHPX */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKHSB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKHSH */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKLPX */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKLSB */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VUPKLSH */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VXOR */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VMADDFP */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMHADDSHS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMHRADDSHS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMLADDUHM */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMMBM */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMSHM */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMSHS */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMUBM */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMUHM */
    {11, 3, 1, 1, 1, 0, 0},  /* PC_VMSUMUHS */
    {14, 4, 1, 1, 1, 1, 0},  /* PC_VNMSUBFP */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VPERM */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VSEL */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VSLDOI */
    {9, 1, 1, 0, 0, 0, 0},   /* PC_VMR */
    {10, 1, 1, 0, 0, 0, 0},  /* PC_VMRP */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_L */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_LU */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_LX */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_LUX */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_ST */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_STU */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_STX */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PSQ_STUX */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_DCBZ_L */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_ADD */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_SUB */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MUL */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_DIV */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MADD */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MSUB */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_NMADD */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_NMSUB */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_RES */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_SEL */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_ABS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_NABS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_NEG */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_RSQRTE */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_CMPU0 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_CMPO0 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_CMPU1 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_CMPO1 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MERGE00 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MERGE01 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MERGE10 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MERGE11 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_SUM0 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_SUM1 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MULS0 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MULS1 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MADDS0 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_PS_MADDS1 */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLE */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLEQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLIQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLLIQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLLQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SLQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRAIQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRAQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRE */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SREA */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SREQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRIQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRLIQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRLQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_SRQ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_MASKG */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_MASKIR */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_LSCBX */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_DIV */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_DIVS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_DOZ */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_MUL */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_NABS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_ABS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_CLCS */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_DOZI */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_RLMI */
    {0, 0, 0, 0, 0, 0, 0},   /* PC_RRIB */
};

/* The instruction in each of the eighteen execution stages and the cycles it has left there: eighteen statics in a
   row, which the code also indexes from the first. */
static PipelineStage pipeline_slots;
static PipelineStage data_00582f08;
static PipelineStage data_00582f10;
static PipelineStage data_00582f18;
static PipelineStage data_00582f20;
static PipelineStage data_00582f28;
static PipelineStage data_00582f30;
static PipelineStage data_00582f38;
static PipelineStage data_00582f40;
static PipelineStage data_00582f48;
static PipelineStage data_00582f50;
static PipelineStage data_00582f58;
static PipelineStage data_00582f60;
static PipelineStage data_00582f68;
static PipelineStage data_00582f70;
static PipelineStage data_00582f78;
static PipelineStage data_00582f80;
static PipelineStage data_00582f88;
static PCodeInstruction *pipelineCompletedInstruction;
static PCodeInstruction *pipeline_completed_instruction;
static int data_00582f98;
static int queued_instruction_count;
static unsigned int pipeline_index;
static SInt32 simulationWriteIndex;
static CompletionEntry instruction_queue[8];

static void Advance(PCodeInstruction *o, PipelineStage *next, const unsigned char *tab)
{
    int v;
    PCodeInstruction *saved = o;
    v = (int)(char)tab[o->opcode * 7];
    next->instr = saved;
    next->remaining = v;
}

int get_adjusted_opcode_table_value(PCodeInstruction *record)
{
    int result;

    result = (SInt8)data_00577660[record->opcode].baseLatency;
    if ((record->flags & fRecordBit) != 0) {
        result = result + 2;
    }
    if ((record->opcode == PC_LMW) || (record->opcode == PC_STMW)) {
        result = result + (record->operand_count - 2);
    }
    return result;
}

void reset_pipeline_state(void)
{
    int slot;

    for (slot = 0; slot < 18; ++slot) {
        (&pipeline_slots)[slot].instr = NULL;
    }
    data_00582f98 = 8;
    queued_instruction_count = 0;
    pipeline_index = 0;
    simulationWriteIndex = 0;
    instruction_queue[0].instr = NULL;
    instruction_queue[1].instr = NULL;
    instruction_queue[2].instr = NULL;
    instruction_queue[3].instr = NULL;
    instruction_queue[4].instr = NULL;
    instruction_queue[5].instr = NULL;
    instruction_queue[6].instr = NULL;
    instruction_queue[7].instr = NULL;
    pipelineCompletedInstruction = NULL;
    pipeline_completed_instruction = NULL;
}

int can_issue_instruction_in_pipeline_slots(PCodeInstruction *node)
{
    int primaryMissing;
    PCodeInstruction *fourth;
    PCodeInstruction *first;
    unsigned firstAbsent;
    int alternateMissing;
    PCodeInstruction *second;
    int secondMissing, thirdMissing;
    PCodeInstruction *primary;
    PCodeInstruction *third;
    PCodeInstruction *other;
    int fourthMissing;
    int kind, firstMissing;

    if (data_00582f98 == 0)
        return 0;
    kind = data_00577660[node->opcode].kind;
    if (kind == 2) {
        PCodeInstruction *alternate;
        firstAbsent = firstMissing = !(first = data_00582f08.instr);
        alternateMissing = !(alternate = data_00582f10.instr);
        if (!firstMissing) {
            if (!alternateMissing)
                return 0;
        }
        if (firstAbsent && alternateMissing)
            return 1;
        if (firstAbsent)
            first = alternate;
        if (Scheduler_ReturnZero(node, first, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(node, pipelineCompletedInstruction, 0) != 0)
            return 0;
        if (Scheduler_ReturnZero(node, pipeline_completed_instruction, 0) != 0)
            return 0;
    } else if (kind == 14 || kind == 9 || kind == 10 || kind == 11) {
        primaryMissing = !(primary = data_00582f50.instr);
        secondMissing = !(second = data_00582f70.instr);
        thirdMissing = !(third = data_00582f58.instr);
        fourthMissing = !(fourth = data_00582f48.instr);
        if (kind == 10) {
            if (!primaryMissing)
                return 0;
            if (secondMissing) {
                if (!thirdMissing)
                    second = third;
                else if (!fourthMissing)
                    second = fourth;
                else
                    second = NULL;
            }
            if (Scheduler_ReturnZero(node, second, 9) != 0)
                return 0;
        } else {
            if (!secondMissing || !thirdMissing || !fourthMissing)
                return 0;
            if (!primaryMissing && Scheduler_ReturnZero(node, primary, 9) != 0)
                return 0;
        }
    } else if ((&pipeline_slots)[kind].instr != NULL)
        return 0;
    if ((node->flags & fIsWrite) != 0) {
        other = data_00582f20.instr;
        if (other != NULL && (other->flags & fIsWrite) != 0)
            return 0;
    }
    return 1;
}

void queue_instruction(PCodeInstruction *obj)
{ /* signature unknown: cdecl, arguments at [esp+4], [esp+8], ... */
    SInt32 t;
    SInt32 c;

    t = data_00577660[obj->opcode].kind;
    c = data_00577660[obj->opcode].cost;

    queued_instruction_count++;
    data_00582f98--;
    instruction_queue[simulationWriteIndex].instr = obj;
    instruction_queue[simulationWriteIndex].completed = 0;
    simulationWriteIndex = (simulationWriteIndex + 1) & 7;
    if (t == 2 && data_00582f08.instr == NULL) {
        t = 1;
    }
    (&pipeline_slots)[t].instr = obj;
    (&pipeline_slots)[t].remaining = c;
}

void advance_pipeline(void)
{
    int stageIndex;

    pipelineCompletedInstruction = NULL;
    pipeline_completed_instruction = NULL;
    for (stageIndex = 0; stageIndex < 18; stageIndex++) {
        if (((&pipeline_slots)[stageIndex].instr != NULL) && ((&pipeline_slots)[stageIndex].remaining != 0)) {
            --(&pipeline_slots)[stageIndex].remaining;
        }
    }
    if ((queued_instruction_count != 0) && (instruction_queue[pipeline_index].completed != 0)) {
        instruction_queue[pipeline_index].instr = NULL;
        --queued_instruction_count;
        ++data_00582f98;
        pipeline_index = (pipeline_index + 1) & 7;
        if ((queued_instruction_count != 0) && (instruction_queue[pipeline_index].completed != 0)) {
            instruction_queue[pipeline_index].instr = NULL;
            --queued_instruction_count;
            ++data_00582f98;
            pipeline_index = (pipeline_index + 1) & 7;
        }
    }
    if ((data_00582f08.instr != NULL) && (data_00582f08.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f08.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f08.instr = NULL;
        pipelineCompletedInstruction = completedInstruction;
    }
    if ((data_00582f50.instr != NULL) && (data_00582f50.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f50.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f50.instr = NULL;
    }
    if ((data_00582f20.instr != NULL) && (data_00582f20.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f20.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f20.instr = NULL;
    }
    if ((data_00582f38.instr != NULL) && (data_00582f38.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f38.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f38.instr = NULL;
    }
    if ((data_00582f40.instr != NULL) && (data_00582f40.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f40.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f40.instr = NULL;
    }
    if ((pipeline_slots.instr != NULL) && (pipeline_slots.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = pipeline_slots.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        pipeline_slots.instr = NULL;
    }
    if ((data_00582f48.instr != NULL) && (data_00582f48.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f48.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f48.instr = NULL;
    }
    if ((data_00582f68.instr != NULL) && (data_00582f68.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f68.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f68.instr = NULL;
    }
    if ((data_00582f88.instr != NULL) && (data_00582f88.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f88.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f88.instr = NULL;
    }
    if ((data_00582f10.instr != NULL) && (data_00582f10.remaining == 0)) {
        int slotIndex;
        PCodeInstruction *completedInstruction;
        completedInstruction = data_00582f10.instr;
        for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
             slotIndex++) {
        }
        instruction_queue[slotIndex].completed = 1;
        data_00582f10.instr = NULL;
        pipeline_completed_instruction = completedInstruction;
    }
    {
        PCodeInstruction *instruction = data_00582f28.instr;
        if ((instruction != NULL) && (data_00582f28.remaining == 0) &&
            ((instruction->opcode == PC_FDIV) || (instruction->opcode == PC_FDIVS))) {
            int slotIndex;
            PCodeInstruction *completedInstruction;
            completedInstruction = data_00582f28.instr;
            for (slotIndex = 0; (slotIndex < 8) && (instruction_queue[slotIndex].instr != completedInstruction);
                 slotIndex++) {
            }
            instruction_queue[slotIndex].completed = 1;
            data_00582f28.instr = NULL;
        }
    }
    if (((data_00582f30.instr != NULL) && (data_00582f30.remaining == 0)) && (data_00582f38.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f30.instr, &data_00582f38, &data_00577660[0].stage3Latency);
        data_00582f30.instr = NULL;
    }
    if (((data_00582f28.instr != NULL) && (data_00582f28.remaining == 0)) && (data_00582f30.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f28.instr, &data_00582f30, &data_00577660[0].stage2Latency);
        data_00582f28.instr = NULL;
    }
    if (((data_00582f18.instr != NULL) && (data_00582f18.remaining == 0)) && (data_00582f20.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f18.instr, &data_00582f20, &data_00577660[0].stage2Latency);
        data_00582f18.instr = NULL;
    }
    if (((data_00582f60.instr != NULL) && (data_00582f60.remaining == 0)) && (data_00582f68.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f60.instr, &data_00582f68, &data_00577660[0].stage3Latency);
        data_00582f60.instr = NULL;
    }
    if (((data_00582f58.instr != NULL) && (data_00582f58.remaining == 0)) && (data_00582f60.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f58.instr, &data_00582f60, &data_00577660[0].stage2Latency);
        data_00582f58.instr = NULL;
    }
    if (((data_00582f80.instr != NULL) && (data_00582f80.remaining == 0)) && (data_00582f88.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f80.instr, &data_00582f88, &data_00577660[0].stage4Latency);
        data_00582f80.instr = NULL;
    }
    if (((data_00582f78.instr != NULL) && (data_00582f78.remaining == 0)) && (data_00582f80.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f78.instr, &data_00582f80, &data_00577660[0].stage3Latency);
        data_00582f78.instr = NULL;
    }
    if (((data_00582f70.instr != NULL) && (data_00582f70.remaining == 0)) && (data_00582f78.instr == NULL)) {
        PCodeInstruction *instruction;
        Advance(instruction = data_00582f70.instr, &data_00582f78, &data_00577660[0].stage2Latency);
        data_00582f70.instr = NULL;
    }
}

int lookup_instruction_opcode_entry(PCodeInstruction *instruction)
{
    return data_00577660[instruction->opcode].opcodeEntryValue;
}

int fn_0052f370(PCodeInstruction *instruction)
{
    return data_00577660[instruction->opcode].kind == '\n';
}
