#ifndef COMPILER_ENODE_H
#define COMPILER_ENODE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENODE_IS(_enode, _etype) ((_enode)->type == (_etype))
typedef enum ENodeType {
    EPOSTINC,
    EPOSTDEC,
    EPREINC,
    EPREDEC,
    EINDIRECT,
    EMONMIN,
    EBINNOT,
    ELOGNOT,
    EFORCELOAD,
    EMUL,
    EMULV,
    EDIV,
    EMODULO,
    EADDV,
    ESUBV,
    EADD,
    ESUB,
    ESHL,
    ESHR,
    ELESS,
    EGREATER,
    ELESSEQU,
    EGREATEREQU,
    EEQU,
    ENOTEQU,
    EAND,
    EXOR,
    EOR,
    ELAND,
    ELOR,
    EASS,
    EMULASS,
    EDIVASS,
    EMODASS,
    EADDASS,
    ESUBASS,
    ESHLASS,
    ESHRASS,
    EANDASS,
    EXORASS,
    EORASS,
    ECOMMA,
    EPMODULO,
    EROTL,
    EROTR,
    EBCLR,
    EBTST,
    EBSET,
    ETYPCON,
    EBITFIELD,
    EINTCONST,
    EFLOATCONST,
    ESTRINGCONST,
    ECOND,
    EFUNCCALL,
    EFUNCCALLP,
    EOBJREF,
    EQUALNAME,
    EMFPOINTER,
    ENULLCHECK,
    EPRECOMP,
    ETEMP,
    EARGOBJ,
    ELOCOBJ,
    ETEMPX,
    ELABEL,
    ESETCONST,
    ENEWEXCEPTION,
    ENEWEXCEPTIONARRAY,
    EOBJLIST,
    EMEMBER,
    EINSTRUCTION,
    EDEFINE,
    EREUSE,
    EASSBLK,
    EVECTOR128CONST,
    ESTMT = 76,
} ENodeType;
enum {
    ENODE_FLAG_CONST = Q_CONST,
    ENODE_FLAG_VOLATILE = Q_VOLATILE,
    ENODE_FLAG_QUALS = Q_CONST | Q_VOLATILE,
    ENODE_FLAG_80 = 0x80
};
/* data.templdep.subtype of a template-dependent expression */
typedef enum TemplDepSubType {
    TDE_PARAM,
    TDE_SIZEOF,
    TDE_ALIGNOF,
    TDE_CAST,
    TDE_QUALNAME,
    TDE_OBJ,
    TDE_SOURCEREF,
    TDE_ADDRESS_OF
} TemplDepSubType;
#pragma options align = mac68k
struct ENodeList {
    ENodeList *next;
    ENode *node;
};
#pragma options align = reset
#pragma options align = mac68k
union ENodeUnion {
    CInt64 intval;
    long long bits;
    Float floatval;
    MWVector128 vector128val;
    SInt32 longval;
    ENode *monadic;
    struct MemberFunctionPointerData *memberFunctionPointer;
    struct EMemberInfo *emember;
    Object *objref;
    struct ObjectList *overloadCandidates;
    struct CLabel *label;
    struct {
        char *info;
        UInt8 unk0e[8];
        SInt32 sourceLocation;
    } inlineasm;
    struct {
        struct NameSpaceObjectList *list;
        struct TemplArg *templargs;
        struct HashNameNode *name;
    } objlist;
    struct {
        ENode *left;
        ENode *right;
    } diadic;
    struct {
        ENode *expression;
        struct Operand *cachedValue;
    } objaccess; /* get_objaccess_cached_value tests EINSTRUCTION before reading this variant */
    struct {
        ENode *label;
        ENode *expression;
        unsigned int labelId;
    } precomp;
    struct {
        ENode *cond;
        ENode *expr1;
        ENode *expr2;
    } cond;
    struct {
        ENode *funcref;
        ENodeList *args;
        TypeFunc *functype;
    } funccall;
    struct {
        Type *type;
        SInt32 uniqueid;
        Boolean needs_dtor;
    } temp;
    struct {
        unsigned int originalValue;
        unsigned int auxiliaryValue;
        unsigned char state;
    } scopebegin;
    struct {
        SInt32 size;
        char *data;
        unsigned char useExplicitSize;
    } string;
    struct {
        Object *objref;
        SInt32 offset;
    } addr;
    struct {
        ENode *initexpr;
        ENode *tryexpr;
        Object *pointertemp;
        Object *deletefunc;
    } newexception;
    struct {
        ENode *allocationAssignment;
        ENode *initialization;
        Object *temporary;
        Object *constructor;
    } argobj;
    struct {
        union {
            TemplParamID pid;
            struct {
                Type *type;
            } typeexpr;
            struct {
                ENodeList *args;
                Type *type;
                UInt32 qual;
            } cast;
            struct {
                TypeTemplDep *type;
                HashNameNode *name;
            } qual;
            struct {
                ENode *expr;
                TStreamElement *token;
            } sourceref;
            ENode *monadic;
            Object *obj;
        } u;
        UInt8 subtype;
    } templdep;
};
#pragma options align = reset
/* sizeof(ENode) is 0x1a: every allocation site requests 26 bytes. */
#pragma options align = mac68k
struct ENode {
    UInt8 type;
    UInt8 cost;
    UInt16 flags;
    Boolean ignored;
    Boolean hascall;
    Type *rtype;
    ENodeUnion data;
};
#pragma options align = reset
struct MemberFunctionPointerData {
    UInt8 metadata[4];
    struct InlineMemberPointerTarget *target;
    struct HashNameNode *name;
};

#ifdef __cplusplus
}
#endif

#endif
