#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
#include "version.h"
#include "compiler/SpillCode.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CParser.h"
#include "compiler/CodeGen.h"
#include "compiler/Coloring.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"

short spill_address_register = 0;

void SpillCode_ComputeSpillCosts(int reg_class)
{
    PCodeBlock *block;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        PCodeInstruction *instruction;
        int block_weight;

        block_weight = copts.optimizesize ? 1 : block->execution_weight;
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            {
                PCodeOperand *operand;
                unsigned int count;

                count = instruction->operand_count;
                for (operand = instruction->operandData.operands; count--; operand++) {
                    if (operand->kind == reg_class && ((signed char)operand->flags & PCodeOperand_Use) != 0) {
                        gInterferenceGraph[operand->value.reg]->spill_cost += block_weight * 2;
                    }
                }
            }
            {
                unsigned int count;
                PCodeOperand *operand;

                count = instruction->operand_count;
                for (operand = instruction->operandData.operands; count--; operand++) {
                    if (operand->kind == reg_class && ((signed char)operand->flags & PCodeOperand_Definition) != 0) {
                        gInterferenceGraph[operand->value.reg]->spill_cost += block_weight;
                    }
                }
            }
        }
    }
}

static Object *make_spill_temporary(Type *type)
{
    Object *object;

    object = (Object *)lalloc(sizeof(Object));
    memclrw(object, sizeof(Object));
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->datatype = DLOCAL;
    object->type = type;
    object->name = CParser_GetUniqueName();
    object->u.var.info = CodeGen_AllocateVarInfo();
    object->u.var.uid = 0;
    return object;
}

static PCodeInstruction *spill_load(short reg, InterferenceNode *node)
{
    Operand operand;
    short opcode;
    Type *type;

    type = node->object->type;
    opcode = type->size == 1 ? PC_LBZ : type->size == 2 ? (is_unsigned(type) ? PC_LHZ : PC_LHA) : PC_LWZ;
    memclrw(&operand, sizeof(Operand));
    operand.kind = OpndType_Symbol;
    operand.object = node->object;
    if (node->object->datatype != DLOCAL)
        CError_FATAL(130);
    Operands_Normalize(&operand);
    if (operand.kind != OpndType_GPR_ImmOffset)
        CError_FATAL(140);
#if VERSION == VERSION_GC_1_0
    return PCodeUtilities_CreateInstruction(opcode, reg, operand.reg, node->object, (node->flags & 32) ? 4 : 0);
#else
    return PCodeUtilities_CreateInstruction(opcode, reg, operand.reg, node->object,
                                            (node->flags & 32) ? low_word_offset
                                                               : ((node->flags & 16) ? high_word_offset : 0));
#endif
}

static PCodeInstruction *spill_load_fpr(short reg, InterferenceNode *node)
{
    Type *type;
    int offset;

    type = node->object->type;
    CError_ASSERT(165, node->object->datatype == DLOCAL);
#if VERSION == VERSION_GC_1_0
    offset = (node->flags & 32) ? 4 : 0;
#else
    if (node->flags & 32)
        offset = low_word_offset;
    else if (node->flags & 16)
        offset = high_word_offset;
    else
        offset = 0;
#endif
    return PCodeUtilities_CreateInstruction(type->size == 8 ? PC_LFD : PC_LFS, reg, stack_base_reg, node->object,
                                            offset);
}

static void spill_load_vr(PCodeInstruction *instruction, short reg, InterferenceNode *node)
{
    PCodeInstruction *address;
    PCodeInstruction *load;

    CError_ASSERT(184, node->object->datatype == DLOCAL);
    address = PCodeUtilities_CreateInstruction(PC_ADDI, spill_address_register, stack_base_reg, node->object, 0);
    load = PCodeUtilities_CreateInstruction(PC_LVX, reg, 0, spill_address_register);
    PCode_InsertInstructionBefore(instruction, address);
    PCode_InsertInstructionAfter(address, load);
}

