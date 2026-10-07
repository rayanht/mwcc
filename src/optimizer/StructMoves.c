#define CERROR_FILE "StructMoves.c"
#include "compiler/common.h"
#include "compiler/StructMoves.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
void StructMoves_EmitCopy(Operand *sourceOperand, Operand *targetOperand, unsigned int size, unsigned int alignment)
{
    Operand operandCopy;
    unsigned int sizeMinusOne;
    unsigned int copySize;
    Operand *destination;
    operandCopy = *sourceOperand;
    destination = targetOperand;
    copySize = size;
    if (operandCopy.kind < 9U) {
        CError_FATAL(458);
    }
    if (destination->kind < 9U) {
        CError_FATAL(460);
    }
    sizeMinusOne = copySize;
    sizeMinusOne -= 1U;
    if (sizeMinusOne <= 1U || copySize == 4U) {
        emit_load_store_copy(&operandCopy, destination, copySize);
    } else if (copts.debugEnabled != 0 && copts.operandsDebug == 0U && copySize == 8U && alignment == 8U) {
        emit_load_store_copy(&operandCopy, destination, copySize);
    } else if ((int)copySize <= 16 || (copts.uniformSpillBlockWeight == 0U && (int)copySize <= 64)) {
        emit_unrolled_copy(&operandCopy, destination, copySize, alignment);
    } else {
        emit_pair_copy_loop(&operandCopy, destination, copySize);
    }
}
static inline SInt16 allocateGPR(void)
{
    SInt16 r = gUsedVirtualRegistersGPR;
    ++gUsedVirtualRegistersGPR;
    return r;
}

static inline void emitMemory(int op, int r, Operand *n, int offset)
{
    emit_opcode_with_base_offset(op, r, n->reg, NULL, offset);
    Operands_AllocateGPR(n->flags);
}
static inline void firstPair(Operand *a, Operand *b)
{
    int x = gUsedVirtualRegistersGPR++;
    int y = gUsedVirtualRegistersGPR++;
    emitMemory(PC_LWZU, x, b, 8);
    emitMemory(PC_LWZ, y, b, 4);
    emitMemory(PC_STWU, x, a, 8);
    emitMemory(PC_STW, y, a, 4);
}
void emit_pair_copy_loop(Operand *destination, Operand *source, int size)
{
    PCodeLabel *loopLabel;
    int wordRegister, tailRegister;
    int count;

    loopLabel = PCode_NewLabel();
    StructMoves_0051aee0(destination, -8);
    StructMoves_0051aee0(source, -8);
    wordRegister = allocateGPR();
    PCodeUtilities_LoadImmediate(wordRegister, count = (SInt32)size >> 3);
    /* (stand-in: the call is compiled as against a two-argument prototype taking the register as SInt16) */
    ((void (*)(int, SInt16))PCodeUtilities_EmitInstruction)(0x78, wordRegister);
    PCodeUtilities_ResolveLabel(loopLabel);

    firstPair(destination, source);
    PCodeUtilities_EmitConditionalBranch(0xb, loopLabel);

    switch ((SInt32)size & 7) {
        case 7:
            wordRegister = gUsedVirtualRegistersGPR++;
            tailRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LWZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_LHZ, tailRegister, source->reg, NULL, 0xc);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STW, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            emit_opcode_with_base_offset(PC_LBZ, wordRegister, source->reg, NULL, 0xe);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STH, tailRegister, destination->reg, NULL, 0xc);
            Operands_AllocateGPR(destination->flags);
            emit_opcode_with_base_offset(PC_STB, wordRegister, destination->reg, NULL, 0xe);
            Operands_AllocateGPR(destination->flags);
            break;
        case 6:
            wordRegister = gUsedVirtualRegistersGPR++;
            tailRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LWZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_LHZ, tailRegister, source->reg, NULL, 0xc);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STW, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            emit_opcode_with_base_offset(PC_STH, tailRegister, destination->reg, NULL, 0xc);
            Operands_AllocateGPR(destination->flags);
            break;
        case 5:
            wordRegister = gUsedVirtualRegistersGPR++;
            tailRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LWZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_LBZ, tailRegister, source->reg, NULL, 0xc);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STW, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            emit_opcode_with_base_offset(PC_STB, tailRegister, destination->reg, NULL, 0xc);
            Operands_AllocateGPR(destination->flags);
            break;
        case 4:
            count = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LWZ, count, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STW, count, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            break;
        case 3:
            wordRegister = gUsedVirtualRegistersGPR++;
            tailRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LHZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_LBZ, tailRegister, source->reg, NULL, 0xa);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STH, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            emit_opcode_with_base_offset(PC_STB, tailRegister, destination->reg, NULL, 0xa);
            Operands_AllocateGPR(destination->flags);
            break;
        case 2:
            wordRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LHZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STH, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            break;
        case 1:
            wordRegister = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LBZ, wordRegister, source->reg, NULL, 8);
            Operands_AllocateGPR(source->flags);
            emit_opcode_with_base_offset(PC_STB, wordRegister, destination->reg, NULL, 8);
            Operands_AllocateGPR(destination->flags);
            break;
    }
    (void)count;
}

