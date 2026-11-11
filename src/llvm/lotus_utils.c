#include "../lexer/lexer.h"
#include "./lotus_types.h"
#include "../utils/cutils.h"

#include <stdbool.h>

const char *lotusTypeName(LotusType type) {
    switch (type) {
        case LOTUS_I32: return "int";

        case LOTUS_I64: return "long";

        case LOTUS_F32: return "float";

        case LOTUS_F64: return "double";

        case LOTUS_BOOL: return "bool";

        case LOTUS_STRING: return "string";

        case LOTUS_VOID: return "void";

        case LOTUS_ANY: return "any";

        case LOTUS_FUNCTION: return "function";
        case LOTUS_I32_ARRAY: return "int[]";
        case LOTUS_I64_ARRAY: return "long[]";
        case LOTUS_F32_ARRAY: return "float[]";
        case LOTUS_F64_ARRAY: return "double[]";
        case LOTUS_BOOL_ARRAY: return "bool[]";
        case LOTUS_STRING_ARRAY: return "string[]";
    }

    return "<unknown>";
}

const char *tokenTypeName(TokenKind type) {
    switch (type) {
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_INCREMENT: return "++";
        case TOKEN_DECREMENT: return "--";
        case TOKEN_EXCLAMATION: return "!";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_PERCENT: return "%";
        case TOKEN_DOUBLE_STAR: return "**";

        case TOKEN_EQ: return "==";
        case TOKEN_NEQ: return "!=";
        case TOKEN_GT: return ">";
        case TOKEN_LT: return "<";
        case TOKEN_GE: return ">=";
        case TOKEN_LE: return "<=";

        case TOKEN_AND: return "&&";
        case TOKEN_OR: return "||";

        case TOKEN_EQUAL: return "=";
        case TOKEN_EQ_PLUS: return "+=";
        case TOKEN_EQ_MINUS: return "-=";
        case TOKEN_EQ_STAR: return "*=";
        case TOKEN_EQ_SLASH: return "/=";
        case TOKEN_EQ_PERCENT: return "%=";

        default: return "<operator>";
    }
}

NumericKind getNumericKind(LotusType type) {
    if (type == LOTUS_I32 || type == LOTUS_I64) return NUMERIC_INTEGER;
    if (type == LOTUS_F32 || type == LOTUS_F64) return NUMERIC_FLOAT;
    exitWithError("Unknown numeric kind of type: %d", type);
}

bool isNumericType(LotusType type) {
    return type == LOTUS_I32 || type == LOTUS_I64 || type == LOTUS_F32 || type == LOTUS_F64;
}

bool isIntegerType(LotusType type) {
    return type == LOTUS_I32 || type == LOTUS_I64;
}

bool isBooleanType(LotusType type) {
    return type == LOTUS_BOOL;
}

bool isArrayType(LotusType type) {
    switch (type) {
        case LOTUS_I32_ARRAY:
        case LOTUS_I64_ARRAY:
        case LOTUS_F32_ARRAY:
        case LOTUS_F64_ARRAY:
        case LOTUS_BOOL_ARRAY:
        case LOTUS_STRING_ARRAY: return true;
        default: return false;
    }
}

bool isMemoryType(LotusType type) {
    return type == LOTUS_STRING || isArrayType(type);
}

LotusType fromArrayType(LotusType type) {
    switch (type) {
        case LOTUS_I32_ARRAY: return LOTUS_I32;
        case LOTUS_I64_ARRAY: return LOTUS_I64;
        case LOTUS_F32_ARRAY: return LOTUS_F32;
        case LOTUS_F64_ARRAY: return LOTUS_F64;
        case LOTUS_BOOL_ARRAY: return LOTUS_BOOL;
        case LOTUS_STRING_ARRAY: return LOTUS_STRING;
        default: exitWithError("Expected array type, got %s", lotusTypeName(type));
    }
}

LotusType toArrayType(LotusType type) {
    switch (type) {
        case LOTUS_I32: return LOTUS_I32_ARRAY;
        case LOTUS_I64: return LOTUS_I64_ARRAY;
        case LOTUS_F32: return LOTUS_F32_ARRAY;
        case LOTUS_F64: return LOTUS_F64_ARRAY;
        case LOTUS_BOOL: return LOTUS_BOOL_ARRAY;
        case LOTUS_STRING: return LOTUS_STRING_ARRAY;
        default: exitWithError("Expected array element type, got %s", lotusTypeName(type));
    }
}