static PCodeInstruction *spill_store(short reg, InterferenceNode *node)
{
    int offset;
    Type *type;
    short opcode;

    type = node->object->type;
#if VERSION == VERSION_GC_1_0
    offset = (node->flags & 32) ? 4 : 0;
#else
    offset = (node->flags & 32) ? low_word_offset : ((node->flags & 16) ? high_word_offset : 0);
#endif
    if (type->size == 1)
        opcode = PC_STB;
    else if (type->size == 2)
        opcode = PC_STH;
    else
        opcode = PC_STW;
    return PCodeUtilities_CreateInstruction(opcode, reg, stack_base_reg, node->object, offset);
}

static PCodeInstruction *spill_store_fpr(short reg, InterferenceNode *node)
{
    return PCodeUtilities_CreateInstruction(node->object->type->size == 8 ? PC_STFD : PC_STFS, reg, stack_base_reg,
                                            node->object, 0);
}

static void spill_store_vr(PCodeInstruction *instruction, Boolean before, short reg, InterferenceNode *node)
{
    PCodeInstruction *address;
    PCodeInstruction *store;

    address = PCodeUtilities_CreateInstruction(PC_ADDI, spill_address_register, stack_base_reg, node->object, 0);
    store = PCodeUtilities_CreateInstruction(PC_STVX, reg, 0, spill_address_register);
    if (before)
        PCode_InsertInstructionBefore(instruction, address);
    else
        PCode_InsertInstructionAfter(instruction, address);
    PCode_InsertInstructionAfter(address, store);
}

void SpillCode_InsertGPRSpillCode(PCodeBlock *block, PCodeInstruction *instruction)
{
    int register_limit;
    int operand_index;
    PCodeOperand *operand;
    int definitions;
    InterferenceNode *node;
    int uses;
    PCodeOperand *other;
    int other_index;
    int original_register;
    int spill_register;

    operand_index = 0;
    register_limit = gUsedVirtualRegistersGPR;
    operand = instruction->operandData.operands;
    while (operand_index < instruction->operand_count) {
        if (operand->kind == PCOp_GPR && operand->value.reg < register_limit &&
            ((node = gInterferenceGraph[(original_register = operand->value.reg)])->flags & 1) != 0) {
            spill_register = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR += 1;
            uses = 0;
            definitions = 0;
            other_index = operand_index;
            other = operand;
            while (other_index < instruction->operand_count) {
                if (other->kind == PCOp_GPR && other->value.reg == original_register) {
                    if ((other->flags & PCodeOperand_Use) != 0)
                        uses++;
                    if ((other->flags & PCodeOperand_Definition) != 0)
                        definitions++;
                    other->value.reg = spill_register;
                }
                other_index++;
                other++;
            }
            if (uses != 0)
                PCode_InsertInstructionBefore(instruction, spill_load(spill_register, node));
            if (definitions != 0)
                PCode_InsertInstructionAfter(instruction, spill_store(spill_register, node));
        }
        operand_index++;
        operand++;
    }
}

void SpillCode_RewriteSpilledFPRs(PCodeBlock *unused, PCodeInstruction *instruction)
{
    int register_limit;
    PCodeOperand *matching_operand;
    int scan_index;
    long original_register;
    int matching_register;
    int use_count;
    int definition_count;
    PCodeOperand *operand;
    int operand_index;
    int spill_register;

    operand_index = 0;
    register_limit = gUsedVirtualRegistersFPR;
    operand = instruction->operandData.operands;
    for (; operand_index < instruction->operand_count; operand_index++, operand++) {
        if (operand->kind == PCOp_FPR) {
            original_register = operand->value.reg;
            if (original_register < register_limit) {
                InterferenceNode *node;
                if ((node = gInterferenceGraph[original_register])->flags & 1) {
                    spill_register = gUsedVirtualRegistersFPR;
                    gUsedVirtualRegistersFPR++;
                    use_count = 0;
                    definition_count = 0;
                    scan_index = operand_index;
                    matching_operand = operand;
                    matching_register = original_register;
                    for (; scan_index < instruction->operand_count; scan_index++, matching_operand++) {
                        if (matching_operand->kind == PCOp_FPR && matching_operand->value.reg == matching_register) {
                            if (matching_operand->flags & PCodeOperand_Use)
                                use_count++;
                            if (matching_operand->flags & PCodeOperand_Definition)
                                definition_count++;
                            matching_operand->value.reg = spill_register;
                        }
                    }
                    if (use_count)
                        PCode_InsertInstructionBefore(instruction, spill_load_fpr(spill_register, node));
                    if (definition_count)
                        PCode_InsertInstructionAfter(instruction, spill_store_fpr(spill_register, node));
                }
            }
        }
    }
}

