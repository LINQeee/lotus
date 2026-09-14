#include "type_checker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../llvm/lotus_types.h"
#include "../utils/cutils.h"

#include "../parser/parser.h"
#include "diagnostics.h"
#include "type_rules.h"
#include "type_scope.h"

#define INITIAL_FUNCTION_CAPACITY 16

struct TypeChecker {
  const char *scriptContent;

  TypeScope *scope;

  TypeFunction *functions;
  size_t functionCount;
  size_t functionCapacity;

  LotusType currentReturnType;

  size_t loopDepth;
};

static TypeFunction *globalFunctions;
static size_t globalFunctionCount;
static size_t globalFunctionCapacity = INITIAL_FUNCTION_CAPACITY;

static void checkStatement(TypeChecker *checker, Node *node);

static LotusType checkExpression(TypeChecker *checker, Node *node);

static void checkProgramBody(TypeChecker *checker, const ProgramNode *program);

static TypeFunction *findFunction(const TypeChecker *checker, const char *name);

static void enterScope(TypeChecker *checker) {
  checker->scope = typeScopeCreate(checker->scope);
}

static void leaveScope(TypeChecker *checker) {
  TypeScope *parent = checker->scope->parent;

  typeScopeDestroy(checker->scope);
  checker->scope = parent;
}

static void declareVariable(const TypeChecker *checker, const char *name,
                            LotusType type, SourceLocation location) {
  for (size_t i = 0; i < checker->scope->count; i++) {
    if (strcmp(checker->scope->variables[i].name, name) == 0)
      diagnosticError(checker->scriptContent, location,
                      "variable '%s' is already declared", name);
  }

  typeScopeDeclare(checker->scope, name, type);
}

static Variable *findVariable(TypeChecker *checker, const char *name,
                              SourceLocation location) {
  Variable *variable = typeScopeFind(checker->scope, name);

  if (!variable)
    diagnosticError(checker->scriptContent, location,
                    "variable '%s' is not declared", name);

  return variable;
}

static void ensureFunctionCapacity(TypeChecker *checker) {
  if (checker != NULL && checker->functionCount >= checker->functionCapacity) {
    checker->functionCapacity *= 2;

    checker->functions = reallocSafe(
        checker->functions, sizeof(TypeFunction) * checker->functionCapacity);
  }

  if (globalFunctions == NULL)
    globalFunctions = mallocSafe(sizeof(TypeFunction) * globalFunctionCapacity);
  if (globalFunctionCount >= globalFunctionCapacity) {
    globalFunctionCapacity *= 2;

    globalFunctions = reallocSafe(globalFunctions, sizeof(TypeFunction) *
                                                       globalFunctionCapacity);
  }
}

static void addFunction(TypeChecker *checker, const char *name,
                        LotusType returnType, const LotusType *parameterTypes,
                        size_t parameterCount, bool variadic,
                        SourceLocation location, bool isGlobal) {
  if (findFunction(checker, name))
    diagnosticError(checker->scriptContent, location,
                    "function '%s' is already declared", name);

  ensureFunctionCapacity(checker);
  TypeFunction *function = isGlobal
                               ? &globalFunctions[globalFunctionCount++]
                               : &checker->functions[checker->functionCount++];

  function->name = strdup(name);
  function->returnType = returnType;
  function->parameterCount = parameterCount;
  function->variadic = variadic;

  if (parameterCount > 0) {
    function->parameterTypes = mallocSafe(sizeof(LotusType) * parameterCount);
    memcpy(function->parameterTypes, parameterTypes,
           sizeof(LotusType) * parameterCount);
  } else
    function->parameterTypes = NULL;
}

static TypeFunction *findFunction(const TypeChecker *checker,
                                  const char *name) {
  for (size_t i = 0; i < globalFunctionCount; i++) {
    if (strcmp(globalFunctions[i].name, name) == 0)
      return &globalFunctions[i];
  }
  if (checker == NULL)
    return NULL;
  for (size_t i = 0; i < checker->functionCount; i++) {
    if (strcmp(checker->functions[i].name, name) == 0)
      return &checker->functions[i];
  }
  return NULL;
}

