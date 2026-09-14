#ifndef LLVM_H
#define LLVM_H
#define MAX_VARS 4096
#define MAX_LOOP_DEPTH 256
#define MAX_NATIVE 256

#include "../parser/parser.h"

#include <llvm-c/Core.h>

#include "var_manager.h"
#include "generator/expression/functions.h"
#include "generator/expression/native_functions.h"

typedef struct {
    LLVMBasicBlockRef continueBlock;
    LLVMBasicBlockRef breakBlock;
} LoopContext;


typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMBuilderRef allocaBuilder;
    LLVMBasicBlockRef currentAllocaBlock;
    LLVMBasicBlockRef currentBlock;

    LLVMValueRef function;

    LLVMVariable vars[MAX_VARS];
    int varCount;

    LoopContext loops[MAX_LOOP_DEPTH];
    int loopCount;

    LLVMNativeFunction natives[MAX_NATIVE];
    int nativeCount;

    LLVMUserFunction functions[256];
    int functionCount;
} Codegen;

Codegen *cg;

void codegenProgram(
    const ProgramNode *program
);

void initCodegen();
#endif