void SpillCode_RewriteSpilledVR(PCodeBlock *block, PCodeInstruction *instruction)
{
    int original_register_count;
    PCodeOperand *scan;
    int scan_index;
    int spilled_register;
    InterferenceNode *node;
    PCodeOperand *operand;
    int operand_index;
    int new_register;
    short replacement_register;
    int uses;
    int definitions;

    operand_index = 0;
    original_register_count = gUsedVirtualRegistersVR;
    operand = instruction->operandData.operands;
    while (operand_index < instruction->operand_count) {
        if (operand->kind == PCOp_VR && operand->value.reg < original_register_count) {
            node = gInterferenceGraph[spilled_register = operand->value.reg];
            if ((node->flags & 1) != 0) {
                new_register = gUsedVirtualRegistersVR++;
                replacement_register = new_register;
                uses = 0;
                definitions = 0;
                scan_index = operand_index;
                scan = operand;
                while (scan_index < instruction->operand_count) {
                    if (scan->kind == PCOp_VR && scan->value.reg == spilled_register) {
                        if ((scan->flags & PCodeOperand_Use) != 0)
                            uses++;
                        if ((scan->flags & PCodeOperand_Definition) != 0)
                            definitions++;
                        scan->value.reg = replacement_register;
                    }
                    scan_index++;
                    scan++;
                }
                if (uses != 0)
                    spill_load_vr(instruction, replacement_register, node);
                if (definitions != 0)
                    spill_store_vr(instruction, 0, replacement_register, node);
            }
        }
        operand_index++;
        operand++;
    }
}

void SpillCode_RewriteSpilledRegisterMove(void *unused, PCodeInstruction *pc)
{
    int temp;
    InterferenceNode *source;
    InterferenceNode *target;
    PCodeInstruction *insn;

    source = gInterferenceGraph[pc->operandData.operands[1].value.reg];
    target = gInterferenceGraph[pc->operandData.operands[0].value.reg];
    if ((source->flags & 1) != 0) {
        if ((target->flags & 1) != 0) {
            temp = gUsedVirtualRegistersGPR++;
            insn = spill_load(temp, source);
            PCode_InsertInstructionBefore(pc, insn);
            insn = spill_store(temp, target);
            PCode_InsertInstructionBefore(pc, insn);
        } else {
            insn = spill_load(pc->operandData.operands[0].value.reg, source);
            PCode_InsertInstructionBefore(pc, insn);
        }
    } else {
        insn = spill_store(pc->operandData.operands[1].value.reg, target);
        PCode_InsertInstructionBefore(pc, insn);
    }
    PCode_UnlinkInstruction(pc);
}

void SpillCode_ReplaceInstructionWithFPRSpillCode(PCodeBlock *block, PCodeInstruction *pc)
{
    int temp;
    InterferenceNode *source;
    InterferenceNode *target;
    PCodeInstruction *insn;

    source = gInterferenceGraph[pc->operandData.operands[1].value.reg];
    target = gInterferenceGraph[pc->operandData.operands[0].value.reg];
    if ((source->flags & 1) != 0) {
        if ((target->flags & 1) != 0) {
            temp = gUsedVirtualRegistersFPR++;
            insn = spill_load_fpr(temp, source);
            PCode_InsertInstructionBefore(pc, insn);
            insn = spill_store_fpr(temp, target);
            PCode_InsertInstructionBefore(pc, insn);
        } else {
            insn = spill_load_fpr(pc->operandData.operands[0].value.reg, source);
            PCode_InsertInstructionBefore(pc, insn);
        }
    } else {
        insn = spill_store_fpr(pc->operandData.operands[1].value.reg, target);
        PCode_InsertInstructionBefore(pc, insn);
    }
    PCode_UnlinkInstruction(pc);
}

