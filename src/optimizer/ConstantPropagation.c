#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/LoadDeletion.h"
#include "compiler/LoopDetection.h"
#include "compiler/PCode.h"
#define NULL 0
#define LowMask(lo) ((lo).value.signed_value > 31 ? 0U : 0xffffffffU >> (lo).value.unsigned_value)
#define HighMask(hi) ((int)(hi).value.unsigned_value + 1 > 31 ? 0U : 0xffffffffU >> ((hi).value.unsigned_value + 1U))
#define RangeMask(lo, hi)                                                                                              \
    ((int)(lo).value.unsigned_value <= (int)(hi).value.unsigned_value ? (LowMask(lo) & ~HighMask(hi))                  \
                                                                      : (LowMask(lo) | ~HighMask(hi)))

static int constantPropagationChanged;
static struct PCodeInstruction **unique_definitions;
static struct PCodeInstruction **virtual_register_definitions;

static int GetMask(int index, unsigned int *out);
static int GetKind2(int index);
static int GetKind(int index);
static int GetSync(int index, short *out, short *typeout);
static int GetConst(int index, short *out);
static void SetType(PCodeInstruction *p, short t);

void COpt_LoadDeletion(void)
{
    int blockIndex;
    CBlockData *sets;
    int instructionIndex;
    struct E *entry;
    int k;
    UInt32 *secondSet;

    gLoadDeletionChanged = 0;
    LoadDeletion_InitializeLoadLivenessRecordCounts();
    if (data_0058820c > 0) {
        COpt_SetLoopCodeMotionMode(0);
        LoadDeletion_RecordImmediateLoadLiveness();
        data_00587c98 = (CBlockData *)oalloc(gPCodeBlockCount * sizeof(CBlockData));
        blockIndex = 0;
        sets = data_00587c98;
        while (blockIndex < gPCodeBlockCount) {
            sets->generatedLoads = (UInt32 *)oalloc(((data_0058820c + 31) >> 5) * sizeof(UInt32));
            secondSet = (UInt32 *)oalloc(((data_0058820c + 31) >> 5) * sizeof(UInt32));
            blockIndex++;
            sets->killedLoads = secondSet;
            sets++;
        }
        LoadDeletion_BuildLoadLivenessSets();
        for (instructionIndex = 0; instructionIndex < data_0058820c; instructionIndex++) {
            k = 0;
            entry = &immediateLoadLiveness[instructionIndex];
            (void)((int)entry * k);
            if (entry->flag != 0)
                continue;
            if ((entry->inst->flags & fSideEffects) != 0)
                continue;
            PCode_UnlinkInstruction(entry->inst);
            gLoadDeletionChanged = 1;
        }
    }
    freeoheap();
}

void ConstantPropagation_FindUniqueDefinitions(struct PCodeBlock *state)
{
    SInt32 i;
    SInt32 j;
    struct CodeMotionEntryLink *def;
    SInt32 reg;
    struct PCodeInstruction *result;

    for (i = 0; i < gUsedVirtualRegistersGPR; i++) {
        result = NULL;
        for (def = code_motion_register_definition_heads[i]; def != NULL; def = def->next) {
            reg = def->entry_index;
            if (data_00587fe4[state->index].definition_sets[2][reg >> 5] & (1 << reg)) {
                if (result == NULL) {
                    result = code_motion_entries[reg].instruction;
                } else {
                    result = NULL;
                    break;
                }
            }
        }
        unique_definitions[i] = result;
    }

    for (j = 0; j < gUsedVirtualRegistersVR; j++) {
        struct PCodeInstruction *vresult;
        SInt32 vreg;
        struct CodeMotionEntryLink *vdef;
        vresult = NULL;
        for (vdef = register_definition_heads[j]; vdef != NULL; vdef = vdef->next) {
            vreg = vdef->entry_index;
            if (data_00587fe4[state->index].definition_sets[2][vreg >> 5] & (1 << vreg)) {
                if (vresult == NULL) {
                    vresult = code_motion_entries[vreg].instruction;
                } else {
                    vresult = NULL;
                    break;
                }
            }
        }
        virtual_register_definitions[j] = vresult;
    }
}

#pragma auto_inline off
struct PCodeInstruction *find_dlocal_addi(PCodeOperand *operand, SInt16 *size_out, SInt16 displacement)
{
    SInt32 size;
    struct PCodeInstruction *entry;
    struct PCodeInstruction *record;
    SInt32 offset;

