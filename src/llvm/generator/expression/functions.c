#include "functions.h"
#include "../../../parser/parser.h"
#include "../../llvm.h"
#include "native_functions.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "../../utils/llvm_extension.h"
#include "../generator.h"
#include "expression_generator.h"

void collectFunctions(Node *node) {
  if (node->type == NODE_FUNCTION) {
    FunctionNode *fn = (FunctionNode *)node;

    LLVMUserFunction *f = &cg->functions[cg->functionCount++];

    strcpy(f->name, fn->name);

    f->argc = fn->paramCount;

    LLVMTypeRef params[32];

    for (int i = 0; i < fn->paramCount; i++) {
      f->args[i] = LOTUS_I32;

      params[i] = LLVMInt32TypeInContext(cg->context);
    }

    LLVMTypeRef type = LLVMFunctionType(LLVMInt32TypeInContext(cg->context),
                                        params, f->argc, 0);

    f->fnType = type;

    f->llvmFunction = LLVMAddFunction(cg->module, f->name, type);
  } else {
    switch (node->type) {
    case NODE_PROGRAM:
      ProgramNode *program = (ProgramNode *)node;
      for (int i = 0; i < program->count; i++)
        collectFunctions(program->statements[i]);
      // TODO collect funcs from entire code
      break;
    }
  }
}

LLVMUserFunction *findFunction(const char *name) {
  for (int i = 0; i < cg->functionCount; i++) {
    if (strcmp(cg->functions[i].name, name) == 0)
      return &cg->functions[i];
  }

  return NULL;
}

void generateFunction(const FunctionNode *node) {
  LLVMUserFunction *fn = findFunction(node->name);

  LLVMBasicBlockRef prevCurBlock = cg->currentBlock;
  LLVMBasicBlockRef prevCurAllocaBlock = cg->currentAllocaBlock;
  LLVMValueRef prevFunc = cg->function;

  cg->function = fn->llvmFunction;

  LLVMBasicBlockRef entry = LLVMAppendBlock("entry");

  LLVMMoveBuilderToEnd(entry);
  LLVMPositionBuilderAtEnd(cg->allocaBuilder, entry);

  for (int i = 0; i < fn->argc; i++) {
    LLVMValueRef arg = LLVMGetParam(fn->llvmFunction, i);
    LLVMVariable *var = createVar(
        node->params[i]->name,
        LLVMBuildAlloca(cg->allocaBuilder, LLVMInt32TypeInContext(cg->context),
                        node->params[i]->name),
        node->params[i]->base.dataType);

    LLVMBuildStore(cg->builder, arg, var->alloca);
  }

  generateNode(node->body);

  if (!LLVMGetBasicBlockTerminator(entry)) {
    LLVMBuildRet(cg->builder,
                 LLVMConstInt(LLVMInt32TypeInContext(cg->context), 0, 0));
  }

  cg->currentBlock = prevCurBlock;
  cg->currentAllocaBlock = prevCurAllocaBlock;
  cg->function = prevFunc;
  LLVMMoveBuilderToEnd(cg->currentBlock);
}

LLVMValueRef generateCall(const CallNode *node) {
  char *name = (node->callee)->name;
  printf("%s\n", name);

  LLVMUserFunction *fn = findFunction(name);

  if (fn) {
    LLVMValueRef args[32];

    for (int i = 0; i < node->argumentCount; i++) {
      args[i] = generateExpression(node->arguments[i]);
    }
    return LLVMBuildCall2(cg->builder, fn->fnType, fn->llvmFunction, args,
                          node->argumentCount, "calltmp");
  }

  LLVMNativeFunction *native = findNative(name);

  if (native)
    return generateNativeCall(node);

  printf("Unknown function %s\n", name);
  exit(1);
}
