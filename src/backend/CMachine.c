#define CERROR_FILE "CMachine.c"
#include "compiler/common.h"
#include "compiler/CMachine.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CClass.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include <string.h>
#include <stdio.h>

/* Declarations gathered from the merged files. */

void initialize_hash_name_globals(void)
{
    HashNameNode *nameHash1;
    HashNameNode *nameHash2;
    HashNameNode *nameHash3;
    HashNameNode *nameHash4;
    HashNameNode *nameHash5;
    HashNameNode *nameHash6;
    HashNameNode *nameHash7;
    HashNameNode *nameHash8;
    HashNameNode *nameHash9;
    HashNameNode *nameHash10;
    HashNameNode *nameHash11;
    HashNameNode *nameHash12;
    HashNameNode *nameHash13;
    HashNameNode *nameHash14;
    HashNameNode *nameHash15;
    HashNameNode *nameHash16;
    HashNameNode *nameHash17;
    HashNameNode *nameHash18;
    HashNameNode *nameHash19;
    HashNameNode *nameHash20;
    HashNameNode *nameHash21;
    HashNameNode *nameHash22;
    HashNameNode *nameHash23;
    HashNameNode *nameHash24;
    HashNameNode *nameHash25;
    HashNameNode *nameHash26;
    HashNameNode *nameHash27;
    nameHash1 = GetHashNameNode("[0]");
    nameHash2 = GetHashNameNode("[1]");
    nameHash3 = GetHashNameNode("[2]");
    nameHash4 = GetHashNameNode("[3]");
    nameHash5 = GetHashNameNode("[4]");
    nameHash6 = GetHashNameNode("[5]");
    nameHash7 = GetHashNameNode("[6]");
    nameHash8 = GetHashNameNode("[7]");
    nameHash9 = GetHashNameNode("[8]");
    nameHash10 = GetHashNameNode("[9]");
    nameHash11 = GetHashNameNode("[10]");
    nameHash12 = GetHashNameNode("[11]");
    nameHash13 = GetHashNameNode("[12]");
    nameHash14 = GetHashNameNode("[13]");
    nameHash15 = GetHashNameNode("[14]");
    nameHash16 = GetHashNameNode("[15]");
    nameHash17 = GetHashNameNode("vector unsigned char");
    nameHash18 = GetHashNameNode("vector unsigned short");
    nameHash19 = GetHashNameNode("vector unsigned int");
    nameHash20 = GetHashNameNode("vector signed char");
    nameHash21 = GetHashNameNode("vector signed short");
    nameHash22 = GetHashNameNode("vector signed int");
    nameHash23 = GetHashNameNode("vector bool char");
    nameHash24 = GetHashNameNode("vector bool short");
    nameHash25 = GetHashNameNode("vector bool int");
    nameHash26 = GetHashNameNode("vector float");
    nameHash27 = GetHashNameNode("vector pixel");
    data_0055fae6 = nameHash17;
    data_0055fb22 = nameHash18;
    data_0055fb5e = nameHash19;
    data_0055fafa = nameHash20;
    data_0055fb36 = nameHash21;
    data_0055fb72 = nameHash22;
    data_0055fb0e = nameHash23;
    data_0055fb4a = nameHash24;
    data_0055fb86 = nameHash25;
    data_0055fb9a = nameHash26;
    data_0055fbae = nameHash27;
    data_0055f764 = nameHash1;
    data_0055f750 = nameHash2;
    data_0055f73c = nameHash3;
    data_0055f728 = nameHash4;
    data_0055f714 = nameHash5;
    name_hash6 = nameHash6;
    data_0055f6ec = nameHash7;
    nameHash8Ptr = nameHash8;
    data_0055f6c4 = nameHash9;
    data_0055f6b0 = nameHash10;
    data_0055f69c = nameHash11;
    data_0055f688 = nameHash12;
    data_0055f674 = nameHash13;
    data_0055f660 = nameHash14;
    data_0055f64c = nameHash15;
    data_0055f638 = nameHash16;
    data_0055f8a4 = nameHash1;
    data_0055f890 = nameHash2;
    data_0055f87c = nameHash3;
    data_0055f868 = nameHash4;
    data_0055f854 = nameHash5;
    data_0055f840 = nameHash6;
    data_0055f82c = nameHash7;
    name_hash8 = nameHash8;
    data_0055f804 = nameHash9;
    data_0055f7f0 = nameHash10;
    data_0055f7dc = nameHash11;
    data_0055f7c8 = nameHash12;
    data_0055f7b4 = nameHash13;
    data_0055f7a0 = nameHash14;
    data_0055f78c = nameHash15;
    data_0055f778 = nameHash16;
    data_0055f944 = nameHash1;
    data_0055f930 = nameHash2;
    data_0055f91c = nameHash3;
    data_0055f908 = nameHash4;
    data_0055f8f4 = nameHash5;
    data_0055f8e0 = nameHash6;
    data_0055f8cc = nameHash7;
    data_0055f8b8 = nameHash8;
    data_0055f9e4 = nameHash1;
    gNameHash2 = nameHash2;
    data_0055f9bc = nameHash3;
    data_0055f9a8 = nameHash4;
    data_0055f994 = nameHash5;
    data_0055f980 = nameHash6;
    data_0055f96c = nameHash7;
    data_0055f958 = nameHash8;
    data_0055fa34 = nameHash1;
    data_0055fa20 = nameHash2;
    data_0055fa0c = nameHash3;
    data_0055f9f8 = nameHash4;
    data_0055fa84 = nameHash1;
    data_0055fa70 = nameHash2;
    nameHash3Ptr = nameHash3;
    data_0055fa48 = nameHash4;
    data_0055fad4 = nameHash1;
    data_0055fac0 = nameHash2;
    data_0055faac = nameHash3;
    data_0055fa98 = nameHash4;
}

