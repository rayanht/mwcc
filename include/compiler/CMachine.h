#ifndef COMPILER_CMACHINE_H
#define COMPILER_CMACHINE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FloatFormatBuffer {
    double value;
};
extern long CMach_StructLayoutBitfield(TypeBitfield *field, int alignmentKind);
extern long CMach_StructLayoutGetOffset(Type *type, int flags);
extern void CMach_StructLayoutInitOffset(unsigned int a0);
extern void CMachine_ResetMaximumAlignment(void);
extern SInt16 CMachine_GetTypeAlignment(Type *type);
extern UInt16 fn_004a8400(TypeStruct *str);
extern SInt16 CMach_MemberAlignValue(Type *a0, SInt32 a1);
extern int CMach_StructLayoutGetCurSize(void);
extern SInt16 get_type_align(Type *a0);
extern short CMach_GetClassAlign(TypeClass *list);
extern void CMach_PragmaParams(void);
extern UInt8 CMach_FloatIsNegOne(double value);
extern unsigned char CMach_FloatIsOne(double value);
extern unsigned char CMach_FloatIsZero(double value);
extern void CMach_InitFloatMem(Type *type, Float value, unsigned char *dest);
extern Float CMach_CalcFloatConvertFromInt(Type *type, CInt64 value);
extern void *CMach_FloatScan(char *arg1, Float *out, char *flag);
extern unsigned char CMach_CalcVectorDiadicBool(unsigned int context, const union MWVector128 *left,
                                                unsigned int operation, const union MWVector128 *right);
extern Boolean CMach_CalcFloatDiadicBool(Type *self, volatile double a, SInt16 op, volatile double b);
extern Float CMach_CalcFloatMonadic(Type *type, short op, double value);
extern Float CMach_CalcFloatDiadic(Type *type, Float left, short op, Float right);
extern void CMachine_InitVectorMem(Type *type, MWVector128 val, void *mem);
extern void CMach_InitIntMem(Type *type, CInt64 val, void *mem);
extern CInt64 CMach_CalcIntConvertFromFloat(Type *type, double value);
extern CInt64 CMach_CalcIntMonadic(Type *type, SInt16 op, CInt64 val);
extern CInt64 CMach_CalcIntDiadic(Type *type, CInt64 a, SInt16 op, CInt64 b);
extern int CMach_GetQUALalign(int a0);
extern void CMach_PrintFloat(char *a0, Float val);
extern Float CMachine_RoundFloatToType(Type *type, Float value);
extern double negative_one;
extern double DAT_0055fff0;
extern double float_zero;
extern SInt16 loadalign_table[];
extern Float data_00560028;
extern int maximumAlignment;
extern int structLayoutOffset;
extern short data_00580fa0;
extern signed short bitfield_storage_size;
extern int data_00580fa4;
extern void initialize_hash_name_globals(void);
extern Float CMach_FloatReciprocal(Float a0);
extern Boolean CMach_FloatIsPowerOf2(Float f);
extern HashNameNode *data_0055f638;
extern HashNameNode *data_0055f64c;
extern HashNameNode *data_0055f660;
extern HashNameNode *data_0055f674;
extern HashNameNode *data_0055f688;
extern HashNameNode *data_0055f69c;
extern HashNameNode *data_0055f6b0;
extern HashNameNode *data_0055f6c4;
extern HashNameNode *nameHash8Ptr;
extern HashNameNode *data_0055f6ec;
extern HashNameNode *name_hash6;
extern HashNameNode *data_0055f714;
extern HashNameNode *data_0055f728;
extern HashNameNode *data_0055f73c;
extern HashNameNode *data_0055f750;
extern HashNameNode *data_0055f764;
extern HashNameNode *data_0055f778;
extern HashNameNode *data_0055f78c;
extern HashNameNode *data_0055f7a0;
extern HashNameNode *data_0055f7b4;
extern HashNameNode *data_0055f7c8;
extern HashNameNode *data_0055f7dc;
extern HashNameNode *data_0055f7f0;
extern HashNameNode *data_0055f804;
extern HashNameNode *name_hash8;
extern HashNameNode *data_0055f82c;
extern HashNameNode *data_0055f840;
extern HashNameNode *data_0055f854;
extern HashNameNode *data_0055f868;
extern HashNameNode *data_0055f87c;
extern HashNameNode *data_0055f890;
extern HashNameNode *data_0055f8a4;
extern HashNameNode *data_0055f8b8;
extern HashNameNode *data_0055f8cc;
extern HashNameNode *data_0055f8e0;
extern HashNameNode *data_0055f8f4;
extern HashNameNode *data_0055f908;
extern HashNameNode *data_0055f91c;
extern HashNameNode *data_0055f930;
extern HashNameNode *data_0055f944;
extern HashNameNode *data_0055f958;
extern HashNameNode *data_0055f96c;
extern HashNameNode *data_0055f980;
extern HashNameNode *data_0055f994;
extern HashNameNode *data_0055f9a8;
extern HashNameNode *data_0055f9bc;
extern HashNameNode *gNameHash2;
extern HashNameNode *data_0055f9e4;
extern HashNameNode *data_0055f9f8;
extern HashNameNode *data_0055fa0c;
extern HashNameNode *data_0055fa20;
extern HashNameNode *data_0055fa34;
extern HashNameNode *data_0055fa48;
extern HashNameNode *nameHash3Ptr;
extern HashNameNode *data_0055fa70;
extern HashNameNode *data_0055fa84;
extern HashNameNode *data_0055fa98;
extern HashNameNode *data_0055faac;
extern HashNameNode *data_0055fac0;
extern HashNameNode *data_0055fad4;
extern HashNameNode *data_0055fae6;
extern HashNameNode *data_0055fafa;
extern HashNameNode *data_0055fb0e;
extern HashNameNode *data_0055fb22;
extern HashNameNode *data_0055fb36;
extern HashNameNode *data_0055fb4a;
extern HashNameNode *data_0055fb5e;
extern HashNameNode *data_0055fb72;
extern HashNameNode *data_0055fb86;
extern HashNameNode *data_0055fb9a;
extern HashNameNode *data_0055fbae;
extern double data_0055fd18;
extern double double_four;
extern double data_0055fd28;
extern double data_0055fd30;
extern double data_0055fd38;
extern double data_0055fd40;
extern double data_0055fd48;
extern double data_0055fd50;
extern double data_0055fd58;
extern double data_0055fd60;
extern Boolean Type_RequiresMemoryReturn(Type *type);
extern const char *CMach_GetCPU(void);
extern Boolean CMach_PassResultInHiddenArg(Type *type);
extern Boolean CMachine_FunctionRequiresMemoryReturn(TypeFunc *functype);
extern TypeIntegral stshortdouble;

#ifdef __cplusplus
}
#endif

#endif
