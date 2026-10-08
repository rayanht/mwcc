#define CERROR_FILE "Intrinsics.c"
#include "compiler/common.h"
#include "compiler/Intrinsics.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/FunctionCalls.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/StructMoves.h"

struct Object *data_00587fc0;
SInt16 gUsedVirtualRegistersVR;

#define false 0
#define true 1

typedef void (*GenNodeProc)(ENode *node, SInt32 a, SInt32 b, Operand *op);

typedef void (*PCodeFn)(void *, int, int, void *);

/* Each intrinsic's function object, by its number (the function's u.intconst.hi). */
static Object *intrinsics[313];
/* The intrinsic whose call is being generated. */
static Object *current_intrinsic = NULL;

/* The AltiVec intrinsics' overloads: for each argument type combination, the result type and the instruction. */
static TypePointer stvectorunsignedchar_ptr = {TYPEPOINTER, 4, TYPE(&stvectorunsignedchar)};

static TypePointer stvectorsignedchar_ptr = {TYPEPOINTER, 4, TYPE(&stvectorsignedchar)};

static TypePointer stvectorboolchar_ptr = {TYPEPOINTER, 4, TYPE(&stvectorboolchar)};

static TypePointer stvectorunsignedshort_ptr = {TYPEPOINTER, 4, TYPE(&stvectorunsignedshort)};

static TypePointer stvectorsignedshort_ptr = {TYPEPOINTER, 4, TYPE(&stvectorsignedshort)};

static TypePointer stvectorboolshort_ptr = {TYPEPOINTER, 4, TYPE(&stvectorboolshort)};

static TypePointer stvectorunsignedlong_ptr = {TYPEPOINTER, 4, TYPE(&stvectorunsignedlong)};

static TypePointer stvectorsignedlong_ptr = {TYPEPOINTER, 4, TYPE(&stvectorsignedlong)};

static TypePointer stvectorboollong_ptr = {TYPEPOINTER, 4, TYPE(&stvectorboollong)};

static TypePointer stvectorfloat_ptr = {TYPEPOINTER, 4, TYPE(&stvectorfloat)};

static TypePointer stvectorpixel_ptr = {TYPEPOINTER, 4, TYPE(&stvectorpixel)};

static TypePointer stunsignedchar_ptr = {TYPEPOINTER, 4, TYPE(&stunsignedchar)};

static TypePointer stsignedchar_ptr = {TYPEPOINTER, 4, TYPE(&stsignedchar)};

static TypePointer stunsignedshort_ptr = {TYPEPOINTER, 4, TYPE(&stunsignedshort)};

static TypePointer stsignedshort_ptr = {TYPEPOINTER, 4, TYPE(&stsignedshort)};

static TypePointer stunsignedlong_ptr = {TYPEPOINTER, 4, TYPE(&stunsignedlong)};

static TypePointer stsignedlong_ptr = {TYPEPOINTER, 4, TYPE(&stsignedlong)};

static TypePointer stunsignedint_ptr = {TYPEPOINTER, 4, TYPE(&stunsignedint)};

static TypePointer stsignedint_ptr = {TYPEPOINTER, 4, TYPE(&stsignedint)};

static TypePointer stfloat_ptr = {TYPEPOINTER, 4, TYPE(&stfloat)};

static IntrinsicBinaryEntry vec_add_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VADDUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VADDUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VADDUBM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VADDUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VADDUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VADDUHM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VADDUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VADDUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VADDUWM},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VADDFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_addc_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VADDCUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_adds_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VADDUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VADDUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VADDUBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VADDSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VADDSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VADDSBS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VADDUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VADDUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VADDUHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VADDSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VADDSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VADDSHS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VADDUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VADDUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VADDUWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VADDSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VADDSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VADDSWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_and_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VAND},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VAND},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VAND},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VAND},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VAND},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VAND},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VAND},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VAND},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VAND},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VAND},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VAND},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VAND},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VAND},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VAND},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VAND},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VAND},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VAND},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VAND},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VAND},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VAND},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VAND},
    {TYPE(&stvectorfloat), TYPE(&stvectorboollong), TYPE(&stvectorfloat), PC_VAND},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorboollong), PC_VAND},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VAND},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_andc_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VANDC},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VANDC},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VANDC},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VANDC},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VANDC},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VANDC},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VANDC},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VANDC},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VANDC},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VANDC},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VANDC},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VANDC},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VANDC},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VANDC},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VANDC},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VANDC},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VANDC},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VANDC},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VANDC},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VANDC},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VANDC},
    {TYPE(&stvectorfloat), TYPE(&stvectorboollong), TYPE(&stvectorfloat), PC_VANDC},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorboollong), PC_VANDC},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VANDC},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_avg_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VAVGUB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VAVGSB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VAVGUH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VAVGSH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VAVGUW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VAVGSW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_ceil_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VRFIP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_cmpb_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_cmpeq_table[] = {
    {TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stvectorboollong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_cmpge_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_cmpgt_table[] = {
    {TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stvectorboollong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_ctf_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), PC_VCFUX},
    {TYPE(&stvectorfloat), TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VCFSX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_cts_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorfloat), TYPE(&stsignedint), PC_VCTSXS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_ctu_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorfloat), TYPE(&stsignedint), PC_VCTUXS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_dss_table[] = {
    {TYPE(&stvoid), TYPE(&stsignedint), PC_DSS},
    {NULL, NULL, 0},
};

static SimpleEntry vec_dssall_table[] = {
    {TYPE(&stvoid), PC_DSSALL},
    {NULL, 0},
};

static IntrinsicTripleEntry vec_dst_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorboolchar_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorboolshort_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorpixel_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorboollong_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stvectorfloat_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stunsignedchar_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stsignedchar_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stunsignedshort_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stsignedshort_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stunsignedint_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stsignedint_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stunsignedlong_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stsignedlong_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {TYPE(&stvoid), TYPE(&stfloat_ptr), TYPE(&stsignedint), TYPE(&stsignedint), PC_DST},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_expte_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VEXPTEFP},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_floor_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VRFIM},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_ld_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stvectorunsignedchar_ptr), PC_LVX},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVX},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stvectorsignedchar_ptr), PC_LVX},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVX},
    {TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stvectorboolchar_ptr), PC_LVX},
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stvectorunsignedshort_ptr), PC_LVX},
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVX},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stvectorsignedshort_ptr), PC_LVX},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVX},
    {TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stvectorboolshort_ptr), PC_LVX},
    {TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stvectorpixel_ptr), PC_LVX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stvectorunsignedlong_ptr), PC_LVX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stvectorsignedlong_ptr), PC_LVX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVX},
    {TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stvectorboollong_ptr), PC_LVX},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stvectorfloat_ptr), PC_LVX},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lde_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVEBX},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVEBX},
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVEHX},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVEHX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVEWX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVEWX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVEWX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVEWX},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVEWX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_ldl_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stvectorunsignedchar_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVXL},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stvectorsignedchar_ptr), PC_LVXL},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVXL},
    {TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stvectorboolchar_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stvectorunsignedshort_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVXL},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stvectorsignedshort_ptr), PC_LVXL},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVXL},
    {TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stvectorboolshort_ptr), PC_LVXL},
    {TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stvectorpixel_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stvectorunsignedlong_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVXL},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVXL},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stvectorsignedlong_ptr), PC_LVXL},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVXL},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVXL},
    {TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stvectorboollong_ptr), PC_LVXL},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stvectorfloat_ptr), PC_LVXL},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVXL},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_loge_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VLOGEFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lvsl_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVSL},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lvsr_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVSR},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_madd_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMADDFP},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_madds_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort),
     PC_VMHADDSHS},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_max_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMAXUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VMAXUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VMAXUB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMAXSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VMAXSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VMAXSB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMAXUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VMAXUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VMAXUH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMAXSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VMAXSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VMAXSH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMAXUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VMAXUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VMAXUW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMAXSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VMAXSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VMAXSW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMAXFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_mergeh_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGHB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMRGHB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VMRGHB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGHH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMRGHH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VMRGHH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VMRGHH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMRGHW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMRGHW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VMRGHW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMRGHW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_mergel_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMRGLB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VMRGLB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMRGLH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VMRGLH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VMRGLH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMRGLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMRGLW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VMRGLW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMRGLW},
    {NULL, NULL, NULL, 0},
};

static SimpleEntry vec_mfvscr_table[] = {
    {TYPE(&stvectorunsignedshort), PC_MFVSCR},
    {NULL, 0},
};

static IntrinsicBinaryEntry vec_min_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMINUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VMINUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VMINUB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMINSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VMINSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VMINSB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMINUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VMINUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VMINUH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMINSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VMINSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VMINSH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMINUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VMINUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VMINUW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMINSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VMINSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VMINSW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMINFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_mladd_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedshort), PC_VMLADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort),
     PC_VMLADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     PC_VMLADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort),
     PC_VMLADDUHM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_mradds_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort),
     PC_VMHRADDSHS},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_msum_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong),
     PC_VMSUMUBM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedlong), PC_VMSUMUHM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedlong),
     PC_VMSUMMBM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong),
     PC_VMSUMSHM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_msums_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedlong), PC_VMSUMUHS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong),
     PC_VMSUMSHS},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_mtvscr_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorpixel), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), PC_MTVSCR},
    {TYPE(&stvoid), TYPE(&stvectorboollong), PC_MTVSCR},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_mule_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMULEUB},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMULESB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMULEUH},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMULESH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_mulo_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMULOUB},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMULOSB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMULOUH},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMULOSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_nmsub_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VNMSUBFP},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_nor_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VNOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VNOR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VNOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VNOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VNOR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VNOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VNOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VNOR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VNOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VNOR},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_or_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VOR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VOR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VOR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VOR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VOR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorboollong), TYPE(&stvectorfloat), PC_VOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorboollong), PC_VOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VOR},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_pack_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VPKUHUM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKUHUM},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VPKUHUM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKUWUM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKUWUM},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VPKUWUM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_packpx_table[] = {
    {TYPE(&stvectorpixel), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKPX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_packs_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VPKUHUS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKSHSS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKUWUS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKSWSS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_packsu_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VPKUHUS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKSHUS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKUWUS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKSWUS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_perm_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VPERM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedchar), PC_VPERM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedchar), PC_VPERM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedchar),
     PC_VPERM},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedchar), PC_VPERM},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorunsignedchar), PC_VPERM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_re_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VREFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_rl_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VRLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VRLB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VRLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VRLH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VRLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VRLW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_round_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VRFIN},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_rsqrte_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VRSQRTEFP},
    {NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_sel_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar),
     PC_VSEL},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar),
     PC_VSEL},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar),
     PC_VSEL},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VSEL},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSEL},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VSEL},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedshort), PC_VSEL},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort),
     PC_VSEL},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort),
     PC_VSEL},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort),
     PC_VSEL},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort),
     PC_VSEL},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VSEL},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong),
     PC_VSEL},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong),
     PC_VSEL},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong),
     PC_VSEL},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VSEL},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSEL},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VSEL},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorunsignedlong), PC_VSEL},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorboollong), PC_VSEL},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sl_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSLB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSLH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSLW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_sld_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stsignedint),
     PC_VSLDOI},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stsignedint), PC_VSLDOI},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stsignedint),
     PC_VSLDOI},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stsignedint), PC_VSLDOI},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stsignedint), PC_VSLDOI},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stsignedint),
     PC_VSLDOI},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VSLDOI},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stsignedint), PC_VSLDOI},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sll_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSL},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedchar), PC_VSL},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedshort), PC_VSL},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSL},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_slo_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), PC_VSLO},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorunsignedchar), PC_VSLO},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorsignedchar), PC_VSLO},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_splat_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), PC_VSPLTB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stsignedint), PC_VSPLTB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stsignedint), PC_VSPLTB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stsignedint), PC_VSPLTW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_s8_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), PC_VSPLTISB},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_s16_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), PC_VSPLTISH},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_s32_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VSPLTISW},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_u8_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), PC_VSPLTISB},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_u16_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), PC_VSPLTISH},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_splat_u32_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), PC_VSPLTISW},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sr_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSRB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSRB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSRH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSRH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSRW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSRW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sra_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSRAB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSRAB},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSRAH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSRAH},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSRAW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSRAW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_srl_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedchar), PC_VSR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedshort), PC_VSR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSR},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sro_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), PC_VSRO},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorunsignedchar), PC_VSRO},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorsignedchar), PC_VSRO},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_st_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stvectorunsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stvectorsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stvectorunsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stvectorsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stvectorunsignedlong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stvectorsignedlong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stvectorfloat_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stvectorpixel_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stvectorboolchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stvectorboolshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stvectorboollong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVX},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_ste_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVEWX},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_stl_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stvectorunsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stvectorsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stvectorunsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stvectorsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stvectorunsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stvectorsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stvectorfloat_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stvectorpixel_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stvectorboolchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stvectorboolshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stvectorboollong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVXL},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVXL},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sub_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VSUBUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VSUBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_subc_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSUBCUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_subs_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSUBUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSUBUBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VSUBSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VSUBSBS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSUBUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSUBUHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VSUBSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VSUBSHS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSUBUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VSUBUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSUBUWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VSUBSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VSUBSWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sum4s_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong), PC_VSUM4UBS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedlong), PC_VSUM4SBS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), PC_VSUM4SHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sum2s_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUM2SWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_sums_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUMSWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_trunc_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VRFIZ},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_unpack2sh_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGHB},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGHH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_unpack2sl_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGLB},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGLH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_unpack2uh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGHB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGHH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_unpack2ul_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGLB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGLH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_unpackh_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VUPKHSB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolchar), PC_VUPKHSB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorpixel), PC_VUPKHPX},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), PC_VUPKHSH},
    {TYPE(&stvectorboollong), TYPE(&stvectorboolshort), PC_VUPKHSH},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_unpackl_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VUPKLSB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolchar), PC_VUPKLSB},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorpixel), PC_VUPKLPX},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), PC_VUPKLSH},
    {TYPE(&stvectorboollong), TYPE(&stvectorboolshort), PC_VUPKLSH},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_xor_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VXOR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VXOR},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VXOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VXOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VXOR},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VXOR},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VXOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VXOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VXOR},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VXOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VXOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VXOR},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VXOR},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VXOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VXOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VXOR},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VXOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VXOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VXOR},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VXOR},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VXOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorboollong), TYPE(&stvectorfloat), PC_VXOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorboollong), PC_VXOR},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VXOR},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_eq_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_ge_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_gt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_in_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_le_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_lt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
};

