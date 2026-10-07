#include "compiler/common.h"
#include "compiler/InlineAsmRegisters.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CBrowse.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/FunctionCalls.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"

#include <setjmp.h>
#include <string.h>
#include <stdio.h>

static struct RegisterBinding *register_binding_hash[64];

void CTemplateNew_ClearGlobalArray(void)
{
    SInt32 index;
    for (index = 0; index < 64; index++)
        register_binding_hash[index] = NULL;
}

void CTemplateNew_InsertRegisterBinding(const char *key, unsigned int attribute1, short registerNumber, Object *object)
{
    struct RegisterBinding *entry;
    struct RegisterBinding **bucket;

    bucket = &register_binding_hash[CHash(key) & 63];
    entry = (struct RegisterBinding *)lalloc(sizeof(*entry));
    entry->key = (unsigned int)key;
    entry->attribute1 = attribute1;
    entry->registerNumber = registerNumber;
    entry->object = object;
    entry->next = *bucket;
    *bucket = entry;
}

void *find_register_binding_key(unsigned int *key)
{
    struct RegisterBinding *binding = register_binding_hash[CHash((const char *)key) & 0x3f];
    while (binding) {
        unsigned int *bindingKey = &binding->key;
        if (strcmp((const char *)binding->key, (const char *)key) == 0)
            return bindingKey;
        binding = binding->next;
    }
    return NULL;
}
