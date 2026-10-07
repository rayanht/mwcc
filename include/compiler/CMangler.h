#ifndef COMPILER_CMANGLER_H
#define COMPILER_CMANGLER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern HashNameNode *COptimizer_GetFunctionObject(Object *obj);
extern HashNameNode *CMangler_GetLinkName(Object *obj);
extern void mangle_function_name(HashNameNode *name, NameSpace *chain, Type *func);
extern void mangle_args(FuncArg *args);
extern void fn_004c2ac0(Type *type, SInt32 flag);
extern HashNameNode *CMangler_ConversionFuncName(Type *type, UInt32 qual);
extern HashNameNode *get_object_link_name(Object *object);
extern HashNameNode *CMangler_GetCovariantFunctionName(Object *object, Type *type);
extern void mangle_type(Type *type, UInt32 flags);
extern void mangle_qualified_name(NameSpace *nameSpace, const char *name);
extern HashNameNode *CMangler_TemplateInstanceName(HashNameNode *name, TemplArg *list);
extern HashNameNode *CMangler_ThunkName(Object *input, int offset, int adjustment, int index);
extern HashNameNode *CMangler_RTTIObjectName(Type *type, unsigned int flags);
extern HashNameNode *CMangler_VTableName(TypeClass *entry);
extern GList data_00583548;
extern HashNameNode *CMangler_OperatorName(short token);
extern char *CMangler_GetOperator(HashNameNode *name);
extern struct HashNameNode *assignment_operator_name;
extern HashNameNode *CMangler_DeleteDtorName(void);
extern HashNameNode *CMangler_SDeleteDtorName(void);
extern HashNameNode *CMangler_ArrayDtorName(void);
extern HashNameNode *CMangler_VBaseDtorName(void);
extern HashNameNode *CMangler_BasicDtorName(void);
extern void CMangler_Setup(void);

#ifdef __cplusplus
}
#endif

#endif
