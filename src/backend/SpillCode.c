#define CERROR_FILE "SpillCode.c"
#include "compiler/common.h"
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
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
/* Declarations gathered from the merged files. */


short spill_address_register = 0;

static void EmitSpill(PCodeInstruction *op, InterferenceNode *node, short reg, int opcode);

static inline void SC_InsertLoad(PCodeInstruction *instruction, InterferenceNode *node, short replacement_register)
{
    PCodeInstruction *load;
    PCodeInstruction *load_address;
    CError_ASSERT(184, node->object->datatype == DLOCAL);
    load_address = PCodeUtilities_CreateInstruction(63, spill_address_register, stack_base_reg, node->object, 0);
    load = PCodeUtilities_CreateInstruction(247, replacement_register, 0, spill_address_register);
    PCode_InsertInstructionBefore(instruction, load_address);
    PCode_InsertInstructionAfter(load_address, load);
}

static inline PCodeInstruction *spill_load(short reg, InterferenceNode *node)
{
    Operand operand;
    short opcode;
    Type *type;
    type = node->object->type;
    opcode = type->size == 1 ? 21 : type->size == 2 ? (is_unsigned(type) ? 25 : 29) : 34;
    memclrw(&operand, 22);
    operand.kind = OpndType_Symbol;
    operand.object = node->object;
    if (node->object->datatype != DLOCAL)
        CError_FATAL(130);
    Operands_Normalize(&operand);
    if (operand.kind != OpndType_GPR_ImmOffset)
        CError_FATAL(140);
    return PCodeUtilities_CreateInstruction(opcode, reg, operand.reg, node->object,
                                            (node->flags & 32) ? low_word_offset
                                                               : ((node->flags & 16) ? high_word_offset : 0));
}

static inline PCodeInstruction *spill_store(short reg, InterferenceNode *node)
{
    int flags;
    Type *type;
    short opcode;
    type = node->object->type;
    flags = (node->flags & 32) ? low_word_offset : ((node->flags & 16) ? high_word_offset : 0);
    if (type->size == 1)
        opcode = 40;
    else if (type->size == 2)
        opcode = 44;
    else
        opcode = 49;
    return PCodeUtilities_CreateInstruction(opcode, reg, stack_base_reg, node->object, flags);
}

static inline void SpillCode_SetAddress(Operand *address, InterferenceNode *node)
{
    address->kind = OpndType_Symbol;
    address->object = node->object;
    if (node->object->datatype != DLOCAL)
        CError_FATAL(130);
}

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
    Type *type;
    int opcode;
    int spill_register;
    int base_register;
    Operand address;

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
            if (uses != 0) {
                type = node->object->type;
                if (type->size == 1)
                    opcode = 21;
                else if (type->size == 2) {
                    if (is_unsigned(type) != 0)
                        opcode = 25;
                    else
                        opcode = 29;
                } else
                    opcode = 34;
                memclrw(&address, sizeof(address));
                SpillCode_SetAddress(&address, node);
                Operands_Normalize(&address);
                if (address.kind != OpndType_GPR_ImmOffset)
                    CError_FATAL(140);
                if (node->flags & 32)
                    base_register = low_word_offset;
                else if (node->flags & 16)
                    base_register = high_word_offset;
                else
                    base_register = 0;
                PCode_InsertInstructionBefore(
                    instruction, PCodeUtilities_CreateInstruction(opcode, (short)spill_register, address.reg,
                                                                  node->object, base_register));
            }
            if (definitions != 0) {
                type = node->object->type;
                if (node->flags & 32)
                    base_register = low_word_offset;
                else if (node->flags & 16)
                    base_register = high_word_offset;
                else
                    base_register = 0;
                if (type->size == 1)
                    opcode = 40;
                else if (type->size == 2)
                    opcode = 44;
                else
                    opcode = 49;
                PCode_InsertInstructionAfter(
                    instruction, PCodeUtilities_CreateInstruction(opcode, (short)spill_register, stack_base_reg,
                                                                  node->object, base_register));
            }
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
    Type *type;
    int base_register;
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
                    if (use_count) {
                        type = node->object->type;
                        CError_ASSERT(165, node->object->datatype == DLOCAL);
                        if (node->flags & 32)
                            base_register = low_word_offset;
                        else if (node->flags & 16)
                            base_register = high_word_offset;
                        else
                            base_register = 0;
                        PCode_InsertInstructionBefore(
                            instruction,
                            PCodeUtilities_CreateInstruction(type->size == 8 ? 146 : 142, (short)spill_register,
                                                             stack_base_reg, node->object, base_register));
                    }
                    if (definition_count) {
                        PCode_InsertInstructionAfter(
                            instruction,
                            PCodeUtilities_CreateInstruction(node->object->type->size == 8 ? 154 : 150,
                                                             (short)spill_register, stack_base_reg, node->object, 0));
                    }
                }
            }
        }
    }
}