static IntrinsicTypeEntry vec_all_nan_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_ne_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_nge_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_ngt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_nle_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_all_nlt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_all_numeric_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_eq_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_ge_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_gt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_le_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_lt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_any_nan_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_ne_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VCMPEQUB},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VCMPEQUH},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VCMPEQUW},
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_nge_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_ngt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_nle_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGEFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_nlt_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_any_numeric_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_any_out_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_step_table[] = {
    {TYPE(&stsignedint), TYPE(&stvectorunsignedchar), 16}, {TYPE(&stsignedint), TYPE(&stvectorsignedchar), 16},
    {TYPE(&stsignedint), TYPE(&stvectorboolchar), 16},     {TYPE(&stsignedint), TYPE(&stvectorunsignedshort), 8},
    {TYPE(&stsignedint), TYPE(&stvectorsignedshort), 8},   {TYPE(&stsignedint), TYPE(&stvectorboolshort), 8},
    {TYPE(&stsignedint), TYPE(&stvectorunsignedlong), 4},  {TYPE(&stsignedint), TYPE(&stvectorsignedlong), 4},
    {TYPE(&stsignedint), TYPE(&stvectorboollong), 4},      {TYPE(&stsignedint), TYPE(&stvectorfloat), 4},
    {TYPE(&stsignedint), TYPE(&stvectorpixel), 8},         {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddubm_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VADDUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VADDUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VADDUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VADDUBM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vadduhm_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VADDUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VADDUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VADDUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VADDUHM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vadduwm_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VADDUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VADDUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VADDUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VADDUWM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddfp_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VADDFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddubs_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VADDUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VADDUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VADDUBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddsbs_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VADDSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VADDSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VADDSBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vadduhs_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VADDUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VADDUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VADDUHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddshs_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VADDSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VADDSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VADDSHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vadduws_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VADDUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VADDUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VADDUWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vaddsws_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VADDSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VADDSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VADDSWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavgub_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VAVGUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavgsb_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VAVGSB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavguh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VAVGUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavgsh_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VAVGSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavguw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VAVGUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vavgsw_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VAVGSW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpequb_table[] = {
    {TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPEQUB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPEQUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpequh_table[] = {
    {TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPEQUH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPEQUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpequw_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPEQUW},
    {TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPEQUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpeqfp_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPEQFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtub_table[] = {
    {TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VCMPGTUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtsb_table[] = {
    {TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VCMPGTSB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtuh_table[] = {
    {TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VCMPGTUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtsh_table[] = {
    {TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VCMPGTSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtuw_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VCMPGTUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtsw_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VCMPGTSW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcmpgtfp_table[] = {
    {TYPE(&stvectorboollong), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VCMPGTFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcfux_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), PC_VCFUX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vcfsx_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VCFSX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lvebx_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_LVEBX},
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_LVEBX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lvehx_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_LVEHX},
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_LVEHX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_lvewx_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_LVEWX},
    {TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_LVEWX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_LVEWX},
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_LVEWX},
    {TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_LVEWX},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxub_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMAXUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VMAXUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VMAXUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxsb_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMAXSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VMAXSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VMAXSB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxuh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMAXUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VMAXUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VMAXUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxsh_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMAXSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VMAXSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VMAXSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxuw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMAXUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VMAXUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VMAXUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxsw_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMAXSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VMAXSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VMAXSW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmaxfp_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMAXFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrghb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGHB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMRGHB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VMRGHB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrghh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGHH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMRGHH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VMRGHH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VMRGHH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrghw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMRGHW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMRGHW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VMRGHW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMRGHW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrglb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMRGLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMRGLB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), PC_VMRGLB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrglh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMRGLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMRGLH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VMRGLH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stvectorpixel), PC_VMRGLH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmrglw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMRGLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMRGLW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VMRGLW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMRGLW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminub_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMINUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VMINUB},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VMINUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminsb_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMINSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VMINSB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VMINSB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminuh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMINUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VMINUH},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VMINUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminsh_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMINSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VMINSH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VMINSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminuw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VMINUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VMINUW},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VMINUW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminsw_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VMINSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VMINSW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VMINSW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vminfp_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VMINFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsumubm_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong),
     PC_VMSUMUBM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsumuhm_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedlong), PC_VMSUMUHM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsummbm_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedlong),
     PC_VMSUMMBM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsumshm_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong),
     PC_VMSUMSHM},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsumuhs_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort),
     TYPE(&stvectorunsignedlong), PC_VMSUMUHS},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_vmsumshs_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong),
     PC_VMSUMSHS},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmuleub_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMULEUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmulesb_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMULESB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmuleuh_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMULEUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmulesh_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMULESH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmuloub_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VMULOUB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmulosb_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VMULOSB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmulouh_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VMULOUH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vmulosh_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VMULOSH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkuhum_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VPKUHUM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKUHUM},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), PC_VPKUHUM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkuwum_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKUWUM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKUWUM},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboollong), TYPE(&stvectorboollong), PC_VPKUWUM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkuhus_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VPKUHUS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkshss_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKSHSS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkuwus_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VPKUWUS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkswss_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKSWSS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkshus_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VPKSHUS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vpkswus_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VPKSWUS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vrlb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VRLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VRLB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vrlh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VRLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VRLH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vrlw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VRLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VRLW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vslb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSLB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSLB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vslh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSLH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSLH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vslw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSLW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSLW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vspltb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), PC_VSPLTB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stsignedint), PC_VSPLTB},
    {TYPE(&stvectorboolchar), TYPE(&stvectorboolchar), TYPE(&stsignedint), PC_VSPLTB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsplth_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolshort), TYPE(&stsignedint), PC_VSPLTH},
    {TYPE(&stvectorpixel), TYPE(&stvectorpixel), TYPE(&stsignedint), PC_VSPLTH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vspltw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorboollong), TYPE(&stvectorboollong), TYPE(&stsignedint), PC_VSPLTW},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stsignedint), PC_VSPLTW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vspltisb_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stsignedint), PC_VSPLTISB},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vspltish_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stsignedint), PC_VSPLTISH},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vspltisw_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stsignedint), PC_VSPLTISW},
    {NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsrb_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSRB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSRB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsrh_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSRH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSRH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsrw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSRW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSRW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsrab_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSRAB},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorunsignedchar), PC_VSRAB},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsrah_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSRAH},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorunsignedshort), PC_VSRAH},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsraw_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSRAW},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorunsignedlong), PC_VSRAW},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_stvebx_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedchar), TYPE(&stsignedint), TYPE(&stunsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorsignedchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVEBX},
    {TYPE(&stvoid), TYPE(&stvectorboolchar), TYPE(&stsignedint), TYPE(&stsignedchar_ptr), PC_STVEBX},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_stvehx_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedshort), TYPE(&stsignedint), TYPE(&stunsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorsignedshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorboolshort), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {TYPE(&stvoid), TYPE(&stvectorpixel), TYPE(&stsignedint), TYPE(&stsignedshort_ptr), PC_STVEHX},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicTripleEntry vec_stvewx_table[] = {
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorunsignedlong), TYPE(&stsignedint), TYPE(&stunsignedlong_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorsignedlong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorfloat), TYPE(&stsignedint), TYPE(&stfloat_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedint_ptr), PC_STVEWX},
    {TYPE(&stvoid), TYPE(&stvectorboollong), TYPE(&stsignedint), TYPE(&stsignedlong_ptr), PC_STVEWX},
    {NULL, NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsububm_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBM},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBM},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VSUBUBM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubuhm_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHM},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHM},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VSUBUHM},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubuwm_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VSUBUWM},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VSUBUWM},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VSUBUWM},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VSUBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubfp_table[] = {
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VSUBFP},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsububs_table[] = {
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), PC_VSUBUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), PC_VSUBUBS},
    {TYPE(&stvectorunsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorunsignedchar), PC_VSUBUBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubsbs_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), PC_VSUBSBS},
    {TYPE(&stvectorsignedchar), TYPE(&stvectorboolchar), TYPE(&stvectorsignedchar), PC_VSUBSBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubuhs_table[] = {
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), PC_VSUBUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), PC_VSUBUHS},
    {TYPE(&stvectorunsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorunsignedshort), PC_VSUBUHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubshs_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), PC_VSUBSHS},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorboolshort), TYPE(&stvectorsignedshort), PC_VSUBSHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubuws_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), PC_VSUBUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), PC_VSUBUWS},
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorunsignedlong), PC_VSUBUWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsubsws_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), PC_VSUBSWS},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorboollong), TYPE(&stvectorsignedlong), PC_VSUBSWS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsum4ubs_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorunsignedchar), TYPE(&stvectorunsignedlong), PC_VSUM4UBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsum4sbs_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedchar), TYPE(&stvectorsignedlong), PC_VSUM4SBS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicBinaryEntry vec_vsum4shs_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), TYPE(&stvectorsignedlong), PC_VSUM4SHS},
    {NULL, NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupkhsb_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VUPKHSB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolchar), PC_VUPKHSB},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupklsb_table[] = {
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedchar), PC_VUPKLSB},
    {TYPE(&stvectorboolshort), TYPE(&stvectorboolchar), PC_VUPKLSB},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupkhpx_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorpixel), PC_VUPKHPX},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupklpx_table[] = {
    {TYPE(&stvectorunsignedlong), TYPE(&stvectorpixel), PC_VUPKLPX},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupkhsh_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), PC_VUPKHSH},
    {TYPE(&stvectorboollong), TYPE(&stvectorboolshort), PC_VUPKHSH},
    {NULL, NULL, 0},
};

static IntrinsicTypeEntry vec_vupklsh_table[] = {
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedshort), PC_VUPKLSH},
    {TYPE(&stvectorboollong), TYPE(&stvectorboolshort), PC_VUPKLSH},
    {NULL, NULL, 0},
};

static IntrinsicVariant vec_abs_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBUBM, 0, PC_VMAXSB, 0},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBUHM, 0, PC_VMAXSH, 0},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBUWM, 0, PC_VMAXSW, 0},
    {TYPE(&stvectorfloat), TYPE(&stvectorfloat), PC_VANDC, 0, 0, 0},
    {NULL, NULL, 0, 0, 0, 0},
};

static IntrinsicVariant vec_abss_table[] = {
    {TYPE(&stvectorsignedchar), TYPE(&stvectorsignedchar), PC_VSUBSBS, 0, PC_VMAXSB, 0},
    {TYPE(&stvectorsignedshort), TYPE(&stvectorsignedshort), PC_VSUBSHS, 0, PC_VMAXSH, 0},
    {TYPE(&stvectorsignedlong), TYPE(&stvectorsignedlong), PC_VSUBSWS, 0, PC_VMAXSW, 0},
    {NULL, NULL, 0, 0, 0, 0},
};

static union IntrinsicTableEntry intrinsic_tables[272] = {
    {vec_add_table},
    {vec_addc_table},
    {vec_adds_table},
    {vec_and_table},
    {vec_andc_table},
    {vec_avg_table},
    {vec_ceil_table},
    {vec_cmpb_table},
    {vec_cmpeq_table},
    {vec_cmpge_table},
    {vec_cmpge_table},
    {vec_cmpgt_table},
    {vec_cmpgt_table},
    {vec_ctf_table},
    {vec_cts_table},
    {vec_ctu_table},
    {vec_dss_table},
    {vec_dssall_table},
    {vec_dst_table},
    {vec_dst_table},
    {vec_dst_table},
    {vec_dst_table},
    {vec_expte_table},
    {vec_floor_table},
    {vec_ld_table},
    {vec_lde_table},
    {vec_ldl_table},
    {vec_loge_table},
    {vec_lvsl_table},
    {vec_lvsr_table},
    {vec_madd_table},
    {vec_madds_table},
    {vec_max_table},
    {vec_mergeh_table},
    {vec_mergel_table},
    {vec_mfvscr_table},
    {vec_min_table},
    {vec_mladd_table},
    {vec_mradds_table},
    {vec_msum_table},
    {vec_msums_table},
    {vec_mtvscr_table},
    {vec_mule_table},
    {vec_mulo_table},
    {vec_nmsub_table},
    {vec_nor_table},
    {vec_or_table},
    {vec_pack_table},
    {vec_packpx_table},
    {vec_packs_table},
    {vec_packsu_table},
    {vec_perm_table},
    {vec_re_table},
    {vec_rl_table},
    {vec_round_table},
    {vec_rsqrte_table},
    {vec_sel_table},
    {vec_sl_table},
    {vec_sld_table},
    {vec_sll_table},
    {vec_slo_table},
    {vec_splat_table},
    {vec_splat_s8_table},
    {vec_splat_s16_table},
    {vec_splat_s32_table},
    {vec_splat_u8_table},
    {vec_splat_u16_table},
    {vec_splat_u32_table},
    {vec_sr_table},
    {vec_sra_table},
    {vec_srl_table},
    {vec_sro_table},
    {vec_st_table},
    {vec_ste_table},
    {vec_stl_table},
    {vec_sub_table},
    {vec_subc_table},
    {vec_subs_table},
    {vec_sum4s_table},
    {vec_sum2s_table},
    {vec_sums_table},
    {vec_trunc_table},
    {vec_unpack2sh_table},
    {vec_unpack2sl_table},
    {vec_unpack2uh_table},
    {vec_unpack2ul_table},
    {vec_unpackh_table},
    {vec_unpackl_table},
    {vec_xor_table},
    {vec_all_eq_table},
    {vec_all_ge_table},
    {vec_all_gt_table},
    {vec_all_in_table},
    {vec_all_le_table},
    {vec_all_lt_table},
    {vec_all_nan_table},
    {vec_all_ne_table},
    {vec_all_nge_table},
    {vec_all_ngt_table},
    {vec_all_nle_table},
    {vec_all_nlt_table},
    {vec_all_numeric_table},
    {vec_any_eq_table},
    {vec_any_ge_table},
    {vec_any_gt_table},
    {vec_any_le_table},
    {vec_any_lt_table},
    {vec_any_nan_table},
    {vec_any_ne_table},
    {vec_any_nge_table},
    {vec_any_ngt_table},
    {vec_any_nle_table},
    {vec_any_nlt_table},
    {vec_any_numeric_table},
    {vec_any_out_table},
    {vec_vaddubm_table},
    {vec_vadduhm_table},
    {vec_vadduwm_table},
    {vec_vaddfp_table},
    {vec_addc_table},
    {vec_vaddubs_table},
    {vec_vaddsbs_table},
    {vec_vadduhs_table},
    {vec_vaddshs_table},
    {vec_vadduws_table},
    {vec_vaddsws_table},
    {vec_and_table},
    {vec_andc_table},
    {vec_vavgub_table},
    {vec_vavgsb_table},
    {vec_vavguh_table},
    {vec_vavgsh_table},
    {vec_vavguw_table},
    {vec_vavgsw_table},
    {vec_ceil_table},
    {vec_cmpb_table},
    {vec_vcmpequb_table},
    {vec_vcmpequh_table},
    {vec_vcmpequw_table},
    {vec_vcmpeqfp_table},
    {vec_cmpge_table},
    {vec_vcmpgtub_table},
    {vec_vcmpgtsb_table},
    {vec_vcmpgtuh_table},
    {vec_vcmpgtsh_table},
    {vec_vcmpgtuw_table},
    {vec_vcmpgtsw_table},
    {vec_vcmpgtfp_table},
    {vec_vcfux_table},
    {vec_vcfsx_table},
    {vec_cts_table},
    {vec_ctu_table},
    {vec_expte_table},
    {vec_floor_table},
    {vec_ld_table},
    {vec_lvebx_table},
    {vec_lvehx_table},
    {vec_lvewx_table},
    {vec_ldl_table},
    {vec_loge_table},
    {vec_madd_table},
    {vec_madds_table},
    {vec_vmaxub_table},
    {vec_vmaxsb_table},
    {vec_vmaxuh_table},
    {vec_vmaxsh_table},
    {vec_vmaxuw_table},
    {vec_vmaxsw_table},
    {vec_vmaxfp_table},
    {vec_vmrghb_table},
    {vec_vmrghh_table},
    {vec_vmrghw_table},
    {vec_vmrglb_table},
    {vec_vmrglh_table},
    {vec_vmrglw_table},
    {vec_vminub_table},
    {vec_vminsb_table},
    {vec_vminuh_table},
    {vec_vminsh_table},
    {vec_vminuw_table},
    {vec_vminsw_table},
    {vec_vminfp_table},
    {vec_mladd_table},
    {vec_mradds_table},
    {vec_vmsumubm_table},
    {vec_vmsumuhm_table},
    {vec_vmsummbm_table},
    {vec_vmsumshm_table},
    {vec_vmsumuhs_table},
    {vec_vmsumshs_table},
    {vec_vmuleub_table},
    {vec_vmulesb_table},
    {vec_vmuleuh_table},
    {vec_vmulesh_table},
    {vec_vmuloub_table},
    {vec_vmulosb_table},
    {vec_vmulouh_table},
    {vec_vmulosh_table},
    {vec_nmsub_table},
    {vec_nor_table},
    {vec_or_table},
    {vec_vpkuhum_table},
    {vec_vpkuwum_table},
    {vec_packpx_table},
    {vec_vpkuhus_table},
    {vec_vpkshss_table},
    {vec_vpkuwus_table},
    {vec_vpkswss_table},
    {vec_vpkshus_table},
    {vec_vpkswus_table},
    {vec_perm_table},
    {vec_re_table},
    {vec_vrlb_table},
    {vec_vrlh_table},
    {vec_vrlw_table},
    {vec_round_table},
    {vec_rsqrte_table},
    {vec_sel_table},
    {vec_vslb_table},
    {vec_vslh_table},
    {vec_vslw_table},
    {vec_sld_table},
    {vec_sll_table},
    {vec_slo_table},
    {vec_vspltb_table},
    {vec_vsplth_table},
    {vec_vspltw_table},
    {vec_vspltisb_table},
    {vec_vspltish_table},
    {vec_vspltisw_table},
    {vec_vsrb_table},
    {vec_vsrh_table},
    {vec_vsrw_table},
    {vec_vsrab_table},
    {vec_vsrah_table},
    {vec_vsraw_table},
    {vec_srl_table},
    {vec_sro_table},
    {vec_st_table},
    {vec_stvebx_table},
    {vec_stvehx_table},
    {vec_stvewx_table},
    {vec_stl_table},
    {vec_vsububm_table},
    {vec_vsubuhm_table},
    {vec_vsubuwm_table},
    {vec_vsubfp_table},
    {vec_subc_table},
    {vec_vsububs_table},
    {vec_vsubsbs_table},
    {vec_vsubuhs_table},
    {vec_vsubshs_table},
    {vec_vsubuws_table},
    {vec_vsubsws_table},
    {vec_vsum4ubs_table},
    {vec_vsum4sbs_table},
    {vec_vsum4shs_table},
    {vec_sum2s_table},
    {vec_sums_table},
    {vec_trunc_table},
    {vec_vupkhsb_table},
    {vec_vupklsb_table},
    {vec_vupkhpx_table},
    {vec_vupklpx_table},
    {vec_vupkhsh_table},
    {vec_vupklsh_table},
    {vec_xor_table},
    {vec_abs_table},
    {vec_abss_table},
    {NULL},
    {NULL},
    {NULL},
};