Float CMach_FloatReciprocal(Float value)
{
    value.data.value = 1.0L / value.data.value;
    return value;
}

static double data_0055fd18 = 0.0;
static double double_four = 1.0;
static double data_0055fd28 = -1.0;
static double data_0055fd30 = 0.5;
static double data_0055fd38 = -0.5;
static double data_0055fd40 = 2.0;
static double data_0055fd48 = -2.0;
static double data_0055fd50 = 4.0;
static double data_0055fd58 = -4.0;
static double data_0055fd60 = 8.0;

Boolean CMach_FloatIsPowerOf2(Float f)
{
    return f.data.value == data_0055fd18 || f.data.value == double_four || f.data.value == data_0055fd28 ||
           f.data.value == data_0055fd30 || f.data.value == data_0055fd38 || f.data.value == data_0055fd40 ||
           f.data.value == data_0055fd48 || f.data.value == data_0055fd50 || f.data.value == data_0055fd58 ||
           f.data.value == data_0055fd60;
}

const char *CMach_GetCPU(void)
{
    switch (copts.processor) {
        case 0:
            return "__PPC401__";
        case 1:
            return "__PPC403__";
        case 2:
            return "__PPC505__";
        case 3:
            return "__PPC509__";
        case 4:
            return "__PPC555__";
        case 5:
            return "__PPC601__";
        case 6:
            return "__PPC602__";
        case 7:
            return "__PPC603__";
        case 8:
            return "__PPC603e__";
        case 9:
            return "__PPC604__";
        case 10:
            return "__PPC604e__";
        case 11:
            return "__PPC740__";
        case 12:
            return "__PPC750__";
        case 13:
            return "__PPC801__";
        case 14:
            return "__PPC821__";
        case 15:
            return "__PPC823__";
        case 16:
            return "__PPC850__";
        case 17:
            return "__PPC860__";
        case 21:
            return "__PPC7400__";
        case 18:
            return "__PPC8240__";
        case 19:
            return "__PPC8260__";
        case 22:
            return "__PPCGEKKO__";
        default:
            return NULL;
    }
}

Boolean CMach_PassResultInHiddenArg(Type *type)
{
    switch ((char)type->type) {
        case 4:
            if ((char)type->type == 4 && TYPE_STRUCT(type)->stype >= TYPESTRUCT &&
                TYPE_STRUCT(type)->stype <= TYPETEMPLDEPEXPR)
                return 0;
            /* fall through */
        case 5:
            return 1;
        case 10:
            if (type->size == 4)
                return 0;
            return 1;
        default:
            return 0;
    }
}

Boolean Type_RequiresMemoryReturn(Type *type)
{
    SInt32 structKind;
    switch ((char)type->type) {
        case TYPESTRUCT: {
            structKind = TYPE_STRUCT(type)->stype;
            if (structKind >= 4 && structKind <= 14)
                return 0;
            if (type->size <= 8 && !copts.returnStructsInMemory)
                return 0;
            return 1;
        }
        case TYPECLASS:
            if (type->size <= 8 && (Boolean)(CClass_Constructor(TYPE_CLASS(type)) == NULL) &&
                !copts.returnStructsInMemory)
                return 0;
            return 1;
        case TYPEMEMBERPOINTER:
            if (type->size == 4)
                return 0;
            return 1;
        default:
            return 0;
    }
}

Boolean CMachine_FunctionRequiresMemoryReturn(TypeFunc *functype)
{
    SInt8 type = functype->functype->type;
    switch (type) {
        case TYPESTRUCT: {
            if (functype->functype->type == TYPESTRUCT) {
                SInt32 structKind = TYPE_STRUCT(functype->functype)->stype;
                if (structKind >= 4 && structKind <= 14)
                    return 0;
            }
        }
        case TYPECLASS:
        case TYPEMEMBERPOINTER:
            if (Type_RequiresMemoryReturn(functype->functype))
                return 1;
            return 0;
        default:
            return 0;
    }
}