void SpillCode_RewriteSpilledVR(PCodeBlock *block, PCodeInstruction *instruction)
{
    struct RegisterOperand {
        char kind;
        signed char flags;
        short virtual_register;
        char operand_data[8];
    };
    int original_register_count;
    struct RegisterOperand *scan;
    int scan_index;
    int spilled_register;
    PCodeInstruction *load;
    PCodeInstruction *load_address;
    PCodeInstruction *store;
    PCodeInstruction *store_address;
    InterferenceNode *node;
    struct RegisterOperand *operand;
    int operand_index;
    int new_register;
    short replacement_register;
    int uses;
    int definitions;
    operand = (struct RegisterOperand *)instruction;
    operand_index = 0;
    original_register_count = gUsedVirtualRegistersVR;
    operand = (struct RegisterOperand *)instruction->operandData.operands;
    while (operand_index < instruction->operand_count) {
        if (operand->kind == 9 && operand->virtual_register < original_register_count) {
            node = gInterferenceGraph[spilled_register = operand->virtual_register];
            if ((node->flags & 1) != 0) {
                new_register = gUsedVirtualRegistersVR++;
                replacement_register = new_register;
                uses = 0;
                definitions = 0;
                scan_index = operand_index;
                scan = operand;
                while (scan_index < instruction->operand_count) {
                    if (scan->kind == 9 && scan->virtual_register == spilled_register) {
                        if ((scan->flags & 1) != 0) {
                            uses++;
                        }
                        if ((scan->flags & 2) != 0) {
                            definitions++;
                        }
                        scan->virtual_register = replacement_register;
                    }
                    scan_index++;
                    scan++;
                }
                if (uses != 0) {
                    SC_InsertLoad(instruction, node, replacement_register);
                }
                if (definitions != 0) {
                    store_address =
                        PCodeUtilities_CreateInstruction(63, spill_address_register, stack_base_reg, node->object, 0);
                    store = PCodeUtilities_CreateInstruction(252, replacement_register, 0, spill_address_register);
                    PCode_InsertInstructionAfter(instruction, store_address);
                    PCode_InsertInstructionAfter(store_address, store);
                }
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

void SpillCode_ReplaceInstructionWithFPRSpillCode(PCodeBlock *block, PCodeInstruction *instruction)
{
    int temporaryRegister;
    InterferenceNode *destination;
    InterferenceNode *source;
    Type *destinationType;
    int destinationOffset;
    Type *reloadType;
    int reloadOffset;
    short sourceRegister;

    destination = gInterferenceGraph[instruction->operandData.operands[1].value.reg];
    source = gInterferenceGraph[sourceRegister = instruction->operandData.operands[0].value.reg];
    if ((destination->flags & 1) != 0) {
        if ((source->flags & 1) != 0) {
            temporaryRegister = gUsedVirtualRegistersFPR;
            gUsedVirtualRegistersFPR++;
            destinationType = destination->object->type;
            CError_ASSERT(165, destination->object->datatype == DLOCAL);
            if ((destination->flags & 32) != 0) {
                destinationOffset = low_word_offset;
            } else if ((destination->flags & 16) != 0) {
                destinationOffset = high_word_offset;
            } else {
                destinationOffset = 0;
            }
            PCode_InsertInstructionBefore(instruction,
                                          PCodeUtilities_CreateInstruction(destinationType->size == 8 ? 146 : 142,
                                                                           (short)temporaryRegister, stack_base_reg,
                                                                           destination->object, destinationOffset));
            PCode_InsertInstructionBefore(instruction,
                                          PCodeUtilities_CreateInstruction(source->object->type->size == 8 ? 154 : 150,
                                                                           (short)temporaryRegister, stack_base_reg,
                                                                           source->object, 0));
        } else {
            reloadType = destination->object->type;
            CError_ASSERT(165, destination->object->datatype == DLOCAL);
            if ((destination->flags & 32) != 0) {
                reloadOffset = low_word_offset;
            } else if ((destination->flags & 16) != 0) {
                reloadOffset = high_word_offset;
            } else {
                reloadOffset = 0;
            }
            PCode_InsertInstructionBefore(
                instruction, PCodeUtilities_CreateInstruction(reloadType->size == 8 ? 146 : 142, sourceRegister,
                                                              stack_base_reg, destination->object, reloadOffset));
        }
    } else {
        PCode_InsertInstructionBefore(instruction,
                                      PCodeUtilities_CreateInstruction(source->object->type->size == 8 ? 154 : 150,
                                                                       instruction->operandData.operands[1].value.reg,
                                                                       stack_base_reg, source->object, 0));
    }
    PCode_UnlinkInstruction(instruction);
}

void SpillCode_EmitOperandSpills(PCodeBlock *unused, PCodeInstruction *op)
{
    InterferenceNode *sourceNode;
    short sourceReg;
    short destinationReg;
    InterferenceNode *destinationNode;

    sourceReg = op->operandData.operands[1].value.reg;
    (void)sourceReg;
    sourceNode = gInterferenceGraph[sourceReg];
    destinationReg = op->operandData.operands[0].value.reg;
    (void)destinationReg;
    destinationNode = gInterferenceGraph[destinationReg];

    if (sourceNode->flags & 1) {
        if (destinationNode->flags & 1) {
            int spillReg = gUsedVirtualRegistersVR;
            gUsedVirtualRegistersVR++;
            CError_ASSERT(184, sourceNode->object->datatype == DLOCAL);
            EmitSpill(op, sourceNode, spillReg, 0xf7);
            EmitSpill(op, destinationNode, spillReg, 0xfc);
        } else {
            CError_ASSERT(184, sourceNode->object->datatype == DLOCAL);
            EmitSpill(op, sourceNode, destinationReg, 0xf7);
        }
    } else {
        EmitSpill(op, destinationNode, sourceReg, 0xfc);
    }
    PCode_UnlinkInstruction(op);
}

static void EmitSpill(PCodeInstruction *op, InterferenceNode *node, short reg, int opcode)
{
    PCodeInstruction *t1;
    PCodeInstruction *t2;
    t1 = PCodeUtilities_CreateInstruction(0x3f, spill_address_register, stack_base_reg, node->object, 0);
    t2 = PCodeUtilities_CreateInstruction(opcode, reg, 0, spill_address_register);
    PCode_InsertInstructionBefore(op, t1);
    PCode_InsertInstructionAfter(t1, t2);
}
