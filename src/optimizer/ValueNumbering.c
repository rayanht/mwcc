#define CERROR_FILE "ValueNumbering.c"
#include "compiler/common.h"
#include "compiler/ValueNumbering.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/CopyPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/StrengthReduction.h"
#include "compiler/Switch.h"

static struct RegisterValueRecord *register_values_by_opcode[466];
static struct RegisterValueState *register_value_state_array;
static struct RegisterValueState *fpr_value_states;
static struct RegisterValueState *gRegisterValueStates;
static struct RegisterValueState *register_value_states;
static struct ObjectIndexEntry *objectIndex;
static struct ObjectIndexEntry *object_indices;
static void *data_00582c38;
static SInt32 next_value_index;
static SInt32 value_index_threshold;
static struct Object *data_00582c44;
static unsigned int data_00582c48;
/* 12-byte operand descriptor. */

void ValueNumbering_PerformValueNumbering(int options)
{
    struct PCodeBlock *block;
    gValueNumberingChanged = 0;
    data_00582c48 = options;
    fn_0051ffc0();
    register_value_state_array =
        CompilerTools_AllocatePoolMemory(gUsedVirtualRegistersGPR * sizeof(*register_value_state_array));
    fpr_value_states = CompilerTools_AllocatePoolMemory(gUsedVirtualRegistersFPR * sizeof(*fpr_value_states));
    gRegisterValueStates = CompilerTools_AllocatePoolMemory(gUsedVirtualRegistersVR * sizeof(*gRegisterValueStates));
    register_value_states = CompilerTools_AllocatePoolMemory(8 * sizeof(*register_value_states));
    for (block = gPCodeBlocks; block != NULL; block = block->next)
        block->flags &= ~4;
    for (block = gPCodeBlocks; block != NULL; block = block->next)
        if ((block->flags & 4) == 0) {
            initialize_value_states();
            traverse_single_predecessor_successors(block);
        }
    CompilerTools_ResetPool();
}

void traverse_single_predecessor_successors(PCodeBlock *node)
{
    struct RegisterValueSnapshot *savedSnapshots;
    SInt32 savedIndex;
    PCodeBlock *child;
    PCodeBlockLink *predecessors;
    SInt32 eligibleCount;
    PCodeBlockLink *successors;
    PCodeBlockLink *entry;

    value_number_block(node);
    while ((successors = node->successors) != NULL && successors->next == NULL &&
           (predecessors = (child = successors->payload.block)->predecessors) != NULL && predecessors->next == NULL) {
        value_number_block(node = child);
    }

    eligibleCount = 0;
    for (entry = successors; entry != NULL; entry = entry->next) {
        if ((entry->payload.block->flags & 4) == 0 && entry->payload.block->predecessors != NULL &&
            entry->payload.block->predecessors->next == NULL)
            eligibleCount++;
    }

    if (eligibleCount == 0)
        return;

    savedSnapshots = data_00582c38;
    savedIndex = value_index_threshold;
    data_00582c38 = NULL;
    value_index_threshold = next_value_index;

    for (entry = node->successors; entry != NULL; entry = entry->next) {
        if ((entry->payload.block->flags & 4) == 0 && entry->payload.block->predecessors != NULL &&
            entry->payload.block->predecessors->next == NULL) {
            traverse_single_predecessor_successors(entry->payload.block);
            fn_0051e720();
        }
    }

    data_00582c38 = savedSnapshots;
    value_index_threshold = savedIndex;
}

void fn_0051e720(void)
{
    ValueUpdate *entry;
    RegisterValueRecord *value;
    RegisterValueRecord **link;
    int bucketIndex;
    RegisterValueRecord **bucket;

    bucketIndex = 0;
    bucket = register_values_by_opcode;
    do {
        link = bucket;
        while ((value = *link) != NULL) {
            if (value->index >= value_index_threshold) {
                *link = value->next;
            } else {
                link = &value->next;
            }
        }
        ++bucketIndex;
        ++bucket;
    } while (bucketIndex < 0x1d2);
    for (entry = data_00582c38; entry != NULL; entry = entry->next) {
        fn_0051fd70(entry);
    }
}

static inline int no_dup(PCodeInstruction *inst)
{
    if (inst->flags & (fRecordBit | fSetsCarry | fOverflow))
        return 0;
    {
        PCodeOperand *arg;
        PCodeOperand *args;
        SInt32 i;
        i = 1;
        args = inst->operandData.operands;
        for (arg = args + 1; i < inst->operand_count; i++, arg++) {
            if (arg->kind == args[0].kind && arg->value.reg == args[0].value.reg)
                return 0;
        }
        return 1;
    }
}

static inline int arraystruct(Type *t)
{
    return t->type == TYPEARRAY || t->type == TYPESTRUCT;
}

static inline int classaggregate(Type *t)
{
    return arraystruct(t) || t->type == TYPECLASS;
}

static inline int memberaggregate(Type *t)
{
    return t->type == TYPEMEMBERPOINTER && t->size == 12;
}

static inline int aggregate(Type *t)
{
    return classaggregate(t) || memberaggregate(t);
}

static inline RegisterValueState *getreg(PCodeOperand *arg)
{
    RegisterValueState *reg;
    switch (arg->kind) {
        case PCOp_GPR:
            reg = &register_value_state_array[arg->value.reg];
            break;
        case PCOp_FPR:
            reg = &fpr_value_states[arg->value.reg];
            break;
        case PCOp_VR:
            reg = &gRegisterValueStates[arg->value.reg];
            break;
        case PCOp_CRFIELD:
            reg = &register_value_states[arg->value.reg];
            break;
    }
    return reg;
}

static inline void invalidate(PCodeInstruction *inst)
{
    ValueUpdate *rec;
    ValueRegisterOperand *node;
    ValueRegisterOperand **link;
    RegisterValueState *reg;
    reg = getreg(&inst->operandData.operands[1]);
    if (reg->index < value_index_threshold && next_value_index >= value_index_threshold) {
        rec = (ValueUpdate *)CompilerTools_AllocatePoolMemory(0x1c);
        rec->next = data_00582c38;
        data_00582c38 = rec;
        rec->descriptor = inst->operandData.operands[1];
        rec->value = *reg;
    }
    if (reg->value != NULL) {
        link = &reg->value->operands;
        while ((node = *link) != NULL) {
            if (node->operand.value.reg == inst->operandData.operands[1].value.reg) {
                *link = node->next;
            } else {
                link = &node->next;
            }
        }
    }
    reg->value = NULL;
    reg->index = next_value_index;
    next_value_index++;
}