long CMach_StructLayoutBitfield(TypeBitfield *field, int alignmentKind)
{
    short requestedAlignment;
    short naturalAlignment;
    short storageSize;
    short padding;
    long size;
    short alignment;
    short storageBits;

    padding = 0;
    naturalAlignment = 0;
    requestedAlignment = CMach_GetQUALalign(alignmentKind);
    if (requestedAlignment <= field->bitfieldtype->size)
        requestedAlignment = 0;
    size = field->bitfieldtype->size;
    switch (size) {
        case 1:
            storageSize = 1;
            storageBits = 8;
            naturalAlignment = 0;
            break;
        case 2:
            storageSize = 2;
            storageBits = 16;
            naturalAlignment = storageSize;
            break;
        case 4:
            size = copts.structalignment;
            if (size != 0 && size != 4)
                naturalAlignment = 4;
            else
                naturalAlignment = 2;
            storageSize = 4;
            storageBits = 32;
            break;
        default:
            CError_Internal("CMachine.c", 1437);
    }
    switch (copts.structalignment) {
        case 3:
        case 8:
            naturalAlignment = 0;
    }
    alignment = naturalAlignment;
    if (requestedAlignment > naturalAlignment)
        alignment = requestedAlignment;
    if (alignment != 0 && ((alignment - 1) & structLayoutOffset) != 0)
        padding = alignment - ((alignment - 1) & structLayoutOffset);
    if (data_00580fa0 == 0 && structLayoutOffset != 0 && requestedAlignment == 0 && padding != 0) {
        bitfield_storage_size = storageSize - padding;
        if ((short)(storageBits - (bitfield_storage_size << 3) - field->bitlength) >= 0)
            data_00580fa0 = bitfield_storage_size << 3;
    }
    if (data_00580fa0 == 0) {
        structLayoutOffset += padding;
        if (field->bitlength == 0)
            return structLayoutOffset;
        if (field->suppressAlignment == 0)
            maximumAlignment = maximumAlignment >= alignment ? maximumAlignment : alignment;
        data_00580fa0 = field->bitlength;
        bitfield_storage_size = storageSize;
        data_00580fa4 = structLayoutOffset;
        structLayoutOffset += storageSize;
        field->offset = 0;
        return data_00580fa4;
    }
    if (field->bitlength == 0 || data_00580fa0 + (signed char)field->bitlength > storageBits ||
        bitfield_storage_size > storageSize) {
        structLayoutOffset += padding;
        data_00580fa0 = 0;
        bitfield_storage_size = storageSize;
        if (field->bitlength == 0)
            return structLayoutOffset;
        if (field->suppressAlignment == 0)
            maximumAlignment = maximumAlignment >= alignment ? maximumAlignment : alignment;
        data_00580fa4 = structLayoutOffset;
        structLayoutOffset += storageSize;
    } else if (bitfield_storage_size < storageSize) {
        structLayoutOffset -= bitfield_storage_size;
        bitfield_storage_size = storageSize;
        if (field->bitlength == 0)
            return structLayoutOffset;
        if (field->suppressAlignment == 0)
            maximumAlignment = maximumAlignment >= naturalAlignment ? maximumAlignment : naturalAlignment;
        data_00580fa4 = structLayoutOffset;
        structLayoutOffset += storageSize;
    }
    field->offset = data_00580fa0;
    data_00580fa0 += field->bitlength;
    return data_00580fa4;
}

long CMach_StructLayoutGetOffset(Type *type, int flags)
{
    int unusedBits;
    unsigned int typeCode;
    int requiredAlignment;
    int currentOffset;
    int alignment;
    int mask;
    int offset;
    if (data_00580fa0 != 0) {
        unusedBits = (bitfield_storage_size << 3) - data_00580fa0;
        if (unusedBits >= 0) {
            structLayoutOffset -= unusedBits / 8;
        }
    }
    typeCode = CParser_GetTypeQualifiers(type, flags);
    data_00580fa0 = 0;
    requiredAlignment = CMach_GetQUALalign(typeCode);
    currentOffset = structLayoutOffset;
    alignment = get_type_align(type);
    if (requiredAlignment > alignment) {
        alignment = requiredAlignment;
    }
    maximumAlignment = maximumAlignment < alignment ? alignment : maximumAlignment;
    mask = alignment - 1;
    offset = (short)(mask & (alignment - (currentOffset & mask))) + structLayoutOffset;
    structLayoutOffset = offset + type->size;
    return offset;
}

static inline SInt16 cm_min(SInt32 index, SInt16 b)
{
    SInt32 a;
    SInt16 r;
    SInt32 n;
    SInt32 limit;
    n = index - 3;
    a = loadalign_table[n];
    r = a;
    limit = b;
    if (a >= limit)
        return b;
    return r;
}

static inline SInt32 cm_loadalign(UInt32 index)
{
    return loadalign_table[index];
}

static inline UInt16 cm_classmin(SInt16 b, SInt32 index)
{
    SInt32 a;
    int r;
    SInt32 limit;
    a = index - 3;
    a = cm_loadalign(a);
    r = a;
    limit = b;
    if (a >= limit)
        return b;
    return r;
}