static short intrinsic_opcodes[313] = {
    PC_EIEIO,   PC_SYNC,  PC_ISYNC,  0,          0,        PC_FABS,   PC_FNABS,  0,         0,         PC_CNTLZW,
    PC_LHBRX,   PC_LWBRX, PC_STHBRX, PC_STWBRX,  PC_DCBF,  PC_DCBT,   PC_DCBST,  PC_DCBTST, PC_DCBZ,   PC_MULHW,
    PC_MULHWU,  PC_DIVW,  PC_DIVWU,  PC_FMADD,   PC_FMSUB, PC_FNMADD, PC_FNMSUB, PC_FMADDS, PC_FMSUBS, PC_FNMADDS,
    PC_FNMSUBS, PC_MFFS,  PC_FRES,   PC_FRSQRTE, PC_FSEL,  0,         0,         PC_RLWIMI, PC_RLWINM, PC_RLWNM,
    PC_FABS,    PC_FNABS, 0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,         0,          0,        0,         0,         0,         0,         0,
    0,          0,        0,
};

static TypePointer stchar_ptr = {TYPEPOINTER, 4, TYPE(&stchar)};

static int OpIndex(UInt16 token);
static void *registration_find(const char *name);
static void GenVR(UInt8 *node, Operand *op);
static void GenFPR(UInt8 *node, Operand *op);
static void GenGPR(UInt8 *node, Operand *op);

static inline void Intrinsics_00486bb0_inline1(ENode *p0, Operand *p1)
{
    unsigned char t1;
    t1 = p0->type;
    data_00560648[t1](p0, 0, 0, p1);
    if (p1->kind != OpndType_VR) {
        Operands_ForceVR(p1, p0->rtype, 0);
    }
}

#define CERROR_FILE __FILE__

static inline void Intrinsics_00487090_inline1(ENode *p0, Operand *p1)
{
    unsigned char t3;
    void (*t4)(void *, short, short, void *);
    t3 = p0->type;
    t4 = data_00560648[t3];
    t4(p0, 0, 0, p1);
    if (p1->kind != OpndType_GPR) {
        Operands_ForceGPR(p1, p0->rtype, 0);
    }
}

static inline void unwrapIntrinsicPointerTypes(Type **expected, Type **actual)
{
    if ((*expected)->type == TYPEPOINTER && (*actual)->type == TYPEPOINTER) {
        *expected = ((TypePointer *)*expected)->target;
        *actual = ((TypePointer *)*actual)->target;
    }
}

static inline void fn_00487de0_inline1(ENode *p0, Operand *p1)
{
    unsigned char t3;
    void (*t4)(void *, short, short, void *);
    t3 = p0->type;
    t4 = data_00560648[t3];
    t4(p0, 0, 0, p1);
    if (p1->kind != OpndType_GPR) {
        Operands_ForceGPR(p1, p0->rtype, 0);
    }
}

unsigned int Intrinsics_IsMonadicObjrefTypeFuncFlag200Set(ENode *expression)
{
    ENode *operand = expression->data.monadic;
    unsigned int result = 0;
    int isKind3 = 0;
    if (operand->type == EOBJREF) {
        if (operand->data.objref->datatype == DFUNC) {
            isKind3 = 1;
        }
    }
    if (isKind3 != 0) {
        if ((((TypeFunc *)operand->data.objref->type)->flags & FUNC_INTRINSIC) != 0) {
            result = 1;
        }
    }
    return result;
}

void emit_two_operand_gpr_instruction(SInt16 opcode, ENode *left, ENode *right, SInt16 requestedReg, Operand *result)
{
    Operand leftOperand;
    Operand rightOperand;
    SInt32 resultReg = requestedReg ? requestedReg : gUsedVirtualRegistersGPR++;

    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->type == EINTCONST && right->data.intval.lo == 0) {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind == OpndType_GPR_Indexed) {
            PCodeUtilities_EmitInstruction(opcode, resultReg, leftOperand.reg, leftOperand.secondary_reg);
        } else {
            if (leftOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
            PCodeUtilities_EmitInstruction(opcode, resultReg, 0, leftOperand.reg);
        }
    } else {
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        PCodeUtilities_EmitInstruction(opcode, resultReg, leftOperand.reg, rightOperand.reg);
    }

    Operands_AllocateGPR(((left->flags | right->flags | result->flags) & 0x30000) | 0x400);
    result->kind = OpndType_GPR;
    result->reg = resultReg;
}

void emit_three_gpr_instruction(SInt16 opcode, ENode *destination, ENode *left, ENode *right)
{
    Operand destinationOperand, leftOperand, rightOperand;

    memclrw(&destinationOperand, sizeof(destinationOperand));
    memclrw(&leftOperand, sizeof(leftOperand));
    memclrw(&rightOperand, sizeof(rightOperand));

    if (right->type == EINTCONST && right->data.intval.lo == 0) {
        data_00560648[destination->type](destination, 0, 0, &destinationOperand);
        if (destinationOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&destinationOperand, destination->rtype, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind == OpndType_GPR_Indexed) {
            PCodeUtilities_EmitInstruction(opcode, destinationOperand.reg, leftOperand.reg, leftOperand.secondary_reg);
        } else {
            if (leftOperand.kind != OpndType_GPR)
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
            PCodeUtilities_EmitInstruction(opcode, destinationOperand.reg, 0, leftOperand.reg);
        }
    } else {
        data_00560648[destination->type](destination, 0, 0, &destinationOperand);
        if (destinationOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&destinationOperand, destination->rtype, 0);
        data_00560648[left->type](left, 0, 0, &leftOperand);
        if (leftOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&leftOperand, left->rtype, 0);
        data_00560648[right->type](right, 0, 0, &rightOperand);
        if (rightOperand.kind != OpndType_GPR)
            Operands_ForceGPR(&rightOperand, right->rtype, 0);
        PCodeUtilities_EmitInstruction(opcode, destinationOperand.reg, leftOperand.reg, rightOperand.reg);
    }

    Operands_AllocateGPR(((destinationOperand.flags | leftOperand.flags | rightOperand.flags) & 0x30000) | 0x400);
}

void emit_operation_from_nodes(short operation, ENode *leftNode, ENode *rightNode)
{
    Operand leftOperand;
    Operand rightOperand;
    memclrw(&leftOperand, 22);
    memclrw(&rightOperand, 22);
    PCodeUtilities_ResolveLabel(PCode_NewLabel());
    if (rightNode->type == EINTCONST && rightNode->data.intval.lo == 0) {
        ENode *left = leftNode;
        unsigned char kind = left->type;
        void (*handler)(void *, short, short, void *) = data_00560648[kind];
        handler(left, 0, 0, &leftOperand);
        if (leftOperand.kind == OpndType_GPR_Indexed) {
            PCodeUtilities_EmitInstruction(operation, leftOperand.reg, leftOperand.secondary_reg);
        } else {
            if (leftOperand.kind != OpndType_GPR) {
                Operands_ForceGPR(&leftOperand, left->rtype, 0);
            }
            PCodeUtilities_EmitInstruction(operation, 0, leftOperand.reg);
        }
    } else {
        fn_00487de0_inline1(leftNode, &leftOperand);
        fn_00487de0_inline1(rightNode, &rightOperand);
        PCodeUtilities_EmitInstruction(operation, leftOperand.reg, rightOperand.reg);
    }
    PCodeUtilities_ResolveLabel(PCode_NewLabel());
}

void emit_rlwimi(ENode *destination, ENode *source, ENode *shift, ENode *maskBegin, ENode *maskEnd, short unused,
                 Operand *result)
{
    Operand destinationOperand;
    Operand sourceOperand;

    memclrw(&destinationOperand, sizeof(destinationOperand));
    memclrw(&sourceOperand, sizeof(sourceOperand));
    if (shift->type != EINTCONST || maskBegin->type != EINTCONST || maskEnd->type != EINTCONST)
        CError_FatalError(ERR_ILLEGAL_OPERAND);
    data_00560648[destination->type](destination, 0, 0, &destinationOperand);
    if (destinationOperand.kind)
        Operands_ForceGPR(&destinationOperand, destination->rtype, 0);
    data_00560648[source->type](source, 0, 0, &sourceOperand);
    if (sourceOperand.kind)
        Operands_ForceGPR(&sourceOperand, source->rtype, 0);
    PCodeUtilities_EmitInstruction(PC_RLWIMI, destinationOperand.reg, sourceOperand.reg, shift->data.intval.lo,
                                   maskBegin->data.intval.lo, maskEnd->data.intval.lo);
    result->kind = OpndType_GPR;
    result->reg = destinationOperand.reg;
}

void emit_rlwnm(ENode *value, ENode *shift, ENode *maskBegin, ENode *maskEnd, short targetReg, Operand *result)
{
    Operand valueOperand;
    Operand shiftOperand;
    SInt16 reg;

    memclrw(&valueOperand, sizeof(valueOperand));
    memclrw(&shiftOperand, sizeof(shiftOperand));
    if (maskBegin->type != EINTCONST || maskEnd->type != EINTCONST)
        CError_FatalError(ERR_ILLEGAL_OPERAND);
    data_00560648[value->type](value, 0, 0, &valueOperand);
    if (valueOperand.kind)
        Operands_ForceGPR(&valueOperand, value->rtype, 0);
    data_00560648[shift->type](shift, 0, 0, &shiftOperand);
    if (shiftOperand.kind)
        Operands_ForceGPR(&shiftOperand, shift->rtype, 0);
    if (targetReg != 0)
        reg = targetReg;
    else
        reg = gUsedVirtualRegistersGPR++;
    PCodeUtilities_EmitInstruction(PC_RLWNM, reg, valueOperand.reg, shiftOperand.reg, maskBegin->data.intval.lo,
                                   maskEnd->data.intval.lo);
    result->kind = OpndType_GPR;
    result->reg = reg;
}

unsigned char is_same_type_or_signedint_compatible(struct Type *firstType, struct Type *secondType)
{
    int result;
    int isSpecial;
    struct TypeIntegral *secondIntegral;

    if (firstType == secondType) {
        result = 1;
        return result;
    }
    isSpecial = &firstType->type == &stsignedint.type;
    secondIntegral = (struct TypeIntegral *)secondType;
    if (isSpecial &&
        (secondIntegral == &stunsignedint || secondIntegral == &stsignedchar || secondIntegral == &stunsignedchar ||
         secondIntegral == &stsignedshort || secondIntegral == &stunsignedshort || secondIntegral == &stsignedlong ||
         secondIntegral == &stunsignedlong || secondIntegral == &stbool)) {
        result = 1;
        return result;
    }
    result = 0;
    return result;
}

Type *find_matching_op_result(UInt16 token, ENodeList *args, HashNameNode *name)
{
    ENode *node;
    OpEntry *entry;
    Type *argumentType;
    Type *operandType;

    node = args->node;
    for (entry = intrinsic_tables[OpIndex(token)].op; entry->result; entry++) {
        operandType = entry->operandType;
        argumentType = node->rtype;
        if (operandType->type == TYPEPOINTER && argumentType->type == TYPEPOINTER) {
            operandType = TPTR_TARGET(operandType);
            argumentType = TPTR_TARGET(argumentType);
        }
        if (is_same_type_or_signedint_compatible(operandType, argumentType))
            break;
    }
    if (!entry->result) {
        PPCError_ReportError(0x68, name->name, name->name, node->rtype, 0);
        return NULL;
    }
    return entry->result;
}

static int OpIndex(UInt16 token)
{
    return token - 0x2a;
}

SInt32 select_altivec_mangle_result(UInt16 intrinsicCode, ENodeList *arguments, HashNameNode *name)
{
    ENode *node = arguments->node;
    SInt32 index = intrinsicCode - 0x2a;
    MangleEntry *entry = (MangleEntry *)intrinsic_tables[index].op;
    SInt32 value;
    SInt32 result;

    while (entry->result != 0) {
        Type *argumentType = node->rtype;
        Type *expectedType = entry->type;
        unsigned char matches;
        if (expectedType->type == TYPEPOINTER && argumentType->type == TYPEPOINTER) {
            expectedType = TPTR_TARGET(expectedType);
            argumentType = TPTR_TARGET(argumentType);
        }
        matches = is_same_type_or_signedint_compatible(expectedType, argumentType);
        if (matches)
            break;
        entry++;
    }

    switch (intrinsicCode) {
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x10d:
        case 0x10e:
        case 0x10f:
            if (node->type == EINTCONST) {
                value = (SInt32)node->data.intval.lo;
                if (value > 15 || value < -16) {
                    PPCError_ReportError(0x6c, name->name, name->name, 5);
                    return 0;
                }
            } else {
                PPCError_ReportError(0x6c, name->name, name->name, 5);
                return 0;
            }
            break;
        case 0x3a:
            if (node->type == EINTCONST) {
                value = (SInt32)node->data.intval.lo;
                if (value > 3 || value < 0) {
                    PPCError_ReportError(0x6c, name->name, name->name, 2);
                    return 0;
                }
            } else {
                PPCError_ReportError(0x6c, name->name, name->name, 2);
                return 0;
            }
            break;
    }

    result = entry->result;
    if (result == 0) {
        PPCError_ReportError(0x68, name->name, name->name, node->rtype, 0);
        return 0;
    }
    return result;
}

Type *check_binary_intrinsic_args(UInt16 op, ENodeList *args, HashNameNode *opname)
{
    ENode *leftOperand;
    Type *leftType;
    Type *expectedLeftType;
    UInt32 index;
    ENode *rightOperand;
    IntrinsicBinaryEntry *entry;
    Type *rightType;
    Type *expectedRightType;

    leftOperand = args->node;
    rightOperand = args->next->node;
    index = op - 0x2a;
    entry = intrinsic_tables[index].binary;
    switch (op) {
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x67:
        case 0xbe:
        case 0xbf:
        case 0xc0:
        case 0xc1:
        case 0x10a:
        case 0x10b:
        case 0x10c:
            if (rightOperand->type == EINTCONST) {
                if (rightOperand->data.intval.lo > 0x1f || rightOperand->data.intval.hi < 0) {
                    PPCError_ReportError(0x6c, opname->name, opname->name, 5);
                    return NULL;
                }
            } else {
                PPCError_ReportError(0x6c, opname->name, opname->name, 5);
                return NULL;
            }
            break;
    }
    for (; entry->result; entry++) {
        leftType = leftOperand->rtype;
        rightType = rightOperand->rtype;
        expectedLeftType = entry->leftType;
        expectedRightType = entry->rightType;
        if (expectedLeftType->type == TYPEPOINTER && leftType->type == TYPEPOINTER) {
            expectedLeftType = ((TypePointer *)expectedLeftType)->target;
            leftType = ((TypePointer *)leftType)->target;
        }
        if (expectedRightType->type == TYPEPOINTER && rightType->type == TYPEPOINTER) {
            expectedRightType = ((TypePointer *)expectedRightType)->target;
            rightType = ((TypePointer *)rightType)->target;
        }
        if (is_same_type_or_signedint_compatible(expectedLeftType, leftType) &&
            is_same_type_or_signedint_compatible(expectedRightType, rightType))
            break;
    }
    if (!entry->result) {
        PPCError_ReportError(0x69, opname->name, opname->name, leftOperand->rtype, 0, rightOperand->rtype, 0);
        return NULL;
    }
    return entry->result;
}

Type *match_intrinsic_triple(UInt16 id, ENodeList *args, HashNameNode *name)
{
    ENode *firstArg = args->node;
    ENode *secondArg = args->next->node;
    ENode *thirdArg = args->next->next->node;
    IntrinsicTripleEntry *entry;
    Type *firstActual, *secondActual, *thirdActual;
    Type *firstExpected, *secondExpected, *thirdExpected;
    SInt32 index = id - 42;
    for (entry = intrinsic_tables[index].triple; entry->result; entry++) {
        firstActual = firstArg->rtype;
        secondActual = secondArg->rtype;
        thirdActual = thirdArg->rtype;
        firstExpected = entry->type1;
        secondExpected = entry->type2;
        thirdExpected = entry->type3;
        unwrapIntrinsicPointerTypes(&firstExpected, &firstActual);
        unwrapIntrinsicPointerTypes(&secondExpected, &secondActual);
        unwrapIntrinsicPointerTypes(&thirdExpected, &thirdActual);
        if (is_same_type_or_signedint_compatible(firstExpected, firstActual) &&
            is_same_type_or_signedint_compatible(secondExpected, secondActual) &&
            is_same_type_or_signedint_compatible(thirdExpected, thirdActual))
            break;
    }
    switch (id) {
        case 60:
        case 61:
        case 62:
        case 63:
            if (thirdArg->type == EINTCONST) {
                SInt32 value = thirdArg->data.intval.lo;
                if (value > 3 || value < 0) {
                    PPCError_ReportError(108, name->name, name->name, 2);
                    return NULL;
                }
            } else {
                PPCError_ReportError(108, name->name, name->name, 2);
                return NULL;
            }
            break;
        case 100:
        case 263:
            if (thirdArg->type == EINTCONST) {
                if (thirdArg->data.intval.lo > 15 || thirdArg->data.intval.hi < 0) {
                    PPCError_ReportError(108, name->name, name->name, 4);
                    return NULL;
                }
            } else {
                PPCError_ReportError(108, name->name, name->name, 4);
                return NULL;
            }
            break;
    }
    if (!entry->result) {
        PPCError_ReportError(106, name->name, name->name, firstArg->rtype, 0, secondArg->rtype, 0, thirdArg->rtype, 0);
        return NULL;
    }
    return entry->result;
}

