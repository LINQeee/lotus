#include "var_manager.h"
#include "llvm.h"

#include <string.h>

static void moveAllocaBuilder() {
    LLVMValueRef firstInstr = LLVMGetFirstInstruction(cg->currentAllocaBlock);
    if (!firstInstr) LLVMPositionBuilderAtEnd(cg->allocaBuilder, cg->currentAllocaBlock);
    else LLVMPositionBuilderBefore(cg->allocaBuilder, firstInstr);
}

LLVMVariable *createVar(const char *name, const LLVMValueRef storage, const LotusType type) {
    moveAllocaBuilder();
    LLVMVariable *var = &cg->vars[cg->varsCount++];
    strcpy(var->name, name);
    var->storage = storage;
    var->type = type;
    var->relatedFn = cg->currentFn;
    return var;
}

LLVMVariable *getVar(const char *name) {
    LLVMVariable *globalFoundVar = NULL;
    for (int i = 0; i < cg->varsCount; i++) {
        if (strcmp(cg->vars[i].name, name) == 0) {
            if (cg->vars[i].relatedFn == cg->currentFn) return &cg->vars[i];
            globalFoundVar = &cg->vars[i];
        }
    }
    return globalFoundVar;
}