static inline SInt16 cm_structalign(SInt32 index, TypeStruct *type)
{
    return cm_min(index, type->align);
}

static inline SInt16 cm_classalign(UInt32 index, SInt16 b)
{
    return cm_classmin(b, index);
}

static inline SInt32 cm_sizealign(Type *type, SInt32 alignment)
{
    SInt32 index = alignment - 3;
    SInt32 limit = cm_loadalign(index);
    if (type->size >= limit)
        return limit;
    return *(volatile SInt32 *)&type->size;
}

static inline SInt32 cm_alignment(void)
{
    return copts.structalignment;
}

int CMach_StructLayoutGetCurSize(void)
{
    int alignment = 0;
    int padding;

    if (data_00580fa0 != 0) {
        switch (copts.structalignment) {
            case 3:
            case 8:
                alignment = 8;
                break;
            case 0:
            case 4:
                alignment = 16;
                break;
        }
        if (alignment != 0) {
            padding = bitfield_storage_size * 8 - data_00580fa0;
            if (padding > 0) {
                structLayoutOffset -= padding / alignment;
            }
        }
        data_00580fa0 = 0;
    }
    return structLayoutOffset;
}

void CMach_StructLayoutInitOffset(unsigned int offset)
{
    structLayoutOffset = offset;
    data_00580fa0 = 0;
    bitfield_storage_size = 0;
    data_00580fa4 = 0;
}

void CMachine_ResetMaximumAlignment(void)
{
    maximumAlignment = 0;
    return;
}

SInt16 CMach_MemberAlignValue(Type *type, SInt32 offset)
{
    SInt16 alignment = get_type_align(type);
    if ((SInt32)alignment <= 1)
        return 0;
    return (alignment - (offset & (alignment - 1U))) & (alignment - 1U);
}

SInt16 get_type_align(Type *type)
{
    SInt32 alignment;
    alignment = CMachine_GetTypeAlignment(type);
    if (alignment <= 1) {
        return 1;
    }
    if (type->type == TYPESTRUCT) {
        TypeStruct *structType = (TypeStruct *)type;
        SInt32 structKind = structType->stype;
        if (structKind >= 4 && structKind <= 14) {
            return 16;
        }
    }
    switch (copts.structalignment) {
        case 8:
            alignment = 1;
            break;
        case 0:
            if (alignment > 2) {
                alignment = 2;
            }
            break;
        case 1:
            if (alignment > 4) {
                alignment = 4;
            }
            break;
    }
    return alignment;
}

SInt16 CMachine_GetTypeAlignment(Type *type)
{
    UInt8 arrayAlignment;
    Boolean isPowerAlignment;

    if (type->type == TYPESTRUCT) {
        SInt32 structType = ((TypeStruct *)type)->stype;
        if (structType >= 4 && structType <= 14)
            return 16;
    }

    switch (cm_alignment()) {
        case 3:
        case 8:
            return 1;
        case 4:
        case 5:
        case 6:
        case 7:
            isPowerAlignment = 1;
            break;
        default:
            isPowerAlignment = 0;
            break;
    }

    arrayAlignment = copts.arrayAlignment;

    for (;;) {
        switch ((SInt8)type->type) {
            case TYPEVOID:
                return 0;
            case TYPEFUNC:
                return 0;
            case TYPEENUM:
                type = ((TypeEnum *)type)->enumtype;
                /* fall through */
            case TYPEINT:
                if (isPowerAlignment) {
                    return cm_sizealign(type, cm_alignment());
                }
                if (type->size == 1)
                    return 1;
                if (cm_alignment() != 0 && type->size >= 8)
                    return 8;
                if (cm_alignment() != 0 && type->size >= 4)
                    return 4;
                return 2;
            case TYPEFLOAT:
                if (isPowerAlignment) {
                    return cm_sizealign(type, cm_alignment());
                }
                switch (cm_alignment()) {
                    case 0:
                        return 2;
                    case 1:
                        return 4;
                    case 2:
                        if (type->size > 4)
                            return 8;
                        return 4;
                    default:
                        CError_FATAL(1173);
                }
                /* fall through */
            case TYPEMEMBERPOINTER:
            case TYPEPOINTER:
                if (isPowerAlignment) {
                    return cm_sizealign(type, cm_alignment());
                }
                if (!copts.structalignment)
                    return 2;
                return 4;
            case TYPEARRAY:
                if (arrayAlignment != 0) {
                    if (isPowerAlignment) {
                        return cm_sizealign(type, cm_alignment());
                    }
                    if (type->size == 1)
                        return 1;
                    if (cm_alignment() == 0 || type->size <= 2)
                        return 2;
                    if (cm_alignment() == 1 || type->size < 8)
                        return 4;
                    {
                        SInt32 elementAlignment = CMachine_GetTypeAlignment(((TypePointer *)type)->target);
                        if (elementAlignment > 4)
                            return elementAlignment;
                        return 4;
                    }
                }
                type = ((TypePointer *)type)->target;
                continue;
            case TYPESTRUCT:
                if (isPowerAlignment)
                    return cm_structalign(cm_alignment(), (TypeStruct *)type);
                return ((TypeStruct *)type)->align;
            case TYPECLASS:
                if (isPowerAlignment)
                    return cm_classalign(cm_alignment(), ((TypeClass *)type)->align);
                return ((TypeClass *)type)->align;
            case TYPEBITFIELD:
                type = ((TypeBitfield *)type)->bitfieldtype;
                continue;
            case TYPETEMPLATE:
                return 1;
            case TYPELABEL:
            default:
                CError_FATAL(1218);
                return 0;
        }
    }
}

