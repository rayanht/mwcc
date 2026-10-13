#ifndef COMPILER_BITVECTORS_H
#define COMPILER_BITVECTORS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void CodeMotion_AllocateBits(UInt32 *destination, const UInt32 *source, SInt32 bit_count);
extern SInt32 BitVectors_CopyAndCheckChanged(UInt32 *destination, UInt32 *source, SInt32 bitCount);
extern void BitVectors_FillWords(UInt32 *words, int bitCount, unsigned int value);
extern void BitVectors_IntersectBitVectors(UInt32 *dst, UInt32 *src, SInt32 nbits);
extern void BitVectors_OrBitVector(UInt32 *destination, const UInt32 *source, unsigned int bitCount);

#ifdef __cplusplus
}
#endif

#endif