    entry = unique_definitions[operand->value.reg];
    if ((record = entry) != NULL && record->opcode == PC_ADDI) {
        if (record->operandData.operands[2].kind == PCOp_MEMORY &&
            record->operandData.operands[1].value.reg == stack_base_reg &&
            record->operandData.operands[2].object->datatype == DLOCAL) {
            size = record->operandData.operands[2].value.signed_value;
            offset = displacement + record->operandData.operands[2].object->u.var.uid + size;
            if (offset == (SInt16)offset) {
                *size_out = (SInt16)size;
                return record;
            }
            return NULL;
        }
        return NULL;
    }
    return NULL;
}
#pragma auto_inline reset

void ConstantPropagation_PropagateConstantsInBlock(struct PCodeBlock *block)
{
    PCodeInstruction *instruction;
    PCodeInstruction *definition;
    short constant;
    short replacementType;
    short secondConstant;
    short immediate;
    short originalImmediate;
    short displacement;
    int kind;
    unsigned int knownMask;
    unsigned int rangeMask;
    PCodeOperand *operand;
    int operandIndex;

    for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
        switch (instruction->opcode) {
            case PC_MR:
                if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                    SetType(instruction, PC_LI);
                    instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                    instruction->operandData.operands[1].value.signed_value = constant;
                    instruction->operandData.operands[1].object = NULL;
                    constantPropagationChanged = gConstantPropagationChanged = 1;
                }
                break;
            case PC_VMR:
                if (GetSync(instruction->operandData.operands[1].value.reg, &constant, &replacementType)) {
                    instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[instruction->opcode].flags) |
                                         gPCodeOpcodeDescriptors[replacementType].flags;
                    instruction->opcode = replacementType;
                    instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                    instruction->operandData.operands[1].value.signed_value = constant;
                    instruction->operandData.operands[1].object = NULL;
                    constantPropagationChanged = gConstantPropagationChanged = 1;
                }
                break;
            case PC_RLWINM:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (instruction->operandData.operands[2].value.signed_value == 0 &&
                        instruction->operandData.operands[4].value.signed_value == 0x1f) {
                        if (GetConst(instruction->operandData.operands[1].value.reg, &constant) &&
                            ((instruction->operandData.operands[3].value.signed_value == 0x10 &&
                              constant == (constant & 0x7fff)) ||
                             (instruction->operandData.operands[3].value.signed_value == 0x18 &&
                              constant == (constant & 0xff)))) {
                            SetType(instruction, PC_LI);
                            instruction->operand_count = 2;
                            instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[1].value.signed_value = constant;
                            instruction->operandData.operands[1].object = NULL;
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        } else {
                            kind = GetKind2(instruction->operandData.operands[1].value.reg);
                            if ((kind == 2 && instruction->operandData.operands[3].value.signed_value <= 0x10) ||
                                (kind == 1 && instruction->operandData.operands[3].value.signed_value <= 0x18)) {
                                instruction->opcode = PC_MR;
                                instruction->operand_count = 2;
                                instruction->flags |= PCodeInstruction_CopySourceExclusion;
                                constantPropagationChanged = gConstantPropagationChanged = 1;
                            } else if (GetMask(instruction->operandData.operands[1].value.reg, &knownMask)) {
                                rangeMask = RangeMask(instruction->operandData.operands[3],
                                                      instruction->operandData.operands[4]);
                                if ((knownMask & rangeMask) == knownMask) {
                                    instruction->opcode = PC_MR;
                                    instruction->operand_count = 2;
                                    instruction->flags |= PCodeInstruction_CopySourceExclusion;
                                    constantPropagationChanged = gConstantPropagationChanged = 1;
                                }
                            }
                        }
                    }
                }
                break;
            case PC_EXTSH:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                        SetType(instruction, PC_LI);
                        instruction->operand_count = 2;
                        instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                        instruction->operandData.operands[1].value.signed_value = constant;
                        instruction->operandData.operands[1].object = NULL;
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    } else if ((unsigned)(GetKind(instruction->operandData.operands[1].value.reg) - 1) <= 1) {
                        instruction->opcode = PC_MR;
                        instruction->operand_count = 2;
                        instruction->flags |= PCodeInstruction_CopySourceExclusion;
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    }
                }
                break;
            case PC_EXTSB:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant) && constant >= -0x80 &&
                        constant <= 0x7f) {
                        SetType(instruction, PC_LI);
                        instruction->operand_count = 2;
                        instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                        instruction->operandData.operands[1].value.signed_value = constant;
                        instruction->operandData.operands[1].object = NULL;
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    } else if (GetKind(instruction->operandData.operands[1].value.reg) == 1) {
                        instruction->opcode = PC_MR;
                        instruction->operand_count = 2;
                        instruction->flags |= PCodeInstruction_CopySourceExclusion;
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    }
                }
                break;
            case PC_ADDI:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    immediate = instruction->operandData.operands[2].value.signed_value;
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                        if (immediate + constant == (short)(immediate + constant)) {
                            SetType(instruction, PC_LI);
                            instruction->operand_count = 2;
                            instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[1].value.signed_value = immediate + constant;
                            instruction->operandData.operands[1].object = NULL;
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        }
                    }
                }
                break;
            case PC_ADD:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (GetConst(instruction->operandData.operands[2].value.reg, &constant)) {
                        if (constant == 0) {
                            instruction->opcode = PC_MR;
                            instruction->flags |= PCodeInstruction_CopySourceExclusion;
                            instruction->operand_count = 2;
                        } else {
                            instruction->opcode = PC_ADDI;
                            instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[2].value.signed_value = constant;
                            instruction->operandData.operands[2].object = NULL;
                        }
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                        immediate = constant;
                    }
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                        if (instruction->opcode == PC_ADDI || instruction->opcode == PC_MR) {
                            if (immediate + constant == (short)(immediate + constant)) {
                                SetType(instruction, PC_LI);
                                instruction->operand_count = 2;
                                instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                                instruction->operandData.operands[1].value.signed_value = immediate + constant;
                                instruction->operandData.operands[1].object = NULL;
                                constantPropagationChanged = gConstantPropagationChanged = 1;
                            }
                        } else {
                            instruction->operandData.operands[1] = instruction->operandData.operands[2];
                            if (constant == 0) {
                                instruction->opcode = PC_MR;
                                instruction->flags |= PCodeInstruction_CopySourceExclusion;
                                instruction->operand_count = 2;
                            } else {
                                instruction->opcode = PC_ADDI;
                                instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                                instruction->operandData.operands[2].value.signed_value = constant;
                                instruction->operandData.operands[2].object = NULL;
                            }
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        }
                    }
                    if (constantPropagationChanged != 0) {
                        if (instruction->opcode == PC_MR) {
                            definition = find_dlocal_addi(&instruction->operandData.operands[1], &constant, 0);
                            if (definition != NULL) {
                                instruction->opcode = PC_ADDI;
                                instruction->flags = definition->flags;
                                instruction->operand_count = 3;
                                instruction->operandData.operands[1] = definition->operandData.operands[1];
                                instruction->operandData.operands[2] = definition->operandData.operands[2];
                                constantPropagationChanged = gConstantPropagationChanged = 1;
                            }
                        } else if (instruction->opcode == PC_ADDI) {
                            if (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                                originalImmediate = instruction->operandData.operands[2].value.signed_value;
                                definition = find_dlocal_addi(&instruction->operandData.operands[1], &constant,
                                                              instruction->operandData.operands[2].value.signed_value);
                                if (definition != NULL) {
                                    instruction->opcode = PC_ADDI;
                                    instruction->flags = definition->flags;
                                    instruction->operandData.operands[1] = definition->operandData.operands[1];
                                    instruction->operandData.operands[2] = definition->operandData.operands[2];
                                    instruction->operandData.operands[2].value.signed_value =
                                        constant + originalImmediate;
                                    constantPropagationChanged = gConstantPropagationChanged = 1;
                                }
                            }
                        }
                    }
                }
                break;
            case PC_OR:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (GetConst(instruction->operandData.operands[2].value.reg, &constant)) {
                        if (constant != 0) {
                            instruction->opcode = PC_ORI;
                            instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[PC_OR].flags) |
                                                 gPCodeOpcodeDescriptors[PC_ORI].flags;
                            instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[2].value.signed_value = constant;
                            instruction->operandData.operands[2].object = NULL;
                        } else {
                            instruction->opcode = PC_MR;
                            instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[PC_OR].flags) |
                                                 gPCodeOpcodeDescriptors[PC_MR].flags,
                            instruction->operand_count = 2;
                        }
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                        immediate = constant;
                    }
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                        if (instruction->opcode == PC_ORI || instruction->opcode == PC_MR) {
                            SetType(instruction, PC_LI);
                            instruction->operand_count = 2;
                            instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[1].value.signed_value = immediate | constant;
                            instruction->operandData.operands[1].object = NULL;
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        } else if (constant != 0) {
                            instruction->opcode = PC_ORI;
                            instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[PC_OR].flags) |
                                                 gPCodeOpcodeDescriptors[PC_ORI].flags;
                            instruction->operandData.operands[1] = instruction->operandData.operands[2];
                            instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[2].value.signed_value = constant;
                            instruction->operandData.operands[2].object = NULL;
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        } else {
                            instruction->opcode = PC_MR;
                            instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[PC_OR].flags) |
                                                 gPCodeOpcodeDescriptors[PC_MR].flags;
                            instruction->operand_count = 2;
                            instruction->operandData.operands[1] = instruction->operandData.operands[2];
                            constantPropagationChanged = gConstantPropagationChanged = 1;
                        }
                    }
                }
                break;
            case PC_SUBF:
                if ((instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
                    if (GetConst(instruction->operandData.operands[1].value.reg, &constant) &&
                        -constant == (short)-constant) {
                        if (GetConst(instruction->operandData.operands[2].value.reg, &secondConstant) &&
                            secondConstant - constant == (short)(secondConstant - constant)) {
                            SetType(instruction, PC_LI);
                            instruction->operand_count = 2;
                            instruction->operandData.operands[1].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[1].value.signed_value = secondConstant - constant;
                            instruction->operandData.operands[1].object = NULL;
                        } else {
                            if (constant == 0) {
                                instruction->opcode = PC_MR;
                                instruction->flags |= PCodeInstruction_CopySourceExclusion;
                                instruction->operand_count = 2;
                                instruction->operandData.operands[1] = instruction->operandData.operands[2];
                            } else {
                                instruction->opcode = PC_ADDI;
                                instruction->operandData.operands[1] = instruction->operandData.operands[2];
                                instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                                instruction->operandData.operands[2].value.signed_value = -constant;
                                instruction->operandData.operands[2].object = NULL;
                            }
                        }
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                        secondConstant = constant;
                    } else if (GetConst(instruction->operandData.operands[2].value.reg, &constant) &&
                               -constant == (short)-constant) {
                        if (constant == 0) {
                            instruction->opcode = PC_NEG;
                            instruction->operand_count = 2;
                        } else {
                            instruction->flags = (instruction->flags & ~gPCodeOpcodeDescriptors[PC_SUBF].flags) |
                                                 gPCodeOpcodeDescriptors[PC_SUBFIC].flags;
                            instruction->opcode = PC_SUBFIC;
                            instruction->operand_count = 4;
                            instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                            instruction->operandData.operands[2].value.signed_value = constant;
                            instruction->operandData.operands[2].object = NULL;
                            instruction->operandData.operands[3].kind = PCOp_SPR;
                            instruction->operandData.operands[3].value.reg = 0;
                            instruction->operandData.operands[3].flags = PCodeOperand_Definition;
                        }
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    }
                }
                break;
            case PC_LBZ:
            case PC_LHZ:
            case PC_LHA:
            case PC_LWZ:
            case PC_STB:
            case PC_STH:
            case PC_STW:
            case PC_LFS:
            case PC_LFD:
            case PC_STFS:
            case PC_STFD:
                if (instruction->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                    displacement = instruction->operandData.operands[2].value.signed_value;
                    definition = find_dlocal_addi(&instruction->operandData.operands[1], &constant,
                                                  instruction->operandData.operands[2].value.signed_value);
                    if (definition != NULL) {
                        instruction->operandData.operands[1] = definition->operandData.operands[1];
                        instruction->operandData.operands[2] = definition->operandData.operands[2];
                        instruction->operandData.operands[2].value.signed_value = constant + displacement;
                        constantPropagationChanged = gConstantPropagationChanged = 1;
                    }
                }
                break;
            case PC_LBZX:
            case PC_LHZX:
            case PC_LHAX:
            case PC_LWZX:
            case PC_STBX:
            case PC_STHX:
            case PC_STWX:
            case PC_LFSX:
            case PC_LFDX:
            case PC_STFSX:
            case PC_STFDX:
                if (GetConst(instruction->operandData.operands[2].value.reg, &constant)) {
                    instruction->opcode -= 2;
                    instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                    instruction->operandData.operands[2].value.signed_value = constant;
                    instruction->operandData.operands[2].object = NULL;
                    constantPropagationChanged = gConstantPropagationChanged = 1;
                } else if (GetConst(instruction->operandData.operands[1].value.reg, &constant)) {
                    instruction->opcode -= 2;
                    instruction->operandData.operands[1] = instruction->operandData.operands[2];
                    instruction->operandData.operands[2].kind = PCOp_IMMEDIATE;
                    instruction->operandData.operands[2].value.signed_value = constant;
                    instruction->operandData.operands[2].object = NULL;
                    constantPropagationChanged = gConstantPropagationChanged = 1;
                }
                break;
        }
        for (operandIndex = 0, operand = instruction->operandData.operands; operandIndex < instruction->operand_count;
             operandIndex++, operand++) {
            if (operand->kind == PCOp_GPR && (operand->flags & PCodeOperand_Definition))
                unique_definitions[operand->value.reg] = instruction;
            else if (operand->kind == PCOp_VR && (operand->flags & PCodeOperand_Definition))
                virtual_register_definitions[operand->value.reg] = instruction;
        }
    }
}