void emit_unrolled_copy(Operand *dest, Operand *src, SInt32 size, SInt32 align)
{
    SInt32 offset = 0;
    SInt32 firstReg, secondReg;

    if (dest->kind == OpndType_IndirectSymbol)
        Operands_Normalize(dest);
    if (dest->kind != OpndType_IndirectGPR_ImmOffset || dest->displacement + size > 0x7fff) {
        firstReg = gUsedVirtualRegistersGPR++;
        Operands_EmitAddress(firstReg, dest);
        dest->kind = OpndType_IndirectGPR_ImmOffset;
        dest->reg = firstReg;
        dest->object = NULL;
        dest->displacement = 0;
    }
    if (src->kind == OpndType_IndirectSymbol)
        Operands_Normalize(src);
    if (src->kind != OpndType_IndirectGPR_ImmOffset || src->displacement + size > 0x7fff) {
        firstReg = gUsedVirtualRegistersGPR++;
        Operands_EmitAddress(firstReg, src);
        src->kind = OpndType_IndirectGPR_ImmOffset;
        src->reg = firstReg;
        src->object = NULL;
        src->displacement = 0;
    }
    if (copts.debugEnabled != 0 && copts.operandsDebug == 0 && align % 8 == 0 && size >= 16) {
        do {
            firstReg = gUsedVirtualRegistersFPR++;
            secondReg = gUsedVirtualRegistersFPR++;
            emit_opcode_with_base_offset(PC_LFD, firstReg, src->reg, src->object, src->displacement + offset);
            Operands_AllocateGPR(src->flags);
            emit_opcode_with_base_offset(PC_LFD, secondReg, src->reg, src->object, src->displacement + offset + 8);
            Operands_AllocateGPR(src->flags);
            emit_opcode_with_base_offset(PC_STFD, firstReg, dest->reg, dest->object, dest->displacement + offset);
            Operands_AllocateGPR(dest->flags);
            emit_opcode_with_base_offset(PC_STFD, secondReg, dest->reg, dest->object, dest->displacement + offset + 8);
            Operands_AllocateGPR(dest->flags);
            offset += 16;
            size -= 16;
        } while (size >= 16);
    }
    while (size >= 8) {
        if (copts.debugEnabled != 0 && copts.operandsDebug == 0 && align % 8 == 0) {
            firstReg = gUsedVirtualRegistersFPR++;
            emit_opcode_with_base_offset(PC_LFD, firstReg, src->reg, src->object, src->displacement + offset);
            Operands_AllocateGPR(src->flags);
            emit_opcode_with_base_offset(PC_STFD, firstReg, dest->reg, dest->object, dest->displacement + offset);
            Operands_AllocateGPR(dest->flags);
        } else {
            firstReg = gUsedVirtualRegistersGPR++;
            secondReg = gUsedVirtualRegistersGPR++;
            emit_opcode_with_base_offset(PC_LWZ, firstReg, src->reg, src->object, src->displacement + offset);
            Operands_AllocateGPR(src->flags);
            emit_opcode_with_base_offset(PC_LWZ, secondReg, src->reg, src->object, src->displacement + offset + 4);
            Operands_AllocateGPR(src->flags);
            emit_opcode_with_base_offset(PC_STW, firstReg, dest->reg, dest->object, dest->displacement + offset);
            Operands_AllocateGPR(dest->flags);
            emit_opcode_with_base_offset(PC_STW, secondReg, dest->reg, dest->object, dest->displacement + offset + 4);
            Operands_AllocateGPR(dest->flags);
        }
        offset += 8;
        size -= 8;
    }
    if ((size & 4) != 0) {
        firstReg = gUsedVirtualRegistersGPR++;
        emit_opcode_with_base_offset(PC_LWZ, firstReg, src->reg, src->object, src->displacement + offset);
        Operands_AllocateGPR(src->flags);
        emit_opcode_with_base_offset(PC_STW, firstReg, dest->reg, dest->object, dest->displacement + offset);
        Operands_AllocateGPR(dest->flags);
        offset += 4;
    }
    if ((size & 2) != 0) {
        firstReg = gUsedVirtualRegistersGPR++;
        emit_opcode_with_base_offset(PC_LHZ, firstReg, src->reg, src->object, src->displacement + offset);
        Operands_AllocateGPR(src->flags);
        emit_opcode_with_base_offset(PC_STH, firstReg, dest->reg, dest->object, dest->displacement + offset);
        Operands_AllocateGPR(dest->flags);
        offset += 2;
    }
    if ((size & 1) != 0) {
        firstReg = gUsedVirtualRegistersGPR++;
        emit_opcode_with_base_offset(PC_LBZ, firstReg, src->reg, src->object, src->displacement + offset);
        Operands_AllocateGPR(src->flags);
        emit_opcode_with_base_offset(PC_STB, firstReg, dest->reg, dest->object, dest->displacement + offset);
        Operands_AllocateGPR(dest->flags);
    }
}

