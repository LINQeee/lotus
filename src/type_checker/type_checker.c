#include "type_checker.h"

#include <stdlib.h>
#include <string.h>

#include "../parser/objects.h"
#include "../llvm/types.h"
#include "../utils/cutils.h"
#include "../parser/parser.h"
#include "diagnostics.h"
#include "type_rules.h"
#include "type_scope.h"
#include "../lexer/lexer.h"

#define INITIAL_FUNCTION_CAPACITY 16

struct TypeChecker {
    const char *scriptContent;

    TypeScope *scope;

    TypeFunction *functions;
    size_t functionCount;
    size_t functionCapacity;

    Type *currentReturnType;

    size_t loopDepth;
};

static TypeFunction *globalFunctions;
static size_t globalFunctionCount;
static size_t globalFunctionCapacity = INITIAL_FUNCTION_CAPACITY;

static void checkStatement(TypeChecker *checker, Node *node);

static Type *checkExpression(TypeChecker *checker, Node *node);

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

static void declareVariable(const TypeChecker *checker, const char *name, Type *type, SourceLocation location) {
    for (size_t i = 0; i < checker->scope->count; i++) {
        if (strcmp(checker->scope->variables[i].name, name) == 0)
            diagnosticError(checker->scriptContent, location, "variable '%s' is already declared", name);
    }

    typeScopeDeclare(checker->scope, name, type);
}

static Variable *findVariable(TypeChecker *checker, const char *name, SourceLocation location) {
    Variable *variable = typeScopeFind(checker->scope, name);

    if (!variable) diagnosticError(checker->scriptContent, location, "variable '%s' is not declared", name);

    return variable;
}

static void ensureFunctionCapacity(TypeChecker *checker) {
    if (checker != NULL && checker->functionCount >= checker->functionCapacity) {
        checker->functionCapacity *= 2;

        checker->functions = reallocSafe(checker->functions, sizeof(TypeFunction) * checker->functionCapacity);
    }

    if (globalFunctions == NULL) globalFunctions = mallocSafe(sizeof(TypeFunction) * globalFunctionCapacity);
    if (globalFunctionCount >= globalFunctionCapacity) {
        globalFunctionCapacity *= 2;

        globalFunctions = reallocSafe(globalFunctions, sizeof(TypeFunction) * globalFunctionCapacity);
    }
}

static void addFunction(TypeChecker *checker, const char *name, Type *returnType, Type *const *parameterTypes,
                        size_t parameterCount, bool variadic, SourceLocation location, bool isGlobal) {
    if (findFunction(checker, name))
        diagnosticError(checker->scriptContent, location, "function '%s' is already declared", name);

    ensureFunctionCapacity(checker);
    TypeFunction *function =
        isGlobal ? &globalFunctions[globalFunctionCount++] : &checker->functions[checker->functionCount++];

    function->name = strdup(name);
    function->returnType = returnType;
    function->parameterCount = parameterCount;
    function->variadic = variadic;

    if (parameterCount > 0) {
        function->parameterTypes = mallocSafe(sizeof(Type *) * parameterCount);
        memcpy(function->parameterTypes, parameterTypes, sizeof(Type *) * parameterCount);
    } else function->parameterTypes = NULL;
}

static TypeFunction *findFunction(const TypeChecker *checker, const char *name) {
    for (size_t i = 0; i < globalFunctionCount; i++) {
        if (strcmp(globalFunctions[i].name, name) == 0) return &globalFunctions[i];
    }
    if (checker == NULL) return NULL;
    for (size_t i = 0; i < checker->functionCount; i++) {
        if (strcmp(checker->functions[i].name, name) == 0) return &checker->functions[i];
    }
    return NULL;
}

TypeChecker *typeCheckerCreate(const char *fileContent) {
    TypeChecker *checker = mallocSafe(sizeof(TypeChecker));

    checker->scriptContent = fileContent;
    checker->scope = NULL;
    checker->functionCapacity = INITIAL_FUNCTION_CAPACITY;
    checker->functionCount = 0;
    checker->functions = mallocSafe(sizeof(TypeFunction) * checker->functionCapacity);
    checker->currentReturnType = unknownType;
    checker->loopDepth = 0;

    enterScope(checker);

    return checker;
}