void value_number_block(PCodeBlock *block)
{
    PCodeInstruction *inst;
    PCodeInstruction *next;
    for (inst = block->instructions; inst != NULL; inst = next) {
        next = inst->next;
        switch (inst->opcode) {
            case PC_BL:
            case PC_BTLR:
            case PC_BTCTR:
            case PC_BFLR:
            case PC_BFCTR:
            case PC_BLR:
            case PC_BCTR:
            case PC_BCTRL:
            case PC_BLRL:
            case PC_TW:
            case PC_TRAP:
            case PC_TWI:
                invalidate_register_values(inst);
                break;
            case PC_MR:
            case PC_FMR:
            case PC_VMR:
                copy_register_value_state(&inst->operandData.operands[1], &inst->operandData.operands[0]);
                break;
            case PC_LBZ:
            case PC_LHZ:
            case PC_LHA:
            case PC_LWZ:
            case PC_LFS:
            case PC_LFD:
                if (inst->flags & fIsPtrOp)
                    value_number_single_instruction(inst);
                else if (aggregate(inst->operandData.operands[2].object->type))
                    value_number_pcode_instruction(inst);
                else
                    value_number_instruction(inst);
                break;
            case PC_STB:
            case PC_STH:
            case PC_STW:
            case PC_STFS:
            case PC_STFD:
                if (inst->flags & fIsPtrOp)
                    invalidate_object_indices(NULL, 1);
                else if (aggregate(inst->operandData.operands[2].object->type))
                    assign_object_value_index(inst);
                else
                    assign_object_value_index(inst);
                break;
            case PC_LBZU:
            case PC_LBZUX:
            case PC_LHZU:
            case PC_LHZUX:
            case PC_LHAU:
            case PC_LHAUX:
            case PC_LWZU:
            case PC_LWZUX:
            case PC_LFSU:
            case PC_LFSUX:
            case PC_LFDU:
            case PC_LFDUX: {
                RegisterValueState *reg;
                RegisterValueRecord *value;
                ValueRegisterOperand **link;
                ValueRegisterOperand *node;
                ValueUpdate *spill;
                switch (inst->operandData.operands[1].kind) {
                    case PCOp_GPR:
                        reg = &register_value_state_array[inst->operandData.operands[1].value.reg];
                        break;
                    case PCOp_FPR:
                        reg = &fpr_value_states[inst->operandData.operands[1].value.reg];
                        break;
                    case PCOp_VR:
                        reg = &gRegisterValueStates[inst->operandData.operands[1].value.reg];
                        break;
                    case PCOp_CRFIELD:
                        reg = &register_value_states[inst->operandData.operands[1].value.reg];
                        break;
                }
                if (reg->index < value_index_threshold && next_value_index >= value_index_threshold) {
                    spill = (ValueUpdate *)CompilerTools_AllocatePoolMemory(sizeof(ValueUpdate));
                    spill->next = data_00582c38;
                    data_00582c38 = spill;
                    spill->descriptor = inst->operandData.operands[1];
                    spill->value = *reg;
                }
                if ((value = reg->value) != NULL) {
                    link = &value->operands;
                    while ((node = *link) != NULL) {
                        if (node->operand.value.reg == inst->operandData.operands[1].value.reg)
                            *link = node->next;
                        else
                            link = &node->next;
                    }
                }
                reg->value = NULL;
                reg->index = next_value_index;
                next_value_index++;
            }
            case PC_LBZX:
            case PC_LHZX:
            case PC_LHAX:
            case PC_LHBRX:
            case PC_LWZX:
            case PC_LWBRX:
            case PC_LFSX:
            case PC_LFDX:
            case PC_LVEBX:
            case PC_LVEHX:
            case PC_LVEWX:
            case PC_LVX:
            case PC_LVXL:
                value_number_single_instruction(inst);
                break;
            case PC_STBU:
            case PC_STBUX:
            case PC_STHU:
            case PC_STHUX:
            case PC_STWU:
            case PC_STWUX:
            case PC_STFSU:
            case PC_STFSUX:
            case PC_STFDU:
            case PC_STFDUX:
            case PC_STVEBX:
            case PC_STVEHX:
            case PC_STVEWX:
            case PC_STVX:
            case PC_STVXL:
                invalidate(inst);
            case PC_STBX:
            case PC_STHX:
            case PC_STHBRX:
            case PC_STWX:
            case PC_STWBRX:
            case PC_STMW:
            case PC_STFSX:
            case PC_STFDX:
                invalidate_object_indices(NULL, 1);
                break;
            case PC_FNMSUBS:
            case PC_ADD:
            case PC_ADDC:
            case PC_ADDI:
            case PC_ADDIC:
            case PC_ADDIS:
            case PC_DIVW:
            case PC_DIVWU:
            case PC_MULHW:
            case PC_MULHWU:
            case PC_MULLI:
            case PC_MULLW:
            case PC_NEG:
            case PC_SUBF:
            case PC_SUBFC:
            case PC_SUBFIC:
            case PC_ANDI:
            case PC_ANDIS:
            case PC_ORI:
            case PC_ORIS:
            case PC_XORI:
            case PC_XORIS:
            case PC_AND:
            case PC_OR:
            case PC_XOR:
            case PC_NAND:
            case PC_NOR:
            case PC_EQV:
            case PC_ANDC:
            case PC_ORC:
            case PC_EXTSB:
            case PC_EXTSH:
            case PC_CNTLZW:
            case PC_RLWINM:
            case PC_RLWNM:
            case PC_SLW:
            case PC_SRW:
            case PC_SRAWI:
            case PC_SRAW:
            case PC_CRAND:
            case PC_CRANDC:
            case PC_CREQV:
            case PC_CRNAND:
            case PC_CRNOR:
            case PC_CROR:
            case PC_CRORC:
            case PC_CRXOR:
            case PC_NOT:
            case PC_FABS:
            case PC_FNEG:
            case PC_FNABS:
            case PC_FADD:
            case PC_FADDS:
            case PC_FSUB:
            case PC_FSUBS:
            case PC_FMUL:
            case PC_FMULS:
            case PC_FDIV:
            case PC_FDIVS:
            case PC_FMADD:
            case PC_FMADDS:
            case PC_FMSUB:
            case PC_FMSUBS:
            case PC_FNMADD:
            case PC_FNMADDS:
            case PC_FNMSUB:
            case PC_FRSP:
            case PC_FCTIW:
            case PC_FCTIWZ:
            case PC_VADDCUW:
            case PC_VADDFP:
            case PC_VADDSBS:
            case PC_VADDSHS:
            case PC_VADDSWS:
            case PC_VADDUBM:
            case PC_VADDUBS:
            case PC_VADDUHM:
            case PC_VADDUHS:
            case PC_VADDUWM:
            case PC_VADDUWS:
            case PC_VAND:
            case PC_VANDC:
            case PC_VAVGSB:
            case PC_VAVGSH:
            case PC_VAVGSW:
            case PC_VAVGUB:
            case PC_VAVGUH:
            case PC_VAVGUW:
            case PC_VCFSX:
            case PC_VCFUX:
            case PC_VCTSXS:
            case PC_VCTUXS:
            case PC_VEXPTEFP:
            case PC_VLOGEFP:
            case PC_VMAXFP:
            case PC_VMAXSB:
            case PC_VMAXSH:
            case PC_VMAXSW:
            case PC_VMAXUB:
            case PC_VMAXUH:
            case PC_VMAXUW:
            case PC_VMINFP:
            case PC_VMINSB:
            case PC_VMINSH:
            case PC_VMINSW:
            case PC_VMINUB:
            case PC_VMINUH:
            case PC_VMINUW:
            case PC_VMRGHB:
            case PC_VMRGHH:
            case PC_VMRGHW:
            case PC_VMRGLB:
            case PC_VMRGLH:
            case PC_VMRGLW:
            case PC_VMULESB:
            case PC_VMULESH:
            case PC_VMULEUB:
            case PC_VMULEUH:
            case PC_VMULOSB:
            case PC_VMULOSH:
            case PC_VMULOUB:
            case PC_VMULOUH:
            case PC_VNOR:
            case PC_VOR:
            case PC_VPKPX:
            case PC_VPKSHSS:
            case PC_VPKSHUS:
            case PC_VPKSWSS:
            case PC_VPKSWUS:
            case PC_VPKUHUM:
            case PC_VPKUHUS:
            case PC_VPKUWUM:
            case PC_VPKUWUS:
            case PC_VREFP:
            case PC_VRFIM:
            case PC_VRFIN:
            case PC_VRFIP:
            case PC_VRFIZ:
            case PC_VRLB:
            case PC_VRLH:
            case PC_VRLW:
            case PC_VRSQRTEFP:
            case PC_VSL:
            case PC_VSLB:
            case PC_VSLH:
            case PC_VSLO:
            case PC_VSLW:
            case PC_VSPLTB:
            case PC_VSPLTH:
            case PC_VSPLTW:
            case PC_VSPLTISB:
            case PC_VSPLTISH:
            case PC_VSPLTISW:
            case PC_VSR:
            case PC_VSRAB:
            case PC_VSRAH:
            case PC_VSRAW:
            case PC_VSRB:
            case PC_VSRH:
            case PC_VSRO:
            case PC_VSRW:
            case PC_VSUBCUW:
            case PC_VSUBFP:
            case PC_VSUBSBS:
            case PC_VSUBSHS:
            case PC_VSUBSWS:
            case PC_VSUBUBM:
            case PC_VSUBUBS:
            case PC_VSUBUHM:
            case PC_VSUBUHS:
            case PC_VSUBUWM:
            case PC_VSUBUWS:
            case PC_VSUMSWS:
            case PC_VSUM2SWS:
            case PC_VSUM4SBS:
            case PC_VSUM4SHS:
            case PC_VSUM4UBS:
            case PC_VUPKHPX:
            case PC_VUPKHSB:
            case PC_VUPKHSH:
            case PC_VUPKLPX:
            case PC_VUPKLSB:
            case PC_VUPKLSH:
            case PC_VXOR:
            case PC_VMADDFP:
            case PC_VMHADDSHS:
            case PC_VMHRADDSHS:
            case PC_VMLADDUHM:
            case PC_VMSUMMBM:
            case PC_VMSUMSHM:
            case PC_VMSUMSHS:
            case PC_VMSUMUBM:
            case PC_VMSUMUHM:
            case PC_VMSUMUHS:
            case PC_VNMSUBFP:
            case PC_VPERM:
            case PC_VSEL:
            case PC_VSLDOI:
                if (no_dup(inst))
                    fn_0051ee40(inst);
                else
                    invalidate_instruction_register_values(inst);
                break;
            case PC_CMPI:
            case PC_CMP:
            case PC_CMPLI:
            case PC_CMPL:
            case PC_FCMPU:
            case PC_FCMPO:
                if (data_00582c48 != 0)
                    fn_0051ee40(inst);
                else
                    invalidate_instruction_register_values(inst);
                break;
            case PC_LI:
            case PC_LIS:
                if (data_00582c48 != 0 && inst->operandData.operands[0].value.reg >= gGPRCoalesceFirst &&
                    inst->operandData.operands[0].value.reg <= gGPRCoalesceLast)
                    fn_0051ee40(inst);
                else
                    invalidate_instruction_register_values(inst);
                break;
            default:
                invalidate_instruction_register_values(inst);
                break;
        }
    }
    block->flags |= 4;
}