UInt16 find_unary_intrinsic_code(UInt16 id, ENode *unused, ENode *expression)
{
    IntrinsicTypeEntry *entry;
    Type *expressionType, *candidateType;
    UInt32 index;
    index = id - 0x2a;
    for (entry = intrinsic_tables[index].unary; entry->result != NULL; entry++) {
        expressionType = expression->rtype;
        candidateType = entry->type;
        if (candidateType->type == TYPEPOINTER && expressionType->type == TYPEPOINTER) {
            candidateType = ((TypePointer *)candidateType)->target;
            expressionType = ((TypePointer *)expressionType)->target;
        }
        if (is_same_type_or_signedint_compatible(candidateType, expressionType))
            break;
    }
    if (entry->result == NULL)
        CError_FATAL(4054);
    return entry->code;
}

UInt16 find_binary_intrinsic_code(UInt16 id, ENode *unused, ENode *left, ENode *right)
{
    IntrinsicBinaryEntry *record;
    Type *leftType, *rightType, *expectedLeft, *expectedRight;
    UInt32 index;
    index = id - 0x2a;
    for (record = intrinsic_tables[index].binary; record->result != NULL; record++) {
        leftType = left->rtype;
        rightType = right->rtype;
        expectedLeft = record->leftType;
        expectedRight = record->rightType;
        if (expectedLeft->type == TYPEPOINTER && leftType->type == TYPEPOINTER) {
            expectedLeft = ((TypePointer *)expectedLeft)->target;
            leftType = ((TypePointer *)leftType)->target;
        }
        if (expectedRight->type == TYPEPOINTER && rightType->type == TYPEPOINTER) {
            expectedRight = ((TypePointer *)expectedRight)->target;
            rightType = ((TypePointer *)rightType)->target;
        }
        if (is_same_type_or_signedint_compatible(expectedLeft, leftType) &&
            is_same_type_or_signedint_compatible(expectedRight, rightType))
            break;
    }
    if (record->result == NULL)
        CError_FATAL(4093);
    return record->code;
}

UInt16 find_intrinsic_triple_code(UInt16 id, ENode *unused, ENode *firstOperand, ENode *secondOperand,
                                  ENode *thirdOperand)
{
    IntrinsicTripleEntry *entry;
    Type *firstActualType, *secondActualType, *firstExpectedType, *secondExpectedType, *thirdExpectedType,
        *thirdActualType;
    UInt32 intrinsicIndex;

    intrinsicIndex = id - 0x2a;
    for (entry = intrinsic_tables[intrinsicIndex].triple; entry->result != NULL; entry++) {
        firstActualType = firstOperand->rtype;
        secondActualType = secondOperand->rtype;
        thirdActualType = thirdOperand->rtype;
        firstExpectedType = entry->type1;
        secondExpectedType = entry->type2;
        thirdExpectedType = entry->type3;
        if (firstExpectedType->type == TYPEPOINTER && firstActualType->type == TYPEPOINTER) {
            firstExpectedType = TPTR_TARGET(firstExpectedType);
            firstActualType = TPTR_TARGET(firstActualType);
        }
        if (secondExpectedType->type == TYPEPOINTER && secondActualType->type == TYPEPOINTER) {
            secondExpectedType = TPTR_TARGET(secondExpectedType);
            secondActualType = TPTR_TARGET(secondActualType);
        }
        if (thirdExpectedType->type == TYPEPOINTER && thirdActualType->type == TYPEPOINTER) {
            thirdExpectedType = TPTR_TARGET(thirdExpectedType);
            thirdActualType = TPTR_TARGET(thirdActualType);
        }
        if (is_same_type_or_signedint_compatible(firstExpectedType, firstActualType) &&
            is_same_type_or_signedint_compatible(secondExpectedType, secondActualType) &&
            is_same_type_or_signedint_compatible(thirdExpectedType, thirdActualType))
            break;
    }
    if (entry->result == NULL)
        CError_FATAL(4140);
    return entry->code;
}

void emit_three_vr_instruction(ENode *firstExpression, ENode *secondExpression, ENode *thirdExpression,
                               SInt16 targetReg, Operand *result, SInt16 opcode)
{
    Operand firstOperand, secondOperand, thirdOperand;
    SInt16 resultReg;

    memclrw(&firstOperand, sizeof(firstOperand));
    memclrw(&secondOperand, sizeof(secondOperand));
    memclrw(&thirdOperand, sizeof(thirdOperand));

    (*data_00560648[firstExpression->type])(firstExpression, 0, 0, &firstOperand);
    if (firstOperand.kind != OpndType_VR)
        Operands_ForceVR(&firstOperand, firstExpression->rtype, 0);

    (*data_00560648[secondExpression->type])(secondExpression, 0, 0, &secondOperand);
    if (secondOperand.kind != OpndType_VR)
        Operands_ForceVR(&secondOperand, secondExpression->rtype, 0);

    (*data_00560648[thirdExpression->type])(thirdExpression, 0, 0, &thirdOperand);
    if (thirdOperand.kind != OpndType_VR)
        Operands_ForceVR(&thirdOperand, thirdExpression->rtype, 0);

    if (targetReg != 0)
        resultReg = targetReg;
    else
        resultReg = gUsedVirtualRegistersVR++;

    PCodeUtilities_EmitInstruction(opcode, resultReg, firstOperand.reg, secondOperand.reg, thirdOperand.reg);

    result->kind = OpndType_VR;
    result->reg = resultReg;
}

/* Operand/register descriptor filled by the per-enode emit routines. */

void emit_two_gpr_immediate_instruction(ENode *destination, ENode *source, ENode *immediate, SInt16 op)
{
    Operand destinationOperand;
    Operand sourceOperand;

    memclrw(&destinationOperand, sizeof(destinationOperand));
    memclrw(&sourceOperand, sizeof(sourceOperand));
    CError_ASSERT(4397, immediate->type == EINTCONST);
    PCodeUtilities_ResolveLabel(PCode_NewLabel());
    data_00560648[destination->type](destination, 0, 0, &destinationOperand);
    if (destinationOperand.kind)
        Operands_ForceGPR(&destinationOperand, destination->rtype, 0);
    data_00560648[source->type](source, 0, 0, &sourceOperand);
    if (sourceOperand.kind)
        Operands_ForceGPR(&sourceOperand, source->rtype, 0);
    switch (op) {
        case 0xee:
        case 0xf0:
            PCodeUtilities_EmitInstruction(op, destinationOperand.reg, sourceOperand.reg, immediate->data.intval.lo, 0);
            break;
        case 0xef:
        case 0xf1:
            PCodeUtilities_EmitInstruction(op, destinationOperand.reg, sourceOperand.reg, immediate->data.intval.lo);
            break;
        default:
            CError_FATAL(4417);
    }
    PCodeUtilities_ResolveLabel(PCode_NewLabel());
}

void emit_instruction_with_vr_result(ENode *expression, ENode *left, ENode *right, short opcode, Operand *result,
                                     int instruction)
{
    unsigned char nodeType;
    void (*handler)(void *, short, short, void *);
    Operand value;
    union {
        Operand legacy;
        Operand operand;
    } leftValue, rightValue;
    memclrw(&value, sizeof(value));
    memclrw(&leftValue, sizeof(Operand));
    memclrw(&rightValue, sizeof(Operand));
    nodeType = expression->type;
    handler = data_00560648[nodeType];
    (*handler)(expression, 0, 0, &value);
    if (value.kind != OpndType_VR) {
        Operands_ForceVR(&value, expression->rtype, 0);
    }
    Intrinsics_00487090_inline1(left, &leftValue.legacy);
    Intrinsics_00487090_inline1(right, &rightValue.legacy);
    ((unsigned int (*)(unsigned int, int, int, int))PCodeUtilities_EmitInstruction)(
        instruction, value.reg, leftValue.operand.reg, rightValue.operand.reg);
    result->kind = OpndType_VR;
    result->reg = value.reg;
}

void generate_unary_vector_intrinsic(UInt16 token, ENode *unused, ENode *node, SInt16 requestedReg, Operand *result)
{
    IntrinsicVariant *variant;
    SInt16 constantReg;
    SInt16 temporaryReg;
    SInt16 resultReg;
    Operand operand;
    SInt32 intrinsicIndex;

    intrinsicIndex = token - 0x2a;
    variant = intrinsic_tables[intrinsicIndex].variant;
    for (; variant->resultType != NULL; variant++) {
        Type *argumentType = node->rtype;
        Type *variantType = variant->type;

        if (variantType->type == TYPEPOINTER && argumentType->type == TYPEPOINTER) {
            variantType = ((TypePointer *)variantType)->target;
            argumentType = ((TypePointer *)argumentType)->target;
        }
        if (is_same_type_or_signedint_compatible(variantType, argumentType)) {
            break;
        }
    }
    if (variant->resultType == NULL) {
        CError_FATAL(4548);
    }

    constantReg = gUsedVirtualRegistersVR++;
    temporaryReg = gUsedVirtualRegistersVR++;
    if (requestedReg != 0) {
        resultReg = requestedReg;
    } else {
        resultReg = gUsedVirtualRegistersVR++;
    }

    memclrw(&operand, sizeof(operand));
    data_00560648[node->type](node, 0, 0, &operand);
    if (operand.kind != OpndType_VR) {
        Operands_ForceVR(&operand, node->rtype, 0);
    }
    if (node->rtype == TYPE(&stvectorfloat)) {
        PCodeUtilities_EmitInstruction(PC_VSPLTISW, constantReg, -1);
        PCodeUtilities_EmitInstruction(PC_VSLW, temporaryReg, constantReg, constantReg);
        PCodeUtilities_EmitInstruction(variant->op1, resultReg, operand.reg, temporaryReg);
    } else {
        PCodeUtilities_EmitInstruction(PC_VSPLTISB, constantReg, 0);
        PCodeUtilities_EmitInstruction(variant->op1, temporaryReg, constantReg, operand.reg);
        PCodeUtilities_EmitInstruction(variant->op3, resultReg, operand.reg, temporaryReg);
    }
    result->kind = OpndType_VR;
    result->reg = resultReg;
}

void fn_00486db0(UInt16 token, ENode *unused, ENode *node, SInt16 requestedReg, Operand *result)
{
    IntrinsicVariant *variant;
    SInt16 zeroReg;
    SInt16 intermediateReg;
    SInt16 resultReg;
    Operand operand;
    SInt32 index;

    index = token - 0x2a;
    variant = intrinsic_tables[index].variant;
    for (; variant->resultType != NULL; variant++) {
        Type *resultType = node->rtype;
        Type *variantType = variant->type;

        if (variantType->type == TYPEPOINTER && resultType->type == TYPEPOINTER) {
            variantType = ((TypePointer *)variantType)->target;
            resultType = ((TypePointer *)resultType)->target;
        }
        if (is_same_type_or_signedint_compatible(variantType, resultType)) {
            break;
        }
    }
    if (variant->resultType == NULL) {
        CError_FATAL(4614);
    }

    zeroReg = gUsedVirtualRegistersVR++;
    intermediateReg = gUsedVirtualRegistersVR++;
    if (requestedReg != 0) {
        resultReg = requestedReg;
    } else {
        resultReg = gUsedVirtualRegistersVR++;
    }

    memclrw(&operand, sizeof(operand));
    data_00560648[node->type](node, 0, 0, &operand);
    if (operand.kind != OpndType_VR) {
        Operands_ForceVR(&operand, node->rtype, 0);
    }
    PCodeUtilities_EmitInstruction(PC_VSPLTISB, zeroReg, 0);
    PCodeUtilities_EmitInstruction(variant->op1, intermediateReg, zeroReg, operand.reg);
    PCodeUtilities_EmitInstruction(variant->op3, resultReg, operand.reg, intermediateReg);
    result->kind = OpndType_VR;
    result->reg = resultReg;
}

void emit_record_form_condition(ENode *left, ENode *right, short unused, Operand *result, int opcode,
                                unsigned short kind)
{
    short conditionRegister;
    short conditionCode;
    short firstRegister;
    short secondRegister;
    short leftRegister;
    short rightRegister;
    union {
        Operand operand;
        Operand storage;
        char extent[24];
    } leftOperand, rightOperand;

    memclrw(&leftOperand, sizeof(Operand));
    memclrw(&rightOperand, sizeof(Operand));
    Intrinsics_00486bb0_inline1(left, &leftOperand.storage);
    Intrinsics_00486bb0_inline1(right, &rightOperand.storage);
    conditionRegister = gUsedVirtualRegistersVR++;
    firstRegister = leftRegister = leftOperand.operand.reg;
    secondRegister = rightRegister = rightOperand.operand.reg;
    if (((kind == 132 || kind == 145) && left->rtype != TYPE(&stvectorfloat)) ||
        ((kind == 135 || kind == 147) && left->rtype == TYPE(&stvectorfloat)) || kind == 136 || kind == 148 ||
        kind == 141 || kind == 142 || kind == 153 || kind == 154) {
        secondRegister = leftRegister;
        firstRegister = rightRegister;
    }
    ((unsigned int (*)(unsigned int, int, int, int))PCodeUtilities_EmitInstruction)(opcode, conditionRegister,
                                                                                    firstRegister, secondRegister);
    PCodeUtilities_MakeRecordForm(gCurrentBlock->reverse_instructions);
    if (left->rtype == TYPE(&stvectorfloat)) {
        switch (kind) {
            case 131:
            case 132:
            case 133:
            case 135:
            case 136:
                conditionCode = 19;
                break;
            case 150:
            case 151:
            case 152:
            case 153:
            case 154:
                conditionCode = 22;
                break;
            case 134:
            case 138:
            case 139:
            case 140:
            case 141:
            case 142:
                conditionCode = 23;
                break;
            case 144:
            case 145:
            case 146:
            case 147:
            case 148:
            case 156:
                conditionCode = 24;
                break;
            default:
                CError_FATAL(4736);
                break;
        }
    } else {
        switch (kind) {
            case 131:
            case 133:
            case 136:
                conditionCode = 19;
                break;
            case 144:
            case 146:
            case 148:
                conditionCode = 24;
                break;
            case 132:
            case 135:
            case 138:
                conditionCode = 23;
                break;
            case 145:
            case 147:
            case 150:
                conditionCode = 22;
                break;
            default:
                CError_FATAL(4765);
        }
    }
    result->kind = OpndType_CRField;
    result->reg = 6;
    result->secondary_reg = conditionCode;
}

void fn_00486ad0(ENode *node, short unused, Operand *result, short target, unsigned short kind)
{
    unsigned short secondaryReg;
    Operand operand;

    memclrw((unsigned char *)&operand, sizeof(operand));
    data_00560648[node->type](node, 0, 0, &operand);
    if (operand.kind != OpndType_VR) {
        Operands_ForceVR(&operand, node->rtype, 0);
    }
    PCodeUtilities_EmitInstruction(target, gUsedVirtualRegistersVR++, operand.reg, operand.reg);
    PCodeUtilities_MakeRecordForm(gCurrentBlock->reverse_instructions);
    switch (kind) {
        case 0x8f:
            secondaryReg = 0x13;
            break;
        case 0x95:
            secondaryReg = 0x16;
            break;
        case 0x89:
            secondaryReg = 0x17;
            break;
        case 0x9b:
            secondaryReg = 0x18;
            break;
        default:
            CError_FATAL(4809);
            break;
    }
    result->kind = OpndType_CRField;
    result->reg = 6;
    result->secondary_reg = secondaryReg;
}