static int GetMask(int index, unsigned int *out)
{
    PCodeInstruction *t = unique_definitions[index];
    if (t != NULL && t->opcode == PC_RLWINM) {
        *out = RangeMask(t->operandData.operands[3], t->operandData.operands[4]);
        return 1;
    }
    return 0;
}

static int GetKind2(int index)
{
    PCodeInstruction *t = unique_definitions[index];
    if (t != NULL && (t->flags & fIsRead)) {
        if (t->opcode >= 0x19 && t->opcode <= 0x1c)
            return 2;
        if (t->opcode >= 0x15 && t->opcode <= 0x18)
            return 1;
    }
    return 0;
}

static int GetKind(int index)
{
    PCodeInstruction *t = unique_definitions[index];
    if (t != NULL) {
        if (t->flags & fIsRead) {
            if (t->opcode >= 29 && t->opcode <= 32)
                return 2;
        } else {
            if (t->opcode == PC_EXTSB)
                return 1;
            if (t->opcode == PC_EXTSH)
                return 2;
        }
    }
    return 0;
}

static int GetSync(int index, short *out, short *typeout)
{
    PCodeInstruction *t = virtual_register_definitions[index];
    if (t != NULL && (t->opcode == PC_VSPLTISB || (unsigned short)(t->opcode - 0x15f) <= 1) &&
        t->operandData.operands[1].kind == PCOp_IMMEDIATE) {
        *out = (short)t->operandData.operands[1].value.signed_value;
        *typeout = t->opcode;
        return 1;
    }
    return 0;
}