/* Records and links used by the maximum-value scan. */

short CMach_GetClassAlign(TypeClass *list)
{
    int maximum;
    ClassList *node;

    maximum = maximumAlignment;
    maximumAlignment = 0;

    node = list->bases;
    while (node != NULL) {
        int value = node->base->align;
        if (value > maximum)
            maximum = value;
        node = node->next;
    }
    return maximum;
}

UInt16 fn_004a8400(TypeStruct *str)
{
    unsigned int lift_value_0;
    lift_value_0 = maximumAlignment;
    maximumAlignment = 0;
    return lift_value_0;
}

void CMach_PragmaParams(void)
{
    if (copts.f9f != 0) {
        CError_Warning(ERR_ILLEGAL_PRAGMA, 0);
    }
    while (CPrep_ScanMacroExpandedChar() != 0) {
        CPrepTokenizer_GetNextToken();
    }
}

void CMach_PrintFloat(char *output, Float value)
{
    FloatFormatBuffer buffer;
    Float doubleValue;
    float singleValue;

    do {
        if (stshortdouble.type == 2) {
            switch (stshortdouble.size) {
                case 4:
                    singleValue = (float)value.data.value;
                    memcpy(&buffer.value, &singleValue, sizeof(singleValue));
                    CTool_EndianConvertMem(&buffer.value, sizeof(singleValue));
                    continue;
                case 8:
                    doubleValue = value;
                    memcpy(&buffer.value, &doubleValue, sizeof(doubleValue));
                    CTool_EndianConvertMem(&buffer.value, sizeof(doubleValue));
                    continue;
                default:
                    break;
            }
        }
        CError_Internal("CMachine.c", 779);
    } while (0);
    CTool_EndianConvertMem(&buffer.value, sizeof(buffer.value));
    sprintf(output, "%g", (long double)buffer.value);
}

void CMach_InitFloatMem(Type *type, Float value, unsigned char *dest)
{
    if (type->type == TYPEFLOAT) {
        switch (type->size) {
            case 4: {
                float f = value.data.value;
                memcpy(dest, &f, 4);
                CTool_EndianConvertMem(dest, 4);
                return;
            }
            case 8: {
                double d = value.data.value;
                memcpy(dest, &d, 8);
                CTool_EndianConvertMem(dest, 8);
                return;
            }
        }
    }
    CError_FATAL(779);
}

UInt8 CMach_FloatIsNegOne(double value)
{
    return value == negative_one;
}

unsigned char CMach_FloatIsOne(double value)
{
    return value == DAT_0055fff0;
}

unsigned char CMach_FloatIsZero(double value)
{
    return value == float_zero;
}

Float CMachine_RoundFloatToType(Type *type, Float value)
{
    switch (type->size) {
        case 4:
            value.data.value = (float)value.data.value;
            break;
        case 8:
            value.data.value = value.data.value;
            break;
        case 10:
            break;
        case 12:
            break;
        default:
            CError_Internal("CMachine.c", 714);
    }
    return value;
}

/* 0x474a30, signed 64 -> double */
/* 0x474ab0, unsigned 64 -> double */

Float CMach_CalcFloatConvertFromInt(Type *type, CInt64 value)
{
    Float f;
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        if (Type_IsUnsigned(type))
            f.data.value = CExpr2_ConvertUnsignedCInt64ToDouble(&value);
        else
            f.data.value = CExpr2_ConvertCInt64ToDouble(&value);
    } else {
        if (Type_IsUnsigned(type))
            f.data.value = value.lo;
        else
            f.data.value = (SInt32)value.lo;
    }
    return f;
}

void *CMach_FloatScan(char *text, Float *result, char *flag)
{
    union {
        double value;
        Float integer;
    } val;

    char *end = CTemplateClass_ParseDouble(text, &val.value, flag);
    if (end == NULL)
        CError_FatalError(ERR_NUMBER_OUT_RANGE);
    if (*flag) {
        Float value = data_00560028;
        *result = value;
    } else
        *result = val.integer;
    return end;
}

/* Four-word operands compared by the machine operation. */

