#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CInt64.h"
#include "compiler/IroLoop.h"
#include "compiler/IroRangePropagation.h"

static float lbl_00555498 = 1.0f;

static CInt64 mask_xor(CInt64 v, CInt64 m)
{
    v.hi ^= m.hi;
    v.lo ^= m.lo;
    return v;
}

static inline CInt64 xorWords(CInt64 value, CInt64 mask)
{
    value.hi ^= mask.hi;
    value.lo ^= mask.lo;
    return value;
}

static CInt64 xor64(CInt64 p, CInt64 q)
{
    p.hi ^= q.hi;
    p.lo ^= q.lo;
    return p;
}

static inline SInt32 compare_u64(UInt32 a_hi, UInt32 a_lo, UInt32 b_hi, UInt32 b_lo)
{
    if (a_hi == b_hi) {
        if (a_lo < b_lo)
            return -1;
        if (a_lo <= b_lo)
            return 0;
    } else if (a_hi < b_hi) {
        return -1;
    }
    return 1;
}

static CInt64 xor_key(CInt64 a, CInt64 b)
{
    a.hi ^= b.hi;
    a.lo ^= b.lo;
    return a;
}

static CInt64 unmask(CInt64 v, CInt64 k)
{
    v.hi ^= k.hi;
    v.lo ^= k.lo;
    return v;
}

CInt64 CInt64_And(CInt64 left, CInt64 right)
{
    left.hi &= right.hi;
    left.lo &= right.lo;
    return left;
}

Boolean CInt64_IsInURange(CInt64 n, SInt16 kind)
{
    SInt32 hi = n.hi;
    UInt32 lo = n.lo;
    switch (kind) {
        case 1:
            return hi == 0 && (lo & 0xffffff00) == 0;
        case 2:
            return hi == 0 && (lo & 0xffff0000) == 0;
        case 4:
            return hi == 0;
        case 8:
            return 1;
        default:
            return 0;
    }
}

unsigned char CInt64_IsInRange(CInt64 value, short byteSize)
{
    CInt64 bound;
    if ((char)((value.hi & (-2147483647 - 1)) != 0) != 0) {
        CInt64 biasedBound, biasedValue;
        switch (byteSize) {
            case 1:
                bound.lo = -128;
                bound.hi = -1;
                break;
            case 2:
                bound.lo = -32768;
                bound.hi = -1;
                break;
            case 4:
                bound.lo = (-2147483647 - 1);
                bound.hi = -1;
                break;
            case 8:
                return 1;
            default:
                return 0;
        }
        biasedValue = xorWords(value, cint64_min);
        biasedBound = xorWords(bound, cint64_min);
        return CInt64_UnsignedCompare(&biasedValue, &biasedBound) >= 0;
    }
    {
        CInt64 biasedBound, biasedValue;
        switch (byteSize) {
            case 1:
                bound.lo = 127;
                bound.hi = 0;
                break;
            case 2:
                bound.lo = 32767;
                bound.hi = 0;
                break;
            case 4:
                bound.lo = 2147483647;
                bound.hi = 0;
                break;
            case 8:
                return 1;
            default:
                return 0;
        }
        biasedValue = xorWords(value, cint64_min);
        biasedBound = xorWords(bound, cint64_min);
        return CInt64_UnsignedCompare(&biasedValue, &biasedBound) <= 0;
    }
}

Boolean CInt64_NotEqual(CInt64 a, CInt64 b)
{
    return a.hi != b.hi || a.lo != b.lo;
}

unsigned char CInt64_Equal(CInt64 left, CInt64 right)
{
    return left.hi == right.hi && left.lo == right.lo;
}

Boolean CInt64_GreaterEqualU(CInt64 a, CInt64 b)
{
    SInt32 cmp;
    do {
        if (a.hi == b.hi) {
            if (a.lo < b.lo) {
                cmp = -1;
                break;
            }
            if (a.lo <= b.lo) {
                cmp = 0;
                break;
            }
        } else if ((UInt32)a.hi < (UInt32)b.hi) {
            cmp = -1;
            break;
        }
        cmp = 1;
    } while (0);
    return cmp >= 0;
}

