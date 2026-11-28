#include "../type_checker/diagnostics.h"
#include "objects.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lexer/lexer.h"
#include "../utils/cutils.h"
#include "parser.h"
#include "../llvm/types.h"

bool isComparison(TokenKind type) {
    return type == TOKEN_GT || type == TOKEN_LT || type == TOKEN_GE || type == TOKEN_LE || type == TOKEN_EQ ||
           type == TOKEN_NEQ;
}

bool isAssignmentOp(TokenKind type) {
    return type == TOKEN_EQUAL || type == TOKEN_EQ_PLUS || type == TOKEN_EQ_MINUS || type == TOKEN_EQ_PERCENT ||
           type == TOKEN_EQ_STAR || type == TOKEN_EQ_SLASH;
}

Type *tokenTypeToType(TokenKind type) {
    switch (type) {
        case TOKEN_TYPE_INT: return intType;
        case TOKEN_TYPE_FLOAT: return floatType;
        case TOKEN_TYPE_BOOL: return boolType;
        case TOKEN_TYPE_LONG: return longType;
        case TOKEN_TYPE_DOUBLE: return doubleType;
        case TOKEN_TYPE_STRING: return stringType;
        default: printf("Unknown LOTUS DATATYPE %d\n", type); exit(EXIT_FAILURE);
    }
}

bool isDataType(TokenKind type) {
    return type == TOKEN_TYPE_INT || type == TOKEN_TYPE_STRING || type == TOKEN_TYPE_BOOL || type == TOKEN_TYPE_FLOAT ||
           type == TOKEN_TYPE_DOUBLE || type == TOKEN_TYPE_LONG;
}

bool isOperator(const TokenKind type) {
    return type == TOKEN_PLUS || type == TOKEN_MINUS || type == TOKEN_PERCENT || type == TOKEN_STAR ||
           type == TOKEN_SLASH || type == TOKEN_DOUBLE_STAR || isComparison(type) || type == TOKEN_AND ||
           type == TOKEN_OR;
}

FunctionNode *newFunctionNode(const char *name, Type *returnType, SourceLocation location) {
    FunctionNode *func = malloc(sizeof(FunctionNode));
    func->base.type = NODE_FUNCTION;
    func->base.location = location;
    func->base.dataType = returnType;
    if (strlen(name) >= sizeof(func->name)) diagnosticError(parserSource, location, "Function name is too long");
    strcpy(func->name, name);
    return func;
}

ReturnNode *newReturnNode(Node *value, SourceLocation location) {
    ReturnNode *node = malloc(sizeof(ReturnNode));
    node->base.type = NODE_RETURN;
    node->base.location = location;
    node->value = value;
    return node;
}

BreakNode *newBreakNode(SourceLocation location) {
    BreakNode *node = malloc(sizeof(BreakNode));
    node->base.type = NODE_BREAK;
    node->base.location = location;
    node->level = 1;
    return node;
}

ContinueNode *newContinueNode(SourceLocation location) {
    ContinueNode *node = malloc(sizeof(ContinueNode));
    node->base.type = NODE_CONTINUE;
    node->base.location = location;
    node->level = 1;
    return node;
}

IdentifierNode *newIdentifierNode(const char *name, Type *type, SourceLocation location) {
    IdentifierNode *node = malloc(sizeof(IdentifierNode));
    node->base.type = NODE_IDENTIFIER;
    node->base.location = location;
    node->base.dataType = type;
    if (strlen(name) >= sizeof(node->name)) diagnosticError(parserSource, location, "Identifier is too long");
    strcpy(node->name, name);
    return node;
}

AssignmentNode *newAssignmentNode(IdentifierNode *target, Node *value, SourceLocation location) {
    AssignmentNode *node = malloc(sizeof(AssignmentNode));
    node->base.type = NODE_ASSIGN;
    node->base.location = location;
    node->target = target;
    node->value = value;
    return node;
}

DeclarationNode *newDeclarationNode(IdentifierNode *target, Node *value, SourceLocation location) {
    DeclarationNode *node = malloc(sizeof(DeclarationNode));
    node->base.type = NODE_DECLARATION;
    node->base.location = location;
    node->target = target;
    node->value = value;
    return node;
}

ProgramNode *newProgramNode() {
    ProgramNode *node = malloc(sizeof(ProgramNode));
    node->base.type = NODE_PROGRAM;
    node->count = 0;
    node->capacity = 8;
    node->statements = malloc(sizeof(Node *) * node->capacity);
    return node;
}

void programAddStatement(ProgramNode *program, Node *statement) {
    if (program->count >= program->capacity) {
        program->capacity *= 2;

        program->statements = reallocSafe(program->statements, sizeof(Node *) * program->capacity);
    }

    program->statements[program->count++] = statement;
}

IfNode *newIfNode(Node *condition, Node *thenBranch, SourceLocation location) {
    IfNode *node = malloc(sizeof(IfNode));
    node->base.type = NODE_IF;
    node->base.location = location;
    node->condition = condition;
    node->thenBranch = thenBranch;
    node->elseBranch = NULL;
    return node;
}

CallNode *newCallNode(IdentifierNode *callee, const int argCount, Node **args, SourceLocation location) {
    CallNode *node = malloc(sizeof(CallNode));
    node->base.type = NODE_CALL;
    node->base.location = location;
    node->callee = callee;
    node->argumentCount = argCount;
    node->arguments = args;
    return node;
}

