#define CERROR_FILE "CodeGen.c"
#include "compiler/common.h"
#include "compiler/CodeGen.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/COptimizer.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/Coloring.h"
#include "compiler/DWARF.h"
#include "compiler/DumpIR.h"
#include "compiler/Exceptions.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Operands.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeListing.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Peephole.h"
#include "compiler/Registers.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/Scheduler.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "compiler/TOC.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>

static struct TemporaryObjectEntry *temporary_objects;

typedef void (*XGenProc)(ENode *, UInt16, UInt16, Operand *);

typedef void (*CGGenFunc)(ENode *enode, SInt32 a, SInt32 b, Operand *dest);

typedef void (*RegAssignFunc)(Object *, SInt32);

static inline void IrOptimizer_CheckVectorByteConstant(const CInt64 *value, TypeStruct *vectorType)
{
    if (copts.extended_errorcheck) {
        if (vectorType->stype == STRUCT_VECTOR_UCHAR) {
            if (!CInt64_IsInURange(*value, 1))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        } else {
            if (!CInt64_IsInRange(*value, 1))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        }
    }
}

static inline void IrOptimizer_CheckVectorShortConstant(const CInt64 *value, TypeStruct *vectorType)
{
    if (copts.extended_errorcheck) {
        SInt32 elementType = vectorType->stype;
        if (elementType == 7 || elementType == 14) {
            if (!CInt64_IsInURange(*value, 2))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        } else {
            if (!CInt64_IsInRange(*value, 2))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        }
    }
}

static inline void IrOptimizer_CheckVectorLongConstant(const CInt64 *value, TypeStruct *vectorType)
{
    if (copts.extended_errorcheck) {
        if (vectorType->stype == STRUCT_VECTOR_UINT) {
            if (!CInt64_IsInURange(*value, 4))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        } else {
            if (!CInt64_IsInRange(*value, 4))
                PPCError_ReportDiagnostic(0x71, vectorType, 0);
        }
    }
}

static inline void CheckPragmaEnd(void)
{
    SInt16 t;

    if (CPrepTokenizer_ScanToken() != -7) {
        CPrep_ReportError(0x71);
        do {
            t = CPrepTokenizer_ScanToken();
        } while (t != -7 && t);
    }
}

static inline void SkipPragma(void)
{
    SInt16 t;

    if (CPrepTokenizer_ScanToken() != -7) {
        do {
            t = CPrepTokenizer_ScanToken();
        } while (t != -7 && t);
    }
}

static inline void CodeGen_SetProcessorOption(UInt8 processor)
{
    CPrep_SaveAndSetOption(4, processor);
    if (!copts.instructionSchedulingMode)
        CPrep_SaveAndSetOption(5, 2);
}

static inline void initialize_codegen_list(Object **list)
{
    *list = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
}

static inline HashNameNode *InternCodeGenName(char *text)
{
    return GetHashNameNode(text);
}

static UInt8 CodeGen_00435040_kind(Type *ftype)
{
    if (Type_RequiresMemoryReturn(ftype))
        return 4;
    return 3;
}

static inline void emit_block(CLabel *block)
{
    if (block->pclabel == NULL)
        block->pclabel = PCode_NewLabel();
    if (block->pclabel->resolved == 0)
        PCodeUtilities_ResolveLabel(block->pclabel);
}

static inline void branch_block(CLabel *block)
{
    if (block->pclabel == NULL)
        block->pclabel = PCode_NewLabel();
    PCodeUtilities_EmitBranch(block->pclabel);
}

static inline Boolean is_tail(Statement *node)
{
    for (node = node->next; node; node = node->next)
        if (node->type > 2)
            return 0;
    return 1;
}

static int IsVolatile(Object *obj)
{
    if (obj->type->type == TYPEPOINTER)
        return ((TypePointer *)obj->type)->qual & Q_VOLATILE;
    return obj->qual & Q_VOLATILE;
}

static VarInfo *new_varinfo(void)
{
    VarInfo *info;

    info = lalloc(0x2c);
    memclrw(info, 0x2c);
    info->usage = 0;
    info->deftoken = *CPrep_GetLastBufferedToken();
    info->varnumber = next_varnumber;
    ++next_varnumber;
    info->noregister = 0;
    info->used = 0;
    info->reg = 0;
    info->regHi = 0;
    info->in_param_area = 0;
    return info;
}

static inline Boolean TooManyStructArgs(SInt16 n)
{
    Boolean r;

    r = 1;
    if (n <= 13) {
        r = 0;
    }
    return r;
}

static inline Boolean IsFloatArgument(Type *type)
{
    return type->type == TYPEFLOAT;
}

Object *CodeGen_AllocateTemporaryObject(Type *type)
{
    Object *object;
    VarInfo *info;
    struct TemporaryObjectEntry *entry;
    struct TemporaryObjectEntry *newEntry;

    entry = temporary_objects;
    while (entry != NULL) {
        object = entry->object;
        if ((object->u.var.uid == 0) && (object->type == type)) {
            object->u.var.uid = 1;
            return object;
        }
        entry = entry->next;
    }
    object = (Object *)lalloc(sizeof(*object));
    memclrw(object, sizeof(*object));
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->datatype = DLOCAL;
    object->type = type;
    object->name = (HashNameNode *)CParser_GetUniqueName();
    info = (VarInfo *)lalloc(sizeof(*info));
    memclrw(info, sizeof(*info));
    info->usage = 0;
    info->deftoken = *CPrep_GetLastBufferedToken();
    info->varnumber = next_varnumber;
    ++next_varnumber;
    info->noregister = 0;
    info->used = 0;
    info->reg = 0;
    info->regHi = 0;
    info->in_param_area = 0;
    object->u.var.info = info;
    object->u.var.uid = 1;
    newEntry = (struct TemporaryObjectEntry *)lalloc(sizeof(*newEntry));
    memclrw(newEntry, sizeof(*newEntry));
    newEntry->next = temporary_objects;
    newEntry->object = object;
    temporary_objects = newEntry;
    return object;
}

void CodeGen_AllocateArgumentSlots(Object *function, Boolean isVariadic, Boolean hasStructReturn)
{
    Object *obj;
    Type *type;
    SInt32 structKind;
    VarInfo *info;
    Boolean useStack;
    Boolean floatInGPR;
    SInt32 offset = 0;
    SInt16 gpr = 3;
    SInt16 fpr = 1;
    SInt16 structArgCount = 2;
    ObjectList *arg = arguments;

    while (arg != NULL) {
        obj = arg->object;
        type = obj->type;
        if (type->type != TYPESTRUCT || (structKind = ((TypeStruct *)type)->stype) < 4 || structKind > 14) {
            useStack = 1;
            if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4 &&
                 copts.incompatible_sfpe_double_params == 0)) {
                if ((gpr % 2) == 0) {
                    gpr++;
                }
            }
            if (type->type == TYPEFLOAT && (!copts.operandsDebug || type->type != TYPEFLOAT)) {
                if (fpr <= 8) {
                    useStack = 0;
                }
                fpr++;
            } else {
                if (gpr <= 10) {
                    useStack = 0;
                }
                if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                    ((floatInGPR = copts.operandsDebug) && type->type == TYPEFLOAT && type->size != 4)) {
                    gpr += 2;
                } else if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                           (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                           (floatInGPR && type->type == TYPEFLOAT && type->size == 4)) {
                    gpr++;
                } else {
                    gpr += type->size >> 2;
                    if ((type->size & 3) != 0) {
                        gpr++;
                    }
                }
            }
            if (useStack) {
                obj->datatype = DLOCAL;
                info = new_varinfo();
                obj->u.var.info = info;
                if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                    (type->type == TYPEFLOAT && type->size != 4)) {
                    offset = (offset + 7) & ~7;
                } else {
                    offset = (offset + 3) & ~3;
                }
                obj->u.var.uid = offset;
                Registers_GetInfo(obj)->in_param_area = 1;
                if (!copts.littleendian) {
                    if ((obj->type->type == TYPEINT || obj->type->type == TYPEENUM) && obj->type->size < 4) {
                        obj->u.var.uid += 4 - obj->type->size;
                    }
                }
                IsFloatArgument(type);
                offset += obj->type->size;
            } else {
                obj->datatype = DLOCAL;
                info = new_varinfo();
                obj->u.var.info = info;
                obj->u.var.uid = 0;
                StackFrameEABI_AllocateObjectSlot(obj);
            }
        } else {
            info = new_varinfo();
            obj->u.var.info = info;
            obj->u.var.uid = 0;
            Registers_GetInfo(obj)->in_param_area = 1;
            obj->datatype = DLOCAL;
            if (TooManyStructArgs(structArgCount)) {
                offset = (offset + 0x17) & ~0xf;
                obj->u.var.uid = offset = offset - 8;
                offset += 0x10;
            } else {
                obj->datatype = DLOCAL;
                info = new_varinfo();
                obj->u.var.info = info;
                obj->u.var.uid = 0;
                StackFrameEABI_AllocateObjectSlot(obj);
            }
            structArgCount++;
        }
        arg = arg->next;
    }
    data_00588274 = offset;
}

void CodeGen_EnumerateArgumentRegisters(void (*callback)(Object *argument, SInt16 registerNumber))
{
    Type *argumentType;
    ObjectList *parameter = arguments;
    SInt16 generalRegister = 3;
    SInt16 floatRegister = 1;
    SInt16 vectorRegister = 2;

    while (parameter != NULL) {
        Object *object = parameter->object;
        argumentType = object->type;

        if (((argumentType->type == TYPEINT || argumentType->type == TYPEENUM) && argumentType->size == 8) ||
            (copts.operandsDebug != 0 && argumentType->type == TYPEFLOAT && argumentType->size != 4 &&
             copts.incompatible_sfpe_double_params == 0)) {
            if (generalRegister % 2 == 0)
                generalRegister++;
        }

        if (argumentType->type == TYPEFLOAT && !(copts.operandsDebug != 0 && argumentType->type == TYPEFLOAT)) {
            callback(object, floatRegister <= 8 ? floatRegister : 0);
            floatRegister++;
        } else if (IS_TYPE_VECTOR(argumentType)) {
            callback(object, vectorRegister <= 13 ? vectorRegister : 0);
            vectorRegister++;
        } else {
            callback(object, generalRegister <= 10 ? generalRegister : 0);
            if (argumentType->type == TYPEINT || argumentType->type == TYPEENUM || argumentType->type == TYPEPOINTER ||
                (argumentType->type == TYPEMEMBERPOINTER && argumentType->size == 4) ||
                (copts.operandsDebug != 0 && argumentType->type == TYPEFLOAT)) {
                if (argumentType->size <= 4)
                    generalRegister++;
                else
                    generalRegister += 2;
            } else {
                CError_FATAL(448);
                generalRegister += argumentType->size >> 2;
                if (argumentType->size & 3)
                    generalRegister++;
            }
        }
        parameter = parameter->next;
    }

    floatRegister--;
    generalRegister--;
    vectorRegister--;
    data_00588476 = generalRegister;
    data_00588478 = floatRegister;
    data_00588434 = vectorRegister;
}

void bind_object_register(Object *object, SInt16 reg)
{
    VarInfo *info = Registers_GetInfo(object);
    Type *type = object->type;

    if (reg != 0 && info->noregister == 0 && info->used != 0) {
        if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
            (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
            (copts.operandsDebug != 0 && type->type == TYPEFLOAT)) {
            if (type->size <= 4) {
                Registers_BindGPR(object, reg);
            } else if (reg < 10) {
                if (copts.littleendian != 0) {
                    Registers_BindGPRPair(object, reg, reg + 1);
                } else {
                    Registers_BindGPRPair(object, reg + 1, reg);
                }
            }
        } else if (type->type == TYPEFLOAT) {
            Registers_BindFPR(object, reg);
        } else if (type->type == TYPESTRUCT) {
            TypeStruct *structType = (TypeStruct *)type;
            int structKind;
            if ((structKind = structType->stype) >= STRUCT_VECTOR_UCHAR && structKind <= STRUCT_VECTOR_PIXEL)
                Registers_BindVR(object, reg);
        }
    }
}

#define IS_VR_STRUCT(tp) ((SInt32)((SInt8 *)(tp))[0xe] >= 4 && (SInt32)((SInt8 *)(tp))[0xe] <= 0xe)

