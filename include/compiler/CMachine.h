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
extern void initialize_hash_name_globals(void);
extern Float CMach_FloatReciprocal(Float a0);
extern Boolean CMach_FloatIsPowerOf2(Float f);
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