ENode *Intrinsics_MakeAltivecCall(Object *descriptor, ENodeList *args)
{
    Object *intrinsic = descriptor;
    ENode *result = NULL;
    unsigned short intrinsicID;
    int tableIndex;
    if (copts.altivec_model != 0) {
        switch (tableIndex = (intrinsicID = intrinsic->u.intrinsic)) {
            case 60:
            case 61:
            case 62:
            case 63:
            case 72:
            case 73:
            case 79:
            case 80:
            case 81:
            case 82:
            case 86:
            case 93:
            case 98:
            case 100:
            case 114:
            case 115:
            case 116:
            case 202:
            case 203:
            case 224:
            case 225:
            case 226:
            case 227:
            case 228:
            case 229:
            case 230:
            case 231:
            case 240:
            case 252:
            case 259:
            case 263:
            case 280:
            case 281:
            case 282:
            case 283:
            case 284: {
                ENodeList *argument;
                HashNameNode *name;
                int count;
                int valid;
                TypeFunc *functionType;
                ENode *call;
                Type *operation;
                count = 0;
                argument = args;
                name = intrinsic->name;
                while (argument != NULL) {
                    count++;
                    argument = argument->next;
                }
                if (count != 3) {
                    PPCError_ReportError(103, name->name, count, 3);
                    valid = 0;
                } else {
                    valid = 1;
                }
                if (!valid) {
                    break;
                }
                operation = match_intrinsic_triple(intrinsicID, args, intrinsic->name);
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)lalloc(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = operation;
                call->ignored = 0;
                call->flags = functionType->qual & Q_CV;
                call->data.funccall.funcref = create_objectrefnode(intrinsic);
                call->data.funccall.args = args;
                call->data.funccall.functype = functionType;
                result = CExpr_AdjustFunctionCall(call);
                break;
            }
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 66:
            case 67:
            case 68:
            case 70:
            case 71:
            case 74:
            case 75:
            case 76:
            case 78:
            case 84:
            case 85:
            case 87:
            case 88:
            case 89:
            case 90:
            case 91:
            case 92:
            case 95:
            case 99:
            case 101:
            case 102:
            case 103:
            case 110:
            case 111:
            case 112:
            case 113:
            case 117:
            case 118:
            case 119:
            case 120:
            case 121:
            case 122:
            case 124:
            case 125:
            case 126:
            case 127:
            case 130:
            case 131:
            case 132:
            case 133:
            case 134:
            case 135:
            case 136:
            case 138:
            case 139:
            case 140:
            case 141:
            case 142:
            case 144:
            case 145:
            case 146:
            case 147:
            case 148:
            case 150:
            case 151:
            case 152:
            case 153:
            case 154:
            case 156:
            case 157:
            case 158:
            case 159:
            case 160:
            case 161:
            case 162:
            case 163:
            case 164:
            case 165:
            case 166:
            case 167:
            case 168:
            case 169:
            case 170:
            case 171:
            case 172:
            case 173:
            case 174:
            case 175:
            case 177:
            case 178:
            case 179:
            case 180:
            case 181:
            case 182:
            case 183:
            case 184:
            case 185:
            case 186:
            case 187:
            case 188:
            case 189:
            case 190:
            case 191:
            case 192:
            case 193:
            case 196:
            case 197:
            case 198:
            case 199:
            case 200:
            case 204:
            case 205:
            case 206:
            case 207:
            case 208:
            case 209:
            case 210:
            case 211:
            case 212:
            case 213:
            case 214:
            case 215:
            case 216:
            case 217:
            case 218:
            case 219:
            case 220:
            case 221:
            case 222:
            case 223:
            case 232:
            case 233:
            case 234:
            case 235:
            case 236:
            case 237:
            case 238:
            case 239:
            case 241:
            case 242:
            case 243:
            case 244:
            case 245:
            case 246:
            case 247:
            case 248:
            case 249:
            case 250:
            case 251:
            case 254:
            case 255:
            case 256:
            case 260:
            case 261:
            case 262:
            case 264:
            case 265:
            case 266:
            case 267:
            case 268:
            case 272:
            case 273:
            case 274:
            case 275:
            case 276:
            case 277:
            case 278:
            case 279:
            case 285:
            case 286:
            case 287:
            case 288:
            case 289:
            case 290:
            case 291:
            case 292:
            case 293:
            case 294:
            case 295:
            case 296:
            case 297:
            case 298:
            case 299:
            case 300:
            case 308: {
                ENodeList *argument;
                HashNameNode *name;
                int count;
                int valid;
                TypeFunc *functionType;
                ENode *call;
                Type *operation;
                count = 0;
                argument = args;
                name = intrinsic->name;
                while (argument != NULL) {
                    count++;
                    argument = argument->next;
                }
                if (count != 2) {
                    PPCError_ReportError(103, name->name, count, 2);
                    valid = 0;
                } else {
                    valid = 1;
                }
                if (!valid) {
                    break;
                }
                operation = check_binary_intrinsic_args(intrinsicID, args, intrinsic->name);
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)lalloc(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = operation;
                call->ignored = 0;
                call->flags = functionType->qual & Q_CV;
                call->data.funccall.funcref = create_objectrefnode(intrinsic);
                call->data.funccall.args = args;
                call->data.funccall.functype = functionType;
                result = CExpr_AdjustFunctionCall(call);
                break;
            }
            case 48:
            case 58:
            case 64:
            case 65:
            case 69:
            case 83:
            case 94:
            case 96:
            case 97:
            case 104:
            case 105:
            case 106:
            case 107:
            case 108:
            case 109:
            case 123:
            case 128:
            case 129:
            case 137:
            case 143:
            case 149:
            case 155:
            case 176:
            case 194:
            case 195:
            case 201:
            case 253:
            case 257:
            case 258:
            case 269:
            case 270:
            case 271:
            case 301:
            case 302:
            case 303:
            case 304:
            case 305:
            case 306:
            case 307: {
                ENodeList *argument;
                HashNameNode *name;
                int count;
                int valid;
                TypeFunc *functionType;
                ENode *call;
                Type *operation;
                count = 0;
                argument = args;
                name = intrinsic->name;
                while (argument != NULL) {
                    count++;
                    argument = argument->next;
                }
                if (count != 1) {
                    PPCError_ReportError(103, name->name, count, 1);
                    valid = 0;
                } else {
                    valid = 1;
                }
                if (!valid) {
                    break;
                }
                operation = (Type *)select_altivec_mangle_result(intrinsicID, args, intrinsic->name);
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)lalloc(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = operation;
                call->ignored = 0;
                call->flags = functionType->qual & Q_CV;
                call->data.funccall.funcref = create_objectrefnode(intrinsic);
                call->data.funccall.args = args;
                call->data.funccall.functype = functionType;
                result = CExpr_AdjustFunctionCall(call);
                break;
            }
            case 59:
            case 77: {
                ENodeList *argument;
                HashNameNode *name;
                int count;
                int valid;
                TypeFunc *functionType;
                ENode *call;
                Type *operation;
                count = 0;
                argument = args;
                name = intrinsic->name;
                while (argument != NULL) {
                    count++;
                    argument = argument->next;
                }
                if (count != 0) {
                    PPCError_ReportError(103, name->name, count, 0);
                    valid = 0;
                } else {
                    valid = 1;
                }
                if (!valid) {
                    break;
                }
                tableIndex -= 42;
                operation = (Type *)intrinsic_tables[tableIndex].operation->operation;
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)lalloc(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = operation;
                call->ignored = 0;
                call->flags = functionType->qual & Q_CV;
                call->data.funccall.funcref = create_objectrefnode(intrinsic);
                call->data.funccall.args = args;
                call->data.funccall.functype = functionType;
                result = CExpr_AdjustFunctionCall(call);
                break;
            }
            case 309:
            case 310: {
                ENodeList *argument;
                HashNameNode *name;
                int count;
                int valid;
                TypeFunc *functionType;
                ENode *call;
                Type *operation;
                count = 0;
                argument = args;
                name = intrinsic->name;
                while (argument != NULL) {
                    count++;
                    argument = argument->next;
                }
                if (count != 1) {
                    PPCError_ReportError(103, name->name, count, 1);
                    valid = 0;
                } else {
                    valid = 1;
                }
                if (!valid) {
                    break;
                }
                operation = find_matching_op_result(intrinsicID, args, intrinsic->name);
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)lalloc(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = operation;
                call->ignored = 0;
                call->flags = functionType->qual & Q_CV;
                call->data.funccall.funcref = create_objectrefnode(intrinsic);
                call->data.funccall.args = args;
                call->data.funccall.functype = functionType;
                result = CExpr_AdjustFunctionCall(call);
                break;
            }
        }
    }
    return result;
}

