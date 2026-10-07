#define CERROR_FILE "Intrinsics.c"
#include "compiler/common.h"
#include "compiler/Intrinsics.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/FunctionCalls.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/StructMoves.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/ENode.h"

#define false 0
#define true 1

typedef void (*GenNodeProc)(ENode *node, SInt32 a, SInt32 b, Operand *op);

typedef void (*PCodeFn)(void *, int, int, void *);
char Intrinsics_IsRegisteredObject(ObjBase *object)
{
    int i;
    for (i = 0; i < 313; i++) {
        if (object == (ObjBase *)registration_slot_00580898[i])
            return 1;
    }
    return 0;
}

static void GenGPR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\0')
        Operands_ForceGPR(op, ((ENode *)node)->rtype, 0);
}

static void GenFPR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\x05')
        Operands_ForceFPR(op, ((ENode *)node)->rtype, 0);
}

static void GenVR(UInt8 *node, Operand *op)
{
    data_00560648[*node](node, 0, 0, op);
    if (op->kind != '\x06')
        Operands_ForceVR(op, ((ENode *)node)->rtype, 0);
}

static void *registration_find(const char *name)
{
    NameSpaceName *r = CScope_FindNameSpaceName(registration_context, GetHashNameNode(name));
    void *object = NULL;
    if (r && (object = ((NameSpaceName *)r)[0].first.object) && !((NameSpaceName *)r)[0].first.next)
        return object;
    return NULL;
}