void typeCheckerDestroy(TypeChecker *checker) {
    if (!checker) return;

    while (checker->scope) leaveScope(checker);

    for (size_t i = 0; i < checker->functionCount; i++) {
        free(checker->functions[i].name);
        free(checker->functions[i].parameterTypes);
    }

    free(checker->functions);
    free(checker);
}

void typeCheckerAddNativeFunction(const char *name, Type *returnType, Type *const *parameterTypes,
                                  size_t parameterCount, bool variadic) {
    SourceLocation location = {"<native>", 1, 1, 1};

    addFunction(NULL, name, returnType, parameterTypes, parameterCount, variadic, location, true);
}

static void collectFunctions(TypeChecker *checker, const ProgramNode *program) {
    for (size_t i = 0; i < program->count; i++) {
        Node *node = program->statements[i];

        if (node->type != NODE_FUNCTION) continue;

        FunctionNode *function = (FunctionNode *)node;
        if (function->paramCount > 32)
            diagnosticError(checker->scriptContent, node->location, "functions support at most 32 parameters");
        if (checker->functionCount >= 256)
            diagnosticError(checker->scriptContent, node->location, "too many functions");
        Type **parameterTypes = NULL;

        if (function->paramCount > 0) {
            parameterTypes = mallocSafe(sizeof(Type *) * function->paramCount);

            for (int j = 0; j < function->paramCount; j++) parameterTypes[j] = function->params[j]->base.dataType;
        }

        addFunction(checker, function->name, function->base.dataType, parameterTypes, function->paramCount, false,
                    function->base.location, false);

        free(parameterTypes);
    }
}

static Type *checkIdentifier(TypeChecker *checker, IdentifierNode *node) {
    Variable *variable = findVariable(checker, node->name, node->base.location);
    node->base.dataType = variable->type;
    return variable->type;
}

static Type *checkExpected(TypeChecker *checker, Node *node, Type *expected) {
    if (node->type == NODE_ARRAY && !((ArrayNode *)node)->size && isArrayType(expected)) node->dataType = expected;
    if (node->type == NODE_OBJECT && node->dataType == unknownType && isClassType(expected)) node->dataType = expected;
    return checkExpression(checker, node);
}

static Type *checkArray(TypeChecker *checker, ArrayNode *node) {
    if (node->size) {
        if (checkExpression(checker, node->size) != intType)
            diagnosticError(checker->scriptContent, node->size->location, "array size must be int");
        return node->base.dataType;
    }
    Type *elementType = isArrayType(node->base.dataType) ? fromArrayType(node->base.dataType) : unknownType;
    for (size_t i = 0; i < node->itemsCount; i++) {
        Type *actual = checkExpression(checker, node->items[i]);
        if (elementType == unknownType) elementType = actual;
        bool scalar = isNumericType(actual) || actual == boolType || actual == stringType;
        if (!scalar || !typesEqual(elementType, actual))
            diagnosticError(checker->scriptContent, node->items[i]->location,
                            "array elements must have the same scalar type");
    }
    if (elementType == unknownType)
        diagnosticError(checker->scriptContent, node->base.location, "empty array requires an array type");
    return node->base.dataType = toArrayType(elementType);
}

static Type *checkIndex(TypeChecker *checker, IndexNode *node) {
    Type *array = checkExpression(checker, node->array);
    if (!isArrayType(array)) diagnosticError(checker->scriptContent, node->base.location, "expected an array");
    if (checkExpression(checker, node->index) != intType)
        diagnosticError(checker->scriptContent, node->index->location, "array index must be int");
    Type *elementType = fromArrayType(array);
    if (node->value && !typesEqual(checkExpression(checker, node->value), elementType))
        diagnosticError(checker->scriptContent, node->value->location, "array element requires %s",
                        typeName(elementType));
    return node->base.dataType = elementType;
}

static Type *checkLength(TypeChecker *checker, LengthNode *node) {
    if (!isArrayType(checkExpression(checker, node->array)))
        diagnosticError(checker->scriptContent, node->base.location, "expected an array");
    return node->base.dataType;
}

static Type *checkBinary(TypeChecker *checker, BinaryNode *node) {
    Type *left = checkExpression(checker, node->left);

    Type *right = checkExpression(checker, node->right);

    if (!isValidBinaryOperation(node->op, left, right)) {
        diagnosticError(checker->scriptContent, node->base.location, "operator '%s' cannot be applied to %s and %s",
                        tokenTypeName(node->op), typeName(left), typeName(right));
    }

    Type *result = binaryResultType(node->op, left, right);
    node->base.dataType = result;

    return result;
}