void invalidate_instruction_register_values(PCodeInstruction *instruction)
{
    SInt32 operand_index = 0;
    PCodeOperand *operand = instruction->operandData.operands;

    for (; operand_index < instruction->operand_count; operand_index++, operand++) {
        RegisterValueState *state;

        if ((operand->kind == PCOp_GPR || operand->kind == PCOp_FPR || operand->kind == PCOp_VR ||
             operand->kind == PCOp_CRFIELD) &&
            (operand->flags & 2)) {
            switch (operand->kind) {
                case PCOp_GPR:
                    state = &register_value_state_array[operand->value.reg];
                    break;
                case PCOp_FPR:
                    state = &fpr_value_states[operand->value.reg];
                    break;
                case PCOp_VR:
                    state = &gRegisterValueStates[operand->value.reg];
                    break;
                case PCOp_CRFIELD:
                    state = &register_value_states[operand->value.reg];
                    break;
            }

            if (state->index < value_index_threshold && next_value_index >= value_index_threshold) {
                RegisterValueSnapshot *snapshot =
                    (RegisterValueSnapshot *)CompilerTools_AllocatePoolMemory(sizeof(RegisterValueSnapshot));
                snapshot->next = data_00582c38;
                data_00582c38 = snapshot;
                snapshot->operand = *operand;
                snapshot->state = *state;
            }

            if (state->value != NULL) {
                ValueRegisterOperand **link = &state->value->operands;
                while (*link != NULL) {
                    if ((*link)->operand.value.reg == operand->value.reg)
                        *link = (*link)->next;
                    else
                        link = &(*link)->next;
                }
            }

            state->value = NULL;
            state->index = next_value_index;
            next_value_index++;
        }
    }
}

