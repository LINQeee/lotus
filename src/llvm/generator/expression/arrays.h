#ifndef ARRAY_GENERATOR_H
#define ARRAY_GENERATOR_H
#include "../../../parser/parser.h"
#include <llvm-c/Types.h>
LLVMValueRef generateArray(const ArrayNode *node);
LLVMValueRef generateIndex(const IndexNode *node);
LLVMValueRef generateLength(const LengthNode *node);
#endif