void allocate_object_registers(void)
{
    Type *type;
    ObjectList *list;
    Object *object;
    VarInfo *info;

    for (list = gInitialObjectList_005882ac; list != NULL; list = list->next) {
        UInt32 qual;

        object = list->object;
        info = Registers_GetInfo(object);
        type = object->type;
        if (info->used != 0 && info->noregister == 0) {
            if (object->type->type == TYPEPOINTER)
                qual = TYPE_POINTER(object->type)->qual;
            else
                qual = object->qual;
            qual &= Q_VOLATILE;
            if (qual == 0 && info->reg == 0) {
                if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                    (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                    (copts.operandsDebug && type->type == TYPEFLOAT)) {
                    if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                        (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4))
                        Registers_AllocateGPRPair(object);
                    else
                        Registers_AllocateGPR(object);
                } else if (type->type == TYPEFLOAT) {
                    Registers_AllocateFPR(object);
                }
            }
        }
    }

    Registers_SnapshotInitialObjectRange();

    for (list = arguments; list != NULL; list = list->next) {
        UInt32 qual;

        object = list->object;
        info = Registers_GetInfo(object);
        type = object->type;
        if (info->used != 0 && info->noregister == 0) {
            if (object->type->type == TYPEPOINTER)
                qual = TYPE_POINTER(object->type)->qual;
            else
                qual = object->qual;
            qual &= Q_VOLATILE;
            if (qual == 0 && info->reg == 0) {
                if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                    (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                    (copts.operandsDebug && type->type == TYPEFLOAT)) {
                    if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                        (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4))
                        Registers_AllocateGPRPair(object);
                    else
                        Registers_AllocateGPR(object);
                } else if (type->type == TYPEFLOAT) {
                    Registers_AllocateFPR(object);
                } else if (type->type == TYPESTRUCT && IS_VR_STRUCT(type)) {
                    Registers_AllocateVR(object);
                }
            }
        }
    }

    for (list = locals; list != NULL; list = list->next) {
        UInt32 qual;

        object = list->object;
        if (CParser_IsNullOrAtOrDollarPrefixedName(object->name) == 0) {
            info = Registers_GetInfo(object);
            type = object->type;
            if (info->used != 0 && info->noregister == 0) {
                if (object->type->type == TYPEPOINTER)
                    qual = TYPE_POINTER(object->type)->qual;
                else
                    qual = object->qual;
                qual &= Q_VOLATILE;
                if (qual == 0 && info->reg == 0) {
                    if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                        (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                        (copts.operandsDebug && type->type == TYPEFLOAT)) {
                        if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                            (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4))
                            Registers_AllocateGPRPair(object);
                        else
                            Registers_AllocateGPR(object);
                    } else if (type->type == TYPEFLOAT) {
                        Registers_AllocateFPR(object);
                    } else if (type->type == TYPESTRUCT && IS_VR_STRUCT(type)) {
                        Registers_AllocateVR(object);
                    }
                }
            }
        }
    }

    Registers_BeginCoalesceWindow();

    for (list = locals; list != NULL; list = list->next) {
        UInt32 qual;

        object = list->object;
        if (CParser_IsNullOrAtOrDollarPrefixedName(object->name) != 0) {
            info = Registers_GetInfo(object);
            type = object->type;
            if (info->used != 0 && info->noregister == 0) {
                if (object->type->type == TYPEPOINTER)
                    qual = TYPE_POINTER(object->type)->qual;
                else
                    qual = object->qual;
                qual &= Q_VOLATILE;
                if (qual == 0 && info->reg == 0) {
                    if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                        (type->type == TYPEMEMBERPOINTER && type->size == 4) ||
                        (copts.operandsDebug && type->type == TYPEFLOAT)) {
                        if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                            (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4))
                            Registers_AllocateGPRPair(object);
                        else
                            Registers_AllocateGPR(object);
                    } else if (type->type == TYPEFLOAT) {
                        Registers_AllocateFPR(object);
                    } else if (type->type == TYPESTRUCT && IS_VR_STRUCT(type)) {
                        Registers_AllocateVR(object);
                    }
                }
            }
        }
    }

    for (list = gTrailingObjectList_005876a0; list != NULL; list = list->next) {
        object = list->object;
        info = Registers_GetInfo(object);
        if (info->used != 0 && (SInt32)info->usage > 1)
            Registers_AllocateGPR(object);
    }
}

#define TYPEINT 1
#define TYPEFLOAT 2
#define TYPEENUM 3
#define TYPEMEMBERPOINTER 10
#define TYPEPOINTER 11

void allocate_saved_gprs(void)
{
    SInt32 bestWeight;
    Object *object;
    Object *otherObject;
    VarInfo *info;
    UInt32 qualifiers;
    Object *candidate;
    ObjectList *entry;
    VarInfo *otherInfo;
    Type *otherType;
    Type *type;
    UInt32 otherQualifiers;
    char useFloat;
    ObjectList *otherEntry;
    ObjectList *specialEntry;
    VarInfo *specialInfo;
    Object *best;
    while (gAvailableSavedGPRs != 0) {
        best = NULL;
        bestWeight = -1;
        if ((data_00588224 & 2) == 0) {
            entry = arguments;
            while (entry != NULL) {
                object = entry->object;
                info = Registers_GetInfo(object);
                type = object->type;
                if (info->reg == 0 && info->used != 0 && info->noregister == 0) {
                    if (object->type->type == TYPEPOINTER) {
                        qualifiers = TYPE_POINTER(object->type)->qual;
                    } else {
                        qualifiers = object->qual;
                    }
                    qualifiers = qualifiers & Q_VOLATILE;
                    if (qualifiers == 0 && info->usage >= bestWeight && info->usage >= 2 &&
                        (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
                         type->type == TYPEMEMBERPOINTER && type->size == 4 ||
                         copts.operandsDebug != 0 && type->type == TYPEFLOAT) &&
                        ((type->type != TYPEINT && type->type != TYPEENUM || type->size != 8) &&
                             (copts.operandsDebug == 0 || type->type != TYPEFLOAT || type->size == 4) ||
                         gAvailableSavedGPRs >= 2)) {
                        best = object;
                        bestWeight = info->usage;
                    }
                }
                entry = entry->next;
            }
        }
        if ((data_00588224 & 2) == 0) {
            otherEntry = locals;
            while (otherEntry != NULL) {
                otherObject = otherEntry->object;
                otherInfo = Registers_GetInfo(otherObject);
                otherType = otherObject->type;
                if (otherInfo->reg == 0 && otherInfo->used != 0 && otherInfo->noregister == 0) {
                    if (otherObject->type->type == TYPEPOINTER) {
                        otherQualifiers = TYPE_POINTER(otherObject->type)->qual;
                    } else {
                        otherQualifiers = otherObject->qual;
                    }
                    otherQualifiers = otherQualifiers & 2;
                    if (otherQualifiers == 0 && otherInfo->usage >= bestWeight && otherInfo->usage >= 2 &&
                        (otherType->type == TYPEINT || otherType->type == TYPEENUM || otherType->type == TYPEPOINTER ||
                         otherType->type == TYPEMEMBERPOINTER && otherType->size == 4)) {
                        if (((useFloat = copts.operandsDebug) == 0 || otherType->type != TYPEFLOAT) &&
                            ((otherType->type != TYPEINT && otherType->type != TYPEENUM || otherType->size != 8) &&
                                 (useFloat == 0 || otherType->type != TYPEFLOAT || otherType->size == 4) ||
                             gAvailableSavedGPRs >= 2)) {
                            best = otherObject;
                            bestWeight = otherInfo->usage;
                        }
                    }
                }
                otherEntry = otherEntry->next;
            }
        }
        specialEntry = gTrailingObjectList_005876a0;
        while (specialEntry != NULL) {
            candidate = specialEntry->object;
            specialInfo = Registers_GetInfo(candidate);
            if (specialInfo->reg == 0 && specialInfo->used != 0 && specialInfo->usage >= bestWeight &&
                specialInfo->usage >= 3) {
                best = candidate;
                bestWeight = specialInfo->usage;
            }
            specialEntry = specialEntry->next;
        }
        if (best == NULL) {
            break;
        }
        if (((best->type->type == TYPEINT || best->type->type == TYPEENUM) && best->type->size == 8) ||
            (copts.operandsDebug != 0 && best->type->type == TYPEFLOAT && best->type->size != 4)) {
            Registers_AllocateGPRPair(best);
        } else {
            Registers_AllocateGPR(best);
        }
    }
}

void allocate_saved_fprs(void)
{
    SInt32 bestWeight;
    Object *object;
    Object *otherObject;
    ObjectList *link;
    VarInfo *info;
    UInt32 qualifiers;
    ObjectList *otherLink;
    Object *bestObject;
    VarInfo *otherInfo;
    UInt32 otherQualifiers;
    while (gAvailableSavedFPRs != 0) {
        bestObject = NULL;
        bestWeight = -1;
        if ((data_00588224 & 2) == 0) {
            link = arguments;
            while (link != NULL) {
                object = link->object;
                info = Registers_GetInfo(object);
                if (info->reg == 0 && info->used != 0 && info->noregister == 0) {
                    if (object->type->type == TYPEPOINTER) {
                        qualifiers = TYPE_POINTER(object->type)->qual;
                    } else {
                        qualifiers = object->qual;
                    }
                    qualifiers = qualifiers & Q_VOLATILE;
                    if (qualifiers == 0 && info->usage >= bestWeight && info->usage >= 2 &&
                        object->type->type == TYPEFLOAT) {
                        bestObject = object;
                        bestWeight = info->usage;
                    }
                }
                link = link->next;
            }
        }
        if ((data_00588224 & 2) == 0) {
            otherLink = locals;
            while (otherLink != NULL) {
                otherObject = otherLink->object;
                otherInfo = Registers_GetInfo(otherObject);
                if (otherInfo->reg == 0 && otherInfo->used != 0 && otherInfo->noregister == 0) {
                    if (otherObject->type->type == TYPEPOINTER) {
                        otherQualifiers = TYPE_POINTER(otherObject->type)->qual;
                    } else {
                        otherQualifiers = otherObject->qual;
                    }
                    otherQualifiers = otherQualifiers & Q_VOLATILE;
                    if (otherQualifiers == 0 && otherInfo->usage >= bestWeight && otherInfo->usage >= 2 &&
                        otherObject->type->type == TYPEFLOAT) {
                        bestObject = otherObject;
                        bestWeight = otherInfo->usage;
                    }
                }
                otherLink = otherLink->next;
            }
        }
        if (bestObject == NULL) {
            break;
        }
        Registers_AllocateFPR(bestObject);
    }
}

void allocate_saved_vrs(void)
{
    ObjectList *list;
    Object *object;
    Object *best;
    SInt32 bestUsage;
    VarInfo *info;

    while (gAvailableSavedVRs) {
        best = NULL;
        bestUsage = -1;
        if (!(data_00588224 & 2)) {
            for (list = arguments; list; list = list->next) {
                object = list->object;
                info = Registers_GetInfo(object);
                if (!info->reg && info->used && !info->noregister) {
                    if (!IsVolatile(object) && info->usage >= bestUsage && info->usage >= 2 &&
                        IS_TYPE_VECTOR(object->type)) {
                        best = object;
                        bestUsage = info->usage;
                    }
                }
            }
        }
        if (!(data_00588224 & 2)) {
            for (list = locals; list; list = list->next) {
                object = list->object;
                info = Registers_GetInfo(object);
                if (!info->reg && info->used && !info->noregister) {
                    if (!IsVolatile(object) && info->usage >= bestUsage && info->usage >= 2 &&
                        IS_TYPE_VECTOR(object->type)) {
                        best = object;
                        bestUsage = info->usage;
                    }
                }
            }
        }
        if (!best)
            break;
        Registers_AllocateVR(best);
    }
}

void allocate_registers_and_local_slots(void)
{
    ObjectList *local;
    Type *type;
    TypeStruct *elementType;

    gVectorArrayConversion = 0;
    if (data_00588521 > 0 && gUseVirtualRegisterNumbers_00587f00 == 0)
        CodeGen_EnumerateArgumentRegisters(bind_object_register);
    if (gUseVirtualRegisterNumbers_00587f00 != 0) {
        allocate_object_registers();
    } else {
        allocate_saved_gprs();
        if (copts.operandsDebug == 0)
            allocate_saved_fprs();
        allocate_saved_vrs();
    }
    for (local = locals; local != NULL; local = local->next) {
        if ((Registers_GetInfo(local->object) != NULL ? Registers_GetInfo(local->object)->reg : 0) == 0)
            StackFrameEABI_AllocateObjectSlot(local->object);
        if ((type = local->object->type) != NULL && type->type == TYPEARRAY &&
            (elementType = TYPE_STRUCT(TPTR_TARGET(type)))->type == TYPESTRUCT) {
            SInt32 structKind = elementType->stype;
            if (structKind >= 4 && structKind <= 0xe)
                gVectorArrayConversion = 1;
        }
    }
}