void invalidate_register_values(PCodeInstruction *obj)
{
    SInt32 i = 0;
    PCodeOperand *e = obj->operandData.operands;
    while (i < obj->operand_count) {
        if ((e->kind == PCOp_GPR || e->kind == PCOp_FPR || e->kind == PCOp_VR || e->kind == PCOp_CRFIELD) &&
            ((SInt8)e->flags & 2) != 0) {
            RegisterValueState *t;
            switch (e->kind) {
                case PCOp_GPR:
                    t = &register_value_state_array[e->value.reg];
                    break;
                case PCOp_FPR:
                    t = &fpr_value_states[e->value.reg];
                    break;
                case PCOp_VR:
                    t = &gRegisterValueStates[e->value.reg];
                    break;
                case PCOp_CRFIELD:
                    t = &register_value_states[e->value.reg];
                    break;
            }
            if (t->index < value_index_threshold && next_value_index >= value_index_threshold) {
                ValueUpdate *n = (ValueUpdate *)CompilerTools_AllocatePoolMemory(0x1c);
                n->next = data_00582c38;
                data_00582c38 = n;
                n->descriptor = *e;
                n->value = *t;
            }
            if (t->value != NULL) {
                ValueRegisterOperand **pp = &t->value->operands;
                ValueRegisterOperand *n;
                while ((n = *pp) != NULL) {
                    if (n->operand.value.reg == e->value.reg)
                        *pp = n->next;
                    else
                        pp = &n->next;
                }
            }
            t->value = NULL;
            t->index = next_value_index;
            next_value_index++;
        }
        i++;
        e++;
    }
    invalidate_object_indices(NULL, 1);
}

void fn_0051ee40(PCodeInstruction *instruction)
{
    PCodeOperand matchingOperand;
    PCodeOperand *destination = &instruction->operandData.operands[0];

    if (find_matching_value_signature(instruction, &matchingOperand)) {
        RegisterValueState *matchingState;
        RegisterValueState *destinationState;

        switch (destination->kind) {
            case PCOp_GPR:
                destinationState = &register_value_state_array[destination->value.reg];
                matchingState = &register_value_state_array[matchingOperand.value.reg];
                break;
            case PCOp_FPR:
                destinationState = &fpr_value_states[destination->value.reg];
                matchingState = &fpr_value_states[matchingOperand.value.reg];
                break;
            case PCOp_VR:
                destinationState = &gRegisterValueStates[destination->value.reg];
                matchingState = &gRegisterValueStates[matchingOperand.value.reg];
                break;
            case PCOp_CRFIELD:
                destinationState = &register_value_states[destination->value.reg];
                matchingState = &register_value_states[matchingOperand.value.reg];
                break;
        }

        if (destinationState->index != matchingState->index) {
            PCodeInstruction *copyInstruction;

            if (matchingOperand.kind == PCOp_GPR)
                copyInstruction =
                    PCodeUtilities_CreateInstruction(0x8b, destination->value.reg, matchingOperand.value.reg);
            else if (matchingOperand.kind == PCOp_FPR)
                copyInstruction =
                    PCodeUtilities_CreateInstruction(0x9e, destination->value.reg, matchingOperand.value.reg);
            else if (matchingOperand.kind == PCOp_VR)
                copyInstruction =
                    PCodeUtilities_CreateInstruction(0x18e, destination->value.reg, matchingOperand.value.reg);
            else
                copyInstruction =
                    PCodeUtilities_CreateInstruction(0x76, destination->value.reg, matchingOperand.value.reg);

            PCode_InsertInstructionBefore(instruction, copyInstruction);
            copy_register_value_state(&matchingOperand, destination);
        }

        PCode_UnlinkInstruction(instruction);
        gValueNumberingChanged = 1;
    } else {
        create_register_value_record(instruction);
    }
}

void value_number_single_instruction(PCodeInstruction *argument)
{
    value_number_instruction(argument);
}

void value_number_pcode_instruction(PCodeInstruction *value)
{
    value_number_instruction(value);
}

static inline ValueRegisterOperand *value_signature_list(RegisterValueRecord *node)
{
    return node->operands;
}

static inline void copy_value_signature(RegisterValueRecord *node, PCodeOperand *out)
{
    SInt32 id = node->instruction->operandData.operands[0].value.reg;
    ValueRegisterOperand *item;
    for (item = value_signature_list(node); item != NULL; item = item->next) {
        if (item->operand.value.reg == id) {
            *out = item->operand;
            return;
        }
    }
    *out = value_signature_list(node)->operand;
}

static inline ObjectIndexEntry *fn_0051efd0_inline1(PCodeOperand *operand)
{
    unsigned int value;
    unsigned int key;
    ObjectIndexEntry *record;
    record = objectIndex;
    key = (unsigned int)operand->object;
    while ((int)record != 0) {
        if (key < (unsigned int)(value = (unsigned int)record->object))
            record = record->left;
        else if (key > value)
            record = record->right;
        else
            return record;
    }
    return NULL;
}