unsigned char Intrinsics_InitRegistrations(unsigned char active)
{
    if (active) {
        if (!(registration_slot_00580898[0] = registration_find("__eieio")))
            return 0;
        if (!(registration_slot_00580898[1] = registration_find("__sync")))
            return 0;
        if (!(registration_slot_00580898[2] = registration_find("__isync")))
            return 0;
        if (!(registration_slot_00580898[3] = registration_find("__abs")))
            return 0;
        if (!(registration_slot_00580898[4] = registration_find("__labs")))
            return 0;
        if (!(registration_slot_00580898[5] = registration_find("__fabs")))
            return 0;
        if (!(registration_slot_00580898[6] = registration_find("__fnabs")))
            return 0;
        if (!(registration_slot_00580898[7] = registration_find("__setflm")))
            return 0;
        if (!(registration_slot_0058091c = registration_find("__frsqrte")))
            return 0;
        if (!(registration_slot_005808b8 = registration_find("__alloca")))
            return 0;
        if (!(registration_slot_005808bc = registration_find("__cntlzw")))
            return 0;
        if (!(registration_slot_005808c0 = registration_find("__lhbrx")))
            return 0;
        if (!(registration_slot_005808c4 = registration_find("__lwbrx")))
            return 0;
        if (!(registration_slot_005808c8 = registration_find("__sthbrx")))
            return 0;
        if (!(registration_slot_005808cc = registration_find("__stwbrx")))
            return 0;
        if (!(registration_slot_005808d0 = registration_find("__dcbf")))
            return 0;
        if (!(registration_slot_005808d4 = registration_find("__dcbt")))
            return 0;
        if (!(registration_slot_005808d8 = registration_find("__dcbst")))
            return 0;
        if (!(registration_slot_005808dc = registration_find("__dcbtst")))
            return 0;
        if (!(registration_slot_005808e0 = registration_find("__dcbz")))
            return 0;
        if (!(registration_slot_005808e4 = registration_find("__mulhw")))
            return 0;
        if (!(registration_slot_005808e8 = registration_find("__mulhwu")))
            return 0;
        if (!(registration_slot_005808ec = registration_find("__divw")))
            return 0;
        if (!(registration_slot_005808f0 = registration_find("__divwu")))
            return 0;
        if (!(registration_slot_005808f4 = registration_find("__fmadd")))
            return 0;
        if (!(registration_slot_005808f8 = registration_find("__fmsub")))
            return 0;
        if (!(registration_slot_005808fc = registration_find("__fnmadd")))
            return 0;
        if (!(registration_slot_00580900 = registration_find("__fnmsub")))
            return 0;
        if (!(registration_slot_00580920 = registration_find("__fsel")))
            return 0;
        if (!(registration_slot_00580904 = registration_find("__fmadds")))
            return 0;
        if (!(registration_slot_00580908 = registration_find("__fmsubs")))
            return 0;
        if (!(registration_slot_0058090c = registration_find("__fnmadds")))
            return 0;
        if (!(registration_slot_00580910 = registration_find("__fnmsubs")))
            return 0;
        if (!(registration_slot_00580914 = registration_find("__mffs")))
            return 0;
        if (!(registration_slot_00580918 = registration_find("__fres")))
            return 0;
        if (!(registration_slot_00580938 = registration_find("__fabsf")))
            return 0;
        if (!(registration_slot_0058093c = registration_find("__fnabsf")))
            return 0;
        if (!(registration_slot_00580898[35] = registration_find("__strcpy")))
            return 0;
        if (!(registration_slot_0058092c = registration_find("__rlwimi")))
            return 0;
        if (!(registration_slot_00580930 = registration_find("__rlwinm")))
            return 0;
        if (!(registration_slot_00580934 = registration_find("__rlwnm")))
            return 0;
        if (!(registration_slot_00580940 = registration_find("vec_add")))
            return 0;
        if (!(registration_slot_00580944 = registration_find("vec_addc")))
            return 0;
        if (!(registration_slot_00580948 = registration_find("vec_adds")))
            return 0;
        if (!(registration_slot_0058094c = registration_find("vec_and")))
            return 0;
        if (!(registration_slot_00580950 = registration_find("vec_andc")))
            return 0;
        if (!(registration_slot_00580954 = registration_find("vec_avg")))
            return 0;
        if (!(registration_slot_00580958 = registration_find("vec_ceil")))
            return 0;
        if (!(registration_slot_0058095c = registration_find("vec_cmpb")))
            return 0;
        if (!(registration_slot_00580960 = registration_find("vec_cmpeq")))
            return 0;
        if (!(registration_slot_00580964 = registration_find("vec_cmpge")))
            return 0;
        if (!(registration_slot_00580968 = registration_find("vec_cmple")))
            return 0;
        if (!(registration_slot_0058096c = registration_find("vec_cmpgt")))
            return 0;
        if (!(registration_slot_00580970 = registration_find("vec_cmplt")))
            return 0;
        if (!(registration_slot_00580974 = registration_find("vec_ctf")))
            return 0;
        if (!(registration_slot_00580978 = registration_find("vec_cts")))
            return 0;
        if (!(registration_slot_00580898[57] = registration_find("vec_ctu")))
            return 0;
        if (!(registration_slot_00580998 = registration_find("vec_expte")))
            return 0;
        if (!(registration_slot_0058099c = registration_find("vec_floor")))
            return 0;
        if (!(registration_slot_005809a0 = registration_find("vec_ld")))
            return 0;
        if (!(registration_slot_005809a4 = registration_find("vec_lde")))
            return 0;
        if (!(registration_slot_005809a8 = registration_find("vec_ldl")))
            return 0;
        if (!(registration_slot_005809ac = registration_find("vec_loge")))
            return 0;
        if (!(registration_slot_005809b0 = registration_find("vec_lvsl")))
            return 0;
        if (!(registration_slot_005809b4 = registration_find("vec_lvsr")))
            return 0;
        if (!(registration_slot_005809b8 = registration_find("vec_madd")))
            return 0;
        if (!(registration_slot_005809bc = registration_find("vec_madds")))
            return 0;
        if (!(registration_slot_005809c0 = registration_find("vec_max")))
            return 0;
        if (!(registration_slot_005809c4 = registration_find("vec_mergeh")))
            return 0;
        if (!(registration_slot_005809c8 = registration_find("vec_mergel")))
            return 0;
        if (!(registration_slot_005809cc = registration_find("vec_mfvscr")))
            return 0;
        if (!(registration_slot_005809d0 = registration_find("vec_min")))
            return 0;
        if (!(registration_slot_005809d4 = registration_find("vec_mladd")))
            return 0;
        if (!(registration_slot_005809d8 = registration_find("vec_mradds")))
            return 0;
        if (!(registration_slot_005809dc = registration_find("vec_msum")))
            return 0;
        if (!(registration_slot_005809e0 = registration_find("vec_msums")))
            return 0;
        if (!(registration_slot_005809e4 = registration_find("vec_mtvscr")))
            return 0;
        if (!(registration_slot_005809e8 = registration_find("vec_mule")))
            return 0;
        if (!(registration_slot_005809ec = registration_find("vec_mulo")))
            return 0;
        if (!(registration_slot_005809f0 = registration_find("vec_nmsub")))
            return 0;
        if (!(registration_slot_005809f4 = registration_find("vec_nor")))
            return 0;
        if (!(registration_slot_005809f8 = registration_find("vec_or")))
            return 0;
        if (!(registration_slot_005809fc = registration_find("vec_pack")))
            return 0;
        if (!(registration_slot_00580a00 = registration_find("vec_packpx")))
            return 0;
        if (!(registration_slot_00580a04 = registration_find("vec_packs")))
            return 0;
        if (!(registration_slot_00580a08 = registration_find("vec_packsu")))
            return 0;
        if (!(registration_slot_00580a0c = registration_find("vec_perm")))
            return 0;
        if (!(registration_slot_00580a10 = registration_find("vec_re")))
            return 0;
        if (!(registration_slot_00580a14 = registration_find("vec_rl")))
            return 0;
        if (!(registration_slot_00580a18 = registration_find("vec_round")))
            return 0;
        if (!(registration_slot_00580a1c = registration_find("vec_rsqrte")))
            return 0;
        if (!(registration_slot_00580a20 = registration_find("vec_sel")))
            return 0;
        if (!(registration_slot_00580a24 = registration_find("vec_sl")))
            return 0;
        if (!(registration_slot_00580a28 = registration_find("vec_sld")))
            return 0;
        if (!(registration_slot_00580a2c = registration_find("vec_sll")))
            return 0;
        if (!(registration_slot_00580a30 = registration_find("vec_slo")))
            return 0;
        if (!(registration_slot_00580a34 = registration_find("vec_splat")))
            return 0;
        if (!(registration_slot_00580a38 = registration_find("vec_splat_s8")))
            return 0;
        if (!(registration_slot_00580a3c = registration_find("vec_splat_s16")))
            return 0;
        if (!(registration_slot_00580a40 = registration_find("vec_splat_s32")))
            return 0;
        if (!(registration_slot_00580a44 = registration_find("vec_splat_u8")))
            return 0;
        if (!(registration_slot_00580a48 = registration_find("vec_splat_u16")))
            return 0;
        if (!(registration_slot_00580a4c = registration_find("vec_splat_u32")))
            return 0;
        if (!(registration_slot_00580a50 = registration_find("vec_sr")))
            return 0;
        if (!(registration_slot_00580a54 = registration_find("vec_sra")))
            return 0;
        if (!(registration_slot_00580a58 = registration_find("vec_srl")))
            return 0;
        if (!(registration_slot_00580a5c = registration_find("vec_sro")))
            return 0;
        if (!(registration_slot_00580a60 = registration_find("vec_st")))
            return 0;
        if (!(registration_slot_00580a64 = registration_find("vec_ste")))
            return 0;
        if (!(registration_slot_00580a68 = registration_find("vec_stl")))
            return 0;
        if (!(registration_slot_00580a6c = registration_find("vec_sub")))
            return 0;
        if (!(registration_slot_00580a70 = registration_find("vec_subc")))
            return 0;
        if (!(registration_slot_00580a74 = registration_find("vec_subs")))
            return 0;
        if (!(registration_slot_00580a78 = registration_find("vec_sum4s")))
            return 0;
        if (!(registration_slot_00580a7c = registration_find("vec_sum2s")))
            return 0;
        if (!(registration_slot_00580a80 = registration_find("vec_sums")))
            return 0;
        if (!(registration_slot_00580a84 = registration_find("vec_trunc")))
            return 0;
        if (!(registration_slot_00580a88 = registration_find("vec_unpack2sh")))
            return 0;
        if (!(registration_slot_00580a8c = registration_find("vec_unpack2sl")))
            return 0;
        if (!(registration_slot_00580a90 = registration_find("vec_unpack2uh")))
            return 0;
        if (!(registration_slot_00580a94 = registration_find("vec_unpack2ul")))
            return 0;
        if (!(registration_slot_00580a98 = registration_find("vec_unpackh")))
            return 0;
        if (!(registration_slot_00580a9c = registration_find("vec_unpackl")))
            return 0;
        if (!(registration_slot_00580aa0 = registration_find("vec_xor")))
            return 0;
        if (!(registration_slot_00580aa4 = registration_find("vec_all_eq")))
            return 0;
        if (!(registration_slot_00580aa8 = registration_find("vec_all_ge")))
            return 0;
        if (!(registration_slot_00580aac = registration_find("vec_all_gt")))
            return 0;
        if (!(registration_slot_00580ab0 = registration_find("vec_all_in")))
            return 0;
        if (!(registration_slot_00580ab4 = registration_find("vec_all_le")))
            return 0;
        if (!(registration_slot_00580ab8 = registration_find("vec_all_lt")))
            return 0;
        if (!(registration_slot_00580abc = registration_find("vec_all_nan")))
            return 0;
        if (!(registration_slot_00580ac0 = registration_find("vec_all_ne")))
            return 0;
        if (!(registration_slot_00580ac4 = registration_find("vec_all_nge")))
            return 0;
        if (!(registration_slot_00580ac8 = registration_find("vec_all_ngt")))
            return 0;
        if (!(registration_slot_00580acc = registration_find("vec_all_nle")))
            return 0;
        if (!(registration_slot_00580ad0 = registration_find("vec_all_nlt")))
            return 0;
        if (!(registration_slot_00580ad4 = registration_find("vec_all_numeric")))
            return 0;
        if (!(registration_slot_00580ad8 = registration_find("vec_any_eq")))
            return 0;
        if (!(registration_slot_00580adc = registration_find("vec_any_ge")))
            return 0;
        if (!(registration_slot_00580ae0 = registration_find("vec_any_gt")))
            return 0;
        if (!(registration_slot_00580ae4 = registration_find("vec_any_le")))
            return 0;
        if (!(registration_slot_00580ae8 = registration_find("vec_any_lt")))
            return 0;
        if (!(registration_slot_00580aec = registration_find("vec_any_nan")))
            return 0;
        if (!(registration_slot_00580af0 = registration_find("vec_any_ne")))
            return 0;
        if (!(registration_slot_00580af4 = registration_find("vec_any_nge")))
            return 0;
        if (!(registration_slot_00580af8 = registration_find("vec_any_ngt")))
            return 0;
        if (!(registration_slot_00580afc = registration_find("vec_any_nle")))
            return 0;
        if (!(registration_slot_00580b00 = registration_find("vec_any_nlt")))
            return 0;
        if (!(registration_slot_00580b04 = registration_find("vec_any_numeric")))
            return 0;
        if (!(registration_slot_00580b08 = registration_find("vec_any_out")))
            return 0;
        if (!(registration_slot_00580b0c = registration_find("vec_vaddubm")))
            return 0;
        if (!(registration_slot_00580b10 = registration_find("vec_vadduhm")))
            return 0;
        if (!(registration_slot_00580b14 = registration_find("vec_vadduwm")))
            return 0;
        if (!(registration_slot_00580b18 = registration_find("vec_vaddfp")))
            return 0;
        if (!(registration_slot_00580b1c = registration_find("vec_vaddcuw")))
            return 0;
        if (!(registration_slot_00580b20 = registration_find("vec_vaddubs")))
            return 0;
        if (!(registration_slot_00580b24 = registration_find("vec_vaddubs")))
            return 0;
        if (!(registration_slot_00580b28 = registration_find("vec_vadduhs")))
            return 0;
        if (!(registration_slot_00580b2c = registration_find("vec_vadduhs")))
            return 0;
        if (!(registration_slot_00580b30 = registration_find("vec_vadduws")))
            return 0;
        if (!(registration_slot_00580b34 = registration_find("vec_vadduws")))
            return 0;
        if (!(registration_slot_00580b38 = registration_find("vec_vand")))
            return 0;
        if (!(registration_slot_00580b3c = registration_find("vec_vandc")))
            return 0;
        if (!(registration_slot_00580b40 = registration_find("vec_vavgub")))
            return 0;
        if (!(registration_slot_00580b44 = registration_find("vec_vavgsb")))
            return 0;
        if (!(registration_slot_00580b48 = registration_find("vec_vavguh")))
            return 0;
        if (!(registration_slot_00580b4c = registration_find("vec_vavgsh")))
            return 0;
        if (!(registration_slot_00580b50 = registration_find("vec_vavguw")))
            return 0;
        if (!(registration_slot_00580b54 = registration_find("vec_vavgsw")))
            return 0;
        if (!(registration_slot_00580b58 = registration_find("vec_vrfip")))
            return 0;
        if (!(registration_slot_00580b5c = registration_find("vec_vcmpbfp")))
            return 0;
        if (!(registration_slot_00580b60 = registration_find("vec_vcmpequb")))
            return 0;
        if (!(registration_slot_00580b64 = registration_find("vec_vcmpequh")))
            return 0;
        if (!(registration_slot_00580b68 = registration_find("vec_vcmpequw")))
            return 0;
        if (!(registration_slot_00580b6c = registration_find("vec_vcmpeqfp")))
            return 0;
        if (!(registration_slot_00580b70 = registration_find("vec_vcmpgefp")))
            return 0;
        if (!(registration_slot_00580b74 = registration_find("vec_vcmpgtub")))
            return 0;
        if (!(registration_slot_00580b78 = registration_find("vec_vcmpgtsb")))
            return 0;
        if (!(registration_slot_00580b7c = registration_find("vec_vcmpgtuh")))
            return 0;
        if (!(registration_slot_00580b80 = registration_find("vec_vcmpgtsh")))
            return 0;
        if (!(registration_slot_00580b84 = registration_find("vec_vcmpgtuw")))
            return 0;
        if (!(registration_slot_00580b88 = registration_find("vec_vcmpgtsw")))
            return 0;
        if (!(registration_slot_00580b8c = registration_find("vec_vcmpgtfp")))
            return 0;
        if (!(registration_slot_00580b90 = registration_find("vec_vcfux")))
            return 0;
        if (!(registration_slot_00580b94 = registration_find("vec_vcfsx")))
            return 0;
        if (!(registration_slot_00580b98 = registration_find("vec_vctsxs")))
            return 0;
        if (!(registration_slot_00580b9c = registration_find("vec_vctuxs")))
            return 0;
        if (!(registration_slot_00580ba0 = registration_find("vec_vexptefp")))
            return 0;
        if (!(registration_slot_00580ba4 = registration_find("vec_vrfim")))
            return 0;
        if (!(registration_slot_00580ba8 = registration_find("vec_lvx")))
            return 0;
        if (!(registration_slot_00580bac = registration_find("vec_lvebx")))
            return 0;
        if (!(registration_slot_00580bb0 = registration_find("vec_lvehx")))
            return 0;
        if (!(registration_slot_00580bb4 = registration_find("vec_lvewx")))
            return 0;
        if (!(registration_slot_00580bb8 = registration_find("vec_lvxl")))
            return 0;
        if (!(registration_slot_00580bbc = registration_find("vec_vlogefp")))
            return 0;
        if (!(registration_slot_00580bc0 = registration_find("vec_vmaddfp")))
            return 0;
        if (!(registration_slot_00580bc4 = registration_find("vec_vmhaddshs")))
            return 0;
        if (!(registration_slot_00580bc8 = registration_find("vec_vmaxub")))
            return 0;
        if (!(registration_slot_00580bcc = registration_find("vec_vmaxsb")))
            return 0;
        if (!(registration_slot_00580bd0 = registration_find("vec_vmaxuh")))
            return 0;
        if (!(registration_slot_00580bd4 = registration_find("vec_vmaxsh")))
            return 0;
        if (!(registration_slot_00580bd8 = registration_find("vec_vmaxuw")))
            return 0;
        if (!(registration_slot_00580bdc = registration_find("vec_vmaxsw")))
            return 0;
        if (!(registration_slot_00580be0 = registration_find("vec_vmaxfp")))
            return 0;
        if (!(registration_slot_00580be4 = registration_find("vec_vmrghb")))
            return 0;
        if (!(registration_slot_00580be8 = registration_find("vec_vmrghh")))
            return 0;
        if (!(registration_slot_00580bec = registration_find("vec_vmrghw")))
            return 0;
        if (!(registration_slot_00580bf0 = registration_find("vec_vmrglb")))
            return 0;
        if (!(registration_slot_00580bf4 = registration_find("vec_vmrglh")))
            return 0;
        if (!(registration_slot_00580898[216] = registration_find("vec_vmrglw")))
            return 0;
        if (!(registration_slot_00580bc8 = registration_find("vec_vminub")))
            return 0;
        if (!(registration_slot_00580bcc = registration_find("vec_vminsb")))
            return 0;
        if (!(registration_slot_00580bd0 = registration_find("vec_vminuh")))
            return 0;
        if (!(registration_slot_00580bd4 = registration_find("vec_vminsh")))
            return 0;
        if (!(registration_slot_00580bd8 = registration_find("vec_vminuw")))
            return 0;
        if (!(registration_slot_00580bdc = registration_find("vec_vminsw")))
            return 0;
        if (!(registration_slot_00580be0 = registration_find("vec_vminfp")))
            return 0;
        if (!(registration_slot_00580c18 = registration_find("vec_vmladduhm")))
            return 0;
        if (!(registration_slot_00580c1c = registration_find("vec_vmhraddshs")))
            return 0;
        if (!(registration_slot_00580c20 = registration_find("vec_vmsumubm")))
            return 0;
        if (!(registration_slot_00580c24 = registration_find("vec_vmsumuhm")))
            return 0;
        if (!(registration_slot_00580c28 = registration_find("vec_vmsummbm")))
            return 0;
        if (!(registration_slot_00580c2c = registration_find("vec_vmsumshm")))
            return 0;
        if (!(registration_slot_00580c30 = registration_find("vec_vmsumuhs")))
            return 0;
        if (!(registration_slot_00580c34 = registration_find("vec_vmsumshs")))
            return 0;
        if (!(registration_slot_00580c38 = registration_find("vec_vmuleub")))
            return 0;
        if (!(registration_slot_00580c3c = registration_find("vec_vmulesb")))
            return 0;
        if (!(registration_slot_00580c40 = registration_find("vec_vmuleuh")))
            return 0;
        if (!(registration_slot_00580c44 = registration_find("vec_vmulesh")))
            return 0;
        if (!(registration_slot_00580c48 = registration_find("vec_vmuloub")))
            return 0;
        if (!(registration_slot_00580c4c = registration_find("vec_vmulosb")))
            return 0;
        if (!(registration_slot_00580c50 = registration_find("vec_vmulouh")))
            return 0;
        if (!(registration_slot_00580c54 = registration_find("vec_vmulosh")))
            return 0;
        if (!(registration_slot_00580c58 = registration_find("vec_vnmsubfp")))
            return 0;
        if (!(registration_slot_00580c5c = registration_find("vec_vnor")))
            return 0;
        if (!(registration_slot_00580c60 = registration_find("vec_vor")))
            return 0;
        if (!(registration_slot_00580c64 = registration_find("vec_vpkuhum")))
            return 0;
        if (!(registration_slot_00580c68 = registration_find("vec_vpkuwum")))
            return 0;
        if (!(registration_slot_00580c6c = registration_find("vec_vpkpx")))
            return 0;
        if (!(registration_slot_00580c70 = registration_find("vec_vpkuhus")))
            return 0;
        if (!(registration_slot_00580c74 = registration_find("vec_vpkshss")))
            return 0;
        if (!(registration_slot_00580c78 = registration_find("vec_vpkuwus")))
            return 0;
        if (!(registration_slot_00580c7c = registration_find("vec_vpkswss")))
            return 0;
        if (!(registration_slot_00580c80 = registration_find("vec_vpkshus")))
            return 0;
        if (!(registration_slot_00580c84 = registration_find("vec_vpkswus")))
            return 0;
        if (!(registration_slot_00580c88 = registration_find("vec_vperm")))
            return 0;
        if (!(registration_slot_00580c8c = registration_find("vec_vrefp")))
            return 0;
        if (!(registration_slot_00580c90 = registration_find("vec_vrlb")))
            return 0;
        if (!(registration_slot_00580c94 = registration_find("vec_vrlh")))
            return 0;
        if (!(registration_slot_00580c98 = registration_find("vec_vrlw")))
            return 0;
        if (!(registration_slot_00580c9c = registration_find("vec_vrfin")))
            return 0;
        if (!(registration_slot_00580ca0 = registration_find("vec_vrsqrtefp")))
            return 0;
        if (!(registration_slot_00580ca4 = registration_find("vec_vsel")))
            return 0;
        if (!(registration_slot_00580ca8 = registration_find("vec_vslb")))
            return 0;
        if (!(registration_slot_00580cac = registration_find("vec_vslh")))
            return 0;
        if (!(registration_slot_00580cb0 = registration_find("vec_vslw")))
            return 0;
        if (!(registration_slot_00580cb4 = registration_find("vec_vsldoi")))
            return 0;
        if (!(registration_slot_00580cb8 = registration_find("vec_vsl")))
            return 0;
        if (!(registration_slot_00580cbc = registration_find("vec_vslo")))
            return 0;
        if (!(registration_slot_00580cc0 = registration_find("vec_vspltb")))
            return 0;
        if (!(registration_slot_00580cc4 = registration_find("vec_vsplth")))
            return 0;
        if (!(registration_slot_00580cc8 = registration_find("vec_vspltw")))
            return 0;
        if (!(registration_slot_00580ccc = registration_find("vec_vspltisb")))
            return 0;
        if (!(registration_slot_00580cd0 = registration_find("vec_vspltish")))
            return 0;
        if (!(registration_slot_00580cd4 = registration_find("vec_vspltisw")))
            return 0;
        if (!(registration_slot_00580cd8 = registration_find("vec_vsrb")))
            return 0;
        if (!(registration_slot_00580cdc = registration_find("vec_vsrh")))
            return 0;
        if (!(registration_slot_00580ce0 = registration_find("vec_vsrw")))
            return 0;
        if (!(registration_slot_00580ce4 = registration_find("vec_vsrab")))
            return 0;
        if (!(registration_slot_00580ce8 = registration_find("vec_vsrah")))
            return 0;
        if (!(registration_slot_00580cec = registration_find("vec_vsraw")))
            return 0;
        if (!(registration_slot_00580cf0 = registration_find("vec_vsr")))
            return 0;
        if (!(registration_slot_00580cf4 = registration_find("vec_vsro")))
            return 0;
        if (!(registration_slot_00580cf8 = registration_find("vec_stvx")))
            return 0;
        if (!(registration_slot_00580cfc = registration_find("vec_stvebx")))
            return 0;
        if (!(registration_slot_00580d00 = registration_find("vec_stvehx")))
            return 0;
        if (!(registration_slot_00580d04 = registration_find("vec_stvewx")))
            return 0;
        if (!(registration_slot_00580d08 = registration_find("vec_stvxl")))
            return 0;
        if (!(registration_slot_00580d0c = registration_find("vec_vsububm")))
            return 0;
        if (!(registration_slot_00580d10 = registration_find("vec_vsubuhm")))
            return 0;
        if (!(registration_slot_00580d14 = registration_find("vec_vsubuwm")))
            return 0;
        if (!(registration_slot_00580d18 = registration_find("vec_vsubfp")))
            return 0;
        if (!(registration_slot_00580d1c = registration_find("vec_vsubcuw")))
            return 0;
        if (!(registration_slot_00580d20 = registration_find("vec_vsububs")))
            return 0;
        if (!(registration_slot_00580d24 = registration_find("vec_vsubsbs")))
            return 0;
        if (!(registration_slot_00580d28 = registration_find("vec_vsubuhs")))
            return 0;
        if (!(registration_slot_00580d2c = registration_find("vec_vsubshs")))
            return 0;
        if (!(registration_slot_00580d30 = registration_find("vec_vsubuws")))
            return 0;
        if (!(registration_slot_00580d34 = registration_find("vec_vsubsws")))
            return 0;
        if (!(registration_slot_00580d38 = registration_find("vec_vsum4ubs")))
            return 0;
        if (!(registration_slot_00580d3c = registration_find("vec_vsum4sbs")))
            return 0;
        if (!(registration_slot_00580d40 = registration_find("vec_vsum4shs")))
            return 0;
        if (!(registration_slot_00580d44 = registration_find("vec_vsum2sws")))
            return 0;
        if (!(registration_slot_00580d48 = registration_find("vec_vsumsws")))
            return 0;
        if (!(registration_slot_00580d4c = registration_find("vec_vrfiz")))
            return 0;
        if (!(registration_slot_00580d50 = registration_find("vec_vupkhsb")))
            return 0;
        if (!(registration_slot_00580d54 = registration_find("vec_vupklsb")))
            return 0;
        if (!(registration_slot_00580d58 = registration_find("vec_vupkhpx")))
            return 0;
        if (!(registration_slot_00580d5c = registration_find("vec_vupklpx")))
            return 0;
        if (!(registration_slot_00580d60 = registration_find("vec_vupkhsh")))
            return 0;
        if (!(registration_slot_00580d64 = registration_find("vec_vupklsh")))
            return 0;
        if (!(registration_slot_00580d68 = registration_find("vec_vxor")))
            return 0;
        if (!(registration_slot_00580d6c = registration_find("vec_abs")))
            return 0;
        if (!(registration_slot_00580d70 = registration_find("vec_abss")))
            return 0;
        if (!(registration_slot_00580d74 = registration_find("__va_setup")))
            return 0;
        if (!(registration_slot_00580d78 = registration_find("__builtin_va_info")))
            return 0;
    }
    return 1;
}

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

