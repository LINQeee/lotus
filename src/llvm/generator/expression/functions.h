#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <llvm-c/Types.h>
#include "native_functions.h"

typedef struct {
    char name[64];
    int argc;
    LotusType args[32];
    LotusType returnType;
    LLVMTypeRef fnType;
    LLVMValueRef llvmFunction;
} LLVMUserFunction;

void collectFunctions();

LLVMValueRef generateCall(const CallNode *node);

LLVMUserFunction *findFunction(const char *name);

void generateFunction(const FunctionNode *node);
#endif