unsigned char CMach_CalcVectorDiadicBool(unsigned int context, const union MWVector128 *left, unsigned int operation,
                                         const union MWVector128 *right)
{
    switch ((short)operation) {
        case 360:
            return left->longElements[0] == right->longElements[0] && left->longElements[1] == right->longElements[1] &&
                   left->longElements[2] == right->longElements[2] && left->longElements[3] == right->longElements[3];
        case 361:
            return left->longElements[0] != right->longElements[0] && left->longElements[1] != right->longElements[1] &&
                   left->longElements[2] != right->longElements[2] && left->longElements[3] != right->longElements[3];
        default:
            CError_Internal("CMachine.c", 653);
            return 0;
    }
}

/* 0x55ff24, string "CMachine.c" */

Boolean CMach_CalcFloatDiadicBool(Type *self, volatile double a, SInt16 op, volatile double b)
{
    int dead_1;
    dead_1 = 0;
    switch (op) {
        case 0x168:
            return a == b;
        case 0x169:
            return a != b;
        case 0x16a:
            return a <= b;
        case 0x16b:
            return a >= b;
        case 0x3e:
            return a > b;
        case 0x3c:
            return a < b;
        default:
            CError_Internal("CMachine.c", 0x273);
            return 0;
    }
}

Float CMach_CalcFloatMonadic(Type *type, short op, double value)
{
    Float result;
    if (op != 0x2d)
        CError_FATAL(605);
    value = -value;
    result.data.value = value;
    switch (type->size) {
        case 4:
            result.data.value = (float)result.data.value;
            break;
        case 8:
            result.data.value = result.data.value;
            break;
        case 10:
        case 12:
            break;
        default:
            CError_FATAL(714);
            break;
    }
    return result;
}

static inline Float CMach_CalcFloatConvert(Type *type, Float value)
{
    switch (type->size) {
        case 4:
            value.data.value = (float)value.data.value;
            break;
        case 8:
            value.data.value = (double)value.data.value;
            break;
        case 10:
        case 12:
            break;
        default:
            CError_FATAL(714);
    }
    return value;
}

Float CMach_CalcFloatDiadic(Type *type, Float left, short op, Float right)
{
    switch (op) {
        case '+':
            left.data.value += right.data.value;
            break;
        case '-':
            left.data.value -= right.data.value;
            break;
        case '*':
            left.data.value *= right.data.value;
            break;
        case '/':
            left.data.value /= right.data.value;
            break;
        default:
            CError_FATAL(592);
    }
    return CMach_CalcFloatConvert(type, left);
}

void CMachine_InitVectorMem(Type *type, MWVector128 val, void *mem)
{
    UInt8 uc[16];
    UInt16 us[8];
    UInt32 ul[4];
    float f[4];
    int i;

    switch ((char)type->type) {
        case TYPESTRUCT:
            switch (TYPE_STRUCT(type)->stype) {
                case 4:
                case 5:
                case 6:
                    for (i = 0; i < 16; i++) {
                        if (!copts.nativeByteOrder)
                            uc[i] = val.byteElements[i];
                        else
                            uc[i] = val.byteElements[15 - i];
                    }
                    memcpy(mem, uc, 16);
                    break;
                case 7:
                case 8:
                case 9:
                case 14:
                    for (i = 0; i < 8; i++) {
                        if (!copts.nativeByteOrder)
                            us[i] = CTool_EndianConvertWord16(val.shortElements[i]);
                        else
                            us[i] = CTool_EndianConvertWord16(val.shortElements[7 - i]);
                    }
                    memcpy(mem, us, 16);
                    break;
                case 10:
                case 11:
                case 12:
                    for (i = 0; i < 4; i++) {
                        if (!copts.nativeByteOrder)
                            ul[i] = CTool_EndianConvertWord32(val.longElements[i]);
                        else
                            ul[i] = CTool_EndianConvertWord32(val.longElements[3 - i]);
                    }
                    memcpy(mem, ul, 16);
                    break;
                case 13:
                    for (i = 0; i < 4; i++) {
                        if (!copts.nativeByteOrder)
                            f[i] = val.floatElements[i];
                        else
                            f[i] = val.floatElements[3 - i];
                        CTool_EndianConvertMem(&f[i], 4);
                    }
                    memcpy(mem, f, 16);
                    break;
                default:
                    CError_FATAL(568);
            }
            break;
        default:
            CError_FATAL(572);
    }
}

void CMach_InitIntMem(Type *type, CInt64 val, void *mem)
{
    UInt8 ch;
    UInt16 sh;
    UInt32 lg;

    switch ((char)type->type) {
        case TYPEINT:
            switch (type->size) {
                case 1:
                    ch = (UInt8)val.lo;
                    memcpy(mem, &ch, 1);
                    break;
                case 2:
                    sh = val.lo;
                    sh = CTool_EndianConvertWord16(sh);
                    memcpy(mem, &sh, 2);
                    break;
                case 4:
                    lg = CTool_EndianConvertWord32(val.lo);
                    memcpy(mem, &lg, 4);
                    break;
                case 8:
                    CTool_EndianConvertWord64(val, mem);
                    break;
                default:
                    CError_FATAL(482);
            }
            break;
        default:
            CError_FATAL(486);
    }
}