void emit_dlocal_initialization(Object *object, SInt16 reg)
{
    VarInfo *registers;
    Type *type;
    SInt32 bytes;
    SInt32 offset;
    SInt32 structKind;

    registers = Registers_GetInfo(object);
    type = object->type;
    CError_ASSERT(908, object->datatype == DLOCAL);
    if (registers->used == 0)
        return;

    if (reg != 0) {
        if (registers->reg != 0) {
            UInt8 typecode;
            UInt8 use_gpr;
            if ((((typecode = type->type) == TYPEINT || typecode == TYPEENUM) && type->size == 8) ||
                ((use_gpr = copts.operandsDebug) && typecode == TYPEFLOAT && type->size != 4)) {
                if (copts.littleendian != 0) {
                    if (registers->reg != reg)
                        PCodeUtilities_EmitInstruction(PC_MR, registers->reg, reg);
                    if (reg < 10) {
                        CError_ASSERT(923, registers->regHi != reg && registers->reg != reg + 1);
                        if (registers->regHi != reg + 1)
                            PCodeUtilities_EmitInstruction(PC_MR, registers->regHi, reg + 1);
                    } else {
                        CError_FATAL(929);
                        has_dlocal_initialization = 1;
                        emit_opcode_with_base_offset(PC_LWZ, registers->regHi, stack_base_reg, object,
                                                     high_word_offset);
                    }
                } else {
                    if (registers->regHi != reg)
                        PCodeUtilities_EmitInstruction(PC_MR, registers->regHi, reg);
                    if (reg < 10) {
                        CError_ASSERT(939, registers->reg != reg && registers->regHi != reg + 1);
                        if (registers->reg != reg + 1)
                            PCodeUtilities_EmitInstruction(PC_MR, registers->reg, reg + 1);
                    } else {
                        CError_FATAL(945);
                        has_dlocal_initialization = 1;
                        emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, low_word_offset);
                    }
                }
            } else if (registers->reg != reg) {
                if (typecode == TYPEFLOAT && !(use_gpr && typecode == TYPEFLOAT)) {
                    PCodeUtilities_EmitInstruction(PC_FMR, registers->reg, reg);
                } else if (typecode == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= STRUCT_VECTOR_UCHAR &&
                           structKind <= STRUCT_VECTOR_PIXEL) {
                    PCodeUtilities_EmitInstruction(PC_VMR, registers->reg, reg);
                } else {
                    PCodeUtilities_EmitInstruction(PC_MR, registers->reg, reg);
                }
            }
        } else {
            has_dlocal_initialization = 1;
            if (type->type == TYPEPOINTER || (type->type == TYPEMEMBERPOINTER && type->size == 4)) {
                emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, 0);
            } else if (type->type == TYPEINT || type->type == TYPEENUM) {
                switch (type->size) {
                    case 1:
                        emit_opcode_with_base_offset(PC_STB, reg, stack_base_reg, object, 0);
                        break;
                    case 2:
                        emit_opcode_with_base_offset(PC_STH, reg, stack_base_reg, object, 0);
                        break;
                    case 4:
                        emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, 0);
                        break;
                    case 8:
                        emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, 0);
                        if (reg < 10)
                            emit_opcode_with_base_offset(PC_STW, reg + 1, stack_base_reg, object, 4);
                        break;
                    default:
                        CError_FATAL(983);
                        break;
                }
            } else if (copts.operandsDebug && type->type == TYPEFLOAT) {
                if (type->size == 4) {
                    emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, 0);
                } else {
                    emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, 0);
                    emit_opcode_with_base_offset(PC_STW, reg + 1, stack_base_reg, object, 4);
                }
            } else if (type->type == TYPEFLOAT) {
                emit_opcode_with_base_offset(type->size == 4 ? PC_STFS : PC_STFD, reg, stack_base_reg, object, 0);
            } else if (type->type == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= STRUCT_VECTOR_UCHAR &&
                       structKind <= STRUCT_VECTOR_PIXEL) {
                emit_opcode_with_base_offset(PC_STVX, reg, stack_base_reg, object, 0);
            } else {
                bytes = (11 - reg) * 4;
                if (bytes > object->type->size)
                    bytes = object->type->size;
                offset = 0;
                while (bytes > 0) {
                    emit_opcode_with_base_offset(PC_STW, reg, stack_base_reg, object, offset);
                    bytes -= 4;
                    reg++;
                    offset += 4;
                }
            }
        }
    } else if (registers->reg != 0) {
        has_dlocal_initialization = 1;
        if (type->type == TYPEPOINTER || (type->type == TYPEMEMBERPOINTER && type->size == 4)) {
            emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, 0);
        } else if (copts.operandsDebug && type->type == TYPEFLOAT) {
            if (type->size == 4) {
                emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, 0);
            } else {
                emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, low_word_offset);
                emit_opcode_with_base_offset(PC_LWZ, registers->regHi, stack_base_reg, object, high_word_offset);
            }
        } else if (type->type == TYPEFLOAT) {
            emit_opcode_with_base_offset(type->size == 4 ? PC_LFS : PC_LFD, registers->reg, stack_base_reg, object, 0);
        } else if (type->type == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= STRUCT_VECTOR_UCHAR &&
                   structKind <= STRUCT_VECTOR_PIXEL) {
            emit_opcode_with_base_offset(PC_LVX, registers->reg, stack_base_reg, object, 0);
        } else {
            switch (type->size) {
                case 1:
                    emit_opcode_with_base_offset(PC_LBZ, registers->reg, stack_base_reg, object, 0);
                    break;
                case 2:
                    emit_opcode_with_base_offset(is_unsigned(type) ? PC_LHZ : PC_LHA, registers->reg, stack_base_reg,
                                                 object, 0);
                    break;
                case 4:
                    emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, 0);
                    break;
                case 8:
                    emit_opcode_with_base_offset(PC_LWZ, registers->regHi, stack_base_reg, object, high_word_offset);
                    emit_opcode_with_base_offset(PC_LWZ, registers->reg, stack_base_reg, object, low_word_offset);
                    break;
                default:
                    CError_FATAL(1043);
                    break;
            }
        }
    }
}

void emit_trailing_object_reg_moves(void)
{
    Operand op;
    ObjectList *elem;
    VarInfo *info;
    Object *obj;

    memclrw(&op, sizeof(op));
    for (elem = gTrailingObjectList_005876a0; elem != NULL; elem = elem->next) {
        obj = elem->object;
        info = Registers_GetInfo(obj);
        switch (obj->datatype) {
            case DDATA:
                if (info->reg != 0) {
                    op.kind = OpndType_IndirectSymbol;
                    op.object = obj;
                    Operands_Normalize(&op);
                    if (op.reg != info->reg)
                        PCodeUtilities_EmitInstruction(PC_MR, info->reg, op.reg);
                }
                break;
        }
    }
}

void CodeGen_AssignMissingEntryValues(Statement *entry)
{
    Statement *previous;
    PCodeLabel *value;

    previous = NULL;
    if (entry != NULL) {
        do {
            if ((entry->type == ST_LABEL) && (entry->label->pclabel == NULL)) {
                if ((previous != NULL) && (previous->type == ST_LABEL)) {
                    entry->label->pclabel = previous->label->pclabel;
                } else {
                    value = PCode_NewLabel();
                    entry->label->pclabel = value;
                }
            }
            previous = entry;
            entry = entry->next;
        } while (entry != NULL);
    }
}

void set_block_line_and_execution_weight(SInt32 value, unsigned int initial_value, unsigned int set_flag)
{
    PCodeBlock *block;
    PCodeLabel *new_value;
    block = gCurrentBlock;
    data_00587ffc = initial_value;
    if (gCurrentBlock->instruction_count == 0)
        block->execution_weight = initial_value;
    if (copts.filesyminfo != 0) {
        if (block->instruction_count > 0 && copts.instructionSchedulingMode == 0 &&
            (signed char)copts.deleteDeadInstructions < 3) {
            new_value = PCode_NewLabel();
            PCodeUtilities_ResolveLabel(new_value);
            block = gCurrentBlock;
        }
        if (block->line == -1 || block->instruction_count == 0)
            block->line = value;
    }
    if (block->instruction_count > 100) {
        new_value = PCode_NewLabel();
        PCodeUtilities_ResolveLabel(new_value);
    }
    if (set_flag != 0)
        block->flags |= 64U;
}

void fn_00436390(ENode *expression)
{
    Operand result;
    Type *type;
    int subtype;

    memclrw(&result, sizeof(result));
    data_00560648[expression->type](expression, 0, 0, &result);
    if ((expression->type == EINDIRECT) && ((result.flags & fIsVolatile) != 0)) {
        type = expression->rtype;
        if ((type->type == TYPEINT) || (type->type == TYPEENUM) || (type->type == TYPEPOINTER) ||
            ((type->type == TYPEMEMBERPOINTER) && (type->size == 4)) ||
            ((copts.operandsDebug != 0) && (type->type == TYPEFLOAT) && (type->size == 4))) {
            if (result.kind != OpndType_GPR) {
                Operands_ForceGPR(&result, type, 0);
            }
        } else if (type->type == TYPEFLOAT) {
            if (result.kind != OpndType_FPR) {
                Operands_ForceFPR(&result, type, 0);
            }
        } else if (type->type == TYPESTRUCT) {
            TypeStruct *structType = (TypeStruct *)type;
            if ((subtype = structType->stype) >= STRUCT_VECTOR_UCHAR && subtype <= STRUCT_VECTOR_PIXEL &&
                result.kind != OpndType_VR) {
                Operands_ForceVR(&result, type, 0);
            }
        }
    }
}

void generate_comparison_branch(ENode *enode, CLabel *context, SInt32 flag)
{
    Operand operand;
    Operand discardedResult;
    TemporaryObjectEntry *entry;

    memclrw(&operand, sizeof(operand));
    while (enode->type == EFORCELOAD || enode->type == ETYPCON || enode->type == ECOMMA) {
        if (!(enode->rtype->type == TYPEINT || enode->rtype->type == TYPEENUM || enode->rtype->type == TYPEPOINTER ||
              (enode->rtype->type == TYPEMEMBERPOINTER && enode->rtype->size == 4)))
            break;
        if (enode->type == ECOMMA) {
            memclrw(&discardedResult, sizeof(discardedResult));
            (*data_00560648[enode->data.diadic.left->type])(enode->data.diadic.left, 0, 0, &discardedResult);
            enode = enode->data.diadic.right;
        } else {
            enode = enode->data.monadic;
        }
    }
    if (context->pclabel == NULL)
        context->pclabel = PCode_NewLabel();
    {
        if ((copts.operandsDebug && enode->data.diadic.right->rtype->type == TYPEFLOAT) ||
            (copts.operandsDebug && enode->data.diadic.left->rtype->type == TYPEFLOAT)) {
            SFPE_PPC_EABI_GenerateComparison(enode, &operand, 0);
        } else if (((enode->data.diadic.right->rtype->type == TYPEINT ||
                     enode->data.diadic.right->rtype->type == TYPEENUM) &&
                    enode->data.diadic.right->rtype->size == 8) ||
                   ((enode->data.diadic.left->rtype->type == TYPEINT ||
                     enode->data.diadic.left->rtype->type == TYPEENUM) &&
                    enode->data.diadic.left->rtype->size == 8)) {
            InstrSelection_GenerateLongLongComparison(enode, &operand, 0);
        } else {
            InstrSelection_SelectComparison(enode, &operand);
        }
    }
    PCodeUtilities_EmitConditionBranch(operand.reg, operand.secondary_reg, flag, context->pclabel);
    for (entry = temporary_objects; entry != NULL; entry = entry->next)
        entry->object->u.var.uid = 0;
}