Boolean CInt64_GreaterEqual(CInt64 a, CInt64 b)
{
    CInt64 y, x;
    x = xor64(a, cint64_min);
    y = xor64(b, cint64_min);
    return CInt64_UnsignedCompare(&x, &y) >= 0;
}

Boolean CInt64_LessEqualU(CInt64 a, CInt64 b)
{
    return compare_u64(a.hi, a.lo, b.hi, b.lo) <= 0;
}

Boolean CInt64_LessEqual(CInt64 a, CInt64 b)
{
    CInt64 y, x;
    x = xor64(a, cint64_min);
    y = xor64(b, cint64_min);
    return CInt64_UnsignedCompare(&x, &y) <= 0;
}

Boolean CInt64_GreaterU(CInt64 x, CInt64 y)
{
    int comparison;

    if (x.hi == y.hi) {
        if (x.lo < y.lo)
            comparison = -1;
        else if (x.lo > y.lo)
            comparison = 1;
        else
            comparison = 0;
    } else if ((UInt32)x.hi < (UInt32)y.hi)
        comparison = -1;
    else
        comparison = 1;

    return comparison > 0;
}

Boolean CInt64_Greater(CInt64 a, CInt64 b)
{
    CInt64 y;
    CInt64 x;

    x = xor_key(a, cint64_min);
    y = xor_key(b, cint64_min);
    {
        CInt64 *xp = &x;
        CInt64 *yp = &y;
        return CInt64_UnsignedCompare(xp, yp) > 0;
    }
}

Boolean CInt64_LessU(CInt64 a, CInt64 b)
{
    SInt32 comparison;

    if (a.hi == b.hi) {
        if (a.lo < b.lo)
            comparison = -1;
        else if (a.lo > b.lo)
            comparison = 1;
        else
            comparison = 0;
    } else {
        if ((UInt32)a.hi < (UInt32)b.hi)
            comparison = -1;
        else
            comparison = 1;
    }
    return comparison < 0;
}

Boolean CInt64_Less(CInt64 a, CInt64 b)
{
    CInt64 y;
    CInt64 x;
    x = unmask(a, cint64_min);
    y = unmask(b, cint64_min);
    return CInt64_UnsignedCompare(&x, &y) < 0;
}

SInt32 CInt64_UnsignedCompare(CInt64 *a, CInt64 *b)
{
    if (a->hi == b->hi) {
        if (a->lo < b->lo)
            return -1;
        if (a->lo > b->lo)
            return 1;
        return 0;
    }
    if ((UInt32)a->hi < (UInt32)b->hi)
        return -1;
    return 1;
}

CInt64 CInt64_ShrU(CInt64 value, CInt64 count)
{
    UInt32 hi, lo, i;

    if (count.hi == 0 && count.lo < 64) {
        hi = (UInt32)value.hi;
        lo = value.lo;
        i = count.lo;
        while (i != 0) {
            lo >>= 1;
            if (hi & 1)
                lo |= 0x80000000;
            hi >>= 1;
            i--;
        }
        value.hi = (SInt32)hi;
        value.lo = lo;
    } else {
        value.hi = 0;
        value.lo = 0;
    }
    return value;
}

CInt64 CInt64_Shr(CInt64 value, CInt64 count)
{
    if (count.hi == 0 && count.lo < 64) {
        UInt32 lo;
        SInt32 hi;
        UInt32 n;
        hi = value.hi;
        lo = value.lo;
        n = count.lo;
        while (n != 0) {
            lo = lo >> 1;
            if (hi & 1)
                lo = lo | 0x80000000;
            hi = hi >> 1;
            n--;
        }
        value.hi = hi;
        value.lo = lo;
    } else {
        Boolean neg = (value.hi & 0x80000000) != 0;
        if (neg) {
            value.hi = -1;
            value.lo = 0xffffffff;
        } else {
            value.hi = 0;
            value.lo = 0;
        }
    }
    return value;
}

