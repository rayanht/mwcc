#include "compiler/common.h"
#include "driver/StaticParserGlue.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/ClientGlue.h"

int fn_0040534e(void)
{
    int __stdcall fn_00405280(void *, void *);
    return fn_00405280(&PTR_fn_005366e8, &PTR_fn_0053670c);
}

int fn_0040535e(void)

{
    return ClientGlue_AddResourceStrings("Parser Strings", 0x2eea, data_005374c4);
}