void Intrinsics_GenerateIntrinsicCall(ENode *node, short requestedReg, Operand *result)
{
    ENodeList *args;
    unsigned short intrinsic;
    ENode *callee;
    Object *object;
    callee = node->data.funccall.funcref;
    args = node->data.funccall.args;
    object = callee->data.objref;
    intrinsic = (unsigned short)object->u.func.u;
    current_intrinsic = object;
    switch (intrinsic) {
        case 0:
        case 1:
        case 2: {
            {
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                PCode_AppendInstruction(gCurrentBlock, PCodeUtilities_CreateInstruction(intrinsic_opcodes[intrinsic]));
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                result[0].kind = OpndType_Immediate;
            }
            break;
        }
        case 3:
        case 4: {
            int selected;
            unsigned char *expr;
            {
                Operand operand;
                int temporaryReg;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                GenGPR(expr, &operand);
                temporaryReg = gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_SRAWI, temporaryReg, operand.reg, 0x1f);
                selected = (short)(requestedReg ? requestedReg : gUsedVirtualRegistersGPR++);
                PCodeUtilities_EmitInstruction(PC_XOR, selected, temporaryReg, operand.reg);
                PCodeUtilities_EmitInstruction(PC_SUBF, selected, temporaryReg, selected);
                result[0].kind = OpndType_GPR;
                result[0].reg = selected;
            }
            break;
        }
        case 5:
        case 6:
        case 0x20:
        case 0x21:
        case 0x28:
        case 0x29:
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(intrinsics[intrinsic])->name);
            } else {
                InstrSelection_EmitUnaryFPRInstruction(intrinsic_opcodes[intrinsic], args->node, requestedReg, result);
            }
            break;
        case 7: {
            short selected;
            unsigned char *expr;
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(intrinsics[intrinsic])->name);
            } else {
                Operand operand;
                unsigned int flags;
                int reuseRequested;
                flags = node->ignored;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                GenFPR(expr, &operand);
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                if (flags == 0) {
                    result[0].kind = OpndType_FPR;
                    reuseRequested = false;
                    if (requestedReg != 0 && requestedReg != operand.reg) {
                        reuseRequested = true;
                    }
                    selected = reuseRequested ? requestedReg : gUsedVirtualRegistersFPR++;
                    result[0].reg = selected;
                    PCodeUtilities_EmitInstruction(PC_MFFS, result[0].reg);
                }
                PCodeUtilities_EmitInstruction(PC_MTFSF, 0xff, operand.reg);
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
            }
            break;
        }
        case 8: {
            int selected;
            unsigned char *expr;
            {
                Operand operand;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr == 0x32) {
                    selected = (short)(requestedReg ? requestedReg : gUsedVirtualRegistersGPR++);
                    StackFrameEABI_EmitFrameAllocation(1, selected, ((ENode *)expr)->data.intval.lo);
                } else {
                    GenGPR(expr, &operand);
                    selected = (short)(requestedReg ? requestedReg : gUsedVirtualRegistersGPR++);
                    PCodeUtilities_EmitInstruction(PC_NEG, selected, operand.reg);
                    PCodeUtilities_EmitInstruction(PC_RLWINM, selected, selected, 0, 0, 0x1b);
                    StackFrameEABI_EmitFrameAllocation(0, selected, 0);
                }
                result[0].kind = OpndType_GPR;
                result[0].reg = selected;
            }
            break;
        }
        case 9:
            InstrSelection_EmitUnaryGPRInstruction(PC_CNTLZW, args->node, requestedReg, result);
            break;
        case 10:
        case 11:
            emit_two_operand_gpr_instruction(intrinsic_opcodes[intrinsic], args->node, args->next->node, requestedReg,
                                             result);
            break;
        case 12:
        case 13:
            emit_three_gpr_instruction(intrinsic_opcodes[intrinsic], args->node, args->next->node,
                                       args->next->next->node);
            result[0].kind = OpndType_Immediate;
            break;
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
            emit_operation_from_nodes(intrinsic_opcodes[intrinsic], args->node, args->next->node);
            result[0].kind = OpndType_Immediate;
            break;
        case 19:
        case 20:
        case 21:
        case 22:
            InstrSelection_EmitBinaryGPRInstruction(intrinsic_opcodes[intrinsic], args->node, args->next->node,
                                                    requestedReg, result);
            break;
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 34:
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(intrinsics[intrinsic])->name);
            } else {
                InstrSelection_EmitThreeOperandFPRInstruction(intrinsic_opcodes[intrinsic], args->node,
                                                              args->next->node, args->next->next->node, requestedReg,
                                                              result);
            }
            break;
        case 31:
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(intrinsics[intrinsic])->name);
            } else {
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                result[0].reg = requestedReg ? requestedReg : gUsedVirtualRegistersFPR++;
                PCodeUtilities_EmitInstruction(PC_MFFS, result[0].reg);
                result[0].kind = OpndType_FPR;
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
            }
            break;
        case 35:
            FunctionCalls_GenerateCall(node, result);
            break;
        case 36: {
            ENode *sourceExpr;
            ENode *destExpr;
            if (*(char *)args->next->next->node == '2') {
                Operand destOperand;
                Operand sourceOperand;
                SInt32 alignment;
                unsigned int length;
                length = ((void)(unsigned int)node->ignored,
                          (unsigned int)((ENode *)args->next->next->node)->data.intval.lo);
                sourceExpr = args->next->node;
                destExpr = args->node;
                memclrw(&sourceOperand, sizeof(sourceOperand));
                memclrw(&destOperand, sizeof(destOperand));
                data_00560648[sourceExpr->type](sourceExpr, 0, 0, &destOperand);
                Operands_MakeIndirect(&destOperand, sourceExpr);
                data_00560648[destExpr->type](destExpr, 0, 0, result);
                sourceOperand = result[0];
                Operands_MakeIndirect(&sourceOperand, destExpr);
                if (destExpr->rtype != NULL) {
                    alignment = StackFrameEABI_GetTypeAlignment(destExpr->rtype);
                } else {
                    alignment = 1;
                }
                StructMoves_EmitCopy(&sourceOperand, &destOperand, length, alignment);
            } else {
                FunctionCalls_GenerateCall(node, result);
            }
            break;
        }
        case 37:
            emit_rlwimi(args->node, args->next->node, args->next->next->node, args->next->next->next->node,
                        args->next->next->next->next->node, requestedReg, result);
            break;
        case 38: {
            short selected;
            {
                Operand operand;
                char *shiftExpr;
                ENodeList *maskArgs;
                char *beginExpr;
                unsigned char *expr;
                char *endExpr;
                maskArgs = args->next->next;
                endExpr = (char *)maskArgs->next->node;
                beginExpr = (char *)maskArgs->node;
                shiftExpr = (char *)(int)args->next->node;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*shiftExpr != '2' || *beginExpr != '2' || *endExpr != '2') {
                    CError_FatalError(ERR_ILLEGAL_OPERAND);
                }
                GenGPR(expr, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersGPR++;
                PCodeUtilities_EmitInstruction(PC_RLWINM, selected, operand.reg, ((ENode *)shiftExpr)->data.intval.lo,
                                               ((ENode *)beginExpr)->data.intval.lo,
                                               ((ENode *)endExpr)->data.intval.lo),
                    result[0].kind = OpndType_GPR, result[0].reg = selected;
            }
            break;
        }
        case 39:
            emit_rlwnm(args->node, args->next->node, args->next->next->node, args->next->next->next->node, requestedReg,
                       result);
            break;
        case 0x48:
        case 0x49:
        case 0x4f:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x56:
        case 0x5d:
        case 0x62:
        case 0xca:
        case 0xcb:
        case 0xe0:
        case 0xe1:
        case 0xe2:
        case 0xe3:
        case 0xe4:
        case 0xe5:
        case 0xe6:
        case 0xe7:
        case 0xf0:
        case 0xfc:
        case 0x103: {
            UInt16 opcode;
            opcode = find_intrinsic_triple_code(intrinsic, node, args->node, args->next->node, args->next->next->node);
            emit_three_vr_instruction(args->node, args->next->node, args->next->next->node, requestedReg, result,
                                      opcode);
        } break;
        case 100:
        case 0x107: {
            short selected;
            unsigned char *leftExpr;
            unsigned char *rightExpr;
            Operand rightOperand;
            Operand leftOperand;
            UInt16 opcode;
            char *immediateExpr;
            opcode = find_intrinsic_triple_code(intrinsic, node, args->node, args->next->node, args->next->next->node);
            rightExpr = (unsigned char *)args->next->node;
            immediateExpr = (char *)args->next->next->node;
            leftExpr = (unsigned char *)args->node;
            memclrw(&leftOperand, sizeof(leftOperand));
            memclrw(&rightOperand, sizeof(rightOperand));
            if (*immediateExpr != '2') {
                CError_FATAL(4435);
            }
            GenVR(leftExpr, &leftOperand);
            GenVR(rightExpr, &rightOperand);
            selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
            PCodeUtilities_EmitInstruction(opcode, selected, leftOperand.reg, rightOperand.reg,
                                           ((ENode *)immediateExpr)->data.intval.lo),
                result[0].kind = OpndType_VR, result[0].reg = selected;
        } break;
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x2f:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x35:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4e:
        case 0x54:
        case 0x55:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5f:
        case 99:
        case 0x65:
        case 0x66:
        case 0x6e:
        case 0x6f:
        case 0x70:
        case 0x71:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
        case 0x82:
        case 0x9d:
        case 0x9e:
        case 0x9f:
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
        case 0xad:
        case 0xae:
        case 0xaf:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xcc:
        case 0xcd:
        case 0xce:
        case 0xcf:
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
        case 0xe8:
        case 0xe9:
        case 0xea:
        case 0xeb:
        case 0xec:
        case 0xed:
        case 0xee:
        case 0xef:
        case 0xf1:
        case 0xf2:
        case 0xf3:
        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
        case 0xf8:
        case 0xf9:
        case 0xfa:
        case 0xfb:
        case 0xfe:
        case 0xff:
        case 0x100:
        case 0x104:
        case 0x105:
        case 0x106:
        case 0x108:
        case 0x109:
        case 0x110:
        case 0x111:
        case 0x112:
        case 0x113:
        case 0x114:
        case 0x115:
        case 0x116:
        case 0x117:
        case 0x11d:
        case 0x11e:
        case 0x11f:
        case 0x120:
        case 0x121:
        case 0x122:
        case 0x123:
        case 0x124:
        case 0x125:
        case 0x126:
        case 0x127:
        case 0x128:
        case 0x129:
        case 0x12a:
        case 299:
        case 300:
        case 0x134: {
            short selected;
            unsigned char *rightExpr;
            unsigned char *leftExpr;
            {
                Operand rightOperand;
                Operand leftOperand;
                UInt16 opcode;
                opcode = find_binary_intrinsic_code(intrinsic, node, args->node, args->next->node);
                rightExpr = (unsigned char *)args->next->node;
                leftExpr = (unsigned char *)args->node;
                memclrw(&leftOperand, sizeof(leftOperand));
                memclrw(&rightOperand, sizeof(rightOperand));

                GenVR(leftExpr, &leftOperand);
                GenVR(rightExpr, &rightOperand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, leftOperand.reg, rightOperand.reg),
                    result[0].kind = OpndType_VR, result[0].reg = selected;
            }
        } break;
        case 0x30:
        case 0x40:
        case 0x41:
        case 0x45:
        case 0x5e:
        case 0x60:
        case 0x61:
        case 0x7b:
        case 0x80:
        case 0x81:
        case 0xb0:
        case 0xc2:
        case 0xc3:
        case 0xc9:
        case 0xfd:
        case 0x101:
        case 0x102:
        case 0x12d:
        case 0x12e:
        case 0x12f:
        case 0x130:
        case 0x131:
        case 0x132:
        case 0x133: {
            short selected;
            unsigned char *expr;
            {
                Operand operand;
                UInt16 opcode;
                opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                GenVR(expr, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, operand.reg), result[0].kind = OpndType_VR,
                                                                               result[0].reg = selected;
            }
        } break;
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x67:
        case 0xbe:
        case 0xbf:
        case 0xc0:
        case 0xc1:
        case 0x10a:
        case 0x10b:
        case 0x10c: {
            short selected;
            unsigned char *rightExpr;
            unsigned char *leftExpr;
            {
                Operand rightOperand;
                Operand leftOperand;
                UInt16 opcode;
                opcode = find_binary_intrinsic_code(intrinsic, node, args->node, args->next->node);
                rightExpr = (unsigned char *)args->next->node;
                leftExpr = (unsigned char *)args->node;
                memclrw(&leftOperand, sizeof(leftOperand));
                memclrw(&rightOperand, sizeof(rightOperand));
                if (*rightExpr != 0x32) {
                    CError_FATAL(4342);
                }
                GenVR(leftExpr, &leftOperand);
                data_00560648[*rightExpr](rightExpr, 0, 0, &rightOperand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, leftOperand.reg, rightOperand.immediate),
                    result[0].kind = OpndType_VR, result[0].reg = selected;
            }
        } break;
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x10d:
        case 0x10e:
        case 0x10f: {
            short selected;
            unsigned char *expr;
            {
                Operand operand;
                UInt16 opcode;
                opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr != 0x32) {
                    CError_FATAL(4202);
                }
                data_00560648[*expr](expr, 0, 0, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, operand.immediate), result[0].kind = OpndType_VR,
                                                                                     result[0].reg = selected;
            }
        } break;
        case 0x6b: {
            short selected;
            unsigned char *expr;
            {
                Operand operand;
                UInt16 opcode;
                opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr != 0x32) {
                    CError_FATAL(4226);
                }
                data_00560648[*expr](expr, 0, 0, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, operand.immediate), result[0].kind = OpndType_VR,
                                                                                     result[0].reg = selected;
            }
        } break;
        case 0x6c: {
            short selected;
            unsigned char *expr;
            {
                Operand operand;
                UInt16 opcode;
                opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr != 0x32) {
                    CError_FATAL(4250);
                }
                data_00560648[*expr](expr, 0, 0, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, operand.immediate), result[0].kind = OpndType_VR,
                                                                                     result[0].reg = selected;
            }
        } break;
        case 0x6d: {
            short selected;
            unsigned char *expr;
            {
                Operand operand;
                UInt16 opcode;
                opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr != 0x32) {
                    CError_FATAL(4274);
                }
                data_00560648[*expr](expr, 0, 0, &operand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, operand.immediate), result[0].kind = OpndType_VR,
                                                                                     result[0].reg = selected;
            }
        } break;
        case 0x53: {
            unsigned char *expr;
            {
                Operand operand;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                GenVR(expr, &operand);
                PCodeUtilities_EmitInstruction(PC_MTVSCR, operand.reg);
            }
        } break;
        case 0x4d: {
            short selected;
            unsigned short opcode;
            opcode = intrinsic_tables[(unsigned int)(intrinsic - 0x2a)].simple->code;
            selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
            PCodeUtilities_EmitInstruction(opcode, selected), result[0].kind = OpndType_VR, result[0].reg = selected;
        } break;
        case 0x3a: {
            unsigned char *expr;
            {
                Operand operand;
                expr = (unsigned char *)args->node;
                memclrw(&operand, sizeof(operand));
                if (*expr != 0x32) {
                    CError_FATAL(4297);
                }
                data_00560648[*expr](expr, 0, 0, &operand);
                PCodeUtilities_EmitInstruction(PC_DSS, operand.immediate, 0);
            }
        } break;
        case 0x3b: {
            unsigned short opcode;
            opcode = intrinsic_tables[(unsigned int)(intrinsic - 0x2a)].simple->code;
            PCodeUtilities_EmitInstruction(opcode);
        } break;
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f: {
            short opcode;
            switch (intrinsic) {
                case 0x3c:
                    opcode = 0xee;
                    break;
                case 0x3d:
                    opcode = 0xf0;
                    break;
                case 0x3e:
                    opcode = 0xf1;
                    break;
                case 0x3f:
                    opcode = 0xef;
                    break;
            }
            emit_two_gpr_immediate_instruction(args->node, args->next->node, args->next->next->node, opcode);
        } break;
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x46:
        case 0x47:
        case 0xc4:
        case 0xc5:
        case 0xc6:
        case 199:
        case 200: {
            short selected;
            unsigned char *rightExpr;
            unsigned char *leftExpr;
            {
                Operand rightOperand;
                Operand leftOperand;
                UInt16 opcode;
                opcode = find_binary_intrinsic_code(intrinsic, node, args->node, args->next->node);
                rightExpr = (unsigned char *)args->next->node;
                leftExpr = (unsigned char *)args->node;
                memclrw(&leftOperand, sizeof(leftOperand));
                memclrw(&rightOperand, sizeof(rightOperand));

                GenGPR(leftExpr, &leftOperand);
                GenGPR(rightExpr, &rightOperand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, leftOperand.reg, rightOperand.reg),
                    result[0].kind = OpndType_VR, result[0].reg = selected;
            }
        } break;
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x118:
        case 0x119:
        case 0x11a:
        case 0x11b:
        case 0x11c: {
            UInt16 opcode;
            opcode = find_intrinsic_triple_code(intrinsic, node, args->node, args->next->node, args->next->next->node);
            ((void (*)(ENode *, ENode *, ENode *, short, Operand *, short))emit_instruction_with_vr_result)(
                (ENode *)args->node, (ENode *)args->next->node, (ENode *)args->next->next->node, requestedReg, result,
                (short)opcode);
        } break;
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        case 0x88:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8d:
        case 0x8e:
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x96:
        case 0x97:
        case 0x98:
        case 0x99:
        case 0x9a:
        case 0x9c: {
            UInt16 opcode;
            opcode = find_binary_intrinsic_code(intrinsic, node, args->node, args->next->node);
            ((void (*)(ENode *, ENode *, short, Operand *, short, unsigned short))emit_record_form_condition)(
                (ENode *)args->node, (ENode *)args->next->node, requestedReg, result, (short)opcode, intrinsic);
        } break;
        case 0x89:
        case 0x8f:
        case 0x95:
        case 0x9b: {
            UInt16 opcode;
            opcode = find_unary_intrinsic_code(intrinsic, node, args->node);
            fn_00486ad0(args->node, requestedReg, result, opcode, intrinsic);
        } break;
        case 0x135:
            generate_unary_vector_intrinsic(intrinsic, node, args->node, requestedReg, result);
            break;
        case 0x136:
            fn_00486db0(intrinsic, node, args->node, requestedReg, result);
            break;
        case 0x34:
        case 0x36: {
            short selected;
            unsigned char *rightExpr;
            unsigned char *leftExpr;
            {
                Operand rightOperand;
                Operand leftOperand;
                UInt16 opcode;
                opcode = find_binary_intrinsic_code(intrinsic, node, args->node, args->next->node);
                leftExpr = (unsigned char *)args->node;
                rightExpr = (unsigned char *)args->next->node;
                memclrw(&leftOperand, sizeof(leftOperand));
                memclrw(&rightOperand, sizeof(rightOperand));

                GenVR(rightExpr, &leftOperand);
                GenVR(leftExpr, &rightOperand);
                selected = requestedReg ? requestedReg : gUsedVirtualRegistersVR++;
                PCodeUtilities_EmitInstruction(opcode, selected, leftOperand.reg, rightOperand.reg),
                    result[0].kind = OpndType_VR, result[0].reg = selected;
            }
        } break;
        case 0x137:
        case 0x138:
            PPCError_EmitClassTypeUpdate(intrinsic_opcodes[intrinsic], args->node, requestedReg, result);
            break;
        default:
            CError_FATAL(6087);
    }
}