void Intrinsics_RegisterIntrinsics(void)
{
    unsigned char saved_cplusplus = copts.cplusplus;
    int i;
    TypeFunc *function;

    copts.cplusplus = 0;
    for (i = 0; i < 313; i++)
        registration_slot_00580898[i] = NULL;
    registration_slot_00580898[0] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__eieio"), 0, 0);
    registration_slot_00580898[1] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__sync"), 0, 0);
    registration_slot_00580898[2] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__isync"), 0, 0);
    registration_slot_00580898[3] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__abs"), 0, 1, &stsignedint);
    registration_slot_00580898[4] =
        CParser_NewRTFunc((Type *)&stsignedlong, GetHashNameNode("__labs"), 0, 1, &stsignedlong);
    registration_slot_00580898[5] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fabs"), 0, 1, &stdouble);
    registration_slot_00580898[6] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnabs"), 0, 1, &stdouble);
    registration_slot_00580898[7] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__setflm"), 0, 1, &stdouble);
    registration_slot_00580898[33] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__frsqrte"), 0, 1, &stdouble);
    registration_slot_00580898[8] =
        CParser_NewRTFunc((Type *)&void_ptr, GetHashNameNode("__alloca"), 0, 1, &stunsignedint);
    registration_slot_00580898[9] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__cntlzw"), 0, 1, &stunsignedint);
    registration_slot_00580898[10] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__lhbrx"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[11] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__lwbrx"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[12] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__sthbrx"), 0, 3, &stunsignedshort, &void_ptr, &stsignedint);
    registration_slot_00580898[13] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__stwbrx"), 0, 3, &stunsignedint, &void_ptr, &stsignedint);
    registration_slot_00580898[14] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbf"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[15] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbt"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[16] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbst"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[17] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbtst"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[18] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("__dcbz"), 0, 2, &void_ptr, &stsignedint);
    registration_slot_00580898[19] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__mulhw"), 0, 2, &stsignedint, &stsignedint);
    registration_slot_00580898[20] =
        CParser_NewRTFunc((Type *)&stunsignedint, GetHashNameNode("__mulhwu"), 0, 2, &stunsignedint, &stunsignedint);
    registration_slot_00580898[21] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__divw"), 0, 2, &stsignedint, &stsignedint);
    registration_slot_00580898[22] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__divwu"), 0, 2, &stsignedint, &stsignedint);
    registration_slot_00580898[23] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fmadd"), 0, 3, &stdouble, &stdouble, &stdouble);
    registration_slot_00580898[24] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fmsub"), 0, 3, &stdouble, &stdouble, &stdouble);
    registration_slot_00580898[25] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnmadd"), 0, 3, &stdouble, &stdouble, &stdouble);
    registration_slot_00580898[26] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fnmsub"), 0, 3, &stdouble, &stdouble, &stdouble);
    registration_slot_00580898[34] =
        CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__fsel"), 0, 3, &stdouble, &stdouble, &stdouble);
    registration_slot_00580898[27] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fmadds"), 0, 3, &stfloat, &stfloat, &stfloat);
    registration_slot_00580898[28] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fmsubs"), 0, 3, &stfloat, &stfloat, &stfloat);
    registration_slot_00580898[29] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnmadds"), 0, 3, &stfloat, &stfloat, &stfloat);
    registration_slot_00580898[30] =
        CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnmsubs"), 0, 3, &stfloat, &stfloat, &stfloat);
    registration_slot_00580898[31] = CParser_NewRTFunc((Type *)&stdouble, GetHashNameNode("__mffs"), 0, 0);
    registration_slot_00580898[32] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fres"), 0, 1, &stfloat);
    registration_slot_00580898[40] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fabsf"), 0, 1, &stfloat);
    registration_slot_00580898[41] = CParser_NewRTFunc((Type *)&stfloat, GetHashNameNode("__fnabsf"), 0, 1, &stfloat);
    registration_slot_00580898[35] = CParser_NewRTFunc(&initializer_data_0055bb90, GetHashNameNode("__strcpy"), 0, 2,
                                                       &initializer_data_0055bb90, &initializer_data_0055bb90);
    function = (TypeFunc *)registration_slot_00580898[35]->type;
    function->args->next->qual |= Q_CONST;
    registration_slot_00580898[36] =
        CParser_NewRTFunc((Type *)&void_ptr, GetHashNameNode("__memcpy"), 0, 3, &void_ptr, &void_ptr, &stunsignedlong);
    data_00587fc0 = registration_slot_00580898[36];
    function = (TypeFunc *)registration_slot_00580898[36]->type;
    function->args->next->qual |= Q_CONST;
    registration_slot_00580898[37] =
        CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwimi"), 0, 5, &stsignedint, &stsignedint,
                          &stsignedint, &stsignedint, &stsignedint);
    registration_slot_00580898[38] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwinm"), 0, 4,
                                                       &stsignedint, &stsignedint, &stsignedint, &stsignedint);
    registration_slot_00580898[39] = CParser_NewRTFunc((Type *)&stsignedint, GetHashNameNode("__rlwnm"), 0, 4,
                                                       &stsignedint, &stsignedint, &stsignedint, &stsignedint);
    registration_slot_00580898[42] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_add"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[43] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_addc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[44] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_adds"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[45] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_and"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[46] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_andc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[47] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_avg"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[48] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ceil"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[49] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[50] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpeq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[51] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[52] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmple"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[53] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmpgt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[54] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cmplt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[55] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ctf"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[56] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_cts"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[57] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ctu"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[58] = CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dss"), 0, 1, &stsignedint);
    registration_slot_00580898[59] = CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dssall"), 0, 0);
    registration_slot_00580898[60] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dst"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    registration_slot_00580898[61] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dstst"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    registration_slot_00580898[62] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dststt"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    registration_slot_00580898[63] =
        CParser_NewRTFunc(&stvoid, GetHashNameNode("vec_dstt"), 0, 3, TYPE(&stvector), &stsignedint, &stsignedint);
    registration_slot_00580898[64] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_expte"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[65] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_floor"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[66] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ld"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[67] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lde"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[68] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ldl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[69] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_loge"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[70] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvsl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[71] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvsr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[72] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_madd"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[73] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_madds"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[74] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_max"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[75] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mergeh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[76] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mergel"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[77] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mfvscr"), 0, 0);
    registration_slot_00580898[78] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_min"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[79] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mladd"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[80] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mradds"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[81] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_msum"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[82] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_msums"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[83] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mtvscr"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[84] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mule"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[85] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_mulo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[86] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_nmsub"), 0, 2,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[87] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_nor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[88] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_or"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[89] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_pack"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[90] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packpx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[91] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[92] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_packsu"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[93] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_perm"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[94] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_re"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[95] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_rl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[96] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_round"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[97] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_rsqrte"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[98] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sel"), 0, 3,
                                                       TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[99] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[100] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sld"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[101] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sll"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[102] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_slo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[103] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[104] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s8"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[105] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s16"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[106] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_s32"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[107] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u8"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[108] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u16"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[109] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_splat_u32"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[110] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[111] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sra"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[112] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_srl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[113] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sro"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[114] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_st"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[115] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_ste"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[116] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stl"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[117] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[118] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_subc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[119] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_subs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[120] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sum4s"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[121] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sum2s"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[122] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_sums"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[123] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_trunc"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[124] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2sh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[125] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2sl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[126] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2uh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[127] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpack2ul"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[128] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpackh"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[129] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_unpackl"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[130] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_xor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[131] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_eq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[132] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[133] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_gt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[134] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_in"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[135] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_le"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[136] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_lt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[137] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nan"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[138] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ne"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[139] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[140] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_ngt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[141] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nle"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[142] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_nlt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[143] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_all_numeric"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[144] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_eq"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[145] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[146] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_gt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[147] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_le"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[148] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_lt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[149] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nan"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[150] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ne"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[151] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nge"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[152] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_ngt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[153] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nle"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[154] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_nlt"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[155] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_numeric"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[156] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_any_out"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[157] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddubm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[158] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduhm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[159] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduwm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[160] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[161] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddcuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[162] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddubs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[163] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddsbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[164] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduhs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[165] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddshs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[166] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vadduws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[167] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vaddsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[168] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vand"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[169] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vandc"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[170] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[171] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[172] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavguh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[173] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[174] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavguw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[175] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vavgsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[176] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfip"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[177] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpbfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[178] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[179] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[180] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpequw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[181] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpeqfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[182] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgefp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[183] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[184] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[185] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[186] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[187] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[188] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[189] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcmpgtfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[190] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcfux"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[191] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vcfsx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[192] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vctsxs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[193] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vctuxs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[194] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vexptefp"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[195] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfim"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[196] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[197] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvebx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[198] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvehx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[199] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvewx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[200] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_lvxl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[201] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vlogefp"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[202] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaddfp"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[203] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmhaddshs"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[204] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[205] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[206] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[207] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[208] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[209] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[210] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmaxfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[211] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[212] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[213] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrghw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[214] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[215] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[216] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmrglw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[217] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[218] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[219] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[220] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[221] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[222] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminsw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[223] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vminfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[224] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmladduhm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[225] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmhraddshs"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[226] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumubm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[227] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumuhm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[228] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsummbm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[229] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumshm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[230] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumuhs"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[231] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmsumshs"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[232] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuleub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[233] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulesb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[234] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuleuh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[235] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulesh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[236] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmuloub"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[237] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulosb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[238] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulouh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[239] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vmulosh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[240] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vnmsubfp"), 0, 2,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[241] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vnor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[242] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[243] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuhum"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[244] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuwum"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[245] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkpx"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[246] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuhus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[247] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkshss"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[248] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkuwus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[249] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkswss"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[250] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkshus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[251] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vpkswus"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[252] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vperm"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[253] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrefp"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[254] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[255] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[256] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrlw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[257] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfin"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[258] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrsqrtefp"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[259] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsel"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[260] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[261] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[262] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[263] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsldoi"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[264] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsl"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[265] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vslo"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[266] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[267] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsplth"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[268] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[269] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltisb"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[270] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltish"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[271] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vspltisw"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[272] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrb"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[273] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrh"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[274] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[275] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrab"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[276] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsrah"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[277] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsraw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[278] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsr"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[279] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsro"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[280] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvx"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[281] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvebx"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[282] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvehx"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[283] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvewx"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[284] = CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_stvxl"), 0, 3,
                                                        TYPE(&stvector), TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[285] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsububm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[286] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuhm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[287] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuwm"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[288] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubfp"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[289] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubcuw"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[290] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsububs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[291] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubsbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[292] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuhs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[293] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubshs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[294] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubuws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[295] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsubsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[296] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4ubs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[297] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4sbs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[298] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum4shs"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[299] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsum2sws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[300] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vsumsws"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[301] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vrfiz"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[302] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhsb"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[303] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklsb"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[304] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhpx"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[305] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklpx"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[306] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupkhsh"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[307] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vupklsh"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[308] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_vxor"), 0, 2, TYPE(&stvector), TYPE(&stvector));
    registration_slot_00580898[309] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_abs"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[310] =
        CParser_NewRTFunc(TYPE(&stvector), GetHashNameNode("vec_abss"), 0, 1, TYPE(&stvector));
    registration_slot_00580898[311] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__va_setup"), 0, 1, &void_ptr);
    registration_slot_00580898[312] = CParser_NewRTFunc(&stvoid, GetHashNameNode("__builtin_va_info"), 0, 1, &void_ptr);
    for (i = 0; i < 313; i++) {
        if (!registration_slot_00580898[i])
            CError_FATAL(6535);
        registration_slot_00580898[i]->u.data.u.intconst.hi = i;
        function = (TypeFunc *)registration_slot_00580898[i]->type;
        function->flags |= FUNC_INTRINSIC;
        CScope_AddGlobalObject(registration_slot_00580898[i]);
    }
    copts.cplusplus = saved_cplusplus;
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
    _DAT_005557c8 = (int)object;
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
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(registration_slot_00580898[intrinsic])->name);
            } else {
                InstrSelection_EmitUnaryFPRInstruction(intrinsic_opcodes[intrinsic], args->node, requestedReg, result);
            }
            break;
        case 7: {
            short selected;
            unsigned char *expr;
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(registration_slot_00580898[intrinsic])->name);
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
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(registration_slot_00580898[intrinsic])->name);
            } else {
                InstrSelection_EmitThreeOperandFPRInstruction(intrinsic_opcodes[intrinsic], args->node,
                                                              args->next->node, args->next->next->node, requestedReg,
                                                              result);
            }
            break;
        case 31:
            if (((copts.debugEnabled == '\0') || (copts.operandsDebug != '\0')) &&
                (node->data.funccall.funcref->type == '8')) {
                PPCError_FatalError(0x83, COptimizer_GetFunctionObject(registration_slot_00580898[intrinsic])->name);
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
            opcode = data_0055b4dc[(unsigned int)(intrinsic - 0x2a)].simple->code;
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
            opcode = data_0055b4dc[(unsigned int)(intrinsic - 0x2a)].simple->code;
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
            /* (stand-in: the original caller pushes the opcode through a short prototype) */ (
                (void (*)(ENode *, ENode *, ENode *, short, Operand *, short))emit_instruction_with_vr_result)(
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
            /* (stand-in: the original caller pushes the opcode through a short prototype) */ (
                (void (*)(ENode *, ENode *, short, Operand *, short, unsigned short))emit_record_form_condition)(
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
            Intrinsics_00486db0(intrinsic, node, args->node, requestedReg, result);
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

#undef false
#undef true

/* Intrinsic object fields used by this dispatcher. */

/* Linked arguments passed to intrinsic handlers. */

/* IntrinsicBinaryEntry in the intrinsic operation table. */

ENode *Intrinsics_MakeAltivecCall(Object *descriptor, ENodeList *args)
{
    Object *intrinsic = descriptor;
    ENode *result = NULL;
    unsigned short intrinsicID;
    int tableIndex;
    if (copts.altivecModel != 0) {
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
                call = (ENode *)CompilerTools_AllocatePool(sizeof(*call));
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
                call = (ENode *)CompilerTools_AllocatePool(sizeof(*call));
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
                call = (ENode *)CompilerTools_AllocatePool(sizeof(*call));
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
                operation = (Type *)data_0055b4dc[tableIndex].operation->operation;
                if (operation == NULL) {
                    break;
                }
                functionType = (TypeFunc *)intrinsic->type;
                if (functionType->type != TYPEFUNC) {
                    CError_FATAL(3718);
                }
                call = (ENode *)CompilerTools_AllocatePool(sizeof(*call));
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
                call = (ENode *)CompilerTools_AllocatePool(sizeof(*call));
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

#undef CE_ASSERT

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

void Intrinsics_00486db0(UInt16 token, ENode *unused, ENode *node, SInt16 requestedReg, Operand *result)
{
    IntrinsicVariant *variant;
    SInt16 zeroReg;
    SInt16 intermediateReg;
    SInt16 resultReg;
    Operand operand;
    SInt32 index;

    index = token - 0x2a;
    variant = data_0055b4dc[index].variant;
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

#undef CE_ASSERT

void generate_unary_vector_intrinsic(UInt16 token, ENode *unused, ENode *node, SInt16 requestedReg, Operand *result)
{
    IntrinsicVariant *variant;
    SInt16 constantReg;
    SInt16 temporaryReg;
    SInt16 resultReg;
    Operand operand;
    SInt32 intrinsicIndex;

    intrinsicIndex = token - 0x2a;
    variant = data_0055b4dc[intrinsicIndex].variant;
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

#undef CE_ASSERT
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

UInt16 find_intrinsic_triple_code(UInt16 id, ENode *unused, ENode *firstOperand, ENode *secondOperand,
                                  ENode *thirdOperand)
{
    IntrinsicTripleEntry *entry;
    Type *firstActualType, *secondActualType, *firstExpectedType, *secondExpectedType, *thirdExpectedType,
        *thirdActualType;
    UInt32 intrinsicIndex;

    intrinsicIndex = id - 0x2a;
    for (entry = data_0055b4dc[intrinsicIndex].triple; entry->result != NULL; entry++) {
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

UInt16 find_binary_intrinsic_code(UInt16 id, ENode *unused, ENode *left, ENode *right)
{
    IntrinsicBinaryEntry *record;
    Type *leftType, *rightType, *expectedLeft, *expectedRight;
    UInt32 index;
    index = id - 0x2a;
    for (record = data_0055b4dc[index].binary; record->result != NULL; record++) {
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

/* Type alternatives for an intrinsic, terminated by a null entry. */

UInt16 find_unary_intrinsic_code(UInt16 id, ENode *unused, ENode *expression)
{
    IntrinsicTypeEntry *entry;
    Type *expressionType, *candidateType;
    UInt32 index;
    index = id - 0x2a;
    for (entry = data_0055b4dc[index].unary; entry->result != NULL; entry++) {
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

static inline void unwrapIntrinsicPointerTypes(Type **expected, Type **actual)
{
    if ((*expected)->type == TYPEPOINTER && (*actual)->type == TYPEPOINTER) {
        *expected = ((TypePointer *)*expected)->target;
        *actual = ((TypePointer *)*actual)->target;
    }
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
    for (entry = data_0055b4dc[index].triple; entry->result; entry++) {
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
    entry = data_0055b4dc[index].binary;
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

SInt32 select_altivec_mangle_result(UInt16 intrinsicCode, ENodeList *arguments, HashNameNode *name)
{
    ENode *node = arguments->node;
    SInt32 index = intrinsicCode - 0x2a;
    MangleEntry *entry = DAT_0055542c[index + 0x182c];
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

static int OpIndex(UInt16 token)
{
    return token - 0x2a;
}

Type *find_matching_op_result(UInt16 token, ENodeList *args, HashNameNode *name)
{
    ENode *node;
    OpEntry *entry;
    Type *argumentType;
    Type *operandType;

    node = args->node;
    for (entry = data_0055b4dc[OpIndex(token)].op; entry->result; entry++) {
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
