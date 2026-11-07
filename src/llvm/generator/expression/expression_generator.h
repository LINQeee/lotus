#ifndef EXPRESSION_GENERATOR_H
#define EXPRESSION_GENERATOR_H
#include "../../../parser/parser.h"
#include <llvm-c/Types.h>

LLVMValueRef generateExpression(const Node *node);
void retainString(LLVMValueRef value);
void releaseString(LLVMValueRef value);
#endif