static Type *checkCall(TypeChecker *checker, CallNode *node) {
    if (!strcmp(node->callee->name, "_arrayCreate") || !strcmp(node->callee->name, "_arrayAt") ||
        !strcmp(node->callee->name, "_arrayLength") || !strcmp(node->callee->name, "_arrayRetain") ||
        !strcmp(node->callee->name, "_arrayRelease"))
        diagnosticError(checker->scriptContent, node->base.location, "array runtime functions are internal");
    if (strcmp(node->callee->name, "_stringRetain") == 0 || strcmp(node->callee->name, "_stringRelease") == 0)
        diagnosticError(checker->scriptContent, node->base.location, "string lifetime is managed automatically");

    TypeFunction *function = findFunction(checker, node->callee->name);

    if (!function)
        diagnosticError(checker->scriptContent, node->callee->base.location, "function '%s' is not declared",
                        node->callee->name);

    if (!function->variadic && node->argumentCount != (int)function->parameterCount)
        diagnosticError(checker->scriptContent, node->base.location, "function '%s' expects %zu argument%s, got %d",
                        function->name, function->parameterCount, function->parameterCount == 1 ? "" : "s",
                        node->argumentCount);

    size_t fixedCount = function->parameterCount;

    if (function->variadic && (size_t)node->argumentCount < fixedCount)
        diagnosticError(checker->scriptContent, node->base.location,
                        "function '%s' expects at least %zu argument%s, got %d", function->name, fixedCount,
                        fixedCount == 1 ? "" : "s", node->argumentCount);

    for (int i = 0; i < node->argumentCount; i++) {
        Type *expectedType = i < (int)function->parameterCount ? function->parameterTypes[i] : anyType;
        Type *actual = checkExpected(checker, node->arguments[i], expectedType);
        if (expectedType == anyType && (isArrayType(actual) || isClassType(actual)))
            diagnosticError(checker->scriptContent, node->arguments[i]->location,
                            isClassType(actual) ? "objects cannot be passed as any" : "arrays cannot be passed as any");

        size_t parameterIndex = (size_t)i;

        if (function->variadic && parameterIndex >= function->parameterCount) {
            if (function->parameterCount == 0) continue;

            parameterIndex = function->parameterCount - 1;
        }

        if (parameterIndex >= function->parameterCount) continue;

        Type *expected = function->parameterTypes[parameterIndex];

        if (!isAssignable(expected, actual))
            diagnosticError(checker->scriptContent, node->arguments[i]->location,
                            "argument %d of '%s': expected %s, got %s", i + 1, function->name, typeName(expected),
                            typeName(actual));
    }

    node->base.dataType = function->returnType;

    return function->returnType;
}