void generate_return(ENode *enode, Boolean flag)
{
    Operand result;
    Type *type;
    SInt32 structKind;

    memclrw(&result, sizeof(result));
    if (enode != NULL) {
        type = enode->rtype;
        if (type->type == TYPEINT || type->type == TYPEENUM || type->type == TYPEPOINTER ||
            (type->type == TYPEMEMBERPOINTER && type->size == 4) || (copts.operandsDebug && type->type == TYPEFLOAT)) {
            if (((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) ||
                (copts.operandsDebug && type->type == TYPEFLOAT && type->size != 4)) {
                data_00560648[enode->type](enode, return_gpr_first, returnRegHi, &result);
                Operands_ForceGPRPair(&result, type, return_gpr_first, returnRegHi);
            } else {
                data_00560648[enode->type](enode, 3, 0, &result);
                if (result.kind > 0)
                    Operands_ForceGPR(&result, type, 3);
                {
                    SInt32 returnRegister = result.reg;
                    if (returnRegister != 3)
                        PCodeUtilities_EmitInstruction(PC_MR, 3, returnRegister);
                }
            }
        } else if (type->type == TYPEFLOAT) {
            data_00560648[enode->type](enode, 1, 0, &result);
            if (result.kind != OpndType_FPR)
                Operands_ForceFPR(&result, type, 1);
            {
                SInt32 returnRegister = result.reg;
                if (returnRegister != 1)
                    PCodeUtilities_EmitInstruction(PC_FMR, 1, returnRegister);
            }
        } else if (type->type == TYPESTRUCT && (structKind = ((TypeStruct *)type)->stype) >= 4 && structKind <= 14) {
            data_00560648[enode->type](enode, 2, 0, &result);
            if (result.kind != OpndType_VR)
                Operands_ForceVR(&result, type, 2);
            {
                SInt32 returnRegister = result.reg;
                if (returnRegister != 2)
                    PCodeUtilities_EmitInstruction(PC_VMR, 2, returnRegister);
            }
        } else if ((type->type == TYPESTRUCT || type->type == TYPECLASS) && !Type_RequiresMemoryReturn(type)) {
            data_00560648[enode->type](enode, 0, 0, &result);
            if (type->size > 4) {
                Operands_ForceGPRPair(&result, (Type *)&stunsignedlonglong, return_gpr_first, returnRegHi);
            } else {
                if (result.kind != OpndType_GPR)
                    Operands_ForceGPR(&result, (Type *)&stunsignedlong, 3);
                {
                    SInt32 returnRegister = result.reg;
                    if (returnRegister != 3) {
                        PCodeUtilities_EmitInstruction(PC_MR, 3, returnRegister);
                        result.reg = 3;
                    }
                }
            }
        } else {
            data_00560648[enode->type](enode, 0, 0, &result);
        }
    }
    if (!flag) {
        if (return_label->pclabel == NULL)
            return_label->pclabel = PCode_NewLabel();
        PCodeUtilities_EmitBranch(return_label->pclabel);
        {
            TemporaryObjectEntry *entry;
            for (entry = temporary_objects; entry != NULL; entry = entry->next)
                entry->object->u.var.uid = 0;
        }
    }
}

void emit_name_string_address(const char *name)
{
    Operand op;
    SInt32 offset;
    Object *stringObject;

    memclrw(&op, sizeof(op));

    if (copts.poolstrings != 0) {
        NameEntry *entry = CInit_DeclarePooledString(name, strlen(name) + 1, 0);
        stringObject = entry->object;
        offset = entry->offset;
    } else {
        stringObject = CInit_DeclareString(name, strlen(name) + 1, 0, 0);
        offset = 0;
    }

    stringObject->section = ObjGen_PPC_EABI_GetHeaderIndex(2);

    {
        Object *indirectObject = fn_0049f230(stringObject, 0, 1);
        if (indirectObject != NULL) {
            op.kind = OpndType_IndirectSymbol;
            op.object = indirectObject;
        } else {
            op.kind = OpndType_Symbol;
            op.object = stringObject;
        }
    }

    if (op.kind != OpndType_GPR)
        Operands_ForceGPR(&op, (Type *)&void_ptr, 3);

    if (op.kind > 0) {
        CError_FATAL(1535);
    } else {
        SInt32 reg = op.reg;
        if (reg != 3) {
            PCodeUtilities_EmitInstruction(PC_MR, 3, reg);
            op.reg = 3;
        }
    }

    if (offset != 0) {
        if (offset != (SInt16)offset) {
            SInt32 high = (offset >> 16) + ((offset >> 15) & 1);
            PCodeUtilities_EmitInstruction(PC_ADDIS, 3, 3, 0, (SInt16)high);
            if ((SInt16)offset != 0)
                PCodeUtilities_EmitInstruction(PC_ADDI, 3, 3, 0, (SInt16)offset);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, 3, 3, 0, offset);
        }
    }

    PCodeUtilities_EmitObjectInstructionWithPayload(data_00588054, 1, 8, 0, 0);
}

void CodeGen_Generator(Statement *statements, Object *functionObject, Boolean context, Boolean zero)
{
    Statement *statementList;
    Statement *entry;
    TemporaryObjectEntry *temporaryEntry;
    PCodeBlock *frameBlock;
    ParsedAsmInstruction *expression;
    FuncArg *argument;
    Boolean hasSentinelArgument;
    PCodeLabel *entryLabel;
    PCodeBlock *firstEntryBlock, *secondEntryBlock, *commonEntryBlock;
    Operand operandBuffer;
    Statement *statement;
    Statement *previousStatement;
    UInt16 initialValue;

    if (copts.littleendian != 0) {
        high_word_offset = 4;
        low_word_offset = 0;
        returnRegHi = 4;
        return_gpr_first = 3;
        sfpe_right_operand_reg_hi = 6;
        sfpe_right_operand_reg = 5;
    } else {
        high_word_offset = 0;
        low_word_offset = 4;
        returnRegHi = 3;
        return_gpr_first = 4;
        sfpe_right_operand_reg_hi = 5;
        sfpe_right_operand_reg = 6;
    }
    has_dlocal_initialization = 0;
    gHasAltivecFrame = 0;
    data_005884ff = 0;
    if (((CPrepCU *)cprep_cu)->precompiling == 1)
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
    if (functionObject == NULL) {
        functionObject = TOC_CreateSinitObject();
        ObjGen_PPC_EABI_EmitObjectRelocation(functionObject);
    } else if (functionObject != NULL && functionObject->name != NULL) {
        cc_eabi_ppc_ReportCompilingFunction(functionObject->name->name);
    }
    gStackFrameSize = 0;
    argument = ((TypeFunc *)functionObject->type)->args;
    while (argument != NULL && argument != &elipsis)
        argument = argument->next;
    hasSentinelArgument = (Boolean)(argument == &elipsis);
    data_005882c0.record = NULL;
    return_operand.kind = 0;
    gStackFrameSize += StackFrameEABI_GetRecordSize(functionObject);
    classTypeUpdates = NULL;
    if ((Boolean)(argument == &elipsis))
        fn_004aa580();
    CodeGen_AllocateArgumentSlots(functionObject, context, hasSentinelArgument);
    data_0058851f = 0;
    data_00588224 = 0;
    gRunLevel2Pipeline = 0;
    gCurrentStatement = NULL;
    function_call_frames = NULL;
    switch_tables = NULL;
    temporary_objects = NULL;
    Exceptions_Reset();
    data_0058802c = newlabel();
    return_label = data_0058802c;
    if (copts.debug_listing)
        fn_004be830(statements, functionObject);
    statementList = DumpIR_OptimizeStatements(functionObject, statements);
    if (copts.debug_listing)
        fn_004be830(statementList, functionObject);
    Operands_ClearTrailingObjectInfo();
    Registers_InitRegisterState();
    fn_0049d420(statementList);
    if (copts.debug_listing)
        fn_004be830(statementList, functionObject);
    if (copts.profile)
        data_00588521 = 0;
    PCode_ResetBlocks();
    PCode_ResolveLabel((prologueBlock = PCode_CreateBlock()), (PCode_NewLabel()));
    prologueBlock->flags |= 1;
    if (hasSentinelArgument) {
        if (copts.operandsDebug != 0 || copts.debugEnabled == 0) {
            firstEntryBlock = NULL;
            secondEntryBlock = NULL;
            entryLabel = NULL;
            PCode_ResolveLabel((commonEntryBlock = PCode_CreateBlock()), (PCode_NewLabel()));
            PCode_AddSuccessor(prologueBlock, commonEntryBlock->labels);
        } else {
            PCode_ResolveLabel((firstEntryBlock = PCode_CreateBlock()), (PCode_NewLabel()));
            PCode_AddSuccessor(prologueBlock, firstEntryBlock->labels);
            PCode_ResolveLabel((secondEntryBlock = PCode_CreateBlock()), (PCode_NewLabel()));
            PCode_AddSuccessor(firstEntryBlock, secondEntryBlock->labels);
            PCode_ResolveLabel((commonEntryBlock = PCode_CreateBlock()), (entryLabel = PCode_NewLabel()));
            PCode_AddSuccessor(firstEntryBlock, commonEntryBlock->labels);
            PCode_AddSuccessor(secondEntryBlock, commonEntryBlock->labels);
            firstEntryBlock->flags |= 1;
            secondEntryBlock->flags |= 1;
        }
        commonEntryBlock->flags |= 1;
        PCode_ResolveLabel((frameBlock = PCode_CreateBlock()), (PCode_NewLabel()));
        PCode_AddSuccessor(commonEntryBlock, frameBlock->labels);
    } else {
        PCode_ResolveLabel((frameBlock = PCode_CreateBlock()), (PCode_NewLabel()));
        PCode_AddSuccessor(prologueBlock, frameBlock->labels);
    }
    StackFrameEABI_Initialize();
    Registers_SetupStackBaseReg();
    allocate_registers_and_local_slots();
    CodeGen_EnumerateArgumentRegisters(&emit_dlocal_initialization);
    if (copts.instructionSchedulingMode != 0 || copts.altivec_model != 0)
        PCodeUtilities_ResolveLabel(PCode_NewLabel());
    if (copts.profile)
        emit_name_string_address(COptimizer_GetFunctionObject(functionObject)->name);
    emit_trailing_object_reg_moves();
    previousStatement = NULL;
    for (entry = statementList->next; entry != NULL; entry = entry->next) {
        if (entry->type == ST_LABEL && entry->label->pclabel == NULL) {
            if (previousStatement != NULL && previousStatement->type == ST_LABEL)
                entry->label->pclabel = previousStatement->label->pclabel;
            else
                entry->label->pclabel = PCode_NewLabel();
        }
        previousStatement = entry;
    }
    Registers_CheckpointCoalesceWindow();
    for (statement = statementList->next; statement != NULL; statement = statement->next) {
        gCurrentStatement = statement;
        switch (statement->type) {
            case ST_EXPRESSION:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                fn_00436390(statement->expr);
                break;
            case ST_LABEL:
                emit_block(statement->label);
                for (entry = (Statement *)temporary_objects; entry != NULL;
                     entry = (Statement *)((TemporaryObjectEntry *)entry)->next)
                    ((TemporaryObjectEntry *)entry)->object->u.var.uid = 0;
                break;
            case ST_IFGOTO:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                generate_comparison_branch(statement->expr, statement->label, 1);
                break;
            case ST_IFNGOTO:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                generate_comparison_branch(statement->expr, statement->label, 0);
                break;
            case ST_GOTOEXPR: {
                struct TOCNameEntry *labelEntry;
                ENode *operand;
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                operand = statement->expr;
                memclrw(&operandBuffer, sizeof(operandBuffer));
                data_00560648[operand->type](operand, 0, 0, &operandBuffer);
                if (operandBuffer.kind != OpndType_GPR)
                    Operands_ForceGPR(&operandBuffer, (Type *)&void_ptr, 0);
                CError_ASSERT(1286, operandBuffer.kind == OpndType_GPR);
                for (labelEntry = toc_name_entries; labelEntry != NULL; labelEntry = labelEntry->next)
                    PCode_AddSuccessor(gCurrentBlock, labelEntry->label->pclabel);
                PCodeUtilities_EmitInstruction(PC_MTCTR, operandBuffer.reg);
                PCodeUtilities_EmitInstructionAndCreateBlock(NULL);
                break;
            }
            case ST_GOTO:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                branch_block(statement->label);
                for (temporaryEntry = temporary_objects; temporaryEntry != NULL; temporaryEntry = temporaryEntry->next)
                    temporaryEntry->object->u.var.uid = 0;
                break;
            case ST_RETURN:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                generate_return(statement->expr, is_tail(statement));
                break;
            case ST_SWITCH:
                initialValue = statement->value;
                set_block_line_and_execution_weight(statement->sourceoffset, initialValue,
                                                    (statement->flags & 0x10) != 0);
                Switch_GenerateSwitch(statement->expr, (SwitchInfo *)statement->label);
                break;
            case ST_BEGINCATCH: {
                Object *object = statement->expr->data.objref;
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                CError_ASSERT(1436, object->datatype == DLOCAL);
                emit_opcode_with_base_offset(PC_STW, 1, stack_base_reg, object, 0x14);
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                break;
            }
            case ST_ENDCATCHDTOR: {
                ENode *operand = statement->expr;
                CError_ASSERT(1845, operand->data.objref->datatype == DLOCAL);
                PCodeUtilities_EmitAddress(3, stack_base_reg, statement->expr->data.objref, 0);
                PCodeUtilities_EmitObjectInstructionWithPayload(data_005875a0, 1, 8, 0, 0);
            }
                /* fall through */
            case ST_ENDCATCH: {
                Object *object = statement->expr->data.objref;
                PCodeInstruction *instruction;
                CError_ASSERT(1463, object->datatype == DLOCAL);
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                emit_opcode_with_base_offset(PC_LWZ, 0, 1, NULL, 0);
                instruction = PCodeUtilities_CreateInstruction(0x22, 1, stack_base_reg, object, 0x14);
                instruction->flags |= fSideEffects;
                PCode_AppendInstruction(gCurrentBlock, instruction);
                emit_opcode_with_base_offset(PC_STW, 0, 1, NULL, 0);
                PCodeUtilities_ResolveLabel(PCode_NewLabel());
                break;
            }
            case ST_ASM:
                expression = (ParsedAsmInstruction *)statement->expr;
                if (expression != NULL) {
                    if ((expression->specialFlags & 1) != 0) {
                        CError_FATAL(1860);
                    } else {
                        PCodeUtilities_ResolveLabel(PCode_NewLabel());
                        gCurrentBlock->line = statement->sourceoffset;
                        InlineAsmPPC_GenerateAsmInstruction(statement);
                    }
                }
                break;
            case ST_NOP:
                break;
            default:
                CError_FATAL(1872);
                break;
        }
        Registers_UpdateCoalesceWindow();
    }
    Registers_CloseCoalesceWindow();
    emit_block(return_label);
    {
        TemporaryObjectEntry *returnTemporary;
        for (returnTemporary = temporary_objects; returnTemporary != NULL; returnTemporary = returnTemporary->next)
            returnTemporary->object->u.var.uid = 0;
    }
    gCurrentStatement = NULL;
    gReturnBlock = gCurrentBlock;
    gReturnBlock->flags |= 2;
    PCode_BuildPredecessors();
    PCode_UnlinkBlocksWithoutFlag4();
    if (copts.deleteDeadInstructions > 0 && data_00588224 == 0) {
        COptimizer_Optimize(functionObject);
    } else {
        fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "INITIAL CODE");
    }
    if (copts.instructionSchedulingMode == 2) {
        if (copts.peephole)
            Peephole_MergeAdjacentBlocks(functionObject, 0);
        if (copts.debug_listing)
            fn_004c4bb0(COptimizer_GetFunctionObject(functionObject)->name, "BEFORE SCHEDULING");
        Scheduler_Schedule(0);
        if (copts.debug_listing)
            fn_004c4ba0();
        if (copts.debug_listing)
            fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER INSTRUCTION SCHEDULING");
    }
    if (copts.peephole) {
        if (copts.instructionSchedulingMode == 0 && copts.deleteDeadInstructions > 1)
            Peephole_MergeAdjacentBlocks(functionObject, 0);
        Peephole_VisitBlocksWithMultipleInstructions(functionObject);
        if (copts.debug_listing)
            fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER PEEPHOLE FORWARD");
    }
    Coloring_AllocateRegisters(functionObject);
    if (copts.debug_listing)
        fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER REGISTER COLORING");
    for (temporaryEntry = temporary_objects; temporaryEntry != NULL; temporaryEntry = temporaryEntry->next)
        StackFrameEABI_AllocateObjectSlot(temporaryEntry->object);
    if (!hasSentinelArgument)
        StackFrameEABI_ClearUnusedStackFrame();
    StackFrameEABI_FinalizeLayout(frameBlock);
    StackFrameEABI_GeneratePrologueEpilogue(prologueBlock, hasSentinelArgument, has_dlocal_initialization);
    if (hasSentinelArgument) {
        StackFrameEABI_SaveArgumentRegisters(firstEntryBlock, secondEntryBlock, commonEntryBlock, entryLabel);
    }
    if (copts.profile)
        PCodeUtilities_EmitObjectInstructionWithPayload(data_00587c8c, 1, 0, 0, 0);
    StackFrameEABI_MergePrologueEpilogue(gReturnBlock, 1);
    if (copts.debug_listing)
        fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER GENERATING EPILOGUE, PROLOGUE");
    if (copts.peephole) {
        if (copts.instructionSchedulingMode != 0) {
            Peephole_MergeAdjacentBlocks(functionObject, 1);
            if (copts.debug_listing)
                fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER MERGING EPILOGUE, PROLOGUE");
        }
        Peephole_OptimizeBlocks(functionObject);
        if (copts.debug_listing)
            fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "AFTER PEEPHOLE OPTIMIZATION");
    }
    if (copts.instructionSchedulingMode != 0) {
        if (copts.debug_listing)
            fn_004c4bb0(COptimizer_GetFunctionObject(functionObject)->name, "BEFORE SCHEDULING");
        Scheduler_Schedule(1);
        if (copts.debug_listing)
            fn_004c4ba0();
        if (copts.debug_listing)
            fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "FINAL CODE AFTER INSTRUCTION SCHEDULING");
    } else {
        if (copts.debug_listing)
            fn_004c4bd0(COptimizer_GetFunctionObject(functionObject)->name, "FINAL CODE");
    }
    {
        int result = PCodeAssembly_EmitFunction(functionObject, NULL);
        InstrSelection_EmitSwitchTables(functionObject);
        TOC_EnumerateObjectCodeOffsets(functionObject);
        if (copts.exceptions != 0 && data_00588521 == 0)
            Exceptions_EmitExceptionTable(functionObject, result);
    }
}

