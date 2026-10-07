#define CERROR_FILE "FunctionCalls.c"
#include "compiler/common.h"
#include "compiler/FunctionCalls.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CodeGen.h"
#include "compiler/Intrinsics.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/StructMoves.h"

static SInt32 lbl_00574168 = 8;

static inline void emit12(Operand *o, unsigned int a, unsigned int b, unsigned int t)
{
    if ((int)o->reg != 0xc)
        PCodeUtilities_EmitInstruction(PC_MR, 0xc, (int)o->reg);
    fn_004a1cb0(a | 0x1000, b, t);
}

static inline void emitobj12(Object *obj, Operand *o, unsigned int a, unsigned int b, unsigned int t)
{
    if ((int)o->reg != 0xc)
        PCodeUtilities_EmitInstruction(PC_MR, 0xc, (int)o->reg);
    PCodeUtilities_EmitObjectInstructionWithPayload(obj, 1, a | 0x1000, b, t);
}

static ArgumentContext *NewParm(ENode *e)
{
    ArgumentContext *p = (ArgumentContext *)lalloc(0x2e);
    p->next = NULL;
    p->node = e;
    p->stack_offset = -1;
    p->gpr = -1;
    p->second_gpr = -1;
    p->fpr = -1;
    p->vectorRegister = -1;
    p->evaluated = 0;
    p->flags = 0;
    return p;
}