static Type *checkObject(TypeChecker *checker, ObjectNode *node) {
    if (!isClassType(node->base.dataType))
        diagnosticError(checker->scriptContent, node->base.location, "object literal requires an object type");
    ClassNode *type = getClass(node->base.dataType);
    for (size_t i = 0; i < node->count; i++) {
        size_t index = 0;
        while (index < type->fieldCount && strcmp(type->fields[index].name, node->names[i])) index++;
        if (index == type->fieldCount)
            diagnosticError(checker->scriptContent, node->values[i]->location, "unknown field '%s' in %s",
                            node->names[i], type->name);
        for (size_t j = 0; j < i; j++)
            if (node->indices[j] == index)
                diagnosticError(checker->scriptContent, node->values[i]->location, "duplicate field '%s'",
                                node->names[i]);
        node->indices[i] = index;
        Type *actual = checkExpected(checker, node->values[i], type->fields[index].type);
        if (!typesEqual(actual, type->fields[index].type))
            diagnosticError(checker->scriptContent, node->values[i]->location, "field '%s' requires %s, got %s",
                            node->names[i], typeName(type->fields[index].type), typeName(actual));
    }
    for (size_t i = 0; i < type->fieldCount; i++) {
        bool found = false;
        for (size_t j = 0; j < node->count; j++)
            if (node->indices[j] == i) found = true;
        if (!found && !type->fields[i].defaultValue)
            diagnosticError(checker->scriptContent, node->base.location, "missing field '%s' in %s",
                            type->fields[i].name, type->name);
    }
    return node->base.dataType;
}
static Type *checkMember(TypeChecker *checker, MemberNode *node) {
    Type *owner = checkExpression(checker, node->object);
    if (isArrayType(owner) && !strcmp(node->name, "length")) {
        if (node->value) diagnosticError(checker->scriptContent, node->base.location, "array length is read-only");
        return node->base.dataType = intType;
    }
    if (!isClassType(owner))
        diagnosticError(checker->scriptContent, node->base.location, "type %s has no field '%s'", typeName(owner),
                        node->name);
    ClassNode *type = getClass(owner);
    size_t index = 0;
    while (index < type->fieldCount && strcmp(type->fields[index].name, node->name)) index++;
    if (index == type->fieldCount)
        diagnosticError(checker->scriptContent, node->base.location, "unknown field '%s' in %s", node->name,
                        type->name);
    node->index = index;
    node->base.dataType = type->fields[index].type;
    if (node->value) {
        Type *actual = checkExpected(checker, node->value, node->base.dataType);
        if (!typesEqual(actual, node->base.dataType))
            diagnosticError(checker->scriptContent, node->value->location, "field '%s' requires %s, got %s", node->name,
                            typeName(node->base.dataType), typeName(actual));
        if (node->op != TOKEN_EQUAL) {
            TokenKind op;
            switch (node->op) {
                case TOKEN_EQ_PLUS: op = TOKEN_PLUS; break;
                case TOKEN_EQ_MINUS: op = TOKEN_MINUS; break;
                case TOKEN_EQ_STAR: op = TOKEN_STAR; break;
                case TOKEN_EQ_SLASH: op = TOKEN_SLASH; break;
                case TOKEN_EQ_PERCENT: op = TOKEN_PERCENT; break;
                default: op = TOKEN_EOF; break;
            }
            if (!isValidBinaryOperation(op, actual, actual))
                diagnosticError(checker->scriptContent, node->base.location, "invalid compound assignment to '%s'",
                                node->name);
        }
    }
    return node->base.dataType;
}
static Type *checkMethodCall(TypeChecker *checker, MethodCallNode *node) {
    Type *owner = checkExpression(checker, node->member->object);
    if (!isClassType(owner))
        diagnosticError(checker->scriptContent, node->base.location, "method call requires an object");
    ClassNode *type = getClass(owner);
    FunctionNode *method = NULL;
    for (size_t i = 0; i < type->methodCount; i++)
        if (!strcmp(strchr(type->methods[i]->name, '.') + 1, node->member->name)) method = type->methods[i];
    if (!method)
        diagnosticError(checker->scriptContent, node->base.location, "unknown method '%s' in %s", node->member->name,
                        type->name);
    strcpy(node->call->callee->name, method->name);
    return node->base.dataType = checkCall(checker, node->call);
}

static Type *checkExpression(TypeChecker *checker, Node *node) {
    switch (node->type) {
        case NODE_OBJECT: return checkObject(checker, (ObjectNode *)node);
        case NODE_MEMBER: return checkMember(checker, (MemberNode *)node);
        case NODE_METHOD_CALL: return checkMethodCall(checker, (MethodCallNode *)node);
        case NODE_ARRAY: return checkArray(checker, (ArrayNode *)node);
        case NODE_LENGTH: return checkLength(checker, (LengthNode *)node);
        case NODE_INDEX:
        case NODE_INDEX_ASSIGN: return checkIndex(checker, (IndexNode *)node);
        case NODE_NUMBER: return node->dataType;

        case NODE_BOOLEAN: return boolType;

        case NODE_STRING: return stringType;

        case NODE_IDENTIFIER: return checkIdentifier(checker, (IdentifierNode *)node);

        case NODE_UNARY: {
            UnaryNode *unary = (UnaryNode *)node;
            Type *type = checkExpression(checker, unary->operand);
            if (unary->op == TOKEN_INCREMENT || unary->op == TOKEN_DECREMENT) {
                if (unary->operand->type != NODE_IDENTIFIER || !isNumericType(type))
                    diagnosticError(checker->scriptContent, node->location, "'++' and '--' require a numeric variable");
            } else if (unary->op == TOKEN_EXCLAMATION) {
                if (type != boolType)
                    diagnosticError(checker->scriptContent, node->location, "unary '!' requires bool, got %s",
                                    typeName(type));
            } else if (!isNumericType(type))
                diagnosticError(checker->scriptContent, node->location, "unary '-' requires a number, got %s",
                                typeName(type));
            node->dataType = type;
            return type;
        }

        case NODE_CAST: {
            CastNode *cast = (CastNode *)node;
            Type *source = checkExpression(checker, cast->operand);
            if (!isValidCast(node->dataType, source))
                diagnosticError(checker->scriptContent, node->location, "cannot cast %s to %s", typeName(source),
                                typeName(node->dataType));
            return node->dataType;
        }

        case NODE_BINARY: return checkBinary(checker, (BinaryNode *)node);

        case NODE_CALL: return checkCall(checker, (CallNode *)node);

        default: diagnosticError(checker->scriptContent, node->location, "node cannot be used as an expression");
    }
}