void CodeGen_GenThunk(Object *stmt, Object *func, SInt32 a, SInt32 flag, SInt32 b)
{
    UInt8 save_fi;
    UInt8 save_d7;
    UInt8 save_27;

    save_fi = copts.filesyminfo;
    save_d7 = copts.peephole;
    save_27 = copts.emitExtraAssemblyData;

    CError_ASSERT(2026, !flag);

    PCode_ResetBlocks();
    PCode_CreateBlock();

    if (a != 0) {
        SInt32 kind = CodeGen_00435040_kind(TYPE_FUNC(func->type)->functype);

        if (b >= 0) {
            if (b != (SInt16)b) {
                PCodeUtilities_EmitInstruction(PC_ADDIS, 0xb, 0, 0, (SInt16)((b >> 16) + ((b >> 15) & 1)));
                if ((SInt16)b != 0)
                    PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, 0xb, 0, (SInt16)b);
            } else {
                PCodeUtilities_EmitInstruction(PC_ADDI, 0xb, 0, 0, b);
            }
            PCodeUtilities_EmitInstruction(PC_LWZX, 0xb, kind, 0xb);
            PCodeUtilities_EmitInstruction(PC_ADD, kind, kind, 0xb);
        }

        if (a != (SInt16)a) {
            PCodeUtilities_EmitInstruction(PC_ADDIS, kind, kind, 0, (SInt16)((a >> 16) + ((a >> 15) & 1)));
            if ((SInt16)a != 0)
                PCodeUtilities_EmitInstruction(PC_ADDI, kind, kind, 0, (SInt16)a);
        } else {
            PCodeUtilities_EmitInstruction(PC_ADDI, kind, kind, 0, a);
        }
    }

    PCodeUtilities_EmitInstruction(PC_B, 0, func);

    copts.filesyminfo = 0;
    copts.peephole = 0;
    copts.emitExtraAssemblyData = 0;
    PCodeAssembly_EmitFunction(stmt, NULL);

    copts.filesyminfo = save_fi;
    copts.peephole = save_d7;
    copts.emitExtraAssemblyData = save_27;
}

void CodeGen_InitializeLists(void)
{
    fn_0049f560();
    initialize_codegen_list(&data_00587640);
    initialize_codegen_list(&data_0058758c);
    initialize_codegen_list(&data_00588054);
    initialize_codegen_list(&data_00587c8c);
    initialize_codegen_list(&data_0058803c);
    initialize_codegen_list(&data_00588038);
    initialize_codegen_list(&data_0058808c);
    initialize_codegen_list(&data_00588078);
    initialize_codegen_list(&data_00587ff4);
    initialize_codegen_list(&data_00587ff0);
    initialize_codegen_list(&data_00587fec);
    initialize_codegen_list(&data_00587eb4);
    initialize_codegen_list(&data_00587ee4);
    initialize_codegen_list(&data_00587eac);
    initialize_codegen_list(&data_00587edc);
    initialize_codegen_list(&data_00587e68);
    initialize_codegen_list(&data_005875e0);
    initialize_codegen_list(&data_005875f4);
    initialize_codegen_list(&data_00587584);
    initialize_codegen_list(&data_0058762c);
    initialize_codegen_list(&data_0058761c);
    initialize_codegen_list(&data_0058760c);
    initialize_codegen_list(&data_00587604);
    initialize_codegen_list(&data_00587618);
    initialize_codegen_list(&data_00587610);
    initialize_codegen_list(&data_005875fc);
    initialize_codegen_list(&data_00587628);
    initialize_codegen_list(&data_00587e9c);
    initialize_codegen_list(&data_00587ea4);
    initialize_codegen_list(&data_00587e94);
    initialize_codegen_list(&data_0058823c);
    initialize_codegen_list(&data_00588068);
    initialize_codegen_list(&data_00587e90);
    initialize_codegen_list(&data_00587e5c);
    initialize_codegen_list(&data_00588250);
    initialize_codegen_list(&data_00587f50);
    initialize_codegen_list(&data_00587614);
    initialize_codegen_list(&data_005875bc);
    initialize_codegen_list(&data_005875ac);
    initialize_codegen_list(&data_005875f0);
    initialize_codegen_list(&data_005875e8);
    initialize_codegen_list(&data_005875d8);
    initialize_codegen_list(&data_005875d4);
    initialize_codegen_list(&data_005875e4);
    initialize_codegen_list(&data_005875dc);
    initialize_codegen_list(&data_005875c8);
    initialize_codegen_list(&data_005875ec);
    initialize_codegen_list(&data_00587e6c);
    initialize_codegen_list(&data_00587e7c);
    initialize_codegen_list(&data_00587e80);
    initialize_codegen_list(&data_00588214);
    initialize_codegen_list(&data_00588020);
    initialize_codegen_list(&data_00587ec0);
    initialize_codegen_list(&data_00587e34);
    initialize_codegen_list(&data_00588210);
    initialize_codegen_list(&data_00587f9c);
    Intrinsics_RegisterIntrinsics();
}

void fn_00434660(Boolean initializationOptions)
{
    data_00587640->name = InternCodeGenName("__ptr_glue");
    blank_name = InternCodeGenName("TOC");
    data_0058758c->name = InternCodeGenName("__cvt_fp2unsigned");
    data_00588054->name = InternCodeGenName("__PROFILE_ENTRY");
    data_00587c8c->name = InternCodeGenName("__PROFILE_EXIT");
    data_0058803c->name = InternCodeGenName("__div2i");
    data_00588038->name = InternCodeGenName("__div2u");
    data_0058808c->name = InternCodeGenName("__mod2i");
    data_00588078->name = InternCodeGenName("__mod2u");
    data_00587ff4->name = InternCodeGenName("__shr2i");
    data_00587ff0->name = InternCodeGenName("__shr2u");
    data_00587fec->name = InternCodeGenName("__shl2i");
    data_00587eb4->name = InternCodeGenName("__cvt_ull_dbl");
    data_00587ee4->name = InternCodeGenName("__cvt_sll_dbl");
    data_00587eac->name = InternCodeGenName("__cvt_ull_flt");
    data_00587edc->name = InternCodeGenName("__cvt_sll_flt");
    data_00587e68->name = InternCodeGenName("__cvt_dbl_usll");
    data_005875e0->name = InternCodeGenName("_d_neg");
    data_005875f4->name = InternCodeGenName("_d_add");
    data_00587584->name = InternCodeGenName("_d_sub");
    data_0058762c->name = InternCodeGenName("_d_mul");
    data_0058761c->name = InternCodeGenName("_d_div");
    data_0058760c->name = InternCodeGenName("_d_feq");
    data_00587604->name = InternCodeGenName("_d_fne");
    data_00587618->name = InternCodeGenName("_d_flt");
    data_00587610->name = InternCodeGenName("_d_fgt");
    data_005875fc->name = InternCodeGenName("_d_fle");
    data_00587628->name = InternCodeGenName("_d_fge");
    data_00587e9c->name = InternCodeGenName("_d_dtof");
    data_00587ea4->name = InternCodeGenName("_d_dtoi");
    data_00587e94->name = InternCodeGenName("_d_dtou");
    data_0058823c->name = InternCodeGenName("_d_dtoll");
    data_00588068->name = InternCodeGenName("_d_dtoull");
    data_00587e90->name = InternCodeGenName("_d_itod");
    data_00587e5c->name = InternCodeGenName("_d_utod");
    data_00588250->name = InternCodeGenName("_d_lltod");
    data_00587f50->name = InternCodeGenName("_d_ulltod");
    data_00587614->name = InternCodeGenName("_f_neg");
    data_005875bc->name = InternCodeGenName("_f_add");
    data_005875ac->name = InternCodeGenName("_f_sub");
    data_005875f0->name = InternCodeGenName("_f_mul");
    data_005875e8->name = InternCodeGenName("_f_div");
    data_005875d8->name = InternCodeGenName("_f_feq");
    data_005875d4->name = InternCodeGenName("_f_fne");
    data_005875e4->name = InternCodeGenName("_f_flt");
    data_005875dc->name = InternCodeGenName("_f_fgt");
    data_005875c8->name = InternCodeGenName("_f_fle");
    data_005875ec->name = InternCodeGenName("_f_fge");
    data_00587e6c->name = InternCodeGenName("_f_ftod");
    data_00587e7c->name = InternCodeGenName("_f_ftoi");
    data_00587e80->name = InternCodeGenName("_f_ftou");
    data_00588214->name = InternCodeGenName("_f_ftoll");
    data_00588020->name = InternCodeGenName("_f_ftoull");
    data_00587ec0->name = InternCodeGenName("_f_itof");
    data_00587e34->name = InternCodeGenName("_f_utof");
    data_00588210->name = InternCodeGenName("_f_lltof");
    data_00587f9c->name = InternCodeGenName("_f_ulltof");
    CMach_ReInitRuntimeObjects();
    Intrinsics_InitRegistrations(initializationOptions);
}

