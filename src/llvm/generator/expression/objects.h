#ifndef OBJECT_GENERATOR_H
#define OBJECT_GENERATOR_H
#include <llvm-c/Types.h>

#include "../../types.h"
#include "../../../parser/parser.h"

void prepareObjectCodegen();
void retainObject(Type *type, LLVMValueRef value);
void releaseObject(Type *type, LLVMValueRef value);
LLVMValueRef generateObject(const ObjectNode *node);
LLVMValueRef generateMember(const MemberNode *node);
#endif