void assign_object_value_index(PCodeInstruction *entry)
{
    PCodeInstruction *instruction = entry;
    PCodeOperand *operand;
    RegisterValueState *record_index;
    Object *object;
    ObjectIndexEntry *record;
    int instruction_flags;
    struct OperandIndexSnapshot {
        struct OperandIndexSnapshot *next;
        char kind;
        char padding[11];
        ObjectIndexEntry *record;
        RegisterValueState previous_index;
    } *snapshot;
    instruction_flags = instruction->flags;
    operand = &instruction->operandData.operands[2];
    object = (Object *)instruction->operandData.operands[2].object;
    if ((instruction_flags & PCodeInstruction_ObjectFlag2) != 0) {
        return;
    }
    if (object->datatype == DDATA || PCodeUtilities_Require(object) != 0 ||
        object->datatype == DLOCAL && (object->u.data.u.string[34] != 0 || object->type->type == TYPEARRAY ||
                                       (object->type->type == TYPESTRUCT || object->type->type == TYPECLASS) ||
                                       object->type->type == TYPEMEMBERPOINTER && object->type->size == 12)) {
        invalidate_object_indices(object->type, 0);
    }
    record = fn_0051efd0_inline1(operand);
    record_index = &record->index;
    if (record->index.index < value_index_threshold && next_value_index >= value_index_threshold) {
        snapshot = (struct OperandIndexSnapshot *)CompilerTools_AllocatePoolMemory(28);
        snapshot->next = data_00582c38;
        data_00582c38 = snapshot;
        snapshot->kind = 5;
        snapshot->record = record;
        snapshot->previous_index = *record_index;
    }
    record_index->index = next_value_index;
    next_value_index += 1;
    record_index->value = NULL;
}

void value_number_instruction(PCodeInstruction *instruction)
{
    PCodeOperand *destination = &instruction->operandData.operands[0];

    if (instruction->flags & fIsVolatile) {
        RegisterValueState *state;
        RegisterValueRecord *value;
        switch (destination->kind) {
            case PCOp_GPR:
                state = register_value_state_array + destination->value.reg;
                break;
            case PCOp_FPR:
                state = fpr_value_states + destination->value.reg;
                break;
            case PCOp_VR:
                state = gRegisterValueStates + destination->value.reg;
                break;
            case PCOp_CRFIELD:
                state = register_value_states + destination->value.reg;
                break;
        }
        if (state->index < value_index_threshold && next_value_index >= value_index_threshold) {
            RegisterValueSnapshot *snapshot =
                (RegisterValueSnapshot *)CompilerTools_AllocatePoolMemory(sizeof(RegisterValueSnapshot));
            snapshot->next = data_00582c38;
            data_00582c38 = snapshot;
            snapshot->operand = *destination;
            snapshot->state = *state;
        }
        if ((value = state->value) != NULL) {
            ValueRegisterOperand **link = &value->operands;
            ValueRegisterOperand *operand;
            while ((operand = *link) != NULL) {
                if (operand->operand.value.reg == destination->value.reg)
                    *link = operand->next;
                else
                    link = &operand->next;
            }
        }
        state->value = NULL;
        state->index = next_value_index;
        next_value_index = next_value_index + 1;
    } else {
        PCodeOperand matchingOperand;
        if (find_matching_value_signature(instruction, &matchingOperand) != 0) {
            RegisterValueState *matchingState, *destinationState;
            switch (destination->kind) {
                case PCOp_GPR:
                    destinationState = register_value_state_array + destination->value.reg;
                    matchingState = register_value_state_array + matchingOperand.value.reg;
                    break;
                case PCOp_FPR:
                    destinationState = fpr_value_states + destination->value.reg;
                    matchingState = fpr_value_states + matchingOperand.value.reg;
                    break;
                case PCOp_VR:
                    destinationState = gRegisterValueStates + destination->value.reg;
                    matchingState = gRegisterValueStates + matchingOperand.value.reg;
                    break;
                case PCOp_CRFIELD:
                    destinationState = register_value_states + destination->value.reg;
                    matchingState = register_value_states + matchingOperand.value.reg;
                    break;
            }
            if (destinationState->index != matchingState->index) {
                PCodeInstruction *move;
                if (matchingOperand.kind == PCOp_GPR)
                    move = PCodeUtilities_CreateInstruction(PC_MR, destination->value.reg, matchingOperand.value.reg);
                else if (matchingOperand.kind == PCOp_FPR)
                    move = PCodeUtilities_CreateInstruction(PC_FMR, destination->value.reg, matchingOperand.value.reg);
                else if (matchingOperand.kind == PCOp_VR)
                    move = PCodeUtilities_CreateInstruction(PC_VMR, destination->value.reg, matchingOperand.value.reg);
                else
                    move = PCodeUtilities_CreateInstruction(PC_MCRF, destination->value.reg, matchingOperand.value.reg);
                PCode_InsertInstructionBefore(instruction, move);
                copy_register_value_state(&matchingOperand, destination);
            }
            PCode_UnlinkInstruction(instruction);
            gValueNumberingChanged = 1;
        } else {
            create_register_value_record(instruction);
        }
    }
}

static inline ObjectIndexEntry *fn_0051f320_inline1(Object *object)
{
    ObjectIndexEntry *entry = objectIndex;
    while (entry != NULL) {
        if (object < (Object *)entry->object)
            entry = entry->left;
        else if (object > (Object *)entry->object)
            entry = entry->right;
        else
            return entry;
    }
    return NULL;
}

