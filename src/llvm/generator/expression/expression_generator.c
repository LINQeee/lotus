#include "../expression/expression_generator.h"
#include <llvm-c/Core.h>

#include <stdio.h>
#include <stdlib.h>

#include "../../../utils/cutils.h"
#include "../../llvm.h"
#include "native_functions.h"

static LLVMValueRef generateNumber(const NumberNode *node) {
  switch (node->base.dataType) {
  case LOTUS_I32:
    return LLVMConstInt(LLVMInt32TypeInContext(cg->context), node->i32, 0);
  case LOTUS_I64:
    return LLVMConstInt(LLVMInt64TypeInContext(cg->context), node->i64, 0);
  case LOTUS_F32:
    return LLVMConstReal(LLVMFloatTypeInContext(cg->context), node->f32);
  case LOTUS_F64:
    return LLVMConstReal(LLVMDoubleTypeInContext(cg->context), node->f64);
  default:
    exitWithError("Unknown number datatype %d", node->base.dataType);
  }
}

static LLVMValueRef generateString(const StringNode *node) {
  return LLVMBuildGlobalStringPtr(cg->builder, node->value, "str");
}

static LLVMValueRef generateBool(const BooleanNode *node) {
  return LLVMConstInt(LLVMInt1TypeInContext(cg->context), node->value, 0);
}

static LLVMValueRef buildAdd(TokenType resultType, LLVMValueRef left,
                             LLVMValueRef right) {
  if (isNumericType(resultType))
    return LLVMBuildAdd(cg->builder, left, right, "add_tmp");
  // if (resultType == TOKEN_STRING)
  // return LLVMBuildGlobalStringPtr(cg->builder, left, right);
  // TODO: heap allocation, garbage collector for string struct {data: ptr, len:
  // int64} P.S. that's diabolical
}

static LLVMValueRef generateBinary(const BinaryNode *node) {
  LLVMValueRef left = generateExpression(node->left);
  LLVMValueRef right = generateExpression(node->right);

  switch (node->op) {
  case TOKEN_PLUS:
    return LLVMBuildAdd(cg->builder, left, right, "add_tmp");
  case TOKEN_MINUS:
    return LLVMBuildSub(cg->builder, left, right, "sub_tmp");
  case TOKEN_STAR:
    return LLVMBuildMul(cg->builder, left, right, "mul_tmp");
  case TOKEN_SLASH:
    return LLVMBuildSDiv(cg->builder, left, right, "div_tmp");
  case TOKEN_PERCENT:
    return LLVMBuildSRem(cg->builder, left, right, "rem_tmp");
  case TOKEN_LT:
    return LLVMBuildICmp(cg->builder, LLVMIntSLT, left, right, "cmp_lt_tmp");
  case TOKEN_GT:
    return LLVMBuildICmp(cg->builder, LLVMIntSGT, left, right, "cmp_gt_tmp");
  case TOKEN_LE:
    return LLVMBuildICmp(cg->builder, LLVMIntSLE, left, right, "cmp_le_tmp");
  case TOKEN_GE:
    return LLVMBuildICmp(cg->builder, LLVMIntSGE, left, right, "cmp_ge_tmp");
  case TOKEN_EQ:
    return LLVMBuildICmp(cg->builder, LLVMIntEQ, left, right, "cmp_eq_tmp");
  case TOKEN_NEQ:
    return LLVMBuildICmp(cg->builder, LLVMIntNE, left, right, "cmp_neq_tmp");
  case TOKEN_AND: {
    LLVMValueRef lhs = generateExpression(node->left);
    LLVMValueRef rhs = generateExpression(node->right);

    return LLVMBuildAnd(cg->builder, lhs, rhs, "andtmp");
  }
  case TOKEN_OR: {
    LLVMValueRef lhs = generateExpression(node->left);
    LLVMValueRef rhs = generateExpression(node->right);

    return LLVMBuildOr(cg->builder, lhs, rhs, "ortmp");
  }
  }
  return NULL;
}

static LLVMValueRef generateIdentifier(const char *name) {
  LLVMVariable *var = getVar(name);
  return LLVMBuildLoad2(cg->builder, convertType(var->type), var->alloca, name);
}

LLVMValueRef generateExpression(const Node *node) {
  switch (node->type) {
  case NODE_NUMBER:
    return generateNumber((NumberNode *)node);
  case NODE_BINARY:
    return generateBinary((BinaryNode *)node);
  case NODE_IDENTIFIER:
    return generateIdentifier(((IdentifierNode *)node)->name);
  case NODE_CALL:
    return generateCall((CallNode *)node);
  case NODE_STRING:
    return generateString((StringNode *)node);
  case NODE_BOOLEAN:
    return generateBool((BooleanNode *)node);
  }
  return NULL;
}
