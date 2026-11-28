#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <llvm-c/Types.h>

#include "../../types.h"
#include "../../../parser/parser.h"

typedef struct {
    char name[64];
    int argc;
    Type *args[32];
    Type *returnType;
    LLVMTypeRef fnType;
    LLVMValueRef llvmFunction;
} LLVMUserFunction;

void collectFunctions(const Node *node);

LLVMValueRef generateCall(const CallNode *node);

LLVMUserFunction *findFunction(const char *name);

void generateFunction(const FunctionNode *node);
#endif
