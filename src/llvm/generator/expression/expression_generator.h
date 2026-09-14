#ifndef EXPRESSION_GENERATOR_H
#define EXPRESSION_GENERATOR_H
#include <llvm-c/Types.h>
#include "../../../parser/parser.h"

LLVMValueRef generateExpression(const Node *node);
#endif