CInt64 CMach_CalcIntConvertFromFloat(Type *type, double value)
{
    CInt64 result;
    if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
        if (Type_IsUnsigned(type))
            CExpr2_ConvertDoubleToUnsignedCInt64(&result, value);
        else
            CExpr2_ConvertDoubleToCInt64(&result, value);
    } else if (Type_IsUnsigned(type)) {
        result.hi = 0;
        result.lo = value;
    } else {
        SInt32 converted;
        result.lo = (SInt32)value;
        converted = value;
        result.hi = (converted < 0) ? -1 : 0;
    }
    return result;
}

CInt64 CMach_CalcIntMonadic(Type *type, SInt16 op, CInt64 val)
{
    if (Type_IsUnsigned(type)) {
        switch (type->size) {
            case 1:
                CExpr2_ConvertCInt64ToUInt8(&val);
                break;
            case 2:
                CExpr2_ConvertCInt64ToUnsignedShort(&val);
                break;
            case 4:
                CExpr2_ClearCInt64Hi(&val);
                break;
            case 8:
                break;
            default:
                CError_Internal("CMachine.c", 364);
        }
        switch (op) {
            case '-':
                val = CInt64_Inv(val);
                break;
            case '~':
                val = CFunc_BitwiseNot(val);
                break;
            case '!':
                val = CFunc_LogicalNotCInt64(val);
                break;
            default:
                CError_ReportError(ERR_UNEXPECTED_TOKEN);
        }
        switch (type->size) {
            case 1:
                CExpr2_ConvertCInt64ToUInt8(&val);
                break;
            case 2:
                CExpr2_ConvertCInt64ToUnsignedShort(&val);
                break;
            case 4:
                CExpr2_ClearCInt64Hi(&val);
                break;
            case 8:
                break;
        }
    } else {
        switch (type->size) {
            case 1:
                CExpr2_SignExtendSignedChar(&val);
                break;
            case 2:
                CExpr2_SignExtendShort(&val);
                break;
            case 4:
                CExpr2_SignExtendCInt64(&val);
                break;
            case 8:
                break;
            default:
                CError_Internal("CMachine.c", 394);
        }
        switch (op) {
            case '-':
                val = CInt64_Inv(val);
                break;
            case '~':
                val = CFunc_BitwiseNot(val);
                break;
            case '!':
                val = CFunc_LogicalNotCInt64(val);
                break;
            default:
                CError_ReportError(ERR_UNEXPECTED_TOKEN);
        }
        switch (type->size) {
            case 1:
                CExpr2_SignExtendSignedChar(&val);
                break;
            case 2:
                CExpr2_SignExtendShort(&val);
                break;
            case 4:
                CExpr2_SignExtendCInt64(&val);
                break;
            case 8:
                break;
        }
    }
    return val;
}

#define ISZERO64(v) ((Boolean)(((v).hi == 0) && ((v).lo == 0)))

