#ifndef COMPILER_BITVECTOR_H
#define COMPILER_BITVECTOR_H

#include "compiler/common.h"
#include "compiler/CError.h"
#include "compiler/IroBitVect.h"

inline void IroBitVect_SetBit(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

#endif