ArgumentContext *assign_argument_locations(ENode *thisArg, ENodeList *args, FuncArg *type, UInt32 *gprMask,
                                           UInt32 *fprMask, Boolean *hasFloatArgs, UInt32 *vectorMask,
                                           Boolean *hasVectorArgs, Boolean hasSpecialArgument)
{
    ArgumentContext *head = NULL;
    SInt32 nextGPR = 3;
    SInt32 nextFPR = 1;
    SInt32 nextVectorReg = 2;
    Boolean onStack = 0;
    SInt32 stackOffset = 0;
    ArgumentContext *parm;
    TypeStruct *argType;
    ENode *arg;
    SInt32 wordCount;

    *fprMask = 0;
    *gprMask = *fprMask;
    *hasFloatArgs = 0;
    *vectorMask = 0;
    *hasVectorArgs = 0;

    while (args != NULL) {
        arg = args->node;
        if (arg == thisArg) {
            args = args->next;
        } else {
            argType = (TypeStruct *)arg->rtype;
            if (head != NULL) {
                parm->next = NewParm(arg);
                parm = parm->next;
            } else {
                head = parm = NewParm(arg);
            }
            if (onStack) {
                (void)(argType->type == TYPEFLOAT);
                if (argType->type == TYPEFLOAT) {
                    if (argType->size == 4)
                        stackOffset = (stackOffset + 3) & ~3;
                    else
                        stackOffset = (stackOffset + 7) & ~7;
                } else if ((argType->type == TYPEINT || argType->type == TYPEENUM) && argType->size == 8) {
                    stackOffset = (stackOffset + 7) & ~7;
                } else {
                    stackOffset = (stackOffset + 3) & ~3;
                }
            }
            parm->stack_offset = stackOffset;
            onStack = 0;

            if (((argType->type == TYPEINT || argType->type == TYPEENUM) && argType->size == 8) ||
                (copts.operandsDebug && argType->type == TYPEFLOAT && argType->size != 4 &&
                 copts.incompatible_sfpe_double_params == 0)) {
                if (nextGPR % 2 == 0)
                    nextGPR++;
            }

            if (argType->type == TYPESTRUCT && (int)(argType->stype) >= 4 && (int)(argType->stype) <= 0xe) {
                *hasVectorArgs = 1;
                if (type == (FuncArg *)&elipsis) {
                    parm->flags |= 4;
                    onStack = 1;
                    stackOffset = (stackOffset + 0x17) & ~0x0f;
                    data_005884ff = 1;
                    parm->stack_offset = stackOffset = stackOffset - 8;
                    stackOffset += 16;
                    nextVectorReg++;
                } else {
                    if (!(nextVectorReg > 13)) {
                        onStack = 0;
                        parm->flags |= 0x20;
                        parm->vectorRegister = nextVectorReg;
                        *vectorMask |= 1u << nextVectorReg;
                    } else {
                        parm->flags |= 4;
                        onStack = 1;
                    }
                    if (onStack) {
                        stackOffset = (stackOffset + 0x17) & ~0x0f;
                        data_005884ff = 1;
                        parm->stack_offset = stackOffset = stackOffset - 8;
                        stackOffset += 16;
                    }
                    nextVectorReg++;
                }
            } else if (copts.operandsDebug && argType->type == TYPEFLOAT) {
                if (argType->size == 4) {
                    if (nextGPR <= 10) {
                        parm->flags |= 1;
                        parm->gpr = nextGPR;
                        *gprMask |= 1u << nextGPR;
                    } else {
                        parm->flags |= 4;
                        onStack = 1;
                    }
                    nextGPR++;
                    if (onStack)
                        stackOffset += 4;
                } else {
                    if (nextGPR < 10) {
                        parm->flags |= 1;
                        if (copts.littleendian != 0) {
                            parm->gpr = nextGPR;
                            parm->second_gpr = nextGPR + 1;
                        } else {
                            parm->gpr = nextGPR + 1;
                            parm->second_gpr = nextGPR;
                        }
                        *gprMask |= 1u << nextGPR;
                        *gprMask |= 1u << (nextGPR + 1);
                    } else {
                        parm->flags |= 4;
                        onStack = 1;
                    }
                    nextGPR += 2;
                    if (onStack)
                        stackOffset += 8;
                }
            } else if (argType->type == TYPEFLOAT) {
                *hasFloatArgs = 1;
                if (type == NULL || type == (FuncArg *)&oldstyle || type == (FuncArg *)&elipsis) {
                    if (nextFPR <= 8) {
                        parm->flags |= 2;
                        parm->fpr = nextFPR;
                        *fprMask |= 1u << nextFPR;
                    } else {
                        parm->flags |= 0x14;
                        onStack = 1;
                    }
                    if (onStack)
                        stackOffset += 8;
                    nextFPR++;
                } else if (type == (FuncArg *)&elipsis) {
                    if (nextGPR < 10) {
                        parm->flags |= 1;
                        parm->gpr = nextGPR;
                        *gprMask |= 3u << nextGPR;
                    } else if (nextGPR == 10) {
                        parm->flags |= 0x15;
                        parm->gpr = nextGPR;
                        *gprMask |= 3u << nextGPR;
                    } else {
                        parm->flags |= 0x14;
                        onStack = 1;
                    }
                    if (onStack)
                        stackOffset += 8;
                    nextFPR++;
                } else {
                    if (nextFPR <= 8) {
                        parm->flags |= 2;
                        parm->fpr = nextFPR;
                        *fprMask |= 1u << nextFPR;
                    } else {
                        parm->flags |= 4;
                        onStack = 1;
                    }
                    nextFPR++;
                    if (onStack) {
                        if (argType->size == 4)
                            stackOffset += 4;
                        else
                            stackOffset += 8;
                    }
                }
            } else if ((argType->type == TYPEINT || argType->type == TYPEENUM) && argType->size == 8) {
                if (nextGPR <= 10) {
                    parm->flags |= 1;
                    if (copts.littleendian != 0) {
                        parm->gpr = nextGPR;
                        parm->second_gpr = nextGPR + 1;
                    } else {
                        parm->gpr = nextGPR + 1;
                        parm->second_gpr = nextGPR;
                    }
                    *gprMask |= 1u << nextGPR;
                    if (nextGPR + 1 <= 10)
                        *gprMask |= 1u << (nextGPR + 1);
                } else {
                    parm->flags |= 4;
                    onStack = 1;
                }
                nextGPR += 2;
                if (onStack)
                    stackOffset += 8;
            } else if (argType->type == TYPEINT || argType->type == TYPEENUM || argType->type == TYPEPOINTER ||
                       (argType->type == TYPEMEMBERPOINTER && argType->size == 4)) {
                if (type == NULL || type == (FuncArg *)&elipsis || type == (FuncArg *)&oldstyle) {
                    if (argType->size < 4)
                        parm->flags |= 8;
                }
                if (nextGPR <= 10) {
                    parm->flags |= 1;
                    parm->gpr = nextGPR;
                    *gprMask |= 1u << nextGPR;
                } else {
                    parm->flags |= 4;
                    onStack = 1;
                }
                nextGPR++;
                if (onStack)
                    stackOffset += 4;
            } else {
                CError_FATAL(413);
                wordCount = (argType->size >> 2) + ((argType->size & 3) != 0);
                if (nextGPR <= 10) {
                    if (nextGPR + wordCount - 1 <= 10) {
                        parm->flags |= 1;
                        parm->gpr = nextGPR;
                        *gprMask |= ((1u << wordCount) - 1) << nextGPR;
                    } else {
                        parm->flags |= 5;
                        parm->gpr = nextGPR;
                        *gprMask |= ((1u << (11 - nextGPR)) - 1) << nextGPR;
                    }
                } else {
                    parm->flags |= 4;
                    onStack = 1;
                }
                nextGPR += wordCount;
                if (onStack)
                    stackOffset = (stackOffset + argType->size + 3) & ~3;
            }

            args = args->next;
            if (type == NULL || type == (FuncArg *)&elipsis || type == (FuncArg *)&oldstyle)
                continue;
        }
        type = type->next;
    }

    if (stackOffset > outgoing_argument_size)
        outgoing_argument_size = stackOffset;
    return head;
}

