#include "var_manager.h"

#include <string.h>

#include "llvm.h"
#include "generator/expression/native_caller.h"
#include "generator/expression/native_functions.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include "lotus_types.h"

LLVMVariable *createVar(const char *name, const LLVMValueRef storage, const LotusType type) {
    LLVMVariable *var = &cg->vars[cg->varsCount++];
    strcpy(var->name, name);
    var->storage = storage;
    var->type = type;
    var->relatedFn = cg->currentFn;
    return var;
}

LLVMVariable *getVar(const char *name) {
    LLVMVariable *globalFoundVar = NULL;
    for (int i = cg->varsCount - 1; i >= 0; i--) {
        if (strcmp(cg->vars[i].name, name) == 0) {
            if (cg->vars[i].relatedFn == cg->currentFn) return &cg->vars[i];
            if (!globalFoundVar && cg->vars[i].relatedFn == cg->mainFunction) globalFoundVar = &cg->vars[i];
        }
    }
    return globalFoundVar;
}

void releaseVariables(int start) {
    for (int i = cg->varsCount - 1; i >= start; i--) {
        LLVMVariable *var = &cg->vars[i];
        if (!isMemoryType(var->type)) continue;
        LLVMValueRef value = LLVMBuildLoad2(cg->builder, convertType(var->type), var->storage, "ref.cleanup");
        releaseIfNeeded(var->type, value);
    }
}
