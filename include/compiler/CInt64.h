#ifndef COMPILER_CINT64_H
#define COMPILER_CINT64_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CInt64 cint64_negone;
extern CInt64 cint64_zero;
extern CInt64 cint64_one;
extern CInt64 cint64_max;
extern CInt64 cint64_min;

extern int CInt64_PrintDec(char *output, CInt64 num);
extern char *CInt64_ScanHexString(CInt64 *value, char *p, Boolean *overflow);
extern UInt8 *CInt64_ScanDecString(CInt64 *v, char *s, Boolean *ovf);
extern char *CInt64_ScanOctString(CInt64 *val, char *s, Boolean *overflow);
extern double CInt64_ConvertToLongDouble(CInt64 *val);
extern void CInt64_ConvertFromLongDouble(CInt64 *p, double x);
extern void CInt64_ConvertUFromLongDouble(CInt64 *result, double value);
extern void CInt64_ConvertUInt8(CInt64 *value);
extern void CInt64_ConvertInt8(CInt64 *value);
extern void CInt64_ConvertUInt16(CInt64 *value);
extern void CInt64_ConvertInt16(register CInt64 *value);
extern void CInt64_ConvertUInt32(CInt64 *a0);
extern int CInt64_ConvertInt32(CInt64 *value);
extern CInt64 CInt64_Or(CInt64 left, CInt64 right);
extern char *CInt64_ScanBinString(CInt64 *value, char *digits, unsigned char *overflow);
extern double CInt64_ConvertUToLongDouble(CInt64 *v);
extern CInt64 CInt64_Xor(CInt64 a0, CInt64 a1);
extern CInt64 CInt64_Inv(CInt64 input);
extern CInt64 CInt64_Not(CInt64 input);
extern CInt64 CInt64_ShrU(CInt64 value, CInt64 count);
extern CInt64 CInt64_Shr(CInt64 value, CInt64 count);
extern CInt64 CInt64_Shl(CInt64 v, CInt64 count);
extern CInt64 CInt64_Add(CInt64 a, CInt64 b);
extern CInt64 CInt64_ModU(CInt64 a, CInt64 b);
extern CInt64 CInt64_Mod(CInt64 a, CInt64 b);
extern CInt64 CInt64_DivU(CInt64 a, CInt64 b);
extern CInt64 CInt64_Div(CInt64 a, CInt64 b);
extern void CInt64_DivMod(const CInt64 *lhs, const CInt64 *rhs, CInt64 *pDiv, CInt64 *pMod);
extern CInt64 CInt64_Mul(CInt64 a, CInt64 b);
extern CInt64 CInt64_MulU(CInt64 lhs, CInt64 rhs);
extern CInt64 CInt64_Sub(CInt64 lhs, CInt64 rhs);
extern CInt64 CInt64_Neg(CInt64 x);
extern CInt64 CInt64_And(CInt64 a0, CInt64 a1);
extern Boolean CInt64_IsInURange(CInt64 n, SInt16 kind);
extern Boolean CInt64_NotEqual(CInt64 a, CInt64 b);
extern unsigned char CInt64_Equal(CInt64 left, CInt64 right);
extern Boolean CInt64_GreaterEqualU(CInt64 a, CInt64 b);
extern Boolean CInt64_LessEqualU(CInt64 a, CInt64 b);
extern Boolean CInt64_GreaterU(CInt64 x, CInt64 y);
extern Boolean CInt64_LessU(CInt64 a, CInt64 b);
extern SInt32 CInt64_UnsignedCompare(CInt64 *a, CInt64 *b);
extern unsigned char CInt64_IsInRange(CInt64 value, short byteSize);
extern Boolean CInt64_GreaterEqual(CInt64 a, CInt64 b);
extern Boolean CInt64_LessEqual(CInt64 a, CInt64 b);
extern Boolean CInt64_Greater(CInt64 a, CInt64 b);
extern Boolean CInt64_Less(CInt64 a, CInt64 b);

#ifdef __cplusplus
}
#endif

#endif