void create_register_value_record(PCodeInstruction *instruction)
{
    RegisterValueRecord *value;
    PCodeOperand *input;
    int input_index;
    int flags;
    PCodeOperand *output;
    ValueRegisterOperand **link;
    ObjectIndexEntry *related;
    unsigned int old_index;
    RegisterValueSnapshot *saved;
    ValueRegisterOperand *operand;
    int new_index;
    RegisterValueState *state;
    value = (RegisterValueRecord *)CompilerTools_AllocatePoolMemory((sizeof(*value) - sizeof(value->input_indices)) +
                                                                    (instruction->operand_count - 1) *
                                                                        sizeof(value->input_indices[0]));
    value->operands = NULL;
    value->instruction = instruction;
    input_index = 1;
    input = &instruction->operandData.operands[1];
    while (input_index < instruction->operand_count) {
        switch (input->kind) {
            case PCOp_GPR:
                value->input_indices[input_index - 1] = register_value_state_array[input->value.reg].index;
                break;
            case PCOp_FPR:
                value->input_indices[input_index - 1] = fpr_value_states[input->value.reg].index;
                break;
            case PCOp_VR:
                value->input_indices[input_index - 1] = gRegisterValueStates[input->value.reg].index;
                break;
            case PCOp_CRFIELD:
                value->input_indices[input_index - 1] = register_value_states[input->value.reg].index;
        }
        ++input_index;
        ++input;
    }
    if ((flags = instruction->flags) & PCodeInstruction_ImplicitUse) {
        related = fn_0051f320_inline1((flags & PCodeInstruction_NullObjectMemory)
                                          ? data_00582c44
                                          : (Object *)instruction->operandData.operands[2].object);
        value->related_index = related->index.index;
    }
    output = &instruction->operandData.operands[0];
    switch (instruction->operandData.operands[0].kind) {
        case PCOp_GPR:
            state = &register_value_state_array[output->value.reg];
            break;
        case PCOp_FPR:
            state = &fpr_value_states[output->value.reg];
            break;
        case PCOp_VR:
            state = &gRegisterValueStates[output->value.reg];
            break;
        case PCOp_CRFIELD:
            state = &register_value_states[output->value.reg];
    }
    old_index = state->index;
    if ((int)old_index < value_index_threshold && next_value_index >= value_index_threshold) {
        saved = (RegisterValueSnapshot *)CompilerTools_AllocatePoolMemory(sizeof(*saved));
        saved->next = data_00582c38;
        data_00582c38 = saved;
        saved->operand = *output;
        saved->state = *state;
    }
    if (state->value != NULL) {
        link = &state->value->operands;
        while (*link != NULL) {
            if ((*link)->operand.value.reg == output->value.reg) {
                *link = (*link)->next;
            } else {
                link = &(*link)->next;
            }
        }
    }
    state->value = NULL;
    new_index = state->index = next_value_index;
    ++next_value_index;
    value->index = new_index;
    operand = (ValueRegisterOperand *)CompilerTools_AllocatePoolMemory(sizeof(*operand));
    operand->operand = *output;
    operand->next = value->operands;
    value->operands = operand;
    switch (output->kind) {
        case PCOp_GPR:
            register_value_state_array[output->value.reg].value = value;
            break;
        case PCOp_FPR:
            fpr_value_states[output->value.reg].value = value;
            break;
        case PCOp_VR:
            gRegisterValueStates[output->value.reg].value = value;
            break;
        case PCOp_CRFIELD:
            register_value_states[output->value.reg].value = value;
    }
    value->next = register_values_by_opcode[instruction->opcode],
    register_values_by_opcode[instruction->opcode] = value;
}

int find_matching_value_signature(PCodeInstruction *func, PCodeOperand *out)
{
    PCodeOperand tmp;
    SInt16 op;
    Boolean commutative;
    RegisterValueRecord *node;

    for (node = register_values_by_opcode[func->opcode]; node != NULL; node = node->next) {
        if (node->operands == NULL || node->instruction->flags != func->flags ||
            ((UInt16)node->instruction->operand_count) != (UInt16)func->operand_count)
            continue;

        if (ValueNumbering_0051f790(node, func) == 0) {
            switch (op = func->opcode) {
                case PC_ADD:
                case PC_MULLW:
                case PC_AND:
                case PC_OR:
                case PC_XOR:
                case PC_FADD:
                case PC_FADDS:
                case PC_FMUL:
                case PC_FMULS:
                case PC_VADDFP:
                case PC_VADDSBS:
                case PC_VADDSHS:
                case PC_VADDSWS:
                case PC_VADDUBM:
                case PC_VADDUBS:
                case PC_VADDUHM:
                case PC_VADDUHS:
                case PC_VADDUWM:
                case PC_VADDUWS:
                case PC_VAND:
                case PC_VAVGSB:
                case PC_VAVGSH:
                case PC_VAVGSW:
                case PC_VAVGUB:
                case PC_VAVGUH:
                case PC_VAVGUW:
                case PC_VMAXSB:
                case PC_VMAXSH:
                case PC_VMAXSW:
                case PC_VMAXUB:
                case PC_VMAXUH:
                case PC_VMAXUW:
                case PC_VMINSB:
                case PC_VMINSH:
                case PC_VMINSW:
                case PC_VMINUB:
                case PC_VMINUH:
                case PC_VMINUW:
                case PC_VMULESB:
                case PC_VMULESH:
                case PC_VMULEUB:
                case PC_VMULEUH:
                case PC_VMULOSB:
                case PC_VMULOSH:
                case PC_VMULOUB:
                case PC_VMULOUH:
                case PC_VOR:
                case PC_VXOR:
                    commutative = 1;
                    break;
                default:
                    commutative = 0;
                    break;
            }
            if (!commutative || func->operand_count != 3)
                continue;

            tmp = func->operandData.operands[1];
            func->operandData.operands[1] = func->operandData.operands[2];
            func->operandData.operands[2] = tmp;
            if (ValueNumbering_0051f790(node, func) == 0) {
                tmp = func->operandData.operands[1];
                func->operandData.operands[1] = func->operandData.operands[2];
                func->operandData.operands[2] = tmp;
                continue;
            }
        }

        copy_value_signature(node, out);
        return 1;
    }
    return 0;
}

static ObjectIndexEntry *VN_FindNode(Object *key)
{
    ObjectIndexEntry *node;

    node = objectIndex;
    while (node != NULL) {
        if (key < node->object)
            node = node->left;
        else if (key > node->object)
            node = node->right;
        else
            return node;
    }
    return NULL;
}

SInt32 ValueNumbering_0051f790(RegisterValueRecord *record, PCodeInstruction *instruction)
{
    SInt32 operandIndex;
    SInt32 operandCount;
    PCodeOperand *storedOperand;
    PCodeOperand *operand;

    operandCount = instruction->operand_count;
    storedOperand = &record->instruction->operandData.operands[1];
    operand = &instruction->operandData.operands[1];
    for (operandIndex = 1; operandIndex < operandCount; operandIndex++, storedOperand++, operand++) {
        switch (storedOperand->kind) {
            case PCOp_GPR:
                if (record->input_indices[operandIndex - 1] != register_value_state_array[operand->value.reg].index)
                    return 0;
                break;
            case PCOp_FPR:
                if (record->input_indices[operandIndex - 1] != fpr_value_states[operand->value.reg].index)
                    return 0;
                break;
            case PCOp_VR:
                if (record->input_indices[operandIndex - 1] != gRegisterValueStates[operand->value.reg].index)
                    return 0;
                break;
            case PCOp_CRFIELD:
                if (record->input_indices[operandIndex - 1] != register_value_states[operand->value.reg].index)
                    return 0;
                break;
            case PCOp_MEMORY:
                if (operand->kind != PCOp_MEMORY)
                    return 0;
                if (operand->value.signed_value != storedOperand->value.signed_value)
                    return 0;
                if (operand->object != storedOperand->object)
                    return 0;
                if (operand->flags != storedOperand->flags)
                    return 0;
                break;
            case PCOp_IMMEDIATE:
                if (operand->kind != PCOp_IMMEDIATE)
                    return 0;
                if (operand->value.signed_value != storedOperand->value.signed_value)
                    return 0;
                break;
            case PCOp_SPR:
            case PCOp_LABEL:
                CError_FATAL(776);
                break;
        }
    }

    if (instruction->flags & fIsRead) {
        Object *key;
        ObjectIndexEntry *node;

        if (instruction->flags & fIsPtrOp)
            key = data_00582c44;
        else
            key = instruction->operandData.operands[2].object;

        node = VN_FindNode(key);
        if (record->related_index != node->index.index)
            return 0;
    }
    return 1;
}