static void checkDeclaration(TypeChecker *checker, DeclarationNode *node) {
    Type *declaredType = node->target->base.dataType;
    Type *valueType = checkExpected(checker, node->value, declaredType);

    if (!isAssignable(declaredType, valueType))
        diagnosticError(checker->scriptContent, node->value->location, "cannot assign %s to %s", typeName(valueType),
                        typeName(declaredType));

    node->base.dataType = declaredType;
    node->target->base.dataType = declaredType;

    declareVariable(checker, node->target->name, declaredType, node->target->base.location);
}

static void checkAssignment(TypeChecker *checker, AssignmentNode *node) {
    Variable *variable = findVariable(checker, node->target->name, node->target->base.location);

    Type *valueType = checkExpected(checker, node->value, variable->type);

    if (!isAssignable(variable->type, valueType))
        diagnosticError(checker->scriptContent, node->value->location, "cannot assign %s to %s", typeName(valueType),
                        typeName(variable->type));

    node->target->base.dataType = variable->type;
    node->base.dataType = variable->type;
}

static void checkReturn(TypeChecker *checker, const ReturnNode *node) {
    if (checker->currentReturnType == unknownType)
        diagnosticError(checker->scriptContent, node->base.location, "return is only allowed inside a function");

    if (!node->value) {
        if (checker->currentReturnType != voidType)
            diagnosticError(checker->scriptContent, node->base.location, "expected return value of type %s",
                            typeName(checker->currentReturnType));
        return;
    }

    Type *actual = checkExpected(checker, node->value, checker->currentReturnType);

    if (checker->currentReturnType == voidType)
        diagnosticError(checker->scriptContent, node->value->location, "void function cannot return a value");

    if (!isAssignable(checker->currentReturnType, actual))
        diagnosticError(checker->scriptContent, node->value->location, "cannot return %s from function returning %s",
                        typeName(actual), typeName(checker->currentReturnType));
}

static void checkCondition(TypeChecker *checker, Node *condition, const char *construct) {
    Type *type = checkExpression(checker, condition);

    if (type != boolType)
        diagnosticError(checker->scriptContent, condition->location, "%s condition must be bool, got %s", construct,
                        typeName(type));
}

static void checkScopedStatement(TypeChecker *checker, Node *node) {
    enterScope(checker);
    checkStatement(checker, node);
    leaveScope(checker);
}

static void checkIf(TypeChecker *checker, const IfNode *node) {
    checkCondition(checker, node->condition, "if");

    checkScopedStatement(checker, node->thenBranch);

    if (node->elseBranch) checkScopedStatement(checker, node->elseBranch);
}

static void checkFor(TypeChecker *checker, ForNode *node) {
    Type *type = checkExpression(checker, node->start);
    if (!node->end) {
        if (!isArrayType(type))
            diagnosticError(checker->scriptContent, node->start->location, "for requires an array or range");
        enterScope(checker);
        node->variable->base.dataType = fromArrayType(type);
        declareVariable(checker, node->variable->name, node->variable->base.dataType, node->variable->base.location);
        checker->loopDepth++;
        checkStatement(checker, node->body);
        checker->loopDepth--;
        leaveScope(checker);
        return;
    }
    Type *endType = checkExpression(checker, node->end);
    if (!isIntegerType(type) || !typesEqual(type, endType))
        diagnosticError(checker->scriptContent, node->base.location,
                        "for bounds must have the same integer type (int or long)");
    if (node->step) {
        Type *stepType = checkExpression(checker, node->step);
        if (node->step->type != NODE_NUMBER || !typesEqual(stepType, type))
            diagnosticError(checker->scriptContent, node->step->location,
                            "for step must be a nonzero integer literal of the range type");
        const NumberNode *step = (const NumberNode *)node->step;
        if ((type == intType ? step->i32 : step->i64) == 0)
            diagnosticError(checker->scriptContent, node->step->location, "for step must not be zero");
    }
    enterScope(checker);
    node->variable->base.dataType = type;
    declareVariable(checker, node->variable->name, type, node->variable->base.location);
    checker->loopDepth++;
    checkStatement(checker, node->body);
    checker->loopDepth--;
    leaveScope(checker);
}

