#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/cutils.h"

static const char *source;
static size_t position;
static size_t line = 1;
static size_t column = 1;
static const char *currentFilePath;

typedef struct {
    const char *word;
    TokenType type;
} Keyword;

static const Keyword KEYWORDS[] = {
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"fn", TOKEN_FUNCTION},
    {"return", TOKEN_RETURN},
    {"continue", TOKEN_CONTINUE},
    {"next", TOKEN_CONTINUE},
    {"break", TOKEN_BREAK},
    {"int", TOKEN_TYPE_INT},
    {"long", TOKEN_TYPE_LONG},
    {"float", TOKEN_TYPE_FLOAT},
    {"double", TOKEN_TYPE_DOUBLE},
    {"bool", TOKEN_TYPE_BOOL},
    {"string", TOKEN_TYPE_STRING},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
};

static char current() {
    return source[position];
}
static char advance() {
    char c = source[position++];

    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }

    return c;
}
static bool match(const char *text) {
    const size_t length = strlen(text);
    return strncmp(source + position, text, length) == 0;
}
static void skipWhitespaces() {
    while (isspace((unsigned char) current())) advance();
}

static char *copySource(size_t start, size_t length) {
    char *value = mallocSafe(length + 1);

    memcpy(value, source+start, length);
    value[length] = '\0';

    return value;
}
static Token makeToken(TokenType type, int length) {
    Token token = {type, copySource(position, length)};

    for (size_t i = 0; i < length; i++)
        advance();

    return token;
}

static TokenType getKeywordType(const char *word) {
    for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++) {
        if (strcmp(word, KEYWORDS[i].word) == 0)
            return KEYWORDS[i].type;
    }

    return TOKEN_IDENTIFIER;
}
static Token readIdentifier() {
    size_t start = position;

    while (isalnum((unsigned char) current()) || current() == '_') advance();

    size_t length = position - start;

    char *value = copySource(start, length);

    Token token = {
        getKeywordType(value),
        value
    };
    return token;
}
static Token readNumber() {
    size_t start = position;

    while (isdigit((unsigned char) current())) advance();

    if (current() == '.') {
        advance();

        while (isdigit((unsigned char) current()))
            advance();
    }

    if (current() == 'L' || current() == 'l' ||
        current() == 'F' || current() == 'f' ||
        current() == 'D' || current() == 'd') {
        advance();
    }

    size_t length = position - start;

    return (Token){
        TOKEN_NUMBER,
        copySource(start, length)
    };
}
static Token readString(char endSymbol) {
    advance();

    size_t capacity = 16;
    size_t length = 0;

    char *value = mallocSafe(capacity);

    while (current() != endSymbol && current() != '\0') {
        char c = advance();

        if (c == '\\') {
            switch (current()) {
                case 'n': advance();
                    c = '\n';
                    break;
                case 't': advance();
                    c = '\t';
                    break;
                case '\\': advance();
                    c = '\\';
                    break;
                case '\'': if (endSymbol != '\'') break;
                    advance();
                    c = '\'';
                    break;
                case '"': if (endSymbol != '"') break;
                    advance();
                    c = '"';
                    break;
                default: break;
            }
        }

        if (length + 1 >= capacity) {
            capacity *= 2;
            value = reallocSafe(value, capacity);
        }

        value[length++] = c;
    }

    if (current() == endSymbol) advance();

    value[length] = '\0';

    return (Token){
        TOKEN_STRING,
        value
    };
}
static Token readSymbol() {
    if (match("==")) return makeToken(TOKEN_EQ, 2);
    if (match("!=")) return makeToken(TOKEN_NEQ, 2);
    if (match("<=")) return makeToken(TOKEN_LE, 2);
    if (match(">=")) return makeToken(TOKEN_GE, 2);

    switch (current()) {
        case '-': return makeToken(TOKEN_MINUS, 1);
        case '+': return makeToken(TOKEN_PLUS, 1);
        case '*': return makeToken(TOKEN_STAR, 1);
        case '/': return makeToken(TOKEN_SLASH, 1);
        case '%': return makeToken(TOKEN_PERCENT, 1);

        case '&': return makeToken(TOKEN_AND, 1);
        case '|': return makeToken(TOKEN_OR, 1);

        case '(': return makeToken(TOKEN_LPAREN, 1);
        case ')': return makeToken(TOKEN_RPAREN, 1);

        case '{': return makeToken(TOKEN_LBRACE, 1);
        case '}': return makeToken(TOKEN_RBRACE, 1);

        case '@': return makeToken(TOKEN_AT, 1);
        case ',': return makeToken(TOKEN_COMMA, 1);
        case '=': return makeToken(TOKEN_EQUAL, 1);
        case '>': return makeToken(TOKEN_GT, 1);
        case '<': return makeToken(TOKEN_LT, 1);
        case ':': return makeToken(TOKEN_COLON, 1);
    }
    exitWithError("Unknown symbol: %s", current());
}

static Token nextToken() {
    skipWhitespaces();

    SourceLocation location = {currentFilePath, line, column, position};

    char c = current();

    if (c == '\0') return (Token){TOKEN_EOF, .location = location};

    Token token;

    if (isalpha((unsigned char) c) || c == '_')
        token = readIdentifier();
    else if (isdigit((unsigned char) c))
        token = readNumber();
    else if (c == '"' || c == '\'')
        token = readString(c);
    else token = readSymbol();

    token.location = location;
    return token;
}

Token *tokenize(const char *code, const char *fileName) {
    size_t capacity = 64;
    Token *tokens = mallocSafe(sizeof(Token) * capacity);

    source = code;
    position = 0;
    line = 1;
    column = 1;
    currentFilePath = fileName;

    Token t;
    size_t pos = 0;
    while ((t = nextToken()).type != TOKEN_EOF) {
        if (pos >= capacity) {
            capacity *= 2;
            Token *tmp = reallocSafe(tokens, sizeof(Token) * capacity);
            tokens = tmp;
        }

        tokens[pos++] = t;
        printf("ADDED TOKEN {type: %d, value: \"%s\"}\n", t.type, t.value);
    }
    tokens[pos] = (Token){TOKEN_EOF, .location = {currentFilePath, line, column, position}};
    return tokens;
}
