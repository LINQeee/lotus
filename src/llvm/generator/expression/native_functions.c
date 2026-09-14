#include "native_functions.h"
#include "../../llvm.h"
#include <stdbool.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "../../../type_checker/type_checker.h"
#include "../../../utils/cutils.h"
#include "expression_generator.h"

LLVMNativeFunction *findNative(const char *name) {
  for (int i = 0; i < cg->nativeCount; i++) {
    if (strcmp(cg->natives[i].name, name) == 0)
      return &cg->natives[i];
  }

  return NULL;
}

LLVMTypeRef lotusValueLLVMType() {
  static LLVMTypeRef type = NULL;

  if (type)
    return type;

  LLVMTypeRef fields[] = {cg->i32Type, cg->i64Type};

  type = LLVMStructCreateNamed(cg->context, "LotusValue");

  LLVMStructSetBody(type, fields, 2, false);

  return type;
}

LLVMTypeRef convertType(const LotusType type) {
  switch (type) {
  case LOTUS_I32:
    return cg->i32Type;
  case LOTUS_I64:
    return cg->i64Type;
  case LOTUS_BOOL:
    return cg->boolType;
  case LOTUS_F32:
    return cg->f32Type;
  case LOTUS_F64:
    return cg->f64Type;
  case LOTUS_STRING:
    return LLVMPointerType(cg->i8Type, 0);
  case LOTUS_VOID:
    return cg->voidType;
  case LOTUS_ANY:
    return lotusValueLLVMType();
  }
  return cg->voidType;
}

void prepareNativeFunction(const LotusFunctionInfo *info) {
  LLVMNativeFunction *native = &cg->natives[cg->nativeCount++];

  strcpy(native->name, info->name);

  native->returnType = info->returnType;
  native->argc = info->argc;
  native->variadic = info->variadic;
  native->fn = info->fn;

  for (int i = 0; i < info->argc; i++)
    native->args[i] = info->args[i];

  LLVMTypeRef params[32];
  if (info->variadic)
    params[0] = cg->i32Type;
  for (int i = 0; i < info->argc; i++)
    params[info->variadic ? i + 1 : i] = convertType(info->args[i]);

  LLVMTypeRef fnType = LLVMFunctionType(convertType(info->returnType), params,
                                        info->argc, info->variadic);
  native->fnType = fnType;

  native->llvmFunction = LLVMAddFunction(cg->module, native->name, fnType);

  typeCheckerAddNativeFunction(info->name, info->returnType, info->args,
                               info->argc, info->variadic);
}

LotusType resolveExpressionType(const Node *node) {
  switch (node->type) {
  case NODE_NUMBER: {
    const NumberNode *n = (NumberNode *)node;
    return n->base.dataType;
  }

  case NODE_STRING:
    return LOTUS_STRING;

    // case NODE_BOOLEAN: TODO
    //     return LOTUS_BOOL;

  case NODE_CALL: {
    LLVMNativeFunction *nativeFn = findNative(((CallNode *)node)->callee->name);
    LLVMUserFunction *userFn = findFunction(((CallNode *)node)->callee->name);
    if (!nativeFn && !userFn) {
      printf("Unknown function %s\n", ((CallNode *)node)->callee->name);
      exit(1);
    }

    return nativeFn ? nativeFn->returnType : userFn->returnType;
  }

  case NODE_IDENTIFIER: {
    LLVMVariable *var = getVar(((IdentifierNode *)node)->name);

    if (!var)
      exitWithError("Unknown variable %s", ((IdentifierNode *)node)->name);
    return var->type;
    // return var->type; TODO
  }

  default:
    return LOTUS_I32;
  }
}

LLVMValueRef buildLotusValue(const Node *node) {
  LotusType type = resolveExpressionType(node);
  LLVMValueRef value = generateExpression(node);

  LLVMTypeRef valueType = lotusValueLLVMType();
  LLVMValueRef result = LLVMGetUndef(valueType);

  result = LLVMBuildInsertValue(cg->builder, result,
                                LLVMConstInt(cg->i32Type, type, 0), 0, "");

  LLVMValueRef payload = NULL;

  switch (type) {
  case LOTUS_I32:
    payload = LLVMBuildSExt(cg->builder, value, cg->i64Type, "");
    break;

  case LOTUS_I64:
    payload = value;
    break;

  case LOTUS_BOOL:
    payload = LLVMBuildZExt(cg->builder, value, cg->i64Type, "");
    break;

  case LOTUS_STRING:
    payload = LLVMBuildPtrToInt(cg->builder, value, cg->i64Type, "");
    break;

  case LOTUS_F32:
    payload = LLVMBuildFPExt(cg->builder, value, cg->f64Type, "");

    payload = LLVMBuildBitCast(cg->builder, payload, cg->i64Type, "");
    break;

  case LOTUS_F64:
    payload = LLVMBuildBitCast(cg->builder, value, cg->i64Type, "");
    break;

  default:
    payload = LLVMConstInt(cg->i64Type, 0, 0);
  }

  result = LLVMBuildInsertValue(cg->builder, result, payload, 1, "");

  return result;
}

LLVMValueRef generateNativeCall(const CallNode *node) {
  LLVMNativeFunction *native = findNative(node->callee->name);
  LLVMValueRef args[64];
  if (native->variadic)
    args[0] = LLVMConstInt(cg->i32Type, node->argumentCount, 0);
  for (int i = 0; i < node->argumentCount; i++) {
    LotusType expected;
    if (i < native->argc)
      expected = native->args[i];
    else
      expected = native->args[native->argc - 1];
    if (expected == LOTUS_ANY)
      args[native->variadic ? i + 1 : i] = buildLotusValue(node->arguments[i]);
    else
      args[native->variadic ? i + 1 : i] =
          generateExpression(node->arguments[i]);
  }
  return LLVMBuildCall2(
      cg->builder, native->fnType, native->llvmFunction, args,
      native->variadic ? node->argumentCount + 1 : node->argumentCount, "");
}
