#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <string.h>

#include "../llvm.h"
#include "../var_manager.h"
#include "../generator/generator.h"
#include "llvm_extension.h"
#include "../generator/expression/functions.h"
#include <llvm-c/Error.h>
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>
#include "../../parser/parser.h"

void LLVMMoveBuilderToEnd(const LLVMBasicBlockRef block) {
    LLVMPositionBuilderAtEnd(cg->builder, block);
    cg->currentBlock = block;
    const char *blockName = LLVMGetBasicBlockName(block);
    if (strstr(blockName, "while.body") == NULL && strstr(blockName, "while.cond") == NULL)
        cg->currentAllocaBlock = block;
}

LLVMBasicBlockRef LLVMAppendBlock(const char *name) {
    return LLVMAppendBasicBlockInContext(cg->context, cg->currentFn, name);
}

void createMainFunctionAndGenerateCode(const ProgramNode *program) {
    collectFunctions((const Node *)program);

    LLVMTypeRef returnType = LLVMInt32TypeInContext(cg->context);
    LLVMTypeRef functionType = LLVMFunctionType(returnType, NULL, 0, 0);
    LLVMValueRef mainFunction = LLVMAddFunction(cg->module, "main", functionType);
    cg->currentFn = mainFunction;
    cg->mainFunction = cg->currentFn;
    LLVMBasicBlockRef entry = LLVMAppendBlock("entry");
    LLVMMoveBuilderToEnd(entry);

    generateNode((Node *)program);

    if (!LLVMGetBasicBlockTerminator(cg->currentBlock)) {
        releaseVariables(0);
        LLVMBuildRet(cg->builder, LLVMConstInt(LLVMInt32TypeInContext(cg->context), 0, 0));
    }
}

void verifyCode() {
    char *error = NULL;
    if (LLVMVerifyModule(cg->module, LLVMReturnStatusAction, &error)) {
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(EXIT_FAILURE);
    }
    LLVMDisposeMessage(error);
}

LLVMTargetMachineRef getTargetMachine() {
    char *triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef target;
    char *error = NULL;

    if (LLVMGetTargetFromTriple(triple, &target, &error)) {
        fprintf(stderr, "Target error: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeMessage(triple);
        return NULL;
    }

    LLVMTargetMachineRef targetMachine =
        LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

    LLVMTargetDataRef dataLayout = LLVMCreateTargetDataLayout(targetMachine);
    LLVMSetModuleDataLayout(cg->module, dataLayout);
    LLVMSetTarget(cg->module, triple);

    LLVMDisposeTargetData(dataLayout);
    LLVMDisposeMessage(triple);

    if (!targetMachine) {
        fprintf(stderr, "Failed to create TargetMachine\n");
        return NULL;
    }
    return targetMachine;
}

void optimizeCode() {
    LLVMTargetMachineRef targetMachine = getTargetMachine();

    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    LLVMPassBuilderOptionsSetLoopVectorization(options, 1);
    LLVMPassBuilderOptionsSetSLPVectorization(options, 1);

    LLVMErrorRef error = LLVMRunPasses(cg->module, "default<O3>", targetMachine, options);
    if (error) {
        char *msg = LLVMGetErrorMessage(error);
        fprintf(stderr, "Pipeline Error: %s\n", msg);
        LLVMDisposeErrorMessage(msg);
    }

    LLVMDisposePassBuilderOptions(options);
    LLVMDisposeTargetMachine(targetMachine);
}