void copy_register_value_state(PCodeOperand *source, PCodeOperand *destination)
{
    RegisterValueState *sourceRecord;
    RegisterValueState *destinationRecord;
    RegisterValueRecord *oldOwner;
    RegisterValueRecord *newOwner;

    switch (source->kind) {
        case PCOp_GPR:
            sourceRecord = register_value_state_array + source->value.reg;
            destinationRecord = register_value_state_array + destination->value.reg;
            break;
        case PCOp_FPR:
            sourceRecord = fpr_value_states + source->value.reg;
            destinationRecord = fpr_value_states + destination->value.reg;
            break;
        case PCOp_VR:
            sourceRecord = gRegisterValueStates + source->value.reg;
            destinationRecord = gRegisterValueStates + destination->value.reg;
            break;
        case PCOp_CRFIELD:
            sourceRecord = register_value_states + source->value.reg;
            destinationRecord = register_value_states + destination->value.reg;
            break;
    }

    if (destinationRecord->index < value_index_threshold && next_value_index >= value_index_threshold) {
        ValueUpdate *saved;
        saved = (ValueUpdate *)CompilerTools_AllocatePoolMemory(sizeof(ValueUpdate));
        saved->next = data_00582c38;
        data_00582c38 = saved;
        saved->descriptor = *destination;
        saved->value = *destinationRecord;
    }

    if ((oldOwner = destinationRecord->value) != NULL) {
        ValueRegisterOperand **link = &oldOwner->operands;
        ValueRegisterOperand *entry;
        while ((entry = *link) != NULL) {
            if (entry->operand.value.reg == destination->value.reg)
                *link = entry->next;
            else
                link = &entry->next;
        }
    }

    destinationRecord->value = sourceRecord->value;
    if (destinationRecord->value != NULL) {
        ValueRegisterOperand *entry;
        newOwner = ((volatile RegisterValueState *)destinationRecord)->value;
        entry = (ValueRegisterOperand *)CompilerTools_AllocatePoolMemory(sizeof(ValueRegisterOperand));
        entry->operand = *destination;
        entry->next = newOwner->operands;
        newOwner->operands = entry;
    }
    destinationRecord->index = sourceRecord->index;
}

void invalidate_object_indices(Type *unused, int mode)
{
    struct ObjectIndexEntry *entry;
    int qualifiers;
    RegisterValueState *index;
    struct SavedObjectIndex *saved;
    entry = object_indices;
    for (; entry != NULL; entry = entry->next) {
        if (entry->object->datatype != 9) {
            if (mode == 0 ||
                entry->object->datatype != DDATA && PCodeUtilities_Require(entry->object) == 0 &&
                    (entry->object->datatype != DLOCAL ||
                     entry->object->u.var.info->noregister == 0 && entry->object->type->type != TYPEARRAY &&
                         !(entry->object->type->type == TYPESTRUCT || entry->object->type->type == TYPECLASS) &&
                         (entry->object->type->type != TYPEMEMBERPOINTER || entry->object->type->size != 12))) {
                continue;
            }
            qualifiers = entry->object->type->type == TYPEPOINTER ? TYPE_POINTER(entry->object->type)->qual
                                                                  : entry->object->qual;
            qualifiers = qualifiers & Q_CONST;
            if (qualifiers != 0) {
                continue;
            }
        }
        index = &entry->index;
        if (entry->index.index < value_index_threshold && next_value_index >= value_index_threshold) {
            saved = CompilerTools_AllocatePoolMemory(28);
            saved->next = data_00582c38;
            data_00582c38 = saved;
            saved->kind = 5;
            saved->entry = entry;
            saved->index = *index;
        }
        index->index = next_value_index;
        next_value_index += 1;
        index->value = NULL;
    }
    return;
}

SInt32 invalidate_register_value(PCodeOperand *operand)
{
    RegisterValueState *reg;
    ValueUpdate *snapshot;
    ValueRegisterOperand **link;
    ValueRegisterOperand *entry;
    SInt32 index;
    switch (operand->kind) {
        case PCOp_GPR:
            reg = &register_value_state_array[operand->value.reg];
            break;
        case PCOp_FPR:
            reg = &fpr_value_states[operand->value.reg];
            break;
        case PCOp_VR:
            reg = &gRegisterValueStates[operand->value.reg];
            break;
        case PCOp_CRFIELD:
            reg = &register_value_states[operand->value.reg];
            break;
    }
    if (reg->index < value_index_threshold && next_value_index >= value_index_threshold) {
        snapshot = (ValueUpdate *)CompilerTools_AllocatePoolMemory(sizeof(ValueUpdate));
        snapshot->next = data_00582c38;
        data_00582c38 = snapshot;
        snapshot->descriptor = *operand;
        snapshot->value = *reg;
    }
    if (reg->value != NULL) {
        link = &reg->value->operands;
        while ((entry = *link) != NULL) {
            if (entry->operand.value.reg == operand->value.reg)
                *link = entry->next;
            else
                link = &entry->next;
        }
    }
    reg->value = NULL;
    index = next_value_index++;
    reg->index = index;
    return reg->index;
}

Boolean compare_register_value_indices(IndexedValueReference *first, IndexedValueReference *second)
{
    RegisterValueState *firstValue;
    RegisterValueState *secondValue;
    switch (first->kind) {
        case 0:
            firstValue = &register_value_state_array[first->index];
            secondValue = &register_value_state_array[second->index];
            break;
        case 1:
            firstValue = &fpr_value_states[first->index];
            secondValue = &fpr_value_states[second->index];
            break;
        case 9:
            firstValue = &gRegisterValueStates[first->index];
            secondValue = &gRegisterValueStates[second->index];
            break;
        case 3:
            firstValue = &register_value_states[first->index];
            secondValue = &register_value_states[second->index];
            break;
    }
    return firstValue->index == secondValue->index;
}

