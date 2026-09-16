#ifndef PARSER_H
#define PARSER_H
#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_ARGS 256

typedef enum {
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_BINARY,
    NODE_ASSIGN,
    NODE_PROGRAM,
    NODE_IF,
    NODE_WHILE,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_RETURN,
    NODE_STRING,
    NODE_CALL,
    NODE_FUNCTION,
    NODE_DECLARATION,
    NODE_BOOLEAN
} NodeType;

typedef struct {
    NodeType type;
    LotusType dataType;
    SourceLocation location;
} Node;

typedef struct {
    Node base;

    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };
} NumberNode;

typedef struct {
    Node base;
    char name[64];
} IdentifierNode;

typedef struct {
    Node base;
    TokenType op;

    Node *left;
    Node *right;
} BinaryNode;

typedef struct {
    Node base;
    TokenType op;

    Node *operand;
} UnaryNode;

typedef struct {
    Node base;

    IdentifierNode *target;
    Node *value;
} AssignmentNode;

typedef struct {
    Node base;

    IdentifierNode *target;
    Node *value;
} DeclarationNode;

typedef struct {
    Node base;
    size_t count;
    size_t capacity;
    Node **statements;
} ProgramNode;

typedef struct {
    Node base;

    Node *condition;
    Node *thenBranch;
    Node *elseBranch;
} IfNode;

typedef struct {
    Node base;

    Node *condition;
    Node *body;
} WhileNode;

typedef struct {
    Node base;
    int level;
} BreakNode;

typedef struct {
    Node base;
    int level;
} ContinueNode;

typedef struct {
    Node base;
    Node *value;
} ReturnNode;

typedef struct {
    Node base;
    char *value;
} StringNode;

typedef struct {
    Node base;
    bool value;
} BooleanNode;

typedef struct {
    Node base;
    char name[64];
    Node *body;
    IdentifierNode **params;
    int paramCount;
} FunctionNode;

typedef struct {
    Node base;
    IdentifierNode *callee;
    Node **arguments;
    int argumentCount;
} CallNode;

Token current();
Token advance();
Token expect(TokenType type);
bool isCurrent(TokenType type);
bool matchCurrent(TokenType type);
Token peekNext();

ProgramNode *buildAST(const Token *tokensToParse);

#endif
