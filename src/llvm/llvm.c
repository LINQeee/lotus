#include "llvm.h"
#include "executable.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <llvm-c/Target.h>

#include "utils/llvm_extension.h"
#include "../utils/cutils.h"
#include "../parser/parser.h"
#include <llvm-c/Types.h>

Codegen *cg;

void initCodegen() {
    cg = mallocSafe(sizeof(Codegen));
    cg->varsCount = 0;
    cg->functionVarsStart = 0;
    cg->loopCount = 0;
    cg->nativeCount = 0;
    cg->functionCount = 0;
    cg->context = LLVMContextCreate();
    cg->module = LLVMModuleCreateWithNameInContext("lotus", cg->context);
    cg->builder = LLVMCreateBuilderInContext(cg->context);
    cg->allocaBuilder = LLVMCreateBuilderInContext(cg->context);

    cg->i64Type = LLVMInt64TypeInContext(cg->context);
    cg->i32Type = LLVMInt32TypeInContext(cg->context);
    cg->boolType = LLVMInt1TypeInContext(cg->context);
    cg->voidType = LLVMVoidTypeInContext(cg->context);
    cg->f32Type = LLVMFloatTypeInContext(cg->context);
    cg->f64Type = LLVMDoubleTypeInContext(cg->context);
    cg->i8Type = LLVMInt8TypeInContext(cg->context);

    cg->stringType = LLVMStructCreateNamed(cg->context, "String");

    LLVMTypeRef fields[] = {cg->i64Type, LLVMPointerType(cg->i8Type, 0), cg->i64Type};
    LLVMStructSetBody(cg->stringType, fields, 3, false);
}

int codegenProgram(const ProgramNode *program, bool debug, const char *irPath, const char *outputBuildPath) {
    createMainFunctionAndGenerateCode(program);
    verifyCode();
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    if (debug) {
        char *ir = LLVMPrintModuleToString(cg->module);
        printf("\n\n###### GENERATED IR:\n%s\n", ir);
        LLVMDisposeMessage(ir);
    }

    optimizeCode();

    char *error = NULL;
    if (irPath && LLVMPrintModuleToFile(cg->module, irPath, &error)) {
        fprintf(stderr, "Cannot write IR: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }

    if (outputBuildPath) return buildExecutable(outputBuildPath);

    LLVMExecutionEngineRef engine;
    if (LLVMCreateExecutionEngineForModule(&engine, cg->module, &error)) {
        fprintf(stderr, "JIT ERROR: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }
    uint64_t addr = LLVMGetFunctionAddress(engine, "main");
    int (*mainFunc)() = (int (*)())addr;
    if (!addr) exitWithError("JIT did not produce main");
    if (debug) printf("###### RUNNING main():\n");
    struct timespec start, end;
    timespec_get(&start, TIME_UTC);
    int result = mainFunc();
    timespec_get(&end, TIME_UTC);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    if (debug) printf("\n\n###### EXECUTED MAIN FUNCTION IN %.0f ms:\nExit code: %d\n", elapsed, result);
    LLVMDisposeExecutionEngine(engine);
    return result;
}