CInt64 CInt64_Shl(CInt64 v, CInt64 count)
{
    SInt32 hi;
    UInt32 lo;
    UInt32 n;
    if (count.hi == 0 && count.lo < 0x40) {
        hi = v.hi;
        lo = v.lo;
        n = count.lo;
        while (n != 0) {
            hi <<= 1;
            if (lo & 0x80000000) {
                hi |= 1;
            }
            lo <<= 1;
            n--;
        }
        v.hi = hi;
        v.lo = lo;
    } else {
        v.hi = 0;
        v.lo = 0;
    }
    return v;
}

CInt64 CInt64_ModU(CInt64 a, CInt64 b)
{
    CInt64 result;
    CInt64_DivMod(&a, &b, NULL, &result);
    return result;
}

static Boolean CInt64_IsNeg(SInt32 hi)
{
    return (hi & 0x80000000) != 0;
}

static CInt64 recovered_complement_CInt64_Neg(CInt64 x)
{
    CInt64 y;
    y.hi = ~x.hi;
    y.lo = ~x.lo;
    return y;
}

static CInt64 CInt64_Neg(CInt64 x)
{
    CInt64 recovered_result;
    recovered_result = CInt64_Add(recovered_complement_CInt64_Neg(x), cint64_one);
    return recovered_result;
}

CInt64 CInt64_Mod(CInt64 a, CInt64 b)
{
    CInt64 r;
    if (CInt64_IsNeg(a.hi)) {
        a = CInt64_Neg(a);
        if (CInt64_IsNeg(b.hi))
            b = CInt64_Neg(b);
        CInt64_DivMod(&a, &b, NULL, &r);
        return CInt64_Neg(r);
    }
    if (CInt64_IsNeg(b.hi))
        b = CInt64_Neg(b);
    CInt64_DivMod(&a, &b, NULL, &r);
    return r;
}

CInt64 CInt64_DivU(CInt64 a, CInt64 b)
{
    CInt64 result;
    CInt64_DivMod(&a, &b, &result, NULL);
    return result;
}

static CInt64 recovered_complement_Neg(CInt64 x)
{
    CInt64 y;
    y.hi = ~x.hi;
    y.lo = ~x.lo;
    return y;
}

static CInt64 Neg(CInt64 x)
{
    CInt64 r;
    r = CInt64_Add(recovered_complement_Neg(x), cint64_one);
    return r;
}

CInt64 CInt64_Div(CInt64 dividend, CInt64 divisor)
{
    CInt64 quotient;
    Boolean divisorNegative, dividendNegative;

    divisorNegative = (divisor.hi & 0x80000000) != 0;
    if (divisorNegative) {
        divisor = Neg(divisor);
        dividendNegative = (dividend.hi & 0x80000000) != 0;
        if (dividendNegative) {
            dividend = Neg(dividend);
            CInt64_DivMod(&dividend, &divisor, &quotient, NULL);
            return quotient;
        }
        CInt64_DivMod(&dividend, &divisor, &quotient, NULL);
        return Neg(quotient);
    } else {
        dividendNegative = (dividend.hi & 0x80000000) != 0;
        if (dividendNegative) {
            dividend = Neg(dividend);
            CInt64_DivMod(&dividend, &divisor, &quotient, NULL);
            return Neg(quotient);
        }
        CInt64_DivMod(&dividend, &divisor, &quotient, NULL);
        return quotient;
    }
}

static Boolean iszero(const CInt64 *n)
{
    return n->hi == 0 && n->lo == 0;
}

static CInt64 inv(CInt64 input)
{
    CInt64 output;
    output.hi = ~input.hi;
    output.lo = ~input.lo;
    return output;
}

static CInt64 neg(CInt64 input)
{
    CInt64 result;
    result = CInt64_Add(inv(input), cint64_one);
    return result;
}

static CInt64 sub(CInt64 lhs, CInt64 rhs)
{
    lhs = CInt64_Add(lhs, neg(rhs));
    return lhs;
}

