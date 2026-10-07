#define CERROR_FILE "IroBitVect.c"
#include "compiler/common.h"
#include "compiler/IroBitVect.h"
#include "compiler/CError.h"
#include "compiler/CompilerTools.h"
#include <string.h>

unsigned int IroBitVect_AllocateBitVector(BitVector **vector, unsigned int bitCount)
{
    bitCount = (bitCount >> 5) + 1U;
    *vector = oalloc(bitCount * sizeof((*vector)->bits[0]) + sizeof((*vector)->size));
    (*vector)->size = bitCount;
    memset((*vector)->bits, 0, (*vector)->size * sizeof((*vector)->bits[0]));
}

void IroBitVect_ClearBit(UInt32 bit, BitVector *vector)
{
    if (bit >> 5 < vector->size) {
        vector->bits[bit >> 5] &= ~(1U << (bit & 0x1f));
    } else {
        CError_FATAL(64);
    }
}

void IroBitVect_Intersect(BitVector *source, BitVector *destination)
{
    unsigned int index;

    for (index = 0; index < destination->size; index = index + 1) {
        destination->bits[index] &= source->bits[index];
    }
}

void IroBitVect_Or(BitVector *sourceData, BitVector *destinationData)
{
    UInt32 *source = &sourceData->size;
    UInt32 *destination = &destinationData->size;
    UInt32 index;
    UInt32 count;

    count = *source;
    if (*destination < count) {
        count = *destination;
    }
    for (index = 0; index < count; index = index + 1) {
        destination[index + 1] |= source[index + 1];
    }
}

unsigned int IroBitVect_Intersects(BitVector *left, BitVector *right)
{
    unsigned int wordCount;
    unsigned int wordIndex;

    wordCount = left->size;
    if (right->size < left->size) {
        wordCount = right->size;
    }
    for (wordIndex = 0; wordIndex < wordCount; wordIndex++) {
        if ((left->bits[wordIndex] & right->bits[wordIndex]) != 0) {
            return 1;
        }
    }
    return 0;
}

int IroBitVect_AreEqual(BitVector *a, BitVector *b)
{
    unsigned int i;

    for (i = 0; i < a->size; i++) {
        if (a->bits[i] != b->bits[i])
            return 0;
    }
    return 1;
}

void IroBitVect_Subtract(BitVector *source, BitVector *destination)
{
    unsigned int i;

    for (i = 0; i < destination->size; i++) {
        destination->bits[i] &= ~source->bits[i];
    }
}

void IroBitVect_CopyBitVector(BitVector *source, BitVector *destination)
{
    memcpy(destination->bits, source->bits, destination->size << 2);
}

void IroBitVect_ClearBitVector(BitVector *vector)
{
    unsigned int word_count;
    word_count = vector->size;
    memset(vector->bits, 0, word_count << 2U);
}

void IroBitVect_SetAllBits(BitVector *array)
{
    memset(array->bits, 255, array->size * sizeof(unsigned int));
}

int IroBitVect_ContainsSubset(BitVector *subset, BitVector *superset)
{
    unsigned int wordIndex;

    for (wordIndex = 0; wordIndex < subset->size; wordIndex = wordIndex + 1) {
        if (superset->size < wordIndex) {
            if (subset->bits[wordIndex] != 0) {
                return 0;
            }
        } else if ((~superset->bits[wordIndex] & subset->bits[wordIndex]) != 0) {
            return 0;
        }
    }
    return 1;
}

unsigned int is_bitvector_empty(struct BitVector *values)
{
    unsigned int index;

    for (index = 0; index < values->size; ++index) {
        if (values->bits[index] != 0) {
            return 0;
        }
    }
    return 1;
}