TypeChecker *typeCheckerCreate(const char *fileContent) {
  TypeChecker *checker = mallocSafe(sizeof(TypeChecker));

  checker->scriptContent = fileContent;
  checker->scope = NULL;
  checker->functionCapacity = INITIAL_FUNCTION_CAPACITY;
  checker->functionCount = 0;
  checker->functions =
      mallocSafe(sizeof(TypeFunction) * checker->functionCapacity);
  checker->currentReturnType = LOTUS_VOID;
  checker->loopDepth = 0;

  enterScope(checker);

  return checker;
}

void typeCheckerDestroy(TypeChecker *checker) {
  if (!checker)
    return;

  while (checker->scope)
    leaveScope(checker);

  for (size_t i = 0; i < checker->functionCount; i++) {
    free(checker->functions[i].name);
    free(checker->functions[i].parameterTypes);
  }

  free(checker->functions);
  free(checker);
}

void typeCheckerAddNativeFunction(const char *name, LotusType returnType,
                                  const LotusType *parameterTypes,
                                  size_t parameterCount, bool variadic) {
  SourceLocation location = {"<native>", 1, 1, 1};

  addFunction(NULL, name, returnType, parameterTypes, parameterCount, variadic,
              location, true);
}

static void collectFunctions(TypeChecker *checker, const ProgramNode *program) {
  for (size_t i = 0; i < program->count; i++) {
    Node *node = program->statements[i];

    if (node->type != NODE_FUNCTION)
      continue;

    FunctionNode *function = (FunctionNode *)node;
    LotusType *parameterTypes = NULL;

    if (function->paramCount > 0) {
      parameterTypes = mallocSafe(sizeof(LotusType) * function->paramCount);

      for (int j = 0; j < function->paramCount; j++)
        parameterTypes[j] = function->params[j]->base.dataType;
    }

    addFunction(checker, function->name, function->base.dataType,
                parameterTypes, function->paramCount, false,
                function->base.location, false);

    free(parameterTypes);
  }
}

static LotusType checkIdentifier(TypeChecker *checker, IdentifierNode *node) {
  Variable *variable = findVariable(checker, node->name, node->base.location);
  node->base.dataType = variable->type;
  return variable->type;
}

static LotusType checkBinary(TypeChecker *checker, BinaryNode *node) {
  LotusType left = checkExpression(checker, node->left);

  LotusType right = checkExpression(checker, node->right);

  if (!isValidBinaryOperation(node->op, left, right)) {
    diagnosticError(checker->scriptContent, node->base.location,
                    "operator '%s' cannot be applied to %s and %s",
                    tokenTypeName(node->op), lotusTypeName(left),
                    lotusTypeName(right));
  }

  LotusType result = binaryResultType(node->op, left, right);
  node->base.dataType = result;

  return result;
}

static LotusType checkCall(TypeChecker *checker, CallNode *node) {
  TypeFunction *function = findFunction(checker, node->callee->name);

  if (!function)
    diagnosticError(checker->scriptContent, node->callee->base.location,
                    "function '%s' is not declared", node->callee->name);

  if (!function->variadic &&
      node->argumentCount != (int)function->parameterCount)
    diagnosticError(checker->scriptContent, node->base.location,
                    "function '%s' expects %zu argument%s, got %d",
                    function->name, function->parameterCount,
                    function->parameterCount == 1 ? "" : "s",
                    node->argumentCount);

  size_t fixedCount = function->parameterCount;

  if (function->variadic && (size_t)node->argumentCount < fixedCount)
    diagnosticError(checker->scriptContent, node->base.location,
                    "function '%s' expects at least %zu argument%s, got %d",
                    function->name, fixedCount, fixedCount == 1 ? "" : "s",
                    node->argumentCount);

  for (int i = 0; i < node->argumentCount; i++) {
    LotusType actual = checkExpression(checker, node->arguments[i]);

    size_t parameterIndex = (size_t)i;

    if (function->variadic && parameterIndex >= function->parameterCount) {
      if (function->parameterCount == 0)
        continue;

      parameterIndex = function->parameterCount - 1;
    }

    if (parameterIndex >= function->parameterCount)
      continue;

    LotusType expected = function->parameterTypes[parameterIndex];

    if (!isAssignable(expected, actual))
      diagnosticError(checker->scriptContent, node->arguments[i]->location,
                      "argument %d of '%s': expected %s, got %s", i + 1,
                      function->name, lotusTypeName(expected),
                      lotusTypeName(actual));
  }

  node->base.dataType = function->returnType;

  return function->returnType;
}