void store_argument_on_stack(ArgumentContext *arg)
{
    Operand stackOperand;
    Type *type;

    type = arg->node->rtype;
    memclrw(&stackOperand, sizeof(stackOperand));

    if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
        (type->type == TYPEMEMBERPOINTER && type->size == 4) || (copts.operandsDebug && type->type == TYPEFLOAT)) {
        if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
            (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4)) {
            if (arg->evaluated == 0) {
                (*data_00560648[arg->node->type])(arg->node, 0, 0, &arg->operand);
            }
            Operands_ForceGPRPair(&arg->operand, type, 0, 0);
            emit_opcode_with_base_offset(PC_STW, arg->operand.reg, 1, NULL, (arg->stack_offset + 8) + low_word_offset);
            emit_opcode_with_base_offset(PC_STW, arg->operand.regHi, 1, NULL,
                                         (arg->stack_offset + 8) + high_word_offset);
        } else {
            if (arg->evaluated == 0) {
                (*data_00560648[arg->node->type])(arg->node, 0, 0, &arg->operand);
            }
            if (arg->flags & 8) {
                Operands_ExtendGPR(&arg->operand, type, 0);
            }
            if (arg->operand.kind != OpndType_GPR) {
                Operands_ForceGPR(&arg->operand, type, 0);
            }
            emit_opcode_with_base_offset(PC_STW, arg->operand.reg, 1, NULL, arg->stack_offset + 8);
        }
    } else if (type->type == TYPEFLOAT) {
        if (arg->evaluated == 0) {
            (*data_00560648[arg->node->type])(arg->node, 0, 0, &arg->operand);
        }
        if (arg->operand.kind != OpndType_FPR) {
            Operands_ForceFPR(&arg->operand, type, 0);
        }
        if (type->size == 4 && !(arg->flags & 0x10)) {
            emit_opcode_with_base_offset(PC_STFS, arg->operand.reg, 1, NULL, arg->stack_offset + 8);
        } else {
            emit_opcode_with_base_offset(PC_STFD, arg->operand.reg, 1, NULL, arg->stack_offset + 8);
        }
    } else if (type->type == TYPESTRUCT && (SInt32)TYPE_STRUCT(type)->stype >= 4 &&
               (SInt32)TYPE_STRUCT(type)->stype <= 14) {
        if (arg->evaluated == 0) {
            (*data_00560648[arg->node->type])(arg->node, 0, 0, &arg->operand);
        }
        if (arg->operand.kind != OpndType_VR) {
            Operands_ForceVR(&arg->operand, type, 0);
        }
        emit_opcode_with_base_offset(PC_STVX, arg->operand.reg, 1, NULL, arg->stack_offset + 8);
    } else {
        stackOperand.kind = OpndType_IndirectGPR_ImmOffset;
        stackOperand.reg = 1;
        stackOperand.object = NULL;
        stackOperand.displacement = arg->stack_offset + 8;
        if (arg->evaluated == 0) {
            (*data_00560648[arg->node->type])(arg->node, 0, 0, &arg->operand);
        }
        StructMoves_EmitCopy(&stackOperand, &arg->operand, type->size, StackFrameEABI_GetTypeAlignment(type));
    }
}

