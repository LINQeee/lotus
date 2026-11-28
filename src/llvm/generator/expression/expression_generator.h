#ifndef EXPRESSION_GENERATOR_H
#define EXPRESSION_GENERATOR_H
#include "../../../parser/parser.h"
#include <llvm-c/Types.h>

LLVMValueRef generateBinaryValues(const BinaryNode *node, LLVMValueRef left, LLVMValueRef right);
LLVMValueRef generateExpression(const Node *node);
#endif
