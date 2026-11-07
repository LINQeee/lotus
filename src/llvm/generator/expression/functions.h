#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <llvm-c/Types.h>

#include "../../lotus_types.h"
#include "../../../parser/parser.h"

typedef struct {
    char name[64];
    int argc;
    LotusType args[32];
    LotusType returnType;
    LLVMTypeRef fnType;
    LLVMValueRef llvmFunction;
} LLVMUserFunction;

void collectFunctions(const Node *node);

LLVMValueRef generateCall(const CallNode *node);

LLVMUserFunction *findFunction(const char *name);

void generateFunction(const FunctionNode *node);
#endif