void SpillCode_EmitOperandSpills(PCodeBlock *unused, PCodeInstruction *pc)
{
    InterferenceNode *source;
    InterferenceNode *target;
    int temp;

    source = gInterferenceGraph[pc->operandData.operands[1].value.reg];
    target = gInterferenceGraph[pc->operandData.operands[0].value.reg];
    if ((source->flags & 1) != 0) {
        if ((target->flags & 1) != 0) {
            temp = gUsedVirtualRegistersVR++;
            spill_load_vr(pc, temp, source);
            spill_store_vr(pc, 1, temp, target);
        } else {
            spill_load_vr(pc, pc->operandData.operands[0].value.reg, source);
        }
    } else {
        spill_store_vr(pc, 1, pc->operandData.operands[1].value.reg, target);
    }
    PCode_UnlinkInstruction(pc);
}

static void remove_spilled_fixed_operands(PCodeBlock *block, PCodeInstruction *instruction)
{
    const PCodeOperand *source_operand;
    PCodeOperand *destination;
    int operand_index;
    int original_count;

    original_count = instruction->operand_count;
    operand_index = 50;
    destination = &instruction->operandData.operands[operand_index];
    source_operand = destination;
    while (operand_index < original_count) {
        if (source_operand->kind == PCOp_GPR && (gInterferenceGraph[source_operand->value.reg]->flags & 1) != 0) {
            instruction->operand_count -= 1;
        } else {
            *destination = *source_operand;
            destination++;
        }
        operand_index++;
        source_operand = source_operand + 1;
    }
}

void SpillCode_SpillRegisters(int reg_class, int register_count)
{
    InterferenceNode *node;
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    int needs_spill;
    Type *type;
    int operand_count;
    unsigned int register_index;
    PCodeBlock *block;
    PCodeInstruction *next_instruction;

    spill_address_register = 0;
    for (register_index = 32; register_index < register_count; register_index++) {
        node = gInterferenceGraph[register_index];
        if ((node->flags & 4) == 0 && (node->flags & 1) != 0) {
            if (node->object == NULL) {
                if (reg_class == 0) {
                    type = (Type *)&stunsignedlong;
                } else if (reg_class == 1) {
                    type = (Type *)&stdouble;
                } else {
                    type = TYPE(&stvectorunsignedchar);
                }
                node->object = make_spill_temporary(type);
            }
            if (node->object->datatype == DLOCAL && Registers_GetInfo(node->object)->in_param_area == 0) {
                StackFrameEABI_AllocateObjectSlot(node->object);
            }
            Registers_GetInfo(node->object)->reg = 0;
        }
    }
    block = gPCodeBlocks;
    while (block != NULL) {
        instruction = block->instructions;
        while (instruction != NULL) {
            needs_spill = 0;
            next_instruction = instruction->next;
            operand = instruction->operandData.operands;
            operand_count = instruction->operand_count;
            while (operand_count--) {
                if (operand->kind == reg_class) {
                    node = gInterferenceGraph[operand->value.reg];
                    if ((node->flags & 1) != 0)
                        needs_spill = 1;
                }
                operand++;
            }
            if (needs_spill != 0) {
                if (reg_class == 9 && spill_address_register == 0) {
                    spill_address_register = gUsedVirtualRegistersGPR;
                    gUsedVirtualRegistersGPR += 1;
                }
                if (instruction->opcode == PC_MR) {
                    SpillCode_RewriteSpilledRegisterMove(block, instruction);
                } else if (instruction->opcode == PC_FMR) {
                    SpillCode_ReplaceInstructionWithFPRSpillCode(block, instruction);
                } else if (instruction->opcode == PC_VMR) {
                    SpillCode_EmitOperandSpills(block, instruction);
                } else if ((instruction->flags & PCodeInstruction_GPRFixedRange) != 0) {
                    remove_spilled_fixed_operands(block, instruction);
                } else if (reg_class == 0) {
                    SpillCode_InsertGPRSpillCode(block, instruction);
                } else if (reg_class == 1) {
                    SpillCode_RewriteSpilledFPRs(block, instruction);
                } else if (reg_class == 9) {
                    SpillCode_RewriteSpilledVR(block, instruction);
                }
            }
            instruction = next_instruction;
        }
        block = block->next;
    }
}
