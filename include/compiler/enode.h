#ifndef COMPILER_ENODE_H
#define COMPILER_ENODE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENODE_IS(_enode, _etype) ((_enode)->type == (_etype))
/* The expression node kinds, in the order and with the names of the compiler's own table of them (DumpIR's, at
 * 0x55268c: index = kind). */
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
#pragma options align = mac68k
struct ENodeList {
    ENodeList *next; /* 0x00: CExpr_MakeFunctionCall traverses and appends call arguments */
    ENode *node;     /* 0x04: CExpr_MakeFunctionCall converts each argument expression */
};
#pragma options align = reset
#pragma options align = mac68k
union ENodeUnion {
    struct {
        UInt32 pid;
        UInt32 name;
    } templdep;
    CInt64 intval;
    long long bits;
    Float floatval;
    MWVector128 vector128;
    SInt32 longval;
    ENode *monadic;
    struct MemberFunctionPointerData *memberFunctionPointer;
    struct MemberFuncRef *
        memberfunc; /* 0x00: make_memberpointer reads ENEWEXCEPTIONARRAY expression, addressTaken, bcl and list from the same member reference */
    Object *objref;
    struct ObjectList *
        overloadCandidates; /* 0x00: make_static_method_setconst stores ENEWEXCEPTION candidates; match_template_function_args reads this kind */
    struct CLabel *
        labelAddress; /* 0x00: unary_expression selects ELOCOBJ for TK_LOGICAL_AND (GNU label address), stores findlabel/newlabel */
    struct {
        char *info;
        UInt8 unk0e[8];
        SInt32 sourceLocation;
    } inlineasm;
    struct {
        struct NameSpaceObjectList *list;
        struct TemplArg *
            templargs; /* 0x04: CExpr.c copies objlist.templargs into MemberFuncRef.templargs; CExpr_MakeFunctionCall reads template arguments for ENEWEXCEPTION */
        struct HashNameNode *name;
    } objlist;
    struct {
        ENode *left;
        ENode *right;
    } diadic;
    struct {
        ENode *expression; /* 0x00: get_objaccess_cached_value, type == EINSTRUCTION, selects this expression */
        struct Operand *
            cachedValue; /* 0x04: get_objaccess_cached_value, type == EINSTRUCTION, allocates and reuses the selected operand */
    } objaccess;         /* get_objaccess_cached_value tests EINSTRUCTION before reading this variant */
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
        ENodeList *
            arguments; /* 0x00: scan_explicit_conversion stores arguments; CInline_00513240 copies them for EOBJLIST tag 3 */
        Type *
            targetType; /* 0x04: scan_explicit_conversion stores the conversion type; CExpr_MakeFunctionCall reads it for EOBJLIST tag 3 */
        SInt32
            qualifiers; /* 0x08: scan_explicit_conversion stores qualifiers; CExpr_MakeFunctionCall passes them to CExpr_DoExplicitConversion for EOBJLIST tag 3 */
        UInt8
            tag; /* 0x0c: CExpr_MakeFunctionCall tests EOBJLIST and templatecomparison.tag == 3 before reading this variant */
    } explicitconversion; /* EOBJLIST with tag 3: dependent explicit conversion, not a function call */
    struct {
        ENode *expression; /* 0x00: CFunc_DefaultArg stores the dependent default argument expression */
        struct TStreamElement *sourcePosition; /* 0x04: CFunc_DefaultArg allocates and copies the last buffered token */
        UInt8 unk08[4];                        /* 0x08: CExpr2_NewENEWEXCEPTIONARRAYNode clears this unused storage */
        UInt8 tag; /* 0x0c: CFunc_DefaultArg selects EOBJLIST with tag ST_IFGOTO via CExpr2_NewENEWEXCEPTIONARRAYNode */
    } defaultargument; /* EOBJLIST, tag ST_IFGOTO: dependent default argument with diagnostic source position */
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
            struct {
                UInt16 parameterIndex;
                UInt8 templateLevel;
            } wb;
            void *p0;
            SInt32 d0;
        } u;
        void *p4;
        SInt32 qualifiers;
        UInt8 tag;
    } templatecomparison;
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
    UInt8 metadata[4]; /* 0x00: ELOCOBJ consumers CInline_005114e0 and print_enode_tree leave these bytes unused */
    struct InlineMemberPointerTarget *target; /* 0x04: CInline_005114e0 marks target flags for ELOCOBJ */
    struct HashNameNode *name; /* 0x08: print_enode_tree prints ELOCOBJ name; inline_member_index compares it */
};

#ifdef __cplusplus
}
#endif

#endif