char CodeGen_IsRegisteredObject(ObjBase *object)
{
    return Intrinsics_IsRegisteredObject(object);
}

void CodeGen_EmitLoadAndBranchFunction(Object *function, Object *branchTarget, Object *table, SInt32 offset)
{
    UInt8 savedFileSymInfo = copts.filesyminfo;
    char savedFb7 = copts.peephole;
    char savedF07 = copts.emitExtraAssemblyData;
    Operand operand;
    Object *indirectSymbol;
    SInt32 reg;

    memclrw(&operand, sizeof(operand));
    CError_ASSERT(2283, offset <= 0x7fff);
    PCode_ResetBlocks();
    PCode_CreateBlock();
    table->section = ObjGen_PPC_EABI_GetHeaderIndex(2);
    indirectSymbol = fn_0049f230(table, 1, 1);
    if (indirectSymbol != NULL) {
        operand.kind = OpndType_IndirectSymbol;
        operand.object = indirectSymbol;
    } else {
        operand.kind = OpndType_Symbol;
        operand.object = table;
    }
    if (operand.kind != OpndType_GPR) {
        Operands_ForceGPR(&operand, (Type *)&void_ptr, 12);
    }
    if (operand.kind != OpndType_GPR) {
        CError_FATAL(2316);
    } else {
        reg = operand.reg;
        if (reg != 12)
            PCodeUtilities_EmitInstruction(PC_MR, 12, reg);
    }
    emit_opcode_with_base_offset(PC_LWZ, 12, 12, NULL, (SInt16)offset);
    PCodeUtilities_EmitInstruction(PC_B, 0, branchTarget);
    copts.filesyminfo = 0;
    copts.peephole = 0;
    copts.emitExtraAssemblyData = 0;
    PCodeAssembly_EmitFunction(function, NULL);
    copts.filesyminfo = savedFileSymInfo;
    copts.peephole = savedFb7;
    copts.emitExtraAssemblyData = savedF07;
}

void CodeGen_ParseDeclspecSection(HashNameNode *node, DeclInfo *value)
{
    if (memcmp(node->name, "section", 8) == 0) {
        if (cscope_currentfunc != NULL) {
            PPCError_ReportDiagnostic(0x9c, "__declspec");
            return;
        }
        tk = CPrepTokenizer_ScanToken();
        if (tk != TK_EOL) {
            if (tk == TK_STRING)
                BE_elf_SetDeclSection(string_token_data, value);
            else
                PPCError_ReportDiagnostic(0x9c, "__declspec");
            return;
        }
    }
    CError_ReportError(ERR_ILLEGAL_TYPE_QUALIFIERS);
}

#define ERRTAIL                                                                                                        \
    if ((short)CPrepTokenizer_ScanToken() != -7) {                                                                     \
        CPrep_ReportError(0x71);                                                                                       \
        do {                                                                                                           \
            u = (short)CPrepTokenizer_ScanToken();                                                                     \
            if (u == -7)                                                                                               \
                break;                                                                                                 \
        } while (u != 0);                                                                                              \
    }                                                                                                                  \
    return;