static void checkWhile(TypeChecker *checker, const WhileNode *node) {
    checkCondition(checker, node->condition, "while");
    checker->loopDepth++;
    checkScopedStatement(checker, node->body);
    checker->loopDepth--;
}

static void checkBreak(const TypeChecker *checker, const BreakNode *node) {
    if (checker->loopDepth == 0)
        diagnosticError(checker->scriptContent, node->base.location, "'break' cannot be used outside a loop");

    if (node->level <= 0)
        diagnosticError(checker->scriptContent, node->base.location, "break level must be greater than zero");

    if ((size_t)node->level > checker->loopDepth)
        diagnosticError(checker->scriptContent, node->base.location, "break @%d exceeds loop depth %zu", node->level,
                        checker->loopDepth);
}

static void checkContinue(TypeChecker *checker, const ContinueNode *node) {
    if (checker->loopDepth == 0)
        diagnosticError(checker->scriptContent, node->base.location, "'continue' cannot be used outside a loop");

    if (node->level <= 0)
        diagnosticError(checker->scriptContent, node->base.location, "continue level must be greater than zero");

    if ((size_t)node->level > checker->loopDepth)
        diagnosticError(checker->scriptContent, node->base.location, "continue @%d exceeds loop depth %zu", node->level,
                        checker->loopDepth);
}

static void checkBlock(TypeChecker *checker, ProgramNode *program) {
    checkProgramBody(checker, program);
}

typedef struct ReturnLoop {
    struct ReturnLoop *parent;
    bool hasBreak;
} ReturnLoop;

static bool canFallThrough(const Node *node, ReturnLoop *loop) {
    switch (node->type) {
        case NODE_RETURN:
        case NODE_CONTINUE: return false;

        case NODE_BREAK: {
            const BreakNode *jump = (const BreakNode *)node;
            for (int level = 1; level < jump->level; level++) loop = loop->parent;
            loop->hasBreak = true;
            return false;
        }

        case NODE_PROGRAM: {
            const ProgramNode *program = (const ProgramNode *)node;
            for (size_t i = 0; i < program->count; i++) {
                if (!canFallThrough(program->statements[i], loop)) return false;
            }
            return true;
        }

        case NODE_IF: {
            const IfNode *branch = (const IfNode *)node;
            if (branch->condition->type == NODE_BOOLEAN) {
                const BooleanNode *condition = (const BooleanNode *)branch->condition;
                if (condition->value) return canFallThrough(branch->thenBranch, loop);
                return branch->elseBranch ? canFallThrough(branch->elseBranch, loop) : true;
            }
            bool thenFallsThrough = canFallThrough(branch->thenBranch, loop);
            bool elseFallsThrough = branch->elseBranch ? canFallThrough(branch->elseBranch, loop) : true;
            return thenFallsThrough || elseFallsThrough;
        }

        case NODE_FOR: {
            const ForNode *cycle = (const ForNode *)node;
            ReturnLoop inner = {loop, false};
            canFallThrough(cycle->body, &inner);
            // A range can be empty; a return inside it does not prove a function returns.
            return true;
        }

        case NODE_WHILE: {
            const WhileNode *cycle = (const WhileNode *)node;
            const BooleanNode *condition =
                cycle->condition->type == NODE_BOOLEAN ? (const BooleanNode *)cycle->condition : NULL;
            if (condition && !condition->value) return true;
            ReturnLoop inner = {loop, false};
            canFallThrough(cycle->body, &inner);
            return !condition || inner.hasBreak;
        }

        default: return true;
    }
}