void fn_0051fd70(ValueUpdate *update)
{
    RegisterValueState *slot;
    PCodeOperand *descriptor;
    ValueRegisterOperand **link;
    ValueDestination *destination;
    ValueRegisterOperand *newLink;
    RegisterValueRecord *chain;
    descriptor = &update->descriptor;
    if (update->descriptor.kind == PCOp_MEMORY) {
        destination = update->destination;
        destination->value = update->value;
        return;
    }
    switch (descriptor->kind) {
        case PCOp_GPR:
            slot = register_value_state_array + descriptor->value.reg;
            break;
        case PCOp_FPR:
            slot = fpr_value_states + descriptor->value.reg;
            break;
        case PCOp_VR:
            slot = gRegisterValueStates + descriptor->value.reg;
            break;
        case PCOp_CRFIELD:
            slot = register_value_states + descriptor->value.reg;
    }
    if (slot->value != NULL) {
        link = &slot->value->operands;
        while (*link != NULL) {
            if ((*link)->operand.value.reg == descriptor->value.reg) {
                *link = (*link)->next;
            } else {
                link = &(*link)->next;
            }
        }
    }
    slot->index = update->value.index;
    if ((slot->value = update->value.value) != NULL) {
        chain = slot->value;
        newLink = (ValueRegisterOperand *)CompilerTools_AllocatePoolMemory(sizeof(*newLink));
        newLink->operand = *descriptor;
        newLink->next = chain->operands;
        chain->operands = newLink;
    }
}

static void ZeroArray(struct RegisterValueRecord **p, SInt32 n)
{
    SInt32 i;
    for (i = 0; i < n; i++)
        p[i] = NULL;
}

void initialize_value_states(void)
{
    RegisterValueState *p;
    struct ObjectIndexEntry *q;
    SInt32 i;
    SInt32 j;
    SInt32 k;
    SInt32 m;

    next_value_index = 0;
    ZeroArray(register_values_by_opcode, 466);

    p = register_value_state_array;
    i = 0;
    while (i < gUsedVirtualRegistersGPR) {
        i++;
        p->index = next_value_index++;
        p->value = NULL;
        p++;
    }
    p = fpr_value_states;
    j = 0;
    while (j < gUsedVirtualRegistersFPR) {
        j++;
        p->index = next_value_index++;
        p->value = NULL;
        p++;
    }
    p = gRegisterValueStates;
    k = 0;
    while (k < gUsedVirtualRegistersVR) {
        k++;
        p->index = next_value_index++;
        p->value = NULL;
        p++;
    }
    p = register_value_states;
    m = 0;
    do {
        p->index = next_value_index++;
        p->value = NULL;
        p++;
    } while (++m < 8);
    q = object_indices;
    while (q != NULL) {
        q->index.index = next_value_index++;
        q->index.value = NULL;
        q = q->next;
    }
    data_00582c38 = NULL;
    value_index_threshold = 0x7fffffff;
}

static inline void InsertObjectIndex(ObjectIndexEntry **root, Object *key)
{
    ObjectIndexEntry *n;
    ObjectIndexEntry **link;
    link = root;
    while ((n = *link) != NULL) {
        if (key < n->object)
            link = &n->left;
        else if (key <= n->object)
            return;
        else
            link = &n->right;
    }
    n = (ObjectIndexEntry *)CompilerTools_AllocatePoolMemory(0x18);
    n->right = NULL;
    n->left = n->right;
    n->object = key;
    n->next = object_indices;
    object_indices = n;
    *link = n;
}

void fn_0051ffc0(void)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;

    objectIndex = object_indices = NULL;
    data_00582c44 = (Object *)galloc(sizeof(Object));
    memclrw(data_00582c44, sizeof(Object));
    data_00582c44->datatype = 9;
    InsertObjectIndex(&objectIndex, data_00582c44);
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            if ((instruction->flags & PCodeInstruction_GPRResultMask) != 0 &&
                (instruction->flags & PCodeInstruction_NullObjectMemory) == 0)
                InsertObjectIndex(&objectIndex, instruction->operandData.operands[2].object);
        }
    }
}

static inline SInt32 CanPropagateCopy(SInt32 copyIndex, CodeMotionListNode *useNode)
{
    for (; useNode != NULL; useNode = useNode->next) {
        if (can_propagate_copy_to_use(copyIndex, useNode->index) == 0)
            return 0;
    }
    return 1;
}

void COpt_CopyPropagation(SInt32 mode)
{
    int blockIndex;
    struct CopyPropagationBitSets *block;
    SInt32 copyIndex;
    SInt32 propagate;

    gCopyPropagationChanged = 0;
    copy_propagation_mode = mode;
    CopyPropagation_CountBlockCopies();
    if (copyCount > 0) {
        COpt_SetLoopCodeMotionMode(0);
        CopyPropagation_BuildCodeMotionRecords();
        copyPropagationBitSets = CompilerTools_AllocatePoolMemory(gPCodeBlockCount * sizeof *block);
        block = copyPropagationBitSets;
        for (blockIndex = 0; blockIndex < gPCodeBlockCount; blockIndex++) {
            block->gen = CompilerTools_AllocatePoolMemory(((copyCount + 31) >> 5) * sizeof *block->gen);
            block->kill = CompilerTools_AllocatePoolMemory(((copyCount + 31) >> 5) * sizeof *block->kill);
            block->out = CompilerTools_AllocatePoolMemory(((copyCount + 31) >> 5) * sizeof *block->out);
            block->in = CompilerTools_AllocatePoolMemory(((copyCount + 31) >> 5) * sizeof *block->in);
            block++;
        }
        CopyPropagation_ComputeGenKill();
        SpillCode_BuildBlockOrder();
        CopyPropagation_ComputeInOutSets();
        for (copyIndex = 0; copyIndex < copyCount; copyIndex++) {
            if (code_motion_records[copyIndex].node->flags & PCodeInstruction_CoalesceDisabled) {
                propagate = 0;
            } else {
                propagate = CanPropagateCopy(copyIndex, code_motion_records[copyIndex].list);
            }
            if (propagate)
                CopyPropagation_ReplaceRegisterUses(copyIndex);
        }
    }
    CompilerTools_ResetPool();
}