void Intrinsics_RegisterIntrinsics(void)
{
    unsigned char saved_cplusplus = copts.cplusplus;
    int i;
    TypeFunc *function;

    copts.cplusplus = 0;
    for (i = 0; i < 313; i++)
        intrinsics[i] = NULL;
    intrinsics[0] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__eieio"), 0, 0);
    intrinsics[1] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__sync"), 0, 0);
    intrinsics[2] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__isync"), 0, 0);
    intrinsics[3] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__abs"), 0, 1, &stsignedint);
    intrinsics[4] = CParser_NewRTFunc((Type *)&stsignedlong, GetHashNameNode("__labs"), 0, 1, &stsignedlong);
    intrinsics[5] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fabs"), 0, 1, &stdouble);
    intrinsics[6] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnabs"), 0, 1, &stdouble);
    intrinsics[7] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__setflm"), 0, 1, &stdouble);
    intrinsics[33] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__frsqrte"), 0, 1, &stdouble);
    intrinsics[8] = CParser_NewRTFunc((Type *)&void_ptr, GetHashNameNode("__alloca"), 0, 1, &stunsignedint);
    intrinsics[9] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__cntlzw"), 0, 1, &stunsignedint);
    intrinsics[10] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__lhbrx"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[11] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__lwbrx"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[12] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__sthbrx"), 0, 3, &stunsignedshort, &void_ptr, &stsignedint);
    intrinsics[13] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__stwbrx"), 0, 3, &stunsignedint, &void_ptr, &stsignedint);
    intrinsics[14] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbf"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[15] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbt"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[16] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbst"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[17] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbtst"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[18] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbz"), 0, 2, &void_ptr, &stsignedint);
    intrinsics[19] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__mulhw"), 0, 2, &stsignedint, &stsignedint);
    intrinsics[20] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__mulhwu"), 0, 2, &stunsignedint, &stunsignedint);
    intrinsics[21] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__divw"), 0, 2, &stsignedint, &stsignedint);
    intrinsics[22] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__divwu"), 0, 2, &stsignedint, &stsignedint);
    intrinsics[23] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fmadd"), 0, 3, &stdouble, &stdouble, &stdouble);
    intrinsics[24] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fmsub"), 0, 3, &stdouble, &stdouble, &stdouble);
    intrinsics[25] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnmadd"), 0, 3, &stdouble, &stdouble, &stdouble);
    intrinsics[26] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnmsub"), 0, 3, &stdouble, &stdouble, &stdouble);
    intrinsics[34] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fsel"), 0, 3, &stdouble, &stdouble, &stdouble);
    intrinsics[27] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fmadds"), 0, 3, &stfloat, &stfloat, &stfloat);
    intrinsics[28] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fmsubs"), 0, 3, &stfloat, &stfloat, &stfloat);
    intrinsics[29] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnmadds"), 0, 3, &stfloat, &stfloat, &stfloat);
    intrinsics[30] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnmsubs"), 0, 3, &stfloat, &stfloat, &stfloat);
    intrinsics[31] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__mffs"), 0, 0);
    intrinsics[32] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fres"), 0, 1, &stfloat);
    intrinsics[40] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fabsf"), 0, 1, &stfloat);
    intrinsics[41] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnabsf"), 0, 1, &stfloat);
    intrinsics[35] =
        CParser_NewRTFunc(TYPE(&stchar_ptr), GetHashNameNode("__strcpy"), 0, 2, TYPE(&stchar_ptr), TYPE(&stchar_ptr));
    function = (TypeFunc *)intrinsics[35]->type;
    function->args->next->qual |= Q_CONST;
    intrinsics[36] =
        CParser_NewRTFunc((Type *)&void_ptr, GetHashNameNode("__memcpy"), 0, 3, &void_ptr, &void_ptr, &stunsignedlong);
    data_00587fc0 = intrinsics[36];
    function = (TypeFunc *)intrinsics[36]->type;
    function->args->next->qual |= Q_CONST;
    intrinsics[37] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwimi"), 0, 5, &stsignedint,
                                       &stsignedint, &stsignedint, &stsignedint, &stsignedint);
    intrinsics[38] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwinm"), 0, 4, &stsignedint,
                                       &stsignedint, &stsignedint, &stsignedint);
    intrinsics[39] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwnm"), 0, 4, &stsignedint,
                                       &stsignedint, &stsignedint, &stsignedint);
    intrinsics[42] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_add"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[43] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_addc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[44] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_adds"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[45] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_and"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[46] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_andc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[47] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_avg"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[48] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ceil"), 0, 1, TYPE(&stvector));
    intrinsics[49] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[50] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpeq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[51] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[52] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmple"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[53] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpgt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[54] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmplt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[55] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ctf"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[56] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cts"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[57] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ctu"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[58] = CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dss"), 0, 1, &stsignedint);
    intrinsics[59] = CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dssall"), 0, 0);
    intrinsics[60] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dst"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    intrinsics[61] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dstst"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    intrinsics[62] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dststt"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    intrinsics[63] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dstt"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    intrinsics[64] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_expte"), 0, 1, TYPE(&stvector));
    intrinsics[65] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_floor"), 0, 1, TYPE(&stvector));
    intrinsics[66] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ld"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[67] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lde"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[68] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ldl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[69] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_loge"), 0, 1, TYPE(&stvector));
    intrinsics[70] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvsl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[71] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvsr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[72] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_madd"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[73] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_madds"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[74] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_max"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[75] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mergeh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[76] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mergel"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[77] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mfvscr"), 0, 0);
    intrinsics[78] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_min"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[79] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mladd"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[80] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mradds"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[81] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_msum"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[82] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_msums"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[83] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mtvscr"), 0, 1, TYPE(&stvector));
    intrinsics[84] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mule"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[85] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mulo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[86] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_nmsub"), 0, 2, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[87] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_nor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[88] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_or"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[89] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_pack"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[90] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packpx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[91] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[92] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packsu"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[93] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_perm"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[94] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_re"), 0, 1, TYPE(&stvector));
    intrinsics[95] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_rl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[96] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_round"), 0, 1, TYPE(&stvector));
    intrinsics[97] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_rsqrte"), 0, 1, TYPE(&stvector));
    intrinsics[98] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sel"), 0, 3, TYPE(&stvector),
                                       TYPE(&stvector), TYPE(&stvector));
    intrinsics[99] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[100] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sld"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[101] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sll"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[102] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_slo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[103] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[104] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s8"), 0, 1, TYPE(&stvector));
    intrinsics[105] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s16"), 0, 1, TYPE(&stvector));
    intrinsics[106] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s32"), 0, 1, TYPE(&stvector));
    intrinsics[107] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u8"), 0, 1, TYPE(&stvector));
    intrinsics[108] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u16"), 0, 1, TYPE(&stvector));
    intrinsics[109] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u32"), 0, 1, TYPE(&stvector));
    intrinsics[110] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[111] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sra"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[112] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_srl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[113] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sro"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[114] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_st"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[115] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ste"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[116] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stl"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[117] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[118] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_subc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[119] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_subs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[120] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sum4s"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[121] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sum2s"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[122] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sums"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[123] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_trunc"), 0, 1, TYPE(&stvector));
    intrinsics[124] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2sh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[125] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2sl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[126] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2uh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[127] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2ul"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[128] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpackh"), 0, 1, TYPE(&stvector));
    intrinsics[129] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpackl"), 0, 1, TYPE(&stvector));
    intrinsics[130] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_xor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[131] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_eq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[132] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[133] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_gt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[134] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_in"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[135] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_le"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[136] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_lt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[137] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nan"), 0, 1, TYPE(&stvector));
    intrinsics[138] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ne"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[139] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[140] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ngt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[141] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nle"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[142] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nlt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[143] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_numeric"), 0, 1, TYPE(&stvector));
    intrinsics[144] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_eq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[145] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[146] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_gt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[147] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_le"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[148] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_lt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[149] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nan"), 0, 1, TYPE(&stvector));
    intrinsics[150] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ne"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[151] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[152] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ngt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[153] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nle"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[154] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nlt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[155] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_numeric"), 0, 1, TYPE(&stvector));
    intrinsics[156] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_out"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[157] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddubm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[158] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduhm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[159] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduwm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[160] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[161] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddcuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[162] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddubs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[163] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddsbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[164] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduhs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[165] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddshs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[166] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[167] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[168] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vand"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[169] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vandc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[170] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[171] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[172] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavguh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[173] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[174] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavguw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[175] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[176] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfip"), 0, 1, TYPE(&stvector));
    intrinsics[177] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpbfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[178] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[179] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[180] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[181] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpeqfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[182] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgefp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[183] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[184] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[185] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[186] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[187] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[188] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[189] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[190] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcfux"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[191] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcfsx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[192] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vctsxs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[193] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vctuxs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[194] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vexptefp"), 0, 1, TYPE(&stvector));
    intrinsics[195] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfim"), 0, 1, TYPE(&stvector));
    intrinsics[196] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[197] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvebx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[198] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvehx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[199] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvewx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[200] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvxl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[201] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vlogefp"), 0, 1, TYPE(&stvector));
    intrinsics[202] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaddfp"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[203] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmhaddshs"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[204] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[205] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[206] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[207] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[208] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[209] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[210] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[211] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[212] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[213] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[214] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[215] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[216] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[217] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[218] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[219] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[220] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[221] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[222] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[223] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[224] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmladduhm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[225] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmhraddshs"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[226] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumubm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[227] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumuhm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[228] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsummbm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[229] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumshm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[230] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumuhs"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[231] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumshs"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[232] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuleub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[233] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulesb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[234] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuleuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[235] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulesh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[236] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuloub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[237] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulosb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[238] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulouh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[239] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulosh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[240] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vnmsubfp"), 0, 2, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[241] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vnor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[242] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[243] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuhum"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[244] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuwum"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[245] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkpx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[246] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuhus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[247] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkshss"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[248] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuwus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[249] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkswss"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[250] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkshus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[251] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkswus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[252] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vperm"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[253] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrefp"), 0, 1, TYPE(&stvector));
    intrinsics[254] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[255] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[256] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[257] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfin"), 0, 1, TYPE(&stvector));
    intrinsics[258] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrsqrtefp"), 0, 1, TYPE(&stvector));
    intrinsics[259] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsel"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[260] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[261] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[262] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[263] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsldoi"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[264] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[265] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[266] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[267] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsplth"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[268] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[269] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltisb"), 0, 1, TYPE(&stvector));
    intrinsics[270] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltish"), 0, 1, TYPE(&stvector));
    intrinsics[271] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltisw"), 0, 1, TYPE(&stvector));
    intrinsics[272] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[273] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[274] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[275] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrab"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[276] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrah"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[277] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsraw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[278] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[279] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsro"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[280] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvx"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[281] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvebx"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[282] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvehx"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[283] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvewx"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[284] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvxl"), 0, 3, TYPE(&stvector),
                                        TYPE(&stvector), TYPE(&stvector));
    intrinsics[285] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsububm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[286] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuhm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[287] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuwm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[288] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[289] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubcuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[290] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsububs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[291] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubsbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[292] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuhs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[293] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubshs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[294] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[295] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[296] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4ubs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[297] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4sbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[298] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4shs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[299] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum2sws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[300] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsumsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[301] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfiz"), 0, 1, TYPE(&stvector));
    intrinsics[302] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhsb"), 0, 1, TYPE(&stvector));
    intrinsics[303] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklsb"), 0, 1, TYPE(&stvector));
    intrinsics[304] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhpx"), 0, 1, TYPE(&stvector));
    intrinsics[305] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklpx"), 0, 1, TYPE(&stvector));
    intrinsics[306] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhsh"), 0, 1, TYPE(&stvector));
    intrinsics[307] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklsh"), 0, 1, TYPE(&stvector));
    intrinsics[308] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vxor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    intrinsics[309] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_abs"), 0, 1, TYPE(&stvector));
    intrinsics[310] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_abss"), 0, 1, TYPE(&stvector));
    intrinsics[311] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__va_setup"), 0, 1, &void_ptr);
    intrinsics[312] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__builtin_va_info"), 0, 1, &void_ptr);
    for (i = 0; i < 313; i++) {
        if (!intrinsics[i])
            CError_FATAL(6535);
        intrinsics[i]->u.data.u.intconst.hi = i;
        function = (TypeFunc *)intrinsics[i]->type;
        function->flags |= FUNC_INTRINSIC;
        CScope_AddGlobalObject(intrinsics[i]);
    }
    copts.cplusplus = saved_cplusplus;
}