void parse_section_pragma(void)
{
    short t;
    short u;
    int tok;
    short ok;
    UInt8 mode;
    UInt8 type;
    char *name;
    char *p;
    SectionRec *rec;

    ok = FALSE;
    rec = (SectionRec *)galloc(sizeof(*rec));
    memset(rec, 0, sizeof(*rec));
    t = CPrep_ScanMacroExpandedChar();
    if (t == 1) {
        tok = CPrepTokenizer_ScanToken();
    }
    while (t != 0) {
        name = NULL;
        if (tok == -3) {
            name = data_00587fa0->name;
        } else if (tok == -4) {
            name = string_token_data;
        }
        if (name != NULL) {
            type = 0;
            mode = 0;
            if (memcmp(name, "code_type", sizeof("code_type")) == 0)
                type = 1;
            else if (memcmp(name, "data_type", sizeof("data_type")) == 0)
                type = 2;
            else if (memcmp(name, "const_type", sizeof("const_type")) == 0)
                type = 4;
            else if (memcmp(name, "sdata_type", sizeof("sdata_type")) == 0)
                type = 8;
            else if (memcmp(name, "sconst_type", sizeof("sconst_type")) == 0)
                type = 0x10;
            else if (memcmp(name, "all_types", sizeof("all_types")) == 0)
                type = 0x1f;
            switch (type) {
                case 1:
                case 2:
                case 4:
                case 8:
                case 0x10:
                case 0x1f:
                    if ((rec->flags & 2) != 0) {
                        PPCError_ReportError(0x9f);
                        ERRTAIL
                    }
                    if (rec->flags > 1) {
                        PPCError_ReportError(0x95, name);
                        ERRTAIL
                    }
                    if (tok == -4) {
                        PPCError_ReportError(0x8a, name);
                        ERRTAIL
                    }
                    rec->flags = 1;
                    ok = TRUE;
                    rec->typebits |= type;
                    break;
                default:
                    if (rec->flags == 0) {
                        for (p = name; *p != 0; p++) {
                            switch (*p) {
                                case 'R':
                                    rec->mode |= 2;
                                    mode = 2;
                                    break;
                                case 'W':
                                    rec->mode |= 3;
                                    mode = 2;
                                    break;
                                case 'X':
                                    rec->mode |= 6;
                                    mode = 2;
                                    break;
                                default:
                                    mode = 0;
                                    rec->mode = 0;
                                    break;
                            }
                            if (mode == 0)
                                break;
                        }
                    }
                    if (mode == 0) {
                        if (memcmp(name, "data_mode", sizeof("data_mode")) == 0)
                            mode = 0x10;
                        else if (memcmp(name, "code_mode", sizeof("code_mode")) == 0)
                            mode = 0x20;
                    }
                    if (mode == 0x10 || mode == 0x20) {
                        if ((rec->flags & mode) != 0) {
                            PPCError_ReportError(0x96, name);
                            ERRTAIL
                        }
                        if (rec->flags > mode) {
                            PPCError_ReportError(0x95, name);
                            ERRTAIL
                        }
                        if (tok == -4) {
                            PPCError_ReportError(0x8b, name);
                            ERRTAIL
                        }
                        rec->flags |= mode;
                        if (CPrep_ScanMacroExpandedChar() != 0 && CPrepTokenizer_ScanToken() == '=') {
                            if (CPrep_ScanMacroExpandedChar() != 0 && (tok = CPrepTokenizer_ScanToken()) == -3) {
                                if (mode == 0x10) {
                                    if (memcmp(data_00587fa0->name, "far_abs", sizeof("far_abs")) == 0)
                                        rec->far_reloc = 1;
                                    else if (memcmp(data_00587fa0->name, "near_abs", sizeof("near_abs")) == 0)
                                        rec->far_reloc = 2;
                                    else if (memcmp(data_00587fa0->name, "sda_rel", sizeof("sda_rel")) == 0)
                                        rec->far_reloc = 5;
                                    else if (memcmp(data_00587fa0->name, "pc_rel", sizeof("pc_rel")) == 0) {
                                        PPCError_ReportError(0x98, data_00587fa0->name, "code_mode");
                                        ERRTAIL
                                    } else {
                                        PPCError_ReportError(0x87, name);
                                        ERRTAIL
                                    }
                                } else {
                                    if (memcmp(data_00587fa0->name, "near_abs", sizeof("near_abs")) == 0)
                                        rec->near_reloc = 0xb;
                                    else if (memcmp(data_00587fa0->name, "pc_rel", sizeof("pc_rel")) == 0)
                                        rec->near_reloc = 0xa;
                                    else if (memcmp(data_00587fa0->name, "far_abs", sizeof("far_abs")) == 0)
                                        rec->near_reloc = 0xc;
                                    else if (memcmp(data_00587fa0->name, "sda_rel", sizeof("sda_rel")) == 0) {
                                        PPCError_ReportError(0x8e);
                                        ERRTAIL
                                    } else {
                                        PPCError_ReportError(0x87, "code");
                                        ERRTAIL
                                    }
                                }
                            } else {
                                PPCError_ReportError(0x88, name);
                                ERRTAIL
                            }
                        } else {
                            PPCError_ReportError(0x89, "'='");
                            ERRTAIL
                        }
                    } else if (mode == 2) {
                        if ((rec->flags & 1) != 0) {
                            PPCError_ReportError(0x9f);
                            ERRTAIL
                        }
                        if (rec->flags > mode) {
                            PPCError_ReportError(0x95, name);
                            ERRTAIL
                        }
                        if (tok == -4) {
                            PPCError_ReportError(0x9e, name);
                            ERRTAIL
                        }
                        rec->flags |= 2;
                    } else {
                        if (rec->flags < 4) {
                            if (tok != -4) {
                                if (rec->flags == 0) {
                                    PPCError_ReportError(0x99);
                                } else {
                                    PPCError_ReportError(0x94);
                                }
                                ERRTAIL
                            }
                            rec->sectionName = name;
                            rec->flags |= 4;
                        } else if (rec->flags < 8) {
                            if (tok != -4) {
                                PPCError_ReportError(0x9b);
                                ERRTAIL
                            }
                            rec->linkedSectionName = name;
                            rec->flags |= 8;
                        } else {
                            if (tok == -4) {
                                if ((rec->flags & 8) != 0) {
                                    PPCError_ReportError(0x97, name);
                                    ERRTAIL
                                }
                                PPCError_ReportError(0x9a, name);
                                ERRTAIL
                            }
                            PPCError_ReportError(0x97, name);
                            ERRTAIL
                        }
                    }
                    break;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
            ERRTAIL
        }
        t = CPrep_ScanMacroExpandedChar();
        if (t == 0)
            break;
        tok = CPrepTokenizer_ScanToken();
    }
    if (rec->sectionName == NULL && ok == 0) {
        PPCError_ReportError(0x99);
        ERRTAIL
    }
    if (rec->sectionName != NULL && ObjGen_PPC_EABI_IsSpecialSectionName(rec->sectionName) != 0) {
        PPCError_ReportError(0x8c, rec->sectionName);
        return;
    }
    if (rec->linkedSectionName != NULL && ObjGen_PPC_EABI_IsSpecialSectionName(rec->linkedSectionName) != 0) {
        PPCError_ReportError(0x8c, rec->linkedSectionName);
        return;
    }
    if (rec->sectionName != NULL && ObjGen_PPC_EABI_IsInvalidAbsName(rec->sectionName) != 0) {
        PPCError_ReportError(0xa2, rec->sectionName);
        return;
    }
    if (rec->linkedSectionName != NULL && ObjGen_PPC_EABI_IsInvalidAbsName(rec->linkedSectionName) != 0) {
        PPCError_ReportError(0xa2, rec->linkedSectionName);
        return;
    }
    ObjGen_PPC_EABI_SetSectionOptions(rec);
    return;
}

void fn_004332e0(void)
{
    SInt32 processor;

    tk = CPrepTokenizer_ScanToken();
    if (tk == TK_IDENTIFIER) {
        if (memcmp(data_00587fa0->name, "altivec", 8) == 0) {
            CodeGen_SetProcessorOption(7);
            return;
        }
        if (memcmp(data_00587fa0->name, "reset", 6) == 0) {
            CPrep_RestoreOption(4);
            CPrep_RestoreOption(5);
            return;
        }
        if (memcmp(data_00587fa0->name, "off", 4) == 0) {
            CPrep_SaveAndSetOption(5, 0);
            return;
        }
        if (memcmp(data_00587fa0->name, "once", 5) == 0) {
            CPrep_SaveAndSetOption(5, 1);
            return;
        }
        if (memcmp(data_00587fa0->name, "twice", 6) == 0) {
            CPrep_SaveAndSetOption(5, 2);
            return;
        }
        if (memcmp(data_00587fa0->name, "on", 3) == 0) {
            CPrep_SaveAndSetOption(5, 2);
            return;
        }
        if (!copts.altivec_model) {
            if (memcmp(data_00587fa0->name, "603e", 5) == 0) {
                CodeGen_SetProcessorOption(5);
                return;
            }
            if (memcmp(data_00587fa0->name, "604e", 5) == 0) {
                CodeGen_SetProcessorOption(6);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC603e", 8) == 0) {
                CodeGen_SetProcessorOption(5);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC604e", 8) == 0) {
                CodeGen_SetProcessorOption(6);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC403GA", 9) == 0) {
                CodeGen_SetProcessorOption(8);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC403GB", 9) == 0) {
                CodeGen_SetProcessorOption(8);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC403GC", 9) == 0) {
                CodeGen_SetProcessorOption(8);
                return;
            }
            if (memcmp(data_00587fa0->name, "PPC403GCX", 10) == 0) {
                CodeGen_SetProcessorOption(8);
                return;
            }
        } else {
            PPCError_ReportError(0x73);
            return;
        }
        CPrep_ReportError(0xba);
        return;
    } else if (tk == TK_INTCONST) {
        switch (intconst_lo) {
            case 601:
                processor = 1;
                break;
            case 603:
                processor = 2;
                break;
            case 604:
                processor = 3;
                break;
            case 750:
                processor = 4;
                break;
            case 7400:
                processor = 7;
                break;
            case 8240:
            case 8260:
                processor = 5;
                break;
            case 401:
            case 403:
            case 505:
            case 509:
            case 555:
            case 602:
                processor = 8;
                break;
            case 740:
                processor = 4;
                break;
            case 801:
            case 821:
            case 823:
            case 850:
            case 860:
                processor = 8;
                break;
            default:
                CPrep_ReportError(0xba);
                return;
        }

        if (!copts.altivec_model) {
            CodeGen_SetProcessorOption(processor);
            return;
        } else {
            PPCError_ReportError(0x73);
            return;
        }
    } else {
        if (copts.warn_illpragma)
            fn_0043f3b0(0xba);
    }
}

void CodeGen_ParsePragma(HashNameNode *name)
{
    SInt16 token;
    SInt32 value;
    SInt32 n;
    Boolean any;
    InterruptGenerationRecord info;
    InterruptList *list;
    InterruptList *savedList;

    if (!strcmp(name->name, "scheduling")) {
        fn_004332e0();
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "ppc_unroll_speculative")) {
        if (CPrepTokenizer_ScanToken() == -3) {
            if (!strcmp(data_00587fa0->name, "off")) {
                copts.unroll_speculative = 0;
            } else if (!strcmp(data_00587fa0->name, "on")) {
                copts.unroll_speculative = 1;
            } else {
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "ppc_unroll_instructions_limit")) {
        token = CPrepTokenizer_ScanToken();
        if (token == -1) {
            if ((copts.unroll_instr_limit = intconst_lo) < 0) {
                copts.unroll_instr_limit = 0;
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
            }
        } else if (token == -3) {
            if (!strcmp(data_00587fa0->name, "off")) {
                copts.unroll_instr_limit = 0;
            } else if (!strcmp(data_00587fa0->name, "on")) {
                copts.unroll_instr_limit = 60;
            } else {
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "ppc_unroll_factor_limit")) {
        token = CPrepTokenizer_ScanToken();
        if (token == -1) {
            if ((copts.unroll_factor_limit = intconst_lo) < 0) {
                copts.unroll_factor_limit = 0;
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
            }
        } else if (token == -3) {
            if (!strcmp(data_00587fa0->name, "off")) {
                copts.unroll_factor_limit = 0;
            } else if (!strcmp(data_00587fa0->name, "on")) {
                copts.unroll_factor_limit = 10;
            } else {
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "altivec_model")) {
        if (CPrepTokenizer_ScanToken() == -3) {
            if (!strcmp(data_00587fa0->name, "off")) {
                copts.altivec_model = 0;
            } else if (!strcmp(data_00587fa0->name, "on")) {
                copts.altivec_model = 1;
            } else {
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "altivec_vrsave")) {
        if (CPrepTokenizer_ScanToken() == -3) {
            if (!strcmp(data_00587fa0->name, "off")) {
                CPrep_SaveAndSetOption(0x11, 0);
            } else if (!strcmp(data_00587fa0->name, "on")) {
                CPrep_SaveAndSetOption(0x11, 1);
            } else if (!strcmp(data_00587fa0->name, "allon")) {
                CPrep_SaveAndSetOption(0x11, 2);
            } else if (!strcmp(data_00587fa0->name, "reset")) {
                CPrep_RestoreOption(0x11);
            } else {
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_PRAGMA);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "function_align")) {
        token = CPrepTokenizer_ScanToken();
        if (token == -1) {
            value = n = intconst_lo;
            switch (n) {
                case 4:
                case 8:
                case 16:
                case 32:
                case 64:
                case 128:
                    CPrep_SaveAndSetOption(0x12, value);
                    break;
                default:
                    PPCError_ReportDiagnostic(0xa1);
                    CheckPragmaEnd();
                    return;
            }
        } else if (token == -3 && !strcmp(data_00587fa0->name, "reset")) {
            CPrep_RestoreOption(0x12);
        } else {
            PPCError_ReportDiagnostic(0xa1);
        }
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "section")) {
        if (cscope_currentfunc) {
            PPCError_ReportDiagnostic(0x9c, "pragma section");
            CheckPragmaEnd();
            return;
        }
        parse_section_pragma();
        return;
    }

    if (!strcmp(name->name, "rel109_offset")) {
        if (CPrepTokenizer_ScanToken() == -1) {
            SInt32 offset;
            SInt32 signedOffset;
            offset = signedOffset = intconst_lo;
            if (signedOffset < 0 || offset > 3) {
                PPCError_ReportDiagnostic(0xa0);
                CheckPragmaEnd();
                return;
            }
            copts.rel109_offset = offset;
            CheckPragmaEnd();
            return;
        }
        PPCError_ReportDiagnostic(0xa0);
        CheckPragmaEnd();
        return;
    }

    if (!strcmp(name->name, "interrupt")) {
        any = 0;
        memset(&info, 0, sizeof(info));
        for (;;) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER)
                break;
            if (!strcmp("enable", data_00587fa0->name)) {
                info.enable = 1;
                any = 1;
            } else if (!strcmp("SRR", data_00587fa0->name)) {
                info.SRR = 1;
                any = 1;
            } else if (!strcmp("DAR", data_00587fa0->name)) {
                info.DAR = 1;
                any = 1;
            } else if (!strcmp("DSISR", data_00587fa0->name)) {
                info.DSISR = 1;
                any = 1;
            } else {
                if (!strcmp("on", data_00587fa0->name)) {
                    InterruptGenerationRecord *current;
                    copts.interruptOptions = galloc(sizeof(InterruptGenerationRecord));
                    memclrw(copts.interruptOptions, sizeof(InterruptGenerationRecord));
                    *(current = copts.interruptOptions) = info;
                    list = galloc(sizeof(InterruptList));
                    list->next = copts.interruptList;
                    copts.interruptList = list;
                    list->info = current;
                    return;
                }
                if (!strcmp("off", data_00587fa0->name)) {
                    if (any) {
                        CError_ReportError(ERR_ILLEGAL_PRAGMA);
                        CheckPragmaEnd();
                        return;
                    }
                    copts.interruptList = NULL;
                    copts.interruptOptions = NULL;
                    return;
                }
                if (!strcmp("reset", data_00587fa0->name)) {
                    if (any) {
                        CError_ReportError(ERR_ILLEGAL_PRAGMA);
                        CheckPragmaEnd();
                        return;
                    }
                    list = savedList = copts.interruptList;
                    if (savedList->next) {
                        list = list->next;
                        copts.interruptOptions = list->info;
                    } else {
                        copts.interruptList = NULL;
                        copts.interruptOptions = NULL;
                    }
                    return;
                }
                CError_ReportError(ERR_ILLEGAL_PRAGMA);
                CheckPragmaEnd();
                return;
            }
        }
        CError_ReportError(ERR_ILLEGAL_PRAGMA);
        CheckPragmaEnd();
        return;
    }

    if (copts.warn_illpragma)
        fn_0043f3b0(0xba);
    SkipPragma();
}

void CodeGen_SetObjectSectionAndInterruptInfo(Object *obj)
{
    SInt32 isConst;
    SInt32 checkConst;
    InterruptGenerationRecord *record;
    BE_SymNode *infoObject;
    UInt32 qual;

    if ((SInt16)obj->section <= 0) {
        if (obj->datatype == DDATA) {
            if (obj == data_00587678) {
                ObjGen_PPC_EABI_SetObjectSection(data_00587678, 0xc, 0);
            }
            if (obj->type == NULL) {
                return;
            }
            if (!CParser_HasInternalLinkage(obj)) {
                isConst = 0;
                checkConst = 0;
                if (copts.cplusplus == 0) {
                    checkConst = 1;
                }
                if (checkConst) {
                    if (obj->type->type == TYPEPOINTER) {
                        qual = TYPE_POINTER(obj->type)->qual;
                    } else {
                        qual = obj->qual;
                    }
                    qual = Q_CONST & qual;
                    if (qual) {
                        isConst = 1;
                    }
                }
                ObjGen_PPC_EABI_SetObjectSection(obj, obj->type->size, isConst);
            }
        } else {
            ObjGen_PPC_EABI_SetObjectSection(obj, 0, 0);
        }
    }

    if (copts.interruptOptions != NULL && obj->datatype != DDATA) {
        InterruptGenerationRecord *options;
        if (obj->name == NULL) {
            return;
        }
        obj->qual |= Q_INTERRUPT;
        infoObject = BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
        if ((record = infoObject->interruptInfo) == NULL) {
            record = (InterruptGenerationRecord *)galloc(sizeof(*record));
            memclrw(record, sizeof(*record));
            infoObject->interruptInfo = record;
            record->next = interrupt_generation_records;
            interrupt_generation_records = record;
            record->id = --data_005876b4;
        }
        options = copts.interruptOptions;
        if (options->enable) {
            record->enable = 1;
        } else if (options->SRR) {
            record->SRR = 1;
        } else if (options->DAR) {
            record->DAR = 1;
        } else if (options->DSISR) {
            record->DSISR = 1;
        }
    }
}

void CodeGen_SetIROptimizationEnabled(void)

{
    copts.globaloptimizer = '\0' < copts.deleteDeadInstructions;
    return;
}

unsigned int CodeGen_GetMethRecRTypeSize(MethRec *record)
{
    unsigned int size;

    if (!record->rtype) {
        size = 4;
    } else {
        Type *type = record->rtype;
        if (type->type == TYPEARRAY || (unsigned char)(type->type - TYPESTRUCT) <= TYPECLASS - TYPESTRUCT ||
            (type->type == TYPEMEMBERPOINTER && type->size == 12))
            size = 8;
        else
            size = type->size;
    }
    if (!size)
        size = 1;
    return (size + 3) & ~3U;
}

int fn_00432480(MethRec *record)
{
    int size;
    if (!record->rtype)
        size = 4;
    else if (record->rtype->type == TYPEARRAY || record->rtype->type == TYPESTRUCT ||
             record->rtype->type == TYPECLASS ||
             (record->rtype->type == TYPEMEMBERPOINTER && record->rtype->size == 12))
        size = 8;
    else
        size = record->rtype->size;
    if (size == 0)
        size = 1;
    return (((size + 3) & ~3) + 7) & ~3;
}

unsigned int CodeGen_GetObjCParameterOffset(MethRec *function, ObjCParameterNode *argument)
{
    unsigned int offset;
    ObjCParameterNode *arg;

    if (function->rtype == NULL)
        offset = 4;
    else if (function->rtype->type == TYPEARRAY || (unsigned char)(function->rtype->type - TYPESTRUCT) <= 1 ||
             (function->rtype->type == TYPEMEMBERPOINTER && function->rtype->size == 12))
        offset = 8;
    else
        offset = function->rtype->size;
    if (offset == 0)
        offset = 1;
    offset = (offset + 3) & ~3U;
    offset = (offset + 7) & ~3U;
    offset += 4;
    arg = function->args;
    while (arg != NULL) {
        if (arg == argument)
            return offset;
        if (arg->type == NULL)
            offset += 4;
        else
            offset += arg->type->size;
        offset = (offset + 3) & ~3U;
        arg = arg->next;
    }
    return 0;
}

SInt32 CodeGen_GetMethRecRtypeAndArgsSize(MethRec *p)
{
    ObjCParameterNode *n;
    SInt32 size;

    if (p->rtype == NULL)
        size = 4;
    else if (p->rtype->type == TYPEARRAY || p->rtype->type == TYPESTRUCT || p->rtype->type == TYPECLASS ||
             (p->rtype->type == TYPEMEMBERPOINTER && p->rtype->size == 12))
        size = 8;
    else
        size = p->rtype->size;
    if (size == 0)
        size = 1;
    size = (size + 3) & ~3;

    n = p->args;
    while (n != NULL) {
        if (n->next == NULL && n->type == NULL)
            return size;
        size = (size + 3) & ~3;
        if (n->type == NULL)
            size += 4;
        else
            size += n->type->size;
        n = n->next;
    }
    return size;
}

InterruptGenerationRecord *CodeGen_FindInterruptGenerationRecord(SInt16 key)
{
    InterruptGenerationRecord *entry = interrupt_generation_records;
    while (entry) {
        if (entry->id == (unsigned short)key)
            break;
        entry = entry->next;
    }
    if (!entry)
        CError_FATAL(3429);
    return entry;
}

ENode *CodeGen_MakeAltivecCall(Object *object, ENodeList *arguments)
{
    return Intrinsics_MakeAltivecCall(object, arguments);
}

ENode *CodeGen_MakeAltivecStructCast(ENode *a, Type *type, UInt32 qual)
{
    short q;

    if (copts.altivec_model) {
        q = qual & Q_CV;
        if (type->type == TYPESTRUCT && a->rtype->type == TYPESTRUCT && a->flags == q) {
            switch (TYPE_STRUCT(type)->stype) {
                case STRUCT_VECTOR_UCHAR:
                case STRUCT_VECTOR_SCHAR:
                case STRUCT_VECTOR_BCHAR:
                case STRUCT_VECTOR_USHORT:
                case STRUCT_VECTOR_SSHORT:
                case STRUCT_VECTOR_BSHORT:
                case STRUCT_VECTOR_UINT:
                case STRUCT_VECTOR_SINT:
                case STRUCT_VECTOR_BINT:
                case STRUCT_VECTOR_FLOAT:
                case STRUCT_VECTOR_PIXEL:
                    a = makemonadicnode(a, 0x30);
                    a->rtype = type;
                    a->flags = q;
                    return a;
            }
        }
    }
    return NULL;
}

int CodeGen_CheckAltivecStypeMatch(ENode *expr, Type *type, Boolean convert, Boolean checkAccess)
{
    TypeStruct *targetType = TYPE_STRUCT(type);
    TypeStruct *exprType = TYPE_STRUCT(expr->rtype);
    SInt32 result;

    if (copts.altivec_model != 0 && targetType->type == TYPESTRUCT) {
        SInt32 targetStype = targetType->stype;
        if (targetStype >= 4 && targetStype <= 0xe && exprType->type == TYPESTRUCT) {
            SInt32 exprStype = exprType->stype;
            if (exprStype >= 4 && exprStype <= 0xe && targetType->stype == exprType->stype)
                result = 3;
            else
                result = 0;
        } else {
            result = 0;
        }
    } else {
        result = 0;
    }
    return result;
}

Boolean IrOptimizer_ConvertToVectorConstant(ENode *expr, union MWVector128 *dst, TypeStruct *vectorType)
{
    int commaCount;
    int elementIndex;
    int splatIndex;
    Boolean result;
    double value;
    double firstValue;
    double splatValue;
    CInt64 integerValue;
    ENode *cursor;
    ENode *node;

    result = 0;
    dst->ul[0] = 0;
    dst->ul[1] = 0;
    dst->ul[2] = 0;
    dst->ul[3] = 0;
    value = 0.0;
    firstValue = 0.0;

    if (expr->type == ECOMMA) {
        commaCount = 0;
        for (node = expr; node->type == ECOMMA; node = node->data.diadic.left) {
            commaCount++;
        }
        switch (vectorType->stype) {
            case STRUCT_VECTOR_UCHAR:
            case STRUCT_VECTOR_SCHAR:
            case STRUCT_VECTOR_BCHAR:
                if (commaCount < 15) {
                    PPCError_ReportError(0x6e, vectorType, 0);
                    break;
                }
                if (commaCount > 15) {
                    PPCError_ReportError(0x6f, vectorType, 0);
                    break;
                }
                for (cursor = expr, elementIndex = 15; cursor->type == ECOMMA; cursor = cursor->data.diadic.left) {
                    node = cursor->data.diadic.right;
                    integerValue = node->data.intval;
                    if (node->type != EINTCONST) {
                        PPCError_ReportError(0x70);
                        break;
                    }
                    IrOptimizer_CheckVectorByteConstant(&integerValue, vectorType);
                    dst->uc[elementIndex] = integerValue.lo;
                    elementIndex--;
                }
                if (cursor->type == EINTCONST) {
                    integerValue = cursor->data.intval;
                    IrOptimizer_CheckVectorByteConstant(&integerValue, vectorType);
                    dst->uc[0] = integerValue.lo;
                } else {
                    PPCError_ReportError(0x70);
                    break;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_USHORT:
            case STRUCT_VECTOR_SSHORT:
            case STRUCT_VECTOR_BSHORT:
            case STRUCT_VECTOR_PIXEL:
                if (commaCount < 7) {
                    PPCError_ReportError(0x6e, vectorType, 0);
                    break;
                }
                if (commaCount > 7) {
                    PPCError_ReportError(0x6f, vectorType, 0);
                    break;
                }
                for (cursor = expr, elementIndex = 7; cursor->type == ECOMMA;
                     cursor = cursor->data.diadic.left, elementIndex--) {
                    node = cursor->data.diadic.right;
                    integerValue = node->data.intval;
                    if (node->type != EINTCONST) {
                        PPCError_ReportError(0x70);
                        break;
                    }
                    IrOptimizer_CheckVectorShortConstant(&integerValue, vectorType);
                    dst->us[elementIndex] = node->data.intval.lo;
                }
                if (cursor->type == EINTCONST) {
                    integerValue = cursor->data.intval;
                    IrOptimizer_CheckVectorShortConstant(&integerValue, vectorType);
                    dst->us[0] = integerValue.lo;
                } else {
                    PPCError_ReportError(0x70);
                    break;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_UINT:
            case STRUCT_VECTOR_SINT:
            case STRUCT_VECTOR_BINT:
                if (commaCount < 3) {
                    PPCError_ReportError(0x6e, vectorType, 0);
                    break;
                }
                if (commaCount > 3) {
                    PPCError_ReportError(0x6f, vectorType, 0);
                    break;
                }
                for (node = expr, elementIndex = 3; node->type == ECOMMA; node = node->data.diadic.left) {
                    cursor = node->data.diadic.right;
                    integerValue = cursor->data.intval;
                    if (cursor->type != EINTCONST) {
                        PPCError_ReportError(0x70);
                        break;
                    }
                    IrOptimizer_CheckVectorLongConstant(&integerValue, vectorType);
                    dst->ul[elementIndex] = cursor->data.intval.lo;
                    elementIndex--;
                }
                if (node->type == EINTCONST) {
                    integerValue = node->data.intval;
                    IrOptimizer_CheckVectorLongConstant(&integerValue, vectorType);
                    dst->ul[0] = integerValue.lo;
                } else {
                    PPCError_ReportError(0x70);
                    break;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_FLOAT:
                if (commaCount < 3) {
                    PPCError_ReportError(0x6e, vectorType, 0);
                    break;
                }
                if (commaCount > 3) {
                    PPCError_ReportError(0x6f, vectorType, 0);
                    break;
                }
                for (cursor = expr, elementIndex = 3; cursor->type == ECOMMA; cursor = cursor->data.diadic.left) {
                    node = cursor->data.diadic.right;
                    if (node->type == EFLOATCONST) {
                        value = node->data.floatval.data.value;
                    } else if (node->type == EINTCONST) {
                        value = CInt64_ConvertToLongDouble(&node->data.intval);
                    } else {
                        PPCError_ReportError(0x70);
                        break;
                    }
                    {
                        static const double float_bounds[6] = {0.0, 3.40282e+38,  1.17549e-38,
                                                               0.0, -3.40282e+38, -1.17549e-38};
                        if (value > float_bounds[0]) {
                            if (value > float_bounds[1]) {
                                PPCError_ReportError(0x70);
                                break;
                            } else if (value < float_bounds[2]) {
                                PPCError_ReportError(0x70);
                                break;
                            }
                        } else if (value < float_bounds[3]) {
                            if (value < float_bounds[4]) {
                                PPCError_ReportError(0x70);
                                break;
                            } else if (value > float_bounds[5]) {
                                PPCError_ReportError(0x70);
                                break;
                            }
                        }
                    }
                    dst->f[elementIndex] = value;
                    elementIndex--;
                }
                if (cursor->type == EFLOATCONST) {
                    firstValue = cursor->data.floatval.data.value;
                } else if (cursor->type == EINTCONST) {
                    firstValue = CInt64_ConvertToLongDouble(&cursor->data.intval);
                } else {
                    PPCError_ReportError(0x70);
                    break;
                }
                {
                    static const double float_bounds[6] = {0.0, 3.40282e+38,  1.17549e-38,
                                                           0.0, -3.40282e+38, -1.17549e-38};
                    if (firstValue > float_bounds[0]) {
                        if (firstValue > float_bounds[1]) {
                            PPCError_ReportError(0x70);
                            break;
                        } else if (firstValue < float_bounds[2]) {
                            PPCError_ReportError(0x70);
                            break;
                        }
                    } else if (firstValue < float_bounds[3]) {
                        if (firstValue < float_bounds[4]) {
                            PPCError_ReportError(0x70);
                            break;
                        } else if (firstValue > float_bounds[5]) {
                            PPCError_ReportError(0x70);
                            break;
                        }
                    }
                }
                dst->f[0] = firstValue;
                result = 1;
                break;
        }
    } else if (expr->type == EINTCONST) {
        splatIndex = 0;
        switch (vectorType->stype) {
            case STRUCT_VECTOR_UCHAR:
            case STRUCT_VECTOR_SCHAR:
            case STRUCT_VECTOR_BCHAR:
                integerValue = expr->data.intval;
                IrOptimizer_CheckVectorByteConstant(&integerValue, vectorType);
                for (; splatIndex < 16; splatIndex++) {
                    dst->uc[splatIndex] = integerValue.lo;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_USHORT:
            case STRUCT_VECTOR_SSHORT:
            case STRUCT_VECTOR_BSHORT:
            case STRUCT_VECTOR_PIXEL:
                integerValue = expr->data.intval;
                IrOptimizer_CheckVectorShortConstant(&integerValue, vectorType);
                for (; splatIndex < 8; splatIndex++) {
                    dst->us[splatIndex] = integerValue.lo;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_UINT:
            case STRUCT_VECTOR_SINT:
            case STRUCT_VECTOR_BINT:
                integerValue = expr->data.intval;
                IrOptimizer_CheckVectorLongConstant(&integerValue, vectorType);
                for (; splatIndex < 4; splatIndex++) {
                    dst->ul[splatIndex] = integerValue.lo;
                }
                result = 1;
                break;
            case STRUCT_VECTOR_FLOAT:
                integerValue = expr->data.intval;
                if (!CInt64_IsInRange(integerValue, 4)) {
                    PPCError_ReportError(0x70);
                    break;
                }
                CInt64_ConvertInt32(&integerValue);
                for (; splatIndex < 4; splatIndex++) {
                    dst->f[splatIndex] = (SInt32)integerValue.lo;
                }
                result = 1;
                break;
            default:
                PPCError_ReportError(0x70);
                break;
        }
    } else if (expr->type == EFLOATCONST) {
        switch (vectorType->stype) {
            case STRUCT_VECTOR_UCHAR:
            case STRUCT_VECTOR_SCHAR:
            case STRUCT_VECTOR_BCHAR:
            case STRUCT_VECTOR_USHORT:
            case STRUCT_VECTOR_SSHORT:
            case STRUCT_VECTOR_BSHORT:
            case STRUCT_VECTOR_UINT:
            case STRUCT_VECTOR_SINT:
            case STRUCT_VECTOR_BINT:
            case STRUCT_VECTOR_PIXEL:
            default:
                PPCError_ReportError(0x70);
                break;
            case STRUCT_VECTOR_FLOAT:
                splatIndex = 0;
                splatValue = expr->data.floatval.data.value;
                {
                    static const double float_bounds[6] = {0.0, 3.40282e+38,  1.17549e-38,
                                                           0.0, -3.40282e+38, -1.17549e-38};
                    if (splatValue > float_bounds[0]) {
                        if (splatValue > float_bounds[1]) {
                            PPCError_ReportError(0x70);
                            break;
                        } else if (splatValue < float_bounds[2]) {
                            PPCError_ReportError(0x70);
                            break;
                        }
                    } else if (splatValue < float_bounds[3]) {
                        if (splatValue < float_bounds[4]) {
                            PPCError_ReportError(0x70);
                            break;
                        } else if (splatValue > float_bounds[5]) {
                            PPCError_ReportError(0x70);
                            break;
                        }
                    }
                }
                for (; splatIndex < 4; splatIndex++) {
                    dst->f[splatIndex] = splatValue;
                }
                result = 1;
                break;
        }
    } else if (expr->type == EINDIRECT || expr->type == EFUNCCALL) {
        if (expr->rtype->type != TYPESTRUCT) {
            PPCError_ReportError(0x70);
        }
    } else if (expr->type != EVECTOR128CONST) {
        PPCError_ReportError(0x70);
    }
    return result;
}