void load_argument_registers(ArgumentContext *argument)
{
    Type *type;
    long word_count;
    int word_index;
    int next_register;
    int struct_kind;

    type = argument->node->rtype;
    if ((argument->flags & 0x27) == 2) {
        if (argument->evaluated == 0) {
            data_00560648[argument->node->type](argument->node, argument->fpr, 0, &argument->operand);
        }
        if (argument->operand.kind != OpndType_FPR) {
            Operands_ForceFPR(&argument->operand, type, argument->fpr);
        }
        if (argument->operand.reg != argument->fpr) {
            PCodeUtilities_EmitInstruction(PC_FMR, argument->fpr, argument->operand.reg);
        }
    } else if ((argument->flags & 0x27) == 0x20) {
        if (argument->evaluated == 0) {
            data_00560648[argument->node->type](argument->node, argument->vectorRegister, 0, &argument->operand);
        }
        if (argument->operand.kind != OpndType_VR) {
            Operands_ForceVR(&argument->operand, type, argument->vectorRegister);
        }
        if (argument->operand.reg != argument->vectorRegister) {
            PCodeUtilities_EmitInstruction(PC_VMR, argument->vectorRegister, argument->operand.reg);
        }
    } else {
        if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
            (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
            (copts.operandsDebug != 0 && type->type == TYPEFLOAT)) {
            if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                (copts.operandsDebug != 0 && type->type == TYPEFLOAT && type->size != 4)) {
                if (argument->evaluated == 0) {
                    data_00560648[argument->node->type](argument->node, argument->gpr, argument->second_gpr,
                                                        &argument->operand);
                }
                Operands_ForceGPRPair(&argument->operand, type, argument->gpr, argument->second_gpr);
                if (copts.littleendian != 0) {
                    if (argument->second_gpr > 10) {
                        emit_opcode_with_base_offset(PC_STW, argument->operand.regHi, 1, NULL,
                                                     high_word_offset + argument->stack_offset + 8);
                    }
                } else {
                    if (argument->gpr > 10) {
                        emit_opcode_with_base_offset(PC_STW, argument->operand.reg, 1, NULL,
                                                     low_word_offset + argument->stack_offset + 8);
                    }
                }
            } else {
                if (argument->evaluated == 0) {
                    data_00560648[argument->node->type](argument->node, argument->gpr, 0, &argument->operand);
                }
                if ((argument->flags & 8) != 0) {
                    Operands_ExtendGPR(&argument->operand, type, argument->gpr);
                }
                if (argument->operand.kind != OpndType_GPR) {
                    Operands_ForceGPR(&argument->operand, type, argument->gpr);
                }
                if (argument->operand.reg != argument->gpr) {
                    PCodeUtilities_EmitInstruction(PC_MR, argument->gpr, argument->operand.reg);
                }
            }
        } else if (type->type == TYPEFLOAT) {
            if (argument->evaluated == 0) {
                data_00560648[argument->node->type](argument->node, 0, 0, &argument->operand);
            }
            if (type->size != 4 && argument->operand.kind == OpndType_IndirectGPR_ImmOffset) {
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr, argument->operand.reg, argument->operand.object,
                                             argument->operand.displacement);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 1, argument->operand.reg, argument->operand.object,
                                             argument->operand.displacement + 4);
            } else {
                if (argument->operand.kind != OpndType_FPR) {
                    Operands_ForceFPR(&argument->operand, type, 0);
                }
                emit_opcode_with_base_offset(PC_STFD, argument->operand.reg, 1, NULL, argument->stack_offset + 8);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr, 1, NULL, argument->stack_offset + 8);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 1, 1, NULL, argument->stack_offset + 0xc);
            }
        } else if (type->type == TYPESTRUCT && (struct_kind = TYPE_STRUCT(type)->stype) >= 4 && struct_kind <= 14) {
            if (argument->evaluated == 0) {
                data_00560648[argument->node->type](argument->node, 0, 0, &argument->operand);
            }
            if (argument->operand.kind == OpndType_IndirectGPR_ImmOffset) {
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr, argument->operand.reg, argument->operand.object,
                                             argument->operand.displacement);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 1, argument->operand.reg, argument->operand.object,
                                             argument->operand.displacement + 4);
                if ((next_register = argument->gpr + 2) < 10) {
                    emit_opcode_with_base_offset(PC_LWZ, next_register, argument->operand.reg, argument->operand.object,
                                                 argument->operand.displacement + 8);
                    emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 3, argument->operand.reg,
                                                 argument->operand.object, argument->operand.displacement + 12);
                }
            } else {
                if (argument->operand.kind != OpndType_VR) {
                    Operands_ForceVR(&argument->operand, type, 0);
                }
                emit_opcode_with_base_offset(PC_STVX, argument->operand.reg, 1, NULL, argument->stack_offset + 8);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr, 1, NULL, argument->stack_offset + 8);
                emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 1, 1, NULL, argument->stack_offset + 0xc);
                if ((next_register = argument->gpr + 2) < 10) {
                    emit_opcode_with_base_offset(PC_LWZ, next_register, 1, NULL, argument->stack_offset + 0x10);
                    emit_opcode_with_base_offset(PC_LWZ, argument->gpr + 3, 1, NULL, argument->stack_offset + 0x14);
                }
            }
        } else {
            if (argument->evaluated == 0) {
                data_00560648[argument->node->type](argument->node, 0, 0, &argument->operand);
            }
            if (type->size <= 4) {
                if (argument->operand.kind == OpndType_IndirectSymbol) {
                    Operands_Normalize(&argument->operand);
                }
                if (argument->operand.kind == OpndType_IndirectGPR_ImmOffset) {
                    emit_opcode_with_base_offset(PC_LWZ, argument->gpr, argument->operand.reg, argument->operand.object,
                                                 argument->operand.displacement);
                } else if (argument->operand.kind == OpndType_IndirectGPR_Indexed) {
                    PCodeUtilities_EmitInstruction(PC_LWZX, argument->gpr, argument->operand.reg,
                                                   argument->operand.secondary_reg);
                }
            } else {
                int count = (type->size >> 2) + ((type->size & 3) != 0);
                int offset;
                word_count = count;
                StructMoves_PrepareOperandForOffset(&argument->operand, count * 4, 12);
                word_index = 0;
                if (word_count > 0) {
                    offset = 0;
                    do {
                        if (argument->operand.reg != argument->gpr + word_index) {
                            emit_opcode_with_base_offset(PC_LWZ, argument->gpr + word_index, argument->operand.reg,
                                                         argument->operand.object,
                                                         argument->operand.displacement + offset);
                        }
                        offset += 4;
                    } while (++word_index < word_count);
                }
                if (argument->operand.reg >= argument->gpr) {
                    if (argument->operand.reg < argument->gpr + word_count) {
                        emit_opcode_with_base_offset(
                            PC_LWZ, argument->operand.reg, argument->operand.reg, argument->operand.object,
                            (argument->operand.reg - argument->gpr) * 4 + argument->operand.displacement);
                    }
                }
            }
        }
    }
}