static LotusType checkExpression(TypeChecker *checker, Node *node) {
  switch (node->type) {
  case NODE_NUMBER:
    return node->dataType;

  case NODE_BOOLEAN:
    return LOTUS_BOOL;

  case NODE_STRING:
    return LOTUS_STRING;

  case NODE_IDENTIFIER:
    return checkIdentifier(checker, (IdentifierNode *)node);

  case NODE_BINARY:
    return checkBinary(checker, (BinaryNode *)node);

  case NODE_CALL:
    return checkCall(checker, (CallNode *)node);

  default:
    diagnosticError(checker->scriptContent, node->location,
                    "node cannot be used as an expression");
  }

  return LOTUS_ANY;
}

static void checkDeclaration(TypeChecker *checker, DeclarationNode *node) {
  LotusType valueType = checkExpression(checker, node->value);

  LotusType declaredType = node->target->base.dataType;

  if (!isAssignable(declaredType, valueType))
    diagnosticError(checker->scriptContent, node->value->location,
                    "cannot assign %s to %s", lotusTypeName(valueType),
                    lotusTypeName(declaredType));

  node->base.dataType = declaredType;
  node->target->base.dataType = declaredType;

  declareVariable(checker, node->target->name, declaredType,
                  node->target->base.location);
}

static void checkAssignment(TypeChecker *checker, AssignmentNode *node) {
  Variable *variable =
      findVariable(checker, node->target->name, node->target->base.location);

  LotusType valueType = checkExpression(checker, node->value);

  if (!isAssignable(variable->type, valueType))
    diagnosticError(checker->scriptContent, node->value->location,
                    "cannot assign %s to %s", lotusTypeName(valueType),
                    lotusTypeName(variable->type));

  node->target->base.dataType = variable->type;
  node->base.dataType = variable->type;
}

static void checkReturn(TypeChecker *checker, const ReturnNode *node) {
  if (!node->value) {
    if (checker->currentReturnType != LOTUS_VOID)
      diagnosticError(checker->scriptContent, node->base.location,
                      "expected return value of type %s",
                      lotusTypeName(checker->currentReturnType));
    return;
  }

  LotusType actual = checkExpression(checker, node->value);

  if (checker->currentReturnType == LOTUS_VOID)
    diagnosticError(checker->scriptContent, node->value->location,
                    "void function cannot return a value");

  if (!isAssignable(checker->currentReturnType, actual))
    diagnosticError(checker->scriptContent, node->value->location,
                    "cannot return %s from function returning %s",
                    lotusTypeName(actual),
                    lotusTypeName(checker->currentReturnType));
}

static void checkCondition(TypeChecker *checker, Node *condition,
                           const char *construct) {
  LotusType type = checkExpression(checker, condition);

  if (type != LOTUS_BOOL)
    diagnosticError(checker->scriptContent, condition->location,
                    "%s condition must be bool, got %s", construct,
                    lotusTypeName(type));
}

static void checkIf(TypeChecker *checker, const IfNode *node) {
  checkCondition(checker, node->condition, "if");

  checkStatement(checker, node->thenBranch);

  if (node->elseBranch)
    checkStatement(checker, node->elseBranch);
}

