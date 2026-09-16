#ifndef LLVM_H
#define LLVM_H
// magic number to be honest TODO: dynamic var realloc
#define MAX_VARS 4096
#define MAX_LOOP_DEPTH 256
#define MAX_NATIVE 256

#include "../parser/parser.h"

#include <llvm-c/Core.h>

#include "generator/expression/functions.h"
#include "generator/expression/native_functions.h"
#include "var_manager.h"

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

    LLVMValueRef currentFn;

    LLVMValueRef mainFunction;

    LLVMVariable vars[MAX_VARS];
    int varsCount;

    LoopContext loops[MAX_LOOP_DEPTH];
    int loopCount;

    LLVMNativeFunction natives[MAX_NATIVE];
    int nativeCount;

    LLVMUserFunction functions[256];
    int functionCount;

    LLVMTypeRef i64Type;
    LLVMTypeRef i32Type;
    LLVMTypeRef boolType;
    LLVMTypeRef f32Type;
    LLVMTypeRef f64Type;
    LLVMTypeRef voidType;
    LLVMTypeRef i8Type;
    LLVMTypeRef stringType;
} Codegen;

Codegen *cg;

void codegenProgram(const ProgramNode *program);

void initCodegen();
#endif