void evaluate_hascall_arguments_reverse(ArgumentContext *x)
{
    ArgumentContext *a, *b, *c, *d;

    a = x->next;
    while (a != NULL && a->node->hascall == 0)
        a = a->next;
    if (a != NULL) {
        b = a->next;
        while (b != NULL && b->node->hascall == 0)
            b = b->next;
        if (b != NULL) {
            c = b->next;
            while (c != NULL && c->node->hascall == 0)
                c = c->next;
            if (c != NULL) {
                d = c->next;
                while (d != NULL && d->node->hascall == 0)
                    d = d->next;
                if (d != NULL)
                    evaluate_hascall_arguments_reverse(d);
                if (c->node->hascall != 0) {
                    data_00560648[c->node->type](c->node, 0, 0, &c->operand);
                    c->evaluated = 1;
                }
            }
            if (b->node->hascall != 0) {
                data_00560648[b->node->type](b->node, 0, 0, &b->operand);
                b->evaluated = 1;
            }
        }
        if (a->node->hascall != 0) {
            data_00560648[a->node->type](a->node, 0, 0, &a->operand);
            a->evaluated = 1;
        }
    }
    if (x->node->hascall != 0) {
        data_00560648[x->node->type](x->node, 0, 0, &x->operand);
        x->evaluated = 1;
    }
}