void CInt64_DivMod(const CInt64 *lhs, const CInt64 *rhs, CInt64 *pDiv, CInt64 *pMod)
{
    UInt32 workF;
    Boolean bad;
    UInt32 workA;
    UInt32 workB;
    UInt32 workC;
    UInt32 workD;
    UInt32 workE;
    UInt32 workG;
    UInt32 workH;
    int counter;
    CInt64 work;

    bad = iszero(rhs);
    if (bad)
        return;

    workF = 0;
    workB = lhs->hi;
    workA = lhs->lo;
    workC = rhs->hi;
    workE = 0;
    workD = rhs->lo;
    workG = 0;
    workH = 0;
    for (counter = 0; counter < 64; counter++) {
        workF <<= 1;
        if (workE & 0x80000000)
            workF |= 1;
        workE <<= 1;
        if (workB & 0x80000000)
            workE |= 1;
        workB <<= 1;
        if (workA & 0x80000000)
            workB |= 1;
        workA <<= 1;
        workG <<= 1;
        if (workH & 0x80000000)
            workG |= 1;
        workH <<= 1;
        if (workF > workC || (workF == workC && workE >= workD)) {
            workH |= 1;
            work.hi = workF;
            work.lo = workE;
            work = sub(work, *rhs);
            workF = work.hi;
            workE = work.lo;
        }
    }
    if (pDiv) {
        pDiv->hi = workG;
        pDiv->lo = workH;
    }
    if (pMod) {
        pMod->hi = workF;
        pMod->lo = workE;
    }
}

static Boolean isneg(CInt64 x)
{
    return (x.hi & 0x80000000) != 0;
}

static CInt64 recovered_complement_lneg(CInt64 x)
{
    CInt64 nx;
    nx.hi = ~x.hi;
    nx.lo = ~x.lo;
    return nx;
}

static CInt64 lneg(CInt64 x)
{
    CInt64 recovered_result;
    recovered_result = CInt64_Add(recovered_complement_lneg(x), cint64_one);
    return recovered_result;
}

CInt64 CInt64_Mul(CInt64 a, CInt64 b)
{
    if (isneg(b)) {
        if (isneg(a)) {
            return CInt64_MulU(lneg(a), lneg(b));
        } else {
            return lneg(CInt64_MulU(a, lneg(b)));
        }
    } else {
        if (isneg(a)) {
            return lneg(CInt64_MulU(lneg(a), b));
        } else {
            return CInt64_MulU(a, b);
        }
    }
}

CInt64 CInt64_MulU(CInt64 lhs, CInt64 rhs)
{
    CInt64 result;
    CInt64 work1;
    UInt32 aaaa;
    UInt32 bbbb;
    UInt32 cccc;
    UInt32 dddd;
    UInt32 eeee;

    eeee = dddd = rhs.lo;
    result.lo = 0;
    result.hi = 0;
    bbbb = lhs.lo;
    cccc = rhs.hi;
    while (bbbb != 0) {
        if (bbbb & 1) {
            work1.hi = cccc;
            work1.lo = dddd;
            result = CInt64_Add(result, work1);
        }
        cccc <<= 1;
        if (dddd & 0x80000000)
            cccc |= 1;
        bbbb >>= 1;
        dddd <<= 1;
    }
    aaaa = lhs.hi;
    while (aaaa != 0 && eeee != 0) {
        if (aaaa & 1)
            result.hi += eeee;
        eeee <<= 1;
        aaaa >>= 1;
    }
    return result;
}

static CInt64 not64(CInt64 v)
{
    CInt64 t;
    t.hi = ~v.hi;
    t.lo = ~v.lo;
    return t;
}

CInt64 CInt64_Sub(CInt64 lhs, CInt64 rhs)
{
    CInt64 t;
    t = CInt64_Add(inv(rhs), cint64_one);
    lhs = CInt64_Add(lhs, t);
    return lhs;
}

CInt64 CInt64_Inv(CInt64 x)
{
    CInt64 r;
    r = CInt64_Add(not64(x), cint64_one);
    return r;
}

CInt64 CInt64_Add(CInt64 a, CInt64 b)
{
    if (a.lo & 0x80000000) {
        if (b.lo & 0x80000000) {
            a.lo += b.lo;
            a.hi += 1;
        } else {
            a.lo += b.lo;
            if (!(a.lo & 0x80000000))
                a.hi += 1;
        }
    } else {
        if (b.lo & 0x80000000) {
            a.lo += b.lo;
            if (!(a.lo & 0x80000000))
                a.hi += 1;
        } else {
            a.lo += b.lo;
        }
    }
    a.hi += b.hi;
    return a;
}
