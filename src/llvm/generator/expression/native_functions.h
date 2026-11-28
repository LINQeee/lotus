#ifndef NATIVE_FUNCTIONS_H
#define NATIVE_FUNCTIONS_H
#include "../../types.h"
#include "lotus.h"
#include <llvm-c/Types.h>
#include <stdbool.h>

typedef struct {
    char name[64];
    Type *returnType;
    Type *args[32];
    int argc;
    void *fn;
    bool variadic;
    LLVMTypeRef fnType;
    LLVMValueRef llvmFunction;
} LLVMNativeFunction;

Type *fromNativeType(NativeType type);
NativeType toNativeType(Type *type);

void prepareNativeFunction(const LotusFunctionInfo *info);

LLVMTypeRef lotusValueLLVMType(void);
LLVMTypeRef convertType(Type *type);

#endif