void FunctionCalls_GenerateCall(ENode *item, Operand *result)
{
    ENode *expr;
    ENode *alternateExpr;
    ArgumentContext *arguments;
    Type *returnType;
    TypeClass *object;
    Boolean hasSpecialArgument;
    unsigned int argumentFlags;
    ArgumentContext *argument;
    FuncArg *formalArgument;
    int offset;
    int registerIndex;
    unsigned char firstRegister;
    int sourceRegister;
    SInt32 value;
    int structureType;
    Operand op;
    TypeClass *resolvedObject;
    UInt32 resolvedOffset;
    UInt32 callArgument20;
    UInt32 callArgument1c;
    UInt32 callArgument18;
    Boolean callArgument12;
    Boolean callArgument11;
    expr = item->data.funccall.funcref;
    returnType = item->data.funccall.functype->functype;
    alternateExpr = NULL;
    memclrw(&op, sizeof(op));
    hasSpecialArgument = 0;
    for (formalArgument = item->data.funccall.functype->args; formalArgument != NULL;
         formalArgument = formalArgument->next) {
        if (formalArgument == &elipsis) {
            hasSpecialArgument = 1;
            break;
        }
    }
    if ((item->data.funccall.functype->flags & 0x80) != 0) {
        if (Type_RequiresMemoryReturn(returnType))
            alternateExpr = item->data.funccall.args->next->node;
        else
            alternateExpr = item->data.funccall.args->node;
    }
    arguments = assign_argument_locations(alternateExpr, item->data.funccall.args, item->data.funccall.functype->args,
                                          &callArgument20, &callArgument1c, &callArgument11, &callArgument18,
                                          &callArgument12, hasSpecialArgument);
    if (arguments != NULL)
        evaluate_hascall_arguments_reverse(arguments);
    if (expr->hascall != 0) {
        data_00560648[expr->type](expr, 0, 0, &op);
        if (op.kind != OpndType_GPR)
            Operands_ForceGPR(&op, (Type *)&void_ptr, 0);
    } else if ((alternateExpr != NULL) && (alternateExpr->hascall != 0)) {
        data_00560648[alternateExpr->type](alternateExpr, 0, 0, &op);
        if (op.kind != OpndType_GPR)
            Operands_ForceGPR(&op, (Type *)&void_ptr, 0);
    }
    for (argument = arguments; argument != NULL; argument = argument->next) {
        if ((argument->flags & 4) != 0)
            store_argument_on_stack(argument);
    }
    for (argument = arguments; argument != NULL; argument = argument->next) {
        if ((argument->flags & 0x27) == 5) {
            registerIndex = argument->gpr;
            offset = 0;
            while (registerIndex <= 10) {
                emit_opcode_with_base_offset(PC_LWZ, registerIndex, 1, NULL, argument->stack_offset + 8 + offset);
                ++registerIndex;
                offset += 4;
            }
        }
    }
    for (argument = arguments; argument != NULL; argument = argument->next) {
        argumentFlags = argument->flags & 0x27;
        if (argumentFlags == 1 || argumentFlags == 2 || argumentFlags == 0x20)
            load_argument_registers(argument);
    }
    if (hasSpecialArgument) {
        if (callArgument11 != 0 && copts.operandsDebug == 0)
            PCodeUtilities_EmitInstruction(PC_CREQV, 1, 2, 1, 2, 1, 2);
        else
            PCodeUtilities_EmitInstruction(PC_CRXOR, 1, 2, 1, 2, 1, 2);
    }
    if (expr->type == EOBJREF) {
        if (CParser_IsVirtualFunction(expr->data.objref, &resolvedObject, &resolvedOffset)) {
            if (Type_RequiresMemoryReturn(returnType))
                firstRegister = 4;
            else
                firstRegister = 3;
            sourceRegister = firstRegister;
            value = resolvedOffset;
            object = resolvedObject;
            if (object->flags & CLASS_HANDLEOBJECT) {
                emit_opcode_with_base_offset(PC_LWZ, 12, sourceRegister, NULL, 0);
                emit_opcode_with_base_offset(PC_LWZ, 12, 12, NULL, object->vtable->offset);
            } else
                emit_opcode_with_base_offset(PC_LWZ, 12, sourceRegister, NULL, object->vtable->offset);
            emit_opcode_with_base_offset(PC_LWZ, 12, 12, NULL, value);
            op.reg = 12;
            op.kind = OpndType_GPR;
            emit12(&op, callArgument20, callArgument1c, callArgument18);
        } else if (alternateExpr != NULL) {
            if (alternateExpr->hascall == 0) {
                data_00560648[alternateExpr->type](alternateExpr, 12, 0, &op);
                if (op.kind != OpndType_GPR)
                    Operands_ForceGPR(&op, (Type *)&void_ptr, 12);
            }
            emitobj12(expr->data.objref, &op, callArgument20, callArgument1c, callArgument18);
        } else {
            value = ObjGen_PPC_EABI_GetSectionAlignmentOrKind(expr->data.objref);
            if (value == 12) {
                if (expr->hascall == 0)
                    data_00560648[expr->type](expr, 12, 0, &op);
                if (op.kind != OpndType_GPR)
                    Operands_ForceGPR(&op, (Type *)&void_ptr, 12);
                emit12(&op, callArgument20, callArgument1c, callArgument18);
            } else
                PCodeUtilities_EmitObjectInstructionWithPayload(expr->data.objref, 0, callArgument20, callArgument1c,
                                                                callArgument18);
        }
    } else {
        if (expr->hascall == 0)
            data_00560648[expr->type](expr, 12, 0, &op);
        if (op.kind != OpndType_GPR)
            Operands_ForceGPR(&op, (Type *)&void_ptr, 12);
        emit12(&op, callArgument20, callArgument1c, callArgument18);
    }
    if (returnType->type == TYPEFLOAT && (copts.operandsDebug == 0 || returnType->type != TYPEFLOAT)) {
        result->kind = OpndType_FPR;
        result->reg = gUsedVirtualRegistersFPR;
        gUsedVirtualRegistersFPR++;
        PCodeUtilities_EmitInstruction(PC_FMR, result->reg, 1);
    } else if (returnType->type == TYPESTRUCT && (structureType = TYPE_STRUCT(returnType)->stype) >= 4 &&
               structureType <= 14) {
        result->kind = OpndType_VR;
        result->reg = gUsedVirtualRegistersVR;
        gUsedVirtualRegistersVR++;
        PCodeUtilities_EmitInstruction(PC_VMR, result->reg, 2);
    } else if ((((unsigned char)(returnType->type - 4U) <= 1) && !Type_RequiresMemoryReturn(returnType)) ||
               returnType->type == TYPEINT || returnType->type == TYPEENUM || returnType->type == TYPEPOINTER ||
               (returnType->type == TYPEMEMBERPOINTER && returnType->size == 4) ||
               (copts.operandsDebug != 0 && returnType->type == TYPEFLOAT)) {
        if (returnType->size > 4) {
            result->kind = OpndType_GPRPair;
            result->reg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR++;
            result->regHi = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, return_gpr_first);
            PCodeUtilities_EmitInstruction(PC_MR, result->regHi, returnRegHi);
        } else {
            result->kind = OpndType_GPR;
            result->reg = gUsedVirtualRegistersGPR;
            gUsedVirtualRegistersGPR++;
            PCodeUtilities_EmitInstruction(PC_MR, result->reg, 3);
        }
    } else {
        result->kind = OpndType_Immediate;
        result->immediate = 0;
    }
}
