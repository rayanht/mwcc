#include "compiler/common.h"
#include "compiler/CInt64.h"

CInt64 cint64_negone = {0xFFFFFFFF, 0xFFFFFFFF};
CInt64 cint64_zero = {0, 0};
CInt64 cint64_one = {0, 1};
CInt64 cint64_max = {0x7FFFFFFF, 0xFFFFFFFF};
CInt64 cint64_min = {0x80000000, 0};

#pragma options align = mac68k
static Boolean data_00580758;
static float data_0058075a;
static float data_0058075e[65];
#pragma options align = reset

CInt64 CInt64_Not(CInt64 input)
{
    CInt64 output;
    SInt32 c;

    c = (Boolean)(input.hi == 0 && input.lo == 0);
    output.lo = c;
    output.hi = c < 0 ? -1 : 0;
    return output;
}

CInt64 CInt64_Inv(CInt64 input)
{
    CInt64 output;
    output.hi = ~input.hi;
    output.lo = ~input.lo;
    return output;
}

CInt64 CInt64_Add(CInt64 lhs, CInt64 rhs)
{
    if (lhs.lo & 0x80000000) {
        if (rhs.lo & 0x80000000) {
            lhs.lo += rhs.lo;
            lhs.hi += 1;
        } else {
            lhs.lo += rhs.lo;
            if (!(lhs.lo & 0x80000000))
                lhs.hi += 1;
        }
    } else {
        if (rhs.lo & 0x80000000) {
            lhs.lo += rhs.lo;
            if (!(lhs.lo & 0x80000000))
                lhs.hi += 1;
        } else {
            lhs.lo += rhs.lo;
        }
    }
    lhs.hi += rhs.hi;
    return lhs;
}

CInt64 CInt64_Neg(CInt64 input)
{
    CInt64 result;
    result = CInt64_Add(CInt64_Inv(input), cint64_one);
    return result;
}

