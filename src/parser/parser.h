#ifndef PARSER_H
#define PARSER_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../lexer/lexer.h"
#include "../llvm/types.h"

#define MAX_ARGS 256

typedef enum {
    NODE_TYPE,
    NODE_OBJECT,
    NODE_MEMBER,
    NODE_METHOD_CALL,
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_BINARY,
    NODE_ASSIGN,
    NODE_PROGRAM,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_RETURN,
    NODE_STRING,
    NODE_CALL,
    NODE_FUNCTION,
    NODE_DECLARATION,
    NODE_BOOLEAN,
    NODE_CAST,
    NODE_UNARY,
    NODE_ARRAY,
    NODE_INDEX,
    NODE_LENGTH,
    NODE_INDEX_ASSIGN
} NodeType;

typedef struct {
    NodeType type;
    Type *dataType;
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
    TokenKind op;

    Node *left;
    Node *right;
} BinaryNode;

typedef struct {
    Node base;
    TokenKind op;

    Node *operand;
    bool postfix;
} UnaryNode;

typedef struct {
    Node base;
    Node *operand;
} CastNode;

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
    IdentifierNode *variable;
    Node *start;
    Node *end;
    Node *step;
    bool excludeStart;
    bool excludeEnd;
    Node *body;
} ForNode;

typedef struct {
    Node base;
    Node **items;
    size_t itemsCount;
    Node *size;
} ArrayNode;

typedef struct {
    Node base; // dataType is the array element type
    Node *array;
    Node *index;
    Node *value;
} IndexNode;

typedef struct {
    Node base;
    Node *array;
} LengthNode;

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

typedef struct {
    char *name;
    Type *type;
    Node *defaultValue;
    FunctionNode *initializer;
    SourceLocation location;
} ObjectField;

typedef enum {
    OBJECT_UNCHECKED,
    OBJECT_CHECKING,
    OBJECT_CHECKED
} ObjectCheckState;

struct ObjectType {
    Node base;
    Type type;
    char *name;
    ObjectField *fields;
    size_t fieldCount;
    FunctionNode **methods;
    size_t methodCount;
    bool defined;
    ObjectCheckState checkState;
};

typedef struct {
    Node base;
    char **names;
    Node **values;
    size_t *indices;
    size_t count;
} ObjectNode;

typedef struct {
    Node base;
    Node *object;
    char *name;
    size_t index;
    Node *value;
    TokenKind op;
} MemberNode;

typedef struct {
    Node base;
    MemberNode *member;
    CallNode *call;
} MethodCallNode;

Token current();
Token advance();
Token expect(TokenKind type);
bool isCurrent(TokenKind type);
bool matchCurrent(TokenKind type);
Token peekNext();
Token previous();

extern const char *parserSource;
ProgramNode *buildAST(const Token *tokensToParse, const char *source);

#endif