unsigned char Intrinsics_InitRegistrations(unsigned char active)
{
    if (active) {
        if (!(intrinsics[0] = registration_find("__eieio")))
            return 0;
        if (!(intrinsics[1] = registration_find("__sync")))
            return 0;
        if (!(intrinsics[2] = registration_find("__isync")))
            return 0;
        if (!(intrinsics[3] = registration_find("__abs")))
            return 0;
        if (!(intrinsics[4] = registration_find("__labs")))
            return 0;
        if (!(intrinsics[5] = registration_find("__fabs")))
            return 0;
        if (!(intrinsics[6] = registration_find("__fnabs")))
            return 0;
        if (!(intrinsics[7] = registration_find("__setflm")))
            return 0;
        if (!(intrinsics[33] = registration_find("__frsqrte")))
            return 0;
        if (!(intrinsics[8] = registration_find("__alloca")))
            return 0;
        if (!(intrinsics[9] = registration_find("__cntlzw")))
            return 0;
        if (!(intrinsics[10] = registration_find("__lhbrx")))
            return 0;
        if (!(intrinsics[11] = registration_find("__lwbrx")))
            return 0;
        if (!(intrinsics[12] = registration_find("__sthbrx")))
            return 0;
        if (!(intrinsics[13] = registration_find("__stwbrx")))
            return 0;
        if (!(intrinsics[14] = registration_find("__dcbf")))
            return 0;
        if (!(intrinsics[15] = registration_find("__dcbt")))
            return 0;
        if (!(intrinsics[16] = registration_find("__dcbst")))
            return 0;
        if (!(intrinsics[17] = registration_find("__dcbtst")))
            return 0;
        if (!(intrinsics[18] = registration_find("__dcbz")))
            return 0;
        if (!(intrinsics[19] = registration_find("__mulhw")))
            return 0;
        if (!(intrinsics[20] = registration_find("__mulhwu")))
            return 0;
        if (!(intrinsics[21] = registration_find("__divw")))
            return 0;
        if (!(intrinsics[22] = registration_find("__divwu")))
            return 0;
        if (!(intrinsics[23] = registration_find("__fmadd")))
            return 0;
        if (!(intrinsics[24] = registration_find("__fmsub")))
            return 0;
        if (!(intrinsics[25] = registration_find("__fnmadd")))
            return 0;
        if (!(intrinsics[26] = registration_find("__fnmsub")))
            return 0;
        if (!(intrinsics[34] = registration_find("__fsel")))
            return 0;
        if (!(intrinsics[27] = registration_find("__fmadds")))
            return 0;
        if (!(intrinsics[28] = registration_find("__fmsubs")))
            return 0;
        if (!(intrinsics[29] = registration_find("__fnmadds")))
            return 0;
        if (!(intrinsics[30] = registration_find("__fnmsubs")))
            return 0;
        if (!(intrinsics[31] = registration_find("__mffs")))
            return 0;
        if (!(intrinsics[32] = registration_find("__fres")))
            return 0;
        if (!(intrinsics[40] = registration_find("__fabsf")))
            return 0;
        if (!(intrinsics[41] = registration_find("__fnabsf")))
            return 0;
        if (!(intrinsics[35] = registration_find("__strcpy")))
            return 0;
        if (!(intrinsics[37] = registration_find("__rlwimi")))
            return 0;
        if (!(intrinsics[38] = registration_find("__rlwinm")))
            return 0;
        if (!(intrinsics[39] = registration_find("__rlwnm")))
            return 0;
        if (!(intrinsics[42] = registration_find("vec_add")))
            return 0;
        if (!(intrinsics[43] = registration_find("vec_addc")))
            return 0;
        if (!(intrinsics[44] = registration_find("vec_adds")))
            return 0;
        if (!(intrinsics[45] = registration_find("vec_and")))
            return 0;
        if (!(intrinsics[46] = registration_find("vec_andc")))
            return 0;
        if (!(intrinsics[47] = registration_find("vec_avg")))
            return 0;
        if (!(intrinsics[48] = registration_find("vec_ceil")))
            return 0;
        if (!(intrinsics[49] = registration_find("vec_cmpb")))
            return 0;
        if (!(intrinsics[50] = registration_find("vec_cmpeq")))
            return 0;
        if (!(intrinsics[51] = registration_find("vec_cmpge")))
            return 0;
        if (!(intrinsics[52] = registration_find("vec_cmple")))
            return 0;
        if (!(intrinsics[53] = registration_find("vec_cmpgt")))
            return 0;
        if (!(intrinsics[54] = registration_find("vec_cmplt")))
            return 0;
        if (!(intrinsics[55] = registration_find("vec_ctf")))
            return 0;
        if (!(intrinsics[56] = registration_find("vec_cts")))
            return 0;
        if (!(intrinsics[57] = registration_find("vec_ctu")))
            return 0;
        if (!(intrinsics[64] = registration_find("vec_expte")))
            return 0;
        if (!(intrinsics[65] = registration_find("vec_floor")))
            return 0;
        if (!(intrinsics[66] = registration_find("vec_ld")))
            return 0;
        if (!(intrinsics[67] = registration_find("vec_lde")))
            return 0;
        if (!(intrinsics[68] = registration_find("vec_ldl")))
            return 0;
        if (!(intrinsics[69] = registration_find("vec_loge")))
            return 0;
        if (!(intrinsics[70] = registration_find("vec_lvsl")))
            return 0;
        if (!(intrinsics[71] = registration_find("vec_lvsr")))
            return 0;
        if (!(intrinsics[72] = registration_find("vec_madd")))
            return 0;
        if (!(intrinsics[73] = registration_find("vec_madds")))
            return 0;
        if (!(intrinsics[74] = registration_find("vec_max")))
            return 0;
        if (!(intrinsics[75] = registration_find("vec_mergeh")))
            return 0;
        if (!(intrinsics[76] = registration_find("vec_mergel")))
            return 0;
        if (!(intrinsics[77] = registration_find("vec_mfvscr")))
            return 0;
        if (!(intrinsics[78] = registration_find("vec_min")))
            return 0;
        if (!(intrinsics[79] = registration_find("vec_mladd")))
            return 0;
        if (!(intrinsics[80] = registration_find("vec_mradds")))
            return 0;
        if (!(intrinsics[81] = registration_find("vec_msum")))
            return 0;
        if (!(intrinsics[82] = registration_find("vec_msums")))
            return 0;
        if (!(intrinsics[83] = registration_find("vec_mtvscr")))
            return 0;
        if (!(intrinsics[84] = registration_find("vec_mule")))
            return 0;
        if (!(intrinsics[85] = registration_find("vec_mulo")))
            return 0;
        if (!(intrinsics[86] = registration_find("vec_nmsub")))
            return 0;
        if (!(intrinsics[87] = registration_find("vec_nor")))
            return 0;
        if (!(intrinsics[88] = registration_find("vec_or")))
            return 0;
        if (!(intrinsics[89] = registration_find("vec_pack")))
            return 0;
        if (!(intrinsics[90] = registration_find("vec_packpx")))
            return 0;
        if (!(intrinsics[91] = registration_find("vec_packs")))
            return 0;
        if (!(intrinsics[92] = registration_find("vec_packsu")))
            return 0;
        if (!(intrinsics[93] = registration_find("vec_perm")))
            return 0;
        if (!(intrinsics[94] = registration_find("vec_re")))
            return 0;
        if (!(intrinsics[95] = registration_find("vec_rl")))
            return 0;
        if (!(intrinsics[96] = registration_find("vec_round")))
            return 0;
        if (!(intrinsics[97] = registration_find("vec_rsqrte")))
            return 0;
        if (!(intrinsics[98] = registration_find("vec_sel")))
            return 0;
        if (!(intrinsics[99] = registration_find("vec_sl")))
            return 0;
        if (!(intrinsics[100] = registration_find("vec_sld")))
            return 0;
        if (!(intrinsics[101] = registration_find("vec_sll")))
            return 0;
        if (!(intrinsics[102] = registration_find("vec_slo")))
            return 0;
        if (!(intrinsics[103] = registration_find("vec_splat")))
            return 0;
        if (!(intrinsics[104] = registration_find("vec_splat_s8")))
            return 0;
        if (!(intrinsics[105] = registration_find("vec_splat_s16")))
            return 0;
        if (!(intrinsics[106] = registration_find("vec_splat_s32")))
            return 0;
        if (!(intrinsics[107] = registration_find("vec_splat_u8")))
            return 0;
        if (!(intrinsics[108] = registration_find("vec_splat_u16")))
            return 0;
        if (!(intrinsics[109] = registration_find("vec_splat_u32")))
            return 0;
        if (!(intrinsics[110] = registration_find("vec_sr")))
            return 0;
        if (!(intrinsics[111] = registration_find("vec_sra")))
            return 0;
        if (!(intrinsics[112] = registration_find("vec_srl")))
            return 0;
        if (!(intrinsics[113] = registration_find("vec_sro")))
            return 0;
        if (!(intrinsics[114] = registration_find("vec_st")))
            return 0;
        if (!(intrinsics[115] = registration_find("vec_ste")))
            return 0;
        if (!(intrinsics[116] = registration_find("vec_stl")))
            return 0;
        if (!(intrinsics[117] = registration_find("vec_sub")))
            return 0;
        if (!(intrinsics[118] = registration_find("vec_subc")))
            return 0;
        if (!(intrinsics[119] = registration_find("vec_subs")))
            return 0;
        if (!(intrinsics[120] = registration_find("vec_sum4s")))
            return 0;
        if (!(intrinsics[121] = registration_find("vec_sum2s")))
            return 0;
        if (!(intrinsics[122] = registration_find("vec_sums")))
            return 0;
        if (!(intrinsics[123] = registration_find("vec_trunc")))
            return 0;
        if (!(intrinsics[124] = registration_find("vec_unpack2sh")))
            return 0;
        if (!(intrinsics[125] = registration_find("vec_unpack2sl")))
            return 0;
        if (!(intrinsics[126] = registration_find("vec_unpack2uh")))
            return 0;
        if (!(intrinsics[127] = registration_find("vec_unpack2ul")))
            return 0;
        if (!(intrinsics[128] = registration_find("vec_unpackh")))
            return 0;
        if (!(intrinsics[129] = registration_find("vec_unpackl")))
            return 0;
        if (!(intrinsics[130] = registration_find("vec_xor")))
            return 0;
        if (!(intrinsics[131] = registration_find("vec_all_eq")))
            return 0;
        if (!(intrinsics[132] = registration_find("vec_all_ge")))
            return 0;
        if (!(intrinsics[133] = registration_find("vec_all_gt")))
            return 0;
        if (!(intrinsics[134] = registration_find("vec_all_in")))
            return 0;
        if (!(intrinsics[135] = registration_find("vec_all_le")))
            return 0;
        if (!(intrinsics[136] = registration_find("vec_all_lt")))
            return 0;
        if (!(intrinsics[137] = registration_find("vec_all_nan")))
            return 0;
        if (!(intrinsics[138] = registration_find("vec_all_ne")))
            return 0;
        if (!(intrinsics[139] = registration_find("vec_all_nge")))
            return 0;
        if (!(intrinsics[140] = registration_find("vec_all_ngt")))
            return 0;
        if (!(intrinsics[141] = registration_find("vec_all_nle")))
            return 0;
        if (!(intrinsics[142] = registration_find("vec_all_nlt")))
            return 0;
        if (!(intrinsics[143] = registration_find("vec_all_numeric")))
            return 0;
        if (!(intrinsics[144] = registration_find("vec_any_eq")))
            return 0;
        if (!(intrinsics[145] = registration_find("vec_any_ge")))
            return 0;
        if (!(intrinsics[146] = registration_find("vec_any_gt")))
            return 0;
        if (!(intrinsics[147] = registration_find("vec_any_le")))
            return 0;
        if (!(intrinsics[148] = registration_find("vec_any_lt")))
            return 0;
        if (!(intrinsics[149] = registration_find("vec_any_nan")))
            return 0;
        if (!(intrinsics[150] = registration_find("vec_any_ne")))
            return 0;
        if (!(intrinsics[151] = registration_find("vec_any_nge")))
            return 0;
        if (!(intrinsics[152] = registration_find("vec_any_ngt")))
            return 0;
        if (!(intrinsics[153] = registration_find("vec_any_nle")))
            return 0;
        if (!(intrinsics[154] = registration_find("vec_any_nlt")))
            return 0;
        if (!(intrinsics[155] = registration_find("vec_any_numeric")))
            return 0;
        if (!(intrinsics[156] = registration_find("vec_any_out")))
            return 0;
        if (!(intrinsics[157] = registration_find("vec_vaddubm")))
            return 0;
        if (!(intrinsics[158] = registration_find("vec_vadduhm")))
            return 0;
        if (!(intrinsics[159] = registration_find("vec_vadduwm")))
            return 0;
        if (!(intrinsics[160] = registration_find("vec_vaddfp")))
            return 0;
        if (!(intrinsics[161] = registration_find("vec_vaddcuw")))
            return 0;
        if (!(intrinsics[162] = registration_find("vec_vaddubs")))
            return 0;
        if (!(intrinsics[163] = registration_find("vec_vaddubs")))
            return 0;
        if (!(intrinsics[164] = registration_find("vec_vadduhs")))
            return 0;
        if (!(intrinsics[165] = registration_find("vec_vadduhs")))
            return 0;
        if (!(intrinsics[166] = registration_find("vec_vadduws")))
            return 0;
        if (!(intrinsics[167] = registration_find("vec_vadduws")))
            return 0;
        if (!(intrinsics[168] = registration_find("vec_vand")))
            return 0;
        if (!(intrinsics[169] = registration_find("vec_vandc")))
            return 0;
        if (!(intrinsics[170] = registration_find("vec_vavgub")))
            return 0;
        if (!(intrinsics[171] = registration_find("vec_vavgsb")))
            return 0;
        if (!(intrinsics[172] = registration_find("vec_vavguh")))
            return 0;
        if (!(intrinsics[173] = registration_find("vec_vavgsh")))
            return 0;
        if (!(intrinsics[174] = registration_find("vec_vavguw")))
            return 0;
        if (!(intrinsics[175] = registration_find("vec_vavgsw")))
            return 0;
        if (!(intrinsics[176] = registration_find("vec_vrfip")))
            return 0;
        if (!(intrinsics[177] = registration_find("vec_vcmpbfp")))
            return 0;
        if (!(intrinsics[178] = registration_find("vec_vcmpequb")))
            return 0;
        if (!(intrinsics[179] = registration_find("vec_vcmpequh")))
            return 0;
        if (!(intrinsics[180] = registration_find("vec_vcmpequw")))
            return 0;
        if (!(intrinsics[181] = registration_find("vec_vcmpeqfp")))
            return 0;
        if (!(intrinsics[182] = registration_find("vec_vcmpgefp")))
            return 0;
        if (!(intrinsics[183] = registration_find("vec_vcmpgtub")))
            return 0;
        if (!(intrinsics[184] = registration_find("vec_vcmpgtsb")))
            return 0;
        if (!(intrinsics[185] = registration_find("vec_vcmpgtuh")))
            return 0;
        if (!(intrinsics[186] = registration_find("vec_vcmpgtsh")))
            return 0;
        if (!(intrinsics[187] = registration_find("vec_vcmpgtuw")))
            return 0;
        if (!(intrinsics[188] = registration_find("vec_vcmpgtsw")))
            return 0;
        if (!(intrinsics[189] = registration_find("vec_vcmpgtfp")))
            return 0;
        if (!(intrinsics[190] = registration_find("vec_vcfux")))
            return 0;
        if (!(intrinsics[191] = registration_find("vec_vcfsx")))
            return 0;
        if (!(intrinsics[192] = registration_find("vec_vctsxs")))
            return 0;
        if (!(intrinsics[193] = registration_find("vec_vctuxs")))
            return 0;
        if (!(intrinsics[194] = registration_find("vec_vexptefp")))
            return 0;
        if (!(intrinsics[195] = registration_find("vec_vrfim")))
            return 0;
        if (!(intrinsics[196] = registration_find("vec_lvx")))
            return 0;
        if (!(intrinsics[197] = registration_find("vec_lvebx")))
            return 0;
        if (!(intrinsics[198] = registration_find("vec_lvehx")))
            return 0;
        if (!(intrinsics[199] = registration_find("vec_lvewx")))
            return 0;
        if (!(intrinsics[200] = registration_find("vec_lvxl")))
            return 0;
        if (!(intrinsics[201] = registration_find("vec_vlogefp")))
            return 0;
        if (!(intrinsics[202] = registration_find("vec_vmaddfp")))
            return 0;
        if (!(intrinsics[203] = registration_find("vec_vmhaddshs")))
            return 0;
        if (!(intrinsics[204] = registration_find("vec_vmaxub")))
            return 0;
        if (!(intrinsics[205] = registration_find("vec_vmaxsb")))
            return 0;
        if (!(intrinsics[206] = registration_find("vec_vmaxuh")))
            return 0;
        if (!(intrinsics[207] = registration_find("vec_vmaxsh")))
            return 0;
        if (!(intrinsics[208] = registration_find("vec_vmaxuw")))
            return 0;
        if (!(intrinsics[209] = registration_find("vec_vmaxsw")))
            return 0;
        if (!(intrinsics[210] = registration_find("vec_vmaxfp")))
            return 0;
        if (!(intrinsics[211] = registration_find("vec_vmrghb")))
            return 0;
        if (!(intrinsics[212] = registration_find("vec_vmrghh")))
            return 0;
        if (!(intrinsics[213] = registration_find("vec_vmrghw")))
            return 0;
        if (!(intrinsics[214] = registration_find("vec_vmrglb")))
            return 0;
        if (!(intrinsics[215] = registration_find("vec_vmrglh")))
            return 0;
        if (!(intrinsics[216] = registration_find("vec_vmrglw")))
            return 0;
        if (!(intrinsics[204] = registration_find("vec_vminub")))
            return 0;
        if (!(intrinsics[205] = registration_find("vec_vminsb")))
            return 0;
        if (!(intrinsics[206] = registration_find("vec_vminuh")))
            return 0;
        if (!(intrinsics[207] = registration_find("vec_vminsh")))
            return 0;
        if (!(intrinsics[208] = registration_find("vec_vminuw")))
            return 0;
        if (!(intrinsics[209] = registration_find("vec_vminsw")))
            return 0;
        if (!(intrinsics[210] = registration_find("vec_vminfp")))
            return 0;
        if (!(intrinsics[224] = registration_find("vec_vmladduhm")))
            return 0;
        if (!(intrinsics[225] = registration_find("vec_vmhraddshs")))
            return 0;
        if (!(intrinsics[226] = registration_find("vec_vmsumubm")))
            return 0;
        if (!(intrinsics[227] = registration_find("vec_vmsumuhm")))
            return 0;
        if (!(intrinsics[228] = registration_find("vec_vmsummbm")))
            return 0;
        if (!(intrinsics[229] = registration_find("vec_vmsumshm")))
            return 0;
        if (!(intrinsics[230] = registration_find("vec_vmsumuhs")))
            return 0;
        if (!(intrinsics[231] = registration_find("vec_vmsumshs")))
            return 0;
        if (!(intrinsics[232] = registration_find("vec_vmuleub")))
            return 0;
        if (!(intrinsics[233] = registration_find("vec_vmulesb")))
            return 0;
        if (!(intrinsics[234] = registration_find("vec_vmuleuh")))
            return 0;
        if (!(intrinsics[235] = registration_find("vec_vmulesh")))
            return 0;
        if (!(intrinsics[236] = registration_find("vec_vmuloub")))
            return 0;
        if (!(intrinsics[237] = registration_find("vec_vmulosb")))
            return 0;
        if (!(intrinsics[238] = registration_find("vec_vmulouh")))
            return 0;
        if (!(intrinsics[239] = registration_find("vec_vmulosh")))
            return 0;
        if (!(intrinsics[240] = registration_find("vec_vnmsubfp")))
            return 0;
        if (!(intrinsics[241] = registration_find("vec_vnor")))
            return 0;
        if (!(intrinsics[242] = registration_find("vec_vor")))
            return 0;
        if (!(intrinsics[243] = registration_find("vec_vpkuhum")))
            return 0;
        if (!(intrinsics[244] = registration_find("vec_vpkuwum")))
            return 0;
        if (!(intrinsics[245] = registration_find("vec_vpkpx")))
            return 0;
        if (!(intrinsics[246] = registration_find("vec_vpkuhus")))
            return 0;
        if (!(intrinsics[247] = registration_find("vec_vpkshss")))
            return 0;
        if (!(intrinsics[248] = registration_find("vec_vpkuwus")))
            return 0;
        if (!(intrinsics[249] = registration_find("vec_vpkswss")))
            return 0;
        if (!(intrinsics[250] = registration_find("vec_vpkshus")))
            return 0;
        if (!(intrinsics[251] = registration_find("vec_vpkswus")))
            return 0;
        if (!(intrinsics[252] = registration_find("vec_vperm")))
            return 0;
        if (!(intrinsics[253] = registration_find("vec_vrefp")))
            return 0;
        if (!(intrinsics[254] = registration_find("vec_vrlb")))
            return 0;
        if (!(intrinsics[255] = registration_find("vec_vrlh")))
            return 0;
        if (!(intrinsics[256] = registration_find("vec_vrlw")))
            return 0;
        if (!(intrinsics[257] = registration_find("vec_vrfin")))
            return 0;
        if (!(intrinsics[258] = registration_find("vec_vrsqrtefp")))
            return 0;
        if (!(intrinsics[259] = registration_find("vec_vsel")))
            return 0;
        if (!(intrinsics[260] = registration_find("vec_vslb")))
            return 0;
        if (!(intrinsics[261] = registration_find("vec_vslh")))
            return 0;
        if (!(intrinsics[262] = registration_find("vec_vslw")))
            return 0;
        if (!(intrinsics[263] = registration_find("vec_vsldoi")))
            return 0;
        if (!(intrinsics[264] = registration_find("vec_vsl")))
            return 0;
        if (!(intrinsics[265] = registration_find("vec_vslo")))
            return 0;
        if (!(intrinsics[266] = registration_find("vec_vspltb")))
            return 0;
        if (!(intrinsics[267] = registration_find("vec_vsplth")))
            return 0;
        if (!(intrinsics[268] = registration_find("vec_vspltw")))
            return 0;
        if (!(intrinsics[269] = registration_find("vec_vspltisb")))
            return 0;
        if (!(intrinsics[270] = registration_find("vec_vspltish")))
            return 0;
        if (!(intrinsics[271] = registration_find("vec_vspltisw")))
            return 0;
        if (!(intrinsics[272] = registration_find("vec_vsrb")))
            return 0;
        if (!(intrinsics[273] = registration_find("vec_vsrh")))
            return 0;
        if (!(intrinsics[274] = registration_find("vec_vsrw")))
            return 0;
        if (!(intrinsics[275] = registration_find("vec_vsrab")))
            return 0;
        if (!(intrinsics[276] = registration_find("vec_vsrah")))
            return 0;
        if (!(intrinsics[277] = registration_find("vec_vsraw")))
            return 0;
        if (!(intrinsics[278] = registration_find("vec_vsr")))
            return 0;
        if (!(intrinsics[279] = registration_find("vec_vsro")))
            return 0;
        if (!(intrinsics[280] = registration_find("vec_stvx")))
            return 0;
        if (!(intrinsics[281] = registration_find("vec_stvebx")))
            return 0;
        if (!(intrinsics[282] = registration_find("vec_stvehx")))
            return 0;
        if (!(intrinsics[283] = registration_find("vec_stvewx")))
            return 0;
        if (!(intrinsics[284] = registration_find("vec_stvxl")))
            return 0;
        if (!(intrinsics[285] = registration_find("vec_vsububm")))
            return 0;
        if (!(intrinsics[286] = registration_find("vec_vsubuhm")))
            return 0;
        if (!(intrinsics[287] = registration_find("vec_vsubuwm")))
            return 0;
        if (!(intrinsics[288] = registration_find("vec_vsubfp")))
            return 0;
        if (!(intrinsics[289] = registration_find("vec_vsubcuw")))
            return 0;
        if (!(intrinsics[290] = registration_find("vec_vsububs")))
            return 0;
        if (!(intrinsics[291] = registration_find("vec_vsubsbs")))
            return 0;
        if (!(intrinsics[292] = registration_find("vec_vsubuhs")))
            return 0;
        if (!(intrinsics[293] = registration_find("vec_vsubshs")))
            return 0;
        if (!(intrinsics[294] = registration_find("vec_vsubuws")))
            return 0;
        if (!(intrinsics[295] = registration_find("vec_vsubsws")))
            return 0;
        if (!(intrinsics[296] = registration_find("vec_vsum4ubs")))
            return 0;
        if (!(intrinsics[297] = registration_find("vec_vsum4sbs")))
            return 0;
        if (!(intrinsics[298] = registration_find("vec_vsum4shs")))
            return 0;
        if (!(intrinsics[299] = registration_find("vec_vsum2sws")))
            return 0;
        if (!(intrinsics[300] = registration_find("vec_vsumsws")))
            return 0;
        if (!(intrinsics[301] = registration_find("vec_vrfiz")))
            return 0;
        if (!(intrinsics[302] = registration_find("vec_vupkhsb")))
            return 0;
        if (!(intrinsics[303] = registration_find("vec_vupklsb")))
            return 0;
        if (!(intrinsics[304] = registration_find("vec_vupkhpx")))
            return 0;
        if (!(intrinsics[305] = registration_find("vec_vupklpx")))
            return 0;
        if (!(intrinsics[306] = registration_find("vec_vupkhsh")))
            return 0;
        if (!(intrinsics[307] = registration_find("vec_vupklsh")))
            return 0;
        if (!(intrinsics[308] = registration_find("vec_vxor")))
            return 0;
        if (!(intrinsics[309] = registration_find("vec_abs")))
            return 0;
        if (!(intrinsics[310] = registration_find("vec_abss")))
            return 0;
        if (!(intrinsics[311] = registration_find("__va_setup")))
            return 0;
        if (!(intrinsics[312] = registration_find("__builtin_va_info")))
            return 0;
    }
    return 1;
}

static void *registration_find(const char *name)
{
    NameSpaceName *r = CScope_FindNameSpaceName(cscope_root, GetHashNameNode(name));
    void *object = NULL;
    if (r && (object = ((NameSpaceName *)r)[0].first.object) && !((NameSpaceName *)r)[0].first.next)
        return object;
    return NULL;
}

static void GenVR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\x06')
        Operands_ForceVR(op, ((ENode *)node)->rtype, 0);
}

static void GenFPR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\x05')
        Operands_ForceFPR(op, ((ENode *)node)->rtype, 0);
}

static void GenGPR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\0')
        Operands_ForceGPR(op, ((ENode *)node)->rtype, 0);
}

char Intrinsics_IsRegisteredObject(ObjBase *object)
{
    int i;
    for (i = 0; i < 313; i++) {
        if (object == (ObjBase *)intrinsics[i])
            return 1;
    }
    return 0;
}
