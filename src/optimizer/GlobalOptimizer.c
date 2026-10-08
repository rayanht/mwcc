#include "compiler/common.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/AddPropagation.h"
#include "compiler/CExpr2.h"
#include "compiler/CMangler.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/CopyPropagation.h"
#include "compiler/LoopDetection.h"
#include "compiler/LoopOptimization.h"
#include "compiler/PCodeListing.h"
#include "compiler/StrengthReduction.h"
#include "compiler/ValueNumbering.h"
#include "compiler/VectorArraysToRegs.h"

int gCodeMotionChanged;
SInt32 gValueNumberingChanged;
int gCopyPropagationChanged;
struct Loop *data_0058763c;
SInt32 gStrengthReductionChanged;

static inline void COptimizer_DumpStage(Object *function, const char *stage)
{
    Object *obj = function;
    fn_004c4bd0(COptimizer_GetFunctionObject(obj)->name, stage);
}

static inline void COptimizer_DumpIfChanged(Object *function, int changed, const char *stage)
{
    if (changed && copts.debug_listing) {
        COptimizer_DumpStage(function, stage);
    }
}

static inline void COptimizer_RunCopyPropagation(Object *function, int mode)
{
    COpt_CopyPropagation(mode);
    COptimizer_DumpIfChanged(function, gCopyPropagationChanged, "AFTER COPY PROPAGATION");
}

static inline void COptimizer_RunAddPropagation(Object *function)
{
    COpt_AddPropagation();
    COptimizer_DumpIfChanged(function, gAddPropagationChanged, "AFTER ADD PROPAGATION");
}

static inline void COptimizer_RunLoopPasses(Object *function)
{
    LoopDetection_DetectLoops();
    if (data_0058763c != NULL) {
        COpt_SetLoopCodeMotionMode(1);
        LoopDetection_ComputeLoopPropertiesRecursive();
        CodeMotion_VisitLoops();
        COpt_SetLoopCodeMotionMode(0);
        LoopDetection_TraverseLoopsPostorder(data_0058763c);
        COptimizer_DumpIfChanged(function, gCodeMotionChanged, "AFTER CODE MOTION");

        StrengthReduction_RunLoopPasses();
        if (gStrengthReductionChanged) {
            COpt_CopyPropagation(1);
            if (copts.debug_listing) {
                COptimizer_DumpStage(function, "AFTER STRENGTH REDUCTION");
            }
        }

        fn_005289b0();
        if (gLoopTransformChanged) {
            COpt_CopyPropagation(1);
            COpt_AddPropagation();
            if (copts.debug_listing) {
                COptimizer_DumpStage(function, "AFTER LOOP TRANSFORMATIONS");
            }
        }
    }

    if (!gCopyPropagationChanged) {
        COptimizer_RunCopyPropagation(function, 1);
    }
}

void COptimizer_Level3(Object *function)
{
    ValueNumbering_PerformValueNumbering(0);
    COptimizer_DumpIfChanged(function, gValueNumberingChanged, "AFTER VALUE NUMBERING");
    COptimizer_RunCopyPropagation(function, 0);

    gCopyPropagationChanged = 0;
    COptimizer_RunAddPropagation(function);
    {
        Object *recovery_inline_5167_0 = (function);

        LoopDetection_DetectLoops();
        if (data_0058763c != NULL) {
            COpt_SetLoopCodeMotionMode(1);
            LoopDetection_ComputeLoopPropertiesRecursive();
            CodeMotion_VisitLoops();
            COpt_SetLoopCodeMotionMode(0);
            LoopDetection_TraverseLoopsPostorder(data_0058763c);
            COptimizer_DumpIfChanged(recovery_inline_5167_0, gCodeMotionChanged, "AFTER CODE MOTION");

            StrengthReduction_RunLoopPasses();
            if (gStrengthReductionChanged) {
                COpt_CopyPropagation(1);
                if (copts.debug_listing) {
                    COptimizer_DumpStage(recovery_inline_5167_0, "AFTER STRENGTH REDUCTION");
                }
            }

            fn_005289b0();
            if (gLoopTransformChanged) {
                COpt_CopyPropagation(1);
                COpt_AddPropagation();
                if (copts.debug_listing) {
                    COptimizer_DumpStage(recovery_inline_5167_0, "AFTER LOOP TRANSFORMATIONS");
                }
            }
        }

        if (!gCopyPropagationChanged) {
            COptimizer_RunCopyPropagation(recovery_inline_5167_0, 1);
        }
    }

    COpt_ConstantPropagation();
    if (gConstantPropagationChanged) {
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER CONSTANT PROPAGATION");
        }
        COpt_LoadDeletion();
        COptimizer_DumpIfChanged(function, gLoadDeletionChanged, "AFTER LOAD DELETION");
        COptimizer_RunAddPropagation(function);
    }

    ValueNumbering_PerformValueNumbering(1);
    if (gValueNumberingChanged) {
        COpt_CopyPropagation(1);
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER VALUE NUMBERING 2");
        }
    }
}