CInt64 CInt64_Sub(CInt64 lhs, CInt64 rhs)
{
    lhs = CInt64_Add(lhs, CInt64_Neg(rhs));
    return lhs;
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

CInt64 CInt64_Mul(CInt64 lhs, CInt64 rhs)
{
    if (CInt64_IsNegative(&rhs)) {
        if (CInt64_IsNegative(&lhs)) {
            return CInt64_MulU(CInt64_Neg(lhs), CInt64_Neg(rhs));
        }
        return CInt64_Neg(CInt64_MulU(lhs, CInt64_Neg(rhs)));
    }
    if (CInt64_IsNegative(&lhs)) {
        return CInt64_Neg(CInt64_MulU(CInt64_Neg(lhs), rhs));
    }
    return CInt64_MulU(lhs, rhs);
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

    bad = (rhs->hi == 0 && rhs->lo == 0);
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
            work = CInt64_Sub(work, *rhs);
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

CInt64 CInt64_Div(CInt64 lhs, CInt64 rhs)
{
    CInt64 result;
    if (CInt64_IsNegative(&rhs)) {
        rhs = CInt64_Neg(rhs);
        if (CInt64_IsNegative(&lhs)) {
            lhs = CInt64_Neg(lhs);
            CInt64_DivMod(&lhs, &rhs, &result, NULL);
            return result;
        } else {
            CInt64_DivMod(&lhs, &rhs, &result, NULL);
            return CInt64_Neg(result);
        }
    } else {
        if (CInt64_IsNegative(&lhs)) {
            lhs = CInt64_Neg(lhs);
            CInt64_DivMod(&lhs, &rhs, &result, NULL);
            return CInt64_Neg(result);
        } else {
            CInt64_DivMod(&lhs, &rhs, &result, NULL);
            return result;
        }
    }
}

CInt64 CInt64_DivU(CInt64 lhs, CInt64 rhs)
{
    CInt64 result;
    CInt64_DivMod(&lhs, &rhs, &result, NULL);
    return result;
}

CInt64 CInt64_Mod(CInt64 lhs, CInt64 rhs)
{
    CInt64 result;
    if (CInt64_IsNegative(&lhs)) {
        lhs = CInt64_Neg(lhs);
        if (CInt64_IsNegative(&rhs))
            rhs = CInt64_Neg(rhs);
        CInt64_DivMod(&lhs, &rhs, NULL, &result);
        return CInt64_Neg(result);
    } else {
        if (CInt64_IsNegative(&rhs))
            rhs = CInt64_Neg(rhs);
        CInt64_DivMod(&lhs, &rhs, NULL, &result);
        return result;
    }
}

CInt64 CInt64_ModU(CInt64 lhs, CInt64 rhs)
{
    CInt64 result;
    CInt64_DivMod(&lhs, &rhs, NULL, &result);
    return result;
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

int CInt64_UnsignedCompare(const CInt64 *lhs, const CInt64 *rhs)
{
    if (lhs->hi == rhs->hi) {
        if (lhs->lo < rhs->lo)
            return -1;
        if (lhs->lo > rhs->lo)
            return 1;
        return 0;
    }
    if ((UInt32)lhs->hi < (UInt32)rhs->hi)
        return -1;
    return 1;
}

int CInt64_SignedCompare(const CInt64 *lhs, const CInt64 *rhs)
{
    CInt64 lhs_;
    CInt64 rhs_;

    lhs_ = CInt64_Xor(*lhs, cint64_min);
    rhs_ = CInt64_Xor(*rhs, cint64_min);
    return CInt64_UnsignedCompare(&lhs_, &rhs_);
}

Boolean CInt64_Less(CInt64 lhs, CInt64 rhs)
{
    return CInt64_SignedCompare(&lhs, &rhs) < 0;
}

Boolean CInt64_LessU(CInt64 lhs, CInt64 rhs)
{
    return CInt64_UnsignedCompare(&lhs, &rhs) < 0;
}

Boolean CInt64_Greater(CInt64 lhs, CInt64 rhs)
{
    return CInt64_SignedCompare(&lhs, &rhs) > 0;
}

Boolean CInt64_GreaterU(CInt64 lhs, CInt64 rhs)
{
    return CInt64_UnsignedCompare(&lhs, &rhs) > 0;
}

Boolean CInt64_LessEqual(CInt64 lhs, CInt64 rhs)
{
    return CInt64_SignedCompare(&lhs, &rhs) <= 0;
}

Boolean CInt64_LessEqualU(CInt64 lhs, CInt64 rhs)
{
    return CInt64_UnsignedCompare(&lhs, &rhs) <= 0;
}

Boolean CInt64_GreaterEqual(CInt64 lhs, CInt64 rhs)
{
    return CInt64_SignedCompare(&lhs, &rhs) >= 0;
}

Boolean CInt64_GreaterEqualU(CInt64 lhs, CInt64 rhs)
{
    return CInt64_UnsignedCompare(&lhs, &rhs) >= 0;
}

Boolean CInt64_Equal(CInt64 lhs, CInt64 rhs)
{
    return lhs.hi == rhs.hi && lhs.lo == rhs.lo;
}

Boolean CInt64_NotEqual(CInt64 lhs, CInt64 rhs)
{
    return lhs.hi != rhs.hi || lhs.lo != rhs.lo;
}

Boolean CInt64_IsInRange(CInt64 value, short len)
{
    CInt64 bound;

    if (CInt64_IsNegative(&value)) {
        switch (len) {
            case 1:
                bound.lo = 0xFFFFFF80;
                bound.hi = 0xFFFFFFFF;
                break;
            case 2:
                bound.lo = 0xFFFF8000;
                bound.hi = 0xFFFFFFFF;
                break;
            case 4:
                bound.lo = 0x80000000;
                bound.hi = 0xFFFFFFFF;
                break;
            case 8:
                return 1;
            default:
                return 0;
        }
        return CInt64_GreaterEqual(value, bound);
    } else {
        switch (len) {
            case 1:
                bound.lo = 0x7F;
                bound.hi = 0;
                break;
            case 2:
                bound.lo = 0x7FFF;
                bound.hi = 0;
                break;
            case 4:
                bound.lo = 0x7FFFFFFF;
                bound.hi = 0;
                break;
            case 8:
                return 1;
            default:
                return 0;
        }
        return CInt64_LessEqual(value, bound);
    }
}

Boolean CInt64_IsInURange(CInt64 value, short len)
{
    SInt32 hi = value.hi;
    UInt32 lo = value.lo;

    switch (len) {
        case 1:
            return hi == 0 && (lo & 0xFFFFFF00) == 0;
        case 2:
            return hi == 0 && (lo & 0xFFFF0000) == 0;
        case 4:
            return hi == 0;
        case 8:
            return 1;
        default:
            return 0;
    }
}

CInt64 CInt64_And(CInt64 lhs, CInt64 rhs)
{
    lhs.hi &= rhs.hi;
    lhs.lo &= rhs.lo;
    return lhs;
}

CInt64 CInt64_Xor(CInt64 lhs, CInt64 rhs)
{
    lhs.hi ^= rhs.hi;
    lhs.lo ^= rhs.lo;
    return lhs;
}

CInt64 CInt64_Or(CInt64 lhs, CInt64 rhs)
{
    lhs.hi |= rhs.hi;
    lhs.lo |= rhs.lo;
    return lhs;
}

void CInt64_ConvertInt32(CInt64 *i)
{
    i->hi = (i->lo & 0x80000000) ? -1 : 0;
}

void CInt64_ConvertUInt32(CInt64 *i)
{
    CInt64_SetULong(i, (UInt32)i->lo);
}

void CInt64_ConvertInt16(CInt64 *i)
{
    i->lo = (SInt16)i->lo;
    i->hi = (i->lo & 0x80000000) ? -1 : 0;
}

void CInt64_ConvertUInt16(CInt64 *i)
{
    CInt64_SetULong(i, (UInt16)i->lo);
}

void CInt64_ConvertInt8(CInt64 *i)
{
    i->lo = (SInt8)i->lo;
    i->hi = (i->lo & 0x80000000) ? -1 : 0;
}

void CInt64_ConvertUInt8(CInt64 *i)
{
    CInt64_SetULong(i, (UInt8)i->lo);
}

static float CInt64_PowerOfTwo(short n)
{
    int i;

    if (!data_00580758) {
        data_0058075a = 1.0f;
        i = 0;
        do {
            data_0058075e[i] = data_0058075a;
            data_0058075a += data_0058075a;
        } while (++i < 65);
        data_00580758 = 1;
    }
    return data_0058075e[n];
}

void CInt64_ConvertUFromLongDouble(CInt64 *pResult, double value)
{
    UInt32 a, b;
    int bits;
    float threshold;

    if (value <= 0.0) {
        pResult->hi = 0;
        pResult->lo = 0;
        return;
    }
    if (value >= CInt64_PowerOfTwo(64)) {
        pResult->hi = 0xFFFFFFFF;
        pResult->lo = 0xFFFFFFFF;
        return;
    }
    a = b = 0;
    for (bits = 63; bits >= 0; bits--) {
        a <<= 1;
        if (b & 0x80000000)
            a |= 1;
        b <<= 1;
        if ((threshold = CInt64_PowerOfTwo(bits)) <= value) {
            b |= 1;
            value -= threshold;
        }
    }
    pResult->hi = a;
    pResult->lo = b;
}

void CInt64_ConvertFromLongDouble(CInt64 *pResult, double value)
{
    if (value < 0.0) {
        CInt64_ConvertUFromLongDouble(pResult, -value);
        *pResult = CInt64_Neg(*pResult);
    } else {
        CInt64_ConvertUFromLongDouble(pResult, value);
    }
}

double CInt64_ConvertUToLongDouble(const CInt64 *value)
{
    Boolean bad;
    SInt32 work;
    int counter;
    double result;

    bad = (value->hi == 0 && value->lo == 0);
    if (bad)
        return 0.0;

    result = 0.0;
    work = value->hi;
    if (work != 0) {
        for (counter = 0; counter < 32; counter++) {
            result += result;
            if (work & 0x80000000)
                result += 1.0;
            work <<= 1;
        }
    }

    work = value->lo;
    for (counter = 0; counter < 32; counter++) {
        result += result;
        if (work & 0x80000000)
            result = result + 1.0L;
        work <<= 1;
    }

    return result;
}

double CInt64_ConvertToLongDouble(const CInt64 *value)
{
    CInt64 tmp;
    if (CInt64_IsNegative(value)) {
        tmp = CInt64_Neg(*value);
        return -CInt64_ConvertUToLongDouble(&tmp);
    } else {
        return CInt64_ConvertUToLongDouble(value);
    }
}

char *CInt64_ScanOctString(CInt64 *val, char *s, Boolean *overflow)
{
    char c;
    UInt32 hi;
    UInt32 lo;
    SInt32 digit;
    CInt64 d;
    *overflow = 0;
    val->lo = 0;
    val->hi = 0;
    while (*s >= '0' && *s <= '7') {
        c = *s;
        hi = val->hi;
        lo = val->lo;
        if (hi & 0xE0000000)
            *overflow = 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        val->hi = hi;
        val->lo = lo;
        digit = c - '0';
        d.lo = digit;
        d.hi = digit < 0 ? -1 : 0;
        *val = CInt64_Add(*val, d);
        s++;
    }
    return s;
}

char *CInt64_ScanDecString(CInt64 *v, char *s, Boolean *ovf)
{
    CInt64 t;
    char c;
    SInt32 hi;
    UInt32 lo;
    *ovf = 0;
    v->lo = 0;
    v->hi = 0;
    while ((c = *s) >= '0' && c <= '9') {
        hi = v->hi;
        lo = v->lo;
        if (hi & 0xe0000000)
            *ovf = 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        t.hi = hi;
        t.lo = lo;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        v->hi = hi;
        v->lo = lo;
        if (CInt64_IsNegative(v)) {
            *v = CInt64_Add(*v, t);
            if (!CInt64_IsNegative(v))
                *ovf = 1;
        } else {
            *v = CInt64_Add(*v, t);
        }
        t.lo = c - '0';
        t.hi = c - '0' < 0 ? -1 : 0;
        if (CInt64_IsNegative(v)) {
            *v = CInt64_Add(*v, t);
            if (!CInt64_IsNegative(v))
                *ovf = 1;
        } else {
            *v = CInt64_Add(*v, t);
        }
        s++;
    }
    return s;
}

char *CInt64_ScanHexString(CInt64 *value, char *p, Boolean *overflow)
{
    SInt8 digit;
    SInt32 n;
    UInt32 hi, lo;
    CInt64 d;

    *overflow = 0;
    value->lo = 0;
    value->hi = 0;
    for (;;) {
        digit = *p;
        if (digit >= '0' && digit <= '9')
            digit -= '0';
        else if (digit >= 'A' && digit <= 'F')
            digit -= 'A' - 10;
        else if (digit >= 'a' && digit <= 'f')
            digit -= 'a' - 10;
        else
            break;

        hi = value->hi;
        p++;
        lo = value->lo;
        if (hi & 0xf0000000)
            *overflow = 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        hi <<= 1;
        if (lo & 0x80000000)
            hi |= 1;
        lo <<= 1;
        value->hi = hi;
        value->lo = lo;

        n = digit;
        d.lo = n;
        d.hi = n < 0 ? -1 : 0;
        *value = CInt64_Add(*value, d);
    }
    return p;
}

char *CInt64_ScanBinString(CInt64 *value, char *digits, Boolean *overflow)
{
    UInt32 low;
    Boolean bit;
    UInt32 high;
    char digit;
    *overflow = 0;
    value->lo = 0;
    value->hi = 0;
    do {
        digit = *digits;
        if (digit == '0')
            bit = FALSE;
        else if (digit == '1')
            bit = TRUE;
        else
            break;
        high = value->hi;
        ++digits;
        low = value->lo;
        if ((high & 0x80000000) != 0)
            *overflow = 1;
        high = high << 1;
        if ((low & 0x80000000) != 0)
            high |= 1;
        value->hi = high;
        value->lo = low << 1;
        if (bit == TRUE)
            *value = CInt64_Add(*value, cint64_one);
    } while (TRUE);
    return digits;
}

/* The linker stripped the function that used these literals; they stay in the unit's .data. */
static void CInt64_ScanAsmNumberLiterals(const char **literals)
{
    literals[0] = "01";
    literals[1] = "bB";
    literals[2] = "234567";
    literals[3] = "89";
    literals[4] = "acdefACEDF";
}

int CInt64_PrintDec(char *output, CInt64 num)
{
    int length;
    CInt64 rem;
    CInt64 divisor;
    char buf[32];
    char *bufp;

    length = 0;
    if (CInt64_IsNegative(&num)) {
        num = CInt64_Neg(num);
        *output = '-';
        output++;
        length++;
    }

    if (!CInt64_IsZero(&num)) {
        divisor.lo = 10;
        divisor.hi = 0;

        bufp = buf;
        for (;;) {
            rem = CInt64_ModU(num, divisor);
            *(bufp++) = rem.lo + '0';
            num = CInt64_DivU(num, divisor);
            if (CInt64_IsZero(&num) != 0)
                break;
        }

        while (--bufp >= buf) {
            *(output++) = *bufp;
            length++;
        }
    } else {
        *(output++) = '0';
        length++;
    }

    *output = 0;
    return length;
}
