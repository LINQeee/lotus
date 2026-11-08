#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>

#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"
#include "parser.h"

bool isComparison(TokenKind type);

bool isAssignmentOp(TokenKind type);

bool isOperator(TokenKind type);

bool isDataType(TokenKind type);

LotusType tokenTypeToLotusType(TokenKind type);

FunctionNode *newFunctionNode(const char *name, LotusType returnType, SourceLocation location);

ReturnNode *newReturnNode(Node *value, SourceLocation location);

BreakNode *newBreakNode(SourceLocation location);

ContinueNode *newContinueNode(SourceLocation location);

IdentifierNode *newIdentifierNode(const char *name, LotusType type, SourceLocation location);

AssignmentNode *newAssignmentNode(IdentifierNode *target, Node *value, SourceLocation location);

DeclarationNode *newDeclarationNode(IdentifierNode *target, Node *value, SourceLocation location);

ProgramNode *newProgramNode();

IfNode *newIfNode(Node *condition, Node *thenBranch, SourceLocation location);

CallNode *newCallNode(IdentifierNode *callee, int argCount, Node **args, SourceLocation location);

StringNode *newStringNode(const char *value, SourceLocation location);

NumberNode *newNumberNode(const char *value, SourceLocation location);

BinaryNode *newBinaryNode(TokenKind operator, Node * left, Node *right, SourceLocation location);

ForNode *newForNode(SourceLocation location);

WhileNode *newWhileNode(Node *condition, Node *body, SourceLocation location);

BooleanNode *newBooleanNode(bool value, SourceLocation location);

CastNode *newCastNode(LotusType type, Node *operand, SourceLocation location);

UnaryNode *newUnaryNode(TokenKind op, Node *operand, SourceLocation location);

void programAddStatement(ProgramNode *program, Node *statement);
#endif