CInt64 CMach_CalcIntDiadic(Type *type, CInt64 a, SInt16 op, CInt64 b)
{
    if (Type_IsUnsigned(type)) {
        switch (type->size) {
            case 1:
                CExpr2_ConvertCInt64ToUInt8(&a);
                CExpr2_ConvertCInt64ToUInt8(&b);
                break;
            case 2:
                CExpr2_ConvertCInt64ToUnsignedShort(&a);
                CExpr2_ConvertCInt64ToUnsignedShort(&b);
                break;
            case 4:
                CExpr2_ClearCInt64Hi(&a);
                CExpr2_ClearCInt64Hi(&b);
                break;
            case 8:
                break;
            default:
                CError_FATAL(243);
        }
        switch (op) {
            case '*':
                a = CInt64_MulU(a, b);
                break;
            case '/': {
                Boolean z = (b.hi == 0) && (b.lo == 0);
                if (z) {
                    CError_Warning(ERR_DIVISION_BY_0);
                } else {
                    a = CInt64_DivU(a, b);
                }
            } break;
            case '%': {
                Boolean z = (b.hi == 0) && (b.lo == 0);
                if (z) {
                    CError_Warning(ERR_DIVISION_BY_0);
                } else {
                    a = CInt64_ModU(a, b);
                }
            } break;
            case '+':
                a = CInt64_Add(a, b);
                break;
            case '-':
                a = CInt64_Sub(a, b);
                break;
            case 0x16c:
                a = CInt64_Shl(a, b);
                break;
            case 0x16d:
                a = CInt64_ShrU(a, b);
                break;
            case '<': {
                SInt32 t = CInt64_LessU(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case '>': {
                SInt32 t = CInt64_GreaterU(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x16a: {
                Boolean t = CInt64_LessEqualU(a, b);
                SInt32 result = t;
                a.lo = result;
                a.hi = (result < 0) ? -1 : 0;
            } break;
            case 0x16b: {
                Boolean t = CInt64_GreaterEqualU(a, b);
                SInt32 result = t;
                a.lo = result;
                a.hi = (result < 0) ? -1 : 0;
            } break;
            case 0x168: {
                SInt32 t = CInt64_Equal(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x169: {
                SInt32 t = CInt64_NotEqual(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case '&':
                a = CInt64_And(a, b);
                break;
            case '^':
                a = xor_64(a, b);
                break;
            case '|':
                a = CExpr2_BitwiseOrCInt64(a, b);
                break;
            case 0x167: {
                SInt32 t = !ISZERO64(a) && !ISZERO64(b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x166: {
                SInt32 t = !ISZERO64(a) || !ISZERO64(b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            default:
                CError_ReportError(ERR_UNEXPECTED_TOKEN);
        }
        switch (type->size) {
            case 1:
                CExpr2_ConvertCInt64ToUInt8(&a);
                break;
            case 2:
                CExpr2_ConvertCInt64ToUnsignedShort(&a);
                break;
            case 4:
                CExpr2_ClearCInt64Hi(&a);
                break;
            case 8:
                break;
        }
    } else {
        switch (type->size) {
            case 1:
                CExpr2_SignExtendSignedChar(&a);
                CExpr2_SignExtendSignedChar(&b);
                break;
            case 2:
                CExpr2_SignExtendShort(&a);
                CExpr2_SignExtendShort(&b);
                break;
            case 4:
                CExpr2_SignExtendCInt64(&a);
                CExpr2_SignExtendCInt64(&b);
                break;
            case 8:
                break;
            default:
                CError_FATAL(305);
        }
        switch (op) {
            case '*':
                a = CInt64_Mul(a, b);
                break;
            case '/': {
                Boolean z = (b.hi == 0) && (b.lo == 0);
                if (z) {
                    CError_Warning(ERR_DIVISION_BY_0);
                } else {
                    a = CInt64_Div(a, b);
                }
            } break;
            case '%': {
                Boolean z = (b.hi == 0) && (b.lo == 0);
                if (z) {
                    CError_Warning(ERR_DIVISION_BY_0);
                } else {
                    a = CInt64_Mod(a, b);
                }
            } break;
            case '+':
                a = CInt64_Add(a, b);
                break;
            case '-':
                a = CInt64_Sub(a, b);
                break;
            case 0x16c:
                a = CInt64_Shl(a, b);
                break;
            case 0x16d:
                a = CInt64_Shr(a, b);
                break;
            case '<': {
                SInt32 t = CInt64_Less(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case '>': {
                SInt32 t = CInt64_Greater(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x16a: {
                SInt32 t = CInt64_LessEqual(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x16b: {
                Boolean t = CInt64_GreaterEqual(a, b);
                SInt32 result = t;
                a.lo = result;
                a.hi = (result < 0) ? -1 : 0;
            } break;
            case 0x168: {
                SInt32 t = CInt64_Equal(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x169: {
                SInt32 t = CInt64_NotEqual(a, b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case '&':
                a = CInt64_And(a, b);
                break;
            case '^':
                a = xor_64(a, b);
                break;
            case '|':
                a = CExpr2_BitwiseOrCInt64(a, b);
                break;
            case 0x167: {
                SInt32 t = !ISZERO64(a) && !ISZERO64(b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            case 0x166: {
                SInt32 t = !ISZERO64(a) || !ISZERO64(b);
                a.lo = t;
                a.hi = (t < 0) ? -1 : 0;
            } break;
            default:
                CError_ReportError(ERR_UNEXPECTED_TOKEN);
        }
        switch (type->size) {
            case 1:
                CExpr2_SignExtendSignedChar(&a);
                break;
            case 2:
                CExpr2_SignExtendShort(&a);
                break;
            case 4:
                CExpr2_SignExtendCInt64(&a);
                break;
            case 8:
                break;
        }
    }
    return a;
}

int CMach_GetQUALalign(int qualifiers)
{
    int alignment = 0;

    if ((qualifiers &= Q_ALIGNED_MASK) != 0) {
        if (qualifiers == 0x02000000) {
            alignment = 1;
        } else if (qualifiers == 0x04000000) {
            alignment = 2;
        } else if (qualifiers == 0x06000000) {
            alignment = 4;
        } else if (qualifiers == 0x08000000) {
            alignment = 8;
        } else if (qualifiers == 0x0A000000) {
            alignment = 16;
        } else if (qualifiers == 0x0C000000) {
            alignment = 32;
        } else if (qualifiers == 0x10000000) {
            alignment = 64;
        } else if (qualifiers == 0x12000000) {
            alignment = 128;
        } else if (qualifiers == 0x14000000) {
            alignment = 256;
        } else if (qualifiers == 0x16000000) {
            alignment = 512;
        } else if (qualifiers == 0x18000000) {
            alignment = 1024;
        } else if (qualifiers == 0x1A000000) {
            alignment = 2048;
        } else if (qualifiers == 0x1C000000) {
            alignment = 4096;
        } else if (qualifiers == Q_ALIGNED_MASK) {
            alignment = 8192;
        } else {
            CError_Internal("CMachine.c", 202);
        }
    }
    return alignment;
}
