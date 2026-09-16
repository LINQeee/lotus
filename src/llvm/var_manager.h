#ifndef VAR_MANAGER_H
#define VAR_MANAGER_H
#include <llvm-c/Types.h>

#include "./lotus_types.h"

typedef struct {
    char name[64];
    LLVMValueRef storage;
    LotusType type;
    LLVMValueRef relatedFn;
} LLVMVariable;

LLVMVariable *createVar(const char *name, LLVMValueRef storage, LotusType type);

LLVMVariable *getVar(const char *name);
#endif
