#ifndef VAR_MANAGER_H
#define VAR_MANAGER_H
#include <llvm-c/Types.h>

#include "./lotus_types.h"

typedef struct {
    char name[64];
    LLVMValueRef alloca;
    LotusType type;
} LLVMVariable;

LLVMVariable *createVar(const char *name, LLVMValueRef alloca, LotusType type);

LLVMVariable *getVar(const char *name);
#endif
