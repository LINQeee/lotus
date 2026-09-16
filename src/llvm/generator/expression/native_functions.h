#ifndef NATIVE_FUNCTIONS_H
#define NATIVE_FUNCTIONS_H
#include "../../../parser/parser.h"
#include "../../lotus_types.h"
#include <llvm-c/Types.h>
#include <stdbool.h>

typedef struct {
    char name[64];
    LotusType returnType;
    LotusType args[32];
    int argc;
    void *fn;
    bool variadic;
    LLVMTypeRef fnType;
    LLVMValueRef llvmFunction;
} LLVMNativeFunction;

typedef struct {
    const char *name;
    bool variadic;
    LotusType returnType;
    int argc;
    const LotusType args[32];
    void *fn;
} LotusFunctionInfo;

void prepareNativeFunction(const LotusFunctionInfo *info);

LLVMNativeFunction *findNative(const char *name);

LLVMTypeRef convertType(LotusType type);

LotusType resolveExpressionType(const Node *node);

LLVMValueRef generateNativeCall(const CallNode *node);
#endif