void COptimizer_Level4(Object *function)
{
    ValueNumbering_PerformValueNumbering(0);
    COptimizer_DumpIfChanged(function, gValueNumberingChanged, "AFTER VALUE NUMBERING");
    COptimizer_RunCopyPropagation(function, 0);

    gCopyPropagationChanged = 0;
    COptimizer_RunAddPropagation(function);
    {
        Object *recovery_inline_6220_0 = (function);

        LoopDetection_DetectLoops();
        if (data_0058763c != NULL) {
            COpt_SetLoopCodeMotionMode(1);
            LoopDetection_ComputeLoopPropertiesRecursive();
            CodeMotion_VisitLoops();
            COpt_SetLoopCodeMotionMode(0);
            LoopDetection_TraverseLoopsPostorder(data_0058763c);
            COptimizer_DumpIfChanged(recovery_inline_6220_0, gCodeMotionChanged, "AFTER CODE MOTION");

            StrengthReduction_RunLoopPasses();
            if (gStrengthReductionChanged) {
                COpt_CopyPropagation(1);
                if (copts.debug_listing) {
                    COptimizer_DumpStage(recovery_inline_6220_0, "AFTER STRENGTH REDUCTION");
                }
            }

            fn_005289b0();
            if (gLoopTransformChanged) {
                COpt_CopyPropagation(1);
                COpt_AddPropagation();
                if (copts.debug_listing) {
                    COptimizer_DumpStage(recovery_inline_6220_0, "AFTER LOOP TRANSFORMATIONS");
                }
            }
        }

        if (!gCopyPropagationChanged) {
            COptimizer_RunCopyPropagation(recovery_inline_6220_0, 1);
        }
    }

    COpt_ConstantPropagation();
    if (gConstantPropagationChanged) {
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER CONSTANT PROPAGATION");
        }
        COpt_LoadDeletion();
        COptimizer_DumpIfChanged(function, gLoadDeletionChanged, "AFTER LOAD DELETATION");

        if (gConstantPropagationChanged) {
            COpt_CopyPropagation(1);
        }
        COptimizer_RunAddPropagation(function);

        if (gArrayToRegisterEnabled) {
            COpt_ArrayToRegister();
            if (gArrayToRegisterChanged && copts.debug_listing) {
                COptimizer_DumpStage(function, "AFTER ARRAY => REGISTER TRANSFORM");
                COpt_ConstantPropagation();
                if (gConstantPropagationChanged) {
                    COpt_CopyPropagation(1);
                }
                if (copts.debug_listing) {
                    COptimizer_DumpStage(function, "AFTER CONSTANT PROPAGATION 2");
                }
            }
        }
    }

    ValueNumbering_PerformValueNumbering(1);
    if (gValueNumberingChanged) {
        COpt_CopyPropagation(1);
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER VALUE NUMBERING 2");
        }
    }

    if (gVectorArrayConversion && fn_0052ce10()) {
        COpt_CopyPropagation(0);
        COpt_CopyPropagation(1);
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER VECTOR ARRAY CONVERSION");
        }
    }

    LoopDetection_DetectLoops();
    if (data_0058763c == NULL) {
        return;
    }

    COpt_SetLoopCodeMotionMode(1);
    LoopDetection_ComputeLoopPropertiesRecursive();
    CodeMotion_VisitLoops();
    COptimizer_DumpIfChanged(function, gCodeMotionChanged, "AFTER CODE MOTION 2");

    ValueNumbering_PerformValueNumbering(1);
    if (gValueNumberingChanged) {
        COpt_CopyPropagation(1);
        if (copts.debug_listing) {
            COptimizer_DumpStage(function, "AFTER VALUE NUMBERING 3");
        }
    }
}

void COptimizer_Optimize(Object *function)
{
    char level;

    if (copts.debug_listing) {
        COptimizer_DumpStage(function, "BEFORE GLOBAL OPTIMIZATION");
    }

    if ((level = copts.deleteDeadInstructions) == 2 || (gRunLevel2Pipeline && level > 2)) {
        ValueNumbering_PerformValueNumbering(1);
        COptimizer_DumpIfChanged(function, gValueNumberingChanged, "AFTER CSE");
        COptimizer_RunCopyPropagation(function, 1);
        COptimizer_RunAddPropagation(function);
    } else if (level == 3) {
        COptimizer_Level3(function);
    } else if (level == 4) {
        COptimizer_Level4(function);
    }
}
