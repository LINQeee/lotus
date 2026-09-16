#ifndef LOTUS_TYPES_H
#define LOTUS_TYPES_H

#include "../lexer/lexer.h"
#include <stdbool.h>

typedef enum {
    LOTUS_I32,
    LOTUS_I64,
    LOTUS_F32,
    LOTUS_F64,
    LOTUS_BOOL,
    LOTUS_STRING,
    LOTUS_VOID,
    LOTUS_ANY,
    LOTUS_FUNCTION,
    LOTUS_UNKNOWN
} LotusType;

typedef struct Variable {
    char *name;
    LotusType type;
} Variable;

const char *lotusTypeName(LotusType type);

const char *tokenTypeName(TokenType type);

bool isNumericType(LotusType type);

bool isIntegerType(LotusType type);

bool isBooleanType(LotusType type);
#endif
