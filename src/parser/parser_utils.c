
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lexer/lexer.h"
#include "parser.h"
#include "../utils/cutils.h"

bool isComparison(const TokenType type) {
    return type == TOKEN_GT || type == TOKEN_LT ||
           type == TOKEN_GE || type == TOKEN_LE ||
           type == TOKEN_EQ || type == TOKEN_NEQ;
}

LotusType tokenTypeToLotusType(TokenType type) {
    switch (type) {
        case TOKEN_TYPE_INT: return LOTUS_I32;
        case TOKEN_TYPE_FLOAT: return LOTUS_F32;
        case TOKEN_TYPE_BOOL: return LOTUS_BOOL;
        case TOKEN_TYPE_LONG: return LOTUS_I64;
        case TOKEN_TYPE_DOUBLE: return LOTUS_F64;
        case TOKEN_TYPE_STRING: return LOTUS_STRING;
        default:
            printf("Unknown LOTUS DATATYPE %d\n", type);
            exit(EXIT_FAILURE);
    }
}

bool isDataType(TokenType type) {
    return type == TOKEN_TYPE_INT || type == TOKEN_TYPE_STRING ||
           type == TOKEN_TYPE_BOOL || type == TOKEN_TYPE_FLOAT ||
           type == TOKEN_TYPE_DOUBLE || type == TOKEN_TYPE_LONG;
}

bool isOperator(const TokenType type) {
    return type == TOKEN_PLUS || type == TOKEN_MINUS || type == TOKEN_PERCENT || type == TOKEN_STAR || type ==
           TOKEN_SLASH || isComparison(type)
           || type == TOKEN_AND || type == TOKEN_OR;
}

FunctionNode *newFunctionNode(const char *name, LotusType returnType, SourceLocation location) {
    FunctionNode *func = malloc(sizeof(FunctionNode));
    func->base.type = NODE_FUNCTION;
    func->base.location = location;
    func->base.dataType = returnType;
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

IdentifierNode *newIdentifierNode(const char *name, LotusType type, SourceLocation location) {
    IdentifierNode *node = malloc(sizeof(IdentifierNode));
    node->base.type = NODE_IDENTIFIER;
    node->base.location = location;
    node->base.dataType = type;
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
    node->statements = malloc(
        sizeof(Node *) * node->capacity
    );
    return node;
}

void programAddStatement(
    ProgramNode *program,
    Node *statement
) {
    if (program->count >= program->capacity) {
        program->capacity *= 2;

        program->statements = reallocSafe(
            program->statements,
            sizeof(Node *) * program->capacity
        );
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
    node->value = value;
    return node;
}

StringNode *newStringNode(const char *value, SourceLocation location) {
    StringNode *node = malloc(sizeof(StringNode));
    node->base.type = NODE_STRING;
    node->base.location = location;
    node->base.dataType = LOTUS_STRING;
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
            node->base.dataType = LOTUS_I64;
            node->i64 = strtoll(value, NULL, 10);
            break;

        case 'F':
        case 'f':
            node->base.dataType = LOTUS_F32;
            node->f32 = strtof(value, NULL);
            break;

        case 'D':
        case 'd':
            node->base.dataType = LOTUS_F64;
            node->f64 = strtod(value, NULL);
            break;

        default:
            if (strchr(value, '.') != NULL) {
                node->base.dataType = LOTUS_F64;
                node->f64 = strtod(value, NULL);
            } else {
                node->base.dataType = LOTUS_I32;
                node->i32 = strtol(value, NULL, 10);
            }
            break;
    }

    return node;
}

BinaryNode *newBinaryNode(const TokenType operator, Node *left, Node *right, SourceLocation location) {
    BinaryNode *node = malloc(sizeof(BinaryNode));
    node->base.type = NODE_BINARY;
    node->base.location = location;
    node->op = operator;
    node->left = left;
    node->right = right;
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
