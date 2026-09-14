#include "var_manager.h"
#include "llvm.h"

#include <string.h>

static void moveAllocaBuilder() {
  LLVMValueRef firstInstr = LLVMGetFirstInstruction(cg->currentAllocaBlock);
  if (!firstInstr)
    LLVMPositionBuilderAtEnd(cg->allocaBuilder, cg->currentAllocaBlock);
  else
    LLVMPositionBuilderBefore(cg->allocaBuilder, firstInstr);
}

LLVMVariable *createVar(const char *name, const LLVMValueRef alloca,
                        const LotusType type) {
  moveAllocaBuilder();
  LLVMVariable *var = &cg->vars[cg->varCount++];
  strcpy(var->name, name);
  var->alloca = alloca;
  var->type = type;
  return var;
}

LLVMVariable *getVar(const char *name) {
  for (int i = 0; i < cg->varCount; i++) {
    if (strcmp(cg->vars[i].name, name) == 0)
      return &cg->vars[i];
  }
  return NULL;
}
