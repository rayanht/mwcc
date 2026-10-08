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
