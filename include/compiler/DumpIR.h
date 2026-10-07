#ifndef COMPILER_DUMPIR_H
#define COMPILER_DUMPIR_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void print_enode_tree(ENode *node, int depth);
extern void dump_eat_nodes(struct CException *p);
extern void format_type(Type *type, char *buf);
extern void fn_004be830(void *arg1, void *arg2);
extern void fn_004be840(void);
extern void write_escaped_string(void *a1, char *s, SInt32 n);
extern Statement *DumpIR_OptimizeStatements(Object *object, Statement *statements);
extern UInt8 data_00581300;
extern void DumpIR_OptimizeStatementList(Object *object, Statement *statements);

#ifdef __cplusplus
}
#endif

#endif