static void checkWhile(TypeChecker *checker, const WhileNode *node) {
  checkCondition(checker, node->condition, "while");
  checker->loopDepth++;
  checkStatement(checker, node->body);
  checker->loopDepth--;
}

static void checkBreak(const TypeChecker *checker, const BreakNode *node) {
  if (checker->loopDepth == 0)
    diagnosticError(checker->scriptContent, node->base.location,
                    "'break' cannot be used outside a loop");

  if (node->level <= 0)
    diagnosticError(checker->scriptContent, node->base.location,
                    "break level must be greater than zero");

  if ((size_t)node->level > checker->loopDepth)
    diagnosticError(checker->scriptContent, node->base.location,
                    "break @%d exceeds loop depth %zu", node->level,
                    checker->loopDepth);
}

static void checkContinue(TypeChecker *checker, const ContinueNode *node) {
  if (checker->loopDepth == 0)
    diagnosticError(checker->scriptContent, node->base.location,
                    "'continue' cannot be used outside a loop");

  if (node->level <= 0)
    diagnosticError(checker->scriptContent, node->base.location,
                    "continue level must be greater than zero");

  if ((size_t)node->level > checker->loopDepth)
    diagnosticError(checker->scriptContent, node->base.location,
                    "continue @%d exceeds loop depth %zu", node->level,
                    checker->loopDepth);
}

static void checkBlock(TypeChecker *checker, ProgramNode *program) {
  enterScope(checker);
  checkProgramBody(checker, program);
  leaveScope(checker);
}

static void checkFunction(TypeChecker *checker, FunctionNode *node) {
  TypeFunction *function = findFunction(checker, node->name);

  if (!function)
    diagnosticError(checker->scriptContent, node->base.location,
                    "internal error: function '%s' is not registered",
                    node->name);

  LotusType previousReturnType = checker->currentReturnType;
  size_t previousLoopDepth = checker->loopDepth;

  checker->currentReturnType = node->base.dataType;
  checker->loopDepth = 0;
  enterScope(checker);

  for (int i = 0; i < node->paramCount; i++) {
    IdentifierNode *parameter = node->params[i];
    declareVariable(checker, parameter->name, parameter->base.dataType,
                    parameter->base.location);
  }

  checkProgramBody(checker, (ProgramNode *)node->body);

  leaveScope(checker);
  checker->currentReturnType = previousReturnType;
  checker->loopDepth = previousLoopDepth;
}

static void checkStatement(TypeChecker *checker, Node *node) {
  switch (node->type) {
  case NODE_PROGRAM:
    checkBlock(checker, (ProgramNode *)node);
    break;

  case NODE_DECLARATION:
    checkDeclaration(checker, (DeclarationNode *)node);
    break;

  case NODE_ASSIGN:
    checkAssignment(checker, (AssignmentNode *)node);
    break;

  case NODE_RETURN:
    checkReturn(checker, (ReturnNode *)node);
    break;

  case NODE_IF:
    checkIf(checker, (IfNode *)node);
    break;

  case NODE_WHILE:
    checkWhile(checker, (WhileNode *)node);
    break;

  case NODE_BREAK:
    checkBreak(checker, (BreakNode *)node);
    break;

  case NODE_CONTINUE:
    checkContinue(checker, (ContinueNode *)node);
    break;

  case NODE_FUNCTION:
    checkFunction(checker, (FunctionNode *)node);
    break;

  case NODE_CALL:
  case NODE_BINARY:
  case NODE_IDENTIFIER:
  case NODE_NUMBER:
  case NODE_STRING:
    checkExpression(checker, node);
    break;

  default:
    diagnosticError(checker->scriptContent, node->location,
                    "unsupported AST node in type checker");
  }
}

static void checkProgramBody(TypeChecker *checker, const ProgramNode *program) {
  for (size_t i = 0; i < program->count; i++)
    checkStatement(checker, program->statements[i]);
}

void typeCheckerCheckProgram(TypeChecker *checker, const ProgramNode *program) {
  collectFunctions(checker, program);

  checkProgramBody(checker, program);
}
