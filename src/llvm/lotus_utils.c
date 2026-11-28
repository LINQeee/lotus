#include <stdbool.h>

#include "types.h"
#include "../utils/cutils.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"

static Type intTypeValue = {.kind = TYPE_I32};
Type *intType = &intTypeValue;
static Type longTypeValue = {.kind = TYPE_I64};
Type *longType = &longTypeValue;
static Type floatTypeValue = {.kind = TYPE_F32};
Type *floatType = &floatTypeValue;
static Type doubleTypeValue = {.kind = TYPE_F64};
Type *doubleType = &doubleTypeValue;
static Type boolTypeValue = {.kind = TYPE_BOOL};
Type *boolType = &boolTypeValue;
static Type stringTypeValue = {.kind = TYPE_STRING};
Type *stringType = &stringTypeValue;
static Type voidTypeValue = {.kind = TYPE_VOID};
Type *voidType = &voidTypeValue;
static Type anyTypeValue = {.kind = TYPE_ANY};
Type *anyType = &anyTypeValue;
static Type functionTypeValue = {.kind = TYPE_FUNCTION};
Type *functionType = &functionTypeValue;
static Type unknownTypeValue = {.kind = TYPE_UNKNOWN};
Type *unknownType = &unknownTypeValue;
static Type anyArrayTypeValue = {.kind = TYPE_ANY_ARRAY};
Type *anyArrayType = &anyArrayTypeValue;

static Type arrays[] = {
    {.kind = TYPE_ARRAY, .elementType = &intTypeValue},   {.kind = TYPE_ARRAY, .elementType = &longTypeValue},
    {.kind = TYPE_ARRAY, .elementType = &floatTypeValue}, {.kind = TYPE_ARRAY, .elementType = &doubleTypeValue},
    {.kind = TYPE_ARRAY, .elementType = &boolTypeValue},  {.kind = TYPE_ARRAY, .elementType = &stringTypeValue},
};

bool typesEqual(Type *left, Type *right) {
    if (left == right) return true;
    if (left->kind != right->kind) return false;
    if (left->kind == TYPE_ARRAY) return typesEqual(left->elementType, right->elementType);
    if (left->kind == TYPE_OBJECT) return left->object == right->object;
    return true;
}
const char *typeName(Type *type) {
    if (type->kind == TYPE_OBJECT) return type->object->name;
    if (type->kind == TYPE_ARRAY) {
        switch (type->elementType->kind) {
            case TYPE_I32: return "int[]";
            case TYPE_I64: return "long[]";
            case TYPE_F32: return "float[]";
            case TYPE_F64: return "double[]";
            case TYPE_BOOL: return "bool[]";
            case TYPE_STRING: return "string[]";
            default: return "array";
        }
    }
    switch (type->kind) {
        case TYPE_I32: return "int";
        case TYPE_I64: return "long";
        case TYPE_F32: return "float";
        case TYPE_F64: return "double";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_ANY: return "any";
        case TYPE_FUNCTION: return "function";
        case TYPE_UNKNOWN: return "unknown";
        default: return "array";
    }
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

bool isNumericType(Type *type) {
    return type->kind == TYPE_I32 || type->kind == TYPE_I64 || type->kind == TYPE_F32 || type->kind == TYPE_F64;
}
bool isIntegerType(Type *type) {
    return type->kind == TYPE_I32 || type->kind == TYPE_I64;
}
bool isBooleanType(Type *type) {
    return type->kind == TYPE_BOOL;
}
bool isArrayType(Type *type) {
    return type->kind == TYPE_ARRAY;
}
bool isMemoryType(Type *type) {
    return type->kind == TYPE_STRING || type->kind == TYPE_ARRAY || type->kind == TYPE_ANY_ARRAY ||
           type->kind == TYPE_OBJECT;
}
NumericKind getNumericKind(Type *type) {
    if (isIntegerType(type)) return NUMERIC_INTEGER;
    if (type->kind == TYPE_F32 || type->kind == TYPE_F64) return NUMERIC_FLOAT;
    exitWithError("Expected numeric type, got %s", typeName(type));
}
Type *fromArrayType(Type *type) {
    return type->elementType;
}
Type *toArrayType(Type *type) {
    for (unsigned i = 0; i < sizeof(arrays) / sizeof(arrays[0]); i++)
        if (typesEqual(arrays[i].elementType, type)) return &arrays[i];
    exitWithError("Expected array element type, got %s", typeName(type));
}
