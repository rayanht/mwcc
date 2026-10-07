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

extern int CExpr2_FormatCInt64Decimal(char *output, CInt64 num);
extern char *CExpr2_ParseHexInt64(CInt64 *value, char *p, Boolean *overflow);
extern UInt8 *CExpr2_ParseDecimalCInt64(CInt64 *v, char *s, Boolean *ovf);
extern char *CExpr2_ParseOctalInt64(CInt64 *val, char *s, Boolean *overflow);
extern double CExpr2_ConvertCInt64ToDouble(CInt64 *val);
extern void CExpr2_ConvertDoubleToCInt64(CInt64 *p, double x);
extern void CExpr2_ConvertDoubleToUnsignedCInt64(CInt64 *result, double value);
extern void CExpr2_ConvertCInt64ToUInt8(CInt64 *value);
extern void CExpr2_SignExtendSignedChar(CInt64 *value);
extern void CExpr2_ConvertCInt64ToUnsignedShort(CInt64 *value);
extern void CExpr2_SignExtendShort(register CInt64 *value);
extern void CExpr2_ClearCInt64Hi(CInt64 *a0);
extern int CExpr2_SignExtendCInt64(CInt64 *value);
extern CInt64 CExpr2_BitwiseOrCInt64(CInt64 left, CInt64 right);
extern char *parse_binary_digits(CInt64 *value, char *digits, unsigned char *overflow);
extern double CExpr2_ConvertUnsignedCInt64ToDouble(CInt64 *v);
extern CInt64 xor_64(CInt64 a0, CInt64 a1);
extern CInt64 CFunc_BitwiseNot(CInt64 input);
extern CInt64 CFunc_LogicalNotCInt64(CInt64 input);
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
extern CInt64 CInt64_Inv(CInt64 x);
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
