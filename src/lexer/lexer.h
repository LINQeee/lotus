#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GE,
    TOKEN_LE,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_FUNCTION,
    TOKEN_AT,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_PERCENT,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_LONG,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_DOUBLE,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_COLON,
    TOKEN_EQ_PLUS,
    TOKEN_EQ_MINUS,
    TOKEN_EQ_STAR,
    TOKEN_EQ_SLASH,
    TOKEN_EQ_PERCENT
} TokenType;

typedef struct {
    const char *file;
    size_t line;
    size_t column;
    size_t length;
} SourceLocation;

typedef struct {
    TokenType type;
    char *value;

    SourceLocation location;
} Token;

Token *tokenize(const char *code, const char *fileName);

#endif
