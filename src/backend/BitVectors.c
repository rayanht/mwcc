#include "compiler/common.h"
#include "compiler/BitVectors.h"

/* Copies bit_count bits rounded up to a whole word count. */

void CodeMotion_AllocateBits(UInt32 *destination, const UInt32 *source, SInt32 bit_count)
{
    int count = (bit_count + 31) >> 5;

    while (count != 0) {
        *destination++ = *source++;
        count--;
    }
}

SInt32 BitVectors_CopyAndCheckChanged(UInt32 *destination, UInt32 *source, SInt32 bitCount)
{
    unsigned int changed;
    const UInt32 *nextSource;
    UInt32 *nextDestination;
    unsigned int remaining;
    unsigned int word;
    unsigned int count;
    int empty;
    int equal;
    int more;
    count = bitCount;
    count += 31U;
    nextDestination = destination;
    count = (int)count >> 5;
    nextSource = source;
    remaining = count;
    changed = 0U;
    empty = (count == 0U);
    if (!empty) {
        do {
            word = *nextSource;
            equal = (*nextDestination == word);
            count = word;
            if (!equal)
                changed = 1U;
            remaining -= 1U;
            *nextDestination = count;
            nextDestination++;
            nextSource++;
            more = (remaining != 0U);
        } while (more);
    }
    return changed;
}

#ifdef __MWERKS__
#endif

#ifdef __MWERKS__
#endif

void BitVectors_FillWords(UInt32 *words, int bitCount, unsigned int value)
{
    int wordCount;

    for (wordCount = (bitCount + 31) >> 5; wordCount != 0; --wordCount) {
        *words = value;
        ++words;
    }
}

void BitVectors_IntersectBitVectors(UInt32 *dst, UInt32 *src, SInt32 nbits)
{
    int wordCount;
    wordCount = (nbits + 31) >> 5;
    while (wordCount != 0) {
        *dst++ &= *src++;
        wordCount = wordCount - 1;
    }
}

void BitVectors_OrBitVector(UInt32 *destination, const UInt32 *source, unsigned int bitCount)
{
    UInt32 word;
    const UInt32 *sourceWord;
    UInt32 *destinationWord;
    UInt32 wordCount;

    word = bitCount;
    word += 31U;
    destinationWord = destination;
    word = (int)word >> 5;
    sourceWord = source;
    wordCount = word;
    if (word != 0U) {
        do {
            word = *sourceWord;
            *destinationWord |= word;
            ++sourceWord;
            ++destinationWord;
        } while (--wordCount);
    }
}