static void checkFunction(TypeChecker *checker, FunctionNode *node) {
    if (checker->scope->parent)
        diagnosticError(checker->scriptContent, node->base.location,
                        "functions can only be declared at top level; nested functions and closures are not supported");

    TypeFunction *function = findFunction(checker, node->name);

    if (!function)
        diagnosticError(checker->scriptContent, node->base.location, "internal error: function '%s' is not registered",
                        node->name);

    Type *previousReturnType = checker->currentReturnType;
    size_t previousLoopDepth = checker->loopDepth;

    checker->currentReturnType = node->base.dataType;
    checker->loopDepth = 0;
    enterScope(checker);

    for (int i = 0; i < node->paramCount; i++) {
        IdentifierNode *parameter = node->params[i];
        declareVariable(checker, parameter->name, parameter->base.dataType, parameter->base.location);
    }

    checkStatement(checker, node->body);
    if (node->base.dataType != voidType && canFallThrough(node->body, NULL))
        diagnosticError(checker->scriptContent, node->base.location,
                        "function '%s' can reach its end without returning %s", node->name,
                        typeName(node->base.dataType));

    leaveScope(checker);
    checker->currentReturnType = previousReturnType;
    checker->loopDepth = previousLoopDepth;
}

static void checkStatement(TypeChecker *checker, Node *node) {
    switch (node->type) {
        case NODE_CLASS:
            if (checker->scope->parent || checker->currentReturnType != unknownType)
                diagnosticError(checker->scriptContent, node->location, "classes can only be declared at top level");
            break;
        case NODE_PROGRAM: checkBlock(checker, (ProgramNode *)node); break;

        case NODE_DECLARATION: checkDeclaration(checker, (DeclarationNode *)node); break;

        case NODE_ASSIGN: checkAssignment(checker, (AssignmentNode *)node); break;

        case NODE_RETURN: checkReturn(checker, (ReturnNode *)node); break;

        case NODE_IF: checkIf(checker, (IfNode *)node); break;

        case NODE_FOR: checkFor(checker, (ForNode *)node); break;

        case NODE_WHILE: checkWhile(checker, (WhileNode *)node); break;

        case NODE_BREAK: checkBreak(checker, (BreakNode *)node); break;

        case NODE_CONTINUE: checkContinue(checker, (ContinueNode *)node); break;

        case NODE_FUNCTION: checkFunction(checker, (FunctionNode *)node); break;

        case NODE_OBJECT:
        case NODE_MEMBER:
        case NODE_METHOD_CALL:
        case NODE_ARRAY:
        case NODE_INDEX:
        case NODE_LENGTH:
        case NODE_INDEX_ASSIGN:
        case NODE_CALL:
        case NODE_UNARY:
        case NODE_BINARY:
        case NODE_IDENTIFIER:
        case NODE_NUMBER:
        case NODE_BOOLEAN:
        case NODE_STRING: checkExpression(checker, node); break;

        default: diagnosticError(checker->scriptContent, node->location, "unsupported AST node in type checker");
    }
}

static void checkProgramBody(TypeChecker *checker, const ProgramNode *program) {
    for (size_t i = 0; i < program->count; i++) {
        Node *node = program->statements[i];
        if (node->type == NODE_FUNCTION && ((FunctionNode *)node)->name[0] == '$') continue;
        checkStatement(checker, node);
    }
}

static void checkClass(TypeChecker *checker, ClassNode *type) {
    if (type->checkState == CLASS_CHECKED) return;
    if (type->checkState == CLASS_CHECKING)
        diagnosticError(checker->scriptContent, type->base.location, "recursive object fields are not supported: %s",
                        type->name);
    type->checkState = CLASS_CHECKING;
    for (size_t i = 0; i < type->fieldCount; i++) {
        ClassField *field = &type->fields[i];
        for (size_t j = 0; j < i; j++)
            if (!strcmp(field->name, type->fields[j].name))
                diagnosticError(checker->scriptContent, field->location, "duplicate field '%s'", field->name);
        for (size_t j = 0; j < type->methodCount; j++)
            if (!strcmp(field->name, strchr(type->methods[j]->name, '.') + 1))
                diagnosticError(checker->scriptContent, field->location, "field and method share name '%s'",
                                field->name);
        if (isClassType(field->type)) checkClass(checker, getClass(field->type));
    }
    type->checkState = CLASS_CHECKED;
}

void typeCheckerCheckProgram(TypeChecker *checker, const ProgramNode *program) {
    collectFunctions(checker, program);
    for (size_t i = 0; i < classCount; i++) checkClass(checker, classes[i]);
    for (size_t i = 0; i < classCount; i++)
        for (size_t j = 0; j < classes[i]->fieldCount; j++)
            if (classes[i]->fields[j].initializer) checkFunction(checker, classes[i]->fields[j].initializer);

    checkProgramBody(checker, program);
}
