#ifndef NATIVE_CALLER_H
#define NATIVE_CALLER_H
#include "../../../parser/parser.h"
#include "../../types.h"
#include <llvm-c/Types.h>

LLVMValueRef generateNativeCall(const CallNode *node);
LLVMValueRef callArrayCreate(LLVMValueRef size, Type *elementType);
LLVMValueRef callArrayAt(LLVMValueRef array, LLVMValueRef index);
LLVMValueRef callArrayLength(LLVMValueRef array);
LLVMValueRef callStringEquals(LLVMValueRef left, LLVMValueRef right);
LLVMValueRef callStringConcat(LLVMValueRef left, LLVMValueRef right);
void retainIfNeeded(Type *type, LLVMValueRef value);
void releaseIfNeeded(Type *type, LLVMValueRef value);
#endif
