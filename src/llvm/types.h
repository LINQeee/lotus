#ifndef TYPES_H
#define TYPES_H
#include "../lexer/lexer.h"
#include <stdbool.h>

typedef enum {
    TYPE_I32,
    TYPE_I64,
    TYPE_F32,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_ANY,
    TYPE_FUNCTION,
    TYPE_UNKNOWN,
    TYPE_ARRAY,
    TYPE_OBJECT,
    TYPE_ANY_ARRAY
} TypeKind;

typedef struct Type Type;
typedef struct ObjectType ObjectType;
struct Type {
    TypeKind kind;
    union {
        Type *elementType;
        ObjectType *object;
    };
};

extern Type *intType;
extern Type *longType;
extern Type *floatType;
extern Type *doubleType;
extern Type *boolType;
extern Type *stringType;
extern Type *voidType;
extern Type *anyType;
extern Type *functionType;
extern Type *unknownType;
extern Type *anyArrayType;

typedef enum {
    NUMERIC_INTEGER,
    NUMERIC_FLOAT
} NumericKind;
typedef struct Variable {
    char *name;
    Type *type;
} Variable;

const char *typeName(Type *type);
const char *tokenTypeName(TokenKind type);
NumericKind getNumericKind(Type *type);
bool typesEqual(Type *left, Type *right);
bool isNumericType(Type *type);
bool isIntegerType(Type *type);
bool isBooleanType(Type *type);
bool isArrayType(Type *type);
bool isMemoryType(Type *type);
Type *fromArrayType(Type *type);
Type *toArrayType(Type *type);
#endif
