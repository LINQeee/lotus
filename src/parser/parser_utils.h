#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>

bool isComparison(TokenType type);

bool isOperator(TokenType type);

bool isDataType(TokenType type);

LotusType tokenTypeToLotusType(TokenType type);

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

BinaryNode *newBinaryNode(TokenType operator, Node * left, Node *right, SourceLocation location);

WhileNode *newWhileNode(Node *condition, Node *body, SourceLocation location);

BooleanNode *newBooleanNode(bool value, SourceLocation location);

void programAddStatement(ProgramNode *program, Node *statement);
#endif