void emit_load_store_copy(Operand *destination, Operand *source, int size)
{
    int fpRegister;
    int integerRegister;
    int loadOpcode;
    int indexedLoadOpcode;
    int storeOpcode;
    int indexedStoreOpcode;
    if (source->kind == OpndType_IndirectSymbol) {
        Operands_Normalize(source);
    }
    if (destination->kind == OpndType_IndirectSymbol) {
        Operands_Normalize(destination);
    }
    if (copts.debugEnabled != 0 && copts.operandsDebug == 0 && size == 8) {
        fpRegister = gUsedVirtualRegistersFPR++;
        if (source->kind == OpndType_IndirectGPR_ImmOffset) {
            emit_opcode_with_base_offset(PC_LFD, fpRegister, source->reg, source->object, source->displacement);
            Operands_AllocateGPR(source->flags);
        } else if (source->kind == OpndType_IndirectGPR_Indexed) {
            PCodeUtilities_EmitInstruction(PC_LFDX, fpRegister, source->reg, source->secondary_reg);
            Operands_AllocateGPR(source->flags);
        } else {
            CError_FATAL(143);
        }
        if (destination->kind == OpndType_IndirectGPR_ImmOffset) {
            emit_opcode_with_base_offset(PC_STFD, fpRegister, destination->reg, destination->object,
                                         destination->displacement);
            Operands_AllocateGPR(destination->flags);
        } else if (destination->kind == OpndType_IndirectGPR_Indexed) {
            PCodeUtilities_EmitInstruction(PC_STFDX, fpRegister, destination->reg, destination->secondary_reg);
            Operands_AllocateGPR(destination->flags);
        } else {
            CError_FATAL(155);
        }
    } else {
        integerRegister = gUsedVirtualRegistersGPR++;
        if (source->kind == OpndType_IndirectGPR_ImmOffset) {
            if (size == 1) {
                loadOpcode = 21;
            } else if (size == 2) {
                loadOpcode = 25;
            } else {
                loadOpcode = 34;
            }
            emit_opcode_with_base_offset(loadOpcode, integerRegister, source->reg, source->object,
                                         source->displacement);
            Operands_AllocateGPR(source->flags);
        } else if (source->kind == OpndType_IndirectGPR_Indexed) {
            if (size == 1) {
                indexedLoadOpcode = 23;
            } else if (size == 2) {
                indexedLoadOpcode = 27;
            } else {
                indexedLoadOpcode = 36;
            }
            PCodeUtilities_EmitInstruction(indexedLoadOpcode, integerRegister, source->reg, source->secondary_reg);
            Operands_AllocateGPR(source->flags);
        } else {
            CError_FATAL(167);
        }
        if (destination->kind == OpndType_IndirectGPR_ImmOffset) {
            if (size == 1) {
                storeOpcode = 40;
            } else if (size == 2) {
                storeOpcode = 44;
            } else {
                storeOpcode = 49;
            }
            emit_opcode_with_base_offset(storeOpcode, integerRegister, destination->reg, destination->object,
                                         destination->displacement);
            Operands_AllocateGPR(destination->flags);
        } else if (destination->kind == OpndType_IndirectGPR_Indexed) {
            if (size == 1) {
                indexedStoreOpcode = 42;
            } else if (size == 2) {
                indexedStoreOpcode = 46;
            } else {
                indexedStoreOpcode = 51;
            }
            PCodeUtilities_EmitInstruction(indexedStoreOpcode, integerRegister, destination->reg,
                                           destination->secondary_reg);
            Operands_AllocateGPR(destination->flags);
        } else {
            CError_FATAL(179);
        }
    }
}