static int GetConst(int index, short *out)
{
    PCodeInstruction *t = unique_definitions[index];
    if (t != NULL && t->opcode == PC_LI && t->operandData.operands[1].kind == PCOp_IMMEDIATE) {
        *out = (short)t->operandData.operands[1].value.signed_value;
        return 1;
    }
    return 0;
}

static void SetType(PCodeInstruction *p, short t)
{
    p->flags = gPCodeOpcodeDescriptors[t].flags | (p->flags & ~gPCodeOpcodeDescriptors[p->opcode].flags);
    p->opcode = (short)t;
}

void COpt_ConstantPropagation(void)
{
    PCodeBlock *block;
    SInt32 i;

    gConstantPropagationChanged = 0;
    COpt_SetLoopCodeMotionMode(0);
    unique_definitions = galloc(gUsedVirtualRegistersGPR * 4);
    virtual_register_definitions = galloc(gUsedVirtualRegistersVR * 4);
    do {
        constantPropagationChanged = 0;
        for (i = 0; i < gPCodeBlockCount; i++) {
            if ((block = gPCodeBlockOrder[i]) != NULL) {
                ConstantPropagation_FindUniqueDefinitions(block);
                ConstantPropagation_PropagateConstantsInBlock(block);
            }
        }
    } while (constantPropagationChanged != 0);
    freeoheap();
}