BooleanNode *newBooleanNode(bool value, SourceLocation location) {
    BooleanNode *node = malloc(sizeof(BooleanNode));
    node->base.type = NODE_BOOLEAN;
    node->base.location = location;
    node->base.dataType = boolType;
    node->value = value;
    return node;
}

StringNode *newStringNode(const char *value, SourceLocation location) {
    StringNode *node = malloc(sizeof(StringNode));
    node->base.type = NODE_STRING;
    node->base.location = location;
    node->base.dataType = stringType;
    node->value = strdup(value);
    return node;
}

NumberNode *newNumberNode(const char *value, SourceLocation location) {
    NumberNode *node = malloc(sizeof(NumberNode));
    node->base.type = NODE_NUMBER;
    node->base.location = location;

    size_t length = strlen(value);
    char suffix = value[length - 1];

    switch (suffix) {
        case 'L':
        case 'l':
            node->base.dataType = longType;
            node->i64 = strtoll(value, NULL, 10);
            break;

        case 'F':
        case 'f':
            node->base.dataType = floatType;
            node->f32 = strtof(value, NULL);
            break;

        case 'D':
        case 'd':
            node->base.dataType = doubleType;
            node->f64 = strtod(value, NULL);
            break;

        default:
            if (strchr(value, '.') != NULL) {
                node->base.dataType = doubleType;
                node->f64 = strtod(value, NULL);
            } else {
                node->base.dataType = intType;
                node->i32 = strtol(value, NULL, 10);
            }
            break;
    }

    return node;
}

BinaryNode *newBinaryNode(const TokenKind operator, Node *left, Node *right, SourceLocation location) {
    BinaryNode *node = malloc(sizeof(BinaryNode));
    node->base.type = NODE_BINARY;
    node->base.location = location;
    node->op = operator;
    node->left = left;
    node->right = right;
    return node;
}

ForNode *newForNode(SourceLocation location) {
    ForNode *node = mallocSafe(sizeof(ForNode));
    *node = (ForNode){.base = {NODE_FOR, voidType, location}};
    return node;
}

WhileNode *newWhileNode(Node *condition, Node *body, SourceLocation location) {
    WhileNode *node = malloc(sizeof(WhileNode));
    node->base.type = NODE_WHILE;
    node->base.location = location;
    node->condition = condition;
    node->body = body;
    return node;
}

CastNode *newCastNode(Type *type, Node *operand, SourceLocation location) {
    CastNode *node = mallocSafe(sizeof(CastNode));
    node->base = (Node){NODE_CAST, type, location};
    node->operand = operand;
    return node;
}

UnaryNode *newUnaryNode(TokenKind op, Node *operand, SourceLocation location) {
    UnaryNode *node = mallocSafe(sizeof(UnaryNode));
    node->base = (Node){NODE_UNARY, unknownType, location};
    node->op = op;
    node->operand = operand;
    node->postfix = false;
    return node;
}

Type *parseType() {
    Token token = advance();
    Type *type = token.type == TOKEN_IDENTIFIER ? findObjectType(token.value) : tokenTypeToType(token.type);
    if (type == unknownType) diagnosticError(parserSource, token.location, "Unknown type '%s'", token.value);
    if (matchCurrent(TOKEN_LBRACKET)) {
        expect(TOKEN_RBRACKET);
        type = toArrayType(type);
    }
    return type;
}

ArrayNode *newArrayNode(SourceLocation location) {
    ArrayNode *node = mallocSafe(sizeof(ArrayNode));
    *node = (ArrayNode){.base = {NODE_ARRAY, unknownType, location}};
    return node;
}

IndexNode *newIndexNode(Node *array, Node *index, SourceLocation location) {
    IndexNode *node = mallocSafe(sizeof(IndexNode));
    *node = (IndexNode){.base = {NODE_INDEX, unknownType, location}, .array = array, .index = index};
    return node;
}

LengthNode *newLengthNode(Node *array, SourceLocation location) {
    LengthNode *node = mallocSafe(sizeof(LengthNode));
    *node = (LengthNode){.base = {NODE_LENGTH, intType, location}, .array = array};
    return node;
}

ObjectType *newTypeNode(char *name, SourceLocation location) {
    ObjectType *node = mallocSafe(sizeof(ObjectType));
    *node = (ObjectType){.base = {NODE_TYPE, &node->type, location}, .name = name};
    node->type = (Type){.kind = TYPE_OBJECT, .object = node};
    return node;
}

ObjectNode *newObjectNode(Type *type, SourceLocation location) {
    ObjectNode *node = mallocSafe(sizeof(ObjectNode));
    *node = (ObjectNode){.base = {NODE_OBJECT, type, location}};
    return node;
}

MemberNode *newMemberNode(Node *object, char *name, SourceLocation location) {
    MemberNode *node = mallocSafe(sizeof(MemberNode));
    *node = (MemberNode){.base = {NODE_MEMBER, unknownType, location}, .object = object, .name = name};
    return node;
}

MethodCallNode *newMethodCallNode(MemberNode *member, int argumentCount, Node **arguments, SourceLocation location) {
    MethodCallNode *node = mallocSafe(sizeof(MethodCallNode));
    *node = (MethodCallNode){.base = {NODE_METHOD_CALL, unknownType, location},
                             .member = member,
                             .call = newCallNode(newIdentifierNode(member->name, functionType, location), argumentCount,
                                                 arguments, location)};
    return node;
}