void StructMoves_0051aee0(Operand *node, SInt32 offset)
{
    SInt32 resultReg = gUsedVirtualRegistersGPR++;

    if (node->kind == OpndType_IndirectSymbol)
        Operands_Normalize(node);

    if (node->kind == OpndType_IndirectGPR_ImmOffset) {
        offset += node->displacement;
        if (offset != (SInt16)offset) {
            PCodeUtilities_EmitAddress(resultReg, node->reg, node->object, node->displacement);
            PCodeUtilities_EmitInstruction(PC_ADDI, resultReg, resultReg, 0, offset - node->displacement);
        } else {
            PCodeUtilities_EmitAddress(resultReg, node->reg, node->object, offset);
        }
    } else if (node->kind == OpndType_IndirectGPR_Indexed) {
        PCodeUtilities_EmitInstruction(PC_ADD, resultReg, node->reg, node->secondary_reg);
        PCodeUtilities_EmitInstruction(PC_ADDI, resultReg, resultReg, 0, offset);
    } else {
        CError_FATAL(80);
    }

    node->kind = OpndType_IndirectGPR_ImmOffset;
    node->reg = resultReg;
    node->object = NULL;
    node->displacement = 0;
}

void StructMoves_PrepareOperandForOffset(Operand *operand, unsigned int offset, int n)
{
    unsigned int reg;
    if (operand->kind == 11U) {
        Operands_Normalize(operand);
    }
    if (operand->kind != 9U || (int)(operand->displacement + offset) > 32767) {
        reg = gUsedVirtualRegistersGPR;
        gUsedVirtualRegistersGPR += 1U;
        Operands_EmitAddress(reg, operand);
        operand->kind = 9U;
        operand->reg = (short)reg;
        operand->object = NULL;
        operand->displacement = 0;
    }
}
